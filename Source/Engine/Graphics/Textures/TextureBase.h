// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/BinaryAsset.h"
#include "StreamingTexture.h"
#include "Engine/Core/Log.h"

/// <summary>
/// Base class for <see cref="Texture"/>, <see cref="SpriteAtlas"/>, <see cref="IESProfile"/> and other assets that can contain texture data.
/// </summary>
/// <seealso cref="FlaxEngine.BinaryAsset" />
API_CLASS(Abstract, NoSpawn) class FLAXENGINE_API TextureBase : public BinaryAsset, public ITextureOwner
{
DECLARE_ASSET_HEADER(TextureBase);
public:

    static const uint32 TexturesSerializedVersion = 4;

public:

    /// <summary>
    /// The texture init data (external source).
    /// </summary>
    struct FLAXENGINE_API InitData
    {
        struct MipData
        {
            BytesContainer Data;
            uint32 RowPitch;
            uint32 SlicePitch;
        };

        PixelFormat Format;
        int32 Width;
        int32 Height;
        int32 ArraySize;
        Array<MipData, FixedAllocation<14>> Mips;

        /// <summary>
        /// Generates the mip map data.
        /// </summary>
        /// <remarks>
        /// Compressed formats are not supported. Point filter supports all types and preserves texture edge values.
        /// </remarks>
        /// <param name="mipIndex">Index of the mip.</param>
        /// <param name="linear">True if use linear filer, otherwise point filtering.</param>
        /// <returns>True if failed, otherwise false.</returns>
        bool GenerateMip(int32 mipIndex, bool linear);
    };

protected:

    StreamingTexture _texture;
    InitData* _customData;

private:

    BinaryAsset* _parent;

public:

    /// <summary>
    /// Gets the streaming texture object handle.
    /// </summary>
    FORCE_INLINE const StreamingTexture* StreamingTexture() const
    {
        return &_texture;
    }

    /// <summary>
    /// Gets GPU texture object allocated by the asset.
    /// </summary>
    API_PROPERTY() FORCE_INLINE GPUTexture* GetTexture() const
    {
        return _texture.GetTexture();
    }

    /// <summary>
    /// Gets the texture data format.
    /// </summary>
    API_PROPERTY() FORCE_INLINE PixelFormat Format() const
    {
        return _texture.GetHeader()->Format;
    }

    /// <summary>
    /// Gets the total width of the texture. Actual resident size may be different due to dynamic content streaming. Returns 0 if texture is not loaded.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 Width() const
    {
        return _texture.TotalWidth();
    }

    /// <summary>
    /// Gets the total height of the texture. Actual resident size may be different due to dynamic content streaming. Returns 0 if texture is not loaded.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 Height() const
    {
        return _texture.TotalHeight();
    }

    /// <summary>
    /// Gets the total size of the texture. Actual resident size may be different due to dynamic content streaming. Returns Vector2::Zero if texture is not loaded.
    /// </summary>
    API_PROPERTY() FORCE_INLINE Vector2 Size() const
    {
        return Vector2(static_cast<float>(_texture.TotalWidth()), static_cast<float>(_texture.TotalHeight()));
    }

    /// <summary>
    /// Gets the total array size of the texture.
    /// </summary>
    API_PROPERTY() int32 GetArraySize() const
    {
        return StreamingTexture()->TotalArraySize();
    }

    /// <summary>
    /// Gets the total mip levels count of the texture. Actual resident mipmaps count may be different due to dynamic content streaming.
    /// </summary>
    API_PROPERTY() int32 GetMipLevels() const
    {
        return StreamingTexture()->TotalMipLevels();
    }

    /// <summary>
    /// Gets the current mip levels count of the texture that are on GPU ready to use.
    /// </summary>
    API_PROPERTY() int32 GetResidentMipLevels() const
    {
        return GetTexture()->ResidentMipLevels();
    }

    /// <summary>
    /// Gets the amount of the memory used by this resource. Exact value may differ due to memory alignment and resource allocation policy.
    /// </summary>
    API_PROPERTY() uint64 GetCurrentMemoryUsage() const
    {
        return GetTexture()->GetMemoryUsage();
    }

    /// <summary>
    /// Gets the total memory usage that texture may have in use (if loaded to the maximum quality). Exact value may differ due to memory alignment and resource allocation policy.
    /// </summary>
    API_PROPERTY() uint64 GetTotalMemoryUsage() const
    {
        return StreamingTexture()->GetTotalMemoryUsage();
    }

public:

    /// <summary>
    /// Gets the mip data.
    /// </summary>
    /// <param name="mipIndex">The mip index (zero-based).</param>
    /// <param name="rowPitch">The data row pitch (in bytes).</param>
    /// <param name="slicePitch">The data slice pitch (in bytes).</param>
    /// <returns>The mip-map data or empty if failed to get it.</returns>
    API_FUNCTION() BytesContainer GetMipData(int32 mipIndex, API_PARAM(out) int32& rowPitch, API_PARAM(out) int32& slicePitch);

    /// <summary>
    /// Loads the texture data from the asset.
    /// </summary>
    /// <param name="result">The result data.</param>
    /// <param name="copyData">True if copy asset data to the result buffer, otherwise texture data will be linked to the internal storage (then the data is valid while asset is loaded and there is no texture data copy operations - faster).</param>
    /// <returns>True if cannot load data, otherwise false.</returns>
    bool GetTextureData(TextureData& result, bool copyData = true);

    /// <summary>
    /// Initializes the texture with specified initialize data source (asset must be virtual).
    /// </summary>
    /// <param name="initData">The initialize data (allocated by the called, will be used and released by the asset internal layer).</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool Init(InitData* initData);

protected:

    virtual int32 calculateChunkIndex(int32 mipIndex) const;

private:

    // Internal bindings
    API_FUNCTION(NoProxy) bool Init(void* ptr);

public:

    // [ITextureOwner]
    CriticalSection* GetOwnerLocker() const override;
    Task* RequestMipDataAsync(int32 mipIndex) override;
    FlaxStorage::LockData LockData() override;
    void GetMipData(int32 mipIndex, BytesContainer& data) const override;
    void GetMipDataWithLoading(int32 mipIndex, BytesContainer& data) const override;
    bool GetMipDataCustomPitch(int32 mipIndex, uint32& rowPitch, uint32& slicePitch) const override;

protected:

    // [BinaryAsset]
    bool init(AssetInitData& initData) override
    {
        // Skip for virtual assets
        if (IsVirtual())
            return false;

        // Validate
        if (initData.SerializedVersion != 4)
        {
            LOG(Error, "Invalid serialized texture version.");
            return true;
        }
        if (initData.CustomData.Length() != sizeof(TextureHeader))
        {
            LOG(Error, "Missing texture header.");
            return true;
        }

        // Load header
        TextureHeader header;
        Platform::MemoryCopy(&header, initData.CustomData.Get(), sizeof(TextureHeader));

        // Create texture
        if (_texture.Create(header))
        {
            LOG(Error, "Cannot initialize texture.");
            return true;
        }

        return false;
    }

    LoadResult load() override
    {
        // Loading textures is very fast xD
        return LoadResult::Ok;
    }

    void unload(bool isReloading) override;
};
