// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Graphics/Config.h"
#include "Engine/Graphics/PixelFormat.h"

/// <summary>
/// Single texture mip map entry data.
/// </summary>
API_STRUCT() struct FLAXENGINE_API TextureMipData
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(TextureMipData);
public:
    // The row pitch.
    API_FIELD() uint32 RowPitch;

    // The depth pitch.
    API_FIELD() uint32 DepthPitch;

    // The number of lines.
    API_FIELD() uint32 Lines;

    // The data.
    API_FIELD(ReadOnly) BytesContainer Data;

    TextureMipData();
    TextureMipData(const TextureMipData& other);
    TextureMipData(TextureMipData&& other) noexcept;
    TextureMipData& operator=(const TextureMipData& other);
    TextureMipData& operator=(TextureMipData&& other) noexcept;

    bool GetPixels(Array<Color32>& pixels, int32 width, int32 height, PixelFormat format) const;
    bool GetPixels(Array<Color>& pixels, int32 width, int32 height, PixelFormat format) const;

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

template<>
struct TIsPODType<TextureMipData>
{
    enum { Value = true };
};

/// <summary>
/// Single entry of the texture array. Contains collection of mip maps.
/// </summary>
API_STRUCT(NoDefault) struct FLAXENGINE_API TextureDataArrayEntry
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(TextureDataArrayEntry);

    /// <summary>
    /// The mip maps collection.
    /// </summary>
    API_FIELD(ReadOnly) Array<TextureMipData, FixedAllocation<GPU_MAX_TEXTURE_MIP_LEVELS>> Mips;
};

template<>
struct TIsPODType<TextureDataArrayEntry>
{
    enum { Value = false };
};

/// <summary>
/// Texture data container (used to keep data downloaded from the GPU).
/// </summary>
API_STRUCT() struct FLAXENGINE_API TextureData
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(TextureData);
public:
    

public:
    /// <summary>
    /// Init
    /// </summary>
    TextureData()
    {
    }

    /// <summary>
    /// Destructor
    /// </summary>
    ~TextureData()
    {
    }

public:
    /// <summary>
    /// Top level texture width (in pixels).
    /// </summary>
    API_FIELD() int32 Width = 0;

    /// <summary>
    /// Top level texture height (in pixels).
    /// </summary>
    API_FIELD() int32 Height = 0;

    /// <summary>
    /// Top level texture depth (in pixels).
    /// </summary>
    API_FIELD() int32 Depth = 0;

    /// <summary>
    /// The texture data format.
    /// </summary>
    API_FIELD() PixelFormat Format = PixelFormat::Unknown;

    /// <summary>
    /// The items collection (depth slices or array slices).
    /// </summary>
    API_FIELD() Array<TextureDataArrayEntry, InlinedAllocation<6>> Items;

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
    /// Gets amount of textures in the array
    /// </summary>
    int32 GetArraySize() const
    {
        return Items.Count();
    }

    /// <summary>
    /// Gets amount of mip maps in the textures
    /// </summary>
    int32 GetMipLevels() const
    {
        return Items.HasItems() ? Items[0].Mips.Count() : 0;
    }

    /// <summary>
    /// Clear data
    /// </summary>
    void Clear()
    {
        Items.Resize(0, false);
    }
};

template<>
struct TIsPODType<TextureData>
{
    enum { Value = false };
};
