// Copyright (c) Wojciech Figat. All rights reserved.

#include "PixelFormatExtensions.h"
#include "PixelFormatSampler.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Half.h"
#include "Engine/Core/Math/Packed.h"
#include "Engine/Core/Math/Vector4.h"

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

static PixelFormatSampler PixelFormatSamplers[] =
{
    {
        PixelFormat::R32G32B32A32_Float,
        sizeof(Float4),
        [](const void* ptr)
        {
            return *(Float4*)ptr;
        },
        [](void* ptr, const Float4& value)
        {
            *(Float4*)ptr = value;
        },
    },
    {
        PixelFormat::R32G32B32_Float,
        sizeof(Float3),
        [](const void* ptr)
        {
            return Float4(*(Float3*)ptr, 1.0f);
        },
        [](void* ptr, const Float4& value)
        {
            *(Float3*)ptr = Float3(value);
        },
    },
    {
        PixelFormat::R16G16B16A16_Float,
        sizeof(Half4),
        [](const void* ptr)
        {
            return ((Half4*)ptr)->ToFloat4();
        },
        [](void* ptr, const Float4& value)
        {
            *(Half4*)ptr = Half4(value.X, value.Y, value.Z, value.W);
        },
    },
    {
        PixelFormat::R16G16B16A16_UNorm,
        sizeof(RGBA16UNorm),
        [](const void* ptr)
        {
            return ((RGBA16UNorm*)ptr)->ToFloat4();
        },
        [](void* ptr, const Float4& value)
        {
            *(RGBA16UNorm*)ptr = RGBA16UNorm(value.X, value.Y, value.Z, value.W);
        }
    },
    {
        PixelFormat::R32G32_Float,
        sizeof(Float2),
        [](const void* ptr)
        {
            return Float4(((Float2*)ptr)->X, ((Float2*)ptr)->Y, 0.0f, 0.0f);
        },
        [](void* ptr, const Float4& value)
        {
            *(Float2*)ptr = Float2(value.X, value.Y);
        },
    },
    {
        PixelFormat::R8G8B8A8_UNorm,
        sizeof(Color32),
        [](const void* ptr)
        {
            return Float4(Color(*(Color32*)ptr));
        },
        [](void* ptr, const Float4& value)
        {
            *(Color32*)ptr = Color32(value);
        },
    },
    {
        PixelFormat::R8G8B8A8_UNorm_sRGB,
        sizeof(Color32),
        [](const void* ptr)
        {
            return Float4(Color::SrgbToLinear(Color(*(Color32*)ptr)));
        },
        [](void* ptr, const Float4& value)
        {
            Color srgb = Color::LinearToSrgb((const Color&)value);
            *(Color32*)ptr = Color32(srgb);
        },
    },
    {
        PixelFormat::R8G8_UNorm,
        sizeof(uint16),
        [](const void* ptr)
        {
            const uint8* rg = (const uint8*)ptr;
            return Float4((float)rg[0] / MAX_uint8, (float)rg[1] / MAX_uint8, 0, 1);
        },
        [](void* ptr, const Float4& value)
        {
            uint8* rg = (uint8*)ptr;
            rg[0] = (uint8)(value.X * MAX_uint8);
            rg[1] = (uint8)(value.Y * MAX_uint8);
        },
    },
    {
        PixelFormat::R16G16_Float,
        sizeof(Half2),
        [](const void* ptr)
        {
            const Float2 rg = ((Half2*)ptr)->ToFloat2();
            return Float4(rg.X, rg.Y, 0, 1);
        },
        [](void* ptr, const Float4& value)
        {
            *(Half2*)ptr = Half2(value.X, value.Y);
        },
    },
    {
        PixelFormat::R16G16_UNorm,
        sizeof(RG16UNorm),
        [](const void* ptr)
        {
            const Float2 rg = ((RG16UNorm*)ptr)->ToFloat2();
            return Float4(rg.X, rg.Y, 0, 1);
        },
        [](void* ptr, const Float4& value)
        {
            *(RG16UNorm*)ptr = RG16UNorm(value.X, value.Y);
        },
    },
    {
        PixelFormat::R32_Float,
        sizeof(float),
        [](const void* ptr)
        {
            return Float4(*(float*)ptr, 0, 0, 1);
        },
        [](void* ptr, const Float4& value)
        {
            *(float*)ptr = value.X;
        },
    },
    {
        PixelFormat::R16_Float,
        sizeof(Half),
        [](const void* ptr)
        {
            return Float4(Float16Compressor::Decompress(*(Half*)ptr), 0, 0, 1);
        },
        [](void* ptr, const Float4& value)
        {
            *(Half*)ptr = Float16Compressor::Compress(value.X);
        },
    },
    {
        PixelFormat::R16_UNorm,
        sizeof(uint16),
        [](const void* ptr)
        {
            return Float4((float)*(uint16*)ptr / MAX_uint16, 0, 0, 1);
        },
        [](void* ptr, const Float4& value)
        {
            *(uint16*)ptr = (uint16)(value.X * MAX_uint16);
        },
    },
    {
        PixelFormat::R8_UNorm,
        sizeof(uint8),
        [](const void* ptr)
        {
            return Float4((float)*(byte*)ptr / MAX_uint8, 0, 0, 1);
        },
        [](void* ptr, const Float4& value)
        {
            *(byte*)ptr = (byte)(value.X * MAX_uint8);
        },
    },
    {
        PixelFormat::A8_UNorm,
        sizeof(uint8),
        [](const void* ptr)
        {
            return Float4(0, 0, 0, (float)*(byte*)ptr / MAX_uint8);
        },
        [](void* ptr, const Float4& value)
        {
            *(byte*)ptr = (byte)(value.W * MAX_uint8);
        },
    },
    {
        PixelFormat::R32_UInt,
        sizeof(uint32),
        [](const void* ptr)
        {
            return Float4((float)*(uint32*)ptr, 0, 0, 1);
        },
        [](void* ptr, const Float4& value)
        {
            *(uint32*)ptr = (uint32)value.X;
        },
    },
    {
        PixelFormat::R32_SInt,
        sizeof(int32),
        [](const void* ptr)
        {
            return Float4((float)*(int32*)ptr, 0, 0, 1);
        },
        [](void* ptr, const Float4& value)
        {
            *(int32*)ptr = (int32)value.X;
        },
    },
    {
        PixelFormat::R16_UInt,
        sizeof(uint16),
        [](const void* ptr)
        {
            return Float4((float)*(uint16*)ptr, 0, 0, 1);
        },
        [](void* ptr, const Float4& value)
        {
            *(uint16*)ptr = (uint16)value.X;
        },
    },
    {
        PixelFormat::R16_SInt,
        sizeof(int16),
        [](const void* ptr)
        {
            return Float4((float)*(int16*)ptr, 0, 0, 1);
        },
        [](void* ptr, const Float4& value)
        {
            *(int16*)ptr = (int16)value.X;
        },
    },
    {
        PixelFormat::R8_UInt,
        sizeof(uint8),
        [](const void* ptr)
        {
            return Float4((float)*(uint8*)ptr, 0, 0, 1);
        },
        [](void* ptr, const Float4& value)
        {
            *(uint8*)ptr = (uint8)value.X;
        },
    },
    {
        PixelFormat::R8_SInt,
        sizeof(int8),
        [](const void* ptr)
        {
            return Float4((float)*(int8*)ptr, 0, 0, 1);
        },
        [](void* ptr, const Float4& value)
        {
            *(int8*)ptr = (int8)value.X;
        },
    },
    {
        PixelFormat::B8G8R8A8_UNorm,
        sizeof(Color32),
        [](const void* ptr)
        {
            const Color32 bgra = *(Color32*)ptr;
            return Float4(Color(Color32(bgra.B, bgra.G, bgra.R, bgra.A)));
        },
        [](void* ptr, const Float4& value)
        {
            *(Color32*)ptr = Color32(byte(value.Z * MAX_uint8), byte(value.Y * MAX_uint8), byte(value.X * MAX_uint8), byte(value.W * MAX_uint8));
        },
    },
    {
        PixelFormat::B8G8R8A8_UNorm_sRGB,
        sizeof(Color32),
        [](const void* ptr)
        {
            const Color32 bgra = *(Color32*)ptr;
            return Float4(Color::SrgbToLinear(Color(Color32(bgra.B, bgra.G, bgra.R, bgra.A))));
        },
        [](void* ptr, const Float4& value)
        {
            Color srgb = Color::LinearToSrgb((const Color&)value);
            *(Color32*)ptr = Color32(byte(srgb.B * MAX_uint8), byte(srgb.G * MAX_uint8), byte(srgb.R * MAX_uint8), byte(srgb.A * MAX_uint8));
        },
    },
    {
        PixelFormat::B8G8R8X8_UNorm,
        sizeof(Color32),
        [](const void* ptr)
        {
            const Color32 bgra = *(Color32*)ptr;
            return Float4(Color(Color32(bgra.B, bgra.G, bgra.R, MAX_uint8)));
        },
        [](void* ptr, const Float4& value)
        {
            *(Color32*)ptr = Color32(byte(value.Z * MAX_uint8), byte(value.Y * MAX_uint8), byte(value.X * MAX_uint8), MAX_uint8);
        },
    },
    {
        PixelFormat::B8G8R8X8_UNorm_sRGB,
        sizeof(Color32),
        [](const void* ptr)
        {
            const Color32 bgra = *(Color32*)ptr;
            return Float4(Color::SrgbToLinear(Color(Color32(bgra.B, bgra.G, bgra.R, MAX_uint8))));
        },
        [](void* ptr, const Float4& value)
        {
            Color srgb = Color::LinearToSrgb((const Color&)value);
            *(Color32*)ptr = Color32(byte(srgb.B * MAX_uint8), byte(srgb.G * MAX_uint8), byte(srgb.R * MAX_uint8), MAX_uint8);
        },
    },
    {
        PixelFormat::R11G11B10_Float,
        sizeof(FloatR11G11B10),
        [](const void* ptr)
        {
            const Float3 rgb = ((FloatR11G11B10*)ptr)->ToFloat3();
            return Float4(rgb.X, rgb.Y, rgb.Z, 0.0f);
        },
        [](void* ptr, const Float4& value)
        {
            *(FloatR11G11B10*)ptr = FloatR11G11B10(value.X, value.Y, value.Z);
        },
    },
    {
        PixelFormat::R10G10B10A2_UNorm,
        sizeof(FloatR10G10B10A2),
        [](const void* ptr)
        {
            return ((FloatR10G10B10A2*)ptr)->ToFloat4();
        },
        [](void* ptr, const Float4& value)
        {
            *(FloatR10G10B10A2*)ptr = FloatR10G10B10A2(value.X, value.Y, value.Z, value.W);
        },
    },
    {
        PixelFormat::R8G8B8A8_UInt,
        sizeof(Color32),
        [](const void* ptr)
        {
            uint8 data[4];
            Platform::MemoryCopy(data, ptr, sizeof(data));
            return Float4(data[0], data[1], data[2], data[3]);
        },
        [](void* ptr, const Float4& value)
        {
            uint8 data[4] = { (uint8)value.X, (uint8)value.Y, (uint8)value.Z, (uint8)value.W};
            Platform::MemoryCopy(ptr, data, sizeof(data));
        },
    },
    {
        PixelFormat::R8G8B8A8_SInt,
        sizeof(Color32),
        [](const void* ptr)
        {
            int8 data[4];
            Platform::MemoryCopy(data, ptr, sizeof(data));
            return Float4(data[0], data[1], data[2], data[3]);
        },
        [](void* ptr, const Float4& value)
        {
            int8 data[4] = { (int8)value.X, (int8)value.Y, (int8)value.Z, (int8)value.W};
            Platform::MemoryCopy(ptr, data, sizeof(data));
        },
    },
    {
        PixelFormat::R16G16B16A16_UInt,
        sizeof(uint16) * 4,
        [](const void* ptr)
        {
            uint16 data[4];
            Platform::MemoryCopy(data, ptr, sizeof(data));
            return Float4(data[0], data[1], data[2], data[3]);
        },
        [](void* ptr, const Float4& value)
        {
            uint16 data[4] = { (uint16)value.X, (uint16)value.Y, (uint16)value.Z, (uint16)value.W};
            Platform::MemoryCopy(ptr, data, sizeof(data));
        },
    },
    {
        PixelFormat::R16G16B16A16_SInt,
        sizeof(int16) * 4,
        [](const void* ptr)
        {
            int16 data[4];
            Platform::MemoryCopy(data, ptr, sizeof(data));
            return Float4(data[0], data[1], data[2], data[3]);
        },
        [](void* ptr, const Float4& value)
        {
            int16 data[4] = { (int16)value.X, (int16)value.Y, (int16)value.Z, (int16)value.W};
            Platform::MemoryCopy(ptr, data, sizeof(data));
        },
    },
    {
        PixelFormat::R32G32B32A32_UInt,
        sizeof(uint32) * 4,
        [](const void* ptr)
        {
            uint32 data[4];
            Platform::MemoryCopy(data, ptr, sizeof(data));
            return Float4((float)data[0], (float)data[1], (float)data[2], (float)data[3]);
        },
        [](void* ptr, const Float4& value)
        {
            uint32 data[4] = { (uint32)value.X, (uint32)value.Y, (uint32)value.Z, (uint32)value.W};
            Platform::MemoryCopy(ptr, data, sizeof(data));
        },
    },
    {
        PixelFormat::R32G32B32A32_SInt,
        sizeof(int32) * 4,
        [](const void* ptr)
        {
            int32 data[4];
            Platform::MemoryCopy(data, ptr, sizeof(data));
            return Float4((float)data[0], (float)data[1], (float)data[2], (float)data[3]);
        },
        [](void* ptr, const Float4& value)
        {
            int32 data[4] = { (int32)value.X, (int32)value.Y, (int32)value.Z, (int32)value.W};
            Platform::MemoryCopy(ptr, data, sizeof(data));
        },
    },
};

