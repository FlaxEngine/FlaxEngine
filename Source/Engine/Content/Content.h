// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"
#include "AssetInfo.h"
#include "Asset.h"
#include "Config.h"

class Engine;
class FlaxFile;
class IAssetFactory;
class AssetsCache;

// Content and assets statistics container.
API_STRUCT() struct FLAXENGINE_API ContentStats
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(ContentStats);

    // Amount of asset objects in memory.
    API_FIELD() int32 AssetsCount = 0;
    // Amount of loaded assets.
    API_FIELD() int32 LoadedAssetsCount = 0;
    // Amount of loading assets. Zero if all assets are loaded in.
    API_FIELD() int32 LoadingAssetsCount = 0;
    // Amount of virtual assets (don't have representation in file).
    API_FIELD() int32 VirtualAssetsCount = 0;
};

/// <summary>
/// Loads and manages assets.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API Content
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(Content);
    friend Engine;
    friend Asset;
public:
    /// <summary>
    /// The time between content pool updates.
    /// </summary>
    static TimeSpan AssetsUpdateInterval;

    /// <summary>
    /// The time after asset with no references will be unloaded.
    /// </summary>
    static TimeSpan AssetsUnloadInterval;

public:
    /// <summary>
    /// Gets the assets registry.
    /// </summary>
    /// <returns>The assets cache.</returns>
    static AssetsCache* GetRegistry();

public:
    /// <summary>
    /// Finds the asset info by id.
    /// </summary>
    /// <param name="id">The asset id.</param>
    /// <param name="info">The output asset info. Filled with valid values only if method returns true.</param>
    /// <returns>True if found any asset, otherwise false.</returns>
    API_FUNCTION() static bool GetAssetInfo(const Guid& id, API_PARAM(Out) AssetInfo& info);

    /// <summary>
    /// Finds the asset info by path.
    /// </summary>
    /// <param name="path">The asset path.</param>
    /// <param name="info">The output asset info. Filled with valid values only if method returns true.</param>
    /// <returns>True if found any asset, otherwise false.</returns>
    API_FUNCTION() static bool GetAssetInfo(const StringView& path, API_PARAM(Out) AssetInfo& info);

    /// <summary>
    /// Finds the asset path by id. In editor it returns the actual asset path, at runtime it returns the mapped asset path.
    /// </summary>
    /// <param name="id">The asset id.</param>
    /// <returns>The asset path, or empty if failed to find.</returns>
    API_FUNCTION() static String GetEditorAssetPath(const Guid& id);

    /// <summary>
    /// Finds all the asset IDs. Uses asset registry.
    /// </summary>
    /// <returns>The list of all asset IDs.</returns>
    API_FUNCTION() static Array<Guid, HeapAllocation> GetAllAssets();

    /// <summary>
    /// Finds all the asset IDs by type (exact type, without inheritance checks). Uses asset registry.
    /// </summary>
    /// <param name="type">The asset type.</param>
    /// <returns>The list of asset IDs that match the given type.</returns>
    API_FUNCTION() static Array<Guid, HeapAllocation> GetAllAssetsByType(const MClass* type);

public:
    /// <summary>
    /// Gets the asset factory used by the given asset type id.
    /// </summary>
    /// <param name="typeName">The asset type name identifier.</param>
    /// <returns>Asset factory or null if not found.</returns>
    static IAssetFactory* GetAssetFactory(const StringView& typeName);

    /// <summary>
    /// Gets the asset factory used by the given asset type id.
    /// </summary>
    /// <param name="assetInfo">The asset info.</param>
    /// <returns>Asset factory or null if not found.</returns>
    static IAssetFactory* GetAssetFactory(const AssetInfo& assetInfo);

public:
    /// <summary>
    /// Generates temporary asset path.
    /// </summary>
    /// <returns>Asset path for a temporary usage.</returns>
    API_FUNCTION() static String CreateTemporaryAssetPath();

