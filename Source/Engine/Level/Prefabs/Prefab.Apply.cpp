// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Prefab.h"

#if USE_EDITOR

#include "Engine/Core/ObjectsRemovalService.h"
#include "Engine/Core/Cache.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/Script.h"
#include "Engine/Serialization/Json.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Level/Actor.h"
#include "Engine/Level/ActorsCache.h"
#include "Engine/Level/SceneQuery.h"
#include "Engine/Level/SceneObjectsFactory.h"
#include "Engine/Level/Prefabs/PrefabManager.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Cache/AssetsCache.h"
#include "Engine/ContentImporters/CreateJson.h"
#include "Engine/Debug/Exceptions/ArgumentNullException.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Editor/Editor.h"

// Apply flow:
// - collect all prefabs using this prefab (load and create default instances)
// - serialize target actors (get actual changes including modifications and new objects or removed objects)
// - cache prefab instances state
// - create pure default instance and apply changes
// - save pure default instance
// - update prefab asset
// - sync prefab instances
// - sync nested prefabs

// Sync flow:
// - cache prefab instances state
// - create pure default instance and apply local prefab changes
// - save pure default instance
// - update prefab asset
// - sync prefab instances
// - sync nested prefabs

// Just typedef these monster things
typedef CollectionPoolCache<ActorsCache::ActorsLookupType>::ScopeCache SceneObjectsLookupCacheType;
typedef CollectionPoolCache<ActorsCache::SceneObjectsListType>::ScopeCache SceneObjectsListCacheType;
typedef CollectionPoolCache<ISerializeModifier, Cache::ISerializeModifierClearCallback>::ScopeCache ISerializeModifierCacheType;
typedef Dictionary<Guid, const ISerializable::DeserializeStream*> IdToDataLookupType;

struct AutoActorCleanup
{
    Actor* Ptr;

    AutoActorCleanup(Actor* ptr)
    {
        Ptr = ptr;
    }

    ~AutoActorCleanup()
    {
        Ptr->DeleteObject();
    }
};

/// <summary>
/// The temporary data container for the prefab instance to restore its local changes after prefab synchronization.
/// </summary>
class PrefabInstanceData
{
public:

    // Don't copy or anything
    PrefabInstanceData()
    {
    }

    PrefabInstanceData(const PrefabInstanceData&)
    {
    }

    PrefabInstanceData& operator=(const PrefabInstanceData&)
    {
        return *this;
    }

    ~PrefabInstanceData()
    {
    }

public:

    /// <summary>
    /// The prefab instance root actor.
    /// </summary>
    Actor* TargetActor;

    /// <summary>
    /// The cached order in parent of the target actor. Used to preserve it after prefab changes synchronization.
    /// </summary>
    int32 OrderInParent;

    /// <summary>
    /// The serialized array of scene objects from the prefab instance (the first item is a root actor).
    /// </summary>
    rapidjson_flax::Document Data;

    /// <summary>
    /// The mapping from prefab instance object id to serialized objects array index (in Data)
    /// </summary>
    Dictionary<Guid, int32> PrefabInstanceIdToDataIndex;

public:

    /// <summary>
    /// Collects all the valid prefab instances to update on prefab data synchronization.
    /// </summary>
    /// <param name="prefabInstancesData">The prefab instances data (result buffer).</param>
    /// <param name="prefabId">The prefab asset identifier.</param>
    /// <param name="defaultInstance">The default instance (prefab internal, can be null).</param>
    /// <param name="targetActor">The target actor (optional actor to skip for counting, can be null).</param>
    static void CollectPrefabInstances(Array<PrefabInstanceData>& prefabInstancesData, const Guid& prefabId, Actor* defaultInstance, Actor* targetActor);

    /// <summary>
    /// Serializes all the prefab instances local changes to restore on prefab data synchronization.
    /// </summary>
    /// <param name="prefabInstancesData">The prefab instances data.</param>
    /// <param name="tmpBuffer">The temporary json buffer (cleared before and after usage).</param>
    /// <param name="sceneObjects">The scene objects collection cache (cleared before and after usage).</param>
    static void SerializePrefabInstances(Array<PrefabInstanceData>& prefabInstancesData, rapidjson_flax::StringBuffer& tmpBuffer, SceneObjectsListCacheType& sceneObjects);

    /// <summary>
    /// Synchronizes the prefab instances by applying changes from the diff data and restoring the local changes captured by SerializePrefabInstances.
    /// </summary>
    /// <param name="prefabInstancesData">The prefab instances data.</param>
    /// <param name="sceneObjects">The scene objects (temporary cache).</param>
    /// <param name="prefabId">The prefab asset identifier.</param>
    /// <param name="prefabObjectIdToDiffData">The hash table that maps the prefab object id to json data for the given prefab object.</param>
    /// <param name="newPrefabObjectIds">The collection of the new prefab objects ids added to prefab during changes synchronization or modifications apply.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool SynchronizePrefabInstances(Array<PrefabInstanceData>& prefabInstancesData, SceneObjectsListCacheType& sceneObjects, const Guid& prefabId, const IdToDataLookupType& prefabObjectIdToDiffData, const Array<Guid>& newPrefabObjectIds);

