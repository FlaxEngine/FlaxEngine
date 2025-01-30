// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_TEXTURE_TOOL && COMPILE_WITH_DIRECTXTEX

#include "TextureTool.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Platform/ConditionVariable.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Async/GPUTask.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#if USE_EDITOR
#include "Engine/Graphics/GPUDevice.h"
#endif
#include "Engine/Utilities/AnsiPathTempFile.h"

// Import DirectXTex library
// Source: https://github.com/Microsoft/DirectXTex
#if PLATFORM_XBOX_SCARLETT || PLATFORM_XBOX_ONE
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
DECLARE_HANDLE(HMONITOR);
#endif
#include <ThirdParty/DirectXTex/DirectXTex.h>

#if USE_EDITOR
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

namespace
{
    FORCE_INLINE PixelFormat ToPixelFormat(const DXGI_FORMAT format)
    {
        return static_cast<PixelFormat>(format);
    }

    FORCE_INLINE DXGI_FORMAT ToDxgiFormat(const PixelFormat format)
    {
        return static_cast<DXGI_FORMAT>(format);
    }

    FORCE_INLINE DXGI_FORMAT ToDecompressFormat(const DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC3_TYPELESS:
            return DXGI_FORMAT_R8G8B8A8_TYPELESS;
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC3_UNORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case DXGI_FORMAT_BC4_TYPELESS:
            return DXGI_FORMAT_R8_TYPELESS;
        case DXGI_FORMAT_BC4_UNORM:
            return DXGI_FORMAT_R8_UNORM;
        case DXGI_FORMAT_BC4_SNORM:
            return DXGI_FORMAT_R8_SNORM;
        case DXGI_FORMAT_BC5_TYPELESS:
            return DXGI_FORMAT_R8G8_TYPELESS;
        case DXGI_FORMAT_BC5_UNORM:
            return DXGI_FORMAT_R8G8_UNORM;
        case DXGI_FORMAT_BC5_SNORM:
            return DXGI_FORMAT_R8G8_SNORM;
        default:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        }
    }

    HRESULT Compress(const DirectX::Image* srcImages, size_t nimages, const DirectX::TexMetadata& metadata, DXGI_FORMAT format, DirectX::TEX_COMPRESS_FLAGS compress, float threshold, DirectX::ScratchImage& cImages)
    {
#if USE_EDITOR
        if ((format == DXGI_FORMAT_BC7_UNORM || format == DXGI_FORMAT_BC7_UNORM_SRGB || format == DXGI_FORMAT_BC6H_UF16 || format == DXGI_FORMAT_BC6H_SF16) &&
            GPUDevice::Instance &&
            GPUDevice::Instance->GetState() == GPUDevice::DeviceState::Ready &&
            GPUDevice::Instance->GetRendererType() == RendererType::DirectX11)
        {
            // Use GPU compression
            GPUDevice::Instance->Locker.Lock();
            if (GPUDevice::Instance->IsRendering())
            {
                const auto result = DirectX::Compress((ID3D11Device*)GPUDevice::Instance->GetNativePtr(), srcImages, nimages, metadata, format, compress, 1.0f, cImages);
                GPUDevice::Instance->Locker.Unlock();
                return result;
            }
            GPUDevice::Instance->Locker.Unlock();
            class GPUCompressTask : public GPUTask
            {
                ConditionVariable* _signal;
                const DirectX::Image* _srcImages;
                size_t _nimages;
                const DirectX::TexMetadata& _metadata;
                DXGI_FORMAT _format;
                DirectX::TEX_COMPRESS_FLAGS _compress;
                DirectX::ScratchImage& _cImages;
            public:
                HRESULT CompressResult = E_FAIL;

                GPUCompressTask(ConditionVariable& signal, const DirectX::Image* srcImages, size_t nimages, const DirectX::TexMetadata& metadata, DXGI_FORMAT format, DirectX::TEX_COMPRESS_FLAGS compress, DirectX::ScratchImage& cImages)
                    : GPUTask(Type::Custom)
                    , _signal(&signal)
                    , _srcImages(srcImages)
                    , _nimages(nimages)
                    , _metadata(metadata)
                    , _format(format)
                    , _compress(compress)
                    , _cImages(cImages)
                {
                }

                Result run(GPUTasksContext* context) override
                {
                    CompressResult = DirectX::Compress((ID3D11Device*)context->GetDevice()->GetNativePtr(), _srcImages, _nimages, _metadata, _format, _compress, 1.0f, _cImages);
                    return CompressResult == S_OK ? Result::Ok : Result::Failed;
                }

                void OnSync() override
                {
                    GPUTask::OnSync();
                    _signal->NotifyOne();
                }

                void OnCancel() override
                {
                    GPUTask::OnCancel();
                    _signal->NotifyOne();
                }

                void OnFail() override
                {
                    GPUTask::OnFail();
                    _signal->NotifyOne();
                }
            };
            ConditionVariable signal;
            CriticalSection mutex;
            auto task = New<GPUCompressTask>(signal, srcImages, nimages, metadata, format, compress, cImages);
            task->Start();
            mutex.Lock();
            signal.Wait(mutex);
            mutex.Unlock();
            return task->CompressResult;
        }
#endif
        return DirectX::Compress(srcImages, nimages, metadata, format, compress, threshold, cImages);
    }
}

