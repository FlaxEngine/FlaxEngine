// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_WEBGPU

#include "RenderToolsWebGPU.h"
#include "Engine/Graphics/PixelFormat.h"
#include <emscripten/emscripten.h>

WGPUWaitStatus WebGPUAsyncWait(AsyncWaitParamsWebGPU params)
{
#if 0
    // This needs WGPUInstanceFeatureName_TimedWaitAny which works only with ASYNCIFY enabled
    WGPUFutureWaitInfo futureWaitInfo;
    futureWaitInfo.future = future;
    futureWaitInfo.completed = WGPU_FALSE;
    uint64 timeoutNS = 5000000000ull; // Wait max 5 second
    return wgpuInstanceWaitAny(params.Instance, 1, &futureWaitInfo, timeoutNS);
#endif

#if WEBGPU_ASYNCIFY
    auto startTime = Platform::GetTimeSeconds();
    int32 ticksLeft = 500; // Wait max 5 second
    while (Platform::AtomicRead(&params.Data->Result) == 0 && ticksLeft-- > 0)
        emscripten_sleep(10);
    if (ticksLeft <= 0)
    {
        params.Data->WaitTime = Platform::GetTimeSeconds() - startTime;
        return WGPUWaitStatus_TimedOut;
    }
    return params.Data->Result == 1 ? WGPUWaitStatus_Success : WGPUWaitStatus_Error;
#else
    // Not possible to implement it here with stack preservation (need to go back with main thread to the browser)
    // Make GPU adapter/device requests register custom retry via emscripten_set_main_loop with coroutine or something like that to make it work without ASYNCIFY
    return WGPUWaitStatus_Error;
#endif
}

WGPUVertexFormat RenderToolsWebGPU::ToVertexFormat(PixelFormat format)
{
    switch (format)
    {
        // @formatter:off
    case PixelFormat::R8_UInt: return WGPUVertexFormat_Uint8;
    case PixelFormat::R8G8_UInt: return WGPUVertexFormat_Uint8x2;
    case PixelFormat::R8G8B8A8_UInt: return WGPUVertexFormat_Uint8x4;
    case PixelFormat::R8_SInt: return WGPUVertexFormat_Sint8;
    case PixelFormat::R8G8_SInt: return WGPUVertexFormat_Sint8x2;
    case PixelFormat::R8G8B8A8_SInt: return WGPUVertexFormat_Sint8x4;
    case PixelFormat::R8_UNorm: return WGPUVertexFormat_Unorm8;
    case PixelFormat::R8G8_UNorm: return WGPUVertexFormat_Unorm8x2;
    case PixelFormat::R8G8B8A8_UNorm: return WGPUVertexFormat_Unorm8x4;
    case PixelFormat::R8_SNorm: return WGPUVertexFormat_Snorm8;
    case PixelFormat::R8G8_SNorm: return WGPUVertexFormat_Snorm8x2;
    case PixelFormat::R8G8B8A8_SNorm: return WGPUVertexFormat_Snorm8x4;
    case PixelFormat::R16_UInt: return WGPUVertexFormat_Uint16;
    case PixelFormat::R16G16_UInt: return WGPUVertexFormat_Uint16x2;
    case PixelFormat::R16G16B16A16_UInt: return WGPUVertexFormat_Uint16x4;
    case PixelFormat::R16_SInt: return WGPUVertexFormat_Sint16;
    case PixelFormat::R16G16_SInt: return WGPUVertexFormat_Sint16x2;
    case PixelFormat::R16G16B16A16_SInt: return WGPUVertexFormat_Sint16x4;
    case PixelFormat::R16_UNorm: return WGPUVertexFormat_Unorm16;
    case PixelFormat::R16G16_UNorm: return WGPUVertexFormat_Unorm16x2;
    case PixelFormat::R16G16B16A16_UNorm: return WGPUVertexFormat_Unorm16x4;
    case PixelFormat::R16_SNorm: return WGPUVertexFormat_Snorm16;
    case PixelFormat::R16G16_SNorm: return WGPUVertexFormat_Snorm16x2;
    case PixelFormat::R16G16B16A16_SNorm: return WGPUVertexFormat_Snorm16x4;
    case PixelFormat::R16_Float: return WGPUVertexFormat_Float16;
    case PixelFormat::R16G16_Float: return WGPUVertexFormat_Float16x2;
    case PixelFormat::R16G16B16A16_Float: return WGPUVertexFormat_Float16x4;
    case PixelFormat::R32_Float: return WGPUVertexFormat_Float32;
    case PixelFormat::R32G32_Float: return WGPUVertexFormat_Float32x2;
    case PixelFormat::R32G32B32_Float: return WGPUVertexFormat_Float32x3;
    case PixelFormat::R32G32B32A32_Float: return WGPUVertexFormat_Float32x4;
    case PixelFormat::R32_UInt: return WGPUVertexFormat_Uint32;
    case PixelFormat::R32G32_UInt: return WGPUVertexFormat_Uint32x2;
    case PixelFormat::R32G32B32_UInt: return WGPUVertexFormat_Uint32x3;
    case PixelFormat::R32G32B32A32_UInt: return WGPUVertexFormat_Uint32x4;
    case PixelFormat::R32_SInt: return WGPUVertexFormat_Sint32;
    case PixelFormat::R32G32_SInt: return WGPUVertexFormat_Sint32x2;
    case PixelFormat::R32G32B32_SInt: return WGPUVertexFormat_Sint32x3;
    case PixelFormat::R32G32B32A32_SInt: return WGPUVertexFormat_Sint32x4;
    case PixelFormat::R10G10B10A2_UNorm: return WGPUVertexFormat_Unorm10_10_10_2;
    case PixelFormat::B8G8R8A8_UNorm: return WGPUVertexFormat_Unorm8x4BGRA;
    default: return WGPUVertexFormat_Force32;
        // @formatter:on
    }
}

