// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "SceneObjectsFactory.h"
#include "Engine/Level/Actor.h"
#include "Engine/Level/Prefabs/Prefab.h"
#include "Engine/Content/Content.h"
#include "Engine/Core/Cache.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/LogContext.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Serialization/ISerializeModifier.h"
#include "Engine/Serialization/SerializationFwd.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Threading/ThreadLocal.h"
#if !BUILD_RELEASE || USE_EDITOR
#include "Engine/Level/Level.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Level/Scripts/MissingScript.h"
#endif
#include "Engine/Level/Scripts/ModelPrefab.h"

#if USE_EDITOR

MissingScript::MissingScript(const SpawnParams& params)
    : Script(params)
{
}

void MissingScript::SetReferenceScript(const ScriptingObjectReference<Script>& value)
{
    if (_referenceScript == value)
        return;
    _referenceScript = value;
    if (Data.IsEmpty() || !_referenceScript)
        return;
    rapidjson_flax::Document document;
    document.Parse(Data.ToStringAnsi().GetText());
    document.RemoveMember("ParentID"); // Prevent changing parent
    auto modifier = Cache::ISerializeModifier.Get();
    const auto idsMapping = Scripting::ObjectsLookupIdMapping.Get();
    if (idsMapping)
        modifier->IdsMapping = *idsMapping;
    _referenceScript->Deserialize(document, modifier.Value);
    DeleteObject();
}

#endif

ModelPrefab::ModelPrefab(const SpawnParams& params)
    : Script(params)
{
}

SceneObjectsFactory::Context::Context(ISerializeModifier* modifier)
    : Modifier(modifier)
{
    // Override the main thread value to not create it again in GetModifier() if called from the same thread
    Modifiers.Set(modifier);
}

SceneObjectsFactory::Context::~Context()
{
    if (Async)
    {
        Array<ISerializeModifier*, InlinedAllocation<PLATFORM_THREADS_LIMIT>> modifiers;
        Modifiers.GetValues(modifiers);
        for (ISerializeModifier* e : modifiers)
        {
            if (e == Modifier)
                continue;
            Cache::ISerializeModifier.Put(e);
        }
    }
}

ISerializeModifier* SceneObjectsFactory::Context::GetModifier()
{
    ISerializeModifier* modifier = Modifier;
    if (Async)
    {
        // When using context in async then use one ISerializeModifier per-thread
        ISerializeModifier*& modifierThread = Modifiers.Get();
        if (!modifierThread)
        {
            modifierThread = Cache::ISerializeModifier.GetUnscoped();
            Modifiers.Set(modifierThread);
            Locker.Lock();
            modifierThread->EngineBuild = modifier->EngineBuild;
            modifierThread->CurrentInstance = modifier->CurrentInstance;
            modifierThread->IdsMapping = modifier->IdsMapping;
            Locker.Unlock();
        }
        modifier = modifierThread;
    }
    return modifier;
}

void SceneObjectsFactory::Context::SetupIdsMapping(const SceneObject* obj, ISerializeModifier* modifier) const
{
    int32 instanceIndex;
    if (ObjectToInstance.TryGet(obj->GetID(), instanceIndex) && instanceIndex != modifier->CurrentInstance)
    {
        // Apply the current prefab instance objects ids table to resolve references inside a prefab properly
        modifier->CurrentInstance = instanceIndex;
        const auto& instance = Instances[instanceIndex];
        for (const auto& e : instance.IdsMapping)
            modifier->IdsMapping[e.Key] = e.Value;
    }
}

