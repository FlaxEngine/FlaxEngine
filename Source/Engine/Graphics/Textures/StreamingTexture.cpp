// Copyright (c) Wojciech Figat. All rights reserved.

#include "StreamingTexture.h"
#include "Engine/Core/Log.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Streaming/StreamingGroup.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Async/Tasks/GPUUploadTextureMipTask.h"
#include "Engine/Scripting/Enums.h"

TextureHeader_Deprecated::TextureHeader_Deprecated()
{
    Platform::MemoryClear(this, sizeof(*this));
}

TextureHeader::TextureHeader()
{
    Platform::MemoryClear(this, sizeof(*this));
    TextureGroup = -1;
}

TextureHeader::TextureHeader(const TextureHeader_Deprecated& old)
{
    Platform::MemoryClear(this, sizeof(*this));
    Width = old.Width;
    Height = old.Height;
    MipLevels = old.MipLevels;
    Format = old.Format;
    Type = old.Type;
    if (old.IsCubeMap)
        IsCubeMap = 1;
    if (old.IsSRGB)
        IsSRGB = 1;
    if (old.NeverStream)
        NeverStream = 1;
    TextureGroup = -1;
    Platform::MemoryCopy(CustomData, old.CustomData, sizeof(CustomData));
}

static_assert(sizeof(TextureHeader_Deprecated) == 10 * sizeof(int32), "Invalid TextureHeader size.");
static_assert(sizeof(TextureHeader) == 36, "Invalid TextureHeader size.");

StreamingTexture::StreamingTexture(ITextureOwner* parent, const String& name)
    : StreamableResource(StreamingGroups::Instance()->Textures())
    , _owner(parent)
    , _texture(nullptr)
    , _isBlockCompressed(false)
{
    ASSERT(parent != nullptr);

    // Always have created texture object
    ASSERT(GPUDevice::Instance);
    _texture = GPUDevice::Instance->CreateTexture(name);
    ASSERT(_texture != nullptr);

    _header.MipLevels = 0;
}

StreamingTexture::~StreamingTexture()
{
    UnloadTexture();
    SAFE_DELETE(_texture);
}

Float2 StreamingTexture::Size() const
{
    return _texture->Size();
}

int32 StreamingTexture::TextureMipIndexToTotalIndex(int32 textureMipIndex) const
{
    const int32 missingMips = TotalMipLevels() - _texture->MipLevels();
    return textureMipIndex + missingMips;
}

int32 StreamingTexture::TotalIndexToTextureMipIndex(int32 mipIndex) const
{
    const int32 missingMips = TotalMipLevels() - _texture->MipLevels();
    return mipIndex - missingMips;
}

bool StreamingTexture::Create(const TextureHeader& header)
{
    // Validate header (further validation is performed by the Texture.Init)
    if (header.MipLevels > GPU_MAX_TEXTURE_MIP_LEVELS
        || Math::IsNotInRange(header.Width, 1, GPU_MAX_TEXTURE_SIZE)
        || Math::IsNotInRange(header.Height, 1, GPU_MAX_TEXTURE_SIZE)
    )
    {
        LOG(Warning, "Invalid texture header.");
        return true;
    }

    ASSERT(_texture);

    ScopeLock lock(_owner->GetOwnerLocker());

    if (IsInitialized())
    {
        _texture->ReleaseGPU();
    }

    // Cache header
    // Note: by caching header we assume that streaming texture has been initialized.
    // Then we can start it's streaming so it may be allocated later (using texture.Init)
    // But this may not happen because resources may be created and loaded but not always allocated.
    // That's one of the main advantages of the current resources streaming system.
    _header = header;
    _isBlockCompressed = PixelFormatExtensions::IsCompressed(_header.Format);
    if (_isBlockCompressed)
    {
        // Ensure that streaming doesn't go too low because the hardware expects the texture to be min in size of compressed texture block
        const int32 blockSize = PixelFormatExtensions::ComputeBlockSize(_header.Format);
        int32 lastMip = header.MipLevels - 1;
        while ((header.Width >> lastMip) < blockSize && (header.Height >> lastMip) < blockSize && lastMip > 0)
            lastMip--;
        _minMipCountBlockCompressed = Math::Min(header.MipLevels - lastMip + 1, header.MipLevels);
    }

    // Request resource streaming
#if GPU_ENABLE_TEXTURES_STREAMING
    bool isDynamic = !_header.NeverStream;
#else
	bool isDynamic = false;
#endif
    StartStreaming(isDynamic);

    return false;
}

void StreamingTexture::UnloadTexture()
{
    ScopeLock lock(_owner->GetOwnerLocker());
    CancelStreamingTasks();
    _texture->ReleaseGPU();
    _header.MipLevels = 0;
    ASSERT(_streamingTasks.Count() == 0);
}

uint64 StreamingTexture::GetTotalMemoryUsage() const
{
    const uint64 arraySize = _header.IsCubeMap ? 6 : 1;
    return RenderTools::CalculateTextureMemoryUsage(_header.Format, _header.Width, _header.Height, _header.MipLevels) * arraySize;
}