WGPUTextureFormat RenderToolsWebGPU::ToTextureFormat(PixelFormat format)
{
    switch (format)
    {
        // @formatter:off
    case PixelFormat::R32G32B32A32_Typeless:
    case PixelFormat::R32G32B32A32_Float: return WGPUTextureFormat_RGBA32Float;
    case PixelFormat::R32G32B32A32_UInt: return WGPUTextureFormat_RGBA32Uint;
    case PixelFormat::R32G32B32A32_SInt: return WGPUTextureFormat_RGBA32Sint;
    case PixelFormat::R16G16B16A16_Typeless:
    case PixelFormat::R16G16B16A16_Float: return WGPUTextureFormat_RGBA16Float;
    case PixelFormat::R16G16B16A16_UNorm: return WGPUTextureFormat_RGBA16Unorm;
    case PixelFormat::R16G16B16A16_UInt: return WGPUTextureFormat_RGBA16Uint;
    case PixelFormat::R16G16B16A16_SNorm: return WGPUTextureFormat_RGBA16Snorm;
    case PixelFormat::R16G16B16A16_SInt: return WGPUTextureFormat_RGBA16Sint;
    case PixelFormat::R32G32_Typeless:
    case PixelFormat::R32G32_Float: return WGPUTextureFormat_RG32Float;
    case PixelFormat::R32G32_UInt: return WGPUTextureFormat_RG32Uint;
    case PixelFormat::R32G32_SInt: return WGPUTextureFormat_RG32Sint;
    case PixelFormat::R32G8X24_Typeless:
    case PixelFormat::D32_Float_S8X24_UInt:
    case PixelFormat::R32_Float_X8X24_Typeless:
    case PixelFormat::X32_Typeless_G8X24_UInt: return WGPUTextureFormat_Depth32FloatStencil8;
    case PixelFormat::R10G10B10A2_Typeless:
    case PixelFormat::R10G10B10A2_UNorm: return WGPUTextureFormat_RGB10A2Unorm;
    case PixelFormat::R10G10B10A2_UInt: return WGPUTextureFormat_RGB10A2Uint;
    case PixelFormat::R11G11B10_Float: return WGPUTextureFormat_RG11B10Ufloat;
    case PixelFormat::R8G8B8A8_Typeless:
    case PixelFormat::R8G8B8A8_UNorm: return WGPUTextureFormat_RGBA8Unorm;
    case PixelFormat::R8G8B8A8_UNorm_sRGB: return WGPUTextureFormat_RGBA8UnormSrgb;
    case PixelFormat::R8G8B8A8_UInt: return WGPUTextureFormat_RGBA8Uint;
    case PixelFormat::R8G8B8A8_SNorm: return WGPUTextureFormat_RGBA8Snorm;
    case PixelFormat::R8G8B8A8_SInt: return WGPUTextureFormat_RGBA8Sint;
    case PixelFormat::R16G16_Typeless:
    case PixelFormat::R16G16_Float: return WGPUTextureFormat_RG16Float;
    case PixelFormat::R16G16_UNorm: return WGPUTextureFormat_RG16Unorm;
    case PixelFormat::R16G16_UInt: return WGPUTextureFormat_RG16Uint;
    case PixelFormat::R16G16_SNorm: return WGPUTextureFormat_RG16Snorm;
    case PixelFormat::R16G16_SInt: return WGPUTextureFormat_RG16Sint;
    case PixelFormat::D32_Float: return WGPUTextureFormat_Depth32Float;
    case PixelFormat::R32_Typeless:
    case PixelFormat::R32_Float: return WGPUTextureFormat_R32Float;
    case PixelFormat::R32_UInt: return WGPUTextureFormat_R32Uint;
    case PixelFormat::R32_SInt: return WGPUTextureFormat_R32Sint;
    case PixelFormat::R24G8_Typeless:
    case PixelFormat::D24_UNorm_S8_UInt:
    case PixelFormat::R24_UNorm_X8_Typeless:
    case PixelFormat::X24_Typeless_G8_UInt: return WGPUTextureFormat_Depth24PlusStencil8;
    case PixelFormat::R8G8_Typeless:
    case PixelFormat::R8G8_UNorm: return WGPUTextureFormat_RG8Unorm;
    case PixelFormat::R8G8_UInt: return WGPUTextureFormat_RG8Uint;
    case PixelFormat::R8G8_SNorm: return WGPUTextureFormat_RG8Snorm;
    case PixelFormat::R8G8_SInt: return WGPUTextureFormat_RG8Sint;
    case PixelFormat::R16_Typeless:
    case PixelFormat::R16_Float: return WGPUTextureFormat_R16Float;
    case PixelFormat::D16_UNorm: return WGPUTextureFormat_Depth16Unorm;
    case PixelFormat::R16_UNorm: return WGPUTextureFormat_R16Unorm;
    case PixelFormat::R16_UInt: return WGPUTextureFormat_R16Uint;
    case PixelFormat::R16_SNorm: return WGPUTextureFormat_R16Snorm;
    case PixelFormat::R16_SInt: return WGPUTextureFormat_R16Sint;
    case PixelFormat::R8_Typeless:
    case PixelFormat::R8_UNorm: return WGPUTextureFormat_R8Unorm;
    case PixelFormat::R8_UInt: return WGPUTextureFormat_R8Uint;
    case PixelFormat::R8_SNorm: return WGPUTextureFormat_R8Snorm;
    case PixelFormat::R8_SInt: return WGPUTextureFormat_R8Sint;
    case PixelFormat::R9G9B9E5_SharedExp: return WGPUTextureFormat_RGB9E5Ufloat;
    case PixelFormat::BC1_Typeless:
    case PixelFormat::BC1_UNorm: return WGPUTextureFormat_BC1RGBAUnorm;
    case PixelFormat::BC1_UNorm_sRGB: return WGPUTextureFormat_BC1RGBAUnormSrgb;
    case PixelFormat::BC2_Typeless:
    case PixelFormat::BC2_UNorm: return WGPUTextureFormat_BC2RGBAUnorm;
    case PixelFormat::BC2_UNorm_sRGB: return WGPUTextureFormat_BC2RGBAUnormSrgb;
    case PixelFormat::BC3_Typeless:
    case PixelFormat::BC3_UNorm: return WGPUTextureFormat_BC3RGBAUnorm;
    case PixelFormat::BC3_UNorm_sRGB: return WGPUTextureFormat_BC3RGBAUnormSrgb;
    case PixelFormat::BC4_Typeless:
    case PixelFormat::BC4_UNorm: return WGPUTextureFormat_BC4RUnorm;
    case PixelFormat::BC4_SNorm: return WGPUTextureFormat_BC4RSnorm;
    case PixelFormat::BC5_Typeless:
    case PixelFormat::BC5_UNorm: return WGPUTextureFormat_BC5RGUnorm;
    case PixelFormat::BC5_SNorm: return WGPUTextureFormat_BC5RGSnorm;
    case PixelFormat::B8G8R8A8_Typeless:
    case PixelFormat::B8G8R8X8_Typeless:
    case PixelFormat::B8G8R8A8_UNorm:
    case PixelFormat::B8G8R8X8_UNorm: return WGPUTextureFormat_BGRA8Unorm;
    case PixelFormat::B8G8R8A8_UNorm_sRGB:
    case PixelFormat::B8G8R8X8_UNorm_sRGB: return WGPUTextureFormat_BGRA8UnormSrgb;
    case PixelFormat::BC6H_Typeless:
    case PixelFormat::BC6H_Uf16: return WGPUTextureFormat_BC6HRGBUfloat;
    case PixelFormat::BC6H_Sf16: return WGPUTextureFormat_BC6HRGBFloat;
    case PixelFormat::BC7_Typeless:
    case PixelFormat::BC7_UNorm: return WGPUTextureFormat_BC7RGBAUnorm;
    case PixelFormat::BC7_UNorm_sRGB: return WGPUTextureFormat_BC7RGBAUnormSrgb;
    case PixelFormat::ASTC_4x4_UNorm: return WGPUTextureFormat_ASTC4x4Unorm;
    case PixelFormat::ASTC_4x4_UNorm_sRGB: return WGPUTextureFormat_ASTC4x4UnormSrgb;
    case PixelFormat::ASTC_6x6_UNorm: return WGPUTextureFormat_ASTC6x6Unorm;
    case PixelFormat::ASTC_6x6_UNorm_sRGB: return WGPUTextureFormat_ASTC6x6UnormSrgb;
    case PixelFormat::ASTC_8x8_UNorm: return WGPUTextureFormat_ASTC8x8Unorm;
    case PixelFormat::ASTC_8x8_UNorm_sRGB: return WGPUTextureFormat_ASTC8x8UnormSrgb;
    case PixelFormat::ASTC_10x10_UNorm: return WGPUTextureFormat_ASTC10x10Unorm;
    case PixelFormat::ASTC_10x10_UNorm_sRGB: return WGPUTextureFormat_ASTC10x10UnormSrgb;
    default: return WGPUTextureFormat_Undefined;
        // @formatter:on
    }
}

