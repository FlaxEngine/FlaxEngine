// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "GPUTexture.h"
#include "GPUTextureDescription.h"
#include "TextureData.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Graphics/Config.h"
#include "Engine/Graphics/Async/Tasks/GPUCopyResourceTask.h"
#include "Engine/Graphics/Async/Tasks/GPUUploadTextureMipTask.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Threading/ThreadPoolTask.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/Enums.h"

namespace
{
    int32 CalculateMipMapCount(int32 requestedLevel, int32 width)
    {
        //int32 size = width;
        //int32 maxMipMap = 1 + Math::CeilToInt(Math::Log(size) / Math::Log(2.0));
        //return requestedLevel == 0 ? maxMipMap : Math::Min(requestedLevel, maxMipMap);
        if (requestedLevel == 0)
            requestedLevel = GPU_MAX_TEXTURE_MIP_LEVELS;
        int32 maxMipMap = 1;
        while (width > 1)
        {
            width >>= 1;
            maxMipMap++;
        }
        return Math::Min(requestedLevel, maxMipMap);
    }
}

const Char* ToString(TextureDimensions value)
{
    const Char* result;
    switch (value)
    {
    case TextureDimensions::Texture:
        result = TEXT("Texture");
        break;
    case TextureDimensions::VolumeTexture:
        result = TEXT("VolumeTexture");
        break;
    case TextureDimensions::CubeTexture:
        result = TEXT("CubeTexture");
        break;
    default:
        result = TEXT("");
    }
    return result;
}

GPUTextureDescription GPUTextureDescription::New1D(int32 width, PixelFormat format, GPUTextureFlags textureFlags, int32 mipCount, int32 arraySize)
{
    GPUTextureDescription desc;
    desc.Dimensions = TextureDimensions::Texture;
    desc.Width = width;
    desc.Height = 1;
    desc.Depth = 1;
    desc.ArraySize = arraySize;
    desc.MipLevels = CalculateMipMapCount(mipCount, width);
    desc.Format = format;
    desc.MultiSampleLevel = MSAALevel::None;
    desc.Flags = textureFlags;
    desc.Usage = GPUResourceUsage::Default;
    desc.DefaultClearColor = Color::Black;
    return desc;
}

GPUTextureDescription GPUTextureDescription::New2D(int32 width, int32 height, PixelFormat format, GPUTextureFlags textureFlags, int32 mipCount, int32 arraySize, MSAALevel msaaLevel)
{
    GPUTextureDescription desc;
    desc.Dimensions = TextureDimensions::Texture;
    desc.Width = width;
    desc.Height = height;
    desc.Depth = 1;
    desc.ArraySize = arraySize;
    desc.MipLevels = CalculateMipMapCount(mipCount, Math::Max(width, height));
    desc.Format = format;
    desc.MultiSampleLevel = msaaLevel;
    desc.Flags = textureFlags;
    desc.DefaultClearColor = Color::Black;
    desc.Usage = GPUResourceUsage::Default;
    return desc;
}

GPUTextureDescription GPUTextureDescription::New3D(const Float3& size, PixelFormat format, GPUTextureFlags textureFlags)
{
    return New3D((int32)size.X, (int32)size.Y, (int32)size.Z, 1, format, textureFlags);
}

GPUTextureDescription GPUTextureDescription::New3D(int32 width, int32 height, int32 depth, PixelFormat format, GPUTextureFlags textureFlags, int32 mipCount)
{
    GPUTextureDescription desc;
    desc.Dimensions = TextureDimensions::VolumeTexture;
    desc.Width = width;
    desc.Height = height;
    desc.Depth = depth;
    desc.ArraySize = 1;
    desc.MipLevels = CalculateMipMapCount(mipCount, Math::Max(width, height, depth));
    desc.Format = format;
    desc.MultiSampleLevel = MSAALevel::None;
    desc.Flags = textureFlags;
    desc.DefaultClearColor = Color::Black;
    desc.Usage = GPUResourceUsage::Default;
    return desc;
}