SceneObject* SceneObjectsFactory::Spawn(Context& context, const ISerializable::DeserializeStream& stream)
{
    // Get object id
    Guid id = JsonTools::GetGuid(stream, "ID");
    ISerializeModifier* modifier = context.GetModifier();
    modifier->IdsMapping.TryGet(id, id);
    if (!id.IsValid())
    {
        LOG(Warning, "Invalid object id.");
        return nullptr;
    }
    SceneObject* obj = nullptr;

    // Check for prefab instance
    Guid prefabObjectId;
    if (JsonTools::GetGuidIfValid(prefabObjectId, stream, "PrefabObjectID"))
    {
        // Get prefab asset id
        const Guid prefabId = JsonTools::GetGuid(stream, "PrefabID");
        if (!prefabId.IsValid())
        {
            LOG(Warning, "Invalid prefab id.");
            return nullptr;
        }

        // Load prefab
        auto prefab = Content::LoadAsync<Prefab>(prefabId);
        if (prefab == nullptr)
        {
            LOG(Warning, "Missing prefab {0}.", prefabId);
            return nullptr;
        }
        if (prefab->WaitForLoaded())
        {
            LOG(Warning, "Failed to load prefab {0}.", prefab->ToString());
            return nullptr;
        }

        // Get prefab object data from the prefab
        const ISerializable::DeserializeStream* prefabData;
        if (!prefab->ObjectsDataCache.TryGet(prefabObjectId, prefabData))
        {
            LOG(Warning, "Missing object {1} data in prefab {0}.", prefab->ToString(), prefabObjectId);
            return nullptr;
        }

        // Map prefab object ID to the deserialized instance ID
        modifier->IdsMapping[prefabObjectId] = id;

        // Create prefab instance (recursive prefab loading to support nested prefabs)
        obj = Spawn(context, *prefabData);
    }
    else
    {
        const auto typeNameMember = stream.FindMember("TypeName");
        if (typeNameMember != stream.MemberEnd())
        {
            if (!typeNameMember->value.IsString())
            {
                LOG(Warning, "Invalid object type (TypeName must be an object type full name string).");
                return nullptr;
            }
            const StringAnsiView typeName(typeNameMember->value.GetStringAnsiView());
            const ScriptingTypeHandle type = Scripting::FindScriptingType(typeName);
            if (type)
            {
                // TODO: cache per-type result in Context to boost loading of the large scenes
                if (!SceneObject::TypeInitializer.IsAssignableFrom(type))
                {
                    LOG(Warning, "Invalid scene object type {0} (inherits from: {1}).", type.ToString(true), type.GetType().GetBaseType().ToString());
                    return nullptr;
                }
                const ScriptingObjectSpawnParams params(id, type);
                obj = (SceneObject*)type.GetType().Script.Spawn(params);
                if (obj == nullptr)
                {
                    LOG(Warning, "Failed to spawn object of type {0}.", type.ToString(true));
                    return nullptr;
                }
            }
            else
            {
                LOG(Warning, "Unknown object type '{0}', ID: {1}", String(typeName.Get(), typeName.Length()), id.ToString());
                return nullptr;
            }
        }
        else
        {
            // [Deprecated: 18.07.2019 expires 18.07.2020]
            const auto typeIdMember = stream.FindMember("TypeID");
            if (typeIdMember == stream.MemberEnd())
            {
                LOG(Warning, "Missing object type.");
                return nullptr;
            }
            if (typeIdMember->value.IsString())
            {
                // Script
                const char* scriptTypeName = typeIdMember->value.GetString();
                const ScriptingTypeHandle type = Scripting::FindScriptingType(scriptTypeName);
                if (type)
                {
                    const ScriptingObjectSpawnParams params(id, type);
                    obj = (SceneObject*)type.GetType().Script.Spawn(params);
                    if (obj == nullptr)
                    {
                        LOG(Warning, "Failed to spawn object of type {0}.", type.ToString(true));
                        return nullptr;
                    }
                }
                else
                {
                    LOG(Warning, "Failed to create script. Invalid type name '{0}'.", String(scriptTypeName));
                    return nullptr;
                }
            }
            else if (typeIdMember->value.IsInt())
            {
                // Actor
                const int32 typeId = typeIdMember->value.GetInt();
                obj = CreateActor(typeId, id);
            }
            else
            {
                LOG(Warning, "Invalid object type.");
            }
        }
    }

    return obj;
}

