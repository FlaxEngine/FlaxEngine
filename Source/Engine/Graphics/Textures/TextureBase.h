// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/BinaryAsset.h"
#include "StreamingTexture.h"

class TextureData;
class TextureMipData;

/// <summary>
/// Base class for <see cref="Texture"/>, <see cref="SpriteAtlas"/>, <see cref="IESProfile"/> and other assets that can contain texture data.
/// </summary>
/// <seealso cref="FlaxEngine.BinaryAsset" />
API_CLASS(Abstract, NoSpawn) class FLAXENGINE_API TextureBase : public BinaryAsset, public ITextureOwner
{
    DECLARE_ASSET_HEADER(TextureBase);
    static const uint32 TexturesSerializedVersion = 4;

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

            MipData() = default;
            MipData(MipData&& other) noexcept;
        };

        PixelFormat Format;
        int32 Width;
        int32 Height;
        int32 ArraySize;
        Array<MipData, FixedAllocation<14>> Mips;

        InitData() = default;
        InitData(InitData&& other) noexcept;

        InitData& operator=(InitData&& other)
        {
            if (this != &other)
                *this = MoveTemp(other);
            return *this;
        }

        /// <summary>
        /// Generates the mip map data.
        /// </summary>
        /// <remarks>
        /// Compressed formats are not supported. Point filter supports all types and preserves texture edge values.
        /// </remarks>
        /// <param name="mipIndex">Index of the mip.</param>
        /// <param name="linear">True if use linear filer, otherwise point filtering.</param>
        /// <returns>True if failed, otherwise false.</returns>
        bool GenerateMip(int32 mipIndex, bool linear = false);

        void FromTextureData(const TextureData& textureData, bool generateMips = false);
    };

