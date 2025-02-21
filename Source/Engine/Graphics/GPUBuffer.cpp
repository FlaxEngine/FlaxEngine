// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "GPUBuffer.h"
#include "GPUDevice.h"
#include "GPUResourceProperty.h"
#include "GPUBufferDescription.h"
#include "PixelFormatExtensions.h"
#include "RenderTask.h"
#include "Async/Tasks/GPUCopyResourceTask.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Debug/Exceptions/InvalidOperationException.h"
#include "Engine/Debug/Exceptions/ArgumentNullException.h"
#include "Engine/Debug/Exceptions/ArgumentOutOfRangeException.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/Enums.h"
#include "Engine/Threading/ThreadPoolTask.h"
#include "Engine/Threading/Threading.h"

GPUBufferDescription GPUBufferDescription::Buffer(uint32 size, GPUBufferFlags flags, PixelFormat format, const void* initData, uint32 stride, GPUResourceUsage usage)
{
    GPUBufferDescription desc;
    desc.Size = size;
    desc.Stride = stride;
    desc.Flags = flags;
    desc.Format = format;
    desc.InitData = initData;
    desc.Usage = usage;
    return desc;
}

GPUBufferDescription GPUBufferDescription::Typed(int32 count, PixelFormat viewFormat, bool isUnorderedAccess, GPUResourceUsage usage)
{
    auto bufferFlags = GPUBufferFlags::ShaderResource;
    if (isUnorderedAccess)
        bufferFlags |= GPUBufferFlags::UnorderedAccess;
    const auto stride = PixelFormatExtensions::SizeInBytes(viewFormat);
    return Buffer(count * stride, bufferFlags, viewFormat, nullptr, stride, usage);
}

GPUBufferDescription GPUBufferDescription::Typed(const void* data, int32 count, PixelFormat viewFormat, bool isUnorderedAccess, GPUResourceUsage usage)
{
    auto bufferFlags = GPUBufferFlags::ShaderResource;
    if (isUnorderedAccess)
        bufferFlags |= GPUBufferFlags::UnorderedAccess;
    const auto stride = PixelFormatExtensions::SizeInBytes(viewFormat);
    return Buffer(count * stride, bufferFlags, viewFormat, data, stride, usage);
}

void GPUBufferDescription::Clear()
{
    Platform::MemoryClear(this, sizeof(GPUBufferDescription));
}

GPUBufferDescription GPUBufferDescription::ToStagingUpload() const
{
    auto desc = *this;
    desc.Usage = GPUResourceUsage::StagingUpload;
    desc.Flags = GPUBufferFlags::None;
    desc.InitData = nullptr;
    return desc;
}

GPUBufferDescription GPUBufferDescription::ToStagingReadback() const
{
    auto desc = *this;
    desc.Usage = GPUResourceUsage::StagingReadback;
    desc.Flags = GPUBufferFlags::None;
    desc.InitData = nullptr;
    return desc;
}

GPUBufferDescription GPUBufferDescription::ToStaging() const
{
    auto desc = *this;
    desc.Usage = GPUResourceUsage::Staging;
    desc.Flags = GPUBufferFlags::None;
    desc.InitData = nullptr;
    return desc;
}

bool GPUBufferDescription::Equals(const GPUBufferDescription& other) const
{
    return Size == other.Size
            && Stride == other.Stride
            && Flags == other.Flags
            && Format == other.Format
            && Usage == other.Usage
            && InitData == other.InitData;
}

String GPUBufferDescription::ToString() const
{
    return String::Format(TEXT("Size: {0}, Stride: {1}, Flags: {2}, Format: {3}, Usage: {4}"),
                          Size,
                          Stride,
                          ScriptingEnum::ToStringFlags(Flags),
                          ScriptingEnum::ToString(Format),
                          (int32)Usage);
}

uint32 GetHash(const GPUBufferDescription& key)
{
    uint32 hashCode = key.Size;
    hashCode = (hashCode * 397) ^ key.Stride;
    hashCode = (hashCode * 397) ^ (uint32)key.Flags;
    hashCode = (hashCode * 397) ^ (uint32)key.Format;
    hashCode = (hashCode * 397) ^ (uint32)key.Usage;
    return hashCode;
}