GPUTextureDescription GPUTextureDescription::NewCube(int32 size, PixelFormat format, GPUTextureFlags textureFlags, int32 mipCount)
{
    auto desc = New2D(size, size, format, textureFlags, mipCount, 6, MSAALevel::None);
    desc.Dimensions = TextureDimensions::CubeTexture;
    return desc;
}

void GPUTextureDescription::Clear()
{
    Platform::MemoryClear(this, sizeof(GPUTextureDescription));
    MultiSampleLevel = MSAALevel::None;
}

GPUTextureDescription GPUTextureDescription::ToStagingUpload() const
{
    auto copy = *this;
    copy.Flags = GPUTextureFlags::None;
    copy.Usage = GPUResourceUsage::StagingUpload;
    return copy;
}

GPUTextureDescription GPUTextureDescription::ToStagingReadback() const
{
    auto copy = *this;
    copy.Flags = GPUTextureFlags::None;
    copy.Usage = GPUResourceUsage::StagingReadback;
    return copy;
}

GPUTextureDescription GPUTextureDescription::ToStaging() const
{
    auto copy = *this;
    copy.Flags = GPUTextureFlags::None;
    copy.Usage = GPUResourceUsage::Staging;
    return copy;
}

bool GPUTextureDescription::Equals(const GPUTextureDescription& other) const
{
    return Dimensions == other.Dimensions
            && Width == other.Width
            && Height == other.Height
            && Depth == other.Depth
            && ArraySize == other.ArraySize
            && MipLevels == other.MipLevels
            && Format == other.Format
            && MultiSampleLevel == other.MultiSampleLevel
            && Flags == other.Flags
            && Usage == other.Usage
            && Color::NearEqual(DefaultClearColor, other.DefaultClearColor);
}

String GPUTextureDescription::ToString() const
{
    return String::Format(TEXT("Size: {0}x{1}x{2}[{3}], Type: {4}, Mips: {5}, Format: {6}, MSAA: {7}, Flags: {8}, Usage: {9}"),
                          Width,
                          Height,
                          Depth,
                          ArraySize,
                          ::ToString(Dimensions),
                          MipLevels,
                          ScriptingEnum::ToString(Format),
                          ::ToString(MultiSampleLevel),
                          ScriptingEnum::ToStringFlags(Flags),
                          (int32)Usage);
}

uint32 GetHash(const GPUTextureDescription& key)
{
    uint32 hashCode = key.Width;
    hashCode = (hashCode * 397) ^ key.Height;
    hashCode = (hashCode * 397) ^ key.Depth;
    hashCode = (hashCode * 397) ^ key.ArraySize;
    hashCode = (hashCode * 397) ^ (uint32)key.Dimensions;
    hashCode = (hashCode * 397) ^ key.MipLevels;
    hashCode = (hashCode * 397) ^ (uint32)key.Format;
    hashCode = (hashCode * 397) ^ (uint32)key.MultiSampleLevel;
    hashCode = (hashCode * 397) ^ (uint32)key.Flags;
    hashCode = (hashCode * 397) ^ (uint32)key.Usage;
    hashCode = (hashCode * 397) ^ key.DefaultClearColor.GetHashCode();
    return hashCode;
}

GPUTexture* GPUTexture::Spawn(const SpawnParams& params)
{
    return GPUDevice::Instance->CreateTexture();
}

GPUTexture* GPUTexture::New()
{
    return GPUDevice::Instance->CreateTexture();
}

GPUTexture::GPUTexture()
    : GPUResource(SpawnParams(Guid::New(), TypeInitializer))
    , _residentMipLevels(0)
    , _sRGB(false)
    , _isBlockCompressed(false)
{
    // Keep description data clear (we use _desc.MipLevels to check if it's has been initated)
    _desc.Clear();
}

bool GPUTexture::IsStaging() const
{
    return _desc.Usage == GPUResourceUsage::StagingUpload || _desc.Usage == GPUResourceUsage::StagingReadback || _desc.Usage == GPUResourceUsage::Staging;
}

Float2 GPUTexture::Size() const
{
    return Float2(static_cast<float>(_desc.Width), static_cast<float>(_desc.Height));
}

