// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "PixelFormatExtensions.h"
#include "Engine/Core/Math/Math.h"

// ReSharper disable CppClangTidyClangDiagnosticSwitchEnum

namespace
{
    int32 sizeOfInBits[(int32)PixelFormat::MAX];
}

#define InitFormat(formats, bitCount) for(int i = 0; i < ARRAY_COUNT(formats); i++) { sizeOfInBits[(int32)formats[i]] = bitCount; }

void PixelFormatExtensions::Init()
{
    // Clear lookup table
    Platform::MemoryClear(sizeOfInBits, sizeof sizeOfInBits);

    PixelFormat formats1[] = {
        PixelFormat::R1_UNorm
    };
    InitFormat(formats1, 1);

    PixelFormat formats2[] = {
        PixelFormat::A8_UNorm,
        PixelFormat::R8_SInt,
        PixelFormat::R8_SNorm,
        PixelFormat::R8_Typeless,
        PixelFormat::R8_UInt,
        PixelFormat::R8_UNorm,
        PixelFormat::BC2_Typeless,
        PixelFormat::BC2_UNorm,
        PixelFormat::BC2_UNorm_sRGB,
        PixelFormat::BC3_Typeless,
        PixelFormat::BC3_UNorm,
        PixelFormat::BC3_UNorm_sRGB,
        PixelFormat::BC5_SNorm,
        PixelFormat::BC5_Typeless,
        PixelFormat::BC5_UNorm,
        PixelFormat::BC6H_Sf16,
        PixelFormat::BC6H_Typeless,
        PixelFormat::BC6H_Uf16,
        PixelFormat::BC7_Typeless,
        PixelFormat::BC7_UNorm,
        PixelFormat::BC7_UNorm_sRGB,
        PixelFormat::ASTC_4x4_UNorm,
        PixelFormat::ASTC_4x4_UNorm_sRGB,
        PixelFormat::YUY2,
    };
    InitFormat(formats2, 8);

    PixelFormat formats3[] = {
        PixelFormat::B5G5R5A1_UNorm,
        PixelFormat::B5G6R5_UNorm,
        PixelFormat::D16_UNorm,
        PixelFormat::R16_Float,
        PixelFormat::R16_SInt,
        PixelFormat::R16_SNorm,
        PixelFormat::R16_Typeless,
        PixelFormat::R16_UInt,
        PixelFormat::R16_UNorm,
        PixelFormat::R8G8_SInt,
        PixelFormat::R8G8_SNorm,
        PixelFormat::R8G8_Typeless,
        PixelFormat::R8G8_UInt,
        PixelFormat::R8G8_UNorm,
    };
    InitFormat(formats3, 16);

    PixelFormat formats4[] = {
        PixelFormat::B8G8R8X8_Typeless,
        PixelFormat::B8G8R8X8_UNorm,
        PixelFormat::B8G8R8X8_UNorm_sRGB,
        PixelFormat::D24_UNorm_S8_UInt,
        PixelFormat::D32_Float,
        PixelFormat::D32_Float_S8X24_UInt,
        PixelFormat::G8R8_G8B8_UNorm,
        PixelFormat::R10G10B10_Xr_Bias_A2_UNorm,
        PixelFormat::R10G10B10A2_Typeless,
        PixelFormat::R10G10B10A2_UInt,
        PixelFormat::R10G10B10A2_UNorm,
        PixelFormat::R11G11B10_Float,
        PixelFormat::R16G16_Float,
        PixelFormat::R16G16_SInt,
        PixelFormat::R16G16_SNorm,
        PixelFormat::R16G16_Typeless,
        PixelFormat::R16G16_UInt,
        PixelFormat::R16G16_UNorm,
        PixelFormat::R24_UNorm_X8_Typeless,
        PixelFormat::R24G8_Typeless,
        PixelFormat::R32_Float,
        PixelFormat::R32_Float_X8X24_Typeless,
        PixelFormat::R32_SInt,
        PixelFormat::R32_Typeless,
        PixelFormat::R32_UInt,
        PixelFormat::R8G8_B8G8_UNorm,
        PixelFormat::R8G8B8A8_SInt,
        PixelFormat::R8G8B8A8_SNorm,
        PixelFormat::R8G8B8A8_Typeless,
        PixelFormat::R8G8B8A8_UInt,
        PixelFormat::R8G8B8A8_UNorm,
        PixelFormat::R8G8B8A8_UNorm_sRGB,
        PixelFormat::B8G8R8A8_Typeless,
        PixelFormat::B8G8R8A8_UNorm,
        PixelFormat::B8G8R8A8_UNorm_sRGB,
        PixelFormat::R9G9B9E5_SharedExp,
        PixelFormat::X24_Typeless_G8_UInt,
        PixelFormat::X32_Typeless_G8X24_UInt,
    };
    InitFormat(formats4, 32);

    PixelFormat formats5[] = {
        PixelFormat::R16G16B16A16_Float,
        PixelFormat::R16G16B16A16_SInt,
        PixelFormat::R16G16B16A16_SNorm,
        PixelFormat::R16G16B16A16_Typeless,
        PixelFormat::R16G16B16A16_UInt,
        PixelFormat::R16G16B16A16_UNorm,
        PixelFormat::R32G32_Float,
        PixelFormat::R32G32_SInt,
        PixelFormat::R32G32_Typeless,
        PixelFormat::R32G32_UInt,
        PixelFormat::R32G8X24_Typeless,
    };
    InitFormat(formats5, 64);

    PixelFormat formats6[] = {
        PixelFormat::R32G32B32_Float,
        PixelFormat::R32G32B32_SInt,
        PixelFormat::R32G32B32_Typeless,
        PixelFormat::R32G32B32_UInt,
    };
    InitFormat(formats6, 96);

    PixelFormat formats7[] = {
        PixelFormat::R32G32B32A32_Float,
        PixelFormat::R32G32B32A32_SInt,
        PixelFormat::R32G32B32A32_Typeless,
        PixelFormat::R32G32B32A32_UInt,
    };
    InitFormat(formats7, 128);

    PixelFormat formats8[] = {
        PixelFormat::BC1_Typeless,
        PixelFormat::BC1_UNorm,
        PixelFormat::BC1_UNorm_sRGB,
        PixelFormat::BC4_SNorm,
        PixelFormat::BC4_Typeless,
        PixelFormat::BC4_UNorm,
    };
    InitFormat(formats8, 4);

    sizeOfInBits[(int32)PixelFormat::NV12] = 12;
}

