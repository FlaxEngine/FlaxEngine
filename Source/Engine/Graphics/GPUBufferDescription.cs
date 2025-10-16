// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Runtime.CompilerServices;
using FlaxEngine.Interop;

namespace FlaxEngine
{
    partial class GPUDevice
    {
        /// <summary>
        /// Gets the list with all active GPU resources.
        /// </summary>
        public GPUResource[] Resources
        {
            get
            {
                IntPtr ptr = Internal_GetResourcesInternal(__unmanagedPtr);
                ManagedArray array = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(ptr).Target);
                return NativeInterop.GCHandleArrayToManagedArray<GPUResource>(array);
            }
        }

        /// <summary>
        /// Gets the list with all active GPU resources.
        /// </summary>
        /// <param name="buffer">Output buffer to fill with resource pointers. Can be provided by a user to avoid memory allocation. Buffer might be larger than actual list size. Use <paramref name="count"/> for actual item count.></param>
        /// <param name="count">Amount of valid items inside <paramref name="buffer"/>.</param>
        public void GetResources(ref GPUResource[] buffer, out int count)
        {
            count = 0;
            IntPtr ptr = Internal_GetResourcesInternal(__unmanagedPtr);
            ManagedArray array = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(ptr).Target);
            buffer = NativeInterop.GCHandleArrayToManagedArray<GPUResource>(array, buffer);
            count = buffer.Length;
        }
    }

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
            desc.VertexLayout = null;
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
            desc.VertexLayout = null;
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
        /// <param name="layout">The vertex buffer layout.</param>
        /// <param name="elementStride">The element stride.</param>
        /// <param name="elementsCount">The elements count.</param>
        /// <param name="data">The data.</param>
        /// <returns>The buffer description.</returns>
        public static GPUBufferDescription Vertex(GPUVertexLayout layout, int elementStride, int elementsCount, IntPtr data)
        {
            GPUBufferDescription desc;
            desc.Size = (uint)(elementsCount * elementStride);
            desc.Stride = (uint)elementStride;
            desc.Flags = GPUBufferFlags.VertexBuffer;
            desc.Format = PixelFormat.Unknown;
            desc.InitData = data;
            desc.Usage = GPUResourceUsage.Default;
            desc.VertexLayout = layout;
            return desc;
        }

        /// <summary>
        /// Creates vertex buffer description.
        /// </summary>
        /// <param name="layout">The vertex buffer layout.</param>
        /// <param name="elementStride">The element stride.</param>
        /// <param name="elementsCount">The elements count.</param>
        /// <param name="usage">The usage mode.</param>
        /// <returns>The buffer description.</returns>
        public static GPUBufferDescription Vertex(GPUVertexLayout layout, int elementStride, int elementsCount, GPUResourceUsage usage = GPUResourceUsage.Default)
        {
            GPUBufferDescription desc;
            desc.Size = (uint)(elementsCount * elementStride);
            desc.Stride = (uint)elementStride;
            desc.Flags = GPUBufferFlags.VertexBuffer;
            desc.Format = PixelFormat.Unknown;
            desc.InitData = new IntPtr();
            desc.Usage = usage;
            desc.VertexLayout = layout;
            return desc;
        }

        /// <summary>
        /// Creates vertex buffer description.
        /// </summary>
        /// <param name="elementStride">The element stride.</param>
        /// <param name="elementsCount">The elements count.</param>
        /// <param name="data">The data.</param>
        /// <param name="layout">The vertex buffer layout.</param>
        /// <returns>The buffer description.</returns>
        [Obsolete("Use Vertex with vertex layout parameter instead")]
        public static GPUBufferDescription Vertex(int elementStride, int elementsCount, IntPtr data, GPUVertexLayout layout = null)
        {
            return Buffer(elementsCount * elementStride, GPUBufferFlags.VertexBuffer, PixelFormat.Unknown, data, elementStride, GPUResourceUsage.Default);
        }

        /// <summary>
        /// Creates vertex buffer description.
        /// </summary>
        /// <param name="elementStride">The element stride.</param>
        /// <param name="elementsCount">The elements count.</param>
        /// <param name="usage">The usage mode.</param>
        /// <param name="layout">The vertex buffer layout.</param>
        /// <returns>The buffer description.</returns>
        [Obsolete("Use Vertex with vertex layout parameter instead")]
        public static GPUBufferDescription Vertex(int elementStride, int elementsCount, GPUResourceUsage usage = GPUResourceUsage.Default, GPUVertexLayout layout = null)
        {
            return Buffer(elementsCount * elementStride, GPUBufferFlags.VertexBuffer, PixelFormat.Unknown, new IntPtr(), elementStride, usage);
        }

        /// <summary>
        /// Creates vertex buffer description.
        /// [Deprecated in v1.10]
        /// </summary>
        /// <param name="size">The size (in bytes).</param>
        /// <param name="usage">The usage mode.</param>
        /// <returns>The buffer description.</returns>
        [Obsolete("Use Vertex with separate vertex stride and count instead")]
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
                   Usage == other.Usage &&
                   VertexLayout == other.VertexLayout;
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
                if (VertexLayout != null)
                    hashCode = (hashCode * 397) ^ VertexLayout.GetHashCode();
                return hashCode;
            }
        }
    }

    partial struct VertexElement : IEquatable<VertexElement>
    {
        /// <summary>
        /// Creates the vertex element description.
        /// </summary>
        /// <param name="type">Element type.</param>
        /// <param name="slot">Vertex buffer bind slot.</param>
        /// <param name="offset">Data byte offset.</param>
        /// <param name="perInstance">True if element data is instanced.</param>
        /// <param name="format">Data format.</param>
        public VertexElement(Types type, byte slot, byte offset, bool perInstance, PixelFormat format)
        {
            Type = type;
            Slot = slot;
            Offset = offset;
            PerInstance = (byte)(perInstance ? 1 : 0);
            Format = format;
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return string.Format("{0}, {1}, offset {2}, {3}, slot {3}", Type, Format, Offset, PerInstance != 0 ? "per-instance" : "per-vertex", Slot);
        }

        /// <inheritdoc />
        public bool Equals(VertexElement other)
        {
            return Type == other.Type &&
                   Slot == other.Slot &&
                   Offset == other.Offset &&
                   PerInstance == other.PerInstance &&
                   Format == other.Format;
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is VertexElement other && Equals(other);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                var hashCode = (int)Type;
                hashCode = (hashCode * 397) ^ Slot;
                hashCode = (hashCode * 397) ^ Offset;
                hashCode = (hashCode * 397) ^ PerInstance;
                hashCode = (hashCode * 397) ^ (int)Format;
                return hashCode;
            }
        }
    }
}