void PixelFormatSampler::Store(void* data, int32 x, int32 y, int32 rowPitch, const Color& color) const
{
    Write((byte*)data + rowPitch * y + PixelSize * x, (Float4&)color);
}

Float4 PixelFormatSampler::Sample(const void* data, int32 x) const
{
    return Read((const byte*)data + x * PixelSize);
}

Color PixelFormatSampler::SamplePoint(const void* data, const Float2& uv, const Int2& size, int32 rowPitch) const
{
    const Int2 end = size - 1;
    const Int2 uvFloor(Math::Min(Math::FloorToInt(uv.X * size.X), end.X), Math::Min(Math::FloorToInt(uv.Y * size.Y), end.Y));
    Float4 result = Read((const byte*)data + rowPitch * uvFloor.Y + PixelSize * uvFloor.X);
    return *(Color*)&result;
}

Color PixelFormatSampler::SamplePoint(const void* data, int32 x, int32 y, int32 rowPitch) const
{
    Float4 result = Read((const byte*)data + rowPitch * y + PixelSize * x);
    return *(Color*)&result;
}

Color PixelFormatSampler::SampleLinear(const void* data, const Float2& uv, const Int2& size, int32 rowPitch) const
{
    const Int2 end = size - 1;
    const Int2 uvFloor(Math::Min(Math::FloorToInt(uv.X * size.X), end.X), Math::Min(Math::FloorToInt(uv.Y * size.Y), end.Y));
    const Int2 uvNext(Math::Min(uvFloor.X + 1, end.X), Math::Min(uvFloor.Y + 1, end.Y));
    const Float2 uvFraction(uv.X * size.Y - uvFloor.X, uv.Y * size.Y - uvFloor.Y);

    const Float4 v00 = Read((const byte*)data + rowPitch * uvFloor.Y + PixelSize * uvFloor.X);
    const Float4 v01 = Read((const byte*)data + rowPitch * uvFloor.Y + PixelSize * uvNext.X);
    const Float4 v10 = Read((const byte*)data + rowPitch * uvNext.Y + PixelSize * uvFloor.X);
    const Float4 v11 = Read((const byte*)data + rowPitch * uvNext.Y + PixelSize * uvNext.X);

    Float4 result = Float4::Lerp(Float4::Lerp(v00, v01, uvFraction.X), Float4::Lerp(v10, v11, uvFraction.X), uvFraction.Y);
    return *(Color*)&result;
}

const PixelFormatSampler* PixelFormatSampler::Get(PixelFormat format)
{
    format = PixelFormatExtensions::MakeTypelessFloat(format);
    for (const auto& sampler : PixelFormatSamplers)
    {
        if (sampler.Format == format)
            return &sampler;
    }
    return nullptr;
}

#if !COMPILE_WITHOUT_CSHARP

void PixelFormatExtensions::GetSamplerInternal(PixelFormat format, int32& pixelSize, void** read, void** write)
{
    if (const PixelFormatSampler* sampler = PixelFormatSampler::Get(format))
    {
        pixelSize = sampler->PixelSize;
        *read = (void*)sampler->Read;
        *write = (void*)sampler->Write;
    }
}

#endif
