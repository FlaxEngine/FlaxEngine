// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Prefab.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Content/Factories/JsonAssetFactory.h"
#include "Engine/Core/Log.h"
#include "Engine/Level/Prefabs/PrefabManager.h"
#include "Engine/Level/Actor.h"
#include "Engine/Threading/Threading.h"
#if USE_EDITOR
#include "Engine/Scripting/Scripting.h"
#endif

REGISTER_JSON_ASSET(Prefab, "FlaxEngine.Prefab", false);

Prefab::Prefab(const SpawnParams& params, const AssetInfo* info)
    : JsonAssetBase(params, info)
    , _isCreatingDefaultInstance(false)
    , _defaultInstance(nullptr)
    , ObjectsCount(0)
{
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
        const void* object;
        if (ObjectsCache.TryGet(objectId, object))
        {
            // Actor or Script
            return (SceneObject*)object;
        }
    }

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
    NestedPrefabs.EnsureCapacity(objectsCount);
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
        ASSERT(!ObjectsDataCache.ContainsKey(objectId));
        ObjectsDataCache.Add(objectId, &objData);
        ObjectsCount++;

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

#if USE_EDITOR
    // Register for scripts reload and unload (need to cleanup all user objects including scripts that may be attached to the default instance - it can be always restored)
    Scripting::ScriptsReloading.Bind<Prefab, &Prefab::DeleteDefaultInstance>(this);
    Scripting::ScriptsUnload.Bind<Prefab, &Prefab::DeleteDefaultInstance>(this);
#endif

    return LoadResult::Ok;
}

void Prefab::unload(bool isReloading)
{
#if USE_EDITOR
    // Unlink
    Scripting::ScriptsReloading.Unbind<Prefab, &Prefab::DeleteDefaultInstance>(this);
    Scripting::ScriptsUnload.Unbind<Prefab, &Prefab::DeleteDefaultInstance>(this);
#endif

    // Base
    JsonAssetBase::unload(isReloading);

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
}
