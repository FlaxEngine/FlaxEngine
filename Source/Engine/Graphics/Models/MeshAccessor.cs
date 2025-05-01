// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Runtime.InteropServices;

namespace FlaxEngine
{
    /// <summary>
    /// General purpose utility for accessing mesh data (both read and write).
    /// </summary>
    public class MeshAccessor
    {
        /// <summary>
        /// Mesh data stream.
        /// </summary>
        public unsafe ref struct Stream
        {
            private Span<byte> _data;
            private PixelFormat _format;
            private int _stride;
            private readonly PixelFormatSampler _sampler;

            internal Stream(Span<byte> data, PixelFormat format, int stride)
            {
                _data = data;
                _stride = stride;
                if (PixelFormatSampler.Get(format, out _sampler))
                {
                    _format = format;
                }
                else if (format != PixelFormat.Unknown)
                {
                    Debug.LogError($"Unsupported pixel format '{format}' to sample vertex attribute.");
                }
            }

            /// <summary>
            /// Gets the raw data block.
            /// </summary>
            public Span<byte> Data => _data;

            /// <summary>
            /// Gets the format of the data.
            /// </summary>
            public PixelFormat Format => _format;

            /// <summary>
            /// Gets the stride (in bytes) of the data.
            /// </summary>
            public int Stride => _stride;

            /// <summary>
            /// Gets the count of the items in the stride.
            /// </summary>
            public int Count => _data.Length / _stride;

            /// <summary>
            /// Returns true if stream is valid.
            /// </summary>
            public bool IsValid => _format != PixelFormat.Unknown;

            /// <summary>
            /// Checks if the stream can use <seealso cref="SetLinear"/> via a single memory copy.
            /// </summary>
            /// <param name="expectedFormat">Source data format.</param>
            /// <returns>True if stream is linear and format matches expected input data.</returns>
            public bool IsLinear(PixelFormat expectedFormat)
            {
                return _format == expectedFormat && _stride == PixelFormatExtensions.SizeInBytes(_format);
            }

            /// <summary>
            /// Reads an integer value from a given item.
            /// </summary>
            /// <param name="index">Zero-based index of the item.</param>
            /// <returns>Loaded value.</returns>
            public int GetInt(int index)
            {
                fixed (byte* data = _data)
                    return (int)_sampler.Read(data + index * _stride).X;
            }

            /// <summary>
            /// Reads a float value from a given item.
            /// </summary>
            /// <param name="index">Zero-based index of the item.</param>
            /// <returns>Loaded value.</returns>
            public float GetFloat(int index)
            {
                fixed (byte* data = _data)
                    return _sampler.Read(data + index * _stride).X;
            }

            /// <summary>
            /// Reads a Float2 value from a given item.
            /// </summary>
            /// <param name="index">Zero-based index of the item.</param>
            /// <returns>Loaded value.</returns>
            public Float2 GetFloat2(int index)
            {
                fixed (byte* data = _data)
                    return new Float2(_sampler.Read(data + index * _stride));
            }

            /// <summary>
            /// Reads a Float3 value from a given item.
            /// </summary>
            /// <param name="index">Zero-based index of the item.</param>
            /// <returns>Loaded value.</returns>
            public Float3 GetFloat3(int index)
            {
                fixed (byte* data = _data)
                    return new Float3(_sampler.Read(data + index * _stride));
            }

            /// <summary>
            /// Reads a Float4 value from a given item.
            /// </summary>
            /// <param name="index">Zero-based index of the item.</param>
            /// <returns>Loaded value.</returns>
            public Float4 GetFloat4(int index)
            {
                fixed (byte* data = _data)
                    return _sampler.Read(data + index * _stride);
            }

            /// <summary>
            /// Writes an integer value to a given item.
            /// </summary>
            /// <param name="index">Zero-based index of the item.</param>
            /// <param name="value">Value to assign.</param>
            public void SetInt(int index, int value)
            {
                var v = new Float4(value);
                fixed (byte* data = _data)
                    _sampler.Write(data + index * _stride, ref v);
            }

            /// <summary>
            /// Writes a float value to a given item.
            /// </summary>
            /// <param name="index">Zero-based index of the item.</param>
            /// <param name="value">Value to assign.</param>
            public void SetFloat(int index, float value)
            {
                var v = new Float4(value);
                fixed (byte* data = _data)
                    _sampler.Write(data + index * _stride, ref v);
            }

            /// <summary>
            /// Writes a Float2 value to a given item.
            /// </summary>
            /// <param name="index">Zero-based index of the item.</param>
            /// <param name="value">Value to assign.</param>
            public void SetFloat2(int index, Float2 value)
            {
                var v = new Float4(value, 0.0f, 0.0f);
                fixed (byte* data = _data)
                    _sampler.Write(data + index * _stride, ref v);
            }

            /// <summary>
            /// Writes a Float3 value to a given item.
            /// </summary>
            /// <param name="index">Zero-based index of the item.</param>
            /// <param name="value">Value to assign.</param>
            public void SetFloat3(int index, Float3 value)
            {
                var v = new Float4(value, 0.0f);
                fixed (byte* data = _data)
                    _sampler.Write(data + index * _stride, ref v);
            }

            /// <summary>
            /// Writes a Float4 value to a given item.
            /// </summary>
            /// <param name="index">Zero-based index of the item.</param>
            /// <param name="value">Value to assign.</param>
            public void SetFloat4(int index, Float4 value)
            {
                fixed (byte* data = _data)
                    _sampler.Write(data + index * _stride, ref value);
            }

            /// <summary>
            /// Copies a linear block of data into the stream. Assumes <seealso cref="IsLinear"/> returned true for the format of the input data.
            /// </summary>
            /// <remarks>Check input data and stream type with IsLinear before calling.</remarks>
            /// <param name="data">Pointer to the source data.</param>
            public void SetLinear(IntPtr data)
            {
                new Span<byte>(data.ToPointer(), _data.Length).CopyTo(_data);
            }

            /// <summary>
            /// Copies the contents of the input <see cref="T:System.Span`1"/> into the elements of this stream.
            /// </summary>
            /// <param name="src">The source <see cref="T:System.Span`1"/>.</param>
            public void Set(Span<Float2> src)
            {
                if (IsLinear(PixelFormat.R32G32_Float))
                {
                    src.CopyTo(MemoryMarshal.Cast<byte, Float2>(_data));
                }
                else
                {
                    var count = Count;
                    fixed (byte* data = _data)
                    {
                        for (int i = 0; i < count; i++)
                        {
                            var v = new Float4(src[i], 0.0f, 0.0f);
                            _sampler.Write(data + i * _stride, ref v);
                        }
                    }
                }
            }

            /// <summary>
            /// Copies the contents of the input <see cref="T:System.Span`1"/> into the elements of this stream.
            /// </summary>
            /// <param name="src">The source <see cref="T:System.Span`1"/>.</param>
            public void Set(Span<Float3> src)
            {
                if (IsLinear(PixelFormat.R32G32B32_Float))
                {
                    src.CopyTo(MemoryMarshal.Cast<byte, Float3>(_data));
                }
                else
                {
                    var count = Count;
                    fixed (byte* data = _data)
                    {
                        for (int i = 0; i < count; i++)
                        {
                            var v = new Float4(src[i], 0.0f);
                            _sampler.Write(data + i * _stride, ref v);
                        }
                    }
                }
            }

            /// <summary>
            /// Copies the contents of the input <see cref="T:System.Span`1"/> into the elements of this stream.
            /// </summary>
            /// <param name="src">The source <see cref="T:System.Span`1"/>.</param>
            public void Set(Span<Color> src)
            {
                if (IsLinear(PixelFormat.R32G32B32A32_Float))
                {
                    src.CopyTo(MemoryMarshal.Cast<byte, Color>(_data));
                }
                else
                {
                    var count = Count;
                    fixed (byte* data = _data)
                    {
                        for (int i = 0; i < count; i++)
                        {
                            var v = (Float4)src[i];
                            _sampler.Write(data + i * _stride, ref v);
                        }
                    }
                }
            }

            /// <summary>
            /// Copies the contents of this stream into a destination <see cref="T:System.Span`1" />.
            /// </summary>
            /// <param name="dst">The destination <see cref="T:System.Span`1" />.</param>
            public void CopyTo(Span<Float2> dst)
            {
                if (IsLinear(PixelFormat.R32G32_Float))
                {
                    _data.CopyTo(MemoryMarshal.Cast<Float2, byte>(dst));
                }
                else
                {
                    var count = Count;
                    fixed (byte* data = _data)
                    {
                        for (int i = 0; i < count; i++)
                        {
                            dst[i] = new Float2(_sampler.Read(data + i * _stride));
                        }
                    }
                }
            }

            /// <summary>
            /// Copies the contents of this stream into a destination <see cref="T:System.Span`1" />.
            /// </summary>
            /// <param name="dst">The destination <see cref="T:System.Span`1" />.</param>
            public void CopyTo(Span<Float3> dst)
            {
                if (IsLinear(PixelFormat.R32G32B32_Float))
                {
                    _data.CopyTo(MemoryMarshal.Cast<Float3, byte>(dst));
                }
                else
                {
                    var count = Count;
                    fixed (byte* data = _data)
                    {
                        for (int i = 0; i < count; i++)
                        {
                            dst[i] = new Float3(_sampler.Read(data + i * _stride));
                        }
                    }
                }
            }

            /// <summary>
            /// Copies the contents of this stream into a destination <see cref="T:System.Span`1" />.
            /// </summary>
            /// <param name="dst">The destination <see cref="T:System.Span`1" />.</param>
            public void CopyTo(Span<Color> dst)
            {
                if (IsLinear(PixelFormat.R32G32B32A32_Float))
                {
                    _data.CopyTo(MemoryMarshal.Cast<Color, byte>(dst));
                }
                else
                {
                    var count = Count;
                    fixed (byte* data = _data)
                    {
                        for (int i = 0; i < count; i++)
                        {
                            dst[i] = (Color)_sampler.Read(data + i * _stride);
                        }
                    }
                }
            }
        }