int32 PixelFormatExtensions::SizeInBits(PixelFormat format)
{
    return sizeOfInBits[(int32)format];
}

int32 PixelFormatExtensions::AlphaSizeInBits(const PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::R32G32B32A32_Typeless:
    case PixelFormat::R32G32B32A32_Float:
    case PixelFormat::R32G32B32A32_UInt:
    case PixelFormat::R32G32B32A32_SInt:
        return 32;
    case PixelFormat::R16G16B16A16_Typeless:
    case PixelFormat::R16G16B16A16_Float:
    case PixelFormat::R16G16B16A16_UNorm:
    case PixelFormat::R16G16B16A16_UInt:
    case PixelFormat::R16G16B16A16_SNorm:
    case PixelFormat::R16G16B16A16_SInt:
        return 16;
    case PixelFormat::R10G10B10A2_Typeless:
    case PixelFormat::R10G10B10A2_UNorm:
    case PixelFormat::R10G10B10A2_UInt:
    case PixelFormat::R10G10B10_Xr_Bias_A2_UNorm:
        return 2;
    case PixelFormat::R8G8B8A8_Typeless:
    case PixelFormat::R8G8B8A8_UNorm:
    case PixelFormat::R8G8B8A8_UNorm_sRGB:
    case PixelFormat::R8G8B8A8_UInt:
    case PixelFormat::R8G8B8A8_SNorm:
    case PixelFormat::R8G8B8A8_SInt:
    case PixelFormat::B8G8R8A8_UNorm:
    case PixelFormat::B8G8R8A8_Typeless:
    case PixelFormat::B8G8R8A8_UNorm_sRGB:
    case PixelFormat::A8_UNorm:
        return 8;
    case PixelFormat::B5G5R5A1_UNorm:
        return 1;
    case PixelFormat::BC1_Typeless:
    case PixelFormat::BC1_UNorm:
    case PixelFormat::BC1_UNorm_sRGB:
        return 1; // or 0
    case PixelFormat::BC2_Typeless:
    case PixelFormat::BC2_UNorm:
    case PixelFormat::BC2_UNorm_sRGB:
        return 4;
    case PixelFormat::BC3_Typeless:
    case PixelFormat::BC3_UNorm:
    case PixelFormat::BC3_UNorm_sRGB:
        return 8;
    case PixelFormat::BC7_Typeless:
    case PixelFormat::BC7_UNorm:
    case PixelFormat::BC7_UNorm_sRGB:
        return 8; // or 0
    default:
        return 0;
    }
}

bool PixelFormatExtensions::IsDepthStencil(const PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::R32G8X24_Typeless:
    case PixelFormat::D32_Float_S8X24_UInt:
    case PixelFormat::R32_Float_X8X24_Typeless:
    case PixelFormat::X32_Typeless_G8X24_UInt:
    case PixelFormat::D32_Float:
    case PixelFormat::R24G8_Typeless:
    case PixelFormat::D24_UNorm_S8_UInt:
    case PixelFormat::R24_UNorm_X8_Typeless:
    case PixelFormat::X24_Typeless_G8_UInt:
    case PixelFormat::D16_UNorm:
        return true;
    default:
        return false;
    }
}

bool PixelFormatExtensions::HasStencil(const PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::D24_UNorm_S8_UInt:
    case PixelFormat::D32_Float_S8X24_UInt:
        return true;
    default:
        return false;
    }
}

bool PixelFormatExtensions::IsTypeless(const PixelFormat format, const bool partialTypeless)
{
    switch (format)
    {
    case PixelFormat::R32G32B32A32_Typeless:
    case PixelFormat::R32G32B32_Typeless:
    case PixelFormat::R16G16B16A16_Typeless:
    case PixelFormat::R32G32_Typeless:
    case PixelFormat::R32G8X24_Typeless:
    case PixelFormat::R10G10B10A2_Typeless:
    case PixelFormat::R8G8B8A8_Typeless:
    case PixelFormat::R16G16_Typeless:
    case PixelFormat::R32_Typeless:
    case PixelFormat::R24G8_Typeless:
    case PixelFormat::R8G8_Typeless:
    case PixelFormat::R16_Typeless:
    case PixelFormat::R8_Typeless:
    case PixelFormat::BC1_Typeless:
    case PixelFormat::BC2_Typeless:
    case PixelFormat::BC3_Typeless:
    case PixelFormat::BC4_Typeless:
    case PixelFormat::BC5_Typeless:
    case PixelFormat::B8G8R8A8_Typeless:
    case PixelFormat::B8G8R8X8_Typeless:
    case PixelFormat::BC6H_Typeless:
    case PixelFormat::BC7_Typeless:
        return true;
    case PixelFormat::R32_Float_X8X24_Typeless:
    case PixelFormat::X32_Typeless_G8X24_UInt:
    case PixelFormat::R24_UNorm_X8_Typeless:
    case PixelFormat::X24_Typeless_G8_UInt:
        return partialTypeless;
    default:
        return false;
    }
}