    /// <summary>
    /// Synchronizes the prefab instances by applying changes from the diff data and restoring the local changes captured by SerializePrefabInstances.
    /// </summary>
    /// <param name="prefabInstancesData">The prefab instances data.</param>
    /// <param name="sceneObjects">The scene objects (temporary cache).</param>
    /// <param name="prefabId">The prefab asset identifier.</param>
    /// <param name="tmpBuffer">The temporary json buffer (cleared before and after usage).</param>
    /// <param name="oldObjectsIds">Collection with ids of the objects (actors and scripts) from the prefab before changes apply. Used to find new objects or old objects and use this information during changes sync (eg. generate ids for the new objects to prevent ids collisions).</param>
    /// <param name="newObjectIds">Collection with ids of the objects (actors and scripts) from the prefab after changes apply. Used to find new objects or old objects and use this information during changes sync (eg. generate ids for the new objects to prevent ids collisions).</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool SynchronizePrefabInstances(Array<PrefabInstanceData>& prefabInstancesData, SceneObjectsListCacheType& sceneObjects, const Guid& prefabId, rapidjson_flax::StringBuffer& tmpBuffer, const Array<Guid>& oldObjectsIds, const Array<Guid>& newObjectIds);
};

void PrefabInstanceData::CollectPrefabInstances(Array<PrefabInstanceData>& prefabInstancesData, const Guid& prefabId, Actor* defaultInstance, Actor* targetActor)
{
    ScopeLock lock(PrefabManager::PrefabsReferencesLocker);
    if (PrefabManager::PrefabsReferences.ContainsKey(prefabId))
    {
        // Skip the input targetActor and prefab defaultInstance if exists
        auto& instances = PrefabManager::PrefabsReferences[prefabId];
        int32 usedCount = 0;
        for (int32 instanceIndex = 0; instanceIndex < instances.Count(); instanceIndex++)
        {
            const auto instance = instances[instanceIndex];
            if (instance != defaultInstance && instance != targetActor)
                usedCount++;
        }
        prefabInstancesData.Resize(usedCount);

        int32 dataIndex = 0;
        for (int32 instanceIndex = 0; instanceIndex < instances.Count(); instanceIndex++)
        {
            // Skip default instance because it will be recreated, skip input actor because it needs just to be linked
            const auto instance = instances[instanceIndex];
            if (instance != defaultInstance && instance != targetActor)
            {
                auto& data = prefabInstancesData[dataIndex++];
                data.TargetActor = instance;
                data.OrderInParent = instance->GetOrderInParent();

#if BUILD_DEBUG
                // Check if actor has not been deleted (the memory access exception could fire)
                ASSERT(data.TargetActor->GetID() == data.TargetActor->GetID());
#endif
            }
        }
    }
}

void PrefabInstanceData::SerializePrefabInstances(Array<PrefabInstanceData>& prefabInstancesData, rapidjson_flax::StringBuffer& tmpBuffer, SceneObjectsListCacheType& sceneObjects)
{
    if (prefabInstancesData.IsEmpty())
        return;

    for (int32 dataIndex = 0; dataIndex < prefabInstancesData.Count(); dataIndex++)
    {
        auto& instance = prefabInstancesData[dataIndex];

        // Get scene objects in the prefab instance
        sceneObjects->Clear();
        SceneQuery::GetAllSerializableSceneObjects(instance.TargetActor, *sceneObjects.Value);

        // TODO: could be optimized by doing serialization and changes restore only for scene objects with a prefab linkage to this prefab

        // Serialize
        tmpBuffer.Clear();
        CompactJsonWriter writerObj(tmpBuffer);
        JsonWriter& writer = writerObj;
        writer.StartArray();
        for (int32 i = 0; i < sceneObjects->Count(); i++)
        {
            SceneObject* obj = sceneObjects.Value->At(i);
            writer.SceneObject(obj);
        }
        writer.EndArray();

        // Parse json to get DOM
        instance.Data.Parse(tmpBuffer.GetString(), tmpBuffer.GetSize());
        if (instance.Data.HasParseError())
        {
            LOG(Warning, "Failed to parse serialized scene objects data.");
            continue;
        }

        // Build acceleration table
        instance.PrefabInstanceIdToDataIndex.EnsureCapacity(sceneObjects->Count() * 4);
        for (int32 i = 0; i < sceneObjects->Count(); i++)
        {
            SceneObject* obj = sceneObjects.Value->At(i);
            instance.PrefabInstanceIdToDataIndex.Add(obj->GetSceneObjectId(), i);
        }
    }

    sceneObjects->Clear();
    tmpBuffer.Clear();
}