        private byte[][] _data = new byte[(int)MeshBufferType.MAX][];
        private PixelFormat[] _formats = new PixelFormat[(int)MeshBufferType.MAX];
        private GPUVertexLayout[] _layouts = new GPUVertexLayout[(int)MeshBufferType.MAX];

        /// <summary>
        /// Loads the data from the mesh.
        /// </summary>
        /// <param name="mesh">The source mesh object to access.</param>
        /// <param name="forceGpu">If set to <c>true</c> the data will be downloaded from the GPU, otherwise it can be loaded from the drive (source asset file) or from memory (if cached). Downloading mesh from GPU requires this call to be made from the other thread than main thread. Virtual assets are always downloaded from GPU memory due to lack of dedicated storage container for the asset data.</param>
        /// <param name="buffers">Custom list of mesh buffers to load. Use empty value to access all of them.</param>
        /// <returns>True if failed, otherwise false.</returns>
        public bool LoadMesh(MeshBase mesh, bool forceGpu = false, Span<MeshBufferType> buffers = new())
        {
            if (mesh == null)
                return true;
            Span<MeshBufferType> allBuffers = stackalloc MeshBufferType[(int)MeshBufferType.MAX] { MeshBufferType.Index, MeshBufferType.Vertex0, MeshBufferType.Vertex1, MeshBufferType.Vertex2 };
            var buffersLocal = buffers.IsEmpty ? allBuffers : buffers;
            if (mesh.DownloadData(buffersLocal.ToArray(), out var meshBuffers, out var meshLayouts, forceGpu))
                return true;
            for (int i = 0; i < buffersLocal.Length; i++)
            {
                _data[(int)buffersLocal[i]] = meshBuffers[i];
                _layouts[(int)buffersLocal[i]] = meshLayouts[i];
            }
            _formats[(int)MeshBufferType.Index] = mesh.Use16BitIndexBuffer ? PixelFormat.R16_UInt : PixelFormat.R32_UInt;
            return false;
        }