bool TextureTool::ExportTextureDirectXTex(ImageType type, const StringView& path, const TextureData& textureData)
{
    // Get source data
    const auto& srcData = *textureData.GetData(0, 0);

    // Setup image container
    DirectX::Image image;
    image.width = textureData.Width;
    image.height = textureData.Height;
    image.format = ToDxgiFormat(textureData.Format);
    image.rowPitch = srcData.RowPitch;
    image.slicePitch = srcData.DepthPitch;
    image.pixels = (uint8_t*)srcData.Data.Get();

    // Save
    HRESULT result;
    switch (type)
    {
    case ImageType::DDS:
    {
        const bool isCubeTexture = textureData.GetArraySize() == 6;

        DirectX::TexMetadata metadata;
        metadata.width = image.width;
        metadata.height = image.height;
        metadata.depth = 1;
        metadata.arraySize = textureData.GetArraySize();
        metadata.mipLevels = textureData.GetMipLevels();
        metadata.miscFlags = isCubeTexture ? DirectX::TEX_MISC_TEXTURECUBE : 0;
        metadata.miscFlags2 = 0;
        metadata.format = image.format;
        metadata.dimension = DirectX::TEX_DIMENSION_TEXTURE2D;

        Array<DirectX::Image> images;
        images.Resize((int32)(metadata.mipLevels * metadata.arraySize));
        for (size_t arrayIndex = 0; arrayIndex < metadata.arraySize; arrayIndex++)
        {
            for (size_t mipIndex = 0; mipIndex < metadata.mipLevels; mipIndex++)
            {
                auto& src = *textureData.GetData((int32)arrayIndex, (int32)mipIndex);
                auto& img = images[(int32)(metadata.mipLevels * arrayIndex + mipIndex)];

                img.width = Math::Max<size_t>(1, image.width >> mipIndex);
                img.height = Math::Max<size_t>(1, image.height >> mipIndex);
                img.format = image.format;
                img.rowPitch = src.RowPitch;
                img.slicePitch = src.DepthPitch;
                img.pixels = (uint8_t*)src.Data.Get();
            }
        }

        result = DirectX::SaveToDDSFile(images.Get(), images.Count(), metadata, DirectX::DDS_FLAGS_NONE, *path);
        break;
    }
    case ImageType::TGA:
        result = SaveToTGAFile(image, *path);
        break;
    case ImageType::PNG:
    case ImageType::BMP:
    case ImageType::GIF:
    case ImageType::TIFF:
    case ImageType::JPEG:
    {
        const DirectX::Image* img = &image;
        DirectX::ScratchImage tmp;
        if (DirectX::IsCompressed(image.format))
        {
            result = Decompress(image, DXGI_FORMAT_R8G8B8A8_UNORM, tmp);
            if (FAILED(result))
            {
                LOG(Error, "Cannot decompress texture, error: {0:x}", static_cast<uint32>(result));
                return true;
            }
            img = tmp.GetImage(0, 0, 0);
        }
        else if (image.format == DXGI_FORMAT_R10G10B10A2_UNORM || image.format == DXGI_FORMAT_R11G11B10_FLOAT)
        {
            result = DirectX::Convert(image, DXGI_FORMAT_R8G8B8A8_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, tmp);
            if (FAILED(result))
            {
                LOG(Error, "Cannot convert texture, error: {0:x}", static_cast<uint32>(result));
                return true;
            }
            img = tmp.GetImage(0, 0, 0);
        }

        DirectX::WICCodecs codec;
        switch (type)
        {
        case ImageType::PNG:
            codec = DirectX::WIC_CODEC_PNG;
            break;
        case ImageType::BMP:
            codec = DirectX::WIC_CODEC_BMP;
            break;
        case ImageType::GIF:
            codec = DirectX::WIC_CODEC_GIF;
            break;
        case ImageType::TIFF:
            codec = DirectX::WIC_CODEC_TIFF;
            break;
        case ImageType::JPEG:
            codec = DirectX::WIC_CODEC_JPEG;
            break;
        default: ;
        }

        result = DirectX::SaveToWICFile(*img, DirectX::WIC_FLAGS_FORCE_SRGB, GetWICCodec(codec), *path);
        break;
    }
    case ImageType::HDR:
        result = SaveToHDRFile(image, *path);
        break;
    default:
        result = E_NOTIMPL;
        break;
    }
    if (FAILED(result))
    {
        LOG(Error, "Exporting texture to '{0}' error: {1:x}", *path, (uint32)result);
        return true;
    }

    return false;
}

HRESULT LoadFromRAWFile(const StringView& path, DirectX::ScratchImage& image)
{
    // Assume 16-bit, grayscale .RAW file in little-endian byte order

    // Load raw bytes from file
    Array<byte> data;
    if (File::ReadAllBytes(path, data))
    {
        LOG(Warning, "Failed to load file data.");
        return ERROR_PATH_NOT_FOUND;
    }

    // Check size
    const auto size = (int32)Math::Sqrt(data.Count() / 2.0f);
    if (data.Count() != size * size * 2)
    {
        LOG(Warning, "Invalid RAW file data size or format. Use 16-bit .RAW file in little-endian byte order (square dimensions).");
        return ERROR_BAD_FORMAT;
    }

    // Setup image
    DirectX::Image img;
    img.format = DXGI_FORMAT_R16_UNORM;
    img.width = size;
    img.height = size;
    img.rowPitch = data.Count() / size;
    img.slicePitch = data.Count();

    // Link data
    img.pixels = data.Get();

    // Init
    return image.InitializeFromImage(img);
}

HRESULT LoadFromEXRFile(const StringView& path, DirectX::ScratchImage& image)
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
        return E_FAIL;
    }

    // Setup image
    DirectX::Image img;
    img.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    img.width = width;
    img.height = height;
    img.rowPitch = width * sizeof(Float4);
    img.slicePitch = img.rowPitch * height;

    // Link data
    img.pixels = (uint8_t*)pixels;

    // Init
    HRESULT result = image.InitializeFromImage(img);
    free(pixels);
    return result;
#else
    LOG(Warning, "EXR format is not supported.");
    return E_FAIL;
#endif
}