bool PrefabInstanceData::SynchronizePrefabInstances(Array<PrefabInstanceData>& prefabInstancesData, SceneObjectsListCacheType& sceneObjects, const Guid& prefabId, const IdToDataLookupType& prefabObjectIdToDiffData, const Array<Guid>& newPrefabObjectIds)
{
    for (int32 instanceIndex = 0; instanceIndex < prefabInstancesData.Count(); instanceIndex++)
    {
        auto& instance = prefabInstancesData[instanceIndex];
        ISerializeModifierCacheType modifier = Cache::ISerializeModifier.Get();
        Scripting::ObjectsLookupIdMapping.Set(&modifier.Value->IdsMapping);

        // Get scene objects in the prefab instance
        sceneObjects->Clear();
        SceneQuery::GetAllSerializableSceneObjects(instance.TargetActor, *sceneObjects.Value);

        int32 existingObjectsCount = sceneObjects->Count();
        modifier->IdsMapping.EnsureCapacity((existingObjectsCount + newPrefabObjectIds.Count()) * 4);

        // Map prefab objects to the prefab instance objects
        for (int32 i = 0; i < existingObjectsCount; i++)
        {
            SceneObject* obj = sceneObjects.Value->At(i);
            if (obj->HasPrefabLink())
            {
                // Special case for nested prefabs if one of the objects in nested prefab gets reparented then prefab using it gets duplicated objects due to waterfall synchronization
                if (modifier.Value->IdsMapping.ContainsKey(obj->GetPrefabObjectID()))
                {
                    // Remove object
                    LOG(Info, "Removing object {0} from instance {1} (prefab: {2})", obj->GetSceneObjectId(), instance.TargetActor->ToString(), prefabId);

                    dynamic_cast<RemovableObject*>(obj)->DeleteObject();
                    obj->SetParent(nullptr);

                    sceneObjects.Value->RemoveAtKeepOrder(i);
                    existingObjectsCount--;
                    i--;

                    continue;
                }

                modifier.Value->IdsMapping.Add(obj->GetPrefabObjectID(), obj->GetSceneObjectId());
            }
        }

        // Generate new IDs for the added objects (objects in prefab has to have a unique Ids, other than the targetActor instance objects to prevent Id collisions)
        for (int32 i = 0; i < newPrefabObjectIds.Count(); i++)
        {
            modifier->IdsMapping.Add(newPrefabObjectIds[i], Guid::New());
        }

        // Apply modifications
        for (int32 i = existingObjectsCount - 1; i >= 0; i--)
        {
            SceneObject* obj = sceneObjects->At(i);

            if (obj->HasPrefabLink() && obj->GetPrefabID() == prefabId)
            {
                const ISerializable::DeserializeStream* data;
                if (prefabObjectIdToDiffData.TryGet(obj->GetPrefabObjectID(), data))
                {
                    // Apply prefab changes
                    obj->Deserialize(*(ISerializable::DeserializeStream*)data, modifier.Value);
                }
                else
                {
                    // Remove object removed from the prefab
                    LOG(Info, "Removing prefab instance object {0} from instance {1} (prefab object: {2}, prefab: {3})", obj->GetSceneObjectId(), instance.TargetActor->ToString(), obj->GetPrefabObjectID(), prefabId);

                    dynamic_cast<RemovableObject*>(obj)->DeleteObject();
                    obj->SetParent(nullptr);

                    sceneObjects.Value->RemoveAtKeepOrder(i);
                    existingObjectsCount--;
                }
            }
        }

        // Create new objects added to prefab
        int32 deserializeSceneObjectIndex = sceneObjects->Count();
        for (int32 i = 0; i < newPrefabObjectIds.Count(); i++)
        {
            const Guid prefabObjectId = newPrefabObjectIds[i];
            const ISerializable::DeserializeStream* data;
            if (!prefabObjectIdToDiffData.TryGet(prefabObjectId, data))
            {
                LOG(Warning, "Missing object linkage to the prefab object diff data.");
                continue;
            }

            SceneObject* obj = SceneObjectsFactory::Spawn(*(ISerializable::DeserializeStream*)data, modifier.Value);
            if (!obj)
                continue;
            obj->RegisterObject();
            sceneObjects->Add(obj);
        }

        // Deserialize new objects added to prefab
        for (int32 i = 0; i < newPrefabObjectIds.Count(); i++)
        {
            const Guid prefabObjectId = newPrefabObjectIds[i];
            const ISerializable::DeserializeStream* data;
            if (!prefabObjectIdToDiffData.TryGet(prefabObjectId, data))
                continue;

            SceneObject* obj = sceneObjects->At(deserializeSceneObjectIndex);
            SceneObjectsFactory::Deserialize(obj, *(ISerializable::DeserializeStream*)data, modifier.Value);

            // Link new prefab instance to prefab and prefab object
            obj->LinkPrefab(prefabId, prefabObjectId);

            deserializeSceneObjectIndex++;
        }

        ObjectsRemovalService::Flush();

        // Restore local changes (for the existing scene objects)
        for (int32 i = 0; i < sceneObjects->Count(); i++)
        {
            SceneObject* obj = sceneObjects->At(i);

            int32 dataIndex;
            if (instance.PrefabInstanceIdToDataIndex.TryGet(obj->GetSceneObjectId(), dataIndex))
            {
                obj->Deserialize(instance.Data[dataIndex], modifier.Value);

                // Send events because some properties may be modified during prefab changes apply
                // TODO: maybe send only valid events (need to track changes for before-after state)
                Actor* actor = dynamic_cast<Actor*>(obj);
                if (actor && actor->IsDuringPlay())
                {
                    Level::callActorEvent(Level::ActorEventType::OnActorNameChanged, actor, nullptr);
                    Level::callActorEvent(Level::ActorEventType::OnActorActiveChanged, actor, nullptr);
                    Level::callActorEvent(Level::ActorEventType::OnActorOrderInParentChanged, actor, nullptr);
                }
            }
        }

        Scripting::ObjectsLookupIdMapping.Set(nullptr);

        // Setup objects after deserialization
        for (int32 i = existingObjectsCount; i < sceneObjects->Count(); i++)
        {
            SceneObject* obj = sceneObjects.Value->At(i);
            obj->PostLoad();
        }

        // Restore order in parent
        instance.TargetActor->SetOrderInParent(instance.OrderInParent);

        // Update transformations
        instance.TargetActor->OnTransformChanged();

        // Spawn new objects (add to gameplay)
        {
            SceneBeginData beginData;
            for (int32 i = existingObjectsCount; i < sceneObjects->Count(); i++)
            {
                SceneObject* obj = sceneObjects.Value->At(i);
                if (!obj->IsDuringPlay() && sceneObjects->Find(obj->GetParent()) < i)
                {
                    obj->BeginPlay(&beginData);

                    if (Script* script = dynamic_cast<Script*>(obj))
                    {
                        if (Editor::IsPlayMode || script->_executeInEditor)
                            script->OnAwake();
                        if (script->GetParent() && !script->_wasEnableCalled && script->GetParent()->IsActiveInHierarchy() && script->GetParent()->GetScene())
                            script->Enable();
                    }
                }
            }
            beginData.OnDone();
            for (int32 i = existingObjectsCount; i < sceneObjects->Count(); i++)
            {
                Actor* actor = dynamic_cast<Actor*>(sceneObjects.Value->At(i));
                if (actor)
                    Level::callActorEvent(Level::ActorEventType::OnActorSpawned, actor, nullptr);
            }
        }
    }

    LOG(Info, "Prefab synced! ({0} instances)", prefabInstancesData.Count());

    return false;
}