String StreamingTexture::ToString() const
{
    return _texture->ToString();
}

int32 StreamingTexture::GetMaxResidency() const
{
    return _header.MipLevels;
}

int32 StreamingTexture::GetCurrentResidency() const
{
    return _texture->ResidentMipLevels();
}

int32 StreamingTexture::GetAllocatedResidency() const
{
    return _texture->MipLevels();
}

bool StreamingTexture::CanBeUpdated() const
{
    // Streaming Texture cannot be updated if:
    // - is not initialized
    // - mip data uploading job running
    // - resize texture job running
    if (IsInitialized())
    {
        ScopeLock lock(_owner->GetOwnerLocker());
        return _streamingTasks.Count() == 0;
    }
    return false;
}

class StreamTextureResizeTask : public GPUTask
{
private:
    StreamingTexture* _streamingTexture;
    GPUTexture* _newTexture;
    int32 _uploadedMipCount;

public:
    StreamTextureResizeTask(StreamingTexture* texture, GPUTexture* newTexture)
        : GPUTask(Type::CopyResource)
        , _streamingTexture(texture)
        , _newTexture(newTexture)
    {
        _streamingTexture->_streamingTasks.Add(this);
    }

    ~StreamTextureResizeTask()
    {
        SAFE_DELETE_GPU_RESOURCE(_newTexture);
    }

protected:
    // [GPUTask]
    Result run(GPUTasksContext* context) override
    {
        if (_streamingTexture == nullptr)
            return Result::MissingResources;

        // Copy all shared mips from previous texture to the new one
        GPUTexture* dstTexture = _newTexture;
        const int32 dstMips = dstTexture->MipLevels();
        GPUTexture* srcTexture = _streamingTexture->GetTexture();
        const int32 srcMips = srcTexture->MipLevels();
        const int32 srcMissingMips = srcMips - srcTexture->ResidentMipLevels();
        const int32 mipCount = Math::Min(dstMips, srcMips);
        for (int32 mipIndex = srcMissingMips; mipIndex < mipCount; mipIndex++)
        {
            context->GPU->CopySubresource(dstTexture, dstMips - mipIndex - 1, srcTexture, srcMips - mipIndex - 1);
        }
        _uploadedMipCount = mipCount - srcMissingMips;

        return Result::Ok;
    }

    void OnEnd() override
    {
        if (_streamingTexture)
        {
            ScopeLock lock(_streamingTexture->GetOwner()->GetOwnerLocker());
            _streamingTexture->_streamingTasks.Remove(this);
        }

        // Base
        GPUTask::OnEnd();
    }

    void OnSync() override
    {
        _newTexture->SetResidentMipLevels(_uploadedMipCount);
        Swap(_streamingTexture->_texture, _newTexture);
        SAFE_DELETE_GPU_RESOURCE(_newTexture);
        _streamingTexture->ResidencyChanged();

        // Base
        GPUTask::OnSync();
    }
};

Task* StreamingTexture::UpdateAllocation(int32 residency)
{
    ScopeLock lock(_owner->GetOwnerLocker());

    ASSERT(_texture && IsInitialized() && Math::IsInRange(residency, 0, TotalMipLevels()));
    Task* result = nullptr;

    const int32 allocatedResidency = GetAllocatedResidency();
    ASSERT(allocatedResidency >= 0);
    if (residency == allocatedResidency)
    {
        // Residency won't change
    }
    else if (residency == 0)
    {
        // Release texture memory
        _texture->ReleaseGPU();
    }
    else
    {
        // Use new texture object for resizing task
        GPUTexture* texture = _texture;
        if (allocatedResidency != 0)
        {
#if GPU_ENABLE_RESOURCE_NAMING
            texture = GPUDevice::Instance->CreateTexture(_texture->GetName());
#else
            texture = GPUDevice::Instance->CreateTexture();
#endif
        }

        // Create texture description
        const int32 mip = TotalMipLevels() - residency;
        const int32 width = Math::Max(TotalWidth() >> mip, 1);
        const int32 height = Math::Max(TotalHeight() >> mip, 1);
        GPUTextureDescription desc;
        if (IsCubeMap())
        {
            ASSERT(width == height);
            desc = GPUTextureDescription::NewCube(width, residency, _header.Format, GPUTextureFlags::ShaderResource);
        }
        else
        {
            desc = GPUTextureDescription::New2D(width, height, residency, _header.Format, GPUTextureFlags::ShaderResource);
        }

        // Setup texture
        if (texture->Init(desc))
        {
            Streaming.Error = true;
#if GPU_ENABLE_RESOURCE_NAMING
            LOG(Error, "Cannot allocate texture {0}", texture->GetName());
#else
            LOG(Error, "Cannot allocate texture");
#endif
        }
        if (allocatedResidency != 0)
        {
            // Copy data from the previous texture
            result = New<StreamTextureResizeTask>(this, texture);
        }
        else
        {
            // Use the new texture
            _texture = texture;
        }
    }

    return result;
}