bool TextureTool::ImportTextureDirectXTex(ImageType type, const StringView& path, TextureData& textureData, bool& hasAlpha)
{
    // Load image data
    DirectX::ScratchImage image;
    HRESULT result;
    switch (type)
    {
    case ImageType::BMP:
    case ImageType::GIF:
    case ImageType::TIFF:
    case ImageType::JPEG:
    case ImageType::PNG:
        result = DirectX::LoadFromWICFile(*path, DirectX::WIC_FLAGS_NONE, nullptr, image);
        break;
    case ImageType::DDS:
        result = DirectX::LoadFromDDSFile(*path, DirectX::DDS_FLAGS_NONE, nullptr, image);
        break;
    case ImageType::TGA:
        result = DirectX::LoadFromTGAFile(*path, nullptr, image);
        break;
    case ImageType::HDR:
        result = DirectX::LoadFromHDRFile(*path, nullptr, image);
        break;
    case ImageType::RAW:
        result = LoadFromRAWFile(path, image);
        break;
    case ImageType::EXR:
        result = LoadFromEXRFile(path, image);
        break;
    default:
        result = DXGI_ERROR_INVALID_CALL;
        break;
    }
    if (FAILED(result))
    {
        LOG(Warning, "Failed to import texture from file. Result: {0:x}", static_cast<uint32>(result));
        return true;
    }

    // Convert into texture data
    auto& meta = image.GetMetadata();
    textureData.Width = (int32)meta.width;
    textureData.Height = (int32)meta.height;
    textureData.Depth = (int32)meta.depth;
    textureData.Format = ::ToPixelFormat(meta.format);
    textureData.Items.Resize(1);
    textureData.Items.Resize((int32)meta.arraySize);
    for (int32 arrayIndex = 0; arrayIndex < (int32)meta.arraySize; arrayIndex++)
    {
        auto& item = textureData.Items[arrayIndex];
        item.Mips.Resize((int32)meta.mipLevels);

        for (int32 mipIndex = 0; mipIndex < (int32)meta.mipLevels; mipIndex++)
        {
            auto& mip = item.Mips[mipIndex];
            const auto img = image.GetImage(mipIndex, arrayIndex, 0);

            mip.RowPitch = (uint32)img->rowPitch;
            mip.DepthPitch = (uint32)img->slicePitch;
            mip.Lines = (uint32)img->height;
            mip.Data.Copy(img->pixels, mip.DepthPitch);
        }

#if USE_EDITOR
        if (!hasAlpha)
            hasAlpha |= !image.IsAlphaAllOpaque();
#endif
    }

    return false;
}

HRESULT CustomGenerateMipMap(DirectX::ScratchImage& mipChain, size_t item, size_t mip)
{
    const DirectX::TexMetadata& metadata = mipChain.GetMetadata();

    if (mip <= 0 || item < 0 || item > metadata.arraySize)
        return E_INVALIDARG;

    const DirectX::Image* srcImg = mipChain.GetImage(mip - 1, item, 0);
    const DirectX::Image* dstImg = mipChain.GetImage(mip, item, 0);

    const float srcWidth = (float)srcImg->width;
    const float srcHeight = (float)srcImg->height;
    const float dstWidth = (float)dstImg->width;
    const float dstHeight = (float)dstImg->height;

    const uint8_t* srcData = srcImg->pixels;
    const uint8_t* dstData = dstImg->pixels;

    if (metadata.format == DXGI_FORMAT_R32G32B32A32_FLOAT)
    {
        // 2x2 linear filter
        for (size_t y = 0; y < dstImg->height; y++)
        {
            float dy = y / dstHeight;
            float sy = dy * srcHeight;
            size_t p0y = Math::FloorToInt(sy);
            float pdy = sy - p0y;
            size_t p1y = Math::Min(p0y + 1, srcImg->height - 1);

            for (size_t x = 0; x < dstImg->width; x++)
            {
                float dx = x / dstWidth;
                float sx = dx * srcWidth;
                size_t p0x = Math::FloorToInt(sx);
                float pdx = sx - p0x;
                size_t p1x = Math::Min(p0x + 1, srcImg->width - 1);

                Float4 pA = *(Float4*)(srcData + srcImg->rowPitch * p0y + sizeof(Float4) * p0x);
                Float4 pB = *(Float4*)(srcData + srcImg->rowPitch * p0y + sizeof(Float4) * p1x);
                Float4 pC = *(Float4*)(srcData + srcImg->rowPitch * p1y + sizeof(Float4) * p0x);
                Float4 pD = *(Float4*)(srcData + srcImg->rowPitch * p1y + sizeof(Float4) * p1x);

                Float4 pAB;
                Float4::Lerp(pA, pB, pdx, pAB);

                Float4 pCD;
                Float4::Lerp(pC, pD, pdx, pCD);

                Float4 p;
                Float4::Lerp(pAB, pCD, pdy, p);

                *(Float4*)(dstData + dstImg->rowPitch * y + sizeof(Float4) * x) = p;
            }
        }

        return S_OK;
    }

    return E_FAIL;
}

HRESULT CustomGenerateMipMaps(const DirectX::Image* srcImages, size_t nimages, const DirectX::TexMetadata& metadata, size_t levels, DirectX::ScratchImage& mipChain)
{
    // Get source images
    Array<DirectX::Image> baseImages;
    baseImages.Resize((int32)metadata.arraySize);
    for (size_t item = 0; item < metadata.arraySize; item++)
    {
        const size_t index = metadata.ComputeIndex(0, item, 0);
        if (index >= nimages)
            return E_FAIL;

        const DirectX::Image& src = srcImages[index];
        if (!src.pixels)
            return E_POINTER;

        if (src.format != metadata.format || src.width != metadata.width || src.height != metadata.height)
        {
            // All base images must be the same format, width, and height
            return E_FAIL;
        }

        baseImages[(int32)item] = src;
    }

    // Setup mip chain
    DirectX::TexMetadata mdata2 = metadata;
    mdata2.mipLevels = levels;
    HRESULT hr = mipChain.Initialize(mdata2);
    if (FAILED(hr))
        return hr;

    // Copy base image(s) to top of mip chain
    for (size_t item = 0; item < nimages; item++)
    {
        const DirectX::Image& src = baseImages[(int32)item];

        const DirectX::Image* dest = mipChain.GetImage(0, item, 0);
        if (!dest)
        {
            mipChain.Release();
            return E_POINTER;
        }

        assert(src.format == dest->format);

        uint8_t* pDest = dest->pixels;
        if (!pDest)
        {
            mipChain.Release();
            return E_POINTER;
        }

        const uint8_t* pSrc = src.pixels;
        size_t rowPitch = src.rowPitch;
        for (size_t h = 0; h < metadata.height; h++)
        {
            const size_t msize = Math::Min<size_t>(dest->rowPitch, rowPitch);
            Platform::MemoryCopy(pDest, pSrc, msize);
            pSrc += rowPitch;
            pDest += dest->rowPitch;
        }
    }

    // Generate mip maps for each array slice
    for (size_t item = 0; item < mdata2.arraySize; item++)
    {
        for (size_t mip = 1; mip < mdata2.mipLevels; mip++)
        {
            hr = CustomGenerateMipMap(mipChain, item, mip);
            if (FAILED(hr))
            {
                mipChain.Release();
                return hr;
            }
        }
    }

    return S_OK;
}

