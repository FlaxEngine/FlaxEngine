// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Enums.h"
#include "PixelFormat.h"

/// <summary>
/// The GPU buffer usage flags.
/// </summary>
API_ENUM(Attributes="Flags") enum class GPUBufferFlags
{
    /// <summary>
    /// Nothing
    /// </summary>
    None = 0x000,

    /// <summary>
    /// Create a buffer that can be bound as a shader resource.
    /// </summary>
    ShaderResource = 0x001,

    /// <summary>
    /// Create a buffer that can be bound as a vertex buffer.
    /// </summary>
    VertexBuffer = 0x002,

    /// <summary>
    /// Create a buffer that can be bound as a index buffer.
    /// </summary>
    IndexBuffer = 0x004,

    /// <summary>
    /// Create a buffer that can be bound as a unordered access.
    /// </summary>
    UnorderedAccess = 0x008,

    /// <summary>
    /// Flag for unordered access buffers that will use append feature.
    /// </summary>
    Append = 0x010,

    /// <summary>
    /// Flag for unordered access buffers that will use counter feature.
    /// </summary>
    Counter = 0x020,

    /// <summary>
    /// Flag for unordered access buffers that will be used as draw indirect argument buffer.
    /// </summary>
    Argument = 0x040,

    /// <summary>
    /// Flag for structured buffers.
    /// </summary>
    Structured = 0x080,

    /// <summary>
    /// Flag for raw buffers.
    /// </summary>
    RawBuffer = 0x100,

    /// <summary>
    /// Creates a structured buffer that supports unordered access and append.
    /// </summary>
    StructuredAppendBuffer = UnorderedAccess | Structured | Append,

    /// <summary>
    /// Creates a structured buffer that supports unordered access and counter.
    /// </summary>
    StructuredCounterBuffer = UnorderedAccess | Structured | Counter
};

DECLARE_ENUM_OPERATORS(GPUBufferFlags);

/// <summary>
/// A common description for all GPU buffers.
/// </summary>
API_STRUCT() struct FLAXENGINE_API GPUBufferDescription
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(GPUBufferDescription);

    /// <summary>
    /// The buffer total size.
    /// </summary>
    API_FIELD() uint32 Size;

    /// <summary>
    /// The buffer structure stride (size in bytes per element).
    /// </summary>
    API_FIELD() uint32 Stride;

    /// <summary>
    /// The buffer flags.
    /// </summary>
    API_FIELD() GPUBufferFlags Flags;

    /// <summary>
    /// The format of the data in a buffer.
    /// </summary>
    API_FIELD() PixelFormat Format;

    /// <summary>
    /// The pointer to location of initial resource data. Null if not used.
    /// </summary>
    API_FIELD() const void* InitData;

    /// <summary>	
    /// Value that identifies how the buffer is to be read from and written to. The most common value is <see cref="GPUResourceUsage.Default"/>; see <strong><see cref="GPUResourceUsage"/></strong> for all possible values.
    /// </summary>
    API_FIELD() GPUResourceUsage Usage;

public:
    /// <summary>
    /// Gets the number elements in the buffer.
    /// </summary>
    FORCE_INLINE uint32 GetElementsCount() const
    {
        return Stride > 0 ? Size / Stride : 0;
    }

    /// <summary>
    /// Gets a value indicating whether this instance is a shader resource.
    /// </summary>
    FORCE_INLINE bool IsShaderResource() const
    {
        return (Flags & GPUBufferFlags::ShaderResource) != GPUBufferFlags::None;
    }

    /// <summary>
    /// Gets a value indicating whether this instance is a unordered access.
    /// </summary>
    FORCE_INLINE bool IsUnorderedAccess() const
    {
        return (Flags & GPUBufferFlags::UnorderedAccess) != GPUBufferFlags::None;
    }