GPUBufferView::GPUBufferView()
    : GPUResourceView(SpawnParams(Guid::New(), TypeInitializer))
{
}

GPUBuffer* GPUBuffer::Spawn(const SpawnParams& params)
{
    return GPUDevice::Instance->CreateBuffer(String::Empty);
}

GPUBuffer* GPUBuffer::New()
{
    return GPUDevice::Instance->CreateBuffer(String::Empty);
}

GPUBuffer::GPUBuffer()
    : GPUResource(SpawnParams(Guid::New(), TypeInitializer))
{
    // Buffer with size 0 is considered to be invalid
    _desc.Size = 0;
}

bool GPUBuffer::IsStaging() const
{
    return _desc.Usage == GPUResourceUsage::StagingReadback || _desc.Usage == GPUResourceUsage::StagingUpload || _desc.Usage == GPUResourceUsage::Staging;
}

bool GPUBuffer::IsDynamic() const
{
    return _desc.Usage == GPUResourceUsage::Dynamic;
}

bool GPUBuffer::Init(const GPUBufferDescription& desc)
{
    ASSERT(Math::IsInRange<uint32>(desc.Size, 1, MAX_int32)
        && Math::IsInRange<uint32>(desc.Stride, 0, 1024));

    // Validate description
    if (EnumHasAnyFlags(desc.Flags, GPUBufferFlags::Structured))
    {
        if (desc.Stride <= 0)
        {
            LOG(Warning, "Cannot create buffer. Element size cannot be less or equal 0 for structured buffer.");
            return true;
        }
    }
    if (EnumHasAnyFlags(desc.Flags, GPUBufferFlags::RawBuffer))
    {
        if (desc.Format != PixelFormat::R32_Typeless)
        {
            LOG(Warning, "Cannot create buffer. Raw buffers must use format R32_Typeless.");
            return true;
        }
    }

    // Release previous data
    ReleaseGPU();

    // Initialize
    _desc = desc;
    if (OnInit())
    {
        ReleaseGPU();
        LOG(Warning, "Cannot initialize buffer. Description: {0}", desc.ToString());
        return true;
    }

    return false;
}

GPUBuffer* GPUBuffer::ToStagingReadback() const
{
    const auto desc = _desc.ToStagingReadback();
    ScopeLock gpuLock(GPUDevice::Instance->Locker);
    auto* staging = GPUDevice::Instance->CreateBuffer(TEXT("Staging.Readback"));
    if (staging->Init(desc))
    {
        staging->ReleaseGPU();
        Delete(staging);
        return nullptr;
    }
    return staging;
}

GPUBuffer* GPUBuffer::ToStagingUpload() const
{
    const auto desc = _desc.ToStagingUpload();
    ScopeLock gpuLock(GPUDevice::Instance->Locker);
    auto* staging = GPUDevice::Instance->CreateBuffer(TEXT("Staging.Upload"));
    if (staging->Init(desc))
    {
        staging->ReleaseGPU();
        Delete(staging);
        return nullptr;
    }
    return staging;
}

bool GPUBuffer::Resize(uint32 newSize)
{
    PROFILE_CPU();
    if (!IsAllocated())
    {
        Log::InvalidOperationException(TEXT("Buffer.Resize"));
        return true;
    }

    // Setup new description
    auto desc = _desc;
    desc.Size = newSize;
    desc.InitData = nullptr;

    // Recreate
    return Init(desc);
}

bool GPUBuffer::DownloadData(BytesContainer& result)
{
    // Skip for empty ones
    if (GetSize() == 0)
    {
        LOG(Warning, "Cannot download GPU buffer data from an empty buffer.");
        return true;
    }
    if (_desc.Usage == GPUResourceUsage::StagingReadback || _desc.Usage == GPUResourceUsage::Dynamic || _desc.Usage == GPUResourceUsage::Staging)
    {
        // Use faster path for staging resources
        return GetData(result);
    }
    PROFILE_CPU();

    // Ensure not running on main thread
    if (IsInMainThread())
    {
        // TODO: support mesh data download from GPU on a main thread during rendering
        LOG(Warning, "Cannot download GPU buffer data on a main thread. Use staging readback buffer or invoke this function from another thread.");
        return true;
    }

    // Create async task
    auto task = DownloadDataAsync(result);
    if (task == nullptr)
    {
        LOG(Warning, "Cannot create async download task for resource {0}.", ToString());
        return true;
    }

    // Wait for work to be done
    task->Start();
    if (task->Wait())
    {
        LOG(Warning, "Resource \'{0}\' copy failed.", ToString());
        return true;
    }

    return false;
}