bool PixelFormatExtensions::IsValid(const PixelFormat format)
{
    return format > PixelFormat::Unknown && format < PixelFormat::MAX;
}

bool PixelFormatExtensions::IsCompressed(const PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::BC1_Typeless:
    case PixelFormat::BC1_UNorm:
    case PixelFormat::BC1_UNorm_sRGB:
    case PixelFormat::BC2_Typeless:
    case PixelFormat::BC2_UNorm:
    case PixelFormat::BC2_UNorm_sRGB:
    case PixelFormat::BC3_Typeless:
    case PixelFormat::BC3_UNorm:
    case PixelFormat::BC3_UNorm_sRGB:
    case PixelFormat::BC4_Typeless:
    case PixelFormat::BC4_UNorm:
    case PixelFormat::BC4_SNorm:
    case PixelFormat::BC5_Typeless:
    case PixelFormat::BC5_UNorm:
    case PixelFormat::BC5_SNorm:
    case PixelFormat::BC6H_Typeless:
    case PixelFormat::BC6H_Uf16:
    case PixelFormat::BC6H_Sf16:
    case PixelFormat::BC7_Typeless:
    case PixelFormat::BC7_UNorm:
    case PixelFormat::BC7_UNorm_sRGB:
    case PixelFormat::ASTC_4x4_UNorm:
    case PixelFormat::ASTC_4x4_UNorm_sRGB:
    case PixelFormat::ASTC_6x6_UNorm:
    case PixelFormat::ASTC_6x6_UNorm_sRGB:
    case PixelFormat::ASTC_8x8_UNorm:
    case PixelFormat::ASTC_8x8_UNorm_sRGB:
    case PixelFormat::ASTC_10x10_UNorm:
    case PixelFormat::ASTC_10x10_UNorm_sRGB:
        return true;
    default:
        return false;
    }
}

bool PixelFormatExtensions::IsCompressedBC(PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::BC1_Typeless:
    case PixelFormat::BC1_UNorm:
    case PixelFormat::BC1_UNorm_sRGB:
    case PixelFormat::BC2_Typeless:
    case PixelFormat::BC2_UNorm:
    case PixelFormat::BC2_UNorm_sRGB:
    case PixelFormat::BC3_Typeless:
    case PixelFormat::BC3_UNorm:
    case PixelFormat::BC3_UNorm_sRGB:
    case PixelFormat::BC4_Typeless:
    case PixelFormat::BC4_UNorm:
    case PixelFormat::BC4_SNorm:
    case PixelFormat::BC5_Typeless:
    case PixelFormat::BC5_UNorm:
    case PixelFormat::BC5_SNorm:
    case PixelFormat::BC6H_Typeless:
    case PixelFormat::BC6H_Uf16:
    case PixelFormat::BC6H_Sf16:
    case PixelFormat::BC7_Typeless:
    case PixelFormat::BC7_UNorm:
    case PixelFormat::BC7_UNorm_sRGB:
        return true;
    default:
        return false;
    }
}

bool PixelFormatExtensions::IsCompressedASTC(PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::ASTC_4x4_UNorm:
    case PixelFormat::ASTC_4x4_UNorm_sRGB:
    case PixelFormat::ASTC_6x6_UNorm:
    case PixelFormat::ASTC_6x6_UNorm_sRGB:
    case PixelFormat::ASTC_8x8_UNorm:
    case PixelFormat::ASTC_8x8_UNorm_sRGB:
    case PixelFormat::ASTC_10x10_UNorm:
    case PixelFormat::ASTC_10x10_UNorm_sRGB:
        return true;
    default:
        return false;
    }
}

bool PixelFormatExtensions::IsVideo(const PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::YUY2:
    case PixelFormat::NV12:
        return true;
    default:
        return false;
    }
}

bool PixelFormatExtensions::IsSRGB(const PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::R8G8B8A8_UNorm_sRGB:
    case PixelFormat::BC1_UNorm_sRGB:
    case PixelFormat::BC2_UNorm_sRGB:
    case PixelFormat::BC3_UNorm_sRGB:
    case PixelFormat::B8G8R8A8_UNorm_sRGB:
    case PixelFormat::B8G8R8X8_UNorm_sRGB:
    case PixelFormat::BC7_UNorm_sRGB:
    case PixelFormat::ASTC_4x4_UNorm_sRGB:
    case PixelFormat::ASTC_6x6_UNorm_sRGB:
    case PixelFormat::ASTC_8x8_UNorm_sRGB:
    case PixelFormat::ASTC_10x10_UNorm_sRGB:
        return true;
    default:
        return false;
    }
}

bool PixelFormatExtensions::IsHDR(const PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::R11G11B10_Float:
    case PixelFormat::R10G10B10A2_UNorm:
    case PixelFormat::R16G16B16A16_Float:
    case PixelFormat::R32G32B32A32_Float:
    case PixelFormat::R16G16_Float:
    case PixelFormat::R16_Float:
    case PixelFormat::BC6H_Sf16:
    case PixelFormat::BC6H_Uf16:
        return true;
    default:
        return false;
    }
}