public:
    /// <summary>
    /// Gets content statistics.
    /// </summary>
    API_PROPERTY() static ContentStats GetStats();

    /// <summary>
    /// Gets the assets (loaded or during load).
    /// </summary>
    /// <returns>The collection of assets.</returns>
    API_PROPERTY() static Array<Asset*, HeapAllocation> GetAssets();

    /// <summary>
    /// Gets the raw dictionary of assets (loaded or during load).
    /// </summary>
    /// <returns>The collection of assets.</returns>
    static const Dictionary<Guid, Asset*, HeapAllocation>& GetAssetsRaw();

    /// <summary>
    /// Loads asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
    /// </summary>
    /// <param name="id">Asset unique ID</param>
    /// <param name="type">The asset type. If loaded object has different type (excluding types derived from the given) the loading fails.</param>
    /// <returns>Loaded asset or null if cannot</returns>
    API_FUNCTION() static Asset* LoadAsync(const Guid& id, API_PARAM(Attributes="TypeReference(typeof(Asset))") const MClass* type);

    /// <summary>
    /// Loads asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
    /// </summary>
    /// <param name="id">Asset unique ID</param>
    /// <param name="type">The asset type. If loaded object has different type (excluding types derived from the given) the loading fails.</param>
    /// <returns>Loaded asset or null if cannot</returns>
    static Asset* LoadAsync(const Guid& id, const ScriptingTypeHandle& type);

    /// <summary>
    /// Loads asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
    /// </summary>
    /// <param name="id">Asset unique ID</param>
    /// <typeparam name="T">Type of the asset to load. Includes any asset types derived from the type.</typeparam>
    /// <returns>Loaded asset or null if cannot</returns>
    template<typename T>
    FORCE_INLINE static T* LoadAsync(const Guid& id)
    {
        return static_cast<T*>(LoadAsync(id, T::TypeInitializer));
    }

    /// <summary>
    /// Loads asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
    /// </summary>
    /// <param name="path">The path of the asset (absolute or relative to the current workspace directory).</param>
    /// <param name="type">The asset type. If loaded object has different type (excluding types derived from the given) the loading fails.</param>
    /// <returns>Loaded asset or null if cannot</returns>
    API_FUNCTION(Attributes="HideInEditor") static Asset* LoadAsync(const StringView& path, const MClass* type);

    /// <summary>
    /// Loads asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
    /// </summary>
    /// <param name="path">The path of the asset (absolute or relative to the current workspace directory).</param>
    /// <param name="type">The asset type. If loaded object has different type (excluding types derived from the given) the loading fails.</param>
    /// <returns>Loaded asset or null if cannot</returns>
    static Asset* LoadAsync(const StringView& path, const ScriptingTypeHandle& type);

    /// <summary>
    /// Loads asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
    /// </summary>
    /// <param name="path">The path of the asset (absolute or relative to the current workspace directory).</param>
    /// <typeparam name="T">Type of the asset to load. Includes any asset types derived from the type.</typeparam>
    /// <returns>Loaded asset or null if cannot</returns>
    template<typename T>
    FORCE_INLINE static T* LoadAsync(const StringView& path)
    {
        return static_cast<T*>(LoadAsync(path, T::TypeInitializer));
    }

    /// <summary>
    /// Loads internal engine asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
    /// </summary>
    /// <param name="internalPath">The path of the asset relative to the engine internal content (excluding the extension).</param>
    /// <param name="type">The asset type. If loaded object has different type (excluding types derived from the given) the loading fails.</param>
    /// <returns>The loaded asset or null if failed.</returns>
    API_FUNCTION(Attributes="HideInEditor") static Asset* LoadAsyncInternal(const StringView& internalPath, const MClass* type);

    /// <summary>
    /// Loads internal engine asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
    /// </summary>
    /// <param name="internalPath">The path of the asset relative to the engine internal content (excluding the extension).</param>
    /// <param name="type">The asset type. If loaded object has different type (excluding types derived from the given) the loading fails.</param>
    /// <returns>The loaded asset or null if failed.</returns>
    static Asset* LoadAsyncInternal(const StringView& internalPath, const ScriptingTypeHandle& type);

    /// <summary>
    /// Loads internal engine asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
    /// </summary>
    /// <param name="internalPath">The path of the asset relative to the engine internal content (excluding the extension).</param>
    /// <param name="type">The asset type. If loaded object has different type (excluding types derived from the given) the loading fails.</param>
    /// <returns>The loaded asset or null if failed.</returns>
    static Asset* LoadAsyncInternal(const Char* internalPath, const ScriptingTypeHandle& type);

    /// <summary>
    /// Loads internal engine asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
    /// </summary>
    /// <param name="internalPath">The path of the asset relative to the engine internal content (excluding the extension).</param>
    /// <returns>The loaded asset or null if failed.</returns>
    template<typename T>
    FORCE_INLINE static T* LoadAsyncInternal(const Char* internalPath)
    {
        return static_cast<T*>(LoadAsyncInternal(internalPath, T::TypeInitializer));
    }

    /// <summary>
    /// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
    /// Waits until asset will be loaded. It's equivalent to LoadAsync + WaitForLoaded.
    /// </summary>
    /// <param name="id">Asset unique ID.</param>
    /// <param name="timeoutInMilliseconds">Custom timeout value in milliseconds.</param>
    /// <typeparam name="T">Type of the asset to load. Includes any asset types derived from the type.</typeparam>
    /// <returns>Asset instance if loaded, null otherwise.</returns>
    template<typename T>
    static T* Load(const Guid& id, double timeoutInMilliseconds = 30000.0)
    {
        auto asset = LoadAsync<T>(id);
        if (asset && !asset->WaitForLoaded(timeoutInMilliseconds))
            return asset;
        return nullptr;
    }

    /// <summary>
    /// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
    /// Waits until asset will be loaded. It's equivalent to LoadAsync + WaitForLoaded.
    /// </summary>
    /// <param name="path">The path of the asset (absolute or relative to the current workspace directory).</param>
    /// <param name="timeoutInMilliseconds">Custom timeout value in milliseconds.</param>
    /// <typeparam name="T">Type of the asset to load. Includes any asset types derived from the type.</typeparam>
    /// <returns>Asset instance if loaded, null otherwise.</returns>
    template<typename T>
    static T* Load(const StringView& path, double timeoutInMilliseconds = 30000.0)
    {
        auto asset = LoadAsync<T>(path);
        if (asset && !asset->WaitForLoaded(timeoutInMilliseconds))
            return asset;
        return nullptr;
    }

    /// <summary>
    /// Determines whether input asset type name identifier is invalid.
    /// </summary>
    /// <param name="type">The requested type of the asset to be.</param>
    /// <param name="assetType">The actual type of the asset.</param>
    /// <returns><c>true</c> if asset type identifier is invalid otherwise, <c>false</c>.</returns>
    static bool IsAssetTypeIdInvalid(const ScriptingTypeHandle& type, const ScriptingTypeHandle& assetType);