void SceneObjectsFactory::Deserialize(Context& context, SceneObject* obj, ISerializable::DeserializeStream& stream)
{
    CHECK_DEBUG(obj);
    ISerializeModifier* modifier = context.GetModifier();
    LogContextScope logContext(obj->GetID());

    // Check for prefab instance
    Guid prefabObjectId;
    if (JsonTools::GetGuidIfValid(prefabObjectId, stream, "PrefabObjectID"))
    {
        // Get prefab asset id
        const Guid prefabId = JsonTools::GetGuid(stream, "PrefabID");
        if (!prefabId.IsValid())
        {
            LOG(Warning, "Invalid prefab id.");
            return;
        }

        // Load prefab
        auto prefab = Content::LoadAsync<Prefab>(prefabId);
        if (prefab == nullptr)
        {
            LOG(Warning, "Missing prefab {0}.", prefabId);
            return;
        }
        if (prefab->WaitForLoaded())
        {
            LOG(Warning, "Failed to load prefab {0}.", prefab->ToString());
            return;
        }

        // Get prefab object data from the prefab
        const ISerializable::DeserializeStream* prefabData;
        if (!prefab->ObjectsDataCache.TryGet(prefabObjectId, prefabData))
        {
            LOG(Warning, "Missing object {1} data in prefab {0}.", prefab->ToString(), prefabObjectId);
            return;
        }

        // Deserialize prefab data (recursive prefab loading to support nested prefabs)
        const auto prevVersion = modifier->EngineBuild;
        modifier->EngineBuild = prefab->DataEngineBuild;
        Deserialize(context, obj, *(ISerializable::DeserializeStream*)prefabData);
        modifier->EngineBuild = prevVersion;
    }

    context.SetupIdsMapping(obj, modifier);

    // Load data
    obj->Deserialize(stream, modifier);
}

void SceneObjectsFactory::HandleObjectDeserializationError(const ISerializable::DeserializeStream& value)
{
#if !BUILD_RELEASE || USE_EDITOR
    // Prevent race-conditions when logging missing objects (especially when adding dummy MissingScript)
    ScopeLock lock(Level::ScenesLock);

    // Print invalid object data contents
    rapidjson_flax::StringBuffer buffer;
    PrettyJsonWriter writer(buffer);
    value.Accept(writer.GetWriter());
    String bufferStr(buffer.GetString());
    LOG(Warning, "Failed to deserialize scene object from data: {0}", bufferStr);

    // Try to log some useful info about missing object (eg. it's parent name for faster fixing)
    const auto parentIdMember = value.FindMember("ParentID");
    if (parentIdMember != value.MemberEnd() && parentIdMember->value.IsString())
    {
        const Guid parentId = JsonTools::GetGuid(parentIdMember->value);
        Actor* parent = Scripting::FindObject<Actor>(parentId);
        if (parent)
        {
#if USE_EDITOR
            // Add dummy script
            const auto typeNameMember = value.FindMember("TypeName");
            if (typeNameMember != value.MemberEnd() && typeNameMember->value.IsString())
            {
                auto* dummyScript = parent->AddScript<MissingScript>();
                dummyScript->MissingTypeName = typeNameMember->value.GetString();
                dummyScript->Data = MoveTemp(bufferStr);
            }
#endif
            LOG(Warning, "Parent actor of the missing object: '{0}' ({1})", parent->GetNamePath(), String(parent->GetType().Fullname));
        }
    }
#endif
}