protected:
    StreamingTexture _texture;
    InitData* _customData;
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
    /// Gets the total size of the texture. Actual resident size may be different due to dynamic content streaming. Returns Float2::Zero if texture is not loaded.
    /// </summary>
    API_PROPERTY() Float2 Size() const;

    /// <summary>
    /// Gets the total array size of the texture.
    /// </summary>
    API_PROPERTY() int32 GetArraySize() const;

    /// <summary>
    /// Gets the total mip levels count of the texture. Actual resident mipmaps count may be different due to dynamic content streaming.
    /// </summary>
    API_PROPERTY() int32 GetMipLevels() const;

    /// <summary>
    /// Gets the current mip levels count of the texture that are on GPU ready to use.
    /// </summary>
    API_PROPERTY() int32 GetResidentMipLevels() const;

    /// <summary>
    /// Gets the amount of the memory used by this resource. Exact value may differ due to memory alignment and resource allocation policy.
    /// </summary>
    API_PROPERTY() uint64 GetCurrentMemoryUsage() const;

    /// <summary>
    /// Gets the total memory usage that texture may have in use (if loaded to the maximum quality). Exact value may differ due to memory alignment and resource allocation policy.
    /// </summary>
    API_PROPERTY() uint64 GetTotalMemoryUsage() const;

    /// <summary>
    /// Gets the index of the texture group used by this texture.
    /// </summary>
    API_PROPERTY() int32 GetTextureGroup() const;

    /// <summary>
    /// Sets the index of the texture group used by this texture.
    /// </summary>
    API_PROPERTY() void SetTextureGroup(int32 textureGroup);

    /// <summary>
    /// Returns true if texture streaming failed (eg. pixel format is unsupported or texture data cannot be uploaded to GPU due to memory limit).
    /// </summary>
    API_PROPERTY() bool HasStreamingError() const;

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
    /// Loads the texture data from the asset (single mip).
    /// </summary>
    /// <param name="result">The result data.</param>
    /// <param name="mipIndex">The mip index (zero-based).</param>
    /// <param name="arrayIndex">The array or depth slice index (zero-based).</param>
    /// <param name="copyData">True if copy asset data to the result buffer, otherwise texture data will be linked to the internal storage (then the data is valid while asset is loaded and there is no texture data copy operations - faster).</param>
    /// <returns>True if cannot load data, otherwise false.</returns>
    bool GetTextureMipData(TextureMipData& result, int32 mipIndex = 0, int32 arrayIndex = 0, bool copyData = true);

    /// <summary>
    /// Gets the texture pixels as Color32 array.
    /// </summary>
    /// <remarks>Supported only for 'basic' texture formats (uncompressed, single plane).</remarks>
    /// <param name="pixels">The result texture pixels array.</param>
    /// <param name="mipIndex">The mip index (zero-based).</param>
    /// <param name="arrayIndex">The array or depth slice index (zero-based).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool GetPixels(API_PARAM(Out) Array<Color32>& pixels, int32 mipIndex = 0, int32 arrayIndex = 0);

    /// <summary>
    /// Gets the texture pixels as Color array.
    /// </summary>
    /// <remarks>Supported only for 'basic' texture formats (uncompressed, single plane).</remarks>
    /// <param name="pixels">The result texture pixels array.</param>
    /// <param name="mipIndex">The mip index (zero-based).</param>
    /// <param name="arrayIndex">The array or depth slice index (zero-based).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool GetPixels(API_PARAM(Out) Array<Color>& pixels, int32 mipIndex = 0, int32 arrayIndex = 0);

    /// <summary>
    /// Sets the texture pixels as Color32 array (asset must be virtual and already initialized).
    /// </summary>
    /// <remarks>Supported only for 'basic' texture formats (uncompressed, single plane).</remarks>
    /// <param name="pixels">The texture pixels array.</param>
    /// <param name="mipIndex">The mip index (zero-based).</param>
    /// <param name="arrayIndex">The array or depth slice index (zero-based).</param>
    /// <param name="generateMips">Enables automatic mip-maps generation (fast point filter) based on the current mip (will generate lower mips).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool SetPixels(const Span<Color32>& pixels, int32 mipIndex = 0, int32 arrayIndex = 0, bool generateMips = false);

    /// <summary>
    /// Sets the texture pixels as Color array (asset must be virtual and already initialized).
    /// </summary>
    /// <remarks>Supported only for 'basic' texture formats (uncompressed, single plane).</remarks>
    /// <param name="pixels">The texture pixels array.</param>
    /// <param name="mipIndex">The mip index (zero-based).</param>
    /// <param name="arrayIndex">The array or depth slice index (zero-based).</param>
    /// <param name="generateMips">Enables automatic mip-maps generation (fast point filter) based on the current mip (will generate lower mips).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool SetPixels(const Span<Color>& pixels, int32 mipIndex = 0, int32 arrayIndex = 0, bool generateMips = false);

    /// <summary>
    /// Initializes the texture with specified initialize data source (asset must be virtual).
    /// </summary>
    /// <param name="initData">The initializer data allocated by the caller with New. It will be owned and released by the asset internal layer.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool Init(InitData* initData);

    /// <summary>
    /// Initializes the texture with specified initialize data source (asset must be virtual).
    /// </summary>
    /// <param name="initData">The initializer data. It will be used and released by the asset internal layer (memory allocation will be swapped).</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool Init(InitData&& initData)
    {
        return Init(New<InitData>(MoveTemp(initData)));
    }

protected:
    virtual int32 CalculateChunkIndex(int32 mipIndex) const;

private:
#if !COMPILE_WITHOUT_CSHARP
    // Internal bindings
    API_FUNCTION(NoProxy) bool InitCSharp(void* ptr);
#endif

public:
    // [BinaryAsset]
    uint64 GetMemoryUsage() const override;
    void CancelStreaming() override;

    // [ITextureOwner]
    CriticalSection& GetOwnerLocker() const override;
    Task* RequestMipDataAsync(int32 mipIndex) override;
    FlaxStorage::LockData LockData() override;
    void GetMipData(int32 mipIndex, BytesContainer& data) const override;
    void GetMipDataWithLoading(int32 mipIndex, BytesContainer& data) const override;
    bool GetMipDataCustomPitch(int32 mipIndex, uint32& rowPitch, uint32& slicePitch) const override;

protected:
    // [BinaryAsset]
    bool init(AssetInitData& initData) override;
    LoadResult load() override;
    void unload(bool isReloading) override;
};
