// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Enums.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Graphics/PixelFormat.h"

/// <summary>
/// Describes texture compression format type
/// </summary>
DECLARE_ENUM_EX_7(TextureFormatType, byte, 0, Unknown, ColorRGB, ColorRGBA, NormalMap, GrayScale, HdrRGBA, HdrRGB);

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
    TextureHeader(TextureHeader_Deprecated& old);
};
