// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
#include "Engine/Debug/Exceptions/ArgumentException.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Scripting/Scripting.h"

#if USE_EDITOR
bool PrefabManager::IsCreatingPrefab = false;
bool PrefabManager::IsNotCreatingPrefab = true;
Dictionary<Guid, Array<Actor*>> PrefabManager::PrefabsReferences;
CriticalSection PrefabManager::PrefabsReferencesLocker;
#endif

class PrefabManagerService : public EngineService
{
public:

    PrefabManagerService()
        : EngineService(TEXT("Prefab Manager"), 110)
    {
    }
};

PrefabManagerService PrefabManagerServiceInstance;

Actor* PrefabManager::SpawnPrefab(Prefab* prefab)
{
    auto scene = Level::GetScene(0);
    if (!scene)
        return nullptr;
    return SpawnPrefab(prefab, scene, nullptr);
}

Actor* PrefabManager::SpawnPrefab(Prefab* prefab, const Vector3& position)
{
    auto instance = SpawnPrefab(prefab);
    if (instance)
    {
        instance->SetPosition(position);
    }
    return instance;
}

Actor* PrefabManager::SpawnPrefab(Prefab* prefab, const Vector3& position, const Quaternion& rotation)
{
    auto instance = SpawnPrefab(prefab);
    if (instance)
    {
        auto transform = instance->GetTransform();
        transform.Translation = position;
        transform.Orientation = rotation;
        instance->SetTransform(transform);
    }
    return instance;
}

Actor* PrefabManager::SpawnPrefab(Prefab* prefab, const Vector3& position, const Quaternion& rotation, const Vector3& scale)
{
    auto instance = SpawnPrefab(prefab);
    if (instance)
    {
        Transform transform;
        transform.Translation = position;
        transform.Orientation = rotation;
        transform.Scale = scale;
        instance->SetTransform(transform);
    }
    return instance;
}

Actor* PrefabManager::SpawnPrefab(Prefab* prefab, const Transform& transform)
{
    auto instance = SpawnPrefab(prefab);
    if (instance)
    {
        instance->SetTransform(transform);
    }
    return instance;
}