bool TextureTool::ImportTextureDirectXTex(ImageType type, const StringView& path, TextureData& textureData, const Options& options, String& errorMsg, bool& hasAlpha)
{
#define SET_CURRENT_IMG(x) currentImage = &x
#define GET_TMP_IMG() (currentImage != &image1 ? image1 : image2)
    DirectX::ScratchImage* currentImage;
    DirectX::ScratchImage image1;
    DirectX::ScratchImage image2;
    TextureData internalData;

    // Load image data
    HRESULT result;
    switch (type)
    {
    case ImageType::BMP:
    case ImageType::GIF:
    case ImageType::TIFF:
    case ImageType::JPEG:
    case ImageType::PNG:
        result = DirectX::LoadFromWICFile(*path, DirectX::WIC_FLAGS_NONE, nullptr, image1);
        break;
    case ImageType::DDS:
        result = DirectX::LoadFromDDSFile(*path, DirectX::DDS_FLAGS_NONE, nullptr, image1);
        break;
    case ImageType::TGA:
        result = DirectX::LoadFromTGAFile(*path, nullptr, image1);
        break;
    case ImageType::HDR:
        result = DirectX::LoadFromHDRFile(*path, nullptr, image1);
        break;
    case ImageType::RAW:
        result = LoadFromRAWFile(path, image1);
        break;
    case ImageType::EXR:
        result = LoadFromEXRFile(path, image1);
        break;
    case ImageType::Internal:
    {
        if (options.InternalLoad.IsBinded())
        {
            if (!options.InternalLoad(internalData))
            {
                ASSERT(internalData.Items.Count() == 1 && internalData.Items[0].Mips.Count() == 1); // Only single 2D texture image is supported for now

                DirectX::Image img;
                auto& mip = internalData.Items[0].Mips[0];
                img.width = internalData.Width;
                img.height = internalData.Height;
                img.format = ToDxgiFormat(internalData.Format);
                img.rowPitch = mip.RowPitch;
                img.slicePitch = mip.DepthPitch;
                img.pixels = mip.Data.Get();

                result = image1.InitializeFromImage(img);
            }
            else
            {
                result = E_FAIL;
            }
        }
        else
        {
            result = DXGI_ERROR_INVALID_CALL;
            break;
        }
    }
    break;
    default:
        result = DXGI_ERROR_INVALID_CALL;
        break;
    }
    if (FAILED(result))
    {
        errorMsg = String::Format(TEXT("Result: {0:x}"), static_cast<uint32>(result));
        return true;
    }
    SET_CURRENT_IMG(image1);

    // Check if resize source image
    const int32 sourceWidth = static_cast<int32>(currentImage->GetMetadata().width);
    const int32 sourceHeight = static_cast<int32>(currentImage->GetMetadata().height);
    int32 width = Math::Clamp(options.Resize ? options.SizeX : static_cast<int32>(sourceWidth * options.Scale), 1, options.MaxSize);
    int32 height = Math::Clamp(options.Resize ? options.SizeY : static_cast<int32>(sourceHeight * options.Scale), 1, options.MaxSize);
    if (sourceWidth != width || sourceHeight != height)
    {
        // During resizing we need to keep texture aspect ratio
        const bool keepAspectRatio = options.KeepAspectRatio;
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
        if (DirectX::IsCompressed(currentImage->GetMetadata().format))
        {
            auto& tmpImg = GET_TMP_IMG();
            result = Decompress(currentImage->GetImages(), currentImage->GetImageCount(), currentImage->GetMetadata(), ToDecompressFormat(currentImage->GetMetadata().format), tmpImg);
            if (FAILED(result))
            {
                errorMsg = String::Format(TEXT("Cannot decompress texture, error: {0:x}"), static_cast<uint32>(result));
                return true;
            }
            SET_CURRENT_IMG(tmpImg);
        }
        {
            auto& tmpImg = GET_TMP_IMG();
            result = DirectX::Resize(*currentImage->GetImages(), width, height, DirectX::TEX_FILTER_LINEAR | DirectX::TEX_FILTER_SEPARATE_ALPHA, tmpImg);
            if (FAILED(result))
            {
                errorMsg = String::Format(TEXT("Cannot resize texture, error: {0:x}"), static_cast<uint32>(result));
                return true;
            }
            SET_CURRENT_IMG(tmpImg);
        }
    }

    // Cache data
    float alphaThreshold = 0.3f;
    bool isPowerOfTwo = Math::IsPowerOfTwo(width) && Math::IsPowerOfTwo(height);
    DXGI_FORMAT sourceDxgiFormat = currentImage->GetMetadata().format;
    PixelFormat targetFormat = TextureTool::ToPixelFormat(options.Type, width, height, options.Compress);
    if (options.sRGB)
        targetFormat = PixelFormatExtensions::TosRGB(targetFormat);
    DXGI_FORMAT targetDxgiFormat = ToDxgiFormat(targetFormat);

    // Check mip levels
    int32 sourceMipLevels = (int32)currentImage->GetMetadata().mipLevels;
    bool hasSourceMipLevels = isPowerOfTwo && sourceMipLevels > 1;
    bool useMipLevels = isPowerOfTwo && (options.GenerateMipMaps || hasSourceMipLevels) && (width > 1 || height > 1);
    int32 arraySize = (int32)currentImage->GetMetadata().arraySize;
    int32 mipLevels = useMipLevels ? MipLevelsCount(width, height) : 1;
    if (useMipLevels && !options.GenerateMipMaps && mipLevels != sourceMipLevels)
    {
        errorMsg = String::Format(TEXT("Imported texture has not full mip chain, loaded mips count: {0}, expected: {1}"), sourceMipLevels, mipLevels);
        return true;
    }

    // Allocate memory for texture data
    auto& data = textureData.Items;
    data.Resize(arraySize);
    for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
    {
        auto& sliceData = data[arrayIndex];
        sliceData.Mips.Resize(mipLevels);
    }

    bool keepAsIs = false;
    if (!options.FlipY &&
        !options.FlipX &&
        !options.InvertGreenChannel &&
        !options.InvertRedChannel &&
        !options.InvertAlphaChannel &&
        !options.InvertBlueChannel &&
        !options.ReconstructZChannel &&
        options.Compress && 
        type == ImageType::DDS && 
        mipLevels == sourceMipLevels && 
        DirectX::IsCompressed(sourceDxgiFormat) && 
        !DirectX::IsSRGB(sourceDxgiFormat) && 
        width >= 4 && 
        height >= 4)
    {
        // Keep image in the current compressed format (artist choice) so we don't have to run the slow mipmap generation
        keepAsIs = true;
        targetDxgiFormat = sourceDxgiFormat;
        targetFormat = ::ToPixelFormat(currentImage->GetMetadata().format);
    }

    // Decompress if texture is compressed (next steps need decompressed input data, for eg. mip maps generation or format changing)
    if (!keepAsIs && DirectX::IsCompressed(sourceDxgiFormat))
    {
        auto& tmpImg = GET_TMP_IMG();
        sourceDxgiFormat = ToDecompressFormat(sourceDxgiFormat);
        result = Decompress(currentImage->GetImages(), currentImage->GetImageCount(), currentImage->GetMetadata(), sourceDxgiFormat, tmpImg);
        if (FAILED(result))
        {
            errorMsg = String::Format(TEXT("Cannot decompress texture, error: {0:x}"), static_cast<uint32>(result));
            return true;
        }
        SET_CURRENT_IMG(tmpImg);
    }

    // Fix sRGB problem
    if (!keepAsIs && DirectX::IsSRGB(sourceDxgiFormat))
    {
        sourceDxgiFormat = ToDxgiFormat(PixelFormatExtensions::ToNonsRGB(::ToPixelFormat(sourceDxgiFormat)));
        ((DirectX::TexMetadata&)currentImage->GetMetadata()).format = sourceDxgiFormat;
        for (size_t i = 0; i < currentImage->GetImageCount(); i++)
            ((DirectX::Image*)currentImage->GetImages())[i].format = sourceDxgiFormat;
    }

    // Remove alpha if source texture has it but output should not, valid for compressed output only (DirectX seams to use alpha to pre-multiply colors because BC1 format has no place for alpha)
    if (!keepAsIs && DirectX::HasAlpha(sourceDxgiFormat) && options.Type == TextureFormatType::ColorRGB && options.Compress)
    {
        auto& tmpImg = GET_TMP_IMG();
        result = TransformImage(currentImage->GetImages(), currentImage->GetImageCount(), currentImage->GetMetadata(),
                       [](DirectX::XMVECTOR* outPixels, const DirectX::XMVECTOR* inPixels, size_t width, size_t y)
                       {
                           UNREFERENCED_PARAMETER(y);
                           for (size_t j = 0; j < width; j++)
                           {
                               outPixels[j] = DirectX::XMVectorSelect(DirectX::g_XMOne, inPixels[j], DirectX::g_XMSelect1110);
                           }
                       }, tmpImg);
        if (FAILED(result))
        {
            errorMsg = String::Format(TEXT("Cannot transform texture to remove unwanted alpha channel, error: {0:x}"), static_cast<uint32>(result));
            return true;
        }
        SET_CURRENT_IMG(tmpImg);
    }

    // Check flip/rotate Y source image
    if (!keepAsIs && options.FlipY)
    {
        auto& tmpImg = GET_TMP_IMG();
        DirectX::TEX_FR_FLAGS flags = DirectX::TEX_FR_FLIP_VERTICAL;
        result = FlipRotate(currentImage->GetImages(), currentImage->GetImageCount(), currentImage->GetMetadata(), flags, tmpImg);
        if (FAILED(result))
        {
            errorMsg = String::Format(TEXT("Cannot rotate/flip texture, error: {0:x}"), static_cast<uint32>(result));
            return true;
        }
        SET_CURRENT_IMG(tmpImg);
    }

    // Check flip/rotate X source image
    if (!keepAsIs && options.FlipX)
    {
        auto& tmpImg = GET_TMP_IMG();
        DirectX::TEX_FR_FLAGS flags = DirectX::TEX_FR_FLIP_HORIZONTAL;
        result = FlipRotate(currentImage->GetImages(), currentImage->GetImageCount(), currentImage->GetMetadata(), flags, tmpImg);
        if (FAILED(result))
        {
            errorMsg = String::Format(TEXT("Cannot rotate/flip texture, error: {0:x}"), static_cast<uint32>(result));
            return true;
        }
        SET_CURRENT_IMG(tmpImg);
    }

    // Check if invert green channel
    if (!keepAsIs && options.InvertGreenChannel)
    {
        auto& timage = GET_TMP_IMG();
        result = TransformImage(currentImage->GetImages(), currentImage->GetImageCount(), currentImage->GetMetadata(),
            [&](DirectX::XMVECTOR* outPixels, const DirectX::XMVECTOR* inPixels, size_t w, size_t y)
            {
                const DirectX::XMVECTORU32 s_selecty = { { { DirectX::XM_SELECT_0, DirectX::XM_SELECT_1, DirectX::XM_SELECT_0, DirectX::XM_SELECT_0 } } };
                UNREFERENCED_PARAMETER(y);
                for (size_t j = 0; j < w; ++j)
                {
                    const DirectX::XMVECTOR value = inPixels[j];
                    const DirectX::XMVECTOR inverty = DirectX::XMVectorSubtract(DirectX::g_XMOne, value);
                    outPixels[j] = DirectX::XMVectorSelect(value, inverty, s_selecty);
                }
            }, timage);
        if (FAILED(result))
        {
            errorMsg = String::Format(TEXT("Cannot invert green channel in texture, error: {0:x}"), static_cast<uint32>(result));
            return true;
        }
        SET_CURRENT_IMG(timage);
    }
    
    // Check if invert red channel
    if (!keepAsIs && options.InvertRedChannel)
    {
        auto& timage = GET_TMP_IMG();
        result = TransformImage(currentImage->GetImages(), currentImage->GetImageCount(), currentImage->GetMetadata(),
            [&](DirectX::XMVECTOR* outPixels, const DirectX::XMVECTOR* inPixels, size_t w, size_t y)
            {
                const DirectX::XMVECTORU32 s_selectx = { { { DirectX::XM_SELECT_1, DirectX::XM_SELECT_0, DirectX::XM_SELECT_0, DirectX::XM_SELECT_0 } } };
                UNREFERENCED_PARAMETER(y);
                for (size_t j = 0; j < w; ++j)
                {
                    const DirectX::XMVECTOR value = inPixels[j];
                    const DirectX::XMVECTOR inverty = DirectX::XMVectorSubtract(DirectX::g_XMOne, value);
                    outPixels[j] = DirectX::XMVectorSelect(value, inverty, s_selectx);
                }
            }, timage);
        if (FAILED(result))
        {
            errorMsg = String::Format(TEXT("Cannot invert red channel in texture, error: {0:x}"), static_cast<uint32>(result));
            return true;
        }
        SET_CURRENT_IMG(timage);
    }

    // Check if invert blue channel
    if (!keepAsIs && options.InvertBlueChannel)
    {
        auto& timage = GET_TMP_IMG();
        result = TransformImage(currentImage->GetImages(), currentImage->GetImageCount(), currentImage->GetMetadata(),
            [&](DirectX::XMVECTOR* outPixels, const DirectX::XMVECTOR* inPixels, size_t w, size_t y)
            {
                const DirectX::XMVECTORU32 s_selectz = { { { DirectX::XM_SELECT_0, DirectX::XM_SELECT_0, DirectX::XM_SELECT_1, DirectX::XM_SELECT_0 } } };
                UNREFERENCED_PARAMETER(y);
                for (size_t j = 0; j < w; ++j)
                {
                    const DirectX::XMVECTOR value = inPixels[j];
                    const DirectX::XMVECTOR inverty = DirectX::XMVectorSubtract(DirectX::g_XMOne, value);
                    outPixels[j] = DirectX::XMVectorSelect(value, inverty, s_selectz);
                }
            }, timage);
        if (FAILED(result))
        {
            errorMsg = String::Format(TEXT("Cannot invert blue channel in texture, error: {0:x}"), static_cast<uint32>(result));
            return true;
        }
        SET_CURRENT_IMG(timage);
    }

    // Check if invert alpha channel
    if (!keepAsIs && options.InvertAlphaChannel)
    {
        auto& timage = GET_TMP_IMG();
        result = TransformImage(currentImage->GetImages(), currentImage->GetImageCount(), currentImage->GetMetadata(),
            [&](DirectX::XMVECTOR* outPixels, const DirectX::XMVECTOR* inPixels, size_t w, size_t y)
            {
                const DirectX::XMVECTORU32 s_selectw = { { { DirectX::XM_SELECT_0, DirectX::XM_SELECT_0, DirectX::XM_SELECT_0, DirectX::XM_SELECT_1 } } };
                UNREFERENCED_PARAMETER(y);
                for (size_t j = 0; j < w; ++j)
                {
                    const DirectX::XMVECTOR value = inPixels[j];
                    const DirectX::XMVECTOR inverty = DirectX::XMVectorSubtract(DirectX::g_XMOne, value);
                    outPixels[j] = DirectX::XMVectorSelect(value, inverty, s_selectw);
                }
            }, timage);
        if (FAILED(result))
        {
            errorMsg = String::Format(TEXT("Cannot invert alpha channel in texture, error: {0:x}"), static_cast<uint32>(result));
            return true;
        }
        SET_CURRENT_IMG(timage);
    }

    // Reconstruct Z Channel
    if (!keepAsIs & options.ReconstructZChannel)
    {
        auto& timage = GET_TMP_IMG();
        bool isunorm = (DirectX::FormatDataType(sourceDxgiFormat) == DirectX::FORMAT_TYPE_UNORM) != 0;
        result = TransformImage(currentImage->GetImages(), currentImage->GetImageCount(), currentImage->GetMetadata(),
            [&](DirectX::XMVECTOR* outPixels, const DirectX::XMVECTOR* inPixels, size_t w, size_t y)
            {
                const DirectX::XMVECTORU32 s_selectz = { { { DirectX::XM_SELECT_0, DirectX::XM_SELECT_0, DirectX::XM_SELECT_1, DirectX::XM_SELECT_0 } } };
                UNREFERENCED_PARAMETER(y);
                for (size_t j = 0; j < w; ++j)
                {
                    const DirectX::XMVECTOR value = inPixels[j];
                    DirectX::XMVECTOR z;
                    if (isunorm)
                    {
                        DirectX::XMVECTOR x2 = DirectX::XMVectorMultiplyAdd(value, DirectX::g_XMTwo, DirectX::g_XMNegativeOne);
                        x2 = DirectX::XMVectorSqrt(DirectX::XMVectorSubtract(DirectX::g_XMOne, DirectX::XMVector2Dot(x2, x2)));
                        z = DirectX::XMVectorMultiplyAdd(x2, DirectX::g_XMOneHalf, DirectX::g_XMOneHalf);
                    }
                    else
                    {
                        z = DirectX::XMVectorSqrt(DirectX::XMVectorSubtract(DirectX::g_XMOne, DirectX::XMVector2Dot(value, value)));
                    }
                    outPixels[j] = DirectX::XMVectorSelect(value, z, s_selectz);
                }
            }, timage);
        if (FAILED(result))
        {
            errorMsg = String::Format(TEXT("Cannot reconstruct z channel in texture, error: {0:x}"), static_cast<uint32>(result));
            return true;
        }
        SET_CURRENT_IMG(timage);
    }

    // Generate mip maps chain
    if (!keepAsIs && useMipLevels && options.GenerateMipMaps)
    {
        auto& tmpImg = GET_TMP_IMG();

        // Check if use custom filter (lightmaps are imported in Vector4 HDR format and generated mip maps by DirectXTex have some issues)
        if (sourceDxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT)
        {
            result = CustomGenerateMipMaps(currentImage->GetImages(), currentImage->GetImageCount(), currentImage->GetMetadata(), mipLevels, tmpImg);
        }
        else
        {
            result = GenerateMipMaps(currentImage->GetImages(), currentImage->GetImageCount(), currentImage->GetMetadata(), DirectX::TEX_FILTER_SEPARATE_ALPHA, mipLevels, tmpImg);
        }
        if (FAILED(result))
        {
            errorMsg = String::Format(TEXT("Cannot generate texture mip maps chain, error: {1:x}"), *path, static_cast<uint32>(result));
            return true;
        }
        SET_CURRENT_IMG(tmpImg);
    }

    // Preserve mipmap alpha coverage (if requested)
    if (!keepAsIs && DirectX::HasAlpha(currentImage->GetMetadata().format) && options.PreserveAlphaCoverage && useMipLevels)
    {
        auto& tmpImg = GET_TMP_IMG();

        auto& info = currentImage->GetMetadata();
        result = tmpImg.Initialize(info);
        if (FAILED(result))
        {
            errorMsg = String::Format(TEXT("Failed initialize image, error: {1:x}"), *path, static_cast<uint32>(result));
            return true;
        }

        for (size_t item = 0; item < info.arraySize; ++item)
        {
            auto img = currentImage->GetImage(0, item, 0);
            ASSERT(img);

            result = ScaleMipMapsAlphaForCoverage(img, info.mipLevels, info, item, options.PreserveAlphaCoverageReference, tmpImg);
            if (FAILED(result))
            {
                errorMsg = String::Format(TEXT("Failed to scale mip maps alpha for coverage, error: {1:x}"), *path, static_cast<uint32>(result));
                return true;
            }
        }

        SET_CURRENT_IMG(tmpImg);
    }

    // Ensure that there are some mip maps in the source texture
    ASSERT((int32)currentImage->GetMetadata().mipLevels >= mipLevels);

    // Compress mip maps or convert image
    if (!keepAsIs && targetDxgiFormat != sourceDxgiFormat)
    {
        auto& tmpImg = GET_TMP_IMG();

        if (DirectX::IsCompressed(targetDxgiFormat))
            result = ::Compress(currentImage->GetImages(), currentImage->GetImageCount(), currentImage->GetMetadata(), targetDxgiFormat, DirectX::TEX_COMPRESS_DEFAULT | DirectX::TEX_COMPRESS_PARALLEL, alphaThreshold, tmpImg);
        else
            result = DirectX::Convert(currentImage->GetImages(), currentImage->GetImageCount(), currentImage->GetMetadata(), targetDxgiFormat, DirectX::TEX_FILTER_DEFAULT, alphaThreshold, tmpImg);
        if (FAILED(result))
        {
            errorMsg = String::Format(TEXT("Cannot compress texture, error: {0:x}"), static_cast<uint32>(result));
            return true;
        }

        SET_CURRENT_IMG(tmpImg);
    }

    // Setup texture data header
    textureData.Width = width;
    textureData.Height = height;
    textureData.Depth = 1;
    textureData.Format = targetFormat;

    // Save texture data
    for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
    {
        for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
        {
            auto mipData = textureData.GetData(arrayIndex, mipIndex);
            auto image = currentImage->GetImage(mipIndex, arrayIndex, 0);
            if (image == nullptr)
            {
                errorMsg = String::Format(TEXT("Missing output image for mip{0} (array slice: {1})"), mipIndex, arrayIndex);
                return true;
            }

            mipData->DepthPitch = (uint32)image->slicePitch;
            mipData->RowPitch = (uint32)image->rowPitch;
            mipData->Lines = (uint32)image->height;
            mipData->Data.Copy(image->pixels, static_cast<uint32>(image->slicePitch));
        }

#if USE_EDITOR
        if (!hasAlpha)
            hasAlpha |= !currentImage->IsAlphaAllOpaque();
#endif
    }

    return false;
}