class BufferDownloadDataTask : public ThreadPoolTask
{
private:
    BufferReference _buffer;
    GPUBuffer* _staging;
    BytesContainer& _data;

public:
    BufferDownloadDataTask(GPUBuffer* buffer, GPUBuffer* staging, BytesContainer& data)
        : _buffer(buffer)
        , _staging(staging)
        , _data(data)
    {
    }

    ~BufferDownloadDataTask()
    {
        SAFE_DELETE_GPU_RESOURCE(_staging);
    }

public:
    // [ThreadPoolTask]
    bool HasReference(Object* resource) const override
    {
        return _buffer == resource || _staging == resource;
    }

protected:
    // [ThreadPoolTask]
    bool Run() override
    {
        // Check resources
        const auto buffer = _buffer.Get();
        if (buffer == nullptr || _staging == nullptr)
        {
            LOG(Warning, "Cannot download buffer data. Missing objects.");
            return true;
        }

        // Gather data
        if (_staging->GetData(_data))
        {
            LOG(Warning, "Staging resource of \'{0}\' get data failed.", buffer->ToString());
            return true;
        }

        return false;
    }

    void OnEnd() override
    {
        _buffer.Unlink();

        // Base
        ThreadPoolTask::OnEnd();
    }
};

Task* GPUBuffer::DownloadDataAsync(BytesContainer& result)
{
    // Skip for empty ones
    if (GetSize() == 0)
        return nullptr;

    // Create the staging resource
    const auto staging = ToStagingReadback();
    if (staging == nullptr)
    {
        LOG(Warning, "Cannot create staging resource from {0}.", ToString());
        return nullptr;
    }

    // Create async resource copy task
    auto copyTask = ::New<GPUCopyResourceTask>(this, staging);
    ASSERT(copyTask->HasReference(this) && copyTask->HasReference(staging));

    // Create task to copy downloaded data to BytesContainer
    const auto getDataTask = ::New<BufferDownloadDataTask>(this, staging, result);
    ASSERT(getDataTask->HasReference(this) && getDataTask->HasReference(staging));

    // Set continuation
    copyTask->ContinueWith(getDataTask);

    return copyTask;
}

bool GPUBuffer::GetData(BytesContainer& output)
{
    PROFILE_CPU();
    void* mapped = Map(GPUResourceMapMode::Read);
    if (!mapped)
        return true;
    output.Copy((byte*)mapped, GetSize());
    Unmap();
    return false;
}

void GPUBuffer::SetData(const void* data, uint32 size)
{
    PROFILE_CPU();
    if (size == 0 || data == nullptr)
    {
        Log::ArgumentNullException(TEXT("Buffer.SetData"));
        return;
    }
    if (size > GetSize())
    {
        Log::ArgumentOutOfRangeException(TEXT("Buffer.SetData"));
        return;
    }

    if (_desc.Usage == GPUResourceUsage::Default && GPUDevice::Instance->IsRendering())
    {
        // Upload using the context (will use internal staging buffer inside command buffer)
        RenderContext::GPULocker.Lock();
        GPUDevice::Instance->GetMainContext()->UpdateBuffer(this, data, size);
        RenderContext::GPULocker.Unlock();
        return;
    }

    void* mapped = Map(GPUResourceMapMode::Write);
    if (!mapped)
        return;
    Platform::MemoryCopy(mapped, data, size);
    Unmap();
}

String GPUBuffer::ToString() const
{
#if GPU_ENABLE_RESOURCE_NAMING
    return String::Format(TEXT("Buffer {0}, Flags: {1}, Stride: {2} bytes, Name: {3}"), Utilities::BytesToText(GetSize()), (int32)GetFlags(), GetStride(), GetName());
#else
	return TEXT("Buffer");
#endif
}

GPUResourceType GPUBuffer::GetResourceType() const
{
    return GPUResourceType::Buffer;
}

void GPUBuffer::OnReleaseGPU()
{
    _desc.Clear();
    _isLocked = false;
}
