// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    partial class SkinnedMesh
    {
        /// <summary>
        /// The Vertex Buffer 0 structure format.
        /// </summary>
        public struct Vertex0
        {
            /// <summary>
            /// The vertex position.
            /// </summary>
            public Vector3 Position;

            /// <summary>
            /// The texture coordinates (packed).
            /// </summary>
            public Half2 TexCoord;

            /// <summary>
            /// The normal vector (packed).
            /// </summary>
            public FloatR10G10B10A2 Normal;

            /// <summary>
            /// The tangent vector (packed). Bitangent sign in component A.
            /// </summary>
            public FloatR10G10B10A2 Tangent;

            /// <summary>
            /// The blend indices (packed). Up to 4 bones.
            /// </summary>
            public Color32 BlendIndices;

            /// <summary>
            /// The blend weights (normalized, packed). Up to 4 bones.
            /// </summary>
            public Half4 BlendWeights;
        }

        /// <summary>
        /// The raw Vertex Buffer structure format.
        /// </summary>
        public struct Vertex
        {
            /// <summary>
            /// The vertex position.
            /// </summary>
            public Vector3 Position;

            /// <summary>
            /// The texture coordinates.
            /// </summary>
            public Vector2 TexCoord;

            /// <summary>
            /// The normal vector.
            /// </summary>
            public Vector3 Normal;

            /// <summary>
            /// The tangent vector.
            /// </summary>
            public Vector3 Tangent;

            /// <summary>
            /// The tangent vector.
            /// </summary>
            public Vector3 Bitangent;

            /// <summary>
            /// The blend indices. Up to 4 bones.
            /// </summary>
            public Int4 BlendIndices;

            /// <summary>
            /// The blend weights (normalized). Up to 4 bones.
            /// </summary>
            public Vector4 BlendWeights;
        }

        /// <summary>
        /// Gets the parent model asset.
        /// </summary>
        public SkinnedModel ParentSkinnedModel => (SkinnedModel)Internal_GetParentModel(__unmanagedPtr);

        /// <summary>
        /// Gets the material slot used by this mesh during rendering.
        /// </summary>
        public MaterialSlot MaterialSlot => ParentSkinnedModel.MaterialSlots[MaterialSlotIndex];

        /// <summary>
        /// Gets a format of the mesh index buffer.
        /// </summary>
        public PixelFormat IndexBufferFormat => Use16BitIndexBuffer ? PixelFormat.R16_UInt : PixelFormat.R32_UInt;

        /// <summary>
        /// Updates the skinned model mesh vertex and index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// </summary>
        /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
        /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 32-bit stride buffer. Cannot be null.</param>
        /// <param name="blendIndices">The skinned mesh blend indices buffer. Contains indices of the skeleton bones (up to 4 bones per vertex) to use for vertex position blending. Cannot be null.</param>
        /// <param name="blendWeights">The skinned mesh blend weights buffer (normalized). Contains weights per blend bone (up to 4 bones per vertex) of the skeleton bones to mix for vertex position blending. Cannot be null.</param>
        /// <param name="normals">The normal vectors (per vertex).</param>
        /// <param name="tangents">The normal vectors (per vertex). Use null to compute them from normal vectors.</param>
        /// <param name="uv">The texture coordinates (per vertex).</param>
        public void UpdateMesh(Vector3[] vertices, int[] triangles, Int4[] blendIndices, Vector4[] blendWeights, Vector3[] normals = null, Vector3[] tangents = null, Vector2[] uv = null)
        {
            // Validate state and input
            if (!ParentSkinnedModel.IsVirtual)
                throw new InvalidOperationException("Only virtual skinned models can be updated at runtime.");
            if (vertices == null)
                throw new ArgumentNullException(nameof(vertices));
            if (triangles == null)
                throw new ArgumentNullException(nameof(triangles));
            if (triangles.Length == 0 || triangles.Length % 3 != 0)
                throw new ArgumentOutOfRangeException(nameof(triangles));
            if (normals != null && normals.Length != vertices.Length)
                throw new ArgumentOutOfRangeException(nameof(normals));
            if (tangents != null && tangents.Length != vertices.Length)
                throw new ArgumentOutOfRangeException(nameof(tangents));
            if (tangents != null && normals == null)
                throw new ArgumentException("If you specify tangents then you need to also provide normals for the mesh.");
            if (uv != null && uv.Length != vertices.Length)
                throw new ArgumentOutOfRangeException(nameof(uv));

            if (Internal_UpdateMeshInt(__unmanagedPtr, vertices, triangles, blendIndices, blendWeights, normals, tangents, uv))
                throw new FlaxException("Failed to update mesh data.");
        }

        /// <summary>
        /// Updates the skinned model mesh vertex and index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// </summary>
        /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
        /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 16-bit stride buffer. Cannot be null.</param>
        /// <param name="blendIndices">The skinned mesh blend indices buffer. Contains indices of the skeleton bones (up to 4 bones per vertex) to use for vertex position blending. Cannot be null.</param>
        /// <param name="blendWeights">The skinned mesh blend weights buffer (normalized). Contains weights per blend bone (up to 4 bones per vertex) of the skeleton bones to mix for vertex position blending. Cannot be null.</param>
        /// <param name="normals">The normal vectors (per vertex).</param>
        /// <param name="tangents">The tangent vectors (per vertex). Use null to compute them from normal vectors.</param>
        /// <param name="uv">The texture coordinates (per vertex).</param>
        public void UpdateMesh(Vector3[] vertices, ushort[] triangles, Int4[] blendIndices, Vector4[] blendWeights, Vector3[] normals = null, Vector3[] tangents = null, Vector2[] uv = null)
        {
            // Validate state and input
            if (!ParentSkinnedModel.IsVirtual)
                throw new InvalidOperationException("Only virtual skinned models can be updated at runtime.");
            if (vertices == null)
                throw new ArgumentNullException(nameof(vertices));
            if (triangles == null)
                throw new ArgumentNullException(nameof(triangles));
            if (triangles.Length == 0 || triangles.Length % 3 != 0)
                throw new ArgumentOutOfRangeException(nameof(triangles));
            if (normals != null && normals.Length != vertices.Length)
                throw new ArgumentOutOfRangeException(nameof(normals));
            if (tangents != null && tangents.Length != vertices.Length)
                throw new ArgumentOutOfRangeException(nameof(tangents));
            if (tangents != null && normals == null)
                throw new ArgumentException("If you specify tangents then you need to also provide normals for the mesh.");
            if (uv != null && uv.Length != vertices.Length)
                throw new ArgumentOutOfRangeException(nameof(uv));

            if (Internal_UpdateMeshUShort(__unmanagedPtr, vertices, triangles, blendIndices, blendWeights, normals, tangents, uv))
                throw new FlaxException("Failed to update mesh data.");
        }

        internal enum InternalBufferType
        {
            VB0 = 0,
            IB16 = 3,
            IB32 = 4,
        }

        /// <summary>
        /// Downloads the first vertex buffer that contains mesh vertices data. To download data from GPU set <paramref name="forceGpu"/> to true and call this method from the thread other than main thread (see <see cref="Platform.IsInMainThread"/>).
        /// </summary>
        /// <param name="forceGpu">If set to <c>true</c> the data will be downloaded from the GPU, otherwise it can be loaded from the drive (source asset file) or from memory (if cached). Downloading mesh from GPU requires this call to be made from the other thread than main thread. Virtual assets are always downloaded from GPU memory due to lack of dedicated storage container for the asset data.</param>
        /// <returns>The gathered data.</returns>
        public Vertex0[] DownloadVertexBuffer0(bool forceGpu = false)
        {
            var vertices = VertexCount;
            var result = new Vertex0[vertices];
            if (Internal_DownloadBuffer(__unmanagedPtr, forceGpu, result, (int)InternalBufferType.VB0))
                throw new FlaxException("Failed to download mesh data.");
            return result;
        }

        /// <summary>
        /// Downloads the raw vertex buffer that contains mesh vertices data. To download data from GPU set <paramref name="forceGpu"/> to true and call this method from the thread other than main thread (see <see cref="Platform.IsInMainThread"/>).
        /// </summary>
        /// <param name="forceGpu">If set to <c>true</c> the data will be downloaded from the GPU, otherwise it can be loaded from the drive (source asset file) or from memory (if cached). Downloading mesh from GPU requires this call to be made from the other thread than main thread. Virtual assets are always downloaded from GPU memory due to lack of dedicated storage container for the asset data.</param>
        /// <returns>The gathered data.</returns>
        public Vertex[] DownloadVertexBuffer(bool forceGpu = false)
        {
            // TODO: perform data conversion on C++ side to make it faster

            var vb0 = DownloadVertexBuffer0(forceGpu);

            var vertices = VertexCount;
            var result = new Vertex[vertices];
            for (int i = 0; i < vertices; i++)
            {
                ref var v0 = ref vb0[i];
                float bitangentSign = v0.Tangent.A > Mathf.Epsilon ? -1.0f : +1.0f;

                result[i].Position = v0.Position;
                result[i].TexCoord = (Vector2)v0.TexCoord;
                result[i].Normal = v0.Normal.ToVector3() * 2.0f - 1.0f;
                result[i].Tangent = v0.Tangent.ToVector3() * 2.0f - 1.0f;
                result[i].Bitangent = Vector3.Cross(result[i].Normal, result[i].Tangent) * bitangentSign;
                result[i].BlendIndices = new Int4(v0.BlendIndices.R, v0.BlendIndices.G, v0.BlendIndices.B, v0.BlendIndices.A);
                result[i].BlendWeights = (Vector4)v0.BlendWeights;
            }

            return result;
        }

        /// <summary>
        /// Downloads the index buffer that contains mesh triangles data. To download data from GPU set <paramref name="forceGpu"/> to true and call this method from the thread other than main thread (see <see cref="Platform.IsInMainThread"/>).
        /// </summary>
        /// <remarks>If mesh index buffer format (see <see cref="IndexBufferFormat"/>) is <see cref="PixelFormat.R16_UInt"/> then it's faster to call .</remarks>
        /// <param name="forceGpu">If set to <c>true</c> the data will be downloaded from the GPU, otherwise it can be loaded from the drive (source asset file) or from memory (if cached). Downloading mesh from GPU requires this call to be made from the other thread than main thread. Virtual assets are always downloaded from GPU memory due to lack of dedicated storage container for the asset data.</param>
        /// <returns>The gathered data.</returns>
        public int[] DownloadIndexBuffer(bool forceGpu = false)
        {
            var triangles = TriangleCount;
            var result = new int[triangles * 3];
            if (Internal_DownloadBuffer(__unmanagedPtr, forceGpu, result, (int)InternalBufferType.IB32))
                throw new FlaxException("Failed to download mesh data.");
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
            var triangles = TriangleCount;
            var result = new ushort[triangles * 3];
            if (Internal_DownloadBuffer(__unmanagedPtr, forceGpu, result, (int)InternalBufferType.IB16))
                throw new FlaxException("Failed to download mesh data.");
            return result;
        }
    }
}
