// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;

namespace FlaxEngine
{
    partial class MeshBase
    {
        /// <summary>
        /// Gets the material slot used by this mesh during rendering.
        /// </summary>
        public MaterialSlot MaterialSlot => ModelBase.MaterialSlots[MaterialSlotIndex];

        /// <summary>
        /// Gets a format of the mesh index buffer.
        /// </summary>
        public PixelFormat IndexBufferFormat => Use16BitIndexBuffer ? PixelFormat.R16_UInt : PixelFormat.R32_UInt;

        /// <summary>
        /// Updates the model mesh index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// </summary>
        /// <param name="triangles">The mesh index buffer (triangles). Uses 32-bit stride buffer. Cannot be null.</param>
        public void UpdateTriangles(int[] triangles)
        {
            if (!ModelBase.IsVirtual)
                throw new InvalidOperationException("Only virtual models can be updated at runtime.");
            if (triangles == null)
                throw new ArgumentNullException(nameof(triangles));
            if (triangles.Length == 0 || triangles.Length % 3 != 0)
                throw new ArgumentOutOfRangeException(nameof(triangles));

            if (Internal_UpdateTrianglesUInt(__unmanagedPtr, triangles.Length / 3, triangles))
                throw new Exception("Failed to update mesh data.");
        }

        /// <summary>
        /// Updates the model mesh index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// </summary>
        /// <param name="triangles">The mesh index buffer (triangles). Uses 32-bit stride buffer. Cannot be null.</param>
        public void UpdateTriangles(List<int> triangles)
        {
            if (!ModelBase.IsVirtual)
                throw new InvalidOperationException("Only virtual models can be updated at runtime.");
            if (triangles == null)
                throw new ArgumentNullException(nameof(triangles));
            if (triangles.Count == 0 || triangles.Count % 3 != 0)
                throw new ArgumentOutOfRangeException(nameof(triangles));

            if (Internal_UpdateTrianglesUInt(__unmanagedPtr, triangles.Count / 3, Utils.ExtractArrayFromList(triangles)))
                throw new Exception("Failed to update mesh data.");
        }

        /// <summary>
        /// Updates the model mesh index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// </summary>
        /// <param name="triangles">The mesh index buffer (triangles). Uses 16-bit stride buffer. Cannot be null.</param>
        public void UpdateTriangles(ushort[] triangles)
        {
            if (!ModelBase.IsVirtual)
                throw new InvalidOperationException("Only virtual models can be updated at runtime.");
            if (triangles == null)
                throw new ArgumentNullException(nameof(triangles));
            if (triangles.Length == 0 || triangles.Length % 3 != 0)
                throw new ArgumentOutOfRangeException(nameof(triangles));

            if (Internal_UpdateTrianglesUShort(__unmanagedPtr, triangles.Length / 3, triangles))
                throw new Exception("Failed to update mesh data.");
        }

        /// <summary>
        /// Updates the model mesh index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// </summary>
        /// <param name="triangles">The mesh index buffer (triangles). Uses 16-bit stride buffer. Cannot be null.</param>
        public void UpdateTriangles(List<ushort> triangles)
        {
            if (!ModelBase.IsVirtual)
                throw new InvalidOperationException("Only virtual models can be updated at runtime.");
            if (triangles == null)
                throw new ArgumentNullException(nameof(triangles));
            if (triangles.Count == 0 || triangles.Count % 3 != 0)
                throw new ArgumentOutOfRangeException(nameof(triangles));

            if (Internal_UpdateTrianglesUShort(__unmanagedPtr, triangles.Count / 3, Utils.ExtractArrayFromList(triangles)))
                throw new Exception("Failed to update mesh data.");
        }

