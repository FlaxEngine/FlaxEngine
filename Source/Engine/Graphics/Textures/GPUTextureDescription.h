// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Color.h"
#include "Engine/Graphics/PixelFormat.h"
#include "Engine/Graphics/Enums.h"

/// <summary>
/// GPU texture usage flags.
/// </summary>
API_ENUM(Attributes="Flags") enum class GPUTextureFlags
{
    /// <summary>
    /// No texture flags.
    /// </summary>
    None = 0x0000,

    /// <summary>
    /// Create a texture that can be bound as a shader resource.
    /// </summary>
    ShaderResource = 0x0001,

    /// <summary>
    /// Create a texture that can be bound as a render target.
    /// </summary>
    RenderTarget = 0x0002,

    /// <summary>
    /// Create a texture can be bound as an unordered access buffer.
    /// </summary>
    UnorderedAccess = 0x0004,

    /// <summary>
    /// Create a texture can be bound as a depth stencil buffer.
    /// </summary>
    DepthStencil = 0x0008,

    /// <summary>
    /// Create texture views per texture mip map (valid only for Texture2D with ShaderResource or RenderTarget flag).
    /// </summary>
    PerMipViews = 0x0010,

    /// <summary>
    /// Create texture views per texture slice map (valid only for Texture3D with ShaderResource or RenderTarget flag).
    /// </summary>
    PerSliceViews = 0x0020,

    /// <summary>
    /// Create read-only view for depth-stencil buffer. Valid only if texture uses depth-stencil and the graphics device supports it.
    /// </summary>
    ReadOnlyDepthView = 0x0040,

    /// <summary>
    /// Create a texture that can be used as a native window swap chain backbuffer surface.
    /// </summary>
    BackBuffer = 0x0080,
};

DECLARE_ENUM_OPERATORS(GPUTextureFlags);

/// <summary>
/// Defines the dimension of a texture object.
/// </summary>
API_ENUM() enum class TextureDimensions
{
    /// <summary>
    /// The texture (2d).
    /// </summary>
    Texture,

    /// <summary>
    /// The volume texture (3d texture).
    /// </summary>
    VolumeTexture,

    /// <summary>
    /// The cube texture (2d texture array of 6 items).
    /// </summary>
    CubeTexture,
};

/// <summary>
/// A common description for all GPU textures.
/// </summary>
API_STRUCT() struct FLAXENGINE_API GPUTextureDescription
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(GPUTextureDescription);

    /// <summary>
    /// The dimensions of the texture.
    /// </summary>
    API_FIELD() TextureDimensions Dimensions;

    /// <summary>
    /// Texture width (in texels).
    /// </summary>
    API_FIELD() int32 Width;

    /// <summary>
    /// Texture height (in texels).
    /// </summary>
    API_FIELD() int32 Height;

    /// <summary>
    /// Texture depth (in texels) for Volume Textures.
    /// </summary>
    API_FIELD() int32 Depth;

    /// <summary>
    /// Number of textures in array for Texture Arrays.
    /// </summary>
    API_FIELD() int32 ArraySize;

    /// <summary>	
    /// The maximum number of mipmap levels in the texture. Use 1 for a multisampled texture; or 0 to generate a full set of subtextures.
    /// </summary>	
    API_FIELD() int32 MipLevels;

    /// <summary>	
    /// Texture format (see <strong><see cref="PixelFormat"/></strong>).
    /// </summary>	
    API_FIELD() PixelFormat Format;

    /// <summary>	
    /// Structure that specifies multisampling parameters for the texture.
    /// </summary>	
    API_FIELD() MSAALevel MultiSampleLevel;

    /// <summary>	
    /// Flags (see <strong><see cref="GPUTextureFlags"/></strong>) for binding to pipeline stages. The flags can be combined by a logical OR.
    /// </summary>
    API_FIELD() GPUTextureFlags Flags;

    /// <summary>	
    /// Value that identifies how the texture is to be read from and written to. The most common value is <see cref="GPUResourceUsage.Default"/>; see <strong><see cref="GPUResourceUsage"/></strong> for all possible values.
    /// </summary>
    API_FIELD() GPUResourceUsage Usage;

    /// <summary>
    /// Default clear color for render targets
    /// </summary>
    API_FIELD() Color DefaultClearColor;

