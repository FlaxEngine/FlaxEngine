// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUResource.h"
#include "GPUTextureDescription.h"

class GPUContext;
class GPUTask;
class TextureMipData;
class TextureData;
template<typename T>
class DataContainer;
typedef DataContainer<byte> BytesContainer;
class Task;

/// <summary>
/// Defines a view for the <see cref="GPUTexture"/> surface, full resource or any of the sub-parts. Can be used to define a single subresource of the texture, volume texture or texture array. Used to render to the texture and/or use textures in the shaders.
/// </summary>
API_CLASS(Sealed, NoSpawn) class FLAXENGINE_API GPUTextureView : public GPUResourceView
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(GPUTextureView);
protected:
    PixelFormat _format = PixelFormat::Unknown;
    MSAALevel _msaa = MSAALevel::None;

    GPUTextureView()
        : GPUResourceView(SpawnParams(Guid::New(), TypeInitializer))
    {
    }

    FORCE_INLINE void Init(GPUResource* parent, PixelFormat format, MSAALevel msaa)
    {
        _parent = parent;
        _format = format;
        _msaa = msaa;
        if (parent)
            LastRenderTime = &parent->LastRenderTime;
    }

public:
    /// <summary>
    /// Gets the view format.
    /// </summary>
    API_PROPERTY() FORCE_INLINE PixelFormat GetFormat() const
    {
        return _format;
    }

    /// <summary>
    /// Gets view MSAA level.
    /// </summary>
    API_PROPERTY() FORCE_INLINE MSAALevel GetMSAA() const
    {
        return _msaa;
    }
};

/// <summary>
/// The GPU texture resource object. This class is able to create 2D/3D textures, volume textures and render targets.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API GPUTexture : public GPUResource
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(GPUTexture);
    static GPUTexture* Spawn(const SpawnParams& params);
    static GPUTexture* New();

protected:
    int32 _residentMipLevels;
    bool _sRGB, _isBlockCompressed;
    GPUTextureDescription _desc;

    GPUTexture();

public:
    /// <summary>
    /// Gets a value indicating whether this texture has any resided mip (data already uploaded to the GPU).
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool HasResidentMip() const
    {
        return _residentMipLevels != 0;
    }

    /// <summary>
    /// Gets a value indicating whether this texture has been allocated.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool IsAllocated() const
    {
        return _desc.MipLevels > 0;
    }

    /// <summary>	
    /// Gets texture width (in texels).
    /// </summary>	
    API_PROPERTY() FORCE_INLINE int32 Width() const
    {
        return _desc.Width;
    }

    /// <summary>	
    /// Gets texture height (in texels).
    /// </summary>	
    API_PROPERTY() FORCE_INLINE int32 Height() const
    {
        return _desc.Height;
    }

    /// <summary>	
    /// Gets texture depth (in texels).
    /// </summary>	
    API_PROPERTY() FORCE_INLINE int32 Depth() const
    {
        return _desc.Depth;
    }

    /// <summary>	
    /// Gets number of textures in the array.
    /// </summary>	
    API_PROPERTY() FORCE_INLINE int32 ArraySize() const
    {
        return _desc.ArraySize;
    }

    /// <summary>	
    /// Gets multi-sampling parameters for the texture.
    /// </summary>	
    API_PROPERTY() FORCE_INLINE MSAALevel MultiSampleLevel() const
    {
        return _desc.MultiSampleLevel;
    }

    /// <summary>	
    /// Gets number of mipmap levels in the texture. 
    /// </summary>	
    API_PROPERTY() FORCE_INLINE int32 MipLevels() const
    {
        return _desc.MipLevels;
    }

    /// <summary>	
    /// Gets the number of resident mipmap levels in the texture (already uploaded to the GPU).
    /// </summary>	
    API_PROPERTY() FORCE_INLINE int32 ResidentMipLevels() const
    {
        return _residentMipLevels;
    }

    /// <summary>
    /// Gets the index of the highest resident mip map (may be equal to MipLevels if no mip has been uploaded). Note: mip=0 is the highest (top quality).
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 HighestResidentMipIndex() const
    {
        return MipLevels() - ResidentMipLevels();
    }

    /// <summary>	
    /// Gets texture data format.
    /// </summary>	
    API_PROPERTY() FORCE_INLINE PixelFormat Format() const
    {
        return _desc.Format;
    }

    /// <summary>	
    /// Gets flags of the texture.
    /// </summary>
    API_PROPERTY() FORCE_INLINE GPUTextureFlags Flags() const
    {
        return _desc.Flags;
    }

    /// <summary>
    /// Gets texture dimensions.
    /// </summary>
    API_PROPERTY() FORCE_INLINE TextureDimensions Dimensions() const
    {
        return _desc.Dimensions;
    }

    /// <summary>
    /// Gets texture description structure.
    /// </summary>
    API_PROPERTY() FORCE_INLINE const GPUTextureDescription& GetDescription() const
    {
        return _desc;
    }