Float3 GPUTexture::Size3() const
{
    return Float3(static_cast<float>(_desc.Width), static_cast<float>(_desc.Height), static_cast<float>(_desc.Depth));
}

bool GPUTexture::IsPowerOfTwo() const
{
    return Math::IsPowerOfTwo(_desc.Width) && Math::IsPowerOfTwo(_desc.Height);
}

void GPUTexture::GetMipSize(int32 mipLevelIndex, int32& mipWidth, int32& mipHeight) const
{
    ASSERT(mipLevelIndex >= 0 && mipLevelIndex < MipLevels());
    mipWidth = Math::Max(1, Width() >> mipLevelIndex);
    mipHeight = Math::Max(1, Height() >> mipLevelIndex);
}

void GPUTexture::GetMipSize(int32 mipLevelIndex, int32& mipWidth, int32& mipHeight, int32& mipDepth) const
{
    ASSERT(mipLevelIndex >= 0 && mipLevelIndex < MipLevels());
    mipWidth = Math::Max(1, Width() >> mipLevelIndex);
    mipHeight = Math::Max(1, Height() >> mipLevelIndex);
    mipDepth = Math::Max(1, Depth() >> mipLevelIndex);
}

void GPUTexture::GetResidentSize(int32& width, int32& height) const
{
    // Check if texture isn't loaded
    if (_residentMipLevels <= 0)
    {
        width = height = 0;
        return;
    }

    const int32 mipIndex = _desc.MipLevels - _residentMipLevels;
    width = Width() >> mipIndex;
    height = Height() >> mipIndex;
}

void GPUTexture::GetResidentSize(int32& width, int32& height, int32& depth) const
{
    // Check if texture isn't loaded
    if (_residentMipLevels <= 0)
    {
        width = height = depth = 0;
        return;
    }

    const int32 mipIndex = _desc.MipLevels - _residentMipLevels;
    width = Width() >> mipIndex;
    height = Height() >> mipIndex;
    depth = Depth() >> mipIndex;
}

uint32 GPUTexture::RowPitch(int32 mipIndex) const
{
    uint32 rowPitch, slicePitch;
    ComputePitch(mipIndex, rowPitch, slicePitch);
    return rowPitch;
}

uint32 GPUTexture::SlicePitch(int32 mipIndex) const
{
    uint32 rowPitch, slicePitch;
    ComputePitch(mipIndex, rowPitch, slicePitch);
    return slicePitch;
}

void GPUTexture::ComputePitch(int32 mipIndex, uint32& rowPitch, uint32& slicePitch) const
{
    int32 mipWidth, mipHeight;
    GetMipSize(mipIndex, mipWidth, mipHeight);
    RenderTools::ComputePitch(Format(), mipWidth, mipHeight, rowPitch, slicePitch);
}

int32 GPUTexture::CalculateMipSize(int32 size, int32 mipLevel) const
{
    mipLevel = Math::Min(mipLevel, MipLevels());
    return Math::Max(1, size >> mipLevel);
}

int32 GPUTexture::ComputeSubresourceSize(int32 subresource, int32 rowAlign, int32 sliceAlign) const
{
    const int32 mipLevel = subresource % MipLevels();
    const int32 slicePitch = ComputeSlicePitch(mipLevel, rowAlign);
    const int32 depth = CalculateMipSize(Depth(), mipLevel);
    return Math::AlignUp<int32>(slicePitch * depth, sliceAlign);
}

int32 GPUTexture::ComputeBufferOffset(int32 subresource, int32 rowAlign, int32 sliceAlign) const
{
    int32 offset = 0;
    for (int32 i = 0; i < subresource; i++)
    {
        offset += ComputeSubresourceSize(i, rowAlign, sliceAlign);
    }
    return offset;
}

int32 GPUTexture::ComputeBufferTotalSize(int32 rowAlign, int32 sliceAlign) const
{
    int32 result = 0;
    for (int32 mipLevel = 0; mipLevel < MipLevels(); mipLevel++)
    {
        const int32 slicePitch = ComputeSlicePitch(mipLevel, rowAlign);
        const int32 depth = CalculateMipSize(Depth(), mipLevel);
        result += Math::AlignUp<int32>(slicePitch * depth, sliceAlign);
    }
    return result * ArraySize();
}

