// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_TEXTURE_TOOL && COMPILE_WITH_STB

#include "TextureTool.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Serialization/FileWriteStream.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Platform/File.h"

#define STBI_ASSERT(x) ASSERT(x)
#define STBI_MALLOC(sz) Allocator::Allocate(sz)
#define STBI_REALLOC(p, newsz) AllocatorExt::Realloc(p, newsz)
#define STBI_REALLOC_SIZED(p, oldsz, newsz) AllocatorExt::Realloc(p, oldsz, newsz)
#define STBI_FREE(p) Allocator::Free(p)
#define STBI_NO_PSD
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_FAILURE_STRINGS
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include <ThirdParty/stb/stb_image.h>

#define STBIW_ASSERT(x) ASSERT(x)
#define STBIW_MALLOC(sz) Allocator::Allocate(sz)
#define STBIW_REALLOC(p, newsz) AllocatorExt::Realloc(p, newsz)
#define STBIW_REALLOC_SIZED(p, oldsz, newsz) AllocatorExt::Realloc(p, oldsz, newsz)
#define STBIW_FREE(p) Allocator::Free(p)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <ThirdParty/stb/stb_image_write.h>

#define STBIR_ASSERT(x) ASSERT(x)
#define STBIR_MALLOC(sz, c) Allocator::Allocate(sz)
#define STBIR_FREE(p, c) Allocator::Free(p)
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <ThirdParty/stb/stb_image_resize.h>

static void stbWrite(void* context, void* data, int size)
{
    auto file = (FileWriteStream*)context;
    file->WriteBytes(data, (uint32)size);
}

bool TextureTool::ExportTextureStb(ImageType type, const StringView& path, const TextureData& textureData)
{
    if (textureData.GetArraySize() != 1)
    {
        LOG(Warning, "Exporting texture arrays and cubemaps is not supported by stb library.");
    }

    // Convert into RGBA8
    const auto sampler = GetSampler(textureData.Format);
    if (sampler == nullptr)
    {
        LOG(Warning, "Texture data format {0} is not supported by stb library.", (int32)textureData.Format);
        return true;
    }
    const auto srcData = textureData.GetData(0, 0);
    const int comp = 4;
    Array<byte> data;
    bool sRGB = PixelFormatExtensions::IsSRGB(textureData.Format);
    if (type == ImageType::HDR)
    {
        data.Resize(sizeof(float) * comp * textureData.Width * textureData.Height);

        auto ptr = (Vector4*)data.Get();
        for (int32 y = 0; y < textureData.Height; y++)
        {
            for (int32 x = 0; x < textureData.Width; x++)
            {
                Color color = SamplePoint(sampler, x, y, srcData->Data.Get(), srcData->RowPitch);
                if (sRGB)
                    color = Color::SrgbToLinear(color);
                *(ptr + x + y * textureData.Width) = color.ToVector4();
            }
        }
    }
    else
    {
        data.Resize(sizeof(Color32) * comp * textureData.Width * textureData.Height);

        auto ptr = (Color32*)data.Get();
        for (int32 y = 0; y < textureData.Height; y++)
        {
            for (int32 x = 0; x < textureData.Width; x++)
            {
                Color color = SamplePoint(sampler, x, y, srcData->Data.Get(), srcData->RowPitch);
                if (sRGB)
                    color = Color::SrgbToLinear(color);
                *(ptr + x + y * textureData.Width) = Color32(color);
            }
        }
    }

    const auto file = FileWriteStream::Open(path);
    if (!file)
    {
        LOG(Warning, "Failed to open file.");
        return true;
    }

    stbi__write_context s;
    s.func = stbWrite;
    s.context = file;

    int32 result = 99;
    switch (type)
    {
    case ImageType::BMP:
        result = stbi_write_bmp_core(&s, textureData.Width, textureData.Height, comp, data.Get());
        break;
    case ImageType::JPEG:
        result = stbi_write_jpg_core(&s, textureData.Width, textureData.Height, comp, data.Get(), 90);
        break;
    case ImageType::TGA:
        result = stbi_write_tga_core(&s, textureData.Width, textureData.Height, comp, data.Get());
        break;
    case ImageType::HDR:
        result = stbi_write_hdr_core(&s, textureData.Width, textureData.Height, comp, (float*)data.Get());
        break;
    case ImageType::PNG:
    {
        int32 ptrSize = 0;
        const auto ptr = stbi_write_png_to_mem(data.Get(), 0, textureData.Width, textureData.Height, comp, &ptrSize);
        if (ptr)
        {
            file->WriteBytes(ptr, ptrSize);
            result = 0;
        }
        else
        {
            result = 99;
        }
        break;
    }
    case ImageType::GIF:
        LOG(Warning, "GIF format is not supported by stb library.");
        break;
    case ImageType::TIFF:
        LOG(Warning, "GIF format is not supported by stb library.");
        break;
    case ImageType::DDS:
        LOG(Warning, "DDS format is not supported by stb library.");
        break;
    case ImageType::RAW:
        LOG(Warning, "RAW format is not supported by stb library.");
        break;
    default:
        LOG(Warning, "Unknown format.");
        break;
    }

    if (result != 0)
    {
        LOG(Warning, "Saving texture failed. Error from stb library: {0}", result);
    }

    file->Close();
    Delete(file);

    return result != 0;
}