bool PixelFormatExtensions::IsRgbAOrder(const PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::R32G32B32A32_Typeless:
    case PixelFormat::R32G32B32A32_Float:
    case PixelFormat::R32G32B32A32_UInt:
    case PixelFormat::R32G32B32A32_SInt:
    case PixelFormat::R32G32B32_Typeless:
    case PixelFormat::R32G32B32_Float:
    case PixelFormat::R32G32B32_UInt:
    case PixelFormat::R32G32B32_SInt:
    case PixelFormat::R16G16B16A16_Typeless:
    case PixelFormat::R16G16B16A16_Float:
    case PixelFormat::R16G16B16A16_UNorm:
    case PixelFormat::R16G16B16A16_UInt:
    case PixelFormat::R16G16B16A16_SNorm:
    case PixelFormat::R16G16B16A16_SInt:
    case PixelFormat::R32G32_Typeless:
    case PixelFormat::R32G32_Float:
    case PixelFormat::R32G32_UInt:
    case PixelFormat::R32G32_SInt:
    case PixelFormat::R32G8X24_Typeless:
    case PixelFormat::R10G10B10A2_Typeless:
    case PixelFormat::R10G10B10A2_UNorm:
    case PixelFormat::R10G10B10A2_UInt:
    case PixelFormat::R11G11B10_Float:
    case PixelFormat::R8G8B8A8_Typeless:
    case PixelFormat::R8G8B8A8_UNorm:
    case PixelFormat::R8G8B8A8_UNorm_sRGB:
    case PixelFormat::R8G8B8A8_UInt:
    case PixelFormat::R8G8B8A8_SNorm:
    case PixelFormat::R8G8B8A8_SInt:
        return true;
    default:
        return false;
    }
}

bool PixelFormatExtensions::IsBGRAOrder(const PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::B8G8R8A8_UNorm:
    case PixelFormat::B8G8R8X8_UNorm:
    case PixelFormat::B8G8R8A8_Typeless:
    case PixelFormat::B8G8R8A8_UNorm_sRGB:
    case PixelFormat::B8G8R8X8_Typeless:
    case PixelFormat::B8G8R8X8_UNorm_sRGB:
        return true;
    default:
        return false;
    }
}

bool PixelFormatExtensions::IsNormalized(const PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::R16G16B16A16_UNorm:
    case PixelFormat::R16G16B16A16_SNorm:
    case PixelFormat::R10G10B10A2_UNorm:
    case PixelFormat::R8G8B8A8_UNorm:
    case PixelFormat::R8G8B8A8_UNorm_sRGB:
    case PixelFormat::R8G8B8A8_SNorm:
    case PixelFormat::R8G8_B8G8_UNorm:
    case PixelFormat::G8R8_G8B8_UNorm:
    case PixelFormat::B5G5R5A1_UNorm:
    case PixelFormat::B8G8R8A8_UNorm:
    case PixelFormat::B8G8R8X8_UNorm:
    case PixelFormat::R10G10B10_Xr_Bias_A2_UNorm:
    case PixelFormat::B8G8R8A8_UNorm_sRGB:
    case PixelFormat::B8G8R8X8_UNorm_sRGB:
    case PixelFormat::B5G6R5_UNorm:
    case PixelFormat::R16G16_UNorm:
    case PixelFormat::R16G16_SNorm:
    case PixelFormat::R8G8_UNorm:
    case PixelFormat::R8G8_SNorm:
    case PixelFormat::D16_UNorm:
    case PixelFormat::R16_UNorm:
    case PixelFormat::R16_SNorm:
    case PixelFormat::R8_UNorm:
    case PixelFormat::R8_SNorm:
    case PixelFormat::A8_UNorm:
    case PixelFormat::R1_UNorm:
        return true;
    default:
        return false;
    }
}

bool PixelFormatExtensions::IsInteger(const PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::R32G32B32A32_UInt:
    case PixelFormat::R32G32B32A32_SInt:
    case PixelFormat::R16G16B16A16_UInt:
    case PixelFormat::R16G16B16A16_SInt:
    case PixelFormat::R10G10B10A2_UInt:
    case PixelFormat::R8G8B8A8_UInt:
    case PixelFormat::R8G8B8A8_SInt:
    case PixelFormat::R32G32B32_UInt:
    case PixelFormat::R32G32B32_SInt:
    case PixelFormat::R32G32_UInt:
    case PixelFormat::R32G32_SInt:
    case PixelFormat::R16G16_UInt:
    case PixelFormat::R16G16_SInt:
    case PixelFormat::R8G8_UInt:
    case PixelFormat::R8G8_SInt:
    case PixelFormat::R32_UInt:
    case PixelFormat::R32_SInt:
    case PixelFormat::R16_UInt:
    case PixelFormat::R16_SInt:
    case PixelFormat::R8_UInt:
    case PixelFormat::R8_SInt:
        return true;
    default:
        return false;
    }
}