int32 GPUTexture::ComputeSlicePitch(int32 mipLevel, int32 rowAlign) const
{
    return ComputeRowPitch(mipLevel, rowAlign) * CalculateMipSize(Height(), mipLevel);
}

int32 GPUTexture::ComputeRowPitch(int32 mipLevel, int32 rowAlign) const
{
    int32 mipWidth = CalculateMipSize(Width(), mipLevel);
    int32 mipHeight = CalculateMipSize(Height(), mipLevel);
    uint32 rowPitch, slicePitch;
    RenderTools::ComputePitch(Format(), mipWidth, mipHeight, rowPitch, slicePitch);
    return Math::AlignUp<int32>(rowPitch, rowAlign);
}

bool GPUTexture::Init(const GPUTextureDescription& desc)
{
    // Validate description
    const auto device = GPUDevice::Instance;
    if (desc.Usage == GPUResourceUsage::Dynamic)
    {
        LOG(Warning, "Cannot create texture. Dynamic textures are not supported. Description: {0}", desc.ToString());
        return true;
    }
    if (desc.MipLevels < 0 || desc.MipLevels > GPU_MAX_TEXTURE_MIP_LEVELS)
    {
        LOG(Warning, "Cannot create texture. Invalid amount of mip levels. Description: {0}", desc.ToString());
        return true;
    }
    if (desc.IsDepthStencil())
    {
        if (desc.MipLevels > 1)
        {
            LOG(Warning, "Cannot create texture. Depth Stencil texture cannot have mip maps. Description: {0}", desc.ToString());
            return true;
        }
        if (desc.IsRenderTarget())
        {
            LOG(Warning, "Cannot create texture. Depth Stencil texture cannot be used as a Render Target. Description: {0}", desc.ToString());
            return true;
        }
        if (EnumHasAnyFlags(desc.Flags, GPUTextureFlags::ReadOnlyDepthView) && !device->Limits.HasReadOnlyDepth)
        {
            LOG(Warning, "Cannot create texture. The current graphics platform does not support read-only Depth Stencil texture. Description: {0}", desc.ToString());
            return true;
        }
    }
    else
    {
        if (EnumHasAnyFlags(desc.Flags, GPUTextureFlags::ReadOnlyDepthView))
        {
            LOG(Warning, "Cannot create texture. Cannot create read-only Depth Stencil texture that is not a Depth Stencil texture. Add DepthStencil flag. Description: {0}", desc.ToString());
            return true;
        }
    }
    if (desc.HasPerMipViews() && !(desc.IsShaderResource() || desc.IsRenderTarget()))
    {
        LOG(Warning, "Cannot create texture. Depth Stencil texture cannot have mip maps. Description: {0}", desc.ToString());
        return true;
    }
    switch (desc.Dimensions)
    {
    case TextureDimensions::Texture:
    {
        if (desc.HasPerSliceViews())
        {
            LOG(Warning, "Cannot create texture. Texture cannot have per slice views. Description: {0}", desc.ToString());
            return true;
        }
        if (desc.Width <= 0 || desc.Height <= 0 || desc.ArraySize <= 0
            || desc.Width > device->Limits.MaximumTexture2DSize
            || desc.Height > device->Limits.MaximumTexture2DSize
            || desc.ArraySize > device->Limits.MaximumTexture2DArraySize)
        {
            LOG(Warning, "Cannot create texture. Invalid dimensions. Description: {0}", desc.ToString());
            return true;
        }

        break;
    }
    case TextureDimensions::VolumeTexture:
    {
        if (desc.IsDepthStencil())
        {
            LOG(Warning, "Cannot create texture. Only 2D Texture can be used as a Depth Stencil. Description: {0}", desc.ToString());
            return true;
        }
        if (desc.ArraySize != 1)
        {
            LOG(Warning, "Cannot create texture. Volume texture cannot create array of volume textures. Description: {0}", desc.ToString());
            return true;
        }
        if (desc.MultiSampleLevel != MSAALevel::None)
        {
            LOG(Warning, "Cannot create texture. Volume texture cannot use multi-sampling. Description: {0}", desc.ToString());
            return true;
        }
        if (desc.HasPerMipViews())
        {
            LOG(Warning, "Cannot create texture. Volume texture cannot have per mip map views. Description: {0}", desc.ToString());
            return true;
        }
        if (desc.HasPerMipViews() && desc.IsRenderTarget() == false)
        {
            LOG(Warning, "Cannot create texture. Volume texture cannot have per slice map views if is not a render target. Description: {0}", desc.ToString());
            return true;
        }
        if (desc.Width <= 0 || desc.Height <= 0 || desc.Depth <= 0
            || desc.Width > device->Limits.MaximumTexture3DSize
            || desc.Height > device->Limits.MaximumTexture3DSize
            || desc.Depth > device->Limits.MaximumTexture3DSize)
        {
            LOG(Warning, "Cannot create texture. Invalid dimensions. Description: {0}", desc.ToString());
            return true;
        }

        break;
    }
    case TextureDimensions::CubeTexture:
    {
        if (desc.HasPerSliceViews())
        {
            LOG(Warning, "Cannot create texture. Cube texture cannot have per slice views. Description: {0}", desc.ToString());
            return true;
        }
        if (desc.Width <= 0 || desc.ArraySize <= 0
            || desc.Width > device->Limits.MaximumTextureCubeSize
            || desc.Height > device->Limits.MaximumTextureCubeSize
            || desc.ArraySize * 6 > device->Limits.MaximumTexture2DArraySize
            || desc.Width != desc.Height)
        {
            LOG(Warning, "Cannot create texture. Invalid dimensions. Description: {0}", desc.ToString());
            return true;
        }

        break;
    }
    }
    const bool isCompressed = PixelFormatExtensions::IsCompressed(desc.Format);
    if (isCompressed)
    {
        const int32 blockSize = PixelFormatExtensions::ComputeBlockSize(desc.Format);
        if (desc.Width < blockSize || desc.Height < blockSize)
        {
            LOG(Warning, "Cannot create texture. Invalid dimensions. Description: {0}", desc.ToString());
            return true;
        }
    }

    // Release previous data
    ReleaseGPU();

    // Initialize
    _desc = desc;
    _sRGB = PixelFormatExtensions::IsSRGB(desc.Format);
    _isBlockCompressed = isCompressed;
    if (OnInit())
    {
        ReleaseGPU();
        _desc.Clear();
        _residentMipLevels = 0;
        LOG(Warning, "Cannot initialize texture. Description: {0}", desc.ToString());
        return true;
    }

    // Render targets and depth buffers doesn't support normal textures streaming and are considered to be always resident
    if (IsRegularTexture() == false)
    {
        _residentMipLevels = MipLevels();
    }

    return false;
}