bool TextureTool::ImportTextureStb(ImageType type, const StringView& path, TextureData& textureData, bool& hasAlpha)
{
    Array<byte> fileData;
    if (File::ReadAllBytes(path, fileData))
    {
        LOG(Warning, "Failed to read data from file.");
        return true;
    }

    switch (type)
    {
    case ImageType::PNG:
    case ImageType::BMP:
    case ImageType::GIF:
    case ImageType::JPEG:
    case ImageType::HDR:
    case ImageType::TGA:
    {
        int width, height, components;
        stbi_uc* stbData = stbi_load_from_memory(fileData.Get(), fileData.Count(), &width, &height, &components, 4);
        if (!stbData)
        {
            LOG(Warning, "Failed to load image.");
            return false;
        }

        // Setup texture data
        textureData.Width = width;
        textureData.Height = height;
        textureData.Depth = 1;
        textureData.Format = PixelFormat::R8G8B8A8_UNorm;
        textureData.Items.Resize(1);
        textureData.Items[0].Mips.Resize(1);
        auto& mip = textureData.Items[0].Mips[0];
        mip.RowPitch = sizeof(Color32) * width;
        mip.DepthPitch = mip.RowPitch * height;
        mip.Lines = height;
        mip.Data.Copy(stbData, mip.DepthPitch);

#if USE_EDITOR
        // Detect alpha channel usage
        auto ptrAlpha = (Color32*)mip.Data.Get();
        for (int32 y = 0; y < height && !hasAlpha; y++)
        {
            for (int32 x = 0; x < width && !hasAlpha; x++)
            {
                hasAlpha |= ptrAlpha->A < 255;
                ptrAlpha++;
            }
        }
#endif

        stbi_image_free(stbData);

        break;
    }
    case ImageType::RAW:
    {
        // Assume 16-bit, grayscale .RAW file in little-endian byte order

        // Check size
        const auto size = (int32)Math::Sqrt(fileData.Count() / 2.0f);
        if (fileData.Count() != size * size * 2)
        {
            LOG(Warning, "Invalid RAW file data size or format. Use 16-bit .RAW file in little-endian byte order (square dimensions).");
            return true;
        }

        // Setup texture data
        textureData.Width = size;
        textureData.Height = size;
        textureData.Depth = 1;
        textureData.Format = PixelFormat::R16_UNorm;
        textureData.Items.Resize(1);
        textureData.Items[0].Mips.Resize(1);
        auto& mip = textureData.Items[0].Mips[0];
        mip.RowPitch = fileData.Count() / size;
        mip.DepthPitch = fileData.Count();
        mip.Lines = size;
        mip.Data.Copy(fileData);

        break;
    }
    case ImageType::DDS:
        LOG(Warning, "DDS format is not supported by stb library.");
        break;
    case ImageType::TIFF:
        LOG(Warning, "TIFF format is not supported by stb library.");
        break;
    default:
        LOG(Warning, "Unknown format.");
        return true;
    }

    return false;
}

