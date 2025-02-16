// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    partial struct GPUBufferDescription : IEquatable<GPUBufferDescription>
    {
        /// <summary>
        /// Gets the number elements in the buffer.
        /// </summary>
        public uint ElementsCount => Stride > 0 ? Size / Stride : 0;

        /// <summary>
        /// Gets a value indicating whether this instance is a shader resource.
        /// </summary>
        public bool IsShaderResource => (Flags & GPUBufferFlags.ShaderResource) != 0;

        /// <summary>
        /// Gets a value indicating whether this instance is a unordered access.
        /// </summary>
        public bool IsUnorderedAccess => (Flags & GPUBufferFlags.UnorderedAccess) != 0;

        /// <summary>
        /// Clears description.
        /// </summary>
        public void Clear()
        {
            this = new GPUBufferDescription();
        }

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
        public static GPUBufferDescription Buffer(uint size, GPUBufferFlags flags, PixelFormat format = PixelFormat.Unknown, IntPtr initData = new IntPtr(), uint stride = 0, GPUResourceUsage usage = GPUResourceUsage.Default)
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
        public static GPUBufferDescription Buffer(int size, GPUBufferFlags flags, PixelFormat format = PixelFormat.Unknown, IntPtr initData = new IntPtr(), int stride = 0, GPUResourceUsage usage = GPUResourceUsage.Default)
        {
            GPUBufferDescription desc;
            desc.Size = (uint)size;
            desc.Stride = (uint)stride;
            desc.Flags = flags;
            desc.Format = format;
            desc.InitData = initData;
            desc.Usage = usage;
            return desc;
        }

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
        public static GPUBufferDescription Typed(int count, PixelFormat viewFormat, bool isUnorderedAccess = false, GPUResourceUsage usage = GPUResourceUsage.Default)
        {
            var bufferFlags = GPUBufferFlags.ShaderResource;
            if (isUnorderedAccess)
                bufferFlags |= GPUBufferFlags.UnorderedAccess;
            var stride = PixelFormatExtensions.SizeInBytes(viewFormat);
            return Buffer(count * stride, bufferFlags, viewFormat, IntPtr.Zero, stride, usage);
        }

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
        public static GPUBufferDescription Typed(IntPtr data, int count, PixelFormat viewFormat, bool isUnorderedAccess = false, GPUResourceUsage usage = GPUResourceUsage.Default)
        {
            var bufferFlags = GPUBufferFlags.ShaderResource;
            if (isUnorderedAccess)
                bufferFlags |= GPUBufferFlags.UnorderedAccess;
            var stride = PixelFormatExtensions.SizeInBytes(viewFormat);
            return Buffer(count * stride, bufferFlags, viewFormat, data, stride, usage);
        }

        /// <summary>
        /// Creates vertex buffer description.
        /// </summary>
        /// <param name="elementStride">The element stride.</param>
        /// <param name="elementsCount">The elements count.</param>
        /// <param name="data">The data.</param>
        /// <returns>The buffer description.</returns>
        public static GPUBufferDescription Vertex(int elementStride, int elementsCount, IntPtr data)
        {
            return Buffer(elementsCount * elementStride, GPUBufferFlags.VertexBuffer, PixelFormat.Unknown, data, elementStride, GPUResourceUsage.Default);
        }

        /// <summary>
        /// Creates vertex buffer description.
        /// </summary>
        /// <param name="elementStride">The element stride.</param>
        /// <param name="elementsCount">The elements count.</param>
        /// <param name="usage">The usage mode.</param>
        /// <returns>The buffer description.</returns>
        public static GPUBufferDescription Vertex(int elementStride, int elementsCount, GPUResourceUsage usage = GPUResourceUsage.Default)
        {
            return Buffer(elementsCount * elementStride, GPUBufferFlags.VertexBuffer, PixelFormat.Unknown, new IntPtr(), elementStride, usage);
        }

        /// <summary>
        /// Creates vertex buffer description.
        /// </summary>
        /// <param name="size">The size (in bytes).</param>
        /// <param name="usage">The usage mode.</param>
        /// <returns>The buffer description.</returns>
        public static GPUBufferDescription Vertex(int size, GPUResourceUsage usage = GPUResourceUsage.Default)
        {
            return Buffer(size, GPUBufferFlags.VertexBuffer, PixelFormat.Unknown, new IntPtr(), 0, usage);
        }

        /// <summary>
        /// Creates index buffer description.
        /// </summary>
        /// <param name="elementStride">The element stride.</param>
        /// <param name="elementsCount">The elements count.</param>
        /// <param name="data">The data.</param>
        /// <returns>The buffer description.</returns>
        public static GPUBufferDescription Index(int elementStride, int elementsCount, IntPtr data)
        {
            var format = elementStride == 4 ? PixelFormat.R32_UInt : PixelFormat.R16_UInt;
            return Buffer(elementsCount * elementStride, GPUBufferFlags.IndexBuffer, format, data, elementStride, GPUResourceUsage.Default);
        }

        /// <summary>
        /// Creates index buffer description.
        /// </summary>
        /// <param name="elementStride">The element stride.</param>
        /// <param name="elementsCount">The elements count.</param>
        /// <param name="usage">The usage mode.</param>
        /// <returns>The buffer description.</returns>
        public static GPUBufferDescription Index(int elementStride, int elementsCount, GPUResourceUsage usage = GPUResourceUsage.Default)
        {
            var format = elementStride == 4 ? PixelFormat.R32_UInt : PixelFormat.R16_UInt;
            return Buffer(elementsCount * elementStride, GPUBufferFlags.IndexBuffer, format, new IntPtr(), elementStride, usage);
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
        public static GPUBufferDescription Structured(int elementCount, int elementSize, bool isUnorderedAccess = false)
        {
            var bufferFlags = GPUBufferFlags.Structured | GPUBufferFlags.ShaderResource;
            if (isUnorderedAccess)
                bufferFlags |= GPUBufferFlags.UnorderedAccess;

            return Buffer(elementCount * elementSize, bufferFlags, PixelFormat.Unknown, new IntPtr(), elementSize);
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
        public static GPUBufferDescription StructuredAppend(int elementCount, int elementSize)
        {
            return Buffer(elementCount * elementSize, GPUBufferFlags.StructuredAppendBuffer | GPUBufferFlags.ShaderResource, PixelFormat.Unknown, new IntPtr(), elementSize);
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
        public static GPUBufferDescription StructuredCounter(int elementCount, int elementSize)
        {
            return Buffer(elementCount * elementSize, GPUBufferFlags.StructuredCounterBuffer | GPUBufferFlags.ShaderResource, PixelFormat.Unknown, new IntPtr(), elementSize);
        }

        /// <summary>
        /// Creates argument buffer description.
        /// </summary>
        /// <param name="size">The size (in bytes).</param>
        /// <param name="usage">The usage.</param>
        /// <returns>The buffer description.</returns>
        public static GPUBufferDescription Argument(int size, GPUResourceUsage usage = GPUResourceUsage.Default)
        {
            return Buffer(size, GPUBufferFlags.Argument, PixelFormat.Unknown, new IntPtr(), 0, usage);
        }

        /// <summary>
        /// Creates argument buffer description.
        /// </summary>
        /// <param name="data">The initial data.</param>
        /// <param name="size">The size (in bytes).</param>
        /// <param name="usage">The usage.</param>
        /// <returns>The buffer description.</returns>
        public static GPUBufferDescription Argument(IntPtr data, int size, GPUResourceUsage usage = GPUResourceUsage.Default)
        {
            return Buffer(size, GPUBufferFlags.Argument, PixelFormat.Unknown, data, 0, usage);
        }

        /// <summary>
        /// Creates raw buffer description.
        /// </summary>
        /// <param name="size">The size (in bytes).</param>
        /// <param name="additionalFlags">The additional bindings (for example, to create a combined raw/index buffer, pass <see cref="GPUBufferFlags.IndexBuffer" />).</param>
        /// <param name="usage">The usage.</param>
        /// <returns>The buffer description.</returns>
        public static GPUBufferDescription Raw(int size, GPUBufferFlags additionalFlags = GPUBufferFlags.None, GPUResourceUsage usage = GPUResourceUsage.Default)
        {
            return Buffer(size, GPUBufferFlags.RawBuffer | additionalFlags, PixelFormat.R32_Typeless, new IntPtr(), sizeof(float), usage);
        }

        /// <summary>
        /// Creates raw buffer description.
        /// </summary>
        /// <param name="data">The initial data.</param>
        /// <param name="size">The size (in bytes).</param>
        /// <param name="additionalFlags">The additional bindings (for example, to create a combined raw/index buffer, pass <see cref="GPUBufferFlags.IndexBuffer" />).</param>
        /// <param name="usage">The usage.</param>
        /// <returns>The buffer description.</returns>
        public static GPUBufferDescription Raw(IntPtr data, int size, GPUBufferFlags additionalFlags = GPUBufferFlags.None, GPUResourceUsage usage = GPUResourceUsage.Default)
        {
            return Buffer(size, GPUBufferFlags.RawBuffer | additionalFlags, PixelFormat.R32_Typeless, data, sizeof(float), usage);
        }

        /// <summary>
        /// Gets the staging upload (CPU write) description for this instance.
        /// </summary>
        /// <returns>A staging buffer description</returns>
        public GPUBufferDescription ToStagingUpload()
        {
            var desc = this;
            desc.Usage = GPUResourceUsage.StagingUpload;
            desc.Flags = GPUBufferFlags.None;
            desc.InitData = IntPtr.Zero;
            return desc;
        }

        /// <summary>
        /// Gets the staging readback (CPU read) description for this instance.
        /// </summary>
        /// <returns>A staging buffer description</returns>
        public GPUBufferDescription ToStagingReadback()
        {
            var desc = this;
            desc.Usage = GPUResourceUsage.StagingReadback;
            desc.Flags = GPUBufferFlags.None;
            desc.InitData = IntPtr.Zero;
            return desc;
        }

        /// <summary>
        /// Gets the staging (CPU read/write) description for this instance.
        /// </summary>
        /// <returns>A staging buffer description</returns>
        public GPUBufferDescription ToStaging()
        {
            var desc = this;
            desc.Usage = GPUResourceUsage.Staging;
            desc.Flags = GPUBufferFlags.None;
            desc.InitData = IntPtr.Zero;
            return desc;
        }

        /// <inheritdoc />
        public bool Equals(GPUBufferDescription other)
        {
            return Size == other.Size &&
                   Stride == other.Stride &&
                   Flags == other.Flags &&
                   Format == other.Format &&
                   InitData.Equals(other.InitData) &&
                   Usage == other.Usage;
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is GPUBufferDescription other && Equals(other);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                var hashCode = (int)Size;
                hashCode = (hashCode * 397) ^ (int)Stride;
                hashCode = (hashCode * 397) ^ (int)Flags;
                hashCode = (hashCode * 397) ^ (int)Format;
                hashCode = (hashCode * 397) ^ InitData.GetHashCode();
                hashCode = (hashCode * 397) ^ (int)Usage;
                return hashCode;
            }
        }
    }
}
