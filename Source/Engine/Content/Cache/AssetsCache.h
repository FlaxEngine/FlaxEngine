// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "../AssetInfo.h"
#include "../Config.h"
#include "Engine/Core/Types/Guid.h"
#if ENABLE_ASSETS_DISCOVERY
#include "Engine/Core/Types/DateTime.h"
#endif
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Platform/CriticalSection.h"

struct AssetHeader;
struct FlaxStorageReference;
class FlaxStorage;

/// <summary>
/// Assets cache flags.
/// </summary>
enum class AssetsCacheFlags : int32
{
    /// <summary>
    /// The none.
    /// </summary>
    None = 0,

    /// <summary>
    /// The serialized paths are relative to the startup folder (should be converted to absolute on load).
    /// </summary>
    RelativePaths = 1,
};

DECLARE_ENUM_OPERATORS(AssetsCacheFlags);

/// <summary>
/// Flax Game Engine assets cache container
/// </summary>
class FLAXENGINE_API AssetsCache
{
public:
    /// <summary>
    /// The registry entry structure.
    /// </summary>
    struct Entry
    {
        /// <summary>
        /// The cached asset information.
        /// </summary>
        AssetInfo Info;

#if ENABLE_ASSETS_DISCOVERY
        /// <summary>
        /// The file modified date.
        /// </summary>
        DateTime FileModified;
#endif

        Entry()
        {
        }

        Entry(const Guid& id, const StringView& typeName, const StringView& path)
            : Info(id, typeName, path)
#if ENABLE_ASSETS_DISCOVERY
            , FileModified(DateTime::NowUTC())
#endif
        {
        }
    };

    typedef Dictionary<Guid, Entry> Registry;
    typedef Dictionary<String, Guid> PathsMapping;

private:
    bool _isDirty = false;
    CriticalSection _locker;
    Registry _registry;
    PathsMapping _pathsMapping;
    String _path;

public:
    /// <summary>
    /// Gets amount of registered assets.
    /// </summary>
    int32 Size() const
    {
        _locker.Lock();
        const int32 result = _registry.Count();
        _locker.Unlock();
        return result;
    }

public:
    /// <summary>
    /// Init registry
    /// </summary>
    void Init();

    /// <summary>
    /// Save registry
    /// </summary>
    /// <returns>True if cannot save registry</returns>
    bool Save();

    /// <summary>
    /// Saves the registry to the given file.
    /// </summary>
    /// <param name="path">The output file path.</param>
    /// <param name="entries">The registry entries.</param>
    /// <param name="pathsMapping">The assets paths mapping table.</param>
    /// <param name="flags">The custom flags.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool Save(const StringView& path, const Registry& entries, const PathsMapping& pathsMapping, const AssetsCacheFlags flags = AssetsCacheFlags::None);

public:
    /// <summary>
    /// Finds the asset path by id. In editor it returns the actual asset path, at runtime it returns the mapped asset path.
    /// </summary>
    /// <param name="id">The asset id.</param>
    /// <returns>The asset path, or empty if failed to find.</returns>
    const String& GetEditorAssetPath(const Guid& id) const;

    /// <summary>
    /// Finds the asset info by path.
    /// </summary>
    /// <param name="path">The asset path.</param>
    /// <param name="info">The output asset info. Filled with valid values if method returns true.</param>
    /// <returns>True if found any asset, otherwise false.</returns>
    bool FindAsset(const StringView& path, AssetInfo& info);

    /// <summary>
    /// Finds the asset info by id.
    /// </summary>
    /// <param name="id">The asset id.</param>
    /// <param name="info">The output asset info. Filled with valid values if method returns true.</param>
    /// <returns>True if found any asset, otherwise false.</returns>
    bool FindAsset(const Guid& id, AssetInfo& info);

    /// <summary>
    /// Checks if asset with given path is in registry.
    /// </summary>
    /// <param name="path">The asset path.</param>
    /// <returns>True if asset is in cache, otherwise false.</returns>
    bool HasAsset(const StringView& path)
    {
        AssetInfo info;
        return FindAsset(path, info);
    }

    /// <summary>
    /// Checks if asset with given ID is in registry.
    /// </summary>
    /// <param name="id">The asset id.</param>
    /// <returns>True if asset is in cache, otherwise false.</returns>
    bool HasAsset(const Guid& id)
    {
        AssetInfo info;
        return FindAsset(id, info);
    }

    /// <summary>
    /// Gets the asset ids.
    /// </summary>
    /// <param name="result">The result array.</param>
    void GetAll(Array<Guid, HeapAllocation>& result) const;

    /// <summary>
    /// Gets the asset ids that match the given typename.
    /// </summary>
    /// <param name="typeName">The asset typename.</param>
    /// <param name="result">The result array.</param>
    void GetAllByTypeName(const StringView& typeName, Array<Guid, HeapAllocation>& result) const;

    /// <summary>
    /// Register assets in the cache
    /// </summary>
    /// <param name="storage">Flax assets container reference</param>
    void RegisterAssets(const FlaxStorageReference& storage);

    /// <summary>
    /// Register assets in the cache
    /// </summary>
    /// <param name="storage">Flax assets container</param>
    void RegisterAssets(FlaxStorage* storage);

    /// <summary>
    /// Register asset in the cache
    /// </summary>
    /// <param name="header">Flax asset file header</param>
    /// <param name="path">Asset path</param>
    void RegisterAsset(const AssetHeader& header, const StringView& path);

    /// <summary>
    /// Register asset in the cache
    /// </summary>
    /// <param name="id">Asset ID</param>
    /// <param name="typeName">Asset type name</param>
    /// <param name="path">Asset path</param>
    void RegisterAsset(const Guid& id, const String& typeName, const StringView& path);

    /// <summary>
    /// Delete asset
    /// </summary>
    /// <param name="path">Asset path</param>
    /// <param name="info">Output asset info</param>
    /// <returns>True if asset has been deleted, otherwise false</returns>
    bool DeleteAsset(const StringView& path, AssetInfo* info);

    /// <summary>
    /// Delete asset
    /// </summary>
    /// <param name="id">Asset ID</param>
    /// <param name="info">Output asset info</param>
    /// <returns>True if asset has been deleted, otherwise false</returns>
    bool DeleteAsset(const Guid& id, AssetInfo* info);

    /// <summary>
    /// Rename asset
    /// </summary>
    /// <param name="oldPath">Old path</param>
    /// <param name="newPath">New path</param>
    /// <returns>True if has been deleted, otherwise false</returns>
    bool RenameAsset(const StringView& oldPath, const StringView& newPath);

    /// <summary>
    /// Determines whether cached asset entry is valid.
    /// </summary>
    /// <param name="e">The asset entry.</param>
    /// <returns>True if is valid, otherwise false.</returns>
    bool IsEntryValid(Entry& e);
};