int32 PixelFormatExtensions::ComputeComponentsCount(const PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::R32G32B32A32_Typeless:
    case PixelFormat::R32G32B32A32_Float:
    case PixelFormat::R32G32B32A32_UInt:
    case PixelFormat::R32G32B32A32_SInt:
    case PixelFormat::R16G16B16A16_Typeless:
    case PixelFormat::R16G16B16A16_Float:
    case PixelFormat::R16G16B16A16_UNorm:
    case PixelFormat::R16G16B16A16_UInt:
    case PixelFormat::R16G16B16A16_SNorm:
    case PixelFormat::R16G16B16A16_SInt:
    case PixelFormat::R10G10B10A2_UNorm:
    case PixelFormat::R10G10B10A2_UInt:
    case PixelFormat::R8G8B8A8_Typeless:
    case PixelFormat::R8G8B8A8_UNorm:
    case PixelFormat::R8G8B8A8_UNorm_sRGB:
    case PixelFormat::R8G8B8A8_UInt:
    case PixelFormat::R8G8B8A8_SNorm:
    case PixelFormat::R8G8B8A8_SInt:
    case PixelFormat::R8G8_B8G8_UNorm:
    case PixelFormat::G8R8_G8B8_UNorm:
    case PixelFormat::BC1_Typeless:
    case PixelFormat::BC1_UNorm:
    case PixelFormat::BC1_UNorm_sRGB:
    case PixelFormat::BC2_Typeless:
    case PixelFormat::BC2_UNorm:
    case PixelFormat::BC2_UNorm_sRGB:
    case PixelFormat::BC3_Typeless:
    case PixelFormat::BC3_UNorm:
    case PixelFormat::BC3_UNorm_sRGB:
    case PixelFormat::B5G5R5A1_UNorm:
    case PixelFormat::B8G8R8A8_UNorm:
    case PixelFormat::B8G8R8X8_UNorm:
    case PixelFormat::R10G10B10_Xr_Bias_A2_UNorm:
    case PixelFormat::B8G8R8A8_Typeless:
    case PixelFormat::B8G8R8A8_UNorm_sRGB:
    case PixelFormat::B8G8R8X8_Typeless:
    case PixelFormat::B8G8R8X8_UNorm_sRGB:
    case PixelFormat::ASTC_4x4_UNorm:
    case PixelFormat::ASTC_4x4_UNorm_sRGB:
    case PixelFormat::ASTC_6x6_UNorm:
    case PixelFormat::ASTC_6x6_UNorm_sRGB:
    case PixelFormat::ASTC_8x8_UNorm:
    case PixelFormat::ASTC_8x8_UNorm_sRGB:
    case PixelFormat::ASTC_10x10_UNorm:
    case PixelFormat::ASTC_10x10_UNorm_sRGB:
        return 4;
    case PixelFormat::R32G32B32_Typeless:
    case PixelFormat::R32G32B32_Float:
    case PixelFormat::R32G32B32_UInt:
    case PixelFormat::R32G32B32_SInt:
    case PixelFormat::R11G11B10_Float:
    case PixelFormat::R9G9B9E5_SharedExp:
    case PixelFormat::B5G6R5_UNorm:
        return 3;
    case PixelFormat::R32G32_Typeless:
    case PixelFormat::R32G32_Float:
    case PixelFormat::R32G32_UInt:
    case PixelFormat::R32G32_SInt:
    case PixelFormat::R32G8X24_Typeless:
    case PixelFormat::R16G16_Typeless:
    case PixelFormat::R16G16_Float:
    case PixelFormat::R16G16_UNorm:
    case PixelFormat::R16G16_UInt:
    case PixelFormat::R16G16_SNorm:
    case PixelFormat::R16G16_SInt:
    case PixelFormat::R24G8_Typeless:
    case PixelFormat::R8G8_Typeless:
    case PixelFormat::R8G8_UNorm:
    case PixelFormat::R8G8_UInt:
    case PixelFormat::R8G8_SNorm:
    case PixelFormat::R8G8_SInt:
    case PixelFormat::BC5_Typeless:
    case PixelFormat::BC5_UNorm:
    case PixelFormat::BC5_SNorm:
        return 2;
    case PixelFormat::R32_Typeless:
    case PixelFormat::D32_Float:
    case PixelFormat::R32_Float:
    case PixelFormat::R32_UInt:
    case PixelFormat::R32_SInt:
    case PixelFormat::D24_UNorm_S8_UInt:
    case PixelFormat::R24_UNorm_X8_Typeless:
    case PixelFormat::X24_Typeless_G8_UInt:
    case PixelFormat::R16_Typeless:
    case PixelFormat::R16_Float:
    case PixelFormat::D16_UNorm:
    case PixelFormat::R16_UNorm:
    case PixelFormat::R16_UInt:
    case PixelFormat::R16_SNorm:
    case PixelFormat::R16_SInt:
    case PixelFormat::R8_Typeless:
    case PixelFormat::R8_UNorm:
    case PixelFormat::R8_UInt:
    case PixelFormat::R8_SNorm:
    case PixelFormat::R8_SInt:
    case PixelFormat::A8_UNorm:
    case PixelFormat::R1_UNorm:
    case PixelFormat::BC4_Typeless:
    case PixelFormat::BC4_UNorm:
    case PixelFormat::BC4_SNorm:
        return 1;
    default:
        return 0;
    }
}

