// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_TEXTURE_TOOL && COMPILE_WITH_STB

#include "TextureTool.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Serialization/FileWriteStream.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Utilities/AnsiPathTempFile.h"
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

#define STBD_ABS(i) Math::Abs(i)
#define STBD_FABS(x) Math::Abs(x)
#define STB_DXT_IMPLEMENTATION
#include <ThirdParty/stb/stb_dxt.h>

#if USE_EDITOR
// Compression libs for Editor
#include <ThirdParty/detex/detex.h>
#include <ThirdParty/bc7enc16/bc7enc16.h>

// Import tinyexr library
// Source: https://github.com/syoyo/tinyexr
#define TINYEXR_IMPLEMENTATION
#define TINYEXR_USE_MINIZ 1
#define TINYEXR_USE_STB_ZLIB 0
#define TINYEXR_USE_THREAD 0
#define TINYEXR_USE_OPENMP 0
#undef min
#undef max
#include <ThirdParty/tinyexr/tinyexr.h>

#endif

static void stbWrite(void* context, void* data, int size)
{
    auto file = (FileWriteStream*)context;
    file->WriteBytes(data, (uint32)size);
}

#if USE_EDITOR

static TextureData const* stbDecompress(const TextureData& textureData, TextureData& decompressed)
{
    if (!PixelFormatExtensions::IsCompressed(textureData.Format))
        return &textureData;
    const bool srgb = PixelFormatExtensions::IsSRGB(textureData.Format);
    switch (textureData.Format)
    {
    case PixelFormat::BC4_UNorm:
    case PixelFormat::BC4_SNorm:
        decompressed.Format = PixelFormat::R8_UNorm;
        break;
    case PixelFormat::BC5_UNorm:
    case PixelFormat::BC5_SNorm:
        decompressed.Format = PixelFormat::R8G8_UNorm;
        break;
    default:
        decompressed.Format = srgb ? PixelFormat::R8G8B8A8_UNorm_sRGB : PixelFormat::R8G8B8A8_UNorm;
        break;
    }
    decompressed.Width = textureData.Width;
    decompressed.Height = textureData.Height;
    decompressed.Depth = textureData.Depth;
    decompressed.Items.Resize(1);
    decompressed.Items[0].Mips.Resize(1);

    TextureMipData* decompressedData = decompressed.GetData(0, 0);
    decompressedData->RowPitch = textureData.Width * PixelFormatExtensions::SizeInBytes(decompressed.Format);
    decompressedData->Lines = textureData.Height;
    decompressedData->DepthPitch = decompressedData->RowPitch * decompressedData->Lines;
    decompressedData->Data.Allocate(decompressedData->DepthPitch);
    byte* decompressedBytes = decompressedData->Data.Get();

    Color32 colors[16];
    int32 blocksWidth = textureData.Width / 4;
    int32 blocksHeight = textureData.Height / 4;
    const TextureMipData* blocksData = textureData.GetData(0, 0);
    const byte* blocksBytes = blocksData->Data.Get();

    typedef bool (*detexDecompressBlockFuncType)(const uint8_t* bitstring, uint32_t mode_mask, uint32_t flags, uint8_t* pixel_buffer);
    detexDecompressBlockFuncType detexDecompressBlockFunc;
    int32 pixelSize, blockSize;
    switch (textureData.Format)
    {
    case PixelFormat::BC1_UNorm:
    case PixelFormat::BC1_UNorm_sRGB:
        detexDecompressBlockFunc = detexDecompressBlockBC1;
        pixelSize = 4;
        blockSize = 8;
        break;
    case PixelFormat::BC2_UNorm:
    case PixelFormat::BC2_UNorm_sRGB:
        detexDecompressBlockFunc = detexDecompressBlockBC2;
        pixelSize = 4;
        blockSize = 16;
        break;
    case PixelFormat::BC3_UNorm:
    case PixelFormat::BC3_UNorm_sRGB:
        detexDecompressBlockFunc = detexDecompressBlockBC3;
        pixelSize = 4;
        blockSize = 16;
        break;
    case PixelFormat::BC4_UNorm:
        detexDecompressBlockFunc = detexDecompressBlockRGTC1;
        pixelSize = 1;
        blockSize = 8;
        break;
    case PixelFormat::BC5_UNorm:
        detexDecompressBlockFunc = detexDecompressBlockRGTC2;
        pixelSize = 2;
        blockSize = 16;
        break;
    case PixelFormat::BC7_UNorm:
    case PixelFormat::BC7_UNorm_sRGB:
        detexDecompressBlockFunc = detexDecompressBlockBPTC;
        pixelSize = 4;
        blockSize = 16;
        break;
    default:
        LOG(Warning, "Texture data format {0} is not supported by detex library.", (int32)textureData.Format);
        return nullptr;
    }

    uint8 blockBuffer[DETEX_MAX_BLOCK_SIZE];
    for (int32 y = 0; y < blocksHeight; y++)
    {
        int32 rows;
        if (y * 4 + 3 >= textureData.Height)
            rows = textureData.Height - y * 4;
        else
            rows = 4;
        for (int32 x = 0; x < blocksWidth; x++)
        {
            const byte* block = blocksBytes + y * blocksData->RowPitch + x * blockSize;
            if (!detexDecompressBlockFunc(block, DETEX_MODE_MASK_ALL, 0, blockBuffer))
                memset(blockBuffer, 0, DETEX_MAX_BLOCK_SIZE);
            uint8* pixels = decompressedBytes + y * 4 * textureData.Width * pixelSize + x * 4 * pixelSize;
            int32 columns;
            if (x * 4 + 3  >= textureData.Width)
                columns = textureData.Width - x * 4;
            else
                columns = 4;
            for (int32 row = 0; row < rows; row++)
                memcpy(pixels + row * textureData.Width * pixelSize, blockBuffer + row * 4 * pixelSize, columns * pixelSize);
        }
    }

    return &decompressed;
}