bool TextureTool::ConvertDirectXTex(TextureData& dst, const TextureData& src, const PixelFormat dstFormat)
{
    if (PixelFormatExtensions::IsCompressedASTC(dstFormat))
    {
#if COMPILE_WITH_ASTC
        return ConvertAstc(dst, src, dstFormat);
#else
        LOG(Error, "Missing ASTC texture format compression lib.");
        return true;
#endif
    }

    HRESULT result;
    DirectX::ScratchImage dstImage;
    DirectX::ScratchImage tmpImage;
    DirectX::ScratchImage srcImage;
    auto width = src.Width;
    auto height = src.Height;
    auto arraySize = src.GetArraySize();
    auto mipLevels = src.GetMipLevels();
    auto srcFormatDxgi = ToDxgiFormat(src.Format);
    auto dstFormatDxgi = ToDxgiFormat(dstFormat);

    // Prepare source data
    result = srcImage.Initialize2D(srcFormatDxgi, width, height, arraySize, mipLevels);
    if (FAILED(result))
    {
        LOG(Warning, "Cannot init source image. Error: {0:x}", static_cast<uint32>(result));
        return true;
    }
    for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
    {
        for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
        {
            const auto mipData = src.GetData(arrayIndex, mipIndex);
            auto image = srcImage.GetImage(mipIndex, arrayIndex, 0);
            if (image == nullptr)
            {
                LOG(Warning, "Missing source image for mip{0} (array slice: {1})", mipIndex, arrayIndex);
                return true;
            }

            // Copy data
            auto sptr = mipData->Data.Get();
            auto dptr = reinterpret_cast<uint8_t*>(image->pixels);
            size_t spitch = mipData->RowPitch;
            size_t dpitch = image->rowPitch;
            if (spitch == dpitch)
            {
                Platform::MemoryCopy(dptr, sptr, image->slicePitch);
            }
            else
            {
                const size_t size = Math::Min<size_t>(dpitch, spitch);
                for (size_t y = 0; y < mipData->Lines; y++)
                {
                    Platform::MemoryCopy(dptr, sptr, size);
                    sptr += spitch;
                    dptr += dpitch;
                }
            }
        }
    }

    // Allocate memory for texture data
    auto& data = dst.Items;
    data.Resize(arraySize);
    for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
    {
        auto& sliceData = data[arrayIndex];
        sliceData.Mips.Resize(mipLevels);
    }

    // Check if need to decompress data
    DirectX::ScratchImage* inImage = &srcImage;
    if (DirectX::IsCompressed(srcFormatDxgi))
    {
        result = DirectX::Decompress(srcImage.GetImages(), srcImage.GetImageCount(), srcImage.GetMetadata(), DXGI_FORMAT_UNKNOWN, tmpImage);
        if (FAILED(result))
        {
            LOG(Warning, "Cannot decompress image. Error: {0:x}", static_cast<uint32>(result));
            return true;
        }
        inImage = &tmpImage;
    }

    // Check if compress data
    DirectX::ScratchImage* outImage = &dstImage;
    if (DirectX::IsCompressed(dstFormatDxgi))
    {
        result = ::Compress(inImage->GetImages(), inImage->GetImageCount(), inImage->GetMetadata(), dstFormatDxgi, DirectX::TEX_COMPRESS_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, dstImage);
        if (FAILED(result))
        {
            LOG(Warning, "Cannot compress image. Error: {0:x}", static_cast<uint32>(result));
            return true;
        }
    }
    // Check if convert data
    else if (inImage->GetMetadata().format != dstFormatDxgi)
    {
        result = DirectX::Convert(inImage->GetImages(), inImage->GetImageCount(), inImage->GetMetadata(), dstFormatDxgi, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, dstImage);
        if (FAILED(result))
        {
            LOG(Warning, "Cannot convert image. Error: {0:x}", static_cast<uint32>(result));
            return true;
        }
    }
    else
    {
        // Use decompressed image output
        outImage = inImage;
    }

    // Save data
    for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
    {
        for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
        {
            auto mipData = dst.GetData(arrayIndex, mipIndex);
            auto image = outImage->GetImage(mipIndex, arrayIndex, 0);
            if (image == nullptr)
            {
                LOG(Warning, "Missing output image for mip{0} (array slice: {1})", mipIndex, arrayIndex);
                return true;
            }

            mipData->DepthPitch = (uint32)image->slicePitch;
            mipData->RowPitch = (uint32)image->rowPitch;
            mipData->Lines = (uint32)image->height;
            mipData->Data.Copy(image->pixels, static_cast<uint32>(image->slicePitch));
        }
    }

    // Setup texture data
    dst.Width = src.Width;
    dst.Height = src.Height;
    dst.Depth = src.Depth;
    dst.Format = dstFormat;

    return false;
}