int32 PixelFormatExtensions::ComputeBlockSize(PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::BC1_Typeless:
    case PixelFormat::BC1_UNorm:
    case PixelFormat::BC1_UNorm_sRGB:
    case PixelFormat::BC2_Typeless:
    case PixelFormat::BC2_UNorm:
    case PixelFormat::BC2_UNorm_sRGB:
    case PixelFormat::BC3_Typeless:
    case PixelFormat::BC3_UNorm:
    case PixelFormat::BC3_UNorm_sRGB:
    case PixelFormat::BC4_Typeless:
    case PixelFormat::BC4_UNorm:
    case PixelFormat::BC4_SNorm:
    case PixelFormat::BC5_Typeless:
    case PixelFormat::BC5_UNorm:
    case PixelFormat::BC5_SNorm:
    case PixelFormat::BC6H_Typeless:
    case PixelFormat::BC6H_Uf16:
    case PixelFormat::BC6H_Sf16:
    case PixelFormat::BC7_Typeless:
    case PixelFormat::BC7_UNorm:
    case PixelFormat::BC7_UNorm_sRGB:
    case PixelFormat::ASTC_4x4_UNorm:
    case PixelFormat::ASTC_4x4_UNorm_sRGB:
        return 4;
    case PixelFormat::ASTC_6x6_UNorm:
    case PixelFormat::ASTC_6x6_UNorm_sRGB:
        return 6;
    case PixelFormat::ASTC_8x8_UNorm:
    case PixelFormat::ASTC_8x8_UNorm_sRGB:
        return 8;
    case PixelFormat::ASTC_10x10_UNorm:
    case PixelFormat::ASTC_10x10_UNorm_sRGB:
        return 10;
    default:
        return 1;
    }
}

PixelFormat PixelFormatExtensions::TosRGB(const PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::R8G8B8A8_UNorm:
        return PixelFormat::R8G8B8A8_UNorm_sRGB;
    case PixelFormat::BC1_UNorm:
        return PixelFormat::BC1_UNorm_sRGB;
    case PixelFormat::BC2_UNorm:
        return PixelFormat::BC2_UNorm_sRGB;
    case PixelFormat::BC3_UNorm:
        return PixelFormat::BC3_UNorm_sRGB;
    case PixelFormat::B8G8R8A8_UNorm:
        return PixelFormat::B8G8R8A8_UNorm_sRGB;
    case PixelFormat::B8G8R8X8_UNorm:
        return PixelFormat::B8G8R8X8_UNorm_sRGB;
    case PixelFormat::BC7_UNorm:
        return PixelFormat::BC7_UNorm_sRGB;
    case PixelFormat::ASTC_4x4_UNorm:
        return PixelFormat::ASTC_4x4_UNorm_sRGB;
    case PixelFormat::ASTC_6x6_UNorm:
        return PixelFormat::ASTC_6x6_UNorm_sRGB;
    case PixelFormat::ASTC_8x8_UNorm:
        return PixelFormat::ASTC_8x8_UNorm_sRGB;
    case PixelFormat::ASTC_10x10_UNorm:
        return PixelFormat::ASTC_10x10_UNorm_sRGB;
    default:
        return format;
    }
}

PixelFormat PixelFormatExtensions::ToNonsRGB(const PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::R8G8B8A8_UNorm_sRGB:
        return PixelFormat::R8G8B8A8_UNorm;
    case PixelFormat::BC1_UNorm_sRGB:
        return PixelFormat::BC1_UNorm;
    case PixelFormat::BC2_UNorm_sRGB:
        return PixelFormat::BC2_UNorm;
    case PixelFormat::BC3_UNorm_sRGB:
        return PixelFormat::BC3_UNorm;
    case PixelFormat::B8G8R8A8_UNorm_sRGB:
        return PixelFormat::B8G8R8A8_UNorm;
    case PixelFormat::B8G8R8X8_UNorm_sRGB:
        return PixelFormat::B8G8R8X8_UNorm;
    case PixelFormat::BC7_UNorm_sRGB:
        return PixelFormat::BC7_UNorm;
    case PixelFormat::ASTC_4x4_UNorm_sRGB:
        return PixelFormat::ASTC_4x4_UNorm;
    case PixelFormat::ASTC_6x6_UNorm_sRGB:
        return PixelFormat::ASTC_6x6_UNorm;
    case PixelFormat::ASTC_8x8_UNorm_sRGB:
        return PixelFormat::ASTC_8x8_UNorm;
    case PixelFormat::ASTC_10x10_UNorm_sRGB:
        return PixelFormat::ASTC_10x10_UNorm;
    default:
        return format;
    }
}