public:
    /// <summary>
    /// Gets a value indicating whether this texture is a render target.
    /// </summary>
    FORCE_INLINE bool IsRenderTarget() const
    {
        return _desc.IsRenderTarget();
    }

    /// <summary>
    /// Gets a value indicating whether this texture is a shader resource.
    /// </summary>
    FORCE_INLINE bool IsShaderResource() const
    {
        return _desc.IsShaderResource();
    }

    /// <summary>
    /// Gets a value indicating whether this texture is a depth stencil.
    /// </summary>
    FORCE_INLINE bool IsDepthStencil() const
    {
        return _desc.IsDepthStencil();
    }

    /// <summary>
    /// Gets a value indicating whether this texture is a unordered access.
    /// </summary>
    FORCE_INLINE bool IsUnorderedAccess() const
    {
        return _desc.IsUnorderedAccess();
    }

    /// <summary>
    /// Gets a value indicating whether this instance has per mip level views.
    /// </summary>
    FORCE_INLINE bool HasPerMipViews() const
    {
        return _desc.HasPerMipViews();
    }

    /// <summary>
    /// Gets a value indicating whether this instance has per slice views.
    /// </summary>
    FORCE_INLINE bool HasPerSliceViews() const
    {
        return _desc.HasPerSliceViews();
    }

    /// <summary>
    /// Gets a value indicating whether this instance is a multi sample texture.
    /// </summary>
    FORCE_INLINE bool IsMultiSample() const
    {
        return _desc.IsMultiSample();
    }

    /// <summary>
    /// Gets a value indicating whether this instance is a cubemap texture.
    /// </summary>
    FORCE_INLINE bool IsCubeMap() const
    {
        return _desc.Dimensions == TextureDimensions::CubeTexture;
    }

    /// <summary>
    /// Gets a value indicating whether this instance is a volume texture.
    /// </summary>
    FORCE_INLINE bool IsVolume() const
    {
        return _desc.Dimensions == TextureDimensions::VolumeTexture;
    }

    /// <summary>
    /// Gets a value indicating whether this instance is an array texture.
    /// </summary>
    FORCE_INLINE bool IsArray() const
    {
        return _desc.ArraySize != 1;
    }

    /// <summary>
    /// Checks if texture contains sRGB colors data.
    /// </summary>
    FORCE_INLINE bool IsSRGB() const
    {
        return _sRGB;
    }

    /// <summary>
    /// Checks if texture is normal texture asset (not render target or unordered access or depth buffer or sth else).
    /// </summary>
    FORCE_INLINE bool IsRegularTexture() const
    {
        return _desc.Flags == GPUTextureFlags::ShaderResource;
    }

    /// <summary>
    /// Checks if texture is a staging buffer (supports direct CPU access).
    /// </summary>
    bool IsStaging() const;

    /// <summary>
    /// Gets a boolean indicating whether this <see cref="GPUTexture"/> is a using a block compress format (BC1, BC2, BC3, BC4, BC5, BC6H, BC7, etc.).
    /// </summary>
    FORCE_INLINE bool IsBlockCompressed() const
    {
        return _isBlockCompressed;
    }