GPUTexture* GPUTexture::ToStagingReadback() const
{
    const auto desc = _desc.ToStagingReadback();
    ScopeLock gpuLock(GPUDevice::Instance->Locker);
    auto* staging = GPUDevice::Instance->CreateTexture(TEXT("Staging.Readback"));
    if (staging->Init(desc))
    {
        Delete(staging);
        return nullptr;
    }
    return staging;
}

GPUTexture* GPUTexture::ToStagingUpload() const
{
    const auto desc = _desc.ToStagingUpload();
    ScopeLock gpuLock(GPUDevice::Instance->Locker);
    auto* staging = GPUDevice::Instance->CreateTexture(TEXT("Staging.Upload"));
    if (staging->Init(desc))
    {
        Delete(staging);
        return nullptr;
    }
    return staging;
}

bool GPUTexture::Resize(int32 width, int32 height, int32 depth, PixelFormat format)
{
    PROFILE_CPU();
    if (!IsAllocated())
    {
        LOG(Warning, "Cannot resize not created textures.");
        return true;
    }

    auto desc = GetDescription();
    if (format == PixelFormat::Unknown)
        format = desc.Format;

    // Skip if size won't change
    if (desc.Width == width && desc.Height == height && desc.Depth == depth && desc.Format == format)
        return false;

    desc.Format = format;
    desc.Width = width;
    desc.Height = height;
    desc.Depth = depth;
    if (desc.MipLevels > 1)
        desc.MipLevels = CalculateMipMapCount(0, Math::Max(width, height));

    // Recreate
    return Init(desc);
}

