// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "ITextureOwner.h"
#include "Engine/Streaming/StreamableResource.h"
#include "Types.h"

/// <summary>
/// GPU texture object which can change it's resolution (quality) at runtime.
/// </summary>
class FLAXENGINE_API StreamingTexture : public Object, public StreamableResource
{
    friend class TextureBase;
    friend class StreamTextureMipTask;
    friend class StreamTextureResizeTask;
protected:

    ITextureOwner* _owner;
    GPUTexture* _texture;
    TextureHeader _header;
    volatile mutable int64 _streamingTasksCount;
    bool _isBlockCompressed;

public:

    StreamingTexture(ITextureOwner* owner, const String& name);
    ~StreamingTexture();

public:

    /// <summary>
    /// Gets the owner.
    /// </summary>
    FORCE_INLINE ITextureOwner* GetOwner() const
    {
        return _owner;
    }

    /// <summary>
    /// Gets texture object handle.
    /// </summary>
    FORCE_INLINE GPUTexture* GetTexture() const
    {
        return _texture;
    }

    /// <summary>
    /// Gets texture size of Vector2::Zero if not loaded.
    /// </summary>
    Vector2 Size() const;

    /// <summary>
    /// Gets a value indicating whether this instance is initialized. 
    /// </summary>
    FORCE_INLINE bool IsInitialized() const
    {
        return _header.MipLevels > 0;
    }

    /// <summary>
    /// Gets total texture width (in texels)
    /// </summary>
    FORCE_INLINE int32 TotalWidth() const
    {
        return _header.Width;
    }

    /// <summary>
    /// Gets total texture height (in texels)
    /// </summary>
    FORCE_INLINE int32 TotalHeight() const
    {
        return _header.Height;
    }

    /// <summary>
    /// Gets total texture array size
    /// </summary>
    FORCE_INLINE int32 TotalArraySize() const
    {
        return IsCubeMap() ? 6 : 1;
    }

    /// <summary>
    /// Gets total texture mip levels count
    /// </summary>
    FORCE_INLINE int32 TotalMipLevels() const
    {
        return _header.MipLevels;
    }

    /// <summary>
    /// Returns texture format type
    /// </summary>
    FORCE_INLINE TextureFormatType GetFormatType() const
    {
        return _header.Type;
    }

    /// <summary>
    /// Returns true if it's a cube map texture
    /// </summary>
    FORCE_INLINE bool IsCubeMap() const
    {
        return _header.IsCubeMap;
    }

    /// <summary>
    /// Returns true if texture cannot be used during GPU resources streaming system
    /// </summary>
    FORCE_INLINE bool NeverStream() const
    {
        return _header.NeverStream;
    }

    /// <summary>
    /// Gets the texture header.
    /// </summary>
    FORCE_INLINE const TextureHeader* GetHeader() const
    {
        return &_header;
    }

    /// <summary>
    /// Gets a boolean indicating whether this <see cref="StreamingTexture"/> is a using a block compress format (BC1, BC2, BC3, BC4, BC5, BC6H, BC7).
    /// </summary>
    FORCE_INLINE bool IsBlockCompressed() const
    {
        return _isBlockCompressed;
    }

public:

    /// <summary>
    /// Converts allocated texture mip index to the absolute mip map index.
    /// </summary>
    /// <param name="textureMipIndex">Index of the texture mip.</param>
    /// <returns>The index of the mip map.</returns>
    int32 TextureMipIndexToTotalIndex(int32 textureMipIndex) const;

    /// <summary>
    /// Converts absolute mip map index to the allocated texture mip index.
    /// </summary>
    /// <param name="mipIndex">Index of the texture mip.</param>
    /// <returns>The index of the mip map.</returns>
    int32 TotalIndexToTextureMipIndex(int32 mipIndex) const;

public:

    /// <summary>
    /// Creates new texture
    /// </summary>
    /// <param name="header">Texture header</param>
    /// <returns>True if cannot create texture, otherwise false</returns>
    bool Create(const TextureHeader& header);

    /// <summary>
    /// Release texture
    /// </summary>
    void UnloadTexture();

    /// <summary>
    /// Gets the total memory usage that texture may have in use (if loaded to the maximum quality).
    /// Exact value may differ due to memory alignment and resource allocation policy.
    /// </summary>
    /// <returns>The amount of bytes.</returns>
    uint64 GetTotalMemoryUsage() const;

public:

    FORCE_INLINE GPUTexture* operator->() const
    {
        return _texture;
    }

public:

    // [Object]
    String ToString() const override;

    // [StreamableResource]
    int32 GetMaxResidency() const override;
    int32 GetCurrentResidency() const override;
    int32 GetAllocatedResidency() const override;
    bool CanBeUpdated() const override;
    Task* UpdateAllocation(int32 residency) override;
    Task* CreateStreamingTask(int32 residency) override;
};