public:
    /// <summary>
    /// Gets a value indicating whether this instance is a render target.
    /// </summary>
    FORCE_INLINE bool IsRenderTarget() const
    {
        return (Flags & GPUTextureFlags::RenderTarget) != GPUTextureFlags::None;
    }

    /// <summary>
    /// Gets a value indicating whether this instance is a depth stencil.
    /// </summary>
    FORCE_INLINE bool IsDepthStencil() const
    {
        return (Flags & GPUTextureFlags::DepthStencil) != GPUTextureFlags::None;
    }

    /// <summary>
    /// Gets a value indicating whether this instance is a shader resource.
    /// </summary>
    FORCE_INLINE bool IsShaderResource() const
    {
        return (Flags & GPUTextureFlags::ShaderResource) != GPUTextureFlags::None;
    }

    /// <summary>
    /// Gets a value indicating whether this instance is a unordered access.
    /// </summary>
    FORCE_INLINE bool IsUnorderedAccess() const
    {
        return (Flags & GPUTextureFlags::UnorderedAccess) != GPUTextureFlags::None;
    }

    /// <summary>
    /// Gets a value indicating whether this instance has per mip level handles.
    /// </summary>
    FORCE_INLINE bool HasPerMipViews() const
    {
        return (Flags & GPUTextureFlags::PerMipViews) != GPUTextureFlags::None;
    }

    /// <summary>
    /// Gets a value indicating whether this instance has per slice views.
    /// </summary>
    FORCE_INLINE bool HasPerSliceViews() const
    {
        return (Flags & GPUTextureFlags::PerSliceViews) != GPUTextureFlags::None;
    }

    /// <summary>
    /// Gets a value indicating whether this instance is a multi sample texture.
    /// </summary>
    FORCE_INLINE bool IsMultiSample() const
    {
        return MultiSampleLevel > MSAALevel::None;
    }

    /// <summary>
    /// Gets a value indicating whether this instance is a cubemap texture.
    /// </summary>
    FORCE_INLINE bool IsCubeMap() const
    {
        return Dimensions == TextureDimensions::CubeTexture;
    }

    /// <summary>
    /// Gets a value indicating whether this instance is a volume texture.
    /// </summary>
    FORCE_INLINE bool IsVolume() const
    {
        return Dimensions == TextureDimensions::VolumeTexture;
    }

    /// <summary>
    /// Gets a value indicating whether this instance is an array texture.
    /// </summary>
    FORCE_INLINE bool IsArray() const
    {
        return ArraySize != 1;
    }

public:
    /// <summary>
    /// Creates a new 1D <see cref="GPUTextureDescription" /> with a single mipmap.
    /// </summary>
    /// <param name="width">The width.</param>
    /// <param name="format">Describes the format to use.</param>
    /// <param name="textureFlags">true if the texture needs to support unordered read write.</param>
    /// <param name="arraySize">Size of the texture 2D array, default to 1.</param>
    /// <returns>A new instance of 1D <see cref="GPUTextureDescription" /> class.</returns>
    static GPUTextureDescription New1D(int32 width, PixelFormat format, GPUTextureFlags textureFlags = GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget, int32 arraySize = 1)
    {
        return New1D(width, format, textureFlags, 1, arraySize);
    }

    /// <summary>
    /// Creates a new 1D <see cref="GPUTextureDescription" />.
    /// </summary>
    /// <param name="width">The width.</param>
    /// <param name="mipCount">Number of mipmaps for the texture. Default is 1. Use 0 to allocate full mip chain.</param>
    /// <param name="format">Describes the format to use.</param>
    /// <param name="textureFlags">true if the texture needs to support unordered read write.</param>
    /// <param name="arraySize">Size of the texture 2D array, default to 1.</param>
    /// <returns>A new instance of 1D <see cref="GPUTextureDescription" /> class.</returns>
    static GPUTextureDescription New1D(int32 width, int32 mipCount, PixelFormat format, GPUTextureFlags textureFlags = GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget, int32 arraySize = 1)
    {
        return New1D(width, format, textureFlags, mipCount, arraySize);
    }

    /// <summary>
    /// Creates a new 1D <see cref="GPUTextureDescription" />.
    /// </summary>
    /// <param name="width">The width.</param>
    /// <param name="format">Describes the format to use.</param>
    /// <param name="textureFlags">true if the texture needs to support unordered read write.</param>
    /// <param name="mipCount">Number of mipmaps for the texture. Default is 1. Use 0 to allocate full mip chain.</param>
    /// <param name="arraySize">Size of the texture 2D array, default to 1.</param>
    /// <returns>A new instance of 1D <see cref="GPUTextureDescription" /> class.</returns>
    static GPUTextureDescription New1D(int32 width, PixelFormat format, GPUTextureFlags textureFlags, int32 mipCount, int32 arraySize);