public:
    /// <summary>
    /// Gets the texture total size in pixels.
    /// </summary>
    API_PROPERTY() Float2 Size() const;

    /// <summary>
    /// Gets the texture total size in pixels (with depth).
    /// </summary>
    API_PROPERTY() Float3 Size3() const;

    /// <summary>
    /// Returns true if texture has size that is power of two.
    /// </summary>
    /// <returns>True if texture has size that is power of two.</returns>
    API_PROPERTY() bool IsPowerOfTwo() const;

    /// <summary>
    /// Gets the texture mip map dimensions.
    /// </summary>
    /// <param name="mipLevelIndex">Mip level index (zero-based where 0 is top texture surface).</param>
    /// <param name="mipWidth">The calculated mip level width (in pixels).</param>
    /// <param name="mipHeight">The calculated mip level height (in pixels).</param>
    API_FUNCTION() void GetMipSize(int32 mipLevelIndex, API_PARAM(Out) int32& mipWidth, API_PARAM(Out) int32& mipHeight) const;

    /// <summary>
    /// Gets the texture mip map dimensions.
    /// </summary>
    /// <param name="mipLevelIndex">Mip level index (zero-based where 0 is top texture surface).</param>
    /// <param name="mipWidth">The calculated mip level width (in pixels).</param>
    /// <param name="mipHeight">The calculated mip level height (in pixels).</param>
    /// <param name="mipDepth">The calculated mip level depth (in pixels).</param>
    API_FUNCTION() void GetMipSize(int32 mipLevelIndex, API_PARAM(Out) int32& mipWidth, API_PARAM(Out) int32& mipHeight, API_PARAM(Out) int32& mipDepth) const;

    /// <summary>
    /// Gets current texture size (uploaded to the GPU and in use).
    /// </summary>
    /// <param name="width">The current width (in pixels).</param>
    /// <param name="height">The current height (in pixels).</param>
    void GetResidentSize(int32& width, int32& height) const;

    /// <summary>
    /// Gets current texture size (uploaded to the GPU and in use).
    /// </summary>
    /// <param name="width">The current width (in pixels).</param>
    /// <param name="height">The current height (in pixels).</param>
    /// <param name="depth">The current depth (in pixels).</param>
    void GetResidentSize(int32& width, int32& height, int32& depth) const;

public:
    /// <summary>
    /// Calculates mip map row pitch (in bytes).
    /// </summary>
    /// <param name="mipIndex">Index of the mip.</param>
    /// <returns>Row pitch.</returns>
    uint32 RowPitch(int32 mipIndex = 0) const;

    /// <summary>
    /// Calculates mip map slice pitch (in bytes).
    /// </summary>
    /// <param name="mipIndex">Index of the mip.</param>
    /// <returns>Slice pitch.</returns>
    uint32 SlicePitch(int32 mipIndex = 0) const;

    /// <summary>
    /// Computes row and slice pitch of the mip map.
    /// </summary>
    /// <param name="mipIndex">Index of the mip.</param>
    /// <param name="rowPitch">The row pitch.</param>
    /// <param name="slicePitch">The slice pitch.</param>
    void ComputePitch(int32 mipIndex, uint32& rowPitch, uint32& slicePitch) const;

    /// <summary>
    /// Calculates the size of a particular mip.
    /// </summary>
    /// <param name="size">The size.</param>
    /// <param name="mipLevel">The mip level.</param>
    /// <returns>Mip size.</returns>
    int32 CalculateMipSize(int32 size, int32 mipLevel) const;

public:
    int32 ComputeSubresourceSize(int32 subresource, int32 rowAlign, int32 sliceAlign) const;
    int32 ComputeBufferOffset(int32 subresource, int32 rowAlign, int32 sliceAlign) const;
    int32 ComputeBufferTotalSize(int32 rowAlign, int32 sliceAlign) const;
    int32 ComputeSlicePitch(int32 mipLevel, int32 rowAlign) const;
    int32 ComputeRowPitch(int32 mipLevel, int32 rowAlign) const;

