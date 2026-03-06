// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_TEXTURE_TOOL && COMPILE_WITH_BASISU

#include "TextureTool.h"
#include "Engine/Core/Log.h"
#include "Engine/Scripting/Enums.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Platform/CPUInfo.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerMemory.h"
#include <ThirdParty/basis_universal/transcoder/basisu_transcoder.h>
#if USE_EDITOR
#include <ThirdParty/basis_universal/encoder/basisu_comp.h>
#endif

bool TextureTool::ConvertBasisUniversal(TextureData& dst, const TextureData& src)
{
#if USE_EDITOR
    PROFILE_CPU();
    static bool Init = true;
    if (Init)
    {
        Init = false;
        basisu::basisu_encoder_init();
        LOG(Info, "Compressing with Basis Universal " BASISU_LIB_VERSION_STRING);
    }

    // Decompress/convert source image
    TextureData const* textureData = &src;
    TextureData converted;
    const PixelFormat inputFormat = PixelFormatExtensions::IsHDR(src.Format) ? PixelFormat::R32G32B32A32_Float : (PixelFormatExtensions::IsSRGB(src.Format) ? PixelFormat::R8G8B8A8_UNorm_sRGB : PixelFormat::R8G8B8A8_UNorm);
    if (src.Format != inputFormat)
    {
        if (textureData != &src)
            converted = src;
        if (!TextureTool::Convert(converted, *textureData, inputFormat))
            textureData = &converted;
    }

    // Initialize the compressor parameters
    basisu::basis_compressor_params comp_params;
    basist::basis_tex_format mode;
    if (PixelFormatExtensions::IsHDR(src.Format))
    {
        comp_params.m_source_images_hdr.resize(1);
        mode = basist::basis_tex_format::cUASTC_HDR_4x4;
    }
    else
    {
        comp_params.m_source_images.resize(1);
        switch (src.Format)
        {
        case PixelFormat::ASTC_4x4_UNorm:
        case PixelFormat::ASTC_4x4_UNorm_sRGB:
            mode = basist::basis_tex_format::cXUASTC_LDR_4x4;
            break;
        case PixelFormat::ASTC_6x6_UNorm:
        case PixelFormat::ASTC_6x6_UNorm_sRGB:
            mode = basist::basis_tex_format::cXUASTC_LDR_6x6;
            break;
        case PixelFormat::ASTC_8x8_UNorm:
        case PixelFormat::ASTC_8x8_UNorm_sRGB:
            mode = basist::basis_tex_format::cXUASTC_LDR_8x8;
            break;
        case PixelFormat::ASTC_10x10_UNorm:
        case PixelFormat::ASTC_10x10_UNorm_sRGB:
            mode = basist::basis_tex_format::cXUASTC_LDR_10x10;
            break;
        default:
            mode = basist::basis_tex_format::cXUASTC_LDR_4x4;
            break;
        }
    }
    comp_params.set_format_mode_and_quality_effort(mode, 80); // TODO: expose quality somehow
    comp_params.set_srgb_options(PixelFormatExtensions::IsSRGB(src.Format));
    if (textureData->Depth > 1)
        comp_params.m_tex_type = basist::cBASISTexTypeVolume;
    else
        comp_params.m_tex_type = basist::cBASISTexType2D;
    comp_params.m_use_opencl = false;
    comp_params.m_write_output_basis_or_ktx2_files = false;
    comp_params.m_create_ktx2_file = false;
#if !BASISD_SUPPORT_KTX2_ZSTD
    comp_params.m_xuastc_ldr_syntax = (int)basist::astc_ldr_t::xuastc_ldr_syntax::cFullArith;
#endif
    comp_params.m_print_stats = false;
    comp_params.m_compute_stats = false;
    comp_params.m_status_output = false;
#if 0 // Enable LinkAsConsoleProgram in FlaxEditor.Build.cs to see debug output in console
    basisu::enable_debug_printf(true);
    comp_params.m_debug = true;
#endif
#if 0 // Debug exporting of the images
    comp_params.m_debug_images = true;
    TextureTool::ExportTexture(TEXT("textureData.png"), *textureData);
    TextureTool::ExportTexture(TEXT("src.png"), src);
#endif

    // Initialize a job pool
    uint32_t num_threads = Math::Min(8u, Platform::GetCPUInfo().ProcessorCoreCount); // Incl. calling thread
    basisu::job_pool job_pool(num_threads);
    comp_params.m_pJob_pool = &job_pool;
    comp_params.m_multithreading = num_threads > 1;

    // Setup output
    dst.Items.Resize(textureData->Items.Count());
    dst.Width = textureData->Width;
    dst.Height = textureData->Height;
    dst.Depth = textureData->Depth;
    dst.Format = PixelFormat::Basis;

    // Compress all array slices
    for (int32 arrayIndex = 0; arrayIndex < textureData->Items.Count(); arrayIndex++)
    {
        const auto& srcSlice = textureData->Items[arrayIndex];
        auto& dstSlice = dst.Items[arrayIndex];
        auto mipLevels = srcSlice.Mips.Count();
        dstSlice.Mips.Resize(mipLevels, false);

        // Compress all mip levels
        for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
        {
            const auto& srcMip = srcSlice.Mips[mipIndex];
            auto& dstMip = dstSlice.Mips[mipIndex];
            auto mipWidth = Math::Max(textureData->Width >> mipIndex, 1);
            auto mipHeight = Math::Max(textureData->Height >> mipIndex, 1);

            // Provide source texture data
            if (PixelFormatExtensions::IsHDR(textureData->Format))
            {
                ASSERT(textureData->Format == PixelFormat::R32G32B32A32_Float);
                auto& img = comp_params.m_source_images_hdr[0];
                img.crop(mipWidth, mipHeight, mipWidth * sizeof(basisu::vec4F));
                for (int32 y = 0; y < mipHeight; y++)
                    Platform::MemoryCopy(&img.get_ptr()[y * img.get_pitch()], &srcMip.Get<Color>(0, y), img.get_pitch());
            }
            else
            {
                ASSERT(textureData->Format == PixelFormat::R8G8B8A8_UNorm || textureData->Format == PixelFormat::R8G8B8A8_UNorm_sRGB);
                ASSERT(srcMip.RowPitch == mipWidth * sizeof(uint32));
                ASSERT(srcMip.Lines == mipHeight);
                ASSERT(srcMip.DepthPitch == mipWidth * mipHeight * sizeof(uint32));
                auto& img = comp_params.m_source_images[0];
                img.init(srcMip.Data.Get(), mipWidth, mipHeight, 4);
                if (src.Format == PixelFormat::BC5_Typeless || src.Format == PixelFormat::BC5_UNorm || src.Format == PixelFormat::R16G16_UNorm)
                {
                    // BC5 blocks are exported from Basis using Red and Alpha channels while engine uses Red and Green channels
                    for (int32_t y = 0; y < mipHeight; y++)
                    {
                        for (int32_t x = 0; x < mipWidth; x++)
                        {
                            basisu::color_rgba& pixel = img(x, y);
                            pixel.a = pixel.g;
                        }
                    }
                }
            }

            // Compress image
            basisu::basis_compressor comp;
            {
                PROFILE_CPU_NAMED("basis_compressor");
                if (!comp.init(comp_params))
                {
                    LOG(Error, "basis_compressor::init() failed");
                    return true;
                }
                basisu::basis_compressor::error_code error = comp.process();
                if (error != basisu::basis_compressor::cECSuccess)
                {
                    LOG(Error, "basis_compressor::process() failed with error code {}", (uint32)error);
                    return true;
                }
            }

            // Copy output data
            {
                PROFILE_CPU_NAMED("Copy");
                const basisu::uint8_vec& basisFile = comp.get_output_basis_file();
                dstMip.DepthPitch = dstMip.RowPitch = (int32)basisFile.size();
                dstMip.Lines = 1;
                dstMip.Data.Copy(basisFile.data(), (int32)basisFile.size());
            }
        }
    }

    return false;
#else
    LOG(Error, "Building Basis Universal textures is not supported on this platform.");
    return true;
#endif
}

