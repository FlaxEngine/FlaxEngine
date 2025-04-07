// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Editor/Cooker/GameCooker.h"
#include "Editor/Cooker/PlatformTools.h"
#include "Engine/Core/Types/Pair.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Content/AssetInfo.h"
#include "Engine/Content/Cache/AssetsCache.h"

class Asset;
class BinaryAsset;
class JsonAssetBase;
struct AssetInitData;

/// <summary>
/// Cooking step that builds all the assets and packages them to the output directory.
/// Uses incremental build cache to provide faster building.
/// </summary>
/// <seealso cref="GameCooker::BuildStep" />
class FLAXENGINE_API CookAssetsStep : public GameCooker::BuildStep
{
public:

    typedef Array<Pair<String, DateTime>> FileDependenciesList;

    /// <summary>
    /// Cached cooked asset entry data.
    /// </summary>
    struct FLAXENGINE_API CacheEntry
    {
        /// <summary>
        /// The asset identifier.
        /// </summary>
        Guid ID;

        /// <summary>
        /// The stored data full typename. Used to recognize asset type.
        /// </summary>
        String TypeName;

        /// <summary>
        /// The asset file modification time.
        /// </summary>
        DateTime FileModified;

        /// <summary>
        /// The list of files on which this entry depends on. Cached date is the last edit time used to discard cache result on modification.
        /// </summary>
        FileDependenciesList FileDependencies;

        bool IsValid(bool withDependencies = false);
    };

    /// <summary>
    /// Assets cooking cache data (incremental building feature).
    /// </summary>
    struct FLAXENGINE_API CacheData : public IBuildCache
    {
        /// <summary>
        /// The cache header file path.
        /// </summary>
        String HeaderFilePath;

        /// <summary>
        /// The cached files folder.
        /// </summary>
        String CacheFolder;

        /// <summary>
        /// The build options used to cook assets. Changing some options in game settings might trigger cached assets invalidation.
        /// </summary>
        struct
        {
            struct
            {
                bool SupportDX12;
                bool SupportDX11;
                bool SupportDX10;
                bool SupportVulkan;
            } Windows;

            struct
            {
                bool SupportDX11;
                bool SupportDX10;
            } UWP;

            struct
            {
                bool SupportVulkan;
            } Linux;

            struct
            {
                bool ShadersNoOptimize;
                bool ShadersGenerateDebugData;
                Guid StreamingSettingsAssetId;
                int32 ShadersVersion;
                int32 MaterialGraphVersion;
                int32 ParticleGraphVersion;
            } Global;
        } Settings;

        /// <summary>
        /// The cached entries.
        /// </summary>
        Dictionary<Guid, CacheEntry> Entries;

    public:

        /// <summary>
        /// Gets the path to the asset of the given id (file may be missing).
        /// </summary>
        /// <param name="id">The asset id.</param>
        /// <param name="cachedFilePath">The cached file path to use for creating cache storage.</param>
        void GetFilePath(const Guid& id, String& cachedFilePath) const
        {
            cachedFilePath = CacheFolder / id.ToString(Guid::FormatType::N);
        }

        /// <summary>
        /// Creates the new entry for the cooked asset file.
        /// </summary>
        /// <param name="asset">The asset.</param>
        /// <param name="cachedFilePath">The cached file path to use for creating cache storage.</param>
        /// <returns>The added entry reference.</returns>
        CacheEntry& CreateEntry(const JsonAssetBase* asset, String& cachedFilePath);

        /// <summary>
        /// Creates the new entry for the cooked asset file.
        /// </summary>
        /// <param name="asset">The asset.</param>
        /// <param name="cachedFilePath">The cached file path to use for creating cache storage.</param>
        /// <returns>The added entry reference.</returns>
        CacheEntry& CreateEntry(const Asset* asset, String& cachedFilePath);

        /// <summary>
        /// Loads the cache for the given cooking data.
        /// </summary>
        /// <param name="data">The data.</param>
        void Load(CookingData& data);

        /// <summary>
        /// Saves this cache (header file).
        /// </summary>
        /// <param name="data">The data.</param>
        void Save(CookingData& data);

        using IBuildCache::InvalidateCachePerType;
        void InvalidateCachePerType(const StringView& typeName) override;
    };

    struct FLAXENGINE_API AssetCookData
    {
        CookingData& Data;
        CacheData& Cache;
        AssetInitData& InitData;
        Asset* Asset;
        FileDependenciesList& FileDependencies;
    };

    typedef bool (*ProcessAssetFunc)(AssetCookData&);

    /// <summary>
    /// The asset processors (key: asset full typename, value: processor function that cooks the asset).
    /// </summary>
    static Dictionary<String, ProcessAssetFunc> AssetProcessors;

    static bool ProcessDefaultAsset(AssetCookData& options);
    
private:

    AssetsCache::Registry AssetsRegistry;
    AssetsCache::PathsMapping AssetPathsMapping;

    bool Process(CookingData& data, CacheData& cache, Asset* asset);
    bool Process(CookingData& data, CacheData& cache, BinaryAsset* asset);
    bool Process(CookingData& data, CacheData& cache, JsonAssetBase* asset);

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="CookAssetsStep"/> class.
    /// </summary>
    CookAssetsStep();

public:

    // [BuildStep]
    bool Perform(CookingData& data) override;
};