public:
    /// <summary>
    /// Creates a new <see cref="GPUTextureDescription" /> with a single mipmap.
    /// </summary>
    /// <param name="width">The width.</param>
    /// <param name="height">The height.</param>
    /// <param name="format">Describes the format to use.</param>
    /// <param name="textureFlags">true if the texture needs to support unordered read write.</param>
    /// <param name="arraySize">Size of the texture 2D array, default to 1.</param>
    /// <returns>A new instance of <see cref="GPUTextureDescription" /> class.</returns>
    static GPUTextureDescription New2D(int32 width, int32 height, PixelFormat format, GPUTextureFlags textureFlags = GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget, int32 arraySize = 1)
    {
        return New2D(width, height, 1, format, textureFlags, arraySize);
    }

    /// <summary>
    /// Creates a new <see cref="GPUTextureDescription" />.
    /// </summary>
    /// <param name="width">The width.</param>
    /// <param name="height">The height.</param>
    /// <param name="mipCount">Number of mipmaps for the texture. Default is 1. Use 0 to allocate full mip chain.</param>
    /// <param name="format">Describes the format to use.</param>
    /// <param name="textureFlags">true if the texture needs to support unordered read write.</param>
    /// <param name="arraySize">Size of the texture 2D array, default to 1.</param>
    /// <param name="msaaLevel">The MSAA Level.</param>
    /// <returns>A new instance of <see cref="GPUTextureDescription" /> class.</returns>
    static GPUTextureDescription New2D(int32 width, int32 height, int32 mipCount, PixelFormat format, GPUTextureFlags textureFlags = GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget, int32 arraySize = 1, MSAALevel msaaLevel = MSAALevel::None)
    {
        return New2D(width, height, format, textureFlags, mipCount, arraySize, msaaLevel);
    }

    /// <summary>
    /// Creates a new <see cref="GPUTextureDescription" />.
    /// </summary>
    /// <param name="width">The width.</param>
    /// <param name="height">The height.</param>
    /// <param name="format">Describes the format to use.</param>
    /// <param name="textureFlags">true if the texture needs to support unordered read write.</param>
    /// <param name="mipCount">Number of mipmaps for the texture. Default is 1. Use 0 to allocate full mip chain.</param>
    /// <param name="arraySize">Size of the texture 2D array, default to 1.</param>
    /// <param name="msaaLevel">The MSAA Level.</param>
    /// <returns>A new instance of <see cref="GPUTextureDescription" /> class.</returns>
    static GPUTextureDescription New2D(int32 width, int32 height, PixelFormat format, GPUTextureFlags textureFlags, int32 mipCount, int32 arraySize, MSAALevel msaaLevel = MSAALevel::None);