bool TextureTool::ResizeDirectXTex(TextureData& dst, const TextureData& src, int32 dstWidth, int32 dstHeight)
{
    HRESULT result;
    DirectX::ScratchImage dstImage;
    DirectX::ScratchImage tmpImage;
    DirectX::ScratchImage srcImage;
    auto width = src.Width;
    auto height = src.Height;
    auto arraySize = src.GetArraySize();
    auto mipLevels = src.GetMipLevels();
    auto srcFormatDxgi = ToDxgiFormat(src.Format);

    // Prepare source data
    result = srcImage.Initialize2D(srcFormatDxgi, width, height, arraySize, mipLevels);
    if (FAILED(result))
    {
        LOG(Warning, "Cannot init source image. Error: {0:x}", static_cast<uint32>(result));
        return true;
    }
    for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
    {
        for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
        {
            const auto mipData = src.GetData(arrayIndex, mipIndex);
            auto image = srcImage.GetImage(mipIndex, arrayIndex, 0);
            if (image == nullptr)
            {
                LOG(Warning, "Missing source image for mip{0} (array slice: {1})", mipIndex, arrayIndex);
                return true;
            }

            // Copy data
            auto sptr = mipData->Data.Get();
            auto dptr = reinterpret_cast<uint8_t*>(image->pixels);
            size_t spitch = mipData->RowPitch;
            size_t dpitch = image->rowPitch;
            if (spitch == dpitch)
            {
                Platform::MemoryCopy(dptr, sptr, image->slicePitch);
            }
            else
            {
                const size_t size = Math::Min<size_t>(dpitch, spitch);
                for (size_t y = 0; y < mipData->Lines; y++)
                {
                    Platform::MemoryCopy(dptr, sptr, size);
                    sptr += spitch;
                    dptr += dpitch;
                }
            }
        }
    }

    // Resize texture
    DirectX::ScratchImage* inImage = &srcImage;
    DirectX::ScratchImage* outImage = &dstImage;
    result = DirectX::Resize(inImage->GetImages(), inImage->GetImageCount(), inImage->GetMetadata(), dstWidth, dstHeight, DirectX::TEX_FILTER_DEFAULT, dstImage);
    if (FAILED(result))
    {
        LOG(Warning, "Cannot resize image. Error: {0:x}", static_cast<uint32>(result));
        return true;
    }

    // Generate missing mipmaps if the input image had any
    DirectX::ScratchImage mipsImage;
    if (outImage->GetMetadata().mipLevels == 1 && mipLevels != 1)
    {
        result = DirectX::GenerateMipMaps(*dstImage.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, 0, mipsImage);
        if (FAILED(result))
        {
            LOG(Warning, "Cannot generate mip maps. Error: {0:x}", static_cast<uint32>(result));
            return true;
        }
        outImage = &mipsImage;
    }
    mipLevels = (int32)outImage->GetMetadata().mipLevels;

    // Allocate memory for texture data
    auto& data = dst.Items;
    data.Resize(arraySize);
    for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
    {
        auto& sliceData = data[arrayIndex];
        sliceData.Mips.Resize(mipLevels);
    }

    // Save data
    for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
    {
        for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
        {
            auto mipData = dst.GetData(arrayIndex, mipIndex);
            const auto image = outImage->GetImage(mipIndex, arrayIndex, 0);
            if (image == nullptr)
            {
                LOG(Warning, "Missing output image for mip{0} (array slice: {1})", mipIndex, arrayIndex);
                return true;
            }

            mipData->DepthPitch = (uint32)image->slicePitch;
            mipData->RowPitch = (uint32)image->rowPitch;
            mipData->Lines = (uint32)image->height;
            mipData->Data.Copy(image->pixels, static_cast<uint32>(image->slicePitch));
        }
    }

    // Setup texture data
    dst.Width = dstWidth;
    dst.Height = dstHeight;
    dst.Depth = src.Depth;
    dst.Format = src.Format;

    return false;
}

#endif