bool PrefabInstanceData::SynchronizePrefabInstances(Array<PrefabInstanceData>& prefabInstancesData, SceneObjectsListCacheType& sceneObjects, const Guid& prefabId, rapidjson_flax::StringBuffer& tmpBuffer, const Array<Guid>& oldObjectsIds, const Array<Guid>& newObjectIds)
{
    // Fully serialize default instance scene objects (accumulate all prefab and nested prefabs changes into a single linear list of objects)
    rapidjson_flax::Document defaultInstanceData;
    {
        // Serialize
        tmpBuffer.Clear();
        CompactJsonWriter writerObj(tmpBuffer);
        JsonWriter& writer = writerObj;
        writer.StartArray();
        for (int32 i = 0; i < sceneObjects->Count(); i++)
        {
            SceneObject* obj = sceneObjects.Value->At(i);

            // Full serialization - no prefab diff, always all non-default properties
            writer.StartObject();
            obj->Serialize(writer, nullptr);
            writer.EndObject();
        }
        writer.EndArray();

        // Parse json to get DOM
        defaultInstanceData.Parse(tmpBuffer.GetString(), tmpBuffer.GetSize());
        if (defaultInstanceData.HasParseError())
        {
            LOG(Warning, "Failed to parse serialized scene objects data.");
            return true;
        }
    }

    // Find new objects
    Array<Guid> newPrefabObjectIds;
    newPrefabObjectIds.EnsureCapacity(Math::Max(32, Math::Abs(newObjectIds.Count() - oldObjectsIds.Count()) * 4));
    for (int32 i = 0; i < newObjectIds.Count(); i++)
    {
        const Guid id = newObjectIds[i];
        if (!oldObjectsIds.Contains(id))
        {
            newPrefabObjectIds.Add(id);
        }
    }

#if 0
	// Strip unwanted properties from the default instance (extract only diff to apply)
	{
		const auto array = defaultInstanceData.GetArray();
		int32 i = 0;
		for (auto it = array.Begin(); it != array.End(); ++it, i++)
		{
			auto data = it->GetObject();

			// Skip for added new objects (need to link them and get all of their data)
			SceneObject* obj = sceneObjects.Value->At(i);
			if (newPrefabObjectIds.Contains(obj->GetSceneObjectId()))
				continue;

			// Strip unwanted data
			data.RemoveMember("ParentID");
		}
	}
#endif

    // Build cache data
    IdToDataLookupType prefabObjectIdToDiffData;
    prefabObjectIdToDiffData.EnsureCapacity(defaultInstanceData.Size() * 3);
    for (int32 i = 0; i < sceneObjects->Count(); i++)
    {
        SceneObject* obj = sceneObjects.Value->At(i);
        prefabObjectIdToDiffData.Add(obj->GetSceneObjectId(), &defaultInstanceData[i]);
    }

    // Process prefab instances to synchronize changes
    return SynchronizePrefabInstances(prefabInstancesData, sceneObjects, prefabId, prefabObjectIdToDiffData, newPrefabObjectIds);
}

bool FindCyclicReferences(Actor* actor, const Guid& prefabRootId)
{
    for (int32 i = 0; i < actor->Children.Count(); i++)
    {
        const auto child = actor->Children[i];

        if (child->GetPrefabObjectID() == prefabRootId || FindCyclicReferences(child, prefabRootId))
            return true;
    }

    return false;
}