public:
    /// <summary>
    /// Creates the buffer description.
    /// </summary>
    /// <param name="size">The size (in bytes).</param>
    /// <param name="flags">The flags.</param>
    /// <param name="format">The format.</param>
    /// <param name="initData">The initial data.</param>
    /// <param name="stride">The stride.</param>
    /// <param name="usage">The usage.</param>
    /// <returns>The buffer description.</returns>
    static GPUBufferDescription Buffer(uint32 size, GPUBufferFlags flags, PixelFormat format = PixelFormat::Unknown, const void* initData = nullptr, uint32 stride = 0, GPUResourceUsage usage = GPUResourceUsage::Default);

    /// <summary>
    /// Creates typed buffer description.
    /// </summary>
    /// <param name="count">The elements count.</param>
    /// <param name="viewFormat">The view format.</param>
    /// <param name="isUnorderedAccess">True if use UAV, otherwise false.</param>
    /// <param name="usage">The usage.</param>
    /// <returns>The buffer description.</returns>
    /// <remarks>
    /// Example in HLSL: Buffer&lt;float4&gt;.
    /// </remarks>
    static GPUBufferDescription Typed(int32 count, PixelFormat viewFormat, bool isUnorderedAccess = false, GPUResourceUsage usage = GPUResourceUsage::Default);

    /// <summary>
    /// Creates typed buffer description.
    /// </summary>
    /// <param name="data">The data.</param>
    /// <param name="count">The elements count.</param>
    /// <param name="viewFormat">The view format.</param>
    /// <param name="isUnorderedAccess">True if use UAV, otherwise false.</param>
    /// <param name="usage">The usage.</param>
    /// <returns>The buffer description.</returns>
    /// <remarks>
    /// Example in HLSL: Buffer&lt;float4&gt;.
    /// </remarks>
    static GPUBufferDescription Typed(const void* data, int32 count, PixelFormat viewFormat, bool isUnorderedAccess = false, GPUResourceUsage usage = GPUResourceUsage::Default);

    /// <summary>
    /// Creates vertex buffer description.
    /// </summary>
    /// <param name="elementStride">The element stride.</param>
    /// <param name="elementsCount">The elements count.</param>
    /// <param name="data">The data.</param>
    /// <returns>The buffer description.</returns>
    static GPUBufferDescription Vertex(int32 elementStride, int32 elementsCount, const void* data)
    {
        return Buffer(elementsCount * elementStride, GPUBufferFlags::VertexBuffer, PixelFormat::Unknown, data, elementStride, GPUResourceUsage::Default);
    }

    /// <summary>
    /// Creates vertex buffer description.
    /// </summary>
    /// <param name="elementStride">The element stride.</param>
    /// <param name="elementsCount">The elements count.</param>
    /// <param name="usage">The usage mode.</param>
    /// <returns>The buffer description.</returns>
    static GPUBufferDescription Vertex(int32 elementStride, int32 elementsCount, GPUResourceUsage usage = GPUResourceUsage::Default)
    {
        return Buffer(elementsCount * elementStride, GPUBufferFlags::VertexBuffer, PixelFormat::Unknown, nullptr, elementStride, usage);
    }

    /// <summary>
    /// Creates vertex buffer description.
    /// </summary>
    /// <param name="size">The size (in bytes).</param>
    /// <param name="usage">The usage mode.</param>
    /// <returns>The buffer description.</returns>
    static GPUBufferDescription Vertex(int32 size, GPUResourceUsage usage = GPUResourceUsage::Default)
    {
        return Buffer(size, GPUBufferFlags::VertexBuffer, PixelFormat::Unknown, nullptr, 0, usage);
    }

    /// <summary>
    /// Creates index buffer description.
    /// </summary>
    /// <param name="elementStride">The element stride.</param>
    /// <param name="elementsCount">The elements count.</param>
    /// <param name="data">The data.</param>
    /// <returns>The buffer description.</returns>
    static GPUBufferDescription Index(int32 elementStride, int32 elementsCount, const void* data)
    {
        const auto format = elementStride == 4 ? PixelFormat::R32_UInt : PixelFormat::R16_UInt;
        return Buffer(elementsCount * elementStride, GPUBufferFlags::IndexBuffer, format, data, elementStride, GPUResourceUsage::Default);
    }

    /// <summary>
    /// Creates index buffer description.
    /// </summary>
    /// <param name="elementStride">The element stride.</param>
    /// <param name="elementsCount">The elements count.</param>
    /// <param name="usage">The usage mode.</param>
    /// <returns>The buffer description.</returns>
    static GPUBufferDescription Index(int32 elementStride, int32 elementsCount, GPUResourceUsage usage = GPUResourceUsage::Default)
    {
        const auto format = elementStride == 4 ? PixelFormat::R32_UInt : PixelFormat::R16_UInt;
        return Buffer(elementsCount * elementStride, GPUBufferFlags::IndexBuffer, format, nullptr, elementStride, usage);
    }

    /// <summary>
    /// Creates structured buffer description.
    /// </summary>
    /// <param name="elementCount">The element count.</param>
    /// <param name="elementSize">Size of the element (in bytes).</param>
    /// <param name="isUnorderedAccess">if set to <c>true</c> [is unordered access].</param>
    /// <returns>The buffer description.</returns>
    /// <remarks>
    /// Example in HLSL: StructuredBuffer&lt;float4&gt; or RWStructuredBuffer&lt;float4&gt; for structured buffers supporting unordered access.
    /// </remarks>
    static GPUBufferDescription Structured(int32 elementCount, int32 elementSize, bool isUnorderedAccess = false)
    {
        auto bufferFlags = GPUBufferFlags::Structured | GPUBufferFlags::ShaderResource;
        if (isUnorderedAccess)
            bufferFlags |= GPUBufferFlags::UnorderedAccess;

        return Buffer(elementCount * elementSize, bufferFlags, PixelFormat::Unknown, nullptr, elementSize);
    }

    /// <summary>
    /// Creates append buffer description (structured buffer).
    /// </summary>
    /// <param name="elementCount">The element count.</param>
    /// <param name="elementSize">Size of the element (in bytes).</param>
    /// <returns>The buffer description.</returns>
    /// <remarks>
    /// Example in HLSL: AppendStructuredBuffer&lt;float4&gt; or ConsumeStructuredBuffer&lt;float4&gt;.
    /// </remarks>
    static GPUBufferDescription StructuredAppend(int32 elementCount, int32 elementSize)
    {
        return Buffer(elementCount * elementSize, GPUBufferFlags::StructuredAppendBuffer | GPUBufferFlags::ShaderResource, PixelFormat::Unknown, nullptr, elementSize);
    }

    /// <summary>
    /// Creates counter buffer description (structured buffer).
    /// </summary>
    /// <param name="elementCount">The element count.</param>
    /// <param name="elementSize">Size of the element (in bytes).</param>
    /// <returns>The buffer description.</returns>
    /// <remarks>
    /// Example in HLSL: StructuredBuffer&lt;float4&gt; or RWStructuredBuffer&lt;float4&gt; for structured buffers supporting unordered access.
    /// </remarks>
    static GPUBufferDescription StructuredCounter(int32 elementCount, int32 elementSize)
    {
        return Buffer(elementCount * elementSize, GPUBufferFlags::StructuredCounterBuffer | GPUBufferFlags::ShaderResource, PixelFormat::Unknown, nullptr, elementSize);
    }

    /// <summary>
    /// Creates argument buffer description.
    /// </summary>
    /// <param name="size">The size (in bytes).</param>
    /// <param name="usage">The usage.</param>
    /// <returns>The buffer description.</returns>
    static GPUBufferDescription Argument(int32 size, GPUResourceUsage usage = GPUResourceUsage::Default)
    {
        return Buffer(size, GPUBufferFlags::Argument, PixelFormat::Unknown, nullptr, 0, usage);
    }

    /// <summary>
    /// Creates argument buffer description.
    /// </summary>
    /// <param name="data">The initial data.</param>
    /// <param name="size">The size (in bytes).</param>
    /// <param name="usage">The usage.</param>
    /// <returns>The buffer description.</returns>
    static GPUBufferDescription Argument(const void* data, int32 size, GPUResourceUsage usage = GPUResourceUsage::Default)
    {
        return Buffer(size, GPUBufferFlags::Argument, PixelFormat::Unknown, data, 0, usage);
    }

    /// <summary>
    /// Creates raw buffer description.
    /// </summary>
    /// <param name="size">The size (in bytes).</param>
    /// <param name="additionalFlags">The additional bindings (for example, to create a combined raw/index buffer, pass <see cref="GPUBufferFlags::IndexBuffer" />).</param>
    /// <param name="usage">The usage.</param>
    /// <returns>The buffer description.</returns>
    static GPUBufferDescription Raw(int32 size, GPUBufferFlags additionalFlags = GPUBufferFlags::None, GPUResourceUsage usage = GPUResourceUsage::Default)
    {
        return Buffer(size, GPUBufferFlags::RawBuffer | additionalFlags, PixelFormat::R32_Typeless, nullptr, sizeof(float), usage);
    }

    /// <summary>
    /// Creates raw buffer description.
    /// </summary>
    /// <param name="data">The initial data.</param>
    /// <param name="size">The size (in bytes).</param>
    /// <param name="additionalFlags">The additional bindings (for example, to create a combined raw/index buffer, pass <see cref="GPUBufferFlags::IndexBuffer" />).</param>
    /// <param name="usage">The usage.</param>
    /// <returns>The buffer description.</returns>
    static GPUBufferDescription Raw(const void* data, int32 size, GPUBufferFlags additionalFlags = GPUBufferFlags::None, GPUResourceUsage usage = GPUResourceUsage::Default)
    {
        return Buffer(size, GPUBufferFlags::RawBuffer | additionalFlags, PixelFormat::R32_Typeless, data, sizeof(float), usage);
    }

public:
    void Clear();
    GPUBufferDescription ToStagingUpload() const;
    GPUBufferDescription ToStagingReadback() const;
    GPUBufferDescription ToStaging() const;
    bool Equals(const GPUBufferDescription& other) const;
    String ToString() const;

public:
    FORCE_INLINE bool operator==(const GPUBufferDescription& other) const
    {
        return Equals(other);
    }

    FORCE_INLINE bool operator!=(const GPUBufferDescription& other) const
    {
        return !Equals(other);
    }
};

uint32 GetHash(const GPUBufferDescription& key);
