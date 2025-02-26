// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Prefab.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Factories/JsonAssetFactory.h"
#include "Engine/Core/Log.h"
#include "Engine/Level/Prefabs/PrefabManager.h"
#include "Engine/Level/Actor.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Scripting/Scripting.h"

REGISTER_JSON_ASSET(Prefab, "FlaxEngine.Prefab", true);

Prefab::Prefab(const SpawnParams& params, const AssetInfo* info)
    : JsonAssetBase(params, info)
    , _isCreatingDefaultInstance(false)
    , _defaultInstance(nullptr)
    , ObjectsCount(0)
{
}

Guid Prefab::GetRootObjectId() const
{
    ASSERT(IsLoaded());
    ScopeLock lock(Locker);

    // Root is always the first but handle case when prefab root was reordered in the base prefab while the nested prefab has still the old state
    // TODO: resave and force sync prefabs during game cooking so this step could be skipped in game
    int32 objectIndex = 0;
    if (NestedPrefabs.HasItems())
    {
        const auto& data = *Data;
        const Guid basePrefabId = JsonTools::GetGuid(data[objectIndex], "PrefabID");
        if (const auto basePrefab = Content::Load<Prefab>(basePrefabId))
        {
            const Guid basePrefabRootId = basePrefab->GetRootObjectId();
            for (int32 i = 0; i < ObjectsCount; i++)
            {
                const Guid prefabObjectId = JsonTools::GetGuid(data[i], "PrefabObjectID");
                if (prefabObjectId == basePrefabRootId)
                {
                    objectIndex = i;
                    break;
                }
            }
        }
    }

    return ObjectsIds[objectIndex];
}

Actor* Prefab::GetDefaultInstance()
{
    ScopeLock lock(Locker);

    // Skip if already created (reuse cached result)
    if (_defaultInstance)
        return _defaultInstance;

    // Skip if not loaded
    if (!IsLoaded())
    {
        LOG(Warning, "Cannot instantiate object from not loaded prefab asset.");
        return nullptr;
    }

    // Prevent recursive calls
    if (_isCreatingDefaultInstance)
    {
        LOG(Warning, "Loop call to Prefab::GetDefaultInstance.");
        return nullptr;
    }
    _isCreatingDefaultInstance = true;

    // Instantiate objects from prefab (default spawning logic)
    _defaultInstance = PrefabManager::SpawnPrefab(this, nullptr, &ObjectsCache);

    _isCreatingDefaultInstance = false;
    return _defaultInstance;
}

SceneObject* Prefab::GetDefaultInstance(const Guid& objectId)
{
    const auto result = GetDefaultInstance();
    if (!result)
        return nullptr;
    if (objectId.IsValid())
    {
        SceneObject* object;
        if (ObjectsCache.TryGet(objectId, object))
            return object;
    }
    return result;
}

bool Prefab::GetNestedObject(const Guid& objectId, Guid& outPrefabId, Guid& outObjectId) const
{
    if (WaitForLoaded())
        return false;
    bool result = false;
    Guid result1 = Guid::Empty, result2 = Guid::Empty;
    const ISerializable::DeserializeStream** prefabObjectDataPtr = ObjectsDataCache.TryGet(objectId);
    if (prefabObjectDataPtr)
    {
        const ISerializable::DeserializeStream& prefabObjectData = **prefabObjectDataPtr;
        result = JsonTools::GetGuidIfValid(result1, prefabObjectData, "PrefabID") &&
                JsonTools::GetGuidIfValid(result2, prefabObjectData, "PrefabObjectID");
    }
    outPrefabId = result1;
    outObjectId = result2;
    return result;
}

void Prefab::DeleteDefaultInstance()
{
    ScopeLock lock(Locker);
    ObjectsCache.Clear();
    if (_defaultInstance)
    {
        _defaultInstance->DeleteObject();
        _defaultInstance = nullptr;
    }
}

Asset::LoadResult Prefab::loadAsset()
{
    // Base
    const auto result = JsonAssetBase::loadAsset();
    if (result != LoadResult::Ok)
        return result;

    // Validate data schema
    if (!Data->IsArray())
    {
        LOG(Warning, "Invalid prefab data.");
        return LoadResult::InvalidData;
    }

    // Get objects amount
    const int32 objectsCount = Data->GetArray().Size();
    if (objectsCount <= 0)
    {
        LOG(Warning, "Prefab is empty or has invalid amount of objects.");
        return LoadResult::InvalidData;
    }

    // Allocate memory for objects
    ObjectsIds.EnsureCapacity(objectsCount * 2);
    ObjectsDataCache.EnsureCapacity(objectsCount * 3);

    // Find serialized object ids (actors and scripts), they are used later for IDs mapping on prefab spawning via PrefabManager
    const auto& data = *Data;
    for (int32 objectIndex = 0; objectIndex < objectsCount; objectIndex++)
    {
        auto& objData = data[objectIndex];

        Guid objectId = JsonTools::GetGuid(objData, "ID");
        if (!objectId.IsValid())
        {
            LOG(Warning, "The object inside prefab has invalid ID.");
            return LoadResult::InvalidData;
        }

        ObjectsIds.Add(objectId);
        ObjectsDataCache.Add(objectId, &objData);
        ObjectsCount++;

        Guid parentID;
        if (JsonTools::GetGuidIfValid(parentID, objData, "ParentID"))
            ObjectsHierarchyCache[parentID].Add(objectId);

        Guid prefabId = JsonTools::GetGuid(objData, "PrefabID");
        if (prefabId.IsValid() && !NestedPrefabs.Contains(prefabId))
        {
            if (prefabId == _id)
            {
                LOG(Error, "Circural reference in prefab.");
                return LoadResult::InvalidData;
            }

            NestedPrefabs.Add(prefabId);
        }
    }

    // Register for scripts reload and unload (need to cleanup all user objects including scripts that may be attached to the default instance - it can be always restored)
    Scripting::ScriptsUnload.Bind<Prefab, &Prefab::DeleteDefaultInstance>(this);
#if USE_EDITOR
    Scripting::ScriptsReloading.Bind<Prefab, &Prefab::DeleteDefaultInstance>(this);
#endif

    return LoadResult::Ok;
}

void Prefab::unload(bool isReloading)
{
    // Unlink
    Scripting::ScriptsUnload.Unbind<Prefab, &Prefab::DeleteDefaultInstance>(this);
#if USE_EDITOR
    Scripting::ScriptsReloading.Unbind<Prefab, &Prefab::DeleteDefaultInstance>(this);
#endif

    // Base
    JsonAssetBase::unload(isReloading);

    ObjectsCount = 0;
    ObjectsIds.Resize(0);
    NestedPrefabs.Resize(0);
    ObjectsDataCache.Clear();
    ObjectsDataCache.SetCapacity(0);
    ObjectsHierarchyCache.Clear();
    ObjectsHierarchyCache.SetCapacity(0);
    ObjectsCache.Clear();
    ObjectsCache.SetCapacity(0);
    if (_defaultInstance)
    {
        _defaultInstance->DeleteObject();
        _defaultInstance = nullptr;
    }
}
