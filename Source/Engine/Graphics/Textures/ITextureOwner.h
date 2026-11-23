// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Content/Storage/FlaxStorage.h"

class GPUTexture;
class Task;

/// <summary>
/// Enumeration of different image formats.
/// </summary>
API_ENUM() enum class ImageFormat
{
    // .dds
    DDS,
    // .tga
    TGA,
    // .png
    PNG,
    // .bmp
    BMP,
    // .gif
    GIF,
    // .tiff
    TIFF,
    // .jpg/.jpeg
    JPEG,
    // .hdr
    HDR,
    // .raw
    RAW,
    // .exr
    EXR,
    // Custom
    Internal,
};

/// <summary>
/// Interface for objects that can manage streamable texture
/// </summary>
class FLAXENGINE_API ITextureOwner
{
public:
    /// <summary>
    /// Gets the texture owner mutex used to synchronize texture logic.
    /// </summary>
    virtual CriticalSection& GetOwnerLocker() const = 0;

    /// <summary>
    /// Get texture mip map data
    /// </summary>
    /// <param name="mipIndex">Mip map index</param>
    /// <returns>Task that will get asset data (may be null if data already loaded).</returns>
    virtual Task* RequestMipDataAsync(int32 mipIndex) = 0;

    /// <summary>
    /// Prepares texture data. May lock data chunks to be keep in cache for a while.
    /// </summary>
    virtual FlaxStorage::LockData LockData() = 0;

    /// <summary>
    /// Gets texture mip map data. May fail if no data requested and canLoad is unset (see RequestMipDataAsync).
    /// </summary>
    /// <param name="mipIndex">Mip map index</param>
    /// <param name="data">Result data</param>
    /// <param name="rowPitch">Data row pitch (in bytes).</param>
    /// <param name="slicePitch">Data slice pitch (in bytes).</param>
    /// <param name="canLoad">True if load data from drive if it's not in memory.</param>
    virtual void GetMipData(int32 mipIndex, BytesContainer& data, uint32& rowPitch, uint32& slicePitch, bool canLoad = false) const = 0;
};
