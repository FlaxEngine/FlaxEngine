// Copyright (c) Wojciech Figat. All rights reserved.

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

void DynamicBuffer::Flush(GPUContext* context)
{
    const uint32 size = Data.Count();
    if (size == 0)
        return;

    // Lazy-resize buffer
    if (_buffer == nullptr)
        _buffer = GPUDevice::Instance->CreateBuffer(_name);
    if (_buffer->GetSize() < size || _buffer->GetDescription().Usage != Usage)
    {
        const int32 numElements = Math::AlignUp<int32>(static_cast<int32>((size / _stride) * 1.3f), 32);
        GPUBufferDescription desc;
        InitDesc(desc, numElements);
        desc.Usage = Usage;
        if (_buffer->Init(desc))
        {
            LOG(Fatal, "Cannot setup dynamic buffer '{0}'! Size: {1}", _name, Utilities::BytesToText(size));
            return;
        }
    }

    // Upload data to the buffer
    if (context)
    {
        context->UpdateBuffer(_buffer, Data.Get(), size);
    }
    else if (GPUDevice::Instance->IsRendering())
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

void DynamicBuffer::Dispose()
{
    SAFE_DELETE_GPU_RESOURCE(_buffer);
    Data.Resize(0);
}

GPUVertexLayout* DynamicVertexBuffer::GetLayout() const
{
    return _layout ? _layout : (GetBuffer() ? GetBuffer()->GetVertexLayout() : nullptr);
}

void DynamicVertexBuffer::SetLayout(GPUVertexLayout* layout)
{
    _layout = layout;
    SAFE_DELETE_GPU_RESOURCE(_buffer);
}

void DynamicVertexBuffer::InitDesc(GPUBufferDescription& desc, int32 numElements)
{
    desc = GPUBufferDescription::Vertex(_layout, _stride, numElements, GPUResourceUsage::Dynamic);
}

void DynamicIndexBuffer::InitDesc(GPUBufferDescription& desc, int32 numElements)
{
    desc = GPUBufferDescription::Index(_stride, numElements, GPUResourceUsage::Dynamic);
}

DynamicStructuredBuffer::DynamicStructuredBuffer(uint32 initialCapacity, uint32 stride, bool isUnorderedAccess, const String& name)
    : DynamicBuffer(initialCapacity, stride, name)
    , _isUnorderedAccess(isUnorderedAccess)
{
    Usage = GPUResourceUsage::Default; // The most common use-case is just for a single upload of data prepared by CPU
}

void DynamicStructuredBuffer::InitDesc(GPUBufferDescription& desc, int32 numElements)
{
    desc = GPUBufferDescription::Structured(numElements, _stride, _isUnorderedAccess);
}

DynamicTypedBuffer::DynamicTypedBuffer(uint32 initialCapacity, PixelFormat format, bool isUnorderedAccess, const String& name)
    : DynamicBuffer(initialCapacity, PixelFormatExtensions::SizeInBytes(format), name)
    , _format(format)
    , _isUnorderedAccess(isUnorderedAccess)
{
    Usage = GPUResourceUsage::Default; // The most common use-case is just for a single upload of data prepared by CPU
}

void DynamicTypedBuffer::InitDesc(GPUBufferDescription& desc, int32 numElements)
{
    auto bufferFlags = GPUBufferFlags::ShaderResource;
    if (_isUnorderedAccess)
        bufferFlags |= GPUBufferFlags::UnorderedAccess;
    desc = GPUBufferDescription::Buffer(numElements * _stride, bufferFlags, _format, nullptr, _stride);
}