public:
    /// <summary>
    /// Finds the asset with at given path. Checks all loaded assets.
    /// </summary>
    /// <param name="path">The path.</param>
    /// <returns>The found asset or null if not loaded.</returns>
    API_FUNCTION() static Asset* GetAsset(const StringView& path);

    /// <summary>
    /// Finds the asset with given ID. Checks all loaded assets.
    /// </summary>
    /// <param name="id">The id.</param>
    /// <returns>The found asset or null if not loaded.</returns>
    API_FUNCTION() static Asset* GetAsset(const Guid& id);

public:
    /// <summary>
    /// Deletes the specified asset.
    /// </summary>
    /// <param name="asset">The asset.</param>
    API_FUNCTION() static void DeleteAsset(Asset* asset);

    /// <summary>
    /// Deletes the asset at the specified path.
    /// </summary>
    /// <param name="path">The asset path.</param>
    API_FUNCTION(Attributes="HideInEditor") static void DeleteAsset(const StringView& path);

public:
#if USE_EDITOR

    /// <summary>
    /// Renames the asset.
    /// </summary>
    /// <param name="oldPath">The old asset path.</param>
    /// <param name="newPath">The new asset path.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() static bool RenameAsset(const StringView& oldPath, const StringView& newPath);

    /// <summary>
    /// Performs the fast temporary asset clone to the temporary folder.
    /// </summary>
    /// <param name="path">The source path.</param>
    /// <param name="resultPath">The result path.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool FastTmpAssetClone(const StringView& path, String& resultPath);

    /// <summary>
    /// Clones the asset file.
    /// </summary>
    /// <param name="dstPath">The destination path.</param>
    /// <param name="srcPath">The source path.</param>
    /// <param name="dstId">The destination id.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool CloneAssetFile(const StringView& dstPath, const StringView& srcPath, const Guid& dstId);

#endif

    /// <summary>
    /// Unloads the specified asset.
    /// </summary>
    /// <param name="asset">The asset.</param>
    API_FUNCTION() static void UnloadAsset(Asset* asset);

    /// <summary>
    /// Creates temporary and virtual asset of the given type.
    /// </summary>
    /// <returns>Created asset or null if failed.</returns>
    template<typename T>
    FORCE_INLINE static T* CreateVirtualAsset()
    {
        return static_cast<T*>(CreateVirtualAsset(T::TypeInitializer));;
    }

    /// <summary>
    /// Creates temporary and virtual asset of the given type.
    /// </summary>
    /// <param name="type">The asset type klass.</param>
    /// <returns>Created asset or null if failed.</returns>
    API_FUNCTION() static Asset* CreateVirtualAsset(API_PARAM(Attributes="TypeReference(typeof(Asset))") const MClass* type);

    /// <summary>
    /// Creates temporary and virtual asset of the given type.
    /// </summary>
    /// <param name="type">The asset type.</param>
    /// <returns>Created asset or null if failed.</returns>
    static Asset* CreateVirtualAsset(const ScriptingTypeHandle& type);

    /// <summary>
    /// Occurs when asset is being disposed and will be unloaded (by force). All references to it should be released.
    /// </summary>
    API_EVENT() static Delegate<Asset*> AssetDisposing;

    /// <summary>
    /// Occurs when asset is being reloaded and will be unloaded (by force) to be loaded again (e.g. after reimport). Always called from the main thread.
    /// </summary>
    API_EVENT() static Delegate<Asset*> AssetReloading;

private:
    static void WaitForTask(ContentLoadTask* loadingTask, double timeoutInMilliseconds);
    static void tryCallOnLoaded(Asset* asset);
    static void onAssetLoaded(Asset* asset);
    static void onAssetUnload(Asset* asset);
    static void onAssetChangeId(Asset* asset, const Guid& oldId, const Guid& newId);
    static void deleteFileSafety(const StringView& path, const Guid& id);
};