        /// <summary>
        /// Loads the data from the provided mesh buffer.
        /// </summary>
        /// <param name="bufferType">Type of the mesh buffer to load.</param>
        /// <param name="bufferData">Data used by that buffer.</param>
        /// <param name="layout">Layout of the elements in the buffer.</param>
        /// <returns>True if failed, otherwise false.</returns>
        public bool LoadBuffer(MeshBufferType bufferType, Span<byte> bufferData, GPUVertexLayout layout)
        {
            if (bufferData.IsEmpty)
                return true;
            if (layout == null)
                return true;
            int idx = (int)bufferType;
            _data[idx] = bufferData.ToArray();
            _layouts[idx] = layout;
            return false;
        }

        /// <summary>
        /// Allocates the data for the mesh vertex buffer.
        /// </summary>
        /// <param name="bufferType">Type of the mesh buffer to initialize.</param>
        /// <param name="count">Amount of items in the buffer.</param>
        /// <param name="layout">Layout of the elements in the buffer.</param>
        /// <returns>True if failed, otherwise false.</returns>
        public bool AllocateBuffer(MeshBufferType bufferType, int count, GPUVertexLayout layout)
        {
            if (count <= 0)
                return true;
            if (layout == null)
                return true;
            int idx = (int)bufferType;
            _data[idx] = new byte[count * layout.Stride];
            _layouts[idx] = layout;
            return false;
        }