PixelFormat RenderToolsWebGPU::ToPixelFormat(WGPUTextureFormat format)
{
    switch (format)
    {
        // @formatter:off
    case WGPUTextureFormat_R8Unorm: return PixelFormat::R8_UNorm;
    case WGPUTextureFormat_R8Snorm: return PixelFormat::R8_SNorm;
    case WGPUTextureFormat_R8Uint: return PixelFormat::R8_UInt;
    case WGPUTextureFormat_R8Sint: return PixelFormat::R8_SInt;
    case WGPUTextureFormat_R16Unorm: return PixelFormat::R16_UNorm;
    case WGPUTextureFormat_R16Snorm: return PixelFormat::R16_SNorm;
    case WGPUTextureFormat_R16Uint: return PixelFormat::R16_UInt;
    case WGPUTextureFormat_R16Sint: return PixelFormat::R16_SInt;
    case WGPUTextureFormat_R16Float: return PixelFormat::R16_Float;
    case WGPUTextureFormat_R32Float: return PixelFormat::R32_Float;
    case WGPUTextureFormat_R32Uint: return PixelFormat::R32_UInt;
    case WGPUTextureFormat_R32Sint: return PixelFormat::R32_SInt;
    case WGPUTextureFormat_RG16Unorm: return PixelFormat::R16G16_UNorm;
    case WGPUTextureFormat_RG16Snorm: return PixelFormat::R16G16_SNorm;
    case WGPUTextureFormat_RG16Uint: return PixelFormat::R16G16_UInt;
    case WGPUTextureFormat_RG16Sint: return PixelFormat::R16G16_SInt;
    case WGPUTextureFormat_RG16Float: return PixelFormat::R16G16_Float;
    case WGPUTextureFormat_RGBA8Unorm: return PixelFormat::R8G8B8A8_UNorm;
    case WGPUTextureFormat_RGBA8UnormSrgb: return PixelFormat::R8G8B8A8_UNorm_sRGB;
    case WGPUTextureFormat_RGBA8Snorm: return PixelFormat::R8G8B8A8_SNorm;
    case WGPUTextureFormat_RGBA8Uint: return PixelFormat::R8G8B8A8_UInt;
    case WGPUTextureFormat_RGBA8Sint: return PixelFormat::R8G8B8A8_SInt;
    case WGPUTextureFormat_BGRA8Unorm: return PixelFormat::B8G8R8A8_UNorm;
    case WGPUTextureFormat_BGRA8UnormSrgb: return PixelFormat::B8G8R8A8_UNorm_sRGB;
    case WGPUTextureFormat_RGB10A2Uint: return PixelFormat::R10G10B10A2_UInt;
    case WGPUTextureFormat_RGB10A2Unorm: return PixelFormat::R10G10B10A2_UNorm;
    case WGPUTextureFormat_RG11B10Ufloat: return PixelFormat::R11G11B10_Float;
    case WGPUTextureFormat_RGB9E5Ufloat: return PixelFormat::R9G9B9E5_SharedExp;
    case WGPUTextureFormat_RG32Float: return PixelFormat::R32G32_Float;
    case WGPUTextureFormat_RG32Uint: return PixelFormat::R32G32_UInt;
    case WGPUTextureFormat_RG32Sint: return PixelFormat::R32G32_SInt;
    case WGPUTextureFormat_RGBA16Unorm: return PixelFormat::R16G16B16A16_UNorm;
    case WGPUTextureFormat_RGBA16Snorm: return PixelFormat::R16G16B16A16_SNorm;
    case WGPUTextureFormat_RGBA16Uint: return PixelFormat::R16G16B16A16_UInt;
    case WGPUTextureFormat_RGBA16Sint: return PixelFormat::R16G16B16A16_SInt;
    case WGPUTextureFormat_RGBA16Float: return PixelFormat::R16G16B16A16_Float;
    case WGPUTextureFormat_RGBA32Float: return PixelFormat::R32G32B32A32_Float;
    case WGPUTextureFormat_RGBA32Uint: return PixelFormat::R32G32B32A32_UInt;
    case WGPUTextureFormat_RGBA32Sint: return PixelFormat::R32G32B32A32_SInt;
    case WGPUTextureFormat_Depth16Unorm: return PixelFormat::D16_UNorm;
    case WGPUTextureFormat_Depth24Plus:
    case WGPUTextureFormat_Depth24PlusStencil8: return PixelFormat::D24_UNorm_S8_UInt;
    case WGPUTextureFormat_Depth32Float: return PixelFormat::D32_Float;
    case WGPUTextureFormat_Depth32FloatStencil8: return PixelFormat::D32_Float_S8X24_UInt;
    case WGPUTextureFormat_BC1RGBAUnorm: return PixelFormat::BC1_UNorm;
    case WGPUTextureFormat_BC1RGBAUnormSrgb: return PixelFormat::BC1_UNorm_sRGB;
    case WGPUTextureFormat_BC2RGBAUnorm: return PixelFormat::BC2_UNorm;
    case WGPUTextureFormat_BC2RGBAUnormSrgb: return PixelFormat::BC2_UNorm_sRGB;
    case WGPUTextureFormat_BC3RGBAUnorm: return PixelFormat::BC3_UNorm;
    case WGPUTextureFormat_BC3RGBAUnormSrgb: return PixelFormat::BC3_UNorm_sRGB;
    case WGPUTextureFormat_BC4RUnorm: return PixelFormat::BC4_UNorm;
    case WGPUTextureFormat_BC4RSnorm: return PixelFormat::BC4_SNorm;
    case WGPUTextureFormat_BC5RGUnorm: return PixelFormat::BC5_UNorm;
    case WGPUTextureFormat_BC5RGSnorm: return PixelFormat::BC5_SNorm;
    case WGPUTextureFormat_BC6HRGBUfloat: return PixelFormat::BC6H_Uf16;
    case WGPUTextureFormat_BC6HRGBFloat: return PixelFormat::BC6H_Sf16;
    case WGPUTextureFormat_BC7RGBAUnorm: return PixelFormat::BC7_UNorm;
    case WGPUTextureFormat_BC7RGBAUnormSrgb: return PixelFormat::BC7_UNorm_sRGB;
    case WGPUTextureFormat_ASTC4x4Unorm: return PixelFormat::ASTC_4x4_UNorm;
    case WGPUTextureFormat_ASTC4x4UnormSrgb: return PixelFormat::ASTC_4x4_UNorm_sRGB;
    case WGPUTextureFormat_ASTC6x6Unorm: return PixelFormat::ASTC_6x6_UNorm;
    case WGPUTextureFormat_ASTC6x6UnormSrgb: return PixelFormat::ASTC_6x6_UNorm_sRGB;
    case WGPUTextureFormat_ASTC8x8Unorm: return PixelFormat::ASTC_8x8_UNorm;
    case WGPUTextureFormat_ASTC8x8UnormSrgb: return PixelFormat::ASTC_8x8_UNorm_sRGB;
    case WGPUTextureFormat_ASTC10x10Unorm: return PixelFormat::ASTC_10x10_UNorm;
    case WGPUTextureFormat_ASTC10x10UnormSrgb: return PixelFormat::ASTC_10x10_UNorm_sRGB;
    default: return PixelFormat::Unknown;
        // @formatter:on
    }
}

