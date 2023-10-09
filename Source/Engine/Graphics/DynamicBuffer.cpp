// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "DynamicBuffer.h"
#include "GPUContext.h"
#include "PixelFormatExtensions.h"
#include "GPUDevice.h"
#include "RenderTask.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Core/Math/Math.h"

DynamicBuffer::DynamicBuffer(uint32 initialCapacity, uint32 stride, const String& name)
    : _buffer(nullptr)
    , _name(name)
    , _stride(stride)
    , Data(initialCapacity)
{
}

DynamicBuffer::~DynamicBuffer()
{
    SAFE_DELETE_GPU_RESOURCE(_buffer);
}

void DynamicBuffer::Flush()
{
    // Check if has sth to flush
    const uint32 size = Data.Count();
    if (size > 0)
    {
        // Check if has no buffer
        if (_buffer == nullptr)
            _buffer = GPUDevice::Instance->CreateBuffer(_name);

        // Check if need to resize buffer
        if (_buffer->GetSize() < size)
        {
            const uint32 numElements = Math::AlignUp<uint32>(static_cast<uint32>((size / _stride) * 1.3f), 32);
            GPUBufferDescription desc;
            InitDesc(desc, numElements);
            if (_buffer->Init(desc))
            {
                LOG(Fatal, "Cannot setup dynamic buffer '{0}'! Size: {1}", _name, Utilities::BytesToText(size));
                return;
            }
        }

        // Upload data to the buffer
        if (GPUDevice::Instance->IsRendering())
        {
            RenderContext::GPULocker.Lock();
            GPUDevice::Instance->GetMainContext()->UpdateBuffer(_buffer, Data.Get(), size);
            RenderContext::GPULocker.Unlock();
        }
        else
        {
            _buffer->SetData(Data.Get(), size);
        }
    }
}

void DynamicBuffer::Flush(GPUContext* context)
{
    // Check if has sth to flush
    const uint32 size = Data.Count();
    if (size > 0)
    {
        // Check if has no buffer
        if (_buffer == nullptr)
            _buffer = GPUDevice::Instance->CreateBuffer(_name);

        // Check if need to resize buffer
        if (_buffer->GetSize() < size)
        {
            const uint32 numElements = Math::AlignUp<uint32>(static_cast<uint32>((size / _stride) * 1.3f), 32);
            GPUBufferDescription desc;
            InitDesc(desc, numElements);
            if (_buffer->Init(desc))
            {
                LOG(Fatal, "Cannot setup dynamic buffer '{0}'! Size: {1}", _name, Utilities::BytesToText(size));
                return;
            }
        }

        // Upload data to the buffer
        context->UpdateBuffer(_buffer, Data.Get(), size);
    }
}

void DynamicBuffer::Dispose()
{
    SAFE_DELETE_GPU_RESOURCE(_buffer);
    Data.Resize(0);
}

void DynamicStructuredBuffer::InitDesc(GPUBufferDescription& desc, int32 numElements)
{
    desc = GPUBufferDescription::Structured(numElements, _stride, _isUnorderedAccess);
    desc.Usage = GPUResourceUsage::Dynamic;
}

DynamicTypedBuffer::DynamicTypedBuffer(uint32 initialCapacity, PixelFormat format, bool isUnorderedAccess, const String& name)
    : DynamicBuffer(initialCapacity, PixelFormatExtensions::SizeInBytes(format), name)
    , _format(format)
    , _isUnorderedAccess(isUnorderedAccess)
{
}

void DynamicTypedBuffer::InitDesc(GPUBufferDescription& desc, int32 numElements)
{
    auto bufferFlags = GPUBufferFlags::ShaderResource;
    if (_isUnorderedAccess)
        bufferFlags |= GPUBufferFlags::UnorderedAccess;
    desc = GPUBufferDescription::Buffer(numElements * _stride, bufferFlags, _format, nullptr, _stride, GPUResourceUsage::Dynamic);
}
