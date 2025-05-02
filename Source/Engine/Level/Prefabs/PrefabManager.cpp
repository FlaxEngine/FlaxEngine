// Copyright (c) Wojciech Figat. All rights reserved.

#include "PrefabManager.h"
#include "../Scene/Scene.h"
#include "Engine/Debug/Exceptions/ArgumentNullException.h"
#include "Engine/Level/Prefabs/Prefab.h"
#include "Engine/Level/SceneObjectsFactory.h"
#include "Engine/Level/SceneQuery.h"
#include "Engine/Level/ActorsCache.h"
#include "Engine/Content/Content.h"
#include "Engine/ContentImporters/CreateJson.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Core/Collections/CollectionPoolCache.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Core/Cache.h"
#include "Engine/Core/LogContext.h"
#include "Engine/Debug/Exceptions/ArgumentException.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Scripting/Script.h"
#include "Engine/Scripting/Scripting.h"

#if USE_EDITOR
bool PrefabManager::IsCreatingPrefab = false;
Dictionary<Guid, Array<Actor*>> PrefabManager::PrefabsReferences;
CriticalSection PrefabManager::PrefabsReferencesLocker;
#endif

class PrefabManagerService : public EngineService
{
public:
    PrefabManagerService()
        : EngineService(TEXT("Prefab Manager"))
    {
    }
};

PrefabManagerService PrefabManagerServiceInstance;

Actor* PrefabManager::SpawnPrefab(Prefab* prefab)
{
    Actor* parent = Level::Scenes.Count() != 0 ? Level::Scenes.Get()[0] : nullptr;
    SpawnOptions options;
    options.Parent = parent;
    return SpawnPrefab(prefab, options);
}

Actor* PrefabManager::SpawnPrefab(Prefab* prefab, const Vector3& position)
{
    Actor* parent = Level::Scenes.Count() != 0 ? Level::Scenes.Get()[0] : nullptr;
    return SpawnPrefab(prefab, Transform(position), parent, nullptr);
}

Actor* PrefabManager::SpawnPrefab(Prefab* prefab, const Vector3& position, const Quaternion& rotation)
{
    Actor* parent = Level::Scenes.Count() != 0 ? Level::Scenes.Get()[0] : nullptr;
    return SpawnPrefab(prefab, Transform(position, rotation), parent, nullptr);
}

Actor* PrefabManager::SpawnPrefab(Prefab* prefab, const Vector3& position, const Quaternion& rotation, const Vector3& scale)
{
    Actor* parent = Level::Scenes.Count() != 0 ? Level::Scenes.Get()[0] : nullptr;
    return SpawnPrefab(prefab, Transform(position, rotation, scale), parent, nullptr);
}

Actor* PrefabManager::SpawnPrefab(Prefab* prefab, const Transform& transform)
{
    Actor* parent = Level::Scenes.Count() != 0 ? Level::Scenes.Get()[0] : nullptr;
    return SpawnPrefab(prefab, transform, parent, nullptr);
}

Actor* PrefabManager::SpawnPrefab(Prefab* prefab, Actor* parent, const Transform& transform)
{
    SpawnOptions options;
    options.Transform = &transform;
    options.Parent = parent;
    return SpawnPrefab(prefab, options);
}

Actor* PrefabManager::SpawnPrefab(Prefab* prefab, Actor* parent)
{
    SpawnOptions options;
    options.Parent = parent;
    return SpawnPrefab(prefab, options);
}

Actor* PrefabManager::SpawnPrefab(Prefab* prefab, Actor* parent, Dictionary<Guid, SceneObject*>* objectsCache, bool withSynchronization)
{
    SpawnOptions options;
    options.Parent = parent;
    options.ObjectsCache = objectsCache;
    options.WithSync = withSynchronization;
    return SpawnPrefab(prefab, options);
}

Actor* PrefabManager::SpawnPrefab(Prefab* prefab, const Transform& transform, Actor* parent, Dictionary<Guid, SceneObject*>* objectsCache, bool withSynchronization)
{
    SpawnOptions options;
    options.Transform = &transform;
    options.Parent = parent;
    options.ObjectsCache = objectsCache;
    options.WithSync = withSynchronization;
    return SpawnPrefab(prefab, options);
}