public:
    /// <summary>
    /// Gets the view to the first surface (only for 2D textures).
    /// </summary>
    /// <returns>The view to the main texture surface.</returns>
    API_FUNCTION() FORCE_INLINE GPUTextureView* View() const
    {
        return View(0);
    }

    /// <summary>
    /// Gets the view to the surface at index in an array.
    /// </summary>
    /// <remarks>
    /// To use per depth/array slice view you need to specify the <see cref="GPUTextureFlags.PerSliceViews"/> when creating the resource.
    /// </remarks>
    /// <param name="arrayOrDepthIndex">The index of the surface in an array (or depth slice index).</param>
    /// <returns>The view to the surface at index in an array.</returns>
    API_FUNCTION() virtual GPUTextureView* View(int32 arrayOrDepthIndex) const = 0;

    /// <summary>
    /// Gets the view to the mip map surface at index in an array.
    /// </summary>
    /// <remarks>
    /// To use per mip map view you need to specify the <see cref="GPUTextureFlags.PerMipViews"/> when creating the resource.
    /// </remarks>
    /// <param name="arrayOrDepthIndex">The index of the surface in an array (or depth slice index).</param>
    /// <param name="mipMapIndex">Index of the mip level.</param>
    /// <returns>The view to the surface at index in an array.</returns>
    API_FUNCTION() virtual GPUTextureView* View(int32 arrayOrDepthIndex, int32 mipMapIndex) const = 0;

    /// <summary>
    /// Gets the view to the array of surfaces 
    /// </summary>
    /// <remarks>
    /// To use array texture view you need to create render target as an array.
    /// </remarks>
    /// <returns>The view to the array of surfaces.</returns>
    API_FUNCTION() virtual GPUTextureView* ViewArray() const = 0;

    /// <summary>
    /// Gets the view to the volume texture (3D).
    /// </summary>
    /// <remarks>
    /// To use volume texture view you need to create render target as a volume resource (3D texture with Depth > 1).
    /// </remarks>
    /// <returns>The view to the volume texture.</returns>
    API_FUNCTION() virtual GPUTextureView* ViewVolume() const = 0;

    /// <summary>
    /// Gets the view to the texture as read-only depth/stencil buffer. Valid only if graphics device supports it and the texture uses depth/stencil.
    /// </summary>
    /// <returns>The view to the depth-stencil resource descriptor as read-only depth.</returns>
    API_FUNCTION() virtual GPUTextureView* ViewReadOnlyDepth() const = 0;

    /// <summary>
    /// Implicit conversion to the first surface (only for 2D textures).
    /// </summary>
    /// <returns>The view to the main texture surface.</returns>
    FORCE_INLINE operator GPUTextureView*() const
    {
        return View(0);
    }

public:
    /// <summary>
    /// Initializes a texture resource (allocates the GPU memory and performs the resource setup).
    /// </summary>
    /// <param name="desc">The texture description.</param>
    /// <returns>True if cannot create texture, otherwise false.</returns>
    API_FUNCTION() bool Init(API_PARAM(Ref) const GPUTextureDescription& desc);

    /// <summary>
    /// Creates new staging readback texture with the same dimensions and properties as a source texture (but without a data transferred; warning: caller must delete object).
    /// </summary>
    /// <returns>The staging readback texture.</returns>
    GPUTexture* ToStagingReadback() const;

    /// <summary>
    /// Creates new staging upload texture with the same dimensions and properties as a source texture (but without a data transferred; warning: caller must delete object).
    /// </summary>
    /// <returns>The staging upload texture.</returns>
    GPUTexture* ToStagingUpload() const;

    /// <summary>
    /// Resizes the texture. It must be created first.
    /// </summary>
    /// <param name="width">The width.</param>
    /// <param name="height">The height.</param>
    /// <param name="format">The new texture format. Use Unknown to remain texture format unchanged.</param>
    /// <returns>True if fails, otherwise false.</returns>
    API_FUNCTION() bool Resize(int32 width, int32 height, PixelFormat format = PixelFormat::Unknown)
    {
        const auto depth = IsAllocated() ? Depth() : 1;
        return Resize(width, height, depth, format);
    }

    /// <summary>
    /// Resizes the texture. It must be created first.
    /// </summary>
    /// <param name="width">The width.</param>
    /// <param name="height">The height.</param>
    /// <param name="depth">The depth.</param>
    /// <param name="format">The new texture format. Use Unknown to remain texture format unchanged.</param>
    /// <returns>True if fails, otherwise false.</returns>
    API_FUNCTION() bool Resize(int32 width, int32 height, int32 depth, PixelFormat format = PixelFormat::Unknown);