uint64 GPUTexture::calculateMemoryUsage() const
{
    return RenderTools::CalculateTextureMemoryUsage(Format(), Width(), Height(), Depth(), MipLevels()) * ArraySize();
}

String GPUTexture::ToString() const
{
#if GPU_ENABLE_RESOURCE_NAMING
    return String::Format(TEXT("Texture {0}, Residency: {1}, Name: {2}"), _desc.ToString(), _residentMipLevels, GetName());
#else
	return TEXT("Texture");
#endif
}

GPUResourceType GPUTexture::GetResourceType() const
{
    if (IsVolume())
        return GPUResourceType::VolumeTexture;
    if (IsCubeMap())
        return GPUResourceType::CubeTexture;
    return IsRegularTexture() ? GPUResourceType::Texture : GPUResourceType::RenderTarget;
}

void GPUTexture::OnReleaseGPU()
{
    _desc.Clear();
    _residentMipLevels = 0;
}

GPUTask* GPUTexture::UploadMipMapAsync(const BytesContainer& data, int32 mipIndex, bool copyData)
{
    uint32 rowPitch, slicePitch;
    ComputePitch(mipIndex, rowPitch, slicePitch);
    return UploadMipMapAsync(data, mipIndex, rowPitch, slicePitch, copyData);
}

GPUTask* GPUTexture::UploadMipMapAsync(const BytesContainer& data, int32 mipIndex, int32 rowPitch, int32 slicePitch, bool copyData)
{
    PROFILE_CPU();
    ASSERT(IsAllocated());
    ASSERT(mipIndex < MipLevels() && data.IsValid());
    ASSERT(data.Length() >= slicePitch);

    // Optimize texture upload invoked during rendering
    if (IsInMainThread() && GPUDevice::Instance->IsRendering())
    {
        // Update all array slices
        const byte* dataSource = data.Get();
        for (int32 arrayIndex = 0; arrayIndex < _desc.ArraySize; arrayIndex++)
        {
            GPUDevice::Instance->GetMainContext()->UpdateTexture(this, arrayIndex, mipIndex, dataSource, rowPitch, slicePitch);
            dataSource += slicePitch;
        }
        if (mipIndex == HighestResidentMipIndex() - 1)
        {
            // Mark as mip loaded
            SetResidentMipLevels(ResidentMipLevels() + 1);
        }
        return nullptr;
    }

    auto task = ::New<GPUUploadTextureMipTask>(this, mipIndex, data, rowPitch, slicePitch, copyData);
    ASSERT_LOW_LAYER(task && task->HasReference(this));
    return task;
}

class TextureDownloadDataTask : public ThreadPoolTask
{
private:
    GPUTextureReference _texture;
    GPUTexture* _staging;
    TextureData* _data;
    bool _deleteStaging;

public:
    TextureDownloadDataTask(GPUTexture* texture, GPUTexture* staging, TextureData& data)
        : _texture(texture)
        , _staging(staging)
        , _data(&data)
    {
        _deleteStaging = texture != staging;
    }

    ~TextureDownloadDataTask()
    {
        if (_deleteStaging && _staging)
            _staging->DeleteObjectNow();
    }

public:
    // [ThreadPoolTask]
    bool HasReference(Object* resource) const override
    {
        return _texture == resource || _staging == resource;
    }

protected:
    // [ThreadPoolTask]
    bool Run() override
    {
        auto texture = _texture.Get();
        if (texture == nullptr || _staging == nullptr || _data == nullptr)
        {
            LOG(Warning, "Cannot download texture data. Missing objects.");
            return true;
        }
        return _staging->DownloadData(*_data);
    }

