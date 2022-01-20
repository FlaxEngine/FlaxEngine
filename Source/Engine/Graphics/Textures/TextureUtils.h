// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Types.h"

/// <summary>
/// Texture utilities class
/// </summary>
class TextureUtils
{
public:

    static PixelFormat ToPixelFormat(const TextureFormatType format, int32 width, int32 height, bool canCompress)
    {
        const bool canUseBlockCompression = width % 4 == 0 && height % 4 == 0;
        if (canCompress && canUseBlockCompression)
        {
            switch (format)
            {
            case TextureFormatType::ColorRGB:
                return PixelFormat::BC1_UNorm;
            case TextureFormatType::ColorRGBA:
                return PixelFormat::BC3_UNorm;
            case TextureFormatType::NormalMap:
                return PixelFormat::BC5_UNorm;
            case TextureFormatType::GrayScale:
                return PixelFormat::BC4_UNorm;
            case TextureFormatType::HdrRGBA:
                return PixelFormat::BC7_UNorm;
            case TextureFormatType::HdrRGB:
#if PLATFORM_LINUX
                // TODO: support BC6H compression for Linux Editor
                return PixelFormat::BC7_UNorm;
#else
                return PixelFormat::BC6H_Uf16;
#endif
            default:
                return PixelFormat::Unknown;
            }
        }

        switch (format)
        {
        case TextureFormatType::ColorRGB:
            return PixelFormat::R8G8B8A8_UNorm;
        case TextureFormatType::ColorRGBA:
            return PixelFormat::R8G8B8A8_UNorm;
        case TextureFormatType::NormalMap:
            return PixelFormat::R16G16_UNorm;
        case TextureFormatType::GrayScale:
            return PixelFormat::R8_UNorm;
        case TextureFormatType::HdrRGBA:
            return PixelFormat::R16G16B16A16_Float;
        case TextureFormatType::HdrRGB:
            return PixelFormat::R11G11B10_Float;
        default:
            return PixelFormat::Unknown;
        }
    }
};