Actor* SceneObjectsFactory::CreateActor(int32 typeId, const Guid& id)
{
    // [Deprecated: 18.07.2019 expires 18.07.2020]

    // Convert deprecated typeId into actor type name
    StringAnsiView typeName;
    switch (typeId)
    {
#define CASE(type, typeId) case typeId: typeName = type; break;
    CASE("FlaxEngine.StaticModel", 1);
    CASE("FlaxEngine.Camera", 2);
    CASE("FlaxEngine.EmptyActor", 3);
    CASE("FlaxEngine.DirectionalLight", 4);
    CASE("FlaxEngine.PointLight", 5);
    CASE("FlaxEngine.Skybox", 6);
    CASE("FlaxEngine.EnvironmentProbe", 7);
    CASE("FlaxEngine.BoxBrush", 8);
    CASE("FlaxEngine.Scene", 9);
    CASE("FlaxEngine.Sky", 10);
    CASE("FlaxEngine.RigidBody", 11);
    CASE("FlaxEngine.SpotLight", 12);
    CASE("FlaxEngine.PostFxVolume", 13);
    CASE("FlaxEngine.BoxCollider", 14);
    CASE("FlaxEngine.SphereCollider", 15);
    CASE("FlaxEngine.CapsuleCollider", 16);
    CASE("FlaxEngine.CharacterController", 17);
    CASE("FlaxEngine.FixedJoint", 18);
    CASE("FlaxEngine.DistanceJoint", 19);
    CASE("FlaxEngine.HingeJoint", 20);
    CASE("FlaxEngine.SliderJoint", 21);
    CASE("FlaxEngine.SphericalJoint", 22);
    CASE("FlaxEngine.D6Joint", 23);
    CASE("FlaxEngine.MeshCollider", 24);
    CASE("FlaxEngine.SkyLight", 25);
    CASE("FlaxEngine.ExponentialHeightFog", 26);
    CASE("FlaxEngine.TextRender", 27);
    CASE("FlaxEngine.AudioSource", 28);
    CASE("FlaxEngine.AudioListener", 29);
    CASE("FlaxEngine.AnimatedModel", 30);
    CASE("FlaxEngine.BoneSocket", 31);
    CASE("FlaxEngine.Decal", 32);
    CASE("FlaxEngine.UICanvas", 33);
    CASE("FlaxEngine.UIControl", 34);
    CASE("FlaxEngine.Terrain", 35);
    CASE("FlaxEngine.Foliage", 36);
    CASE("FlaxEngine.NavMeshBoundsVolume", 37);
    CASE("FlaxEngine.NavLink", 38);
    CASE("FlaxEngine.ParticleEffect", 39);
#undef CASE
    default:
        LOG(Warning, "Unknown actor type id \'{0}\'", typeId);
        return nullptr;
    }

    const ScriptingTypeHandle type = Scripting::FindScriptingType(typeName);
    if (type)
    {
        const ScriptingObjectSpawnParams params(id, type);
        const auto result = dynamic_cast<Actor*>(type.GetType().Script.Spawn(params));
        if (result == nullptr)
        {
            LOG(Warning, "Failed to spawn object of type {0}.", type.ToString(true));
            return nullptr;
        }
        return result;
    }

    LOG(Warning, "Unknown actor type \'{0}\'", String(typeName.Get(), typeName.Length()));
    return nullptr;
}

SceneObjectsFactory::PrefabSyncData::PrefabSyncData(Array<SceneObject*>& sceneObjects, const ISerializable::DeserializeStream& data, ISerializeModifier* modifier)
    : SceneObjects(sceneObjects)
    , Data(data)
    , Modifier(modifier)
    , InitialCount(0)
{
}

void SceneObjectsFactory::PrefabSyncData::InitNewObjects()
{
    for (int32 i = 0; i < NewObjects.Count(); i++)
    {
        SceneObject* obj = SceneObjects[InitialCount + i];
        obj->Initialize();
    }
}