Actor* PrefabManager::SpawnPrefab(Prefab* prefab, Actor* parent, Dictionary<Guid, const void*>* objectsCache, bool withSynchronization)
{
    PROFILE_CPU_NAMED("Prefab.Spawn");

    // Validate input
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
    const int32 objectsCount = prefab->ObjectsCount;
    if (objectsCount == 0)
    {
        LOG(Warning, "Prefab has no objects. {0}", prefab->ToString());
        return nullptr;
    }

    // Note: we need to generate unique Ids for the deserialized objects (actors and scripts) to prevent Ids collisions
    // Prefab asset during loading caches the object Ids stored inside the file

    const Guid prefabId = prefab->GetID();

    // Prepare
    CollectionPoolCache<ActorsCache::SceneObjectsListType>::ScopeCache sceneObjects = ActorsCache::SceneObjectsListCache.Get();
    sceneObjects->Resize(objectsCount);
    CollectionPoolCache<ActorsCache::SceneObjectsListType>::ScopeCache prefabDataIndexToSceneObject = ActorsCache::SceneObjectsListCache.Get();
    prefabDataIndexToSceneObject->Resize(objectsCount);
    CollectionPoolCache<ISerializeModifier, Cache::ISerializeModifierClearCallback>::ScopeCache modifier = Cache::ISerializeModifier.Get();
    modifier->IdsMapping.EnsureCapacity(prefab->ObjectsIds.Count() * 4);
    for (int32 i = 0; i < prefab->ObjectsIds.Count(); i++)
    {
        modifier->IdsMapping.Add(prefab->ObjectsIds[i], Guid::New());
    }
    if (objectsCache)
    {
        objectsCache->Clear();
        objectsCache->SetCapacity(prefab->ObjectsDataCache.Capacity());
    }

    // Deserialize prefab objects
    auto& data = *prefab->Data;
    Scripting::ObjectsLookupIdMapping.Set(&modifier.Value->IdsMapping);
    for (int32 i = 0; i < objectsCount; i++)
    {
        auto& stream = data[i];

        SceneObject* obj = SceneObjectsFactory::Spawn(stream, modifier.Value);
        prefabDataIndexToSceneObject->operator[](i) = obj;
        sceneObjects->At(i) = obj;
        if (obj)
        {
            obj->RegisterObject();
        }
        else
        {
            SceneObjectsFactory::HandleObjectDeserializationError(stream);
        }
    }
    for (int32 i = 0; i < objectsCount; i++)
    {
        auto& stream = data[i];
        SceneObject* obj = prefabDataIndexToSceneObject->At(i);
        if (obj)
        {
            SceneObjectsFactory::Deserialize(obj, stream, modifier.Value);
        }
    }
    Scripting::ObjectsLookupIdMapping.Set(nullptr);

    // Assume that prefab has always only one root actor that is serialized first
    if (sceneObjects.Value->IsEmpty())
    {
        LOG(Warning, "No valid objects in prefab.");
        return nullptr;
    }
    auto root = (Actor*)sceneObjects.Value->At(0);
    if (!root)
    {
        LOG(Warning, "Failed to load prefab root object.");
        return nullptr;
    }

    // Prepare parent linkage for prefab root actor
    root->_parent = parent;
    if (parent)
        parent->Children.Add(root);

    // Link actors hierarchy
    for (int32 i = 0; i < sceneObjects->Count(); i++)
    {
        auto obj = sceneObjects->At(i);
        if (obj)
        {
            obj->PostLoad();
        }
    }

    // Synchronize prefab instances (prefab may have new objects added or some removed so deserialized instances need to synchronize with it)
    if (withSynchronization)
    {
        // Maps the loaded actor object to the json data with the RemovedObjects array (used to skip restoring objects removed per prefab instance)
        SceneObjectsFactory::ActorToRemovedObjectsDataLookup actorToRemovedObjectsData;
        for (int32 i = 0; i < objectsCount; i++)
        {
            auto& stream = data[i];
            Actor* actor = dynamic_cast<Actor*>(prefabDataIndexToSceneObject->At(i));
            if (!actor)
                continue;

            // Check for RemovedObjects listing
            const auto removedObjects = stream.FindMember("RemovedObjects");
            if (removedObjects != stream.MemberEnd())
            {
                actorToRemovedObjectsData.Add(actor, &removedObjects->value);
            }
        }

        // TODO: consider caching actorToRemovedObjectsData per prefab

        Scripting::ObjectsLookupIdMapping.Set(&modifier.Value->IdsMapping);
        SceneObjectsFactory::SynchronizePrefabInstances(*sceneObjects.Value, actorToRemovedObjectsData, modifier.Value);
        Scripting::ObjectsLookupIdMapping.Set(nullptr);
    }

    // Delete objects without parent
    for (int32 i = 1; i < sceneObjects->Count(); i++)
    {
        SceneObject* obj = sceneObjects->At(i);
        if (obj && obj->GetParent() == nullptr)
        {
            sceneObjects->At(i) = nullptr;
            LOG(Warning, "Scene object {0} {1} has missing parent object after load. Removing it.", obj->GetID(), obj->ToString());
            obj->DeleteObject();
        }
    }

    // Link objects to prefab (only deserialized from prefab data)
    for (int32 i = 0; i < objectsCount; i++)
    {
        auto& stream = data[i];
        SceneObject* obj = prefabDataIndexToSceneObject->At(i);
        if (!obj)
            continue;

        const auto prefabObjectId = JsonTools::GetGuid(stream, "ID");

        if (objectsCache)
            objectsCache->Add(prefabObjectId, dynamic_cast<ScriptingObject*>(obj));
        obj->LinkPrefab(prefabId, prefabObjectId);
    }

    // Update transformations
    root->OnTransformChanged();

    // Spawn if need to
    if (parent && parent->IsDuringPlay())
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
    if ((targetActor->HideFlags & HideFlags::DontSave) != 0)
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
    IsCreatingPrefab = true;
    IsNotCreatingPrefab = false;
    rapidjson_flax::StringBuffer actorsDataBuffer;
    {
        CompactJsonWriter writerObj(actorsDataBuffer);
        JsonWriter& writer = writerObj;
        writer.StartArray();
        for (int32 i = 0; i < sceneObjects->Count(); i++)
        {
            SceneObject* obj = sceneObjects.Value->At(i);
            writer.SceneObject(obj);
        }
        writer.EndArray();
    }
    IsCreatingPrefab = false;
    IsNotCreatingPrefab = true;

    // Randomize the objects ids (prevent overlapping of the prefab instance objects ids and the prefab objects ids)
    Dictionary<Guid, Guid> objectInstanceIdToPrefabObjectId;
    objectInstanceIdToPrefabObjectId.EnsureCapacity(sceneObjects->Count() * 3);
    if (targetActor->HasParent())
    {
        // Unlink from parent actor
        objectInstanceIdToPrefabObjectId.Add(targetActor->GetParent()->GetID(), Guid::Empty);
    }
    for (int32 i = 0; i < sceneObjects->Count(); i++)
    {
        // Generate new IDs for the prefab objects (other than reference instance used to create prefab)
        const SceneObject* obj = sceneObjects.Value->At(i);
        objectInstanceIdToPrefabObjectId.Add(obj->GetSceneObjectId(), Guid::New());
    }
    {
        // Parse json to DOM document
        rapidjson_flax::Document doc;
        doc.Parse(actorsDataBuffer.GetString(), actorsDataBuffer.GetSize());
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
            SceneObject* obj = sceneObjects.Value->At(i);
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