public:
    /// <summary>
    /// Gets the native pointer to the underlying resource. It's a low-level platform-specific handle.
    /// </summary>
    /// <returns>The pointer.</returns>
    API_PROPERTY() virtual void* GetNativePtr() const = 0;

    /// <summary>
    /// Uploads mip map data to the GPU. Creates async GPU task.
    /// </summary>
    /// <param name="data">Data to upload (it must be valid for the next a few frames due to GPU latency and async works executing)</param>
    /// <param name="mipIndex">Mip level index.</param>
    /// <param name="copyData">If true, the data will be copied to the async execution task instead of using the input pointer provided.</param>
    /// <returns>Created async task or null if cannot.</returns>
    GPUTask* UploadMipMapAsync(const BytesContainer& data, int32 mipIndex, bool copyData = false);

    /// <summary>
    /// Uploads mip map data to the GPU. Creates async GPU task.
    /// </summary>
    /// <param name="data">Data to upload (it must be valid for the next a few frames due to GPU latency and async works executing)</param>
    /// <param name="mipIndex">Mip level index.</param>
    /// <param name="rowPitch">The data row pitch.</param>
    /// <param name="slicePitch">The data slice pitch.</param>
    /// <param name="copyData">If true, the data will be copied to the async execution task instead of using the input pointer provided.</param>
    /// <returns>Created async task or null if cannot.</returns>
    GPUTask* UploadMipMapAsync(const BytesContainer& data, int32 mipIndex, int32 rowPitch, int32 slicePitch, bool copyData = false);

    /// <summary>
    /// Stops current thread execution to gather texture data from the GPU.
    /// </summary>
    /// <param name="result">The result data.</param>
    /// <returns>True if cannot download data, otherwise false.</returns>
    bool DownloadData(TextureData& result);

    /// <summary>
    /// Creates GPU async task that will gather texture data from the GPU.
    /// </summary>
    /// <param name="result">The result data.</param>
    /// <returns>Download data task (not started yet).</returns>
    Task* DownloadDataAsync(TextureData& result);

    /// <summary>
    /// Gets texture mipmap data (raw bytes). Can be used only with textures created with Staging flag.
    /// </summary>
    /// <param name="arrayIndex">Array slice index.</param>
    /// <param name="mipMapIndex">Mip map index.</param>
    /// <param name="data">Output mip data.</param>
    /// <param name="mipRowPitch">Output mip data row pitch to use. Use 0 to use the pitch from the internal GPU storage.</param>
    /// <returns>True if failed, otherwise false.</returns>
    virtual bool GetData(int32 arrayIndex, int32 mipMapIndex, TextureMipData& data, uint32 mipRowPitch = 0) = 0;

    /// <summary>
    /// Sets the number of resident mipmap levels in the texture (already uploaded to the GPU).
    /// </summary>
    API_PROPERTY() void SetResidentMipLevels(int32 count);

    /// <summary>
    /// Event called when texture residency gets changed. Texture Mip gets loaded into GPU memory and is ready to use.
    /// </summary>
    Delegate<GPUTexture*> ResidentMipsChanged;

protected:
    virtual bool OnInit() = 0;
    uint64 calculateMemoryUsage() const;
    virtual void OnResidentMipsChanged() = 0;

public:
    // [GPUResource]
    String ToString() const override;
    GPUResourceType GetResourceType() const final override;

protected:
    // [GPUResource]
    void OnReleaseGPU() override;
};