void SceneObjectsFactory::SetupPrefabInstances(Context& context, const PrefabSyncData& data)
{
    PROFILE_CPU_NAMED("SetupPrefabInstances");
    const int32 count = data.Data.Size();
    ASSERT(count <= data.SceneObjects.Count());
    Dictionary<Guid, Guid> parentIdsLookup;
    for (int32 i = 0; i < count; i++)
    {
        const auto& stream = data.Data[i];
        Guid prefabObjectId, prefabId;
        if (!JsonTools::GetGuidIfValid(prefabObjectId, stream, "PrefabObjectID"))
            continue;
        if (!JsonTools::GetGuidIfValid(prefabId, stream, "PrefabID"))
            continue;
        Guid parentId = JsonTools::GetGuid(stream, "ParentID");
        if (!parentIdsLookup.TryGet(parentId, parentId))
        {
            Guid parentIdKep = parentId;
            for (int32 j = i - 1; j >= 0; j--)
            {
                // Find ID of the parent to this object (use data in json for relationship)
                if (parentId == JsonTools::GetGuid(data.Data[j], "ID") && data.SceneObjects[j])
                {
                    parentId = data.SceneObjects[j]->GetID();
                    break;
                }
            }
            parentIdsLookup.Add(parentIdKep, parentId);
        }
        const SceneObject* obj = data.SceneObjects[i];
        const Guid id = obj ? obj->GetID() : JsonTools::GetGuid(stream, "ID");
        auto prefab = Content::LoadAsync<Prefab>(prefabId);
        if (!prefab)
            continue;

        // Check if it's parent is in the same prefab
        int32 index;
        if (context.ObjectToInstance.TryGet(parentId, index) && context.Instances[index].Prefab == prefab)
        {
            // Use parent object as prefab instance
        }
        else
        {
            // Use new prefab instance
            index = context.Instances.Count();
            auto& e = context.Instances.AddOne();
            e.Prefab = prefab;
            e.RootId = id;
            e.RootIndex = i;
            e.StatIndex = i;
        }
        context.ObjectToInstance[id] = index;

        // Add to the prefab instance IDs mapping
        auto& prefabInstance = context.Instances[index];
        prefabInstance.IdsMapping[prefabObjectId] = id;

        // Walk over nested prefabs to link any subobjects into this object (eg. if nested prefab uses cross-object references to link them correctly)
    NESTED_PREFAB_WALK:
        const ISerializable::DeserializeStream* prefabData;
        if (prefab->ObjectsDataCache.TryGet(prefabObjectId, prefabData) && JsonTools::GetGuidIfValid(prefabObjectId, *prefabData, "PrefabObjectID"))
        {
            prefabId = JsonTools::GetGuid(*prefabData, "PrefabID");
            prefab = Content::LoadAsync<Prefab>(prefabId);
            if (prefab && !prefab->WaitForLoaded())
            {
                // Map prefab object ID to the deserialized instance ID
                prefabInstance.IdsMapping[prefabObjectId] = id;
                goto NESTED_PREFAB_WALK;
            }
        }
    }
}