PixelFormat PixelFormatExtensions::MakeTypeless(const PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::R32G32B32A32_Float:
    case PixelFormat::R32G32B32A32_UInt:
    case PixelFormat::R32G32B32A32_SInt:
        return PixelFormat::R32G32B32A32_Typeless;
    case PixelFormat::R32G32B32_Float:
    case PixelFormat::R32G32B32_UInt:
    case PixelFormat::R32G32B32_SInt:
        return PixelFormat::R32G32B32_Typeless;
    case PixelFormat::R16G16B16A16_Float:
    case PixelFormat::R16G16B16A16_UNorm:
    case PixelFormat::R16G16B16A16_UInt:
    case PixelFormat::R16G16B16A16_SNorm:
    case PixelFormat::R16G16B16A16_SInt:
        return PixelFormat::R16G16B16A16_Typeless;
    case PixelFormat::R32G32_Float:
    case PixelFormat::R32G32_UInt:
    case PixelFormat::R32G32_SInt:
        return PixelFormat::R32G32_Typeless;
    case PixelFormat::R10G10B10A2_UNorm:
    case PixelFormat::R10G10B10A2_UInt:
        return PixelFormat::R10G10B10A2_Typeless;
    case PixelFormat::R8G8B8A8_UNorm:
    case PixelFormat::R8G8B8A8_UNorm_sRGB:
    case PixelFormat::R8G8B8A8_UInt:
    case PixelFormat::R8G8B8A8_SNorm:
    case PixelFormat::R8G8B8A8_SInt:
        return PixelFormat::R8G8B8A8_Typeless;
    case PixelFormat::R16G16_Float:
    case PixelFormat::R16G16_UNorm:
    case PixelFormat::R16G16_UInt:
    case PixelFormat::R16G16_SNorm:
    case PixelFormat::R16G16_SInt:
        return PixelFormat::R16G16_Typeless;
    case PixelFormat::D32_Float:
    case PixelFormat::R32_Float:
    case PixelFormat::R32_UInt:
    case PixelFormat::R32_SInt:
        return PixelFormat::R32_Typeless;
    case PixelFormat::R8G8_UNorm:
    case PixelFormat::R8G8_UInt:
    case PixelFormat::R8G8_SNorm:
    case PixelFormat::R8G8_SInt:
        return PixelFormat::R8G8_Typeless;
    case PixelFormat::R16_Float:
    case PixelFormat::D16_UNorm:
    case PixelFormat::R16_UNorm:
    case PixelFormat::R16_UInt:
    case PixelFormat::R16_SNorm:
    case PixelFormat::R16_SInt:
        return PixelFormat::R16_Typeless;
    case PixelFormat::R8_UNorm:
    case PixelFormat::R8_UInt:
    case PixelFormat::R8_SNorm:
    case PixelFormat::R8_SInt:
        return PixelFormat::R8_Typeless;
    case PixelFormat::BC1_UNorm:
    case PixelFormat::BC1_UNorm_sRGB:
        return PixelFormat::BC1_Typeless;
    case PixelFormat::BC2_UNorm:
    case PixelFormat::BC2_UNorm_sRGB:
        return PixelFormat::BC2_Typeless;
    case PixelFormat::BC3_UNorm:
    case PixelFormat::BC3_UNorm_sRGB:
        return PixelFormat::BC3_Typeless;
    case PixelFormat::BC4_UNorm:
    case PixelFormat::BC4_SNorm:
        return PixelFormat::BC4_Typeless;
    case PixelFormat::BC5_UNorm:
    case PixelFormat::BC5_SNorm:
        return PixelFormat::BC5_Typeless;
    case PixelFormat::B8G8R8A8_UNorm:
    case PixelFormat::B8G8R8A8_UNorm_sRGB:
        return PixelFormat::B8G8R8A8_Typeless;
    case PixelFormat::B8G8R8X8_UNorm:
    case PixelFormat::B8G8R8X8_UNorm_sRGB:
        return PixelFormat::B8G8R8X8_Typeless;
    case PixelFormat::BC6H_Uf16:
    case PixelFormat::BC6H_Sf16:
        return PixelFormat::BC6H_Typeless;
    case PixelFormat::BC7_UNorm:
    case PixelFormat::BC7_UNorm_sRGB:
        return PixelFormat::BC7_Typeless;
    case PixelFormat::D24_UNorm_S8_UInt:
        return PixelFormat::R24G8_Typeless;
    case PixelFormat::D32_Float_S8X24_UInt:
        return PixelFormat::R32G8X24_Typeless;
    default:
        return format;
    }
}

PixelFormat PixelFormatExtensions::MakeTypelessFloat(const PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::R32G32B32A32_Typeless:
        return PixelFormat::R32G32B32A32_Float;
    case PixelFormat::R32G32B32_Typeless:
        return PixelFormat::R32G32B32_Float;
    case PixelFormat::R16G16B16A16_Typeless:
        return PixelFormat::R16G16B16A16_Float;
    case PixelFormat::R32G32_Typeless:
        return PixelFormat::R32G32_Float;
    case PixelFormat::R16G16_Typeless:
        return PixelFormat::R16G16_Float;
    case PixelFormat::R32_Typeless:
        return PixelFormat::R32_Float;
    case PixelFormat::R16_Typeless:
        return PixelFormat::R16_Float;
    default:
        return format;
    }
}

PixelFormat PixelFormatExtensions::MakeTypelessUNorm(const PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::R16G16B16A16_Typeless:
        return PixelFormat::R16G16B16A16_UNorm;
    case PixelFormat::R10G10B10A2_Typeless:
        return PixelFormat::R10G10B10A2_UNorm;
    case PixelFormat::R8G8B8A8_Typeless:
        return PixelFormat::R8G8B8A8_UNorm;
    case PixelFormat::R16G16_Typeless:
        return PixelFormat::R16G16_UNorm;
    case PixelFormat::R8G8_Typeless:
        return PixelFormat::R8G8_UNorm;
    case PixelFormat::R16_Typeless:
        return PixelFormat::R16_UNorm;
    case PixelFormat::R8_Typeless:
        return PixelFormat::R8_UNorm;
    case PixelFormat::BC1_Typeless:
        return PixelFormat::BC1_UNorm;
    case PixelFormat::BC2_Typeless:
        return PixelFormat::BC2_UNorm;
    case PixelFormat::BC3_Typeless:
        return PixelFormat::BC3_UNorm;
    case PixelFormat::BC4_Typeless:
        return PixelFormat::BC4_UNorm;
    case PixelFormat::BC5_Typeless:
        return PixelFormat::BC5_UNorm;
    case PixelFormat::B8G8R8A8_Typeless:
        return PixelFormat::B8G8R8A8_UNorm;
    case PixelFormat::B8G8R8X8_Typeless:
        return PixelFormat::B8G8R8X8_UNorm;
    case PixelFormat::BC7_Typeless:
        return PixelFormat::BC7_UNorm;
    default:
        return format;
    }
}