bool TextureTool::ResizeStb(TextureData& dst, const TextureData& src, int32 dstWidth, int32 dstHeight)
{
    // Setup
    auto arraySize = src.GetArraySize();
    dst.Width = dstWidth;
    dst.Height = dstHeight;
    dst.Depth = src.Depth;
    dst.Format = src.Format;
    dst.Items.Resize(arraySize);
    auto formatSize = PixelFormatExtensions::SizeInBytes(src.Format);
    auto components = PixelFormatExtensions::ComputeComponentsCount(src.Format);

    // Resize all array slices
    for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
    {
        const auto& srcSlice = src.Items[arrayIndex];
        auto& dstSlice = dst.Items[arrayIndex];
        auto mipLevels = srcSlice.Mips.Count();
        dstSlice.Mips.Resize(mipLevels);

        // Resize all mip levels
        for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
        {
            const auto& srcMip = srcSlice.Mips[mipIndex];
            auto& dstMip = dstSlice.Mips[mipIndex];
            auto srcMipWidth = srcMip.RowPitch / formatSize;
            auto srcMipHeight = srcMip.DepthPitch / srcMip.RowPitch;
            auto dstMipWidth = Math::Max(dstWidth << mipIndex, 1);
            auto dstMipHeight = Math::Max(dstHeight << mipIndex, 1);

            // Allocate memory
            dstMip.RowPitch = dstMipWidth * formatSize;
            dstMip.DepthPitch = dstMip.RowPitch * dstMipHeight;
            dstMip.Lines = dstMipHeight;
            dstMip.Data.Allocate(dstMip.DepthPitch);

            // Resize texture
            switch (src.Format)
            {
            case PixelFormat::R8_Typeless:
            case PixelFormat::R8_SInt:
            case PixelFormat::R8_SNorm:
            case PixelFormat::R8G8_Typeless:
            case PixelFormat::R8G8_SInt:
            case PixelFormat::R8G8_SNorm:
            case PixelFormat::R8G8B8A8_Typeless:
            case PixelFormat::R8G8B8A8_UNorm:
            case PixelFormat::R8G8B8A8_UInt:
            case PixelFormat::R8G8B8A8_SNorm:
            case PixelFormat::R8G8B8A8_SInt:
            case PixelFormat::B8G8R8A8_UNorm:
            case PixelFormat::B8G8R8X8_Typeless:
            case PixelFormat::B8G8R8X8_UNorm:
            {
                if (!stbir_resize_uint8((const uint8*)srcMip.Data.Get(), srcMipWidth, srcMipHeight, srcMip.RowPitch, (uint8*)dstMip.Data.Get(), dstMipWidth, dstMipHeight, dstMip.RowPitch, components))
                {
                    LOG(Warning, "Cannot resize image.");
                    return true;
                }
                break;
            }
            case PixelFormat::R8G8B8A8_UNorm_sRGB:
            case PixelFormat::B8G8R8A8_UNorm_sRGB:
            case PixelFormat::B8G8R8X8_UNorm_sRGB:
            {
                auto alphaChannel = src.Format == PixelFormat::B8G8R8X8_UNorm_sRGB ? STBIR_ALPHA_CHANNEL_NONE : 3;
                if (!stbir_resize_uint8_srgb((const uint8*)srcMip.Data.Get(), srcMipWidth, srcMipHeight, srcMip.RowPitch, (uint8*)dstMip.Data.Get(), dstMipWidth, dstMipHeight, dstMip.RowPitch, components, alphaChannel, 0))
                {
                    LOG(Warning, "Cannot resize image.");
                    return true;
                }
                break;
            }
            case PixelFormat::R32_Typeless:
            case PixelFormat::R32_Float:
            case PixelFormat::R32G32_Float:
            case PixelFormat::R32G32B32_Float:
            case PixelFormat::R32G32B32A32_Float:
            {
                if (!stbir_resize_float((const float*)srcMip.Data.Get(), srcMipWidth, srcMipHeight, srcMip.RowPitch, (float*)dstMip.Data.Get(), dstMipWidth, dstMipHeight, dstMip.RowPitch, components))
                {
                    LOG(Warning, "Cannot resize image.");
                    return true;
                }
                break;
            }
            default:
                LOG(Warning, "Cannot resize image. Unsupported format {0}", static_cast<int32>(src.Format));
                return true;
            }
        }
    }

    return false;
}

#endif