bool Prefab::ApplyAll(Actor* targetActor)
{
    // TODO: use more cached dictionaries and other collections containers to prevent memory allocations during apply (optimize for apply 10 times per second the same prefab on many changes in editor)

    PROFILE_CPU();
    const auto startTime = DateTime::NowUTC();

    // Validate input
    if (!IsLoaded())
    {
        Log::Exception(TEXT("Cannot apply changes on not loaded prefab asset."));
        return true;
    }
    if (targetActor == nullptr)
    {
        Log::ArgumentNullException();
        return true;
    }
    if (targetActor->GetPrefabID() != GetID())
    {
        Log::Exception(TEXT("Cannot apply changes to the prefab. Prefab instance root object has link to the other prefab."));
        return true;
    }
    if (targetActor->GetPrefabObjectID() != GetRootObjectId())
    {
        LOG(Warning, "Applying prefab changes with modified root object. Root object id: {0}, new root: {1}", GetRootObjectId().ToString(), targetActor->ToString());
    }

    // Ensure to have default instance created
    if (GetDefaultInstance() == nullptr)
    {
        LOG(Warning, "Failed to create default prefab instance for the prefab asset.");
        return true;
    }

    // Prevent cyclic references
    {
        PROFILE_CPU_NAMED("Prefab.FindCyclicReferences");

        ASSERT(GetDefaultInstance() != nullptr);
        if (FindCyclicReferences(targetActor, targetActor->GetPrefabObjectID()))
        {
            Log::Exception(TEXT("Cannot apply changes to the prefab. Cyclic reference found in the actor."));
            return true;
        }
    }

    // Collect all prefabs that use this prefab, load them and create default instance for each prefab
    // To apply changes in a proper way the default instance is required to preserve the local modification applied to the nested prefab
    NestedPrefabsList allPrefabs;
    {
        PROFILE_CPU_NAMED("Prefab.CollectNestedPrefabs");

        // Get all prefab assets ids from project
        Array<Guid> nestedPrefabIds;
        Content::GetRegistry()->GetAllByTypeName(Prefab::TypeName, nestedPrefabIds);

        // Assign references to the prefabs
        allPrefabs.EnsureCapacity(Math::RoundUpToPowerOf2(Math::Max(30, nestedPrefabIds.Count())));
        for (int32 i = 0; i < nestedPrefabIds.Count(); i++)
        {
            const auto nestedPrefab = Content::LoadAsync<Prefab>(nestedPrefabIds[i]);
            if (nestedPrefab && nestedPrefab != this)
            {
                allPrefabs.Add(nestedPrefab);
            }
        }

        // Setup default instances
        for (int32 i = 0; i < allPrefabs.Count(); i++)
        {
            Prefab* prefab = allPrefabs[i];
            if (prefab->WaitForLoaded())
            {
                LOG(Warning, "Waiting for nesting prefab asset load failed.");
                return true;
            }
            if (prefab->GetDefaultInstance() == nullptr)
            {
                LOG(Warning, "Failed to create default prefab instance for the nested prefab asset.");
                return true;
            }
        }
    }

    ObjectsRemovalService::Flush();

    // Use internal call to improve shared collections memory sharing
    if (ApplyAllInternal(targetActor, true))
        return true;

    SyncNestedPrefabs(allPrefabs);

    const auto endTime = DateTime::NowUTC();
    LOG(Info, "Prefab updated! {0} ms", (int32)(endTime - startTime).GetTotalMilliseconds());
    return false;
}