void SceneObjectsFactory::SynchronizeNewPrefabInstances(Context& context, PrefabSyncData& data)
{
    PROFILE_CPU_NAMED("SynchronizeNewPrefabInstances");

    Scripting::ObjectsLookupIdMapping.Set(&data.Modifier->IdsMapping);
    data.InitialCount = data.SceneObjects.Count();

    // Recreate any missing prefab root objects that were deleted (eg. spawned prefab got its root changed and deleted so old prefab instance needs to respawn it)
    for (int32 instanceIndex = 0; instanceIndex < context.Instances.Count(); instanceIndex++)
    {
        PrefabInstance& instance = context.Instances[instanceIndex];
        SceneObject* root = data.SceneObjects[instance.RootIndex];
        if (!root && instance.Prefab)
        {
            instance.FixRootParent = true;

            // Check if current prefab root existed in the deserialized data
            const auto& oldRootData = data.Data[instance.RootIndex];
            const Guid oldRootId = JsonTools::GetGuid(oldRootData, "ID");
            const Guid prefabObjectId = JsonTools::GetGuid(oldRootData, "PrefabObjectID");
            const Guid prefabRootId = instance.Prefab->GetRootObjectId();
            Guid id;
            int32 idInstance = -1;
            bool syncNewRoot = false;
            if (instance.IdsMapping.TryGet(prefabRootId, id) && context.ObjectToInstance.TryGet(id, idInstance) && idInstance == instanceIndex)
            {
                // Update the missing root with the valid object from this prefab instance
                LOG(Warning, "Changed prefab instance root from ID={0}, PrefabObjectID={1} to ID={2}, PrefabObjectID={3} ({4})", instance.RootId, prefabObjectId, id, prefabRootId, instance.Prefab->ToString());
            }
            else
            {
                LOG(Warning, "Missing prefab instance root (ID={0}, PrefabObjectID={1}, {2})", instance.RootId, prefabObjectId, instance.Prefab->ToString());

                // Get prefab object data from the prefab
                const ISerializable::DeserializeStream* prefabData;
                if (!instance.Prefab->ObjectsDataCache.TryGet(prefabRootId, prefabData))
                {
                    LOG(Warning, "Missing object {1} data in prefab {0}.", instance.Prefab->ToString(), prefabObjectId);
                    continue;
                }

                // Map prefab object ID to the new prefab object instance
                id = Guid::New();
                data.Modifier->IdsMapping[prefabRootId] = id;

                // Create prefab instance (recursive prefab loading to support nested prefabs)
                root = Spawn(context, *prefabData);
                if (!root)
                {
                    LOG(Warning, "Failed to create object {1} from prefab {0}.", instance.Prefab->ToString(), prefabRootId);
                    continue;
                }

                // Register object
                root->RegisterObject();
                data.SceneObjects.Add(root);
                auto& newObj = data.NewObjects.AddOne();
                newObj.Prefab = instance.Prefab;
                newObj.PrefabData = prefabData;
                newObj.PrefabObjectId = prefabRootId;
                newObj.Id = id;
                context.ObjectToInstance[id] = instanceIndex;
                syncNewRoot = true;
            }

            // Update prefab root info
            instance.RootId = id;
            instance.RootIndex = data.SceneObjects.Find(Scripting::FindObject<SceneObject>(id));
            CHECK(instance.RootIndex != -1);

            // Remap removed prefab root into the current root (can be different type but is needed for proper hierarchy linkage)
            instance.IdsMapping[prefabObjectId] = id;
            instance.IdsMapping[prefabRootId] = id;
            instance.IdsMapping[oldRootId] = id;
            data.Modifier->IdsMapping[prefabObjectId] = id;
            data.Modifier->IdsMapping[prefabRootId] = id;
            data.Modifier->IdsMapping[oldRootId] = id;

            // Add any sub-objects that are missing (in case new root was created)
            if (syncNewRoot)
            {
                SynchronizeNewPrefabInstances(context, data, instance.Prefab, (Actor*)root, prefabRootId, instance.RootIndex, oldRootData);
            }
        }
    }

    // Check all actors with prefab linkage for adding missing objects
    for (int32 i = 0; i < data.InitialCount; i++)
    {
        Actor* actor = dynamic_cast<Actor*>(data.SceneObjects[i]);
        if (!actor)
            continue;
        const auto& stream = data.Data[i];
        Guid actorId, actorPrefabObjectId, prefabId;
        if (!JsonTools::GetGuidIfValid(actorPrefabObjectId, stream, "PrefabObjectID"))
            continue;
        if (!JsonTools::GetGuidIfValid(prefabId, stream, "PrefabID"))
            continue;
        if (!JsonTools::GetGuidIfValid(actorId, stream, "ID"))
            continue;

        // Load prefab
        auto prefab = Content::LoadAsync<Prefab>(prefabId);
        if (prefab == nullptr)
        {
            LOG(Warning, "Missing prefab {0}.", prefabId);
            continue;
        }
        if (prefab->WaitForLoaded())
        {
            LOG(Warning, "Failed to load prefab {0}.", prefab->ToString());
            continue;
        }

        SynchronizeNewPrefabInstances(context, data, prefab, actor, actorPrefabObjectId, i, stream);
    }

    Scripting::ObjectsLookupIdMapping.Set(nullptr);
}

