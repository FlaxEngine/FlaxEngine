// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Content/BinaryAsset.h"
#include "Engine/Graphics/GPUDevice.h"
#include "ShaderStorage.h"

/// <summary>
/// Base class for assets that can contain shader.
/// </summary>
class FLAXENGINE_API ShaderAssetBase
{
protected:

    ShaderStorage::Header _shaderHeader;

public:

    /// <summary>
    /// Gets internal shader cache chunk index
    /// </summary>
    /// <returns>Chunk index</returns>
    FORCE_INLINE static int32 GetCacheChunkIndex()
    {
        return GetCacheChunkIndex(GPUDevice::Instance->GetShaderProfile());
    }

    /// <summary>
    /// Gets internal shader cache chunk index
    /// </summary>
    /// <param name="profile">Shader profile</param>
    /// <returns>Chunk index</returns>
    static int32 GetCacheChunkIndex(ShaderProfile profile);

#if USE_EDITOR

    /// <summary>
    /// Prepare shader compilation options
    /// </summary>
    /// <param name="options">Options</param>
    virtual void InitCompilationOptions(struct ShaderCompilationOptions& options)
    {
    }

#endif

protected:
    
    /// <summary>
    /// Gets the parent asset.
    /// </summary>
    /// <returns>The asset.</returns>
    virtual BinaryAsset* GetShaderAsset() const = 0;

#if USE_EDITOR

    /// <summary>
    /// Saves this shader asset to the storage container.
    /// </summary>
    /// <returns>True if failed, otherwise false.</returns>
    bool Save();

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
/// Base class for assets that can contain shader
/// </summary>
template<typename BaseType>
class ShaderAssetTypeBase : public BaseType, public ShaderAssetBase
{
public:

    static const uint32 ShadersSerializedVersion = ShaderStorage::Header::Version;

protected:

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="params">Asset scripting class metadata</param>
    /// <param name="info">Asset information</param>
    explicit ShaderAssetTypeBase(const ScriptingObjectSpawnParams& params, const AssetInfo* info)
        : BaseType(params, info)
    {
    }

protected:

    // [BaseType]
    BinaryAsset* GetShaderAsset() const override
    {
        return (BinaryAsset*)this;
    }
    bool init(AssetInitData& initData) override
    {
        // Validate version
        if (initData.SerializedVersion != ShadersSerializedVersion)
        {
            LOG(Warning, "Invalid shader serialized version.");
            return true;
        }

        // Validate data
        if (initData.CustomData.Length() != sizeof(_shaderHeader))
        {
            LOG(Warning, "Invalid shader header.");
            return true;
        }

        // Load header 'as-is'
        Platform::MemoryCopy(&_shaderHeader, initData.CustomData.Get(), sizeof(_shaderHeader));

        return false;
    }

    AssetChunksFlag getChunksToPreload() const override
    {
        AssetChunksFlag result = 0;
        const auto cachingMode = ShaderStorage::GetCachingMode();
        if (cachingMode == ShaderStorage::CachingMode::AssetInternal && GPUDevice::Instance->GetRendererType() != RendererType::Null)
            result |= GET_CHUNK_FLAG(GetCacheChunkIndex());
        return result;
    }
};