        /// <summary>
        /// Downloads the index buffer that contains mesh triangles data. To download data from GPU set <paramref name="forceGpu"/> to true and call this method from the thread other than main thread (see <see cref="Platform.IsInMainThread"/>).
        /// </summary>
        /// <remarks>If mesh index buffer format (see <see cref="IndexBufferFormat"/>) is <see cref="PixelFormat.R16_UInt"/> then it's faster to call .</remarks>
        /// <param name="forceGpu">If set to <c>true</c> the data will be downloaded from the GPU, otherwise it can be loaded from the drive (source asset file) or from memory (if cached). Downloading mesh from GPU requires this call to be made from the other thread than main thread. Virtual assets are always downloaded from GPU memory due to lack of dedicated storage container for the asset data.</param>
        /// <returns>The gathered data.</returns>
        public uint[] DownloadIndexBuffer(bool forceGpu = false)
        {
            var result = (uint[])Internal_DownloadIndexBuffer(__unmanagedPtr, forceGpu, typeof(uint), false);
            if (result == null)
                throw new Exception("Failed to download mesh data.");
            return result;
        }

        /// <summary>
        /// Downloads the index buffer that contains mesh triangles data. To download data from GPU set <paramref name="forceGpu"/> to true and call this method from the thread other than main thread (see <see cref="Platform.IsInMainThread"/>).
        /// </summary>
        /// <remarks>If mesh index buffer format (see <see cref="IndexBufferFormat"/>) is <see cref="PixelFormat.R32_UInt"/> then data won't be downloaded.</remarks>
        /// <param name="forceGpu">If set to <c>true</c> the data will be downloaded from the GPU, otherwise it can be loaded from the drive (source asset file) or from memory (if cached). Downloading mesh from GPU requires this call to be made from the other thread than main thread. Virtual assets are always downloaded from GPU memory due to lack of dedicated storage container for the asset data.</param>
        /// <returns>The gathered data.</returns>
        public ushort[] DownloadIndexBufferUShort(bool forceGpu = false)
        {
            var result = (ushort[])Internal_DownloadIndexBuffer(__unmanagedPtr, forceGpu, typeof(ushort), true);
            if (result == null)
                throw new Exception("Failed to download mesh data.");
            return result;
        }

        /// <summary>
        /// Extracts mesh buffers data.
        /// </summary>
        /// <param name="types">List of buffers to load.</param>
        /// <param name="buffers">The result mesh buffers.</param>
        /// <param name="layouts">The result layouts of the vertex buffers.</param>
        /// <param name="forceGpu">If set to <c>true</c> the data will be downloaded from the GPU, otherwise it can be loaded from the drive (source asset file) or from memory (if cached). Downloading mesh from GPU requires this call to be made from the other thread than main thread. Virtual assets are always downloaded from GPU memory due to lack of dedicated storage container for the asset data.</param>
        /// <returns>True if failed, otherwise false</returns>
        public unsafe bool DownloadData(Span<MeshBufferType> types, out byte[][] buffers, out GPUVertexLayout[] layouts, bool forceGpu = false)
        {
            if (types == null)
                throw new ArgumentNullException(nameof(types));
            buffers = null;
            layouts = null;
            var count = types.Length;
            fixed (MeshBufferType* typesPtr = types)
            {
                if (Internal_DownloadData(__unmanagedPtr,
                                          count, typesPtr,
                                          out byte[] buffer0, out byte[] buffer1, out byte[] buffer2, out byte[] buffer3,
                                          out GPUVertexLayout layout0, out GPUVertexLayout layout1, out GPUVertexLayout layout2, out GPUVertexLayout layout3,
                                          forceGpu,
                                          out int __buffer0Count, out int __buffer1Count, out int __buffer2Count, out int __buffer3Count
                                         ))
                    return true;
                buffers = new byte[count][];
                layouts = new GPUVertexLayout[count];
                if (count > 0)
                {
                    buffers[0] = buffer0;
                    layouts[0] = layout0;
                    if (count > 1)
                    {
                        buffers[1] = buffer1;
                        layouts[1] = layout1;
                        if (count > 2)
                        {
                            buffers[2] = buffer2;
                            layouts[2] = layout2;
                            if (count > 3)
                            {
                                buffers[3] = buffer3;
                                layouts[3] = layout3;
                            }
                        }
                    }
                }
            }
            return false;
        }
    }
}