void SceneObjectsFactory::SynchronizePrefabInstances(Context& context, PrefabSyncData& data)
{
    PROFILE_CPU_NAMED("SynchronizePrefabInstances");

    // Check all objects with prefab linkage for moving to a proper parent
    for (int32 i = 0; i < data.InitialCount; i++)
    {
        SceneObject* obj = data.SceneObjects[i];
        if (!obj)
            continue;
        SceneObject* parent = obj->GetParent();
        const Guid prefabId = obj->GetPrefabID();
        if (parent == nullptr || !obj->HasPrefabLink() || !parent->HasPrefabLink() || parent->GetPrefabID() != prefabId)
            continue;
        const Guid prefabObjectId = obj->GetPrefabObjectID();
        const Guid parentPrefabObjectId = parent->GetPrefabObjectID();

        // Load prefab
        auto prefab = Content::LoadAsync<Prefab>(prefabId);
        if (prefab == nullptr)
        {
            LOG(Warning, "Missing prefab {0}.", prefabId);
            continue;
        }
        if (prefab->WaitForLoaded())
        {
            LOG(Warning, "Failed to load prefab {0}.", prefab->ToString());
            continue;
        }

        // Get the actual parent object stored in the prefab data
        const ISerializable::DeserializeStream* objData;
        Guid actualParentPrefabId;
        if (!prefab->ObjectsDataCache.TryGet(prefabObjectId, objData) || !JsonTools::GetGuidIfValid(actualParentPrefabId, *objData, "ParentID"))
            continue;

        // Validate
        if (actualParentPrefabId != parentPrefabObjectId)
        {
            // Invalid connection object found!
            LOG(Info, "Object {0} has invalid parent object {4} -> {5} (PrefabObjectID: {1}, PrefabID: {2}, Path: {3})", obj->GetSceneObjectId(), prefabObjectId, prefab->GetID(), prefab->GetPath(), parentPrefabObjectId, actualParentPrefabId);

            // Map actual prefab object ID to the current scene objects collection
            context.SetupIdsMapping(obj, data.Modifier);
            data.Modifier->IdsMapping.TryGet(actualParentPrefabId, actualParentPrefabId);

            // Find parent
            const auto actualParent = Scripting::FindObject<Actor>(actualParentPrefabId);
            if (!actualParent)
            {
                LOG(Warning, "The actual parent is missing.");
                continue;
            }

            // Reparent
            obj->SetParent(actualParent, false);
        }

        // Preserve order in parent (values from prefab are used)
        if (i != 0)
        {
            const auto defaultInstance = prefab->GetDefaultInstance(obj->GetPrefabObjectID());
            const int32 order = defaultInstance ? defaultInstance->GetOrderInParent() : -1;
            if (order != -1)
                obj->SetOrderInParent(order);
        }
    }

    // Synchronize new prefab objects
    for (int32 i = 0; i < data.NewObjects.Count(); i++)
    {
        SceneObject* obj = data.SceneObjects[data.InitialCount + i];
        auto& newObj = data.NewObjects[i];

        // Deserialize object with prefab data
        Scripting::ObjectsLookupIdMapping.Set(&data.Modifier->IdsMapping);
        Deserialize(context, obj, *(ISerializable::DeserializeStream*)newObj.PrefabData);
        obj->LinkPrefab(newObj.Prefab->GetID(), newObj.PrefabObjectId);

        // Preserve order in parent (values from prefab are used)
        const auto defaultInstance = newObj.Prefab->GetDefaultInstance(newObj.PrefabObjectId);
        const int32 order = defaultInstance ? defaultInstance->GetOrderInParent() : -1;
        if (order != -1)
            obj->SetOrderInParent(order);
    }

    // Setup hierarchy for the prefab instances (ensure any new objects are connected)
    for (const auto& instance : context.Instances)
    {
        const auto& prefabStartData = data.Data[instance.StatIndex];
        Guid prefabStartParentId;
        if (instance.FixRootParent && JsonTools::GetGuidIfValid(prefabStartParentId, prefabStartData, "ParentID"))
        {
            auto* root = data.SceneObjects[instance.RootIndex];
            const auto rootParent = Scripting::FindObject<Actor>(prefabStartParentId);
            root->SetParent(rootParent, false);
        }
    }

    Scripting::ObjectsLookupIdMapping.Set(nullptr);
}

