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

#define STBI_WRITE_NO_STDIO
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <ThirdParty/stb/stb_image_write.h>

#define STBI_NO_PSD
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_FAILURE_STRINGS
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include <ThirdParty/stb/stb_image.h>

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
        int x, y, comp;
        stbi_uc* stbData = stbi_load_from_memory(fileData.Get(), fileData.Count(), &x, &y, &comp, 4);
        if (!stbData)
        {
            LOG(Warning, "Failed to load image.");
            return false;
        }

        // Setup texture data
        textureData.Width = x;
        textureData.Height = y;
        textureData.Depth = 1;
        textureData.Format = PixelFormat::R8G8B8A8_UNorm;
        textureData.Items.Resize(1);
        textureData.Items[0].Mips.Resize(1);
        auto& mip = textureData.Items[0].Mips[0];
        mip.RowPitch = sizeof(Color32) * x;
        mip.DepthPitch = mip.RowPitch * y;
        mip.Lines = y;
        mip.Data.Copy(stbData, mip.DepthPitch);

#if USE_EDITOR
        // TODO: detect hasAlpha
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

#endif
