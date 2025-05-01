// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Graphics/Config.h"
#include "Engine/Graphics/PixelFormat.h"

/// <summary>
/// Single texture mip map entry data.
/// </summary>
class FLAXENGINE_API TextureMipData
{
public:
    uint32 RowPitch;
    uint32 DepthPitch;
    uint32 Lines;
    BytesContainer Data;

    TextureMipData();
    TextureMipData(const TextureMipData& other);
    TextureMipData(TextureMipData&& other) noexcept;
    TextureMipData& operator=(const TextureMipData& other);
    TextureMipData& operator=(TextureMipData&& other) noexcept;

    bool GetPixels(Array<Color32>& pixels, int32 width, int32 height, PixelFormat format) const;
    bool GetPixels(Array<Color>& pixels, int32 width, int32 height, PixelFormat format) const;
    void Copy(void* data, uint32 dataRowPitch, uint32 dataDepthPitch, uint32 dataDepthSlices, uint32 targetRowPitch);

    template<typename T>
    T& Get(int32 x, int32 y)
    {
        return *(T*)(Data.Get() + y * RowPitch + x * sizeof(T));
    }

    template<typename T>
    const T& Get(int32 x, int32 y) const
    {
        return *(const T*)(Data.Get() + y * RowPitch + x * sizeof(T));
    }
};

/// <summary>
/// Texture data container (used to keep data downloaded from the GPU).
/// </summary>
API_CLASS() class FLAXENGINE_API TextureData : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(TextureData, ScriptingObject);
public:
    /// <summary>
    /// Single entry of the texture array. Contains collection of mip maps.
    /// </summary>
    struct FLAXENGINE_API ArrayEntry
    {
        /// <summary>
        /// The mip maps collection.
        /// </summary>
        Array<TextureMipData, FixedAllocation<GPU_MAX_TEXTURE_MIP_LEVELS>> Mips;
    };

public:
    /// <summary>
    /// Top level texture surface width (in pixels).
    /// </summary>
    API_FIELD(ReadOnly) int32 Width = 0;

    /// <summary>
    /// Top level texture surface height (in pixels).
    /// </summary>
    API_FIELD(ReadOnly) int32 Height = 0;

    /// <summary>
    /// Top level texture surface depth (in pixels).
    /// </summary>
    API_FIELD(ReadOnly) int32 Depth = 0;

    /// <summary>
    /// The texture data format.
    /// </summary>
    API_FIELD(ReadOnly) PixelFormat Format = PixelFormat::Unknown;

    /// <summary>
    /// The items collection (depth slices or array slices).
    /// </summary>
    Array<ArrayEntry, InlinedAllocation<6>> Items;

public:
    /// <summary>
    /// Gather texture data
    /// </summary>
    /// <param name="arrayIndex">Texture array index</param>
    /// <param name="mipLevel">Mip map index</param>
    /// <returns>Result data</returns>
    TextureMipData* GetData(int32 arrayIndex, int32 mipLevel)
    {
        return &Items[arrayIndex].Mips[mipLevel];
    }

    /// <summary>
    /// Gather texture data
    /// </summary>
    /// <param name="arrayIndex">Texture array index</param>
    /// <param name="mipLevel">Mip map index</param>
    /// <returns>Result data</returns>
    const TextureMipData* GetData(int32 arrayIndex, int32 mipLevel) const
    {
        return &Items[arrayIndex].Mips[mipLevel];
    }

    /// <summary>
    /// Gets amount of texture slices in the array.
    /// </summary>
    API_PROPERTY() int32 GetArraySize() const;

    /// <summary>
    /// Gets amount of mip maps in the texture.
    /// </summary>
    API_PROPERTY() int32 GetMipLevels() const;

    /// <summary>
    /// Clear allocated memory.
    /// </summary>
    API_FUNCTION() void Clear();

public:
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
};