        /// <summary>
        /// Allocates the data for the mesh buffer.
        /// </summary>
        /// <param name="bufferType">Type of the mesh buffer to initialize.</param>
        /// <param name="count">Amount of items in the buffer.</param>
        /// <param name="format">Format of the elements in the buffer.</param>
        /// <returns>True if failed, otherwise false.</returns>
        public bool AllocateBuffer(MeshBufferType bufferType, int count, PixelFormat format)
        {
            if (count <= 0)
                return true;
            int stride = PixelFormatExtensions.SizeInBytes(format);
            if (stride <= 0)
                return true;
            int idx = (int)bufferType;
            _data[idx] = new byte[count * stride];
            _formats[idx] = format;
            return false;
        }

        /// <summary>
        /// Updates the mesh vertex and index buffers with data assigned to the accessor (eg. via AllocateBuffer).
        /// </summary>
        /// <param name="mesh">The target mesh to update.</param>
        /// <param name="calculateBounds">True if auto-calculate mesh local bounding box based on input positions stream. Otherwise, mesh bound swill stay unchanged.</param>
        /// <returns>True if failed, otherwise false.</returns>
        public unsafe bool UpdateMesh(MeshBase mesh, bool calculateBounds = true)
        {
            if (mesh == null)
                return true;
            int IB = (int)MeshBufferType.Index;
            int VB0 = (int)MeshBufferType.Vertex0;
            int VB1 = (int)MeshBufferType.Vertex1;
            int VB2 = (int)MeshBufferType.Vertex2;

            uint vertices = 0, triangles = 0;
            fixed (byte* data0Ptr = _data[0])
            fixed (byte* data1Ptr = _data[1])
            fixed (byte* data2Ptr = _data[2])
            fixed (byte* data3Ptr = _data[3])
            {
                Span<IntPtr> dataPtr = stackalloc IntPtr[(int)MeshBufferType.MAX] { new IntPtr(data0Ptr), new IntPtr(data1Ptr), new IntPtr(data2Ptr), new IntPtr(data3Ptr) };
                IntPtr ibData = IntPtr.Zero;
                bool use16BitIndexBuffer = false;
                IntPtr[] vbData = new IntPtr[3];
                GPUVertexLayout[] vbLayout = new GPUVertexLayout[3];
                if (_data[VB0] != null)
                {
                    vbData[0] = dataPtr[VB0];
                    vbLayout[0] = _layouts[VB0];
                    vertices = (uint)_data[VB0].Length / _layouts[VB0].Stride;
                }
                if (_data[VB1] != null)
                {
                    vbData[1] = dataPtr[VB1];
                    vbLayout[1] = _layouts[VB1];
                    vertices = (uint)_data[VB1].Length / _layouts[VB1].Stride;
                }
                if (_data[VB2] != null)
                {
                    vbData[2] = dataPtr[VB2];
                    vbLayout[2] = _layouts[VB2];
                    vertices = (uint)_data[VB2].Length / _layouts[VB2].Stride;
                }
                if (_data[IB] != null && _formats[IB] != PixelFormat.Unknown)
                {
                    ibData = dataPtr[IB];
                    use16BitIndexBuffer = _formats[IB] == PixelFormat.R16_UInt;
                    triangles = (uint)(_data[IB].Length / PixelFormatExtensions.SizeInBytes(_formats[IB]));
                }

                if (mesh.Init(vertices, triangles, vbData, ibData, use16BitIndexBuffer, vbLayout))
                    return true;
            }

            if (calculateBounds)
            {
                // Calculate mesh bounds
                BoundingBox bounds;
                var positionStream = Position();
                if (!positionStream.IsValid)
                    return true;
                if (positionStream.IsLinear(PixelFormat.R32G32B32_Float))
                {
                    Span<byte> positionData = positionStream.Data;
                    BoundingBox.FromPoints(MemoryMarshal.Cast<byte, Float3>(positionData), out bounds);
                }
                else
                {
                    Float3 min = Float3.Maximum, max = Float3.Minimum;
                    for (int i = 0; i < vertices; i++)
                    {
                        Float3 pos = positionStream.GetFloat3(i);
                        Float3.Min(ref min, ref pos, out min);
                        Float3.Max(ref max, ref pos, out max);
                    }
                    bounds = new BoundingBox(min, max);
                }
                mesh.SetBounds(ref bounds);
            }

            return false;
        }