PixelFormat PixelFormatExtensions::FindShaderResourceFormat(const PixelFormat format, bool sRGB)
{
    if (sRGB)
    {
        switch (format)
        {
        case PixelFormat::B8G8R8A8_Typeless:
            return PixelFormat::B8G8R8A8_UNorm_sRGB;
        case PixelFormat::R8G8B8A8_Typeless:
            return PixelFormat::R8G8B8A8_UNorm_sRGB;
        case PixelFormat::BC1_Typeless:
            return PixelFormat::BC1_UNorm_sRGB;
        case PixelFormat::BC2_Typeless:
            return PixelFormat::BC2_UNorm_sRGB;
        case PixelFormat::BC3_Typeless:
            return PixelFormat::BC3_UNorm_sRGB;
        case PixelFormat::BC7_Typeless:
            return PixelFormat::BC7_UNorm_sRGB;
        }
    }
    else
    {
        switch (format)
        {
        case PixelFormat::B8G8R8A8_Typeless:
            return PixelFormat::B8G8R8A8_UNorm;
        case PixelFormat::R8G8B8A8_Typeless:
            return PixelFormat::R8G8B8A8_UNorm;
        case PixelFormat::BC1_Typeless:
            return PixelFormat::BC1_UNorm;
        case PixelFormat::BC2_Typeless:
            return PixelFormat::BC2_UNorm;
        case PixelFormat::BC3_Typeless:
            return PixelFormat::BC3_UNorm;
        case PixelFormat::BC7_Typeless:
            return PixelFormat::BC7_UNorm;
        }
    }
    switch (format)
    {
    case PixelFormat::R24G8_Typeless:
        return PixelFormat::R24_UNorm_X8_Typeless;
    case PixelFormat::R32_Typeless:
        return PixelFormat::R32_Float;
    case PixelFormat::R16_Typeless:
        return PixelFormat::R16_UNorm;
    case PixelFormat::D16_UNorm:
        return PixelFormat::R16_UNorm;
    case PixelFormat::D24_UNorm_S8_UInt:
        return PixelFormat::R24_UNorm_X8_Typeless;
    case PixelFormat::D32_Float:
        return PixelFormat::R32_Float;
    case PixelFormat::D32_Float_S8X24_UInt:
        return PixelFormat::R32_Float_X8X24_Typeless;
    case PixelFormat::YUY2:
        return PixelFormat::R8G8B8A8_UNorm;
    }
    return format;
}

PixelFormat PixelFormatExtensions::FindUnorderedAccessFormat(const PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::B8G8R8A8_Typeless:
        return PixelFormat::B8G8R8A8_UNorm;
    case PixelFormat::R8G8B8A8_Typeless:
        return PixelFormat::R8G8B8A8_UNorm;
    case PixelFormat::YUY2:
        return PixelFormat::R8G8B8A8_UNorm;
    }
    return format;
}

PixelFormat PixelFormatExtensions::FindDepthStencilFormat(const PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::R24G8_Typeless:
    case PixelFormat::R24_UNorm_X8_Typeless:
        return PixelFormat::D24_UNorm_S8_UInt;
    case PixelFormat::R32_Typeless:
        return PixelFormat::D32_Float;
    case PixelFormat::R16_Typeless:
        return PixelFormat::D16_UNorm;
    }
    return format;
}

PixelFormat PixelFormatExtensions::FindUncompressedFormat(PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::BC1_Typeless:
    case PixelFormat::BC2_Typeless:
    case PixelFormat::BC3_Typeless:
        return PixelFormat::R8G8B8A8_Typeless;
    case PixelFormat::BC1_UNorm:
    case PixelFormat::BC2_UNorm:
    case PixelFormat::BC3_UNorm:
        return PixelFormat::R8G8B8A8_UNorm;
    case PixelFormat::BC1_UNorm_sRGB:
    case PixelFormat::BC2_UNorm_sRGB:
    case PixelFormat::BC3_UNorm_sRGB:
        return PixelFormat::R8G8B8A8_UNorm_sRGB;
    case PixelFormat::BC4_Typeless:
        return PixelFormat::R8_Typeless;
    case PixelFormat::BC4_UNorm:
        return PixelFormat::R8_UNorm;
    case PixelFormat::BC4_SNorm:
        return PixelFormat::R8_SNorm;
    case PixelFormat::BC5_Typeless:
        return PixelFormat::R16G16_Typeless;
    case PixelFormat::BC5_UNorm:
        return PixelFormat::R16G16_UNorm;
    case PixelFormat::BC5_SNorm:
        return PixelFormat::R16G16_SNorm;
    case PixelFormat::BC7_Typeless:
    case PixelFormat::BC6H_Typeless:
        return PixelFormat::R16G16B16A16_Typeless;
    case PixelFormat::BC7_UNorm:
    case PixelFormat::BC6H_Uf16:
    case PixelFormat::BC6H_Sf16:
        return PixelFormat::R16G16B16A16_Float;
    case PixelFormat::BC7_UNorm_sRGB:
        return PixelFormat::R16G16B16A16_UNorm;
    case PixelFormat::ASTC_4x4_UNorm:
    case PixelFormat::ASTC_6x6_UNorm:
    case PixelFormat::ASTC_8x8_UNorm:
    case PixelFormat::ASTC_10x10_UNorm:
        return PixelFormat::R8G8B8A8_UNorm;
    case PixelFormat::ASTC_4x4_UNorm_sRGB:
    case PixelFormat::ASTC_6x6_UNorm_sRGB:
    case PixelFormat::ASTC_8x8_UNorm_sRGB:
    case PixelFormat::ASTC_10x10_UNorm_sRGB:
        return PixelFormat::R8G8B8A8_UNorm_sRGB;
    default:
        return format;
    }
}