#endif

bool TextureTool::ExportTextureStb(ImageType type, const StringView& path, const TextureData& textureData)
{
    if (textureData.GetArraySize() != 1)
    {
        LOG(Warning, "Exporting texture arrays and cubemaps is not supported.");
    }
    TextureData const* texture = &textureData;

#if USE_EDITOR
    // Handle compressed textures
    TextureData decompressed;
    texture = stbDecompress(textureData, decompressed);
    if (!texture)
        return true;
#endif

    // Convert into RGBA8
    const auto sampler = GetSampler(texture->Format);
    if (sampler == nullptr)
    {
        LOG(Warning, "Texture data format {0} is not supported.", (int32)textureData.Format);
        return true;
    }
    const auto srcData = texture->GetData(0, 0);
    const int comp = 4;
    Array<byte> data;
    bool sRGB = PixelFormatExtensions::IsSRGB(texture->Format);
    if (type == ImageType::HDR)
    {
        data.Resize(sizeof(float) * comp * texture->Width * texture->Height);

        auto ptr = (Float4*)data.Get();
        for (int32 y = 0; y < texture->Height; y++)
        {
            for (int32 x = 0; x < texture->Width; x++)
            {
                Color color = SamplePoint(sampler, x, y, srcData->Data.Get(), srcData->RowPitch);
                if (sRGB)
                    color = Color::SrgbToLinear(color);
                *(ptr + x + y * texture->Width) = color.ToFloat4();
            }
        }
    }
    else
    {
        data.Resize(sizeof(Color32) * comp * texture->Width * texture->Height);

        auto ptr = (Color32*)data.Get();
        for (int32 y = 0; y < texture->Height; y++)
        {
            for (int32 x = 0; x < texture->Width; x++)
            {
                Color color = SamplePoint(sampler, x, y, srcData->Data.Get(), srcData->RowPitch);
                if (sRGB)
                    color = Color::SrgbToLinear(color);
                *(ptr + x + y * texture->Width) = Color32(color);
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
        result = stbi_write_bmp_core(&s, texture->Width, texture->Height, comp, data.Get());
        break;
    case ImageType::JPEG:
        result = stbi_write_jpg_core(&s, texture->Width, texture->Height, comp, data.Get(), 90);
        break;
    case ImageType::TGA:
        result = stbi_write_tga_core(&s, texture->Width, texture->Height, comp, data.Get());
        break;
    case ImageType::HDR:
        result = stbi_write_hdr_core(&s, texture->Width, texture->Height, comp, (float*)data.Get());
        break;
    case ImageType::PNG:
    {
        int32 ptrSize = 0;
        const auto ptr = stbi_write_png_to_mem(data.Get(), 0, texture->Width, texture->Height, comp, &ptrSize);
        if (ptr)
        {
            file->WriteBytes(ptr, ptrSize);
            STBIW_FREE(ptr);
            result = 0;
        }
        else
        {
            result = 99;
        }
        break;
    }
    case ImageType::GIF:
        LOG(Warning, "GIF format is not supported.");
        break;
    case ImageType::TIFF:
        LOG(Warning, "GIF format is not supported.");
        break;
    case ImageType::DDS:
        LOG(Warning, "DDS format is not supported.");
        break;
    case ImageType::RAW:
        LOG(Warning, "RAW format is not supported.");
        break;
    case ImageType::EXR:
        LOG(Warning, "EXR format is not supported.");
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
            LOG(Warning, "Failed to load image. {0}", String(stbi_failure_reason()));
            return false;
        }
        fileData.Resize(0);

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
    case ImageType::EXR:
    {
#if USE_EDITOR
        // Load exr file
        AnsiPathTempFile tempFile(path);
        float* pixels;
        int width, height;
        const char* err = nullptr;
        int ret = LoadEXR(&pixels, &width, &height, tempFile.Path.Get(), &err);
        if (ret != TINYEXR_SUCCESS)
        {
            if (err)
            {
                LOG_STR(Warning, String(err));
                FreeEXRErrorMessage(err);
            }
            return true;
        }
        
        // Setup texture data
        textureData.Width = width;
        textureData.Height = height;
        textureData.Depth = 1;
        textureData.Format = PixelFormat::R32G32B32A32_Float;
        textureData.Items.Resize(1);
        textureData.Items[0].Mips.Resize(1);
        auto& mip = textureData.Items[0].Mips[0];
        mip.RowPitch = width * sizeof(Float4);
        mip.DepthPitch = mip.RowPitch * height;
        mip.Lines = height;
        mip.Data.Copy((const byte*)pixels, mip.DepthPitch);

        free(pixels);
#else
        LOG(Warning, "EXR format is not supported.");
#endif
        break;
    }
    case ImageType::DDS:
        LOG(Warning, "DDS format is not supported.");
        break;
    case ImageType::TIFF:
        LOG(Warning, "TIFF format is not supported.");
        break;
    default:
        LOG(Warning, "Unknown format.");
        return true;
    }

    return false;
}

bool TextureTool::ImportTextureStb(ImageType type, const StringView& path, TextureData& textureData, const Options& options, String& errorMsg, bool& hasAlpha)
{
    // Load image data
    if (type == ImageType::Internal)
    {
        if (!options.InternalLoad.IsBinded() || options.InternalLoad(textureData))
            return true;
        if (options.FlipY || options.FlipX)
        {
            // TODO: impl this
            errorMsg = TEXT("Flipping images imported from Internal source is not supported by stb.");
            return true;
        }
    }
    else
    {
        stbi_set_flip_vertically_on_load_thread(options.FlipY);
        bool failed = ImportTextureStb(type, path, textureData, hasAlpha);
        stbi_set_flip_vertically_on_load_thread(false);
        if (failed)
        {
            return true;
        }
    }

    // Use two data containers for texture importing for more optimzied performance
    TextureData textureDataTmp;
    TextureData* textureDataSrc = &textureData;
    TextureData* textureDataDst = &textureDataTmp;

    // Check if resize source image
    const int32 sourceWidth = textureData.Width;
    const int32 sourceHeight = textureData.Height;
    int32 width = Math::Clamp(options.Resize ? options.SizeX : static_cast<int32>(sourceWidth * options.Scale), 1, options.MaxSize);
    int32 height = Math::Clamp(options.Resize ? options.SizeY : static_cast<int32>(sourceHeight * options.Scale), 1, options.MaxSize);
    if (sourceWidth != width || sourceHeight != height)
    {
        // During resizing we need to keep texture aspect ratio
        const bool keepAspectRatio = options.KeepAspectRatio; // TODO: expose as import option
        if (keepAspectRatio)
        {
            const float aspectRatio = static_cast<float>(sourceWidth) / sourceHeight;
            if (width >= height)
                height = Math::CeilToInt(width / aspectRatio);
            else
                width = Math::CeilToInt(height / aspectRatio);
        }

        // Resize source texture
        LOG(Info, "Resizing texture from {0}x{1} to {2}x{3}.", sourceWidth, sourceHeight, width, height);
        if (ResizeStb(*textureDataDst, *textureDataSrc, width, height))
        {
            errorMsg = String::Format(TEXT("Cannot resize texture."));
            return true;
        }
        ::Swap(textureDataSrc, textureDataDst);
    }

    // Cache data
    float alphaThreshold = 0.3f;
    bool isPowerOfTwo = Math::IsPowerOfTwo(width) && Math::IsPowerOfTwo(height);
    PixelFormat targetFormat = ToPixelFormat(options.Type, width, height, options.Compress);
    if (options.sRGB)
        targetFormat = PixelFormatExtensions::TosRGB(targetFormat);

    // Check mip levels
    int32 sourceMipLevels = textureDataSrc->GetMipLevels();
    bool hasSourceMipLevels = isPowerOfTwo && sourceMipLevels > 1;
    bool useMipLevels = isPowerOfTwo && (options.GenerateMipMaps || hasSourceMipLevels) && (width > 1 || height > 1);
    int32 arraySize = (int32)textureDataSrc->GetArraySize();
    int32 mipLevels = MipLevelsCount(width, height, useMipLevels);
    if (useMipLevels && !options.GenerateMipMaps && mipLevels != sourceMipLevels)
    {
        errorMsg = String::Format(TEXT("Imported texture has not full mip chain, loaded mips count: {0}, expected: {1}"), sourceMipLevels, mipLevels);
        return true;
    }

    // Decompress if texture is compressed (next steps need decompressed input data, for eg. mip maps generation or format changing)
    if (PixelFormatExtensions::IsCompressed(textureDataSrc->Format))
    {
        // TODO: implement texture decompression
        errorMsg = String::Format(TEXT("Imported texture used compressed format {0}. Not supported for importing on this platform.."), (int32)textureDataSrc->Format);
        return true;
    }

    if (options.FlipX)
    {
        // TODO: impl this
        LOG(Warning, "Option 'Flip X' is not supported");
    }
    if (options.InvertRedChannel || options.InvertGreenChannel || options.InvertBlueChannel || options.InvertAlphaChannel)
    {
        // TODO: impl this
        LOG(Warning, "Option to invert channels is not supported");
    }
    if (options.ReconstructZChannel)
    {
        // TODO: impl this
        LOG(Warning, "Option 'Reconstruct Z Channel' is not supported");
    }

    // Generate mip maps chain
    if (useMipLevels && options.GenerateMipMaps)
    {
        for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
        {
            auto& slice = textureDataSrc->Items[arrayIndex];
            slice.Mips.Resize(mipLevels);
            for (int32 mipIndex = 1; mipIndex < mipLevels; mipIndex++)
            {
                const auto& srcMip = slice.Mips[mipIndex - 1];
                auto& dstMip = slice.Mips[mipIndex];
                auto dstMipWidth = Math::Max(textureDataSrc->Width >> mipIndex, 1);
                auto dstMipHeight = Math::Max(textureDataSrc->Height >> mipIndex, 1);
                if (ResizeStb(textureDataSrc->Format, dstMip, srcMip, dstMipWidth, dstMipHeight))
                {
                    errorMsg = TEXT("Failed to generate mip texture.");
                    return true;
                }
            }
        }
    }

    // Preserve mipmap alpha coverage (if requested)
    if (PixelFormatExtensions::HasAlpha(textureDataSrc->Format) && options.PreserveAlphaCoverage && useMipLevels)
    {
        // TODO: implement alpha coverage preserving
        errorMsg = TEXT("Importing textures with alpha coverage preserving is not supported on this platform.");
        return true;
    }

    // Compress mip maps or convert image
    if (targetFormat != textureDataSrc->Format)
    {
        if (ConvertStb(*textureDataDst, *textureDataSrc, targetFormat))
        {
            errorMsg = String::Format(TEXT("Cannot convert/compress texture."));
            return true;
        }
        ::Swap(textureDataSrc, textureDataDst);
    }

    // Copy data to the output if not in the result container
    if (textureDataSrc != &textureData)
    {
        textureData = textureDataTmp;
    }

    return false;
}

bool TextureTool::ConvertStb(TextureData& dst, const TextureData& src, const PixelFormat dstFormat)
{
    TextureData const* textureData = &src;

#if USE_EDITOR
    // Handle compressed textures
    TextureData decompressed;
    textureData = stbDecompress(src, decompressed);
    if (!textureData)
        return true;
#endif

    // Setup
    auto arraySize = textureData->GetArraySize();
    dst.Width = textureData->Width;
    dst.Height = textureData->Height;
    dst.Depth = textureData->Depth;
    dst.Format = dstFormat;
    dst.Items.Resize(arraySize, false);
    auto formatSize = PixelFormatExtensions::SizeInBytes(textureData->Format);
    auto components = PixelFormatExtensions::ComputeComponentsCount(textureData->Format);
    auto sampler = TextureTool::GetSampler(textureData->Format);
    if (!sampler)
    {
        LOG(Warning, "Cannot convert image. Unsupported format {0}", static_cast<int32>(textureData->Format));
        return true;
    }

#if USE_EDITOR
    if (PixelFormatExtensions::IsCompressedBC(dstFormat))
    {
        int32 bytesPerBlock;
        switch (dstFormat)
        {
        case PixelFormat::BC1_UNorm:
        case PixelFormat::BC1_UNorm_sRGB:
        case PixelFormat::BC4_UNorm:
            bytesPerBlock = 8;
            break;
        default:
            bytesPerBlock = 16;
            break;
        }
        bool isDstSRGB = PixelFormatExtensions::IsSRGB(dstFormat);

        // bc7enc init
        bc7enc16_compress_block_params params;
        if (dstFormat == PixelFormat::BC7_UNorm || dstFormat == PixelFormat::BC7_UNorm_sRGB)
        {
            bc7enc16_compress_block_params_init(&params);
            bc7enc16_compress_block_init();
        }

        // Compress all array slices
        for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
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
                auto blocksWidth = Math::Max(Math::DivideAndRoundUp(mipWidth, 4), 1);
                auto blocksHeight = Math::Max(Math::DivideAndRoundUp(mipHeight, 4), 1);

                // Allocate memory
                dstMip.RowPitch = blocksWidth * bytesPerBlock;
                dstMip.DepthPitch = dstMip.RowPitch * blocksHeight;
                dstMip.Lines = blocksHeight;
                dstMip.Data.Allocate(dstMip.DepthPitch);

                // Compress texture
                for (int32 yBlock = 0; yBlock < blocksHeight; yBlock++)
                {
                    for (int32 xBlock = 0; xBlock < blocksWidth; xBlock++)
                    {
                        // Sample source texture 4x4 block
                        Color32 srcBlock[16];
                        for (int32 y = 0; y < 4; y++)
                        {
                            for (int32 x = 0; x < 4; x++)
                            {
                                Color color = TextureTool::SamplePoint(sampler, xBlock * 4 + x, yBlock * 4 + y, srcMip.Data.Get(), srcMip.RowPitch);
                                if (isDstSRGB)
                                    color = Color::LinearToSrgb(color);
                                srcBlock[y * 4 + x] = Color32(color);
                            }
                        }

                        // Compress block
                        byte* dstBlock = dstMip.Data.Get() + (yBlock * blocksWidth + xBlock) * bytesPerBlock;
                        switch (dstFormat)
                        {
                        case PixelFormat::BC1_UNorm:
                        case PixelFormat::BC1_UNorm_sRGB:
                            stb_compress_dxt_block(dstBlock, (byte*)&srcBlock, 0, STB_DXT_HIGHQUAL);
                            break;
                        case PixelFormat::BC3_UNorm:
                        case PixelFormat::BC3_UNorm_sRGB:
                            stb_compress_dxt_block(dstBlock, (byte*)&srcBlock, 1, STB_DXT_HIGHQUAL);
                            break;
                        case PixelFormat::BC4_UNorm:
                            for (int32 i = 1; i < 16; i++)
                                ((byte*)&srcBlock)[i] = srcBlock[i].R;
                            stb_compress_bc4_block(dstBlock, (byte*)&srcBlock);
                            break;
                        case PixelFormat::BC5_UNorm:
                            for (int32 i = 0; i < 16; i++)
                                ((uint16*)&srcBlock)[i] = srcBlock[i].R << 8 | srcBlock[i].G;
                            stb_compress_bc5_block(dstBlock, (byte*)&srcBlock);
                            break;
                        case PixelFormat::BC7_UNorm:
                        case PixelFormat::BC7_UNorm_sRGB:
                            bc7enc16_compress_block(dstBlock, &srcBlock, &params);
                            break;
                        default:
                            LOG(Warning, "Cannot compress image. Unsupported format {0}", static_cast<int32>(dstFormat));
                            return true;
                        }
                    }
                }
            }
        }
    }
    else if (PixelFormatExtensions::IsCompressedASTC(dstFormat))
    {
#if COMPILE_WITH_ASTC
        if (ConvertAstc(dst, *textureData, dstFormat))
#else
        LOG(Error, "Missing ASTC texture format compression lib.");
#endif
        {
            return true;
        }
    }
    else
