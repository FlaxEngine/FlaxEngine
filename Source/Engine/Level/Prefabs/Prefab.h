// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/JsonAsset.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"

class Actor;
class SceneObject;

/// <summary>
/// Json asset that stores the collection of scene objects including actors and scripts. In general it can serve as any grouping of scene objects (for example a level) or be used as a form of a template instantiated and reused throughout the scene.
/// </summary>
/// <seealso cref="JsonAssetBase" />
API_CLASS(NoSpawn) class FLAXENGINE_API Prefab : public JsonAssetBase
{
DECLARE_ASSET_HEADER(Prefab);
private:

    bool _isCreatingDefaultInstance;
    Actor* _defaultInstance;

public:

    /// <summary>
    /// The serialized scene objects amount (actors and scripts).
    /// </summary>
    int32 ObjectsCount;

    /// <summary>
    /// The objects ids contained within the prefab asset. Valid only if asset is loaded.
    /// </summary>
    Array<Guid> ObjectsIds;

    /// <summary>
    /// The prefab assets ids contained within the prefab asset. Valid only if asset is loaded. Remember that each nested prefab can contain deeper references to the other assets.
    /// </summary>
    Array<Guid> NestedPrefabs;

    /// <summary>
    /// The objects data cache maps the id of the object contained in the prefab asset (actor or script) to the json data node for its data. Valid only if asset is loaded.
    /// </summary>
    Dictionary<Guid, const ISerializable::DeserializeStream*> ObjectsDataCache;

    /// <summary>
    /// The objects cache maps the id of the object contained in the prefab asset (actor or script) to the default instance deserialized from prefab data. Valid only if asset is loaded and GetDefaultInstance was called.
    /// </summary>
    Dictionary<Guid, const void*> ObjectsCache;

public:

    /// <summary>
    /// Gets the root object identifier (prefab object ID). Asset must be loaded.
    /// </summary>
    /// <returns>The ID.</returns>
    Guid GetRootObjectId() const
    {
        ASSERT(IsLoaded());
        return ObjectsIds[0];
    }

    /// <summary>
    /// Requests the default prefab object instance. Deserializes the prefab objects from the asset. Skips if already done.
    /// </summary>
    /// <returns>The root of the prefab object loaded from the prefab. Contains the default values. It's not added to gameplay but deserialized with postLoad and init event fired.</returns>
    API_FUNCTION() Actor* GetDefaultInstance();

    /// <summary>
    /// Requests the default prefab object instance. Deserializes the prefab objects from the asset. Skips if already done.
    /// </summary>
    /// <param name="objectId">The ID of the object to get from prefab default object. It can be one of the child-actors or any script that exists in the prefab. Methods returns root if id is empty.</param>
    /// <returns>The object of the prefab loaded from the prefab. Contains the default values. It's not added to gameplay but deserialized with postLoad and init event fired.</returns>
    API_FUNCTION() SceneObject* GetDefaultInstance(API_PARAM(Ref) const Guid& objectId);

#if USE_EDITOR

    /// <summary>
    /// Applies the difference from the prefab object instance, saves the changes and synchronizes them with the active instances of the prefab asset.
    /// </summary>
    /// <remarks>
    /// Applies all the changes from not only the given actor instance but all actors created within that prefab instance.
    /// </remarks>
    /// <param name="targetActor">The root actor of spawned prefab instance to use as modified changes sources.</param>
    bool ApplyAll(Actor* targetActor);

#endif

private:

#if USE_EDITOR
    typedef Array<AssetReference<Prefab>> NestedPrefabsList;
    bool ApplyAllInternal(Actor* targetActor, bool linkTargetActorObjectToPrefab);
    bool UpdateInternal(const Array<SceneObject*>& defaultInstanceObjects, rapidjson_flax::StringBuffer& tmpBuffer);
    bool SyncChanges(const NestedPrefabsList& allPrefabs);
    bool SyncChangesInternal();
    void SyncNestedPrefabs(const NestedPrefabsList& allPrefabs);
#endif
    void DeleteDefaultInstance();

protected:

    // [JsonAssetBase]
    LoadResult loadAsset() override;
    void unload(bool isReloading) override;
};