    void OnEnd() override
    {
        _texture.Unlink();

        // Base
        ThreadPoolTask::OnEnd();
    }
};

bool GPUTexture::DownloadData(TextureData& result)
{
    // Skip for empty ones
    if (MipLevels() == 0)
    {
        LOG(Warning, "Cannot download GPU texture data from an empty texture.");
        return true;
    }
    if (Depth() != 1)
    {
        MISSING_CODE("support volume texture data downloading.");
    }
    PROFILE_CPU();

    // Use faster path for staging resources
    if (IsStaging())
    {
        const auto arraySize = ArraySize();
        const auto mipLevels = MipLevels();

        // Set texture info
        result.Width = Width();
        result.Height = Height();
        result.Depth = Depth();
        result.Format = Format();

        // Get all mip maps for each array slice
        auto& rawResultData = result.Items;
        rawResultData.Resize(arraySize, false);
        for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
        {
            auto& arraySlice = rawResultData[arrayIndex];
            arraySlice.Mips.Resize(mipLevels);

            for (int32 mipMapIndex = 0; mipMapIndex < mipLevels; mipMapIndex++)
            {
                auto& mip = arraySlice.Mips[mipMapIndex];
                const int32 mipWidth = result.Width >> mipMapIndex;
                const int32 mipHeight = result.Height >> mipMapIndex;
                uint32 mipRowPitch, mipSlicePitch;
                RenderTools::ComputePitch(result.Format, mipWidth, mipHeight, mipRowPitch, mipSlicePitch);

                // Gather data
                if (GetData(arrayIndex, mipMapIndex, mip, mipRowPitch))
                {
                    LOG(Warning, "Staging resource of \'{0}\' get data failed.", ToString());
                    return true;
                }
            }
        }

        return false;
    }

    const auto name = ToString();

    // Ensure not running on main thread - we support DownloadData from textures only on a worker threads (Thread Pool Workers or Content Loaders)
    if (IsInMainThread())
    {
        LOG(Warning, "Downloading GPU texture data from the main thread is not supported.");
        return true;
    }

    // Create async task
    auto task = DownloadDataAsync(result);
    if (task == nullptr)
    {
        LOG(Warning, "Cannot create async download task for resource {0}.", name);
        return true;
    }

    // Wait for work to be done
    task->Start();
    if (task->Wait())
    {
        LOG(Warning, "Resource \'{0}\' copy failed.", name);
        return true;
    }

    return false;
}

Task* GPUTexture::DownloadDataAsync(TextureData& result)
{
    // Skip for empty ones
    if (MipLevels() == 0)
    {
        LOG(Warning, "Cannot download texture data. It has not ben created yet.");
        return nullptr;
    }
    if (Depth() != 1)
    {
        MISSING_CODE("support volume texture data downloading.");
    }
    PROFILE_CPU();

    // Use faster path for staging resources
    if (IsStaging())
    {
        // Create task to copy downloaded data to TextureData container
        auto getDataTask = ::New<TextureDownloadDataTask>(this, this, result);
        ASSERT(getDataTask->HasReference(this));
        return getDataTask;
    }

    // Create the staging resource
    const auto staging = ToStagingReadback();
    if (staging == nullptr)
    {
        LOG(Error, "Cannot create staging resource from {0}.", ToString());
        return nullptr;
    }

    // Create async resource copy task
    auto copyTask = ::New<GPUCopyResourceTask>(this, staging);
    ASSERT(copyTask->HasReference(this) && copyTask->HasReference(staging));

    // Create task to copy downloaded data to TextureData container
    auto getDataTask = ::New<TextureDownloadDataTask>(this, staging, result);
    ASSERT(getDataTask->HasReference(this) && getDataTask->HasReference(staging));

    // Set continuation
    copyTask->ContinueWith(getDataTask);

    return copyTask;
}

void GPUTexture::SetResidentMipLevels(int32 count)
{
    count = Math::Clamp(count, 0, MipLevels());
    if (_residentMipLevels == count || !IsRegularTexture())
        return;
    _residentMipLevels = count;
    OnResidentMipsChanged();
    ResidentMipsChanged(this);
}