        /// <summary>
        /// Access stream with index buffer.
        /// </summary>
        /// <returns>Mesh data stream (might be invalid if data is not provided).</returns>
        public Stream Index()
        {
            Span<byte> data = new Span<byte>();
            PixelFormat format = PixelFormat.Unknown;
            int stride = 0;
            var ib = _data[(int)MeshBufferType.Index];
            if (ib != null)
            {
                data = ib;
                format = _formats[(int)MeshBufferType.Index];
                stride = PixelFormatExtensions.SizeInBytes(format);
            }
            return new Stream(data, format, stride);
        }

        /// <summary>
        /// Access stream with a specific vertex attribute.
        /// </summary>
        /// <param name="attribute">Type of the attribute.</param>
        /// <returns>Mesh data stream (might be invalid if attribute is not provided).</returns>
        public Stream Attribute(VertexElement.Types attribute)
        {
            Span<byte> data = new Span<byte>();
            PixelFormat format = PixelFormat.Unknown;
            int stride = 0;
            for (int vbIndex = 0; vbIndex < 3 && format == PixelFormat.Unknown; vbIndex++)
            {
                int idx = vbIndex + 1;
                var layout = _layouts[idx];
                if (!layout)
                    continue;
                foreach (VertexElement e in layout.Elements)
                {
                    var vb = _data[idx];
                    if (e.Type == attribute && vb != null)
                    {
                        data = new Span<byte>(vb).Slice(e.Offset);
                        format = e.Format;
                        stride = (int)layout.Stride;
                        break;
                    }
                }
            }
            return new Stream(data, format, stride);
        }

        /// <summary>
        /// Access stream with vertex position attribute.
        /// </summary>
        /// <returns>Mesh data stream (might be invalid if attribute is not provided).</returns>
        public Stream Position()
        {
            return Attribute(VertexElement.Types.Position);
        }

        /// <summary>
        /// Access stream with vertex color attribute.
        /// </summary>
        /// <returns>Mesh data stream (might be invalid if attribute is not provided).</returns>
        public Stream Color()
        {
            return Attribute(VertexElement.Types.Color);
        }

        /// <summary>
        /// Access stream with vertex normal vector attribute.
        /// </summary>
        /// <returns>Mesh data stream (might be invalid if attribute is not provided).</returns>
        public Stream Normal()
        {
            return Attribute(VertexElement.Types.Normal);
        }

        /// <summary>
        /// Access stream with vertex tangent vector attribute.
        /// </summary>
        /// <returns>Mesh data stream (might be invalid if attribute is not provided).</returns>
        public Stream Tangent()
        {
            return Attribute(VertexElement.Types.Tangent);
        }

        /// <summary>
        /// Access stream with vertex skeleton bones blend indices attribute.
        /// </summary>
        /// <returns>Mesh data stream (might be invalid if attribute is not provided).</returns>
        public Stream BlendIndices()
        {
            return Attribute(VertexElement.Types.BlendIndices);
        }

        /// <summary>
        /// Access stream with vertex skeleton bones blend weights attribute.
        /// </summary>
        /// <returns>Mesh data stream (might be invalid if attribute is not provided).</returns>
        public Stream BlendWeights()
        {
            return Attribute(VertexElement.Types.BlendWeights);
        }

        /// <summary>
        /// Access stream with vertex texture coordinates attribute (specific UV channel).
        /// </summary>
        /// <param name="channel">UV channel index (zero-based).</param>
        /// <returns>Mesh data stream (might be invalid if attribute is not provided).</returns>
        public Stream TexCoord(int channel = 0)
        {
            return Attribute((VertexElement.Types)((byte)VertexElement.Types.TexCoord0 + channel));
        }

        /// <summary>
        /// Gets or sets the vertex positions. Null if <see cref="VertexElement.Types.Position"/> does not exist in vertex buffers of the mesh.
        /// </summary>
        /// <remarks>Uses <see cref="Position"/> stream to read or write data to the vertex buffer.</remarks>
        public Float3[] Positions
        {
            get => GetStreamFloat3(VertexElement.Types.Position);
            set => SetStreamFloat3(VertexElement.Types.Position, value);
        }

