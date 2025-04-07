// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Graphics/PixelFormat.h"

/// <summary>
/// Describes texture compression format type
/// </summary>
API_ENUM() enum class TextureFormatType : byte
{
    // Invalid value.
    API_ENUM(Attributes="HideInEditor")
    Unknown = 0,
    // The color with RGB channels.
    ColorRGB,
    // The color with RGBA channels.
    ColorRGBA,
    // Normal map data (packed and compressed).
    NormalMap,
    // The gray scale (R channel).
    GrayScale,
    // The HDR color (RGBA channels).
    HdrRGBA,
    // The HDR color (RGB channels).
    HdrRGB,
};

/// <summary>
/// Old texture header structure (was not fully initialized to zero).
/// </summary>
struct TextureHeader_Deprecated
{
    int32 Width;
    int32 Height;
    int32 MipLevels;
    PixelFormat Format;
    TextureFormatType Type;
    bool IsCubeMap;
    bool NeverStream;
    bool IsSRGB;
    byte CustomData[17];

    TextureHeader_Deprecated();
};

/// <summary>
/// Texture header structure.
/// </summary>
struct FLAXENGINE_API TextureHeader
{
    /// <summary>
    /// Width in pixels
    /// </summary>
    int32 Width;

    /// <summary>
    /// Height in pixels
    /// </summary>
    int32 Height;

    /// <summary>
    /// Depth in pixels
    /// </summary>
    int32 Depth;

    /// <summary>
    /// Amount of mip levels
    /// </summary>
    int32 MipLevels;

    /// <summary>
    /// Texture group for streaming (negative if unused).
    /// </summary>
    int32 TextureGroup;

    /// <summary>
    /// Texture pixels format
    /// </summary>
    PixelFormat Format;

    /// <summary>
    /// Texture compression type
    /// </summary>
    TextureFormatType Type;

    /// <summary>
    /// True if texture is a cubemap (has 6 array slices per mip).
    /// </summary>
    byte IsCubeMap : 1;

    /// <summary>
    /// True if texture contains sRGB colors data
    /// </summary>
    byte IsSRGB : 1;

    /// <summary>
    /// True if disable dynamic texture streaming
    /// </summary>
    byte NeverStream : 1;

    /// <summary>
    /// The custom data to be used per texture storage layer (faster access).
    /// </summary>
    byte CustomData[10];

    TextureHeader();
    TextureHeader(const TextureHeader_Deprecated& old);
};
