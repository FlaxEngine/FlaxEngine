// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    partial class SkinnedMesh
    {
        /// <summary>
        /// The Vertex Buffer 0 structure format.
        /// [Deprecated in v1.10]
        /// </summary>
        [Obsolete("Use new MeshAccessor and depend on GPUVertexLayout when accessing mesh data.")]
        public struct Vertex0
        {
            /// <summary>
            /// The vertex position.
            /// </summary>
            public Float3 Position;

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
        /// [Deprecated in v1.10]
        /// </summary>
        [Obsolete("Use new MeshAccessor and depend on GPUVertexLayout when accessing mesh data.")]
        public struct Vertex
        {
            /// <summary>
            /// The vertex position.
            /// </summary>
            public Float3 Position;

            /// <summary>
            /// The texture coordinates.
            /// </summary>
            public Float2 TexCoord;

            /// <summary>
            /// The normal vector.
            /// </summary>
            public Float3 Normal;

            /// <summary>
            /// The tangent vector.
            /// </summary>
            public Float3 Tangent;

            /// <summary>
            /// The tangent vector.
            /// </summary>
            public Float3 Bitangent;

            /// <summary>
            /// The blend indices. Up to 4 bones.
            /// </summary>
            public Int4 BlendIndices;

            /// <summary>
            /// The blend weights (normalized). Up to 4 bones.
            /// </summary>
            public Float4 BlendWeights;
        }

        /// <summary>
        /// Gets the parent model asset.
        /// </summary>
        public SkinnedModel ParentSkinnedModel => (SkinnedModel)Internal_GetParentModel(__unmanagedPtr);

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
        /// <param name="colors">The vertex colors (per vertex).</param>
        public void UpdateMesh(Float3[] vertices, int[] triangles, Int4[] blendIndices, Float4[] blendWeights, Float3[] normals = null, Float3[] tangents = null, Float2[] uv = null, Color32[] colors = null)
        {
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
            if (colors != null && colors.Length != vertices.Length)
                throw new ArgumentOutOfRangeException(nameof(colors));

            if (Internal_UpdateMeshUInt(__unmanagedPtr, vertices.Length, triangles.Length / 3, vertices, triangles, blendIndices, blendWeights, normals, tangents, uv, colors))
                throw new Exception("Failed to update mesh data.");
        }

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
        /// <param name="colors">The vertex colors (per vertex).</param>
        public void UpdateMesh(Float3[] vertices, uint[] triangles, Int4[] blendIndices, Float4[] blendWeights, Float3[] normals = null, Float3[] tangents = null, Float2[] uv = null, Color32[] colors = null)
        {
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
            if (colors != null && colors.Length != vertices.Length)
                throw new ArgumentOutOfRangeException(nameof(colors));

            if (Internal_UpdateMeshUInt(__unmanagedPtr, vertices.Length, triangles.Length / 3, vertices, triangles, blendIndices, blendWeights, normals, tangents, uv, colors))
                throw new Exception("Failed to update mesh data.");
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
        /// <param name="colors">The vertex colors (per vertex).</param>
        public void UpdateMesh(Float3[] vertices, ushort[] triangles, Int4[] blendIndices, Float4[] blendWeights, Float3[] normals = null, Float3[] tangents = null, Float2[] uv = null, Color32[] colors = null)
        {
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
            if (colors != null && colors.Length != vertices.Length)
                throw new ArgumentOutOfRangeException(nameof(colors));

            if (Internal_UpdateMeshUShort(__unmanagedPtr, vertices.Length, triangles.Length / 3, vertices, triangles, blendIndices, blendWeights, normals, tangents, uv, colors))
                throw new Exception("Failed to update mesh data.");
        }

        /// <summary>
        /// Updates the skinned model mesh vertex and index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// [Deprecated on 26.05.2022, expires on 26.05.2024]
        /// </summary>
        /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
        /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 32-bit stride buffer. Cannot be null.</param>
        /// <param name="blendIndices">The skinned mesh blend indices buffer. Contains indices of the skeleton bones (up to 4 bones per vertex) to use for vertex position blending. Cannot be null.</param>
        /// <param name="blendWeights">The skinned mesh blend weights buffer (normalized). Contains weights per blend bone (up to 4 bones per vertex) of the skeleton bones to mix for vertex position blending. Cannot be null.</param>
        /// <param name="normals">The normal vectors (per vertex).</param>
        /// <param name="tangents">The normal vectors (per vertex). Use null to compute them from normal vectors.</param>
        /// <param name="uv">The texture coordinates (per vertex).</param>
        [Obsolete("Use UpdateMesh with Float3 and Float2 parameters instead")]
        public void UpdateMesh(Vector3[] vertices, int[] triangles, Int4[] blendIndices, Vector4[] blendWeights, Vector3[] normals = null, Vector3[] tangents = null, Vector2[] uv = null)
        {
            UpdateMesh(Utils.ConvertCollection(vertices), triangles, blendIndices, Utils.ConvertCollection(blendWeights), Utils.ConvertCollection(normals), Utils.ConvertCollection(tangents), Utils.ConvertCollection(uv));
        }

        /// <summary>
        /// Updates the skinned model mesh vertex and index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// [Deprecated on 26.05.2022, expires on 26.05.2024]
        /// </summary>
        /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
        /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 32-bit stride buffer. Cannot be null.</param>
        /// <param name="blendIndices">The skinned mesh blend indices buffer. Contains indices of the skeleton bones (up to 4 bones per vertex) to use for vertex position blending. Cannot be null.</param>
        /// <param name="blendWeights">The skinned mesh blend weights buffer (normalized). Contains weights per blend bone (up to 4 bones per vertex) of the skeleton bones to mix for vertex position blending. Cannot be null.</param>
        /// <param name="normals">The normal vectors (per vertex).</param>
        /// <param name="tangents">The normal vectors (per vertex). Use null to compute them from normal vectors.</param>
        /// <param name="uv">The texture coordinates (per vertex).</param>
        [Obsolete("Use UpdateMesh with Float3 and Float2 parameters instead")]
        public void UpdateMesh(Vector3[] vertices, uint[] triangles, Int4[] blendIndices, Vector4[] blendWeights, Vector3[] normals = null, Vector3[] tangents = null, Vector2[] uv = null)
        {
            UpdateMesh(Utils.ConvertCollection(vertices), triangles, blendIndices, Utils.ConvertCollection(blendWeights), Utils.ConvertCollection(normals), Utils.ConvertCollection(tangents), Utils.ConvertCollection(uv));
        }

        /// <summary>
        /// Updates the skinned model mesh vertex and index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// [Deprecated on 26.05.2022, expires on 26.05.2024]
        /// </summary>
        /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
        /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 16-bit stride buffer. Cannot be null.</param>
        /// <param name="blendIndices">The skinned mesh blend indices buffer. Contains indices of the skeleton bones (up to 4 bones per vertex) to use for vertex position blending. Cannot be null.</param>
        /// <param name="blendWeights">The skinned mesh blend weights buffer (normalized). Contains weights per blend bone (up to 4 bones per vertex) of the skeleton bones to mix for vertex position blending. Cannot be null.</param>
        /// <param name="normals">The normal vectors (per vertex).</param>
        /// <param name="tangents">The tangent vectors (per vertex). Use null to compute them from normal vectors.</param>
        /// <param name="uv">The texture coordinates (per vertex).</param>
        [Obsolete("Use UpdateMesh with Float3 and Float2 parameters instead")]
        public void UpdateMesh(Vector3[] vertices, ushort[] triangles, Int4[] blendIndices, Vector4[] blendWeights, Vector3[] normals = null, Vector3[] tangents = null, Vector2[] uv = null)
        {
            UpdateMesh(Utils.ConvertCollection(vertices), triangles, blendIndices, Utils.ConvertCollection(blendWeights), Utils.ConvertCollection(normals), Utils.ConvertCollection(tangents), Utils.ConvertCollection(uv));
        }

        /// <summary>
        /// [Deprecated in v1.10]
        /// </summary>
        [Obsolete("Use new MeshAccessor and depend on GPUVertexLayout when accessing mesh data.")]
        internal enum InternalBufferType
        {
            VB0 = 0,
        }

        /// <summary>
        /// Downloads the first vertex buffer that contains mesh vertices data. To download data from GPU set <paramref name="forceGpu"/> to true and call this method from the thread other than main thread (see <see cref="Platform.IsInMainThread"/>).
        /// [Deprecated in v1.10]
        /// </summary>
        /// <param name="forceGpu">If set to <c>true</c> the data will be downloaded from the GPU, otherwise it can be loaded from the drive (source asset file) or from memory (if cached). Downloading mesh from GPU requires this call to be made from the other thread than main thread. Virtual assets are always downloaded from GPU memory due to lack of dedicated storage container for the asset data.</param>
        /// <returns>The gathered data.</returns>
        [Obsolete("Use new MeshAccessor and depend on GPUVertexLayout when accessing mesh data.")]
        public Vertex0[] DownloadVertexBuffer0(bool forceGpu = false)
        {
            var result = (Vertex0[])Internal_DownloadBuffer(__unmanagedPtr, forceGpu, typeof(Vertex0), (int)InternalBufferType.VB0);
            if (result == null)
                throw new Exception("Failed to download mesh data.");
            return result;
        }

        /// <summary>
        /// Downloads the raw vertex buffer that contains mesh vertices data. To download data from GPU set <paramref name="forceGpu"/> to true and call this method from the thread other than main thread (see <see cref="Platform.IsInMainThread"/>).
        /// [Deprecated in v1.10]
        /// </summary>
        /// <param name="forceGpu">If set to <c>true</c> the data will be downloaded from the GPU, otherwise it can be loaded from the drive (source asset file) or from memory (if cached). Downloading mesh from GPU requires this call to be made from the other thread than main thread. Virtual assets are always downloaded from GPU memory due to lack of dedicated storage container for the asset data.</param>
        /// <returns>The gathered data.</returns>
        [Obsolete("Use new MeshAccessor and depend on GPUVertexLayout when accessing mesh data.")]
        public Vertex[] DownloadVertexBuffer(bool forceGpu = false)
        {
            var vb0 = DownloadVertexBuffer0(forceGpu);

            var vertices = vb0.Length;
            var result = new Vertex[vertices];
            for (int i = 0; i < vertices; i++)
            {
                ref var v0 = ref vb0[i];
                float bitangentSign = v0.Tangent.A > Mathf.Epsilon ? -1.0f : +1.0f;

                result[i].Position = v0.Position;
                result[i].TexCoord = (Float2)v0.TexCoord;
                result[i].Normal = v0.Normal.ToFloat3() * 2.0f - 1.0f;
                result[i].Tangent = v0.Tangent.ToFloat3() * 2.0f - 1.0f;
                result[i].Bitangent = Float3.Cross(result[i].Normal, result[i].Tangent) * bitangentSign;
                result[i].BlendIndices = new Int4(v0.BlendIndices.R, v0.BlendIndices.G, v0.BlendIndices.B, v0.BlendIndices.A);
                result[i].BlendWeights = (Float4)v0.BlendWeights;
            }

            return result;
        }
    }
}
