// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Content/BinaryAsset.h"
#include "ShaderStorage.h"

/// <summary>
/// Base class for assets that can contain shader.
/// </summary>
class FLAXENGINE_API ShaderAssetBase
{
protected:
    ShaderStorage::Header _shaderHeader;

public:
    static bool IsNullRenderer();

    /// <summary>
    /// Gets internal shader cache chunk index (for current GPU device shader profile).
    /// </summary>
    static int32 GetCacheChunkIndex();

    /// <summary>
    /// Gets internal shader cache chunk index.
    /// </summary>
    /// <param name="profile">Shader profile</param>
    /// <returns>Chunk index</returns>
    static int32 GetCacheChunkIndex(ShaderProfile profile);

#if USE_EDITOR
    /// <summary>
    /// Prepare shader compilation options.
    /// </summary>
    /// <param name="options">The output options.</param>
    virtual void InitCompilationOptions(struct ShaderCompilationOptions& options)
    {
    }
#endif

protected:
    bool initBase(AssetInitData& initData);

    /// <summary>
    /// Gets the parent asset.
    /// </summary>
    virtual BinaryAsset* GetShaderAsset() const = 0;

#if USE_EDITOR
    /// <summary>
    /// Saves this shader asset to the storage container.
    /// </summary>
    /// <returns>True if failed, otherwise false.</returns>
    bool SaveShaderAsset() const;
#endif

    /// <summary>
    /// Shader cache loading result data container.
    /// </summary>
    struct ShaderCacheResult
    {
        /// <summary>
        /// The shader cache data. Allocated or linked (if gathered from asset chunk).
        /// </summary>
        DataContainer<byte> Data;

#if COMPILE_WITH_SHADER_COMPILER
        /// <summary>
        /// The list of files included by the shader source (used by the given cache on the runtime graphics platform shader profile). Paths are absolute and unique.
        /// </summary>
        Array<String> Includes;
#endif
    };

    /// <summary>
    /// Loads shader cache (it may call compilation or gather precached data).
    /// </summary>
    /// <param name="result">The output data.</param>
    /// <returns>True if cannot load data, otherwise false.</returns>
    bool LoadShaderCache(ShaderCacheResult& result);

#if COMPILE_WITH_SHADER_COMPILER
    /// <summary>
    /// Registers shader asset for the automated reloads on source includes changes.
    /// </summary>
    /// <param name="asset">The asset.</param>
    /// <param name="shaderCache">The loaded shader cache.</param>
    void RegisterForShaderReloads(Asset* asset, const ShaderCacheResult& shaderCache);

    /// <summary>
    /// Unregisters shader asset from the automated reloads on source includes changes.
    /// </summary>
    /// <param name="asset">The asset.</param>
    void UnregisterForShaderReloads(Asset* asset);
#endif
};

/// <summary>
/// Base class for binary assets that can contain shader.
/// </summary>
template<typename BaseType>
class ShaderAssetTypeBase : public BaseType, public ShaderAssetBase
{
public:
    static const uint32 ShadersSerializedVersion = ShaderStorage::Header::Version;

protected:
    explicit ShaderAssetTypeBase(const ScriptingObjectSpawnParams& params, const AssetInfo* info)
        : BaseType(params, info)
    {
    }

protected:
    // [BaseType]
    bool init(AssetInitData& initData) override
    {
        return initBase(initData);
    }

    AssetChunksFlag getChunksToPreload() const override
    {
        AssetChunksFlag result = 0;
        const auto cachingMode = ShaderStorage::GetCachingMode();
        if (cachingMode == ShaderStorage::CachingMode::AssetInternal && !IsNullRenderer())
            result |= GET_CHUNK_FLAG(GetCacheChunkIndex());
        return result;
    }

    // [ShaderAssetBase]
    BinaryAsset* GetShaderAsset() const override
    {
        return (BinaryAsset*)this;
    }
};