Actor* PrefabManager::SpawnPrefab(Prefab* prefab, const SpawnOptions& options)
{
    PROFILE_CPU_NAMED("Prefab.Spawn");
    if (prefab == nullptr)
    {
        Log::ArgumentNullException();
        return nullptr;
    }
    if (prefab->WaitForLoaded())
    {
        LOG(Warning, "Waiting for prefab asset be loaded failed. {0}", prefab->ToString());
        return nullptr;
    }
    const int32 dataCount = prefab->ObjectsCount;
    if (dataCount == 0)
    {
        LOG(Warning, "Prefab has no objects. {0}", prefab->ToString());
        return nullptr;
    }
    const Guid prefabId = prefab->GetID();

    // Note: we need to generate unique Ids for the deserialized objects (actors and scripts) to prevent Ids collisions
    // Prefab asset during loading caches the object Ids stored inside the file

    // Prepare
    CollectionPoolCache<ActorsCache::SceneObjectsListType>::ScopeCache sceneObjects = ActorsCache::SceneObjectsListCache.Get();
    sceneObjects->Resize(dataCount);
    CollectionPoolCache<ISerializeModifier, Cache::ISerializeModifierClearCallback>::ScopeCache modifier = Cache::ISerializeModifier.Get();
    modifier->EngineBuild = prefab->DataEngineBuild;
    modifier->IdsMapping.EnsureCapacity(prefab->ObjectsIds.Count());
    if (options.IDs)
    {
        for (int32 i = 0; i < prefab->ObjectsIds.Count(); i++)
        {
            Guid prefabObjectId = prefab->ObjectsIds[i];
            Guid id;
            if (!options.IDs->TryGet(prefabObjectId, id))
                id = Guid::New();
            modifier->IdsMapping.Add(id, id);
        }
    }
    else
    {
        for (int32 i = 0; i < prefab->ObjectsIds.Count(); i++)
            modifier->IdsMapping.Add(prefab->ObjectsIds[i], Guid::New());
    }
    if (options.ObjectsCache)
    {
        options.ObjectsCache->Clear();
        options.ObjectsCache->SetCapacity(prefab->ObjectsDataCache.Count());
    }
    auto& data = *prefab->Data;
    SceneObjectsFactory::Context context(modifier.Value);
    LogContextScope logContext(prefabId);

    // Deserialize prefab objects
    auto prevIdMapping = Scripting::ObjectsLookupIdMapping.Get();
    Scripting::ObjectsLookupIdMapping.Set(&modifier.Value->IdsMapping);
    for (int32 i = 0; i < dataCount; i++)
    {
        auto& stream = data[i];
        SceneObject* obj = SceneObjectsFactory::Spawn(context, stream);
        sceneObjects->At(i) = obj;
        if (obj)
            obj->RegisterObject();
        else
            SceneObjectsFactory::HandleObjectDeserializationError(stream);
    }
    SceneObjectsFactory::PrefabSyncData prefabSyncData(*sceneObjects.Value, data, modifier.Value);
    if (options.WithSync)
    {
        // Synchronize new prefab instances (prefab may have new objects added so deserialized instances need to synchronize with it)
        // TODO: resave and force sync prefabs during game cooking so this step could be skipped in game
        SceneObjectsFactory::SetupPrefabInstances(context, prefabSyncData);
        SceneObjectsFactory::SynchronizeNewPrefabInstances(context, prefabSyncData);
        Scripting::ObjectsLookupIdMapping.Set(&modifier.Value->IdsMapping);
    }
    for (int32 i = 0; i < dataCount; i++)
    {
        auto& stream = data[i];
        SceneObject* obj = sceneObjects->At(i);
        if (obj)
            SceneObjectsFactory::Deserialize(context, obj, stream);
    }
    Scripting::ObjectsLookupIdMapping.Set(prevIdMapping);

    // Synchronize prefab instances (prefab may have new objects added or some removed so deserialized instances need to synchronize with it)
    if (options.WithSync)
    {
        // TODO: resave and force sync scenes during game cooking so this step could be skipped in game
        SceneObjectsFactory::SynchronizePrefabInstances(context, prefabSyncData);
    }

    // Pick prefab root object
    Actor* root = nullptr;
    const Guid prefabRootObjectId = prefab->GetRootObjectId();
    for (int32 i = 0; i < dataCount && !root; i++)
    {
        if (JsonTools::GetGuid(data[i], "ID") == prefabRootObjectId)
            root = dynamic_cast<Actor*>(sceneObjects->At(i));
    }
    if (!root)
    {
        // Fallback to the first actor that has no parent
        for (int32 i = 0; i < sceneObjects->Count() && !root; i++)
        {
            SceneObject* obj = sceneObjects->At(i);
            if (obj && !obj->GetParent())
                root = dynamic_cast<Actor*>(obj);
        }
    }
    if (!root)
    {
        LOG(Warning, "Missing prefab root object. {0}", prefab->ToString());
        return nullptr;
    }

    // Prepare parent linkage for prefab root actor
    if (root->_parent)
        root->_parent->Children.Remove(root);
    root->_parent = options.Parent;
    if (options.Parent)
        options.Parent->Children.Add(root);

    // Move root to the right location
    if (options.Transform)
        root->SetTransform(*options.Transform);

    // Link actors hierarchy
    for (int32 i = 0; i < sceneObjects->Count(); i++)
    {
        SceneObject* obj = sceneObjects->At(i);
        if (obj)
            obj->Initialize();
    }

    // Delete objects without parent or with invalid linkage to the prefab
    for (int32 i = 0; i < sceneObjects->Count(); i++)
    {
        SceneObject* obj = sceneObjects->At(i);
        if (!obj || obj == root)
            continue;

        // Check for missing parent (eg. parent object has been deleted)
        if (obj->GetParent() == nullptr)
        {
            LOG(Warning, "Scene object {0} {1} has missing parent object after load. Removing it.", obj->GetID(), obj->ToString());
            sceneObjects->At(i) = nullptr;
            obj->DeleteObject();
            continue;
        }

#if (USE_EDITOR && !BUILD_RELEASE) || FLAX_TESTS
        // Check for not being added to the parent (eg. invalid setup events fault on registration)
        auto actor = dynamic_cast<Actor*>(obj);
        auto script = dynamic_cast<Script*>(obj);
        if (obj->GetParent() == obj || (actor && !actor->GetParent()->Children.Contains(actor)) || (script && !script->GetParent()->Scripts.Contains(script)))
        {
            LOG(Warning, "Scene object {0} {1} has invalid parent object linkage after load. Removing it.", obj->GetID(), obj->ToString());
            sceneObjects->At(i) = nullptr;
            obj->DeleteObject();
            continue;
        }
#endif

#if (USE_EDITOR && BUILD_DEBUG) || FLAX_TESTS
        // Check for being added to parent not from spawned prefab (eg. invalid parentId linkage fault)
        bool hasParentInInstance = false;
        for (int32 j = 0; j < sceneObjects->Count(); j++)
        {
            if (sceneObjects->At(j) == obj->GetParent())
            {
                hasParentInInstance = true;
                break;
            }
        }
        if (!hasParentInInstance)
        {
            LOG(Warning, "Scene object {0} {1} has invalid parent object after load. Removing it.", obj->GetID(), obj->ToString());
            sceneObjects->At(i) = nullptr;
            obj->DeleteObject();
            continue;
        }

#if FLAX_TESTS
        // Perform extensive validation of the prefab instance structure
        if (actor && actor->HasActorInHierarchy(actor))
        {
            LOG(Warning, "Scene object {0} {1} has invalid hierarchy after load. Removing it.", obj->GetID(), obj->ToString());
            sceneObjects->At(i) = nullptr;
            obj->DeleteObject();
            continue;
        }
#endif
#endif
    }

    // Link objects to prefab (only deserialized from prefab data)
    if (options.WithLink)
    {
        for (int32 i = 0; i < dataCount; i++)
        {
            auto& stream = data[i];
            SceneObject* obj = sceneObjects->At(i);
            if (!obj)
                continue;

            const Guid prefabObjectId = JsonTools::GetGuid(stream, "ID");
            if (options.ObjectsCache)
                options.ObjectsCache->Add(prefabObjectId, obj);
            obj->LinkPrefab(prefabId, prefabObjectId);
        }
    }
    else if (options.ObjectsCache)
    {
        for (int32 i = 0; i < dataCount; i++)
        {
            auto& stream = data[i];
            SceneObject* obj = sceneObjects->At(i);
            if (!obj)
                continue;
            const Guid prefabObjectId = JsonTools::GetGuid(stream, "ID");
            options.ObjectsCache->Add(prefabObjectId, obj);
        }
    }

    // Update transformations
    root->OnTransformChanged();

    // Spawn if need to
    if (options.Parent && options.Parent->IsDuringPlay())
    {
        // Begin play
        SceneBeginData beginData;
        root->BeginPlay(&beginData);
        beginData.OnDone();

        // Send event
        Level::callActorEvent(Level::ActorEventType::OnActorSpawned, root, nullptr);
    }

    return root;
}