class StreamTextureMipTask : public GPUUploadTextureMipTask
{
private:
    StreamingTexture* _streamingTexture;
    Task* _rootTask;
    FlaxStorage::LockData _dataLock;

public:
    StreamTextureMipTask(StreamingTexture* texture, int32 mipIndex, Task* rootTask)
        : GPUUploadTextureMipTask(texture->GetTexture(), mipIndex, Span<byte>(nullptr, 0), 0, 0, false)
        , _streamingTexture(texture)
        , _rootTask(rootTask ? rootTask : this)
        , _dataLock(_streamingTexture->GetOwner()->LockData())
    {
        _streamingTexture->_streamingTasks.Add(_rootTask);
        _texture.Released.Bind<StreamTextureMipTask, &StreamTextureMipTask::OnResourceReleased2>(this);
    }

private:
    void OnResourceReleased2()
    {
        // Unlink texture
        if (_streamingTexture)
        {
            ScopeLock lock(_streamingTexture->GetOwner()->GetOwnerLocker());
            _streamingTexture->_streamingTasks.Remove(_rootTask);
            _streamingTexture = nullptr;
        }
    }

protected:
    // [GPUTask]
    Result run(GPUTasksContext* context) override
    {
        const auto texture = _texture.Get();
        if (texture == nullptr)
            return Result::MissingResources;

        // Ensure that texture has been allocated before this task and has proper format
        if (!texture->IsAllocated() || texture->Format() != _streamingTexture->GetHeader()->Format)
        {
            LOG(Error, "Cannot stream texture {0} (streaming format: {1})", texture->ToString(), ScriptingEnum::ToString(_streamingTexture->GetHeader()->Format));
            return Result::Failed;
        }

        // Get asset data
        BytesContainer data;
        const auto absoluteMipIndex = _streamingTexture->TextureMipIndexToTotalIndex(_mipIndex);
        _streamingTexture->GetOwner()->GetMipData(absoluteMipIndex, data);
        if (data.IsInvalid())
            return Result::MissingData;

        // Cache data
        const int32 arraySize = texture->ArraySize();
        uint32 rowPitch, slicePitch;
        if (!_streamingTexture->GetOwner()->GetMipDataCustomPitch(absoluteMipIndex, rowPitch, slicePitch))
            texture->ComputePitch(_mipIndex, rowPitch, slicePitch);
        _data.Link(data);
        ASSERT(data.Length() >= (int32)slicePitch * arraySize);

        // Update all array slices
        const byte* dataSource = data.Get();
        for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
        {
            context->GPU->UpdateTexture(texture, arrayIndex, _mipIndex, dataSource, rowPitch, slicePitch);
            dataSource += slicePitch;
        }

        return Result::Ok;
    }

    void OnEnd() override
    {
        _dataLock.Release();
        if (_streamingTexture)
        {
            ScopeLock lock(_streamingTexture->GetOwner()->GetOwnerLocker());
            _streamingTexture->_streamingTasks.Remove(_rootTask);
            _streamingTexture = nullptr;
        }

        // Base
        GPUUploadTextureMipTask::OnEnd();
    }

    void OnFail() override
    {
        if (_streamingTexture)
        {
            // Stop streaming on fail
            _streamingTexture->ResetStreaming();
        }

        GPUUploadTextureMipTask::OnFail();
    }
};

Task* StreamingTexture::CreateStreamingTask(int32 residency)
{
    ScopeLock lock(_owner->GetOwnerLocker());

    ASSERT(_texture && IsInitialized() && Math::IsInRange(residency, 0, TotalMipLevels()));
    Task* result = nullptr;

    // Switch if go up or down with residency
    const int32 mipsCount = residency - GetCurrentResidency();
    if (mipsCount > 0)
    {
        // Create tasks collection
        const auto startMipIndex = TotalMipLevels() - _texture->ResidentMipLevels() - 1;
        const auto endMipIndex = startMipIndex - mipsCount;
        for (int32 mipIndex = startMipIndex; mipIndex > endMipIndex; mipIndex--)
        {
            ASSERT(mipIndex >= 0 && mipIndex < _header.MipLevels);

            // Request texture mip map data
            auto task = _owner->RequestMipDataAsync(mipIndex);
            if (task)
            {
                if (result)
                    result->ContinueWith(task);
                else
                    result = task;
            }

            // Add upload data task
            const int32 allocatedMipIndex = TotalIndexToTextureMipIndex(mipIndex);
            task = New<StreamTextureMipTask>(this, allocatedMipIndex, result);
            if (result)
                result->ContinueWith(task);
            else
                result = task;
        }

        ASSERT(result);
    }
    else
    {
        // Check if trim the mips down to 0 (full texture release)
        if (residency == 0)
        {
            // Do the quick data release
            _texture->ReleaseGPU();
            ResidencyChanged();
        }
        else
        {
            // TODO: create task for reducing texture quality, or update SRV now
            MISSING_CODE("add support for streaming quality down");
        }
    }

    return result;
}

void StreamingTexture::CancelStreamingTasks()
{
    ScopeLock lock(_owner->GetOwnerLocker());
    auto tasks = _streamingTasks;
    for (auto task : tasks)
        task->Cancel();
}