bool Prefab::ApplyAllInternal(Actor* targetActor, bool linkTargetActorObjectToPrefab)
{
    PROFILE_CPU_NAMED("Prefab.Apply");
    ScopeLock lock(Locker);

    const auto prefabId = GetID();

    // Gather all scene objects in target instance (reused later)
    CollectionPoolCache<ActorsCache::SceneObjectsListType>::ScopeCache targetObjects = ActorsCache::SceneObjectsListCache.Get();
    targetObjects->EnsureCapacity(ObjectsCount * 4);
    SceneQuery::GetAllSerializableSceneObjects(targetActor, *targetObjects.Value);
    if (PrefabManager::FilterPrefabInstancesToSave(prefabId, *targetObjects.Value))
        return true;

    LOG(Info, "Applying prefab changes from actor {0} (total objects count: {2}) to {1}...", targetActor->ToString(), ToString(), targetObjects->Count());

    Array<Guid> oldObjectsIds = ObjectsIds;

    // Serialize to json data
    rapidjson_flax::StringBuffer dataBuffer;
    {
        CompactJsonWriter writerObj(dataBuffer);
        JsonWriter& writer = writerObj;
        writer.StartArray();
        for (int32 i = 0; i < targetObjects->Count(); i++)
        {
            SceneObject* obj = targetObjects.Value->At(i);
            writer.SceneObject(obj);
        }
        writer.EndArray();
    }

    // Parse json document and modify serialized data to extract only modified properties
    rapidjson_flax::Document diffDataDocument;
    Dictionary<Guid, int32> diffPrefabObjectIdToDataIndex; // Maps Prefab Object Id -> Actor Data index in diffDataDocument json array (for actors/scripts to modify prefab)
    Dictionary<Guid, int32> newPrefabInstanceIdToDataIndex; // Maps Prefab Instance Id -> Actor Data index in diffDataDocument json array (for new actors/scripts to add to prefab), maps to -1 for scripts
    diffPrefabObjectIdToDataIndex.EnsureCapacity(ObjectsCount * 4);
    newPrefabInstanceIdToDataIndex.EnsureCapacity(ObjectsCount * 4);
    {
        // Parse json to DOM document
        diffDataDocument.Parse(dataBuffer.GetString(), dataBuffer.GetSize());
        if (diffDataDocument.HasParseError())
        {
            LOG(Warning, "Failed to parse serialized scene objects data.");
            return true;
        }

        // Process json
        const auto array = diffDataDocument.GetArray();
        int32 i = 0;
        for (auto it = array.Begin(); it != array.End(); ++it, i++)
        {
            SceneObject* obj = targetObjects.Value->At(i);
            auto data = it->GetObject();

            // Check if object is from that prefab
            if (obj->GetPrefabID() == prefabId)
            {
                if (!obj->GetPrefabObjectID().IsValid())
                {
                    LOG(Warning, "One of the target instance objects has missing link to prefab object.");
                    return true;
                }
                if (!ObjectsIds.Contains(obj->GetPrefabObjectID()))
                {
                    LOG(Warning, "One of the target instance objects has link to prefab object that does not exist in prefab.");
                    return true;
                }

                // Cache connection for fast lookup
                diffPrefabObjectIdToDataIndex.Add(obj->GetPrefabObjectID(), i);

                // Strip unwanted data
                data.RemoveMember("ID");
                data.RemoveMember("PrefabID");
                data.RemoveMember("PrefabObjectID");
            }
            else
            {
                // Object if a new thing
                newPrefabInstanceIdToDataIndex.Add(obj->GetSceneObjectId(), i);
            }
        }

        // Change object ids to match the prefab objects ids (helps with linking references in scripts)
        Dictionary<Guid, Guid> objectInstanceIdToPrefabObjectId;
        objectInstanceIdToPrefabObjectId.EnsureCapacity(ObjectsCount * 3);
        i = 0;
        for (auto it = array.Begin(); it != array.End(); ++it, i++)
        {
            SceneObject* obj = targetObjects.Value->At(i);
            auto data = it->GetObject();
            if (obj->GetPrefabID() == prefabId)
            {
                objectInstanceIdToPrefabObjectId.Add(obj->GetSceneObjectId(), obj->GetPrefabObjectID());
            }
        }
        // TODO: what if user applied prefab with references to the other objects from scene? clear them or what?
        JsonTools::ChangeIds(diffDataDocument, objectInstanceIdToPrefabObjectId);
    }

    dataBuffer.Clear();

    // Fetch all existing prefab instances
    Array<PrefabInstanceData> prefabInstancesData;
    PrefabInstanceData::CollectPrefabInstances(prefabInstancesData, prefabId, _defaultInstance, targetActor);

    // Serialize all prefab instances data (temporary storage)
    CollectionPoolCache<ActorsCache::SceneObjectsListType>::ScopeCache sceneObjects = ActorsCache::SceneObjectsListCache.Get();
    sceneObjects->EnsureCapacity(ObjectsCount * 4);
    PrefabInstanceData::SerializePrefabInstances(prefabInstancesData, dataBuffer, sceneObjects);

    // Destroy default instance and some cache data in Prefab
    DeleteDefaultInstance();

    // Create default instance of the prefab (but without a link to this prefab) and apply modifications during deserialization
    Actor* defaultInstance;
    Dictionary<Guid, Guid> newPrefabInstanceIdToPrefabObjectId; // Maps Prefab Instance Id -> Prefab Object Id (for new actors/scripts to add to prefab), value is a new generated Id of the prefab object for prefab links usage
    {
        // Prepare
        sceneObjects->EnsureCapacity(diffPrefabObjectIdToDataIndex.Count() + newPrefabInstanceIdToDataIndex.Count());
        ISerializeModifierCacheType modifier = Cache::ISerializeModifier.Get();
        Scripting::ObjectsLookupIdMapping.Set(&modifier.Value->IdsMapping);

        // Generate new IDs for the added objects (objects in prefab has to have a unique Ids, other than the targetActor instance objects to prevent Id collisions)
        newPrefabInstanceIdToPrefabObjectId.EnsureCapacity(newPrefabInstanceIdToDataIndex.Count() * 4);
        for (auto i = newPrefabInstanceIdToDataIndex.Begin(); i.IsNotEnd(); ++i)
        {
            const auto prefabObjectId = Guid::New();
            newPrefabInstanceIdToPrefabObjectId.Add(i->Key, prefabObjectId);
            modifier->IdsMapping.Add(i->Key, prefabObjectId);
        }

        // Add inverse IDs mapping to link added objects and references inside them to the prefab objects
        for (int32 i = 0; i < targetObjects->Count(); i++)
        {
            const SceneObject* obj = targetObjects->At(i);

            // Check if object is from that prefab
            if (obj->GetPrefabID() == prefabId)
            {
                // Map prefab instance to existing prefab object
                modifier->IdsMapping.Add(obj->GetSceneObjectId(), obj->GetPrefabObjectID());
            }
            else
            {
                // Map prefab instance to new prefab object
                // <already added via newPrefabInstanceIdToDataIndex>
            }
        }

        // Deserialize prefab objects (apply modifications)
        auto& data = *Data;
        sceneObjects->Resize(ObjectsCount);
        for (int32 i = 0; i < ObjectsCount; i++)
        {
            SceneObject* obj = SceneObjectsFactory::Spawn(data[i], modifier.Value);
            sceneObjects->At(i) = obj;
            if (!obj)
            {
                // This may happen if nested prefab has missing or invalid object but it still exists in this prefab (need to skip it)
                SceneObjectsFactory::HandleObjectDeserializationError(data[i]);
                continue;
            }
            obj->RegisterObject();
        }
        for (int32 i = 0; i < ObjectsCount; i++)
        {
            SceneObject* obj = sceneObjects->At(i);
            if (!obj)
                continue;
            SceneObjectsFactory::Deserialize(obj, data[i], modifier.Value);

            int32 dataIndex;
            if (diffPrefabObjectIdToDataIndex.TryGet(obj->GetSceneObjectId(), dataIndex))
            {
                obj->Deserialize(diffDataDocument[dataIndex], modifier.Value);

                sceneObjects->Add(obj);
            }
            else
            {
                // Remove object removed from the prefab
                LOG(Info, "Removing object {0} from prefab default instance", obj->GetSceneObjectId());

                dynamic_cast<RemovableObject*>(obj)->DeleteObject();
                obj->SetParent(nullptr);
            }
        }
        for (int32 i = 0; i < sceneObjects->Count(); i++)
        {
            if (sceneObjects->At(i) == nullptr)
                sceneObjects->RemoveAtKeepOrder(i);
        }

        // Deserialize new prefab objects (add new objects)
        int32 newPrefabInstanceIdToDataIndexCounter = 0;
        int32 newPrefabInstanceIdToDataIndexStart = sceneObjects->Count();
        sceneObjects->Resize(sceneObjects->Count() + newPrefabInstanceIdToDataIndex.Count());
        for (auto i = newPrefabInstanceIdToDataIndex.Begin(); i.IsNotEnd(); ++i)
        {
            const int32 dataIndex = i->Value;
            SceneObject* obj = SceneObjectsFactory::Spawn(diffDataDocument[dataIndex], modifier.Value);
            sceneObjects->At(newPrefabInstanceIdToDataIndexStart + newPrefabInstanceIdToDataIndexCounter++) = obj;
            if (!obj)
            {
                // This should not happen but who knows
                SceneObjectsFactory::HandleObjectDeserializationError(diffDataDocument[dataIndex]);
                continue;
            }
            obj->RegisterObject();
        }
        newPrefabInstanceIdToDataIndexCounter = 0;
        for (auto i = newPrefabInstanceIdToDataIndex.Begin(); i.IsNotEnd(); ++i)
        {
            const int32 dataIndex = i->Value;
            SceneObject* obj = sceneObjects->At(newPrefabInstanceIdToDataIndexStart + newPrefabInstanceIdToDataIndexCounter++);
            if (obj)
            {
                SceneObjectsFactory::Deserialize(obj, diffDataDocument[dataIndex], modifier.Value);
            }
        }
        Scripting::ObjectsLookupIdMapping.Set(nullptr);
        if (sceneObjects.Value->IsEmpty())
        {
            LOG(Warning, "No valid objects in prefab.");
            return true;
        }

        // Find the prefab root object (the root is usually serialized first)
        auto root = dynamic_cast<Actor*>(sceneObjects.Value->At(0));
        if (root && root->_parent)
        {
            for (int32 i = 1; i < sceneObjects->Count(); i++)
            {
                SceneObject* obj = sceneObjects.Value->At(i);
                auto actor = dynamic_cast<Actor*>(obj);
                if (actor && !actor->_parent)
                {
                    root = actor;
                    break;
                }
            }

            // Keep root unlinked
            if (root->_parent)
            {
                root->_parent->Children.Remove(root);
                root->_parent = nullptr;
            }
        }

        // Link objects hierarchy
        for (int32 i = 0; i < sceneObjects->Count(); i++)
        {
            auto obj = sceneObjects.Value->At(i);
            obj->PostLoad();
        }

        // Update transformations
        root->OnTransformChanged();

        defaultInstance = root;
    }

    // Ensure to delete the spawned default instance with diff applied
    AutoActorCleanup cleanupDefaultInstance(defaultInstance);

    // Gather all default instance actors
    sceneObjects->Clear();
    SceneQuery::GetAllSerializableSceneObjects(defaultInstance, *sceneObjects.Value);

    // Refresh asset data
    if (UpdateInternal(*sceneObjects.Value, dataBuffer))
        return true;

    // Refresh all prefab instances (using the cached data)
    LOG(Info, "Reloading prefab instances");
    if (PrefabInstanceData::SynchronizePrefabInstances(prefabInstancesData, sceneObjects, prefabId, dataBuffer, oldObjectsIds, ObjectsIds))
        return true;

    // Link the input objects to the prefab objects (prefab and instance are even, only links for the new objects added to prefab are required)
    if (linkTargetActorObjectToPrefab)
    {
        for (int32 i = 0; i < targetObjects->Count(); i++)
        {
            auto obj = targetObjects->At(i);

            if (obj->GetPrefabID() != prefabId)
            {
                Guid prefabObjectId;
                if (!newPrefabInstanceIdToPrefabObjectId.TryGet(obj->GetSceneObjectId(), prefabObjectId))
                {
                    LOG(Warning, "Missing prefab object linkage in 'NewPrefabInstanceIdToPrefabObjectId' cache table.");
                    return true;
                }

                obj->LinkPrefab(prefabId, prefabObjectId);
            }
        }
    }

    return false;
}