#if USE_EDITOR

bool PrefabManager::CreatePrefab(Actor* targetActor, const StringView& outputPath, bool autoLink)
{
    PROFILE_CPU_NAMED("Prefab.Create");

    // Validate input
    if (targetActor == nullptr)
    {
        Log::ArgumentNullException();
        return true;
    }
    if (dynamic_cast<Scene*>(targetActor) != nullptr)
    {
        LOG(Error, "Cannot create prefab from scene actor.");
        return true;
    }
    if (EnumHasAnyFlags(targetActor->HideFlags, HideFlags::DontSave))
    {
        LOG(Error, "Cannot create prefab from actor marked with HideFlags.DontSave.");
        return true;
    }

    // Gather all objects
    CollectionPoolCache<ActorsCache::SceneObjectsListType>::ScopeCache sceneObjects = ActorsCache::SceneObjectsListCache.Get();
    sceneObjects->EnsureCapacity(256);
    SceneQuery::GetAllSerializableSceneObjects(targetActor, *sceneObjects.Value);

    // Filter actors for prefab
    if (FilterPrefabInstancesToSave(Guid::Empty, *sceneObjects.Value))
        return true;

    LOG(Info, "Creating prefab from actor {0} (total objects count: {2}) to {1}...", targetActor->ToString(), outputPath, sceneObjects->Count());

    // Serialize to json data
    ASSERT(!IsCreatingPrefab);
    IsCreatingPrefab = true;
    rapidjson_flax::StringBuffer actorsDataBuffer;
    {
        CompactJsonWriter writerObj(actorsDataBuffer);
        JsonWriter& writer = writerObj;
        writer.StartArray();
        for (int32 i = 0; i < sceneObjects->Count(); i++)
        {
            SceneObject* obj = sceneObjects->At(i);
            writer.SceneObject(obj);
        }
        writer.EndArray();
    }
    IsCreatingPrefab = false;

    // Randomize the objects ids (prevent overlapping of the prefab instance objects ids and the prefab objects ids)
    Dictionary<Guid, Guid> objectInstanceIdToPrefabObjectId;
    objectInstanceIdToPrefabObjectId.EnsureCapacity(sceneObjects->Count());
    if (targetActor->HasParent())
    {
        // Unlink from parent actor
        objectInstanceIdToPrefabObjectId[targetActor->GetParent()->GetID()] = Guid::Empty;
    }
    for (int32 i = 0; i < sceneObjects->Count(); i++)
    {
        // Generate new IDs for the prefab objects (other than reference instance used to create prefab)
        const SceneObject* obj = sceneObjects->At(i);
        objectInstanceIdToPrefabObjectId[obj->GetSceneObjectId()] = Guid::New();
    }
    {
        // Parse json to DOM document
        rapidjson_flax::Document doc;
        {
            PROFILE_CPU_NAMED("Json.Parse");
            doc.Parse(actorsDataBuffer.GetString(), actorsDataBuffer.GetSize());
        }
        if (doc.HasParseError())
        {
            LOG(Warning, "Failed to parse serialized actors data.");
            return true;
        }

        // Process json
        JsonTools::ChangeIds(doc, objectInstanceIdToPrefabObjectId);

        // Save back to text
        actorsDataBuffer.Clear();
        PrettyJsonWriter writer(actorsDataBuffer);
        doc.Accept(writer.GetWriter());
    }

    // Save to file
#if COMPILE_WITH_ASSETS_IMPORTER
    if (CreateJson::Create(outputPath, actorsDataBuffer, Prefab::TypeName))
    {
        LOG(Warning, "Failed to serialize prefab data to the asset.");
        return true;
    }
#else
#error "Cannot support prefabs creating without assets importing enabled."
#endif

    // Auto link objects
    if (autoLink)
    {
        LOG(Info, "Linking objects to prefab");

        AssetInfo assetInfo;
        if (!Content::GetAssetInfo(outputPath, assetInfo))
            return true;

        for (int32 i = 0; i < sceneObjects->Count(); i++)
        {
            SceneObject* obj = sceneObjects->At(i);
            Guid prefabObjectId;
            if (objectInstanceIdToPrefabObjectId.TryGet(obj->GetSceneObjectId(), prefabObjectId))
            {
                obj->LinkPrefab(assetInfo.ID, prefabObjectId);
            }
        }
    }

    LOG(Info, "Prefab created!");
    return false;
}