PixelFormat RenderToolsWebGPU::ToPixelFormat(WGPUVertexFormat format)
{
    switch (format)
    {
        // @formatter:off
    case WGPUVertexFormat_Uint8: return PixelFormat::R8_UInt;
    case WGPUVertexFormat_Uint8x2: return PixelFormat::R8G8_UInt;
    case WGPUVertexFormat_Uint8x4: return PixelFormat::R8G8B8A8_UInt;
    case WGPUVertexFormat_Sint8: return PixelFormat::R8_SInt;
    case WGPUVertexFormat_Sint8x2: return PixelFormat::R8G8_SInt;
    case WGPUVertexFormat_Sint8x4: return PixelFormat::R8G8B8A8_SInt;
    case WGPUVertexFormat_Unorm8: return PixelFormat::R8_UNorm;
    case WGPUVertexFormat_Unorm8x2: return PixelFormat::R8G8_UNorm;
    case WGPUVertexFormat_Unorm8x4: return PixelFormat::R8G8B8A8_UNorm;
    case WGPUVertexFormat_Snorm8: return PixelFormat::R8_SNorm;
    case WGPUVertexFormat_Snorm8x2: return PixelFormat::R8G8_SNorm;
    case WGPUVertexFormat_Snorm8x4: return PixelFormat::R8G8B8A8_SNorm;
    case WGPUVertexFormat_Uint16: return PixelFormat::R16_UInt;
    case WGPUVertexFormat_Uint16x2: return PixelFormat::R16G16_UInt;
    case WGPUVertexFormat_Uint16x4: return PixelFormat::R16G16B16A16_UInt;
    case WGPUVertexFormat_Sint16: return PixelFormat::R16_SInt;
    case WGPUVertexFormat_Sint16x2: return PixelFormat::R16G16_SInt;
    case WGPUVertexFormat_Sint16x4: return PixelFormat::R16G16B16A16_SInt;
    case WGPUVertexFormat_Unorm16: return PixelFormat::R16_UNorm;
    case WGPUVertexFormat_Unorm16x2: return PixelFormat::R16G16_UNorm;
    case WGPUVertexFormat_Unorm16x4: return PixelFormat::R16G16B16A16_UNorm;
    case WGPUVertexFormat_Snorm16: return PixelFormat::R16_SNorm;
    case WGPUVertexFormat_Snorm16x2: return PixelFormat::R16G16_SNorm;
    case WGPUVertexFormat_Snorm16x4: return PixelFormat::R16G16B16A16_SNorm;
    case WGPUVertexFormat_Float16: return PixelFormat::R16_Float;
    case WGPUVertexFormat_Float16x2: return PixelFormat::R16G16_Float;
    case WGPUVertexFormat_Float16x4: return PixelFormat::R16G16B16A16_Float;
    case WGPUVertexFormat_Float32: return PixelFormat::R32_Float;
    case WGPUVertexFormat_Float32x2: return PixelFormat::R32G32_Float;
    case WGPUVertexFormat_Float32x3: return PixelFormat::R32G32B32_Float;
    case WGPUVertexFormat_Float32x4: return PixelFormat::R32G32B32A32_Float;
    case WGPUVertexFormat_Uint32: return PixelFormat::R32_UInt;
    case WGPUVertexFormat_Uint32x2: return PixelFormat::R32G32_UInt;
    case WGPUVertexFormat_Uint32x3: return PixelFormat::R32G32B32_UInt;
    case WGPUVertexFormat_Uint32x4: return PixelFormat::R32G32B32A32_UInt;
    case WGPUVertexFormat_Sint32: return PixelFormat::R32_SInt;
    case WGPUVertexFormat_Sint32x2: return PixelFormat::R32G32_SInt;
    case WGPUVertexFormat_Sint32x3: return PixelFormat::R32G32B32_SInt;
    case WGPUVertexFormat_Sint32x4: return PixelFormat::R32G32B32A32_SInt;
    case WGPUVertexFormat_Unorm10_10_10_2: return PixelFormat::R10G10B10A2_UNorm;
    case WGPUVertexFormat_Unorm8x4BGRA: return PixelFormat::B8G8R8A8_UNorm;
    default: return PixelFormat::Unknown;
        // @formatter:on
    }
}

#endif