bool Prefab::UpdateInternal(const Array<SceneObject*>& defaultInstanceObjects, rapidjson_flax::StringBuffer& tmpBuffer)
{
    PROFILE_CPU_NAMED("Prefab.UpdateData");

    // Serialize to json data
    {
        tmpBuffer.Clear();
        PrettyJsonWriter writerObj(tmpBuffer);
        JsonWriter& writer = writerObj;
        writer.StartArray();
        for (int32 i = 0; i < defaultInstanceObjects.Count(); i++)
        {
            auto obj = defaultInstanceObjects.At(i);
            writer.SceneObject(obj);
        }
        writer.EndArray();
    }

    LOG(Info, "Updating prefab data");

    // Reload prefab data
#if 1 // Set to 0 to use memory-only reload that does not modifies the source file - useful for testing and debugging prefabs apply
#if COMPILE_WITH_ASSETS_IMPORTER
    Locker.Unlock();

    // Save to file
    if (CreateJson::Create(GetPath(), tmpBuffer, Prefab::TypeName))
    {
        Locker.Lock();
        LOG(Warning, "Failed to serialize prefab data to the asset.");
        return true;
    }

    // Ensure to be loaded
    if (WaitForLoaded())
    {
        Locker.Lock();
        LOG(Warning, "Waiting for prefab asset reload failed.");
        return true;
    }

    Locker.Lock();
#else
#error "Cannot support prefabs creating without assets importing enabled."
#endif
#else
    {
        // Create object data
        rapidjson_flax::StringBuffer buffer;
        PrettyJsonWriter writerObj(buffer);
        JsonWriter& writer = writerObj;
        writer.StartObject();
        {
            // Json resource header
            writer.JKEY("ID");
            writer.Guid(GetID());
            writer.JKEY("TypeName");
            writer.String(TypeName);
            writer.JKEY("EngineBuild");
            writer.Int(FLAXENGINE_VERSION_BUILD);

            // Json resource data
            writer.JKEY("Data");
            writer.RawValue(tmpBuffer.GetString(), (int32)tmpBuffer.GetSize());
        }
        writer.EndObject();

        // Unload
        ObjectsCount = 0;
        ObjectsIds.Resize(0);
        NestedPrefabs.Resize(0);
        ObjectsDataCache.Clear();
        ObjectsDataCache.SetCapacity(0);
        ObjectsCache.Clear();
        ObjectsCache.SetCapacity(0);
        if (_defaultInstance)
        {
            _defaultInstance->DeleteObject();
            _defaultInstance = nullptr;
        }
        _isLoaded = false;

        // Update prefab data manually (to prevent updating source asset file - just for testing)
        Document.Parse(buffer.GetString(), buffer.GetSize());
        if (Document.HasParseError())
            return true;
        const auto id = JsonTools::GetGuid(Document, "ID");
        if (id != _id)
            return true;
        DataTypeName = JsonTools::GetString(Document, "TypeName");
        DataEngineBuild = JsonTools::GetInt(Document, "EngineBuild", FLAXENGINE_VERSION_BUILD);
        auto dataMember = Document.FindMember("Data");
        if (dataMember == Document.MemberEnd())
            return true;
        Data = &dataMember->value;
        if (!Data->IsArray())
            return true;
        const int32 objectsCount = Data->GetArray().Size();
        if (objectsCount <= 0)
            return true;
        ObjectsIds.EnsureCapacity(objectsCount * 2);
        NestedPrefabs.EnsureCapacity(objectsCount);
        ObjectsDataCache.EnsureCapacity(objectsCount * 3);
        const auto& data = *Data;
        for (int32 objectIndex = 0; objectIndex < objectsCount; objectIndex++)
        {
            auto& objData = data[objectIndex];

            Guid objectId = JsonTools::GetGuid(objData, "ID");
            if (!objectId.IsValid())
                return true;

            ObjectsIds.Add(objectId);
            ASSERT(!ObjectsDataCache.ContainsKey(objectId));
            ObjectsDataCache.Add(objectId, &objData);
            ObjectsCount++;

            Guid prefabId = JsonTools::GetGuid(objData, "PrefabID");
            if (prefabId.IsValid() && !NestedPrefabs.Contains(prefabId))
            {
                if (prefabId == _id)
                {
                    LOG(Error, "Circural reference in prefab.");
                    continue;
                }

                NestedPrefabs.Add(prefabId);
            }
        }
        _isLoaded = true;
    }
#endif

    return false;
}