#endif
    {
        int32 bytesPerPixel = PixelFormatExtensions::SizeInBytes(dstFormat);
        auto dstSampler = TextureTool::GetSampler(dstFormat);
        if (!dstSampler)
        {
            LOG(Warning, "Cannot convert image. Unsupported format {0}", static_cast<int32>(dstFormat));
            return true;
        }

        // Convert all array slices
        for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
        {
            const auto& srcSlice = textureData->Items[arrayIndex];
            auto& dstSlice = dst.Items[arrayIndex];
            auto mipLevels = srcSlice.Mips.Count();
            dstSlice.Mips.Resize(mipLevels, false);

            // Convert all mip levels
            for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
            {
                const auto& srcMip = srcSlice.Mips[mipIndex];
                auto& dstMip = dstSlice.Mips[mipIndex];
                auto mipWidth = Math::Max(textureData->Width >> mipIndex, 1);
                auto mipHeight = Math::Max(textureData->Height >> mipIndex, 1);

                // Allocate memory
                dstMip.RowPitch = mipWidth * bytesPerPixel;
                dstMip.DepthPitch = dstMip.RowPitch * mipHeight;
                dstMip.Lines = mipHeight;
                dstMip.Data.Allocate(dstMip.DepthPitch);

                // Convert texture
                for (int32 y = 0; y < mipHeight; y++)
                {
                    for (int32 x = 0; x < mipWidth; x++)
                    {
                        // Sample source texture
                        Color color = TextureTool::SamplePoint(sampler, x, y, srcMip.Data.Get(), srcMip.RowPitch);

                        // Store destination texture
                        TextureTool::Store(dstSampler, x, y, dstMip.Data.Get(), dstMip.RowPitch, color);
                    }
                }
            }
        }
    }

    return false;
}

