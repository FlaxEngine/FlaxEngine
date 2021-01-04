// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "SceneObjectsFactory.h"
#include "Engine/Level/Actor.h"
#include "Engine/Level/Prefabs/Prefab.h"
#include "Engine/Content/Content.h"
#include "Engine/Core/Log.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Serialization/ISerializeModifier.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Profiler/ProfilerCPU.h"

SceneObject* SceneObjectsFactory::Spawn(ISerializable::DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Get object id
    Guid id = JsonTools::GetGuid(stream, "ID");
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
            LOG(Warning, "Missing prefab with id={0}.", prefabId);
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
        obj = Spawn(*(ISerializable::DeserializeStream*)prefabData, modifier);
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
            const StringAnsiView typeName(typeNameMember->value.GetString(), typeNameMember->value.GetStringLength());

            const ScriptingTypeHandle type = Scripting::FindScriptingType(typeName);
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

void SceneObjectsFactory::Deserialize(SceneObject* obj, ISerializable::DeserializeStream& stream, ISerializeModifier* modifier)
{
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
            LOG(Warning, "Missing prefab with id={0}.", prefabId);
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
        Deserialize(obj, *(ISerializable::DeserializeStream*)prefabData, modifier);
    }

    // Load data
    obj->Deserialize(stream, modifier);
}

bool Contains(Actor* actor, const SceneObjectsFactory::ActorToRemovedObjectsDataLookup& actorToRemovedObjectsData, const Guid& prefabObjectId)
{
    // Check if actor has any removed objects registered
    const rapidjson_flax::Value* data;
    if (actorToRemovedObjectsData.TryGet(actor, data))
    {
        const int32 size = static_cast<int32>(data->Size());
        for (int32 i = 0; i < size; i++)
        {
            if (JsonTools::GetGuid(data->operator[](i)) == prefabObjectId)
            {
                return true;
            }
        }
    }

    return false;
}

void SceneObjectsFactory::SynchronizePrefabInstances(Array<SceneObject*>& sceneObjects, const ActorToRemovedObjectsDataLookup& actorToRemovedObjectsData, ISerializeModifier* modifier)
{
    PROFILE_CPU_NAMED("SynchronizePrefabInstances");

    // Check all objects with prefab linkage for moving to a proper parent
    const int32 objectsToCheckCount = sceneObjects.Count();
    for (int32 i = 0; i < objectsToCheckCount; i++)
    {
        SceneObject* obj = sceneObjects[i];
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
            LOG(Warning, "Missing prefab with id={0}.", prefabId);
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

            // Map actual prefab object id to the current scene objects collection
            modifier->IdsMapping.TryGet(actualParentPrefabId, actualParentPrefabId);

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
    }

    // Check all actors with prefab linkage for adding missing objects
    for (int32 i = 0; i < objectsToCheckCount; i++)
    {
        Actor* actor = dynamic_cast<Actor*>(sceneObjects[i]);
        if (!actor || !actor->HasPrefabLink())
            continue;
        const Guid actorId = actor->GetID();
        const Guid prefabId = actor->GetPrefabID();
        const Guid actorPrefabObjectId = actor->GetPrefabObjectID();

        // Map prefab object id to this actor so the new objects gets added to it
        modifier->IdsMapping[actorPrefabObjectId] = actorId;

        // Load prefab
        auto prefab = Content::LoadAsync<Prefab>(prefabId);
        if (prefab == nullptr)
        {
            LOG(Warning, "Missing prefab with id={0}.", prefabId);
            continue;
        }
        if (prefab->WaitForLoaded())
        {
            LOG(Warning, "Failed to load prefab {0}.", prefab->ToString());
            continue;
        }

        // Check if the given actor has new children or scripts added (inside the prefab that it uses)
        // TODO: consider caching prefab objects structure maybe to boost this logic?
        for (auto it = prefab->ObjectsDataCache.Begin(); it.IsNotEnd(); ++it)
        {
            // Use only objects that are linked to the current actor
            const Guid parentId = JsonTools::GetGuid(*it->Value, "ParentID");
            if (parentId != actorPrefabObjectId)
                continue;

            // Use only objects that are missing
            const Guid prefabObjectId = JsonTools::GetGuid(*it->Value, "ID");
            if (actor->GetChildByPrefabObjectId(prefabObjectId) != nullptr ||
                actor->GetScriptByPrefabObjectId(prefabObjectId) != nullptr)
                continue;

            // Skip if object was marked to be removed per instance
            if (Contains(actor, actorToRemovedObjectsData, prefabObjectId))
                continue;

            // Create instance (including all children)
            SynchronizeNewPrefabInstance(prefab, actor, prefabObjectId, sceneObjects, modifier);
        }
    }

    // Call post load event to the new objects
    for (int32 i = objectsToCheckCount; i < sceneObjects.Count(); i++)
    {
        SceneObject* obj = sceneObjects[i];
        obj->PostLoad();
    }
}

void SceneObjectsFactory::HandleObjectDeserializationError(const ISerializable::DeserializeStream& value)
{
    rapidjson_flax::StringBuffer buffer;
    PrettyJsonWriter writer(buffer);
    value.Accept(writer.GetWriter());

    LOG(Warning, "Failed to deserialize scene object from data: {0}", String(buffer.GetString()));
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

void SceneObjectsFactory::SynchronizeNewPrefabInstance(Prefab* prefab, Actor* actor, const Guid& prefabObjectId, Array<SceneObject*>& sceneObjects, ISerializeModifier* modifier)
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
    modifier->IdsMapping[prefabObjectId] = Guid::New();

    // Create prefab instance (recursive prefab loading to support nested prefabs)
    auto child = Spawn(*(ISerializable::DeserializeStream*)prefabData, modifier);
    if (!child)
    {
        LOG(Warning, "Failed to create object {1} from prefab {0}.", prefab->ToString(), prefabObjectId);
        return;
    }
    child->RegisterObject();
    Deserialize(child, *(ISerializable::DeserializeStream*)prefabData, modifier);

    // Register object
    child->LinkPrefab(prefab->GetID(), prefabObjectId);
    sceneObjects.Add(child);

    // Use loop to add even more objects to added objects (prefab can have one new object that has another child, we need to add that child)
    // TODO: prefab could cache lookup object id -> children ids
    for (auto q = prefab->ObjectsDataCache.Begin(); q.IsNotEnd(); ++q)
    {
        Guid qParentId;
        if (JsonTools::GetGuidIfValid(qParentId, *q->Value, "ParentID") && qParentId == prefabObjectId)
        {
            const Guid qPrefabObjectId = JsonTools::GetGuid(*q->Value, "ID");
            SynchronizeNewPrefabInstance(prefab, actor, qPrefabObjectId, sceneObjects, modifier);
        }
    }
}
