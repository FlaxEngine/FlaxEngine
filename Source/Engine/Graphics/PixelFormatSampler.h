// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "PixelFormat.h"

/// <summary>
/// Utility for writing and reading from different pixels formats within a single code path.
/// </summary>
API_STRUCT(NoDefault) struct FLAXENGINE_API PixelFormatSampler
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(PixelFormatSampler);
    typedef Float4 (*ReadPixel)(const void* data);
    typedef void (*WritePixel)(void* data, const Float4& value);

public:
    // Element format.
    PixelFormat Format;
    // Element size in bytes.
    int32 PixelSize;
    // Read data function.
    ReadPixel Read;
    // Write data function.
    WritePixel Write;

public:
    /// <summary>
    /// Stores the color into the specified texture data (uses no interpolation).
    /// </summary>
    /// <param name="data">The data pointer for the texture slice (1D or 2D image).</param>
    /// <param name="x">The X texture coordinates (normalized to range 0-width).</param>
    /// <param name="y">The Y texture coordinates (normalized to range 0-height).</param>
    /// <param name="rowPitch">The row pitch (in bytes). The offset between each image rows.</param>
    /// <param name="color">The color to store (linear).</param>
    void Store(void* data, int32 x, int32 y, int32 rowPitch, const Color& color) const;

    /// <summary>
    /// Samples the specified linear data (uses no interpolation).
    /// </summary>
    /// <param name="data">The data pointer for the data slice (linear buffer or 1D image).</param>
    /// <param name="x">Index of the element.</param>
    /// <returns>The sampled value.</returns>
    Float4 Sample(const void* data, int32 x) const;

    /// <summary>
    /// Samples the specified texture data (uses no interpolation).
    /// </summary>
    /// <param name="data">The data pointer for the texture slice (1D or 2D image).</param>
    /// <param name="uv">The texture coordinates (normalized to range 0-1).</param>
    /// <param name="size">The size of the input texture (in pixels).</param>
    /// <param name="rowPitch">The row pitch (in bytes). The offset between each image rows.</param>
    /// <returns>The sampled color (linear).</returns>
    Color SamplePoint(const void* data, const Float2& uv, const Int2& size, int32 rowPitch) const;

    /// <summary>
    /// Samples the specified texture data (uses no interpolation).
    /// </summary>
    /// <param name="data">The data pointer for the texture slice (1D or 2D image).</param>
    /// <param name="x">The X texture coordinates (normalized to range 0-width).</param>
    /// <param name="y">The Y texture coordinates (normalized to range 0-height).</param>
    /// <param name="rowPitch">The row pitch (in bytes). The offset between each image rows.</param>
    /// <returns>The sampled color (linear).</returns>
    Color SamplePoint(const void* data, int32 x, int32 y, int32 rowPitch) const;

    /// <summary>
    /// Samples the specified texture data (uses linear interpolation).
    /// </summary>
    /// <param name="data">The data pointer for the texture slice (1D or 2D image).</param>
    /// <param name="uv">The texture coordinates (normalized to range 0-1).</param>
    /// <param name="size">The size of the input texture (in pixels).</param>
    /// <param name="rowPitch">The row pitch (in bytes). The offset between each image rows.</param>
    /// <returns>The sampled color (linear).</returns>
    Color SampleLinear(const void* data, const Float2& uv, const Int2& size, int32 rowPitch) const;

public:
    /// <summary>
    /// Tries to get a sampler tool for the specified format to read pixels.
    /// </summary>
    /// <param name="format">The format.</param>
    /// <returns>The pointer to the sampler object or null when cannot sample the given format.</returns>
    static const PixelFormatSampler* Get(PixelFormat format);
};
