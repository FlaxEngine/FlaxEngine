// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    partial class CollisionData
    {
        /// <summary>
        /// Cooks the mesh collision data and updates the virtual asset. action cannot be performed on a main thread.
        /// [Deprecated on 16.06.2022, expires on 16.06.2024]
        /// </summary>
        /// <remarks>
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// </remarks>
        /// <param name="type">The collision data type.</param>
        /// <param name="vertices">The source geometry vertex buffer with vertices positions. Cannot be empty.</param>
        /// <param name="triangles">The source data index buffer (triangles list). Uses 32-bit stride buffer. Cannot be empty. Length must be multiple of 3 (as 3 vertices build a triangle).</param>
        /// <param name="convexFlags">The convex mesh generation flags.</param>
        /// <param name="convexVertexLimit">The convex mesh vertex limit. Use values in range [8;255]</param>
        /// <returns>True if failed, otherwise false.</returns>
        [Obsolete("Use CookCollision with Float3 and Float2 parameters instead")]
        public bool CookCollision(CollisionDataType type, Vector3[] vertices, uint[] triangles, ConvexMeshGenerationFlags convexFlags = ConvexMeshGenerationFlags.None, int convexVertexLimit = 255)
        {
            if (vertices == null)
                throw new ArgumentNullException();
            var tmp = new Float3[vertices.Length];
            for (int i = 0; i < tmp.Length; i++)
                tmp[i] = vertices[i];
            return CookCollision(type, tmp, triangles, convexFlags, convexVertexLimit);
        }

        /// <summary>
        /// Cooks the mesh collision data and updates the virtual asset. action cannot be performed on a main thread.
        /// [Deprecated on 16.06.2022, expires on 16.06.2024]
        /// </summary>
        /// <remarks>
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// </remarks>
        /// <param name="type">The collision data type.</param>
        /// <param name="vertices">The source geometry vertex buffer with vertices positions. Cannot be empty.</param>
        /// <param name="triangles">The source data index buffer (triangles list). Uses 32-bit stride buffer. Cannot be empty. Length must be multiple of 3 (as 3 vertices build a triangle).</param>
        /// <param name="convexFlags">The convex mesh generation flags.</param>
        /// <param name="convexVertexLimit">The convex mesh vertex limit. Use values in range [8;255]</param>
        /// <returns>True if failed, otherwise false.</returns>
        [Obsolete("Use CookCollision with Float3 and Float2 parameters instead")]
        public bool CookCollision(CollisionDataType type, Vector3[] vertices, int[] triangles, ConvexMeshGenerationFlags convexFlags = ConvexMeshGenerationFlags.None, int convexVertexLimit = 255)
        {
            if (vertices == null)
                throw new ArgumentNullException();
            var tmp = new Float3[vertices.Length];
            for (int i = 0; i < tmp.Length; i++)
                tmp[i] = vertices[i];
            return CookCollision(type, tmp, triangles, convexFlags, convexVertexLimit);
        }

        /// <summary>
        /// Extracts the collision data geometry into list of triangles.
        /// [Deprecated on 16.06.2022, expires on 16.06.2024]
        /// </summary>
        /// <param name="vertexBuffer">The output vertex buffer.</param>
        /// <param name="indexBuffer">The output index buffer.</param>
        [Obsolete("Use ExtractGeometry with Float3 and Float2 parameters instead")]
        public void ExtractGeometry(out Vector3[] vertexBuffer, out int[] indexBuffer)
        {
            ExtractGeometry(out Float3[] tmp, out indexBuffer);
            vertexBuffer = new Vector3[tmp.Length];
            for (int i = 0; i < tmp.Length; i++)
                vertexBuffer[i] = tmp[i];
        }
    }
}
