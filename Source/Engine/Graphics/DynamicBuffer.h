// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "GPUBuffer.h"

/// <summary>
/// Dynamic GPU buffer that allows to update and use GPU data (index/vertex/other) during single frame (supports dynamic resizing).
/// </summary>
class FLAXENGINE_API DynamicBuffer
{
protected:
    GPUBuffer* _buffer;
    String _name;
    uint32 _stride;

public:
    NON_COPYABLE(DynamicBuffer);

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
    /// GPU usage of the resource. Use Dynamic for resources that can be updated multiple timers per-frame.
    /// </summary>
    GPUResourceUsage Usage = GPUResourceUsage::Dynamic;

    /// <summary>
    /// The data container (raw bytes storage).
    /// </summary>
    Array<byte> Data;

    /// <summary>
    /// Gets buffer (can be null due to 'late init' feature).
    /// </summary>
    FORCE_INLINE GPUBuffer* GetBuffer() const
    {
        return _buffer;
    }

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
    FORCE_INLINE void Write(const void* bytes, int32 size)
    {
        Data.Add((const byte*)bytes, size);
    }

    /// <summary>
    /// Allocates bytes in the buffer by resizing the buffer for new memory and returns the pointer to the start of the allocated space.
    /// </summary>
    /// <param name="size">Amount of data to allocate (in bytes)</param>
    FORCE_INLINE byte* WriteReserve(int32 size)
    {
        const int32 start = Data.Count();
        Data.AddUninitialized(size);
        return Data.Get() + start;
    }

    /// <summary>
    /// Allocates bytes in the buffer by resizing the buffer for new memory and returns the pointer to the start of the allocated space.
    /// </summary>
    /// <param name="count">Amount of items to allocate</param>
    template<typename T>
    FORCE_INLINE T* WriteReserve(int32 count)
    {
        return (T*)WriteReserve(count * sizeof(T));
    }

    /// <summary>
    /// Unlock buffer and flush data with a buffer (it will be ready for an immediate draw).
    /// </summary>
    void Flush()
    {
        Flush(nullptr);
    }

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
private:
    GPUVertexLayout* _layout;

public:
    /// <summary>
    /// Init
    /// </summary>
    /// <param name="initialCapacity">Initial capacity of the buffer (in bytes)</param>
    /// <param name="stride">Stride in bytes</param>
    /// <param name="name">Buffer name</param>
    /// <param name="layout">The vertex buffer layout.</param>
    DynamicVertexBuffer(uint32 initialCapacity, uint32 stride, const String& name = String::Empty, GPUVertexLayout* layout = nullptr)
        : DynamicBuffer(initialCapacity, stride, name)
        , _layout(layout)
    {
    }

    // Gets the vertex buffer layout.
    GPUVertexLayout* GetLayout() const;
    // Sets the vertex buffer layout.
    void SetLayout(GPUVertexLayout* layout);

protected:
    // [DynamicBuffer]
    void InitDesc(GPUBufferDescription& desc, int32 numElements) override;
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
    void InitDesc(GPUBufferDescription& desc, int32 numElements) override;
};

/// <summary>
/// Dynamic structured buffer that allows to upload data to the GPU from CPU (supports dynamic resizing).
/// </summary>
class FLAXENGINE_API DynamicStructuredBuffer : public DynamicBuffer
{
private:
    bool _isUnorderedAccess;

public:
    /// <summary>
    /// Init
    /// </summary>
    /// <param name="initialCapacity">Initial capacity of the buffer (in bytes).</param>
    /// <param name="stride">Stride in bytes.</param>
    /// <param name="isUnorderedAccess">True if unordered access usage.</param>
    /// <param name="name">Buffer name.</param>
    DynamicStructuredBuffer(uint32 initialCapacity, uint32 stride, bool isUnorderedAccess = false, const String& name = String::Empty);

protected:
    // [DynamicBuffer]
    void InitDesc(GPUBufferDescription& desc, int32 numElements) override;
};

/// <summary>
/// Dynamic Typed buffer that allows to upload data to the GPU from CPU (supports dynamic resizing).
/// </summary>
class FLAXENGINE_API DynamicTypedBuffer : public DynamicBuffer
{
private:
    PixelFormat _format;
    bool _isUnorderedAccess;

public:
    /// <summary>
    /// Init
    /// </summary>
    /// <param name="initialCapacity">Initial capacity of the buffer (in bytes).</param>
    /// <param name="format">Format of the data.</param>
    /// <param name="isUnorderedAccess">True if unordered access usage.</param>
    /// <param name="name">Buffer name.</param>
    DynamicTypedBuffer(uint32 initialCapacity, PixelFormat format, bool isUnorderedAccess = false, const String& name = String::Empty);

protected:
    // [DynamicBuffer]
    void InitDesc(GPUBufferDescription& desc, int32 numElements) override;
};
