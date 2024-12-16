// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
    }
}