bool FindPrefabLink(const Guid& targetPrefabId, const Guid& prefabId)
{
    // Get prefab asset
    auto prefab = Content::LoadAsync<Prefab>(prefabId);
    if (prefab == nullptr)
    {
        Log::Exception(TEXT("Missing prefab asset."));
        return true;
    }
    if (prefab->WaitForLoaded())
    {
        Log::Exception(TEXT("Failed to load prefab asset."));
        return true;
    }

    // TODO: check if prefab has any reference to targetPrefabId, then do recursive call to nested prefabs inside the prefab

    return false;
}

bool PrefabManager::FilterPrefabInstancesToSave(const Guid& prefabId, Array<SceneObject*>& objects, bool showWarning)
{
#if 0 // Done by collecting actors without DontSave flag
	// Remove hidden actors for saving
	for (int32 i = 0; i < actors.Count() && actors.HasItems(); i++)
	{
		auto actor = actors[i];

		if ((actor->HideFlags & HideFlags::DontSave) != 0)
		{
			actors.RemoveAt(i);
			i--;
		}
	}
#endif

    // Validate circular references
    if (prefabId.IsValid())
    {
        bool hasLoopPrefabRef = false;

        for (int32 i = 0; i < objects.Count() && objects.HasItems(); i++)
        {
            SceneObject* obj = objects[i];

            if (obj->GetPrefabID().IsValid() && FindPrefabLink(prefabId, obj->GetPrefabID()))
            {
                hasLoopPrefabRef = true;

                objects.RemoveAt(i);
                i--;
            }
        }

        if (hasLoopPrefabRef && showWarning)
        {
            LOG(Error, "Cannot setup prefab with circular reference to itself.");
        }
    }

    // Check if list is empty (after validation)
    if (objects.IsEmpty())
    {
        LOG(Warning, "Cannot create prefab. No valid objects to use.");
        return true;
    }

    return false;
}