public:
    /// <summary>
    /// Creates a new <see cref="GPUTextureDescription" /> with a single mipmap.
    /// </summary>
    /// <param name="size">The size (width, height and depth).</param>
    /// <param name="format">Describes the format to use.</param>
    /// <param name="textureFlags">true if the texture needs to support unordered read write.</param>
    /// <returns>A new instance of <see cref="GPUTextureDescription" /> class.</returns>
    static GPUTextureDescription New3D(const Float3& size, PixelFormat format, GPUTextureFlags textureFlags = GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget);

    /// <summary>
    /// Creates a new <see cref="GPUTextureDescription" /> with a single mipmap.
    /// </summary>
    /// <param name="width">The width.</param>
    /// <param name="height">The height.</param>
    /// <param name="depth">The depth.</param>
    /// <param name="format">Describes the format to use.</param>
    /// <param name="textureFlags">true if the texture needs to support unordered read write.</param>
    /// <returns>A new instance of <see cref="GPUTextureDescription" /> class.</returns>
    static GPUTextureDescription New3D(int32 width, int32 height, int32 depth, PixelFormat format, GPUTextureFlags textureFlags = GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget)
    {
        return New3D(width, height, depth, 1, format, textureFlags);
    }

    /// <summary>
    /// Creates a new <see cref="GPUTextureDescription" />.
    /// </summary>
    /// <param name="width">The width.</param>
    /// <param name="height">The height.</param>
    /// <param name="depth">The depth.</param>
    /// <param name="mipCount">Number of mipmaps for the texture. Default is 1. Use 0 to allocate full mip chain.</param>
    /// <param name="format">Describes the format to use.</param>
    /// <param name="textureFlags">true if the texture needs to support unordered read write.</param>
    /// <returns>A new instance of <see cref="GPUTextureDescription" /> class.</returns>
    static GPUTextureDescription New3D(int32 width, int32 height, int32 depth, int32 mipCount, PixelFormat format, GPUTextureFlags textureFlags = GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget)
    {
        return New3D(width, height, depth, format, textureFlags, mipCount);
    }

    /// <summary>
    /// Creates a new <see cref="GPUTextureDescription" />.
    /// </summary>
    /// <param name="width">The width.</param>
    /// <param name="height">The height.</param>
    /// <param name="depth">The depth.</param>
    /// <param name="format">Describes the format to use.</param>
    /// <param name="textureFlags">true if the texture needs to support unordered read write.</param>
    /// <param name="mipCount">Number of mipmaps for the texture. Default is 1. Use 0 to allocate full mip chain.</param>
    /// <returns>A new instance of <see cref="GPUTextureDescription" /> class.</returns>
    static GPUTextureDescription New3D(int32 width, int32 height, int32 depth, PixelFormat format, GPUTextureFlags textureFlags, int32 mipCount);

public:
    /// <summary>
    /// Creates a new Cube <see cref="GPUTextureDescription" />.
    /// </summary>
    /// <param name="size">The size (in pixels) of the top-level faces of the cube texture.</param>
    /// <param name="format">Describes the format to use.</param>
    /// <param name="textureFlags">The texture flags.</param>
    /// <returns>A new instance of <see cref="GPUTextureDescription" /> class.</returns>
    static GPUTextureDescription NewCube(int32 size, PixelFormat format, GPUTextureFlags textureFlags = GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget)
    {
        return NewCube(size, 1, format, textureFlags);
    }

    /// <summary>
    /// Creates a new Cube <see cref="GPUTextureDescription"/>.
    /// </summary>
    /// <param name="size">The size (in pixels) of the top-level faces of the cube texture.</param>
    /// <param name="mipCount">Number of mipmaps for the texture. Default is 1. Use 0 to allocate full mip chain.</param>
    /// <param name="format">Describes the format to use.</param>
    /// <param name="textureFlags">The texture flags.</param>
    /// <returns>A new instance of <see cref="GPUTextureDescription"/> class.</returns>
    static GPUTextureDescription NewCube(int32 size, int32 mipCount, PixelFormat format, GPUTextureFlags textureFlags = GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget)
    {
        return NewCube(size, format, textureFlags, mipCount);
    }

    /// <summary>
    /// Creates a new Cube <see cref="GPUTextureDescription"/>.
    /// </summary>
    /// <param name="size">The size (in pixels) of the top-level faces of the cube texture.</param>
    /// <param name="format">Describes the format to use.</param>
    /// <param name="textureFlags">The texture flags.</param>
    /// <param name="mipCount">Number of mipmaps for the texture. Default is 1. Use 0 to allocate full mip chain.</param>
    /// <returns>A new instance of <see cref="GPUTextureDescription"/> class.</returns>
    static GPUTextureDescription NewCube(int32 size, PixelFormat format, GPUTextureFlags textureFlags, int32 mipCount);

public:
    void Clear();
    GPUTextureDescription ToStagingUpload() const;
    GPUTextureDescription ToStagingReadback() const;
    GPUTextureDescription ToStaging() const;
    bool Equals(const GPUTextureDescription& other) const;
    String ToString() const;

public:
    FORCE_INLINE bool operator==(const GPUTextureDescription& other) const
    {
        return Equals(other);
    }

    FORCE_INLINE bool operator!=(const GPUTextureDescription& other) const
    {
        return !Equals(other);
    }
};

uint32 GetHash(const GPUTextureDescription& key);
