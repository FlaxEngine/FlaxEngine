// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "DynamicBuffer.h"
#include "GPUDevice.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Threading/Threading.h"

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
        if (IsInMainThread() && GPUDevice::Instance->IsRendering())
        {
            GPUDevice::Instance->GetMainContext()->UpdateBuffer(_buffer, Data.Get(), size);
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