bool PrefabManager::ApplyAll(Actor* instance)
{
    PROFILE_CPU_NAMED("Prefab.ApplyAll");

    // Validate input
    if (instance == nullptr)
    {
        Log::ArgumentNullException();
        return true;
    }
    if (!instance->HasPrefabLink() || instance->GetPrefabID() == Guid::Empty)
    {
        Log::ArgumentException(TEXT("The modified actor instance has missing prefab link."));
        return true;
    }

    // Get prefab asset
    auto prefab = Content::LoadAsync<Prefab>(instance->GetPrefabID());
    if (prefab == nullptr)
    {
        Log::Exception(TEXT("Missing prefab asset."));
        return true;
    }
    if (prefab->WaitForLoaded())
    {
        Log::Exception(TEXT("Failed to load prefab asset."));
        return true;
    }

    // Get root object of this prefab instance
    // Note: given actor instance can be already part of the spawned prefab or be a new actor added to the prefab instance
    // Find the actual root of the prefab instance
    const auto rootObjectId = prefab->GetRootObjectId();
    auto rootObjectInstance = instance;
    while (rootObjectInstance && rootObjectInstance->GetPrefabObjectID() != rootObjectId)
    {
        rootObjectInstance = rootObjectInstance->GetParent();
    }
    if (rootObjectInstance == nullptr)
    {
        // Use the input object as fallback
        rootObjectInstance = instance;
    }

    return prefab->ApplyAll(rootObjectInstance);
}

#endif