void SceneObjectsFactory::SynchronizeNewPrefabInstances(Context& context, PrefabSyncData& data, Prefab* prefab, Actor* actor, const Guid& actorPrefabObjectId, int32 i, const ISerializable::DeserializeStream& stream)
{
    // Use cached acceleration structure for prefab hierarchy validation
    const auto* hierarchy = prefab->ObjectsHierarchyCache.TryGet(actorPrefabObjectId);
    if (!hierarchy)
        return;
    const Guid actorId = actor->GetID();
    const auto removedObjects = SERIALIZE_FIND_MEMBER(stream, "RemovedObjects");
    for (const Guid& prefabObjectId : *hierarchy)
    {
        // Skip if object was marked to be removed per instance
        if (removedObjects != stream.MemberEnd())
        {
            auto& list = removedObjects->value;
            const rapidjson::SizeType size = list.Size();
            bool removed = false;
            for (rapidjson::SizeType j = 0; j < size; j++)
            {
                if (JsonTools::GetGuid(list[j]) == prefabObjectId)
                {
                    removed = true;
                    break;
                }
            }
            if (removed)
                continue;
        }

        // Use only objects that are missing
        bool spawned = false;
        int32 childSearchStart = i + 1; // Objects are serialized with parent followed by its children
        int32 instanceIndex = -1;
        if (context.ObjectToInstance.TryGet(actorId, instanceIndex) && context.Instances[instanceIndex].Prefab == prefab)
        {
            // Quickly check if that object exists
            auto& prefabInstance = context.Instances[instanceIndex];
            Guid id;
            int32 idInstanceIndex;
            if (prefabInstance.IdsMapping.TryGet(prefabObjectId, id) && 
                context.ObjectToInstance.TryGet(id, idInstanceIndex) && 
                idInstanceIndex == instanceIndex)
                continue;

            // Start searching from the beginning of that prefab instance (eg. in case prefab objects were reordered)
            childSearchStart = Math::Min(childSearchStart, prefabInstance.StatIndex);
        }
        for (int32 j = childSearchStart; j < data.InitialCount; j++)
        {
            if (JsonTools::GetGuid(data.Data[j], "PrefabObjectID") == prefabObjectId)
            {
                // This object exists in the saved scene objects list
                spawned = true;
                break;
            }
        }
        if (spawned)
            continue;

        // Map prefab object ID to this actor's prefab instance so the new objects gets added to it
        context.SetupIdsMapping(actor, data.Modifier);
        data.Modifier->IdsMapping[actorPrefabObjectId] = actorId;
        Scripting::ObjectsLookupIdMapping.Set(&data.Modifier->IdsMapping);

        // Create instance (including all children)
        SynchronizeNewPrefabInstance(context, data, prefab, actor, prefabObjectId);
    }
}

void SceneObjectsFactory::SynchronizeNewPrefabInstance(Context& context, PrefabSyncData& data, Prefab* prefab, Actor* actor, const Guid& prefabObjectId)
{
    PROFILE_CPU_NAMED("SynchronizeNewPrefabInstance");

    // Missing object found!
    LOG(Info, "Actor {0} has missing child object (PrefabObjectID: {1}, PrefabID: {2}, Path: {3})", actor->ToString(), prefabObjectId, prefab->GetID(), prefab->GetPath());

    // Get prefab object data from the prefab
    const ISerializable::DeserializeStream* prefabData;
    if (!prefab->ObjectsDataCache.TryGet(prefabObjectId, prefabData))
    {
        LOG(Warning, "Missing object {1} data in prefab {0}.", prefab->ToString(), prefabObjectId);
        return;
    }

    // Map prefab object ID to the new prefab object instance
    Guid id = Guid::New();
    data.Modifier->IdsMapping[prefabObjectId] = id;

    // Create prefab instance (recursive prefab loading to support nested prefabs)
    auto child = Spawn(context, *prefabData);
    if (!child)
    {
        LOG(Warning, "Failed to create object {1} from prefab {0}.", prefab->ToString(), prefabObjectId);
        return;
    }

    // Register object
    child->RegisterObject();
    data.SceneObjects.Add(child);
    auto& newObj = data.NewObjects.AddOne();
    newObj.Prefab = prefab;
    newObj.PrefabData = prefabData;
    newObj.PrefabObjectId = prefabObjectId;
    newObj.Id = id;
    int32 instanceIndex = -1;
    if (context.ObjectToInstance.TryGet(actor->GetID(), instanceIndex) && context.Instances[instanceIndex].Prefab == prefab)
    {
        // Add to the prefab instance IDs mapping
        context.ObjectToInstance[id] = instanceIndex;
        auto& prefabInstance = context.Instances[instanceIndex];
        prefabInstance.IdsMapping[prefabObjectId] = id;
    }

    // Use loop to add even more objects to added objects (prefab can have one new object that has another child, we need to add that child)
    // TODO: prefab could cache lookup object id -> children ids
    for (auto q = prefab->ObjectsDataCache.Begin(); q.IsNotEnd(); ++q)
    {
        Guid qParentId;
        if (JsonTools::GetGuidIfValid(qParentId, *q->Value, "ParentID") && qParentId == prefabObjectId)
        {
            const Guid qPrefabObjectId = JsonTools::GetGuid(*q->Value, "ID");
            SynchronizeNewPrefabInstance(context, data, prefab, actor, qPrefabObjectId);
        }
    }
}