bool TextureTool::UpdateTextureBasisUniversal(GPUContext* context, GPUTexture* texture, int32 arrayIndex, int32 mipIndex, Span<byte> data, uint32 rowPitch, uint32 slicePitch, PixelFormat dataFormat)
{
    PROFILE_CPU_NAMED("Basis Universal");
    PROFILE_MEM(GraphicsTextures);
    static bool Init = true;
    if (Init)
    {
        Init = false;
        basist::basisu_transcoder_init();
    }

    // Open Basis file
    basist::basisu_transcoder transcoder;
    basist::basisu_file_info fileInfo;
    basist::basisu_image_info imageInfo;
    if (!transcoder.get_file_info(data.Get(), data.Length(), fileInfo) || !transcoder.get_image_info(data.Get(), data.Length(), imageInfo, 0))
    {
        LOG(Error, "Failed to read Basis file");
        return true;
    }
    if (!transcoder.start_transcoding(data.Get(), data.Length()))
    {
        LOG(Error, "Failed to start transcoding Basis file");
        return true;
    }
    const int32 mipWidth = Math::Max(texture->Width() >> mipIndex, 1);
    const int32 mipHeight = Math::Max(texture->Height() >> mipIndex, 1);
    if (mipWidth != imageInfo.m_orig_width || mipHeight != imageInfo.m_orig_height)
    {
        LOG(Error, "Failed to transcode Basis file of size {}x{} into texture of size {}x{}", imageInfo.m_orig_width, imageInfo.m_orig_height, texture->Width(), texture->Height());
        return true;
    }
    basist::transcoder_texture_format transcoder_tex_fmt;
    PixelFormat textureFormat = texture->Format();
    switch (textureFormat)
    {
    case PixelFormat::BC1_Typeless:
    case PixelFormat::BC1_UNorm:
    case PixelFormat::BC1_UNorm_sRGB:
        transcoder_tex_fmt = basist::transcoder_texture_format::cTFBC1_RGB;
        break;
    case PixelFormat::BC3_Typeless:
    case PixelFormat::BC3_UNorm:
    case PixelFormat::BC3_UNorm_sRGB:
        transcoder_tex_fmt = basist::transcoder_texture_format::cTFBC3_RGBA;
        break;
    case PixelFormat::BC4_Typeless:
    case PixelFormat::BC4_UNorm:
        transcoder_tex_fmt = basist::transcoder_texture_format::cTFBC4_R;
        break;
    case PixelFormat::BC5_Typeless:
    case PixelFormat::BC5_UNorm:
        transcoder_tex_fmt = basist::transcoder_texture_format::cTFBC5_RG;
        break;
    case PixelFormat::BC6H_Typeless:
    case PixelFormat::BC6H_Uf16:
        transcoder_tex_fmt = basist::transcoder_texture_format::cTFBC6H;
        break;
    case PixelFormat::BC7_Typeless:
    case PixelFormat::BC7_UNorm:
    case PixelFormat::BC7_UNorm_sRGB:
        transcoder_tex_fmt = basist::transcoder_texture_format::cTFBC7_RGBA;
        break;
    case PixelFormat::ASTC_4x4_UNorm:
    case PixelFormat::ASTC_4x4_UNorm_sRGB:
        transcoder_tex_fmt = basist::transcoder_texture_format::cTFASTC_LDR_4x4_RGBA;
        break;
    case PixelFormat::ASTC_6x6_UNorm:
    case PixelFormat::ASTC_6x6_UNorm_sRGB:
        transcoder_tex_fmt = basist::transcoder_texture_format::cTFASTC_LDR_6x6_RGBA;
        break;
    case PixelFormat::ASTC_8x8_UNorm:
    case PixelFormat::ASTC_8x8_UNorm_sRGB:
        transcoder_tex_fmt = basist::transcoder_texture_format::cTFASTC_LDR_8x8_RGBA;
        break;
    case PixelFormat::ASTC_10x10_UNorm:
    case PixelFormat::ASTC_10x10_UNorm_sRGB:
        transcoder_tex_fmt = basist::transcoder_texture_format::cTFASTC_LDR_10x10_RGBA;
        break;
    case PixelFormat::R8G8B8A8_UNorm:
    case PixelFormat::R8G8B8A8_UNorm_sRGB:
        transcoder_tex_fmt = basist::transcoder_texture_format::cTFRGBA32;
        break;
    case PixelFormat::B5G6R5_UNorm:
        transcoder_tex_fmt = basist::transcoder_texture_format::cTFBGR565;
        break;
    case PixelFormat::R16G16B16A16_Typeless:
    case PixelFormat::R16G16B16A16_Float:
        transcoder_tex_fmt = basist::transcoder_texture_format::cTFRGBA_HALF;
        break;
    case PixelFormat::R9G9B9E5_SharedExp:
        transcoder_tex_fmt = basist::transcoder_texture_format::cTFRGB_9E5;
        break;
    default:
        LOG(Error, "Unsupported texture format {} to transcode Basis from {}", ScriptingEnum::ToString(textureFormat), String(basist::basis_get_tex_format_name(fileInfo.m_tex_format)));
        return true;
    }
    if (!basist::basis_is_format_supported(transcoder_tex_fmt, fileInfo.m_tex_format))
    {
        LOG(Error, "Basis library doesn't support transcoding texture format {} into {}", String(basist::basis_get_tex_format_name(fileInfo.m_tex_format)), String(basist::basis_get_format_name(transcoder_tex_fmt)));
        return true;
    }

    // Allocate memory
    // TODO: use some transient frame allocator for that
    Array<byte> transcodedData;
    const bool isBlockUncompressed = basist::basis_transcoder_format_is_uncompressed(transcoder_tex_fmt);
    uint32 transcodedSize = basist::basis_compute_transcoded_image_size_in_bytes(transcoder_tex_fmt, imageInfo.m_orig_width, imageInfo.m_orig_height);
    uint32 transcadedBlocksOrPixels = isBlockUncompressed ? imageInfo.m_orig_width * imageInfo.m_orig_height : imageInfo.m_total_blocks;
    transcodedData.Resize(transcodedSize);

    // Transcode to the GPU format
    uint32 decodeFlags = 0; // basist::basisu_decode_flags
    if (!transcoder.transcode_image_level(data.Get(), data.Length(), 0, 0, transcodedData.Get(), transcadedBlocksOrPixels, transcoder_tex_fmt, decodeFlags))
    {
        LOG(Error, "Failed to transcode Basis image of size {}x{} from format {} to {} ({})", imageInfo.m_orig_width, imageInfo.m_orig_height, String(basist::basis_get_tex_format_name(fileInfo.m_tex_format)), String(basist::basis_get_format_name(transcoder_tex_fmt)), ScriptingEnum::ToString(textureFormat));
        return false;
    }

    // Send data to the GPU
    slicePitch = transcodedData.Count();
    rowPitch = slicePitch / (isBlockUncompressed ? imageInfo.m_orig_height : imageInfo.m_num_blocks_y);
    context->UpdateTexture(texture, arrayIndex, mipIndex, transcodedData.Get(), rowPitch, slicePitch);
    return false;
}

#endif
