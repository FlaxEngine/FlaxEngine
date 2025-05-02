// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_TEXTURE_TOOL && COMPILE_WITH_ASTC

#include "TextureTool.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include <ThirdParty/astc/astcenc.h>

bool TextureTool::ConvertAstc(TextureData& dst, const TextureData& src, const PixelFormat dstFormat)
{
    PROFILE_CPU();
    ASSERT(PixelFormatExtensions::IsCompressedASTC(dstFormat));
    const int32 blockSize = PixelFormatExtensions::ComputeBlockSize(dstFormat);
    const int32 bytesPerBlock = 16; // All ASTC blocks use 128 bits

    // Configure the compressor run
    const bool isSRGB = PixelFormatExtensions::IsSRGB(dstFormat);
    const bool isHDR = PixelFormatExtensions::IsHDR(src.Format);
    astcenc_profile astcProfile = isHDR ? ASTCENC_PRF_HDR_RGB_LDR_A : (isSRGB ? ASTCENC_PRF_LDR_SRGB : ASTCENC_PRF_LDR);
    float astcQuality = ASTCENC_PRE_MEDIUM;
    unsigned int astcFlags = 0; // TODO: add custom flags support for converter to handle ASTCENC_FLG_MAP_NORMAL
    astcenc_config astcConfig;
	astcenc_error astcError = astcenc_config_init(astcProfile, blockSize, blockSize, 1, astcQuality, astcFlags, &astcConfig);
    if (astcError != ASTCENC_SUCCESS)
    {
        LOG(Warning, "Cannot compress image. ASTC failed with error: {}", String(astcenc_get_error_string(astcError)));
        return true;
    }
	astcenc_swizzle astcSwizzle = { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };
    if (!PixelFormatExtensions::HasAlpha(src.Format))
    {
        // No alpha channel in use so fill with 1
        astcSwizzle.a = ASTCENC_SWZ_1;
    }

    // Allocate working state given config and thread_count
    astcenc_context* astcContext;
    astcError = astcenc_context_alloc(&astcConfig, 1, &astcContext);
    if (astcError != ASTCENC_SUCCESS)
    {
        LOG(Warning, "Cannot compress image. ASTC failed with error: {}", String(astcenc_get_error_string(astcError)));
        return true;
    }
    TextureData const* textureData = &src;
    TextureData converted;

    // Encoder uses full 4-component RGBA input image so convert it if needed
    if (PixelFormatExtensions::ComputeComponentsCount(src.Format) != 4 || 
        PixelFormatExtensions::IsCompressed(textureData->Format) ||
        !PixelFormatExtensions::IsRgbAOrder(textureData->Format))
    {
        if (textureData != &src)
            converted = src;
        const PixelFormat tempFormat = isHDR ? PixelFormat::R16G16B16A16_Float : (PixelFormatExtensions::IsSRGB(src.Format) ? PixelFormat::R8G8B8A8_UNorm_sRGB : PixelFormat::R8G8B8A8_UNorm);
        if (!TextureTool::Convert(converted, *textureData, tempFormat))
            textureData = &converted;
    }

    // When converting from non-sRGB to sRGB we need to change the color-space manually (otherwise image is dark)
    if (PixelFormatExtensions::IsSRGB(src.Format) != isSRGB)
    {
        if (textureData != &src)
            converted = src;
        Function<void(Color&)> transform = [](Color& c)
        {
            c = Color::LinearToSrgb(c);
        };
        if (!TextureTool::Transform(converted, transform))
            textureData = &converted;
    }

    // Setup output
    dst.Items.Resize(textureData->Items.Count());
    dst.Width = textureData->Width;
    dst.Height = textureData->Height;
    dst.Depth = 1;
    dst.Format = dstFormat;

    // Compress all array slices
    for (int32 arrayIndex = 0; arrayIndex < textureData->Items.Count() && astcError == ASTCENC_SUCCESS; arrayIndex++)
    {
        const auto& srcSlice = textureData->Items[arrayIndex];
        auto& dstSlice = dst.Items[arrayIndex];
        auto mipLevels = srcSlice.Mips.Count();
        dstSlice.Mips.Resize(mipLevels, false);

        // Compress all mip levels
        for (int32 mipIndex = 0; mipIndex < mipLevels && astcError == ASTCENC_SUCCESS; mipIndex++)
        {
            const auto& srcMip = srcSlice.Mips[mipIndex];
            auto& dstMip = dstSlice.Mips[mipIndex];
            auto mipWidth = Math::Max(textureData->Width >> mipIndex, 1);
            auto mipHeight = Math::Max(textureData->Height >> mipIndex, 1);
            auto blocksWidth = Math::Max(Math::DivideAndRoundUp(mipWidth, blockSize), 1);
            auto blocksHeight = Math::Max(Math::DivideAndRoundUp(mipHeight, blockSize), 1);
            uint32 mipRowPitch, mipSlicePitch;
            RenderTools::ComputePitch(textureData->Format, mipWidth, mipHeight, mipRowPitch, mipSlicePitch);
            ASSERT(srcMip.RowPitch == mipRowPitch);
            ASSERT(srcMip.DepthPitch == mipSlicePitch);
            ASSERT(srcMip.Lines == mipHeight);

            // Allocate memory
            dstMip.RowPitch = blocksWidth * bytesPerBlock;
            dstMip.DepthPitch = dstMip.RowPitch * blocksHeight;
            dstMip.Lines = blocksHeight;
            dstMip.Data.Allocate(dstMip.DepthPitch);

            // Compress image
            astcenc_image astcInput;
            astcInput.dim_x = mipWidth;
            astcInput.dim_y = mipHeight;
            astcInput.dim_z = 1;
            astcInput.data_type = isHDR ? ASTCENC_TYPE_F16 : ASTCENC_TYPE_U8;
            void* srcData = (void*)srcMip.Data.Get();
            astcInput.data = &srcData;
            astcError = astcenc_compress_image(astcContext, &astcInput, &astcSwizzle, dstMip.Data.Get(), dstMip.Data.Length(), 0);
            if (astcError == ASTCENC_SUCCESS)
                astcError = astcenc_compress_reset(astcContext);
        }
    }

    // Clean up
    if (astcError != ASTCENC_SUCCESS)
    {
        LOG(Warning, "Cannot compress image. ASTC failed with error: {}", String(astcenc_get_error_string(astcError)));
        return true;
    }
    astcenc_context_free(astcContext);
    return astcError != ASTCENC_SUCCESS;
}

#endif
