// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Config.h"

/// <summary>
/// Enumeration with all pixel formats used by the graphics pipeline.
/// </summary>
/// <remarks>
/// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb173059(v=vs.85).aspx
/// </remarks>
API_ENUM() enum class PixelFormat : uint32
{
    /// <summary>
    /// The format is not known.
    /// </summary>
    Unknown = 0,

    /// <summary>
    /// A four-component, 128-bit typeless format that supports 32 bits per channel including alpha.
    /// </summary>
    R32G32B32A32_Typeless = 1,

    /// <summary>
    /// A four-component, 128-bit floating-point format that supports 32 bits per channel including alpha.
    /// </summary>
    R32G32B32A32_Float = 2,

    /// <summary>
    /// A four-component, 128-bit unsigned-integer format that supports 32 bits per channel including alpha.
    /// </summary>
    R32G32B32A32_UInt = 3,

    /// <summary>
    /// A four-component, 128-bit signed-integer format that supports 32 bits per channel including alpha.
    /// </summary>
    R32G32B32A32_SInt = 4,

    /// <summary>
    /// A three-component, 96-bit typeless format that supports 32 bits per color channel.
    /// </summary>
    R32G32B32_Typeless = 5,

    /// <summary>
    /// A three-component, 96-bit floating-point format that supports 32 bits per color channel.
    /// </summary>
    R32G32B32_Float = 6,

    /// <summary>
    /// A three-component, 96-bit unsigned-integer format that supports 32 bits per color channel.
    /// </summary>
    R32G32B32_UInt = 7,

    /// <summary>
    /// A three-component, 96-bit signed-integer format that supports 32 bits per color channel.
    /// </summary>
    R32G32B32_SInt = 8,

    /// <summary>
    /// A four-component, 64-bit typeless format that supports 16 bits per channel including alpha.
    /// </summary>
    R16G16B16A16_Typeless = 9,

    /// <summary>
    /// A four-component, 64-bit floating-point format that supports 16 bits per channel including alpha.
    /// </summary>
    R16G16B16A16_Float = 10,

    /// <summary>
    /// A four-component, 64-bit unsigned-normalized-integer format that supports 16 bits per channel including alpha.
    /// </summary>
    R16G16B16A16_UNorm = 11,

    /// <summary>
    /// A four-component, 64-bit unsigned-integer format that supports 16 bits per channel including alpha.
    /// </summary>
    R16G16B16A16_UInt = 12,

    /// <summary>
    /// A four-component, 64-bit signed-normalized-integer format that supports 16 bits per channel including alpha.
    /// </summary>
    R16G16B16A16_SNorm = 13,

    /// <summary>
    /// A four-component, 64-bit signed-integer format that supports 16 bits per channel including alpha.
    /// </summary>
    R16G16B16A16_SInt = 14,

    /// <summary>
    /// A two-component, 64-bit typeless format that supports 32 bits for the red channel and 32 bits for the green channel.
    /// </summary>
    R32G32_Typeless = 15,

    /// <summary>
    /// A two-component, 64-bit floating-point format that supports 32 bits for the red channel and 32 bits for the green channel.
    /// </summary>
    R32G32_Float = 16,

    /// <summary>
    /// A two-component, 64-bit unsigned-integer format that supports 32 bits for the red channel and 32 bits for the green channel.
    /// </summary>
    R32G32_UInt = 17,

    /// <summary>
    /// A two-component, 64-bit signed-integer format that supports 32 bits for the red channel and 32 bits for the green channel.
    /// </summary>
    R32G32_SInt = 18,

    /// <summary>
    /// A two-component, 64-bit typeless format that supports 32 bits for the red channel, 8 bits for the green channel, and 24 bits are unused.
    /// </summary>
    R32G8X24_Typeless = 19,

    /// <summary>
    /// A 32-bit floating-point component, and two unsigned-integer components (with an additional 32 bits). This format supports 32-bit depth, 8-bit stencil, and 24 bits are unused.
    /// </summary>
    D32_Float_S8X24_UInt = 20,

    /// <summary>
    /// A 32-bit floating-point component, and two typeless components (with an additional 32 bits). This format supports 32-bit red channel, 8 bits are unused, and 24 bits are unused.
    /// </summary>
    R32_Float_X8X24_Typeless = 21,

    /// <summary>
    /// A 32-bit typeless component, and two unsigned-integer components (with an additional 32 bits). This format has 32 bits unused, 8 bits for green channel, and 24 bits are unused.
    /// </summary>
    X32_Typeless_G8X24_UInt = 22,

    /// <summary>
    /// A four-component, 32-bit typeless format that supports 10 bits for each color and 2 bits for alpha.
    /// </summary>
    R10G10B10A2_Typeless = 23,

    /// <summary>
    /// A four-component, 32-bit unsigned-normalized-integer format that supports 10 bits for each color and 2 bits for alpha.
    /// </summary>
    R10G10B10A2_UNorm = 24,

    /// <summary>
    /// A four-component, 32-bit unsigned-integer format that supports 10 bits for each color and 2 bits for alpha.
    /// </summary>
    R10G10B10A2_UInt = 25,

    /// <summary>
    /// Three partial-precision floating-point numbers encoded into a single 32-bit value (a variant of s10e5, which is sign bit, 10-bit mantissa, and 5-bit biased (15) exponent). There are no sign bits, and there is a 5-bit biased (15) exponent for each channel, 6-bit mantissa for R and G, and a 5-bit mantissa for B.
    /// </summary>
    R11G11B10_Float = 26,

    /// <summary>
    /// A four-component, 32-bit typeless format that supports 8 bits per channel including alpha.
    /// </summary>
    R8G8B8A8_Typeless = 27,

    /// <summary>
    /// A four-component, 32-bit unsigned-normalized-integer format that supports 8 bits per channel including alpha.
    /// </summary>
    R8G8B8A8_UNorm = 28,

    /// <summary>
    /// A four-component, 32-bit unsigned-normalized integer sRGB format that supports 8 bits per channel including alpha.
    /// </summary>
    R8G8B8A8_UNorm_sRGB = 29,

    /// <summary>
    /// A four-component, 32-bit unsigned-integer format that supports 8 bits per channel including alpha.
    /// </summary>
    R8G8B8A8_UInt = 30,

    /// <summary>
    /// A four-component, 32-bit signed-normalized-integer format that supports 8 bits per channel including alpha.
    /// </summary>
    R8G8B8A8_SNorm = 31,

    /// <summary>
    /// A four-component, 32-bit signed-integer format that supports 8 bits per channel including alpha.
    /// </summary>
    R8G8B8A8_SInt = 32,

    /// <summary>
    /// A two-component, 32-bit typeless format that supports 16 bits for the red channel and 16 bits for the green channel.
    /// </summary>
    R16G16_Typeless = 33,

    /// <summary>
    /// A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel.
    /// </summary>
    R16G16_Float = 34,

    /// <summary>
    /// A two-component, 32-bit unsigned-normalized-integer format that supports 16 bits each for the green and red channels.
    /// </summary>
    R16G16_UNorm = 35,

    /// <summary>
    /// A two-component, 32-bit unsigned-integer format that supports 16 bits for the red channel and 16 bits for the green channel.
    /// </summary>
    R16G16_UInt = 36,

    /// <summary>
    /// A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel.
    /// </summary>
    R16G16_SNorm = 37,

    /// <summary>
    /// A two-component, 32-bit signed-integer format that supports 16 bits for the red channel and 16 bits for the green channel.
    /// </summary>
    R16G16_SInt = 38,

    /// <summary>
    /// A single-component, 32-bit typeless format that supports 32 bits for the red channel.
    /// </summary>
    R32_Typeless = 39,

    /// <summary>
    /// A single-component, 32-bit floating-point format that supports 32 bits for depth.
    /// </summary>
    D32_Float = 40,

    /// <summary>
    /// A single-component, 32-bit floating-point format that supports 32 bits for the red channel.
    /// </summary>
    R32_Float = 41,

    /// <summary>
    /// A single-component, 32-bit unsigned-integer format that supports 32 bits for the red channel.
    /// </summary>
    R32_UInt = 42,

    /// <summary>
    /// A single-component, 32-bit signed-integer format that supports 32 bits for the red channel.
    /// </summary>
    R32_SInt = 43,

    /// <summary>
    /// A two-component, 32-bit typeless format that supports 24 bits for the red channel and 8 bits for the green channel.
    /// </summary>
    R24G8_Typeless = 44,

    /// <summary>
    /// A 32-bit z-buffer format that supports 24 bits for depth and 8 bits for stencil.
    /// </summary>
    D24_UNorm_S8_UInt = 45,

    /// <summary>
    /// A 32-bit format, that contains a 24 bit, single-component, unsigned-normalized integer, with an additional typeless 8 bits. This format has 24 bits red channel and 8 bits unused.
    /// </summary>
    R24_UNorm_X8_Typeless = 46,

    /// <summary>
    /// A 32-bit format, that contains a 24 bit, single-component, typeless format, with an additional 8 bit unsigned integer component. This format has 24 bits unused and 8 bits green channel.
    /// </summary>
    X24_Typeless_G8_UInt = 47,

    /// <summary>
    /// A two-component, 16-bit typeless format that supports 8 bits for the red channel and 8 bits for the green channel.
    /// </summary>
    R8G8_Typeless = 48,

    /// <summary>
    /// A two-component, 16-bit unsigned-normalized-integer format that supports 8 bits for the red channel and 8 bits for the green channel.
    /// </summary>
    R8G8_UNorm = 49,

    /// <summary>
    /// A two-component, 16-bit unsigned-integer format that supports 8 bits for the red channel and 8 bits for the green channel.
    /// </summary>
    R8G8_UInt = 50,

    /// <summary>
    /// A two-component, 16-bit signed-normalized-integer format that supports 8 bits for the red channel and 8 bits for the green channel.
    /// </summary>
    R8G8_SNorm = 51,

    /// <summary>
    /// A two-component, 16-bit signed-integer format that supports 8 bits for the red channel and 8 bits for the green channel.
    /// </summary>
    R8G8_SInt = 52,

    /// <summary>
    /// A single-component, 16-bit typeless format that supports 16 bits for the red channel.
    /// </summary>
    R16_Typeless = 53,

    /// <summary>
    /// A single-component, 16-bit floating-point format that supports 16 bits for the red channel.
    /// </summary>
    R16_Float = 54,

    /// <summary>
    /// A single-component, 16-bit unsigned-normalized-integer format that supports 16 bits for depth.
    /// </summary>
    D16_UNorm = 55,

    /// <summary>
    /// A single-component, 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel.
    /// </summary>
    R16_UNorm = 56,

    /// <summary>
    /// A single-component, 16-bit unsigned-integer format that supports 16 bits for the red channel.
    /// </summary>
    R16_UInt = 57,

    /// <summary>
    /// A single-component, 16-bit signed-normalized-integer format that supports 16 bits for the red channel.
    /// </summary>
    R16_SNorm = 58,

    /// <summary>
    /// A single-component, 16-bit signed-integer format that supports 16 bits for the red channel.
    /// </summary>
    R16_SInt = 59,

    /// <summary>
    /// A single-component, 8-bit typeless format that supports 8 bits for the red channel.
    /// </summary>
    R8_Typeless = 60,

    /// <summary>
    /// A single-component, 8-bit unsigned-normalized-integer format that supports 8 bits for the red channel.
    /// </summary>
    R8_UNorm = 61,

    /// <summary>
    /// A single-component, 8-bit unsigned-integer format that supports 8 bits for the red channel.
    /// </summary>
    R8_UInt = 62,

    /// <summary>
    /// A single-component, 8-bit signed-normalized-integer format that supports 8 bits for the red channel.
    /// </summary>
    R8_SNorm = 63,

    /// <summary>
    /// A single-component, 8-bit signed-integer format that supports 8 bits for the red channel.
    /// </summary>
    R8_SInt = 64,

    /// <summary>
    /// A single-component, 8-bit unsigned-normalized-integer format for alpha only.
    /// </summary>
    A8_UNorm = 65,

    /// <summary>
    /// A single-component, 1-bit unsigned-normalized integer format that supports 1 bit for the red channel.
    /// </summary>
    R1_UNorm = 66,

    /// <summary>
    /// Three partial-precision floating-point numbers encoded into a single 32-bit value all sharing the same 5-bit exponent (variant of s10e5, which is sign bit, 10-bit mantissa, and 5-bit biased (15) exponent). There is no sign bit, and there is a shared 5-bit biased (15) exponent and a 9-bit mantissa for each channel.
    /// </summary>
    R9G9B9E5_SharedExp = 67,

    /// <summary>
    /// A four-component, 32-bit unsigned-normalized-integer format. This packed RGB format is analogous to the UYVY format. Each 32-bit block describes a pair of pixels: (R8, G8, B8) and (R8, G8, B8) where the R8/B8 values are repeated, and the G8 values are unique to each pixel. Width must be even.
    /// </summary>
    R8G8_B8G8_UNorm = 68,

    /// <summary>
    /// A four-component, 32-bit unsigned-normalized-integer format. This packed RGB format is analogous to the YUY2 format. Each 32-bit block describes a pair of pixels: (R8, G8, B8) and (R8, G8, B8) where the R8/B8 values are repeated, and the G8 values are unique to each pixel. Width must be even.
    /// </summary>
    G8R8_G8B8_UNorm = 69,

    /// <summary>
    /// Four-component typeless block-compression format
    /// </summary>
    BC1_Typeless = 70,

    /// <summary>
    /// Four-component block-compression format.
    /// </summary>
    BC1_UNorm = 71,

    /// <summary>
    /// Four-component block-compression format for sRGB data.
    /// </summary>
    BC1_UNorm_sRGB = 72,

    /// <summary>
    /// Four-component typeless block-compression format.
    /// </summary>
    BC2_Typeless = 73,

    /// <summary>
    /// Four-component block-compression format.
    /// </summary>
    BC2_UNorm = 74,

    /// <summary>
    /// Four-component block-compression format for sRGB data.
    /// </summary>
    BC2_UNorm_sRGB = 75,

    /// <summary>
    /// Four-component typeless block-compression format.
    /// </summary>
    BC3_Typeless = 76,

    /// <summary>
    /// Four-component block-compression format.
    /// </summary>
    BC3_UNorm = 77,

    /// <summary>
    /// Four-component block-compression format for sRGB data.
    /// </summary>
    BC3_UNorm_sRGB = 78,

    /// <summary>
    /// One-component typeless block-compression format.
    /// </summary>
    BC4_Typeless = 79,

    /// <summary>
    /// One-component block-compression format.
    /// </summary>
    BC4_UNorm = 80,

    /// <summary>
    /// One-component block-compression format.
    /// </summary>
    BC4_SNorm = 81,

    /// <summary>
    /// Two-component typeless block-compression format.
    /// </summary>
    BC5_Typeless = 82,

    /// <summary>
    /// Two-component block-compression format.
    /// </summary>
    BC5_UNorm = 83,

    /// <summary>
    /// Two-component block-compression format.
    /// </summary>
    BC5_SNorm = 84,

    /// <summary>
    /// A three-component, 16-bit unsigned-normalized-integer format that supports 5 bits for blue, 6 bits for green, and 5 bits for red.
    /// </summary>
    B5G6R5_UNorm = 85,

    /// <summary>
    /// A four-component, 16-bit unsigned-normalized-integer format that supports 5 bits for each color channel and 1-bit alpha.
    /// </summary>
    B5G5R5A1_UNorm = 86,

    /// <summary>
    /// A four-component, 32-bit unsigned-normalized-integer format that supports 8 bits for each color channel and 8-bit alpha.
    /// </summary>
    B8G8R8A8_UNorm = 87,

    /// <summary>
    /// A four-component, 32-bit unsigned-normalized-integer format that supports 8 bits for each color channel and 8 bits unused.
    /// </summary>
    B8G8R8X8_UNorm = 88,

    /// <summary>
    /// A four-component, 32-bit 2.8-biased fixed-point format that supports 10 bits for each color channel and 2-bit alpha.
    /// </summary>
    R10G10B10_Xr_Bias_A2_UNorm = 89,

    /// <summary>
    /// A four-component, 32-bit typeless format that supports 8 bits for each channel including alpha.
    /// </summary>
    B8G8R8A8_Typeless = 90,

    /// <summary>
    /// A four-component, 32-bit unsigned-normalized standard RGB format that supports 8 bits for each channel including alpha.
    /// </summary>
    B8G8R8A8_UNorm_sRGB = 91,

    /// <summary>
    /// A four-component, 32-bit typeless format that supports 8 bits for each color channel, and 8 bits are unused.
    /// </summary>
    B8G8R8X8_Typeless = 92,

    /// <summary>
    /// A four-component, 32-bit unsigned-normalized standard RGB format that supports 8 bits for each color channel, and 8 bits are unused.
    /// </summary>
    B8G8R8X8_UNorm_sRGB = 93,

    /// <summary>
    /// A three-component, HDR floating point format for color channels format that supports RGB channels. There is no support for an alpha channel.
    /// </summary>
    BC6H_Typeless = 94,

    /// <summary>
    /// A three-component, HDR unsigned floating point format for color channels format that supports RGB channels. There is no support for an alpha channel.
    /// </summary>
    BC6H_Uf16 = 95,

    /// <summary>
    /// A three-component, HDR signed floating point format for color channels format that supports RGB channels. There is no support for an alpha channel.
    /// </summary>
    BC6H_Sf16 = 96,

    /// <summary>
    /// A four-component, HDR floating point format for color channels format that supports RGBA channels.
    /// </summary>
    BC7_Typeless = 97,

    /// <summary>
    /// A four-component, HDR floating point format for color channels format that supports RGBA channels.
    /// </summary>
    BC7_UNorm = 98,

    /// <summary>
    /// A four-component, HDR floating point format for color channels format that supports RGBA channels.
    /// </summary>
    BC7_UNorm_sRGB = 99,

    /// <summary>
    /// A four-component ASTC (4x4 pixel block in 128 bits) block-compression format that supports RGBA channels.
    /// </summary>
    ASTC_4x4_UNorm = 100,

    /// <summary>
    /// A four-component ASTC (4x4 pixel block in 128 bits) block-compression format that supports RGBA channels. Data in sRGB color space.
    /// </summary>
    ASTC_4x4_UNorm_sRGB = 101,

    /// <summary>
    /// A four-component ASTC (6x6 pixel block in 128 bits) block-compression format that supports RGBA channels.
    /// </summary>
    ASTC_6x6_UNorm = 102,

    /// <summary>
    /// A four-component ASTC (6x6 pixel block in 128 bits) block-compression format that supports RGBA channels. Data in sRGB color space.
    /// </summary>
    ASTC_6x6_UNorm_sRGB = 103,

    /// <summary>
    /// A four-component ASTC (8x8 pixel block in 128 bits) block-compression format that supports RGBA channels.
    /// </summary>
    ASTC_8x8_UNorm = 104,

    /// <summary>
    /// A four-component ASTC (8x8 pixel block in 128 bits) block-compression format that supports RGBA channels. Data in sRGB color space.
    /// </summary>
    ASTC_8x8_UNorm_sRGB = 105,

    /// <summary>
    /// A four-component ASTC (10x10 pixel block in 128 bits) block-compression format that supports RGBA channels.
    /// </summary>
    ASTC_10x10_UNorm = 106,

    /// <summary>
    /// A four-component ASTC (10x10 pixel block in 128 bits) block-compression format that supports RGBA channels. Data in sRGB color space.
    /// </summary>
    ASTC_10x10_UNorm_sRGB = 107,

    /// <summary>
    /// Packed YUV 4:2:2 video texture format. The mapping to the view channel in shader is Y0->R8, U0->G8, Y1->B8, and V0->A8.
    /// </summary>
    YUY2 = 108,

    /// <summary>
    /// Packed YUV 4:2:0 video texture format. Texture uses separate views to access the luma (Y) plane separately from the chroma (UV) planes. For luminance data view, the mapping to the view channel is Y->R8. For chrominance data view, the mapping to the view channel is U->R8 and V->G8.
    /// </summary>
    NV12 = 109,

    /// <summary>
    /// The maximum format value (for internal use only).
    /// </summary>
    MAX
};
