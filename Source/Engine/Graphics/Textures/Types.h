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
/// Texture header structure
/// </summary>
struct FLAXENGINE_API TextureHeader
{
    /// <summary>
    /// Top mip width in pixels
    /// </summary>
    int32 Width;

    /// <summary>
    /// Top mip height in pixels
    /// </summary>
    int32 Height;

    /// <summary>
    /// Amount of mip levels
    /// </summary>
    int32 MipLevels;

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
    bool IsCubeMap;

    /// <summary>
    /// True if disable dynamic texture streaming
    /// </summary>
    bool NeverStream;

    /// <summary>
    /// True if texture contains sRGB colors data
    /// </summary>
    bool IsSRGB;

    /// <summary>
    /// The custom data to be used per texture storage layer (faster access).
    /// </summary>
    byte CustomData[17];
};

static_assert(sizeof(TextureHeader) == 10 * sizeof(int32), "Invalid TextureHeader size.");