        /// <summary>
        /// Gets or sets the vertex colors. Null if <see cref="VertexElement.Types.Color"/> does not exist in vertex buffers of the mesh.
        /// </summary>
        /// <remarks>Uses <see cref="Color"/> stream to read or write data to the vertex buffer.</remarks>
        public Color[] Colors
        {
            get => GetStreamColor(VertexElement.Types.Color);
            set => SetStreamColor(VertexElement.Types.Color, value);
        }

        /// <summary>
        /// Gets or sets the vertex normal vectors (unpacked, normalized). Null if <see cref="VertexElement.Types.Normal"/> does not exist in vertex buffers of the mesh.
        /// </summary>
        /// <remarks>Uses <see cref="Normal"/> stream to read or write data to the vertex buffer.</remarks>
        public Float3[] Normals
        {
            get => GetStreamFloat3(VertexElement.Types.Normal, UnpackNormal);
            set => SetStreamFloat3(VertexElement.Types.Normal, value, PackNormal);
        }

        /// <summary>
        /// Gets or sets the vertex UVs (texcoord channel 0). Null if <see cref="VertexElement.Types.TexCoord"/> does not exist in vertex buffers of the mesh.
        /// </summary>
        /// <remarks>Uses <see cref="TexCoord"/> stream to read or write data to the vertex buffer.</remarks>
        public Float2[] TexCoords
        {
            get => GetStreamFloat2(VertexElement.Types.TexCoord);
            set => SetStreamFloat2(VertexElement.Types.TexCoord, value);
        }

        private delegate void TransformDelegate3(ref Float3 value);

        private Float3[] GetStreamFloat3(VertexElement.Types attribute, TransformDelegate3 transform = null)
        {
            Float3[] result = null;
            var stream = Attribute(attribute);
            if (stream.IsValid)
            {
                result = new Float3[stream.Count];
                stream.CopyTo(result);
                if (transform != null)
                {
                    for (int i = 0; i < result.Length; i++)
                        transform(ref result[i]);
                }
            }
            return result;
        }

        private void SetStreamFloat3(VertexElement.Types attribute, Float3[] value, TransformDelegate3 transform = null)
        {
            var stream = Attribute(attribute);
            if (stream.IsValid)
            {
                if (transform != null)
                {
                    // TODO: transform in-place?
                    value = (Float3[])value.Clone();
                    for (int i = 0; i < value.Length; i++)
                        transform(ref value[i]);
                }
                stream.Set(value);
            }
        }

        private Float2[] GetStreamFloat2(VertexElement.Types attribute)
        {
            Float2[] result = null;
            var stream = Attribute(attribute);
            if (stream.IsValid)
            {
                result = new Float2[stream.Count];
                stream.CopyTo(result);
            }
            return result;
        }

        private void SetStreamFloat2(VertexElement.Types attribute, Float2[] value)
        {
            var stream = Attribute(attribute);
            if (stream.IsValid)
            {
                stream.Set(value);
            }
        }

        private Color[] GetStreamColor(VertexElement.Types attribute)
        {
            Color[] result = null;
            var stream = Attribute(attribute);
            if (stream.IsValid)
            {
                result = new Color[stream.Count];
                stream.CopyTo(result);
            }
            return result;
        }

        private void SetStreamColor(VertexElement.Types attribute, Color[] value)
        {
            var stream = Attribute(attribute);
            if (stream.IsValid)
            {
                stream.Set(value);
            }
        }

        /// <summary>
        /// Unpacks normal/tangent vector from normalized range to full range.
        /// </summary>
        /// <param name="value">In and out value.</param>
        public static void UnpackNormal(ref Float3 value)
        {
            // [0; 1] -> [-1; 1]
            value = value * 2.0f - 1.0f;
        }

        /// <summary>
        /// Unpacks normal/tangent vector from normalized range to full range.
        /// </summary>
        /// <param name="value">In and out value.</param>
        /// <param name="sign">Encoded sign in the alpha channel.</param>
        public static void UnpackNormal(ref Float4 value, out float sign)
        {
            sign = value.W > Mathf.Epsilon ? -1.0f : +1.0f;

            // [0; 1] -> [-1; 1]
            value = value * 2.0f - 1.0f;
        }

        /// <summary>
        /// Packs normal/tangent vector to normalized range from full range.
        /// </summary>
        /// <param name="value">In and out value.</param>
        public static void PackNormal(ref Float3 value)
        {
            // [-1; 1] -> [0; 1]
            value = value * 0.5f + 0.5f;
        }
    }
}