bool Prefab::SyncChanges(const NestedPrefabsList& allPrefabs)
{
    // Use internal call to improve shared collections memory sharing
    if (SyncChangesInternal())
        return true;

    SyncNestedPrefabs(allPrefabs);

    return false;
}

bool Prefab::SyncChangesInternal()
{
    PROFILE_CPU_NAMED("Prefab.SyncChanges");

    LOG(Info, "Syncing prefab {0}", ToString());

    // Ensure to be loaded
    if (WaitForLoaded())
    {
        LOG(Warning, "Waiting for prefab asset load failed.");
        return true;
    }

    // Instantiate prefab instance from prefab (default spawning logic)
    // Note: it will get any added or removed objects from the nested prefabs
    const auto targetActor = PrefabManager::SpawnPrefab(this, nullptr, nullptr, true);
    if (targetActor == nullptr)
    {
        LOG(Warning, "Failed to instantiate default prefab instance from changes synchronization.");
        return true;
    }

    // Ensure to delete the spawned objects instance with diff applied
    AutoActorCleanup cleanupDefaultInstance(targetActor);

    // Apply changes
    return ApplyAllInternal(targetActor, false);
}

void Prefab::SyncNestedPrefabs(const NestedPrefabsList& allPrefabs)
{
    PROFILE_CPU();
    LOG(Info, "Updating referencing prefabs");

    // TODO: this may not work well for very complex prefab nesting -> loop order matters, maybe build a graph of dependencies?

    // Call recursive for all referencing prefab assets to refresh nested prefabs
    for (int32 i = 0; i < allPrefabs.Count(); i++)
    {
        auto nestedPrefab = allPrefabs[i].Get();
        if (nestedPrefab)
        {
            if (WaitForLoaded())
            {
                LOG(Warning, "Waiting for prefab asset load failed.");
                continue;
            }

            if (nestedPrefab->NestedPrefabs.Contains(GetID()))
            {
                nestedPrefab->SyncChanges(allPrefabs);

                ObjectsRemovalService::Flush();
            }
        }
    }
}

#endif