bool TextureTool::ResizeStb(PixelFormat format, TextureMipData& dstMip, const TextureMipData& srcMip, int32 dstMipWidth, int32 dstMipHeight)
{
    // Setup
    auto formatSize = PixelFormatExtensions::SizeInBytes(format);
    auto components = PixelFormatExtensions::ComputeComponentsCount(format);
    auto srcMipWidth = srcMip.RowPitch / formatSize;
    auto srcMipHeight = srcMip.DepthPitch / srcMip.RowPitch;
    auto sampler = GetSampler(format);

    // Allocate memory
    dstMip.RowPitch = dstMipWidth * formatSize;
    dstMip.DepthPitch = dstMip.RowPitch * dstMipHeight;
    dstMip.Lines = dstMipHeight;
    dstMip.Data.Allocate(dstMip.DepthPitch);

    // Resize texture
    switch (format)
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
        auto alphaChannel = format == PixelFormat::B8G8R8X8_UNorm_sRGB ? STBIR_ALPHA_CHANNEL_NONE : 3;
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
        if (sampler)
        {
            const Int2 srcSize(srcMipWidth, srcMipHeight);
            for (int32 y = 0; y < dstMipHeight; y++)
            {
                for (int32 x = 0; x < dstMipWidth; x++)
                {
                    const Float2 uv((float)x / dstMipWidth, (float)y / dstMipHeight);
                    Color color = SamplePoint(sampler, uv, srcMip.Data.Get(), srcSize, srcMip.RowPitch);
                    Store(sampler, x, y, dstMip.Data.Get(), dstMip.RowPitch, color);
                }
            }
            return false;
        }
        LOG(Warning, "Cannot resize image. Unsupported format {0}", static_cast<int32>(format));
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
    dst.Items.Resize(arraySize, false);
    auto formatSize = PixelFormatExtensions::SizeInBytes(src.Format);
    auto components = PixelFormatExtensions::ComputeComponentsCount(src.Format);

    // Resize all array slices
    for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
    {
        const auto& srcSlice = src.Items[arrayIndex];
        auto& dstSlice = dst.Items[arrayIndex];
        auto mipLevels = srcSlice.Mips.Count();
        dstSlice.Mips.Resize(mipLevels, false);

        // Resize all mip levels
        for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
        {
            const auto& srcMip = srcSlice.Mips[mipIndex];
            auto& dstMip = dstSlice.Mips[mipIndex];
            auto srcMipWidth = srcMip.RowPitch / formatSize;
            auto srcMipHeight = srcMip.DepthPitch / srcMip.RowPitch;
            auto dstMipWidth = Math::Max(dstWidth >> mipIndex, 1);
            auto dstMipHeight = Math::Max(dstHeight >> mipIndex, 1);
            if (ResizeStb(src.Format, dstMip, srcMip, dstMipWidth, dstMipHeight))
                return true;
        }
    }

    return false;
}

#endif
