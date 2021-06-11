// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "GPUBuffer.h"

/// <summary>
/// Dynamic GPU buffer that allows to update and use GPU data (index/vertex/other) during single frame (supports dynamic resizing)
/// </summary>
class FLAXENGINE_API DynamicBuffer : public NonCopyable
{
protected:

    GPUBuffer* _buffer;
    String _name;
    uint32 _stride;

public:

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="initialCapacity">Initial capacity of the buffer (in bytes)</param>
    /// <param name="stride">Stride in bytes</param>
    /// <param name="name">Buffer name</param>
    DynamicBuffer(uint32 initialCapacity, uint32 stride, const String& name);

    /// <summary>
    /// Destructor
    /// </summary>
    virtual ~DynamicBuffer();

public:

    /// <summary>
    /// The data container (raw bytes storage).
    /// </summary>
    Array<byte> Data;

    /// <summary>
    /// Gets buffer (may be null since it's using 'late init' feature)
    /// </summary>
    FORCE_INLINE GPUBuffer* GetBuffer() const
    {
        return _buffer;
    }

public:

    /// <summary>
    /// Clear data (begin for writing)
    /// </summary>
    FORCE_INLINE void Clear()
    {
        Data.Clear();
    }

    /// <summary>
    /// Write bytes to the buffer
    /// </summary>
    /// <param name="data">Data to write</param>
    template<typename T>
    FORCE_INLINE void Write(const T& data)
    {
        Data.Add((byte*)&data, sizeof(T));
    }

    /// <summary>
    /// Write bytes to the buffer
    /// </summary>
    /// <param name="bytes">Pointer to data to write</param>
    /// <param name="size">Amount of data to write (in bytes)</param>
    FORCE_INLINE void Write(void* bytes, int32 size)
    {
        Data.Add((byte*)bytes, size);
    }

    /// <summary>
    /// Unlock buffer and flush data with a buffer (it will be ready for an immediate draw).
    /// </summary>
    void Flush();

    /// <summary>
    /// Unlock buffer and flush data with a buffer (it will be ready for a during next frame draw).
    /// </summary>
    /// <param name="context">The GPU command list context to use for data uploading.</param>
    void Flush(class GPUContext* context);

    /// <summary>
    /// Disposes the buffer resource and clears the used memory.
    /// </summary>
    void Dispose();

protected:

    virtual void InitDesc(GPUBufferDescription& desc, int32 numElements) = 0;
};

/// <summary>
/// Dynamic vertex buffer that allows to render any vertices during single frame (supports dynamic resizing)
/// </summary>
class FLAXENGINE_API DynamicVertexBuffer : public DynamicBuffer
{
public:

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="initialCapacity">Initial capacity of the buffer (in bytes)</param>
    /// <param name="stride">Stride in bytes</param>
    /// <param name="name">Buffer name</param>
    DynamicVertexBuffer(uint32 initialCapacity, uint32 stride, const String& name = String::Empty)
        : DynamicBuffer(initialCapacity, stride, name)
    {
    }

protected:

    // [DynamicBuffer]
    void InitDesc(GPUBufferDescription& desc, int32 numElements) override
    {
        desc = GPUBufferDescription::Vertex(_stride, numElements, GPUResourceUsage::Dynamic);
    }
};

/// <summary>
/// Dynamic index buffer that allows to render any indices during single frame (supports dynamic resizing)
/// </summary>
class FLAXENGINE_API DynamicIndexBuffer : public DynamicBuffer
{
public:

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="initialCapacity">Initial capacity of the buffer (in bytes)</param>
    /// <param name="stride">Stride in bytes</param>
    /// <param name="name">Buffer name</param>
    DynamicIndexBuffer(uint32 initialCapacity, uint32 stride, const String& name = String::Empty)
        : DynamicBuffer(initialCapacity, stride, name)
    {
    }

protected:

    // [DynamicBuffer]
    void InitDesc(GPUBufferDescription& desc, int32 numElements) override
    {
        desc = GPUBufferDescription::Index(_stride, numElements, GPUResourceUsage::Dynamic);
    }
};
