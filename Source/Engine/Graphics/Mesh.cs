// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;

namespace FlaxEngine
{
    partial class Mesh
    {
        /// <summary>
        /// The Vertex Buffer 0 structure format.
        /// </summary>
        public struct Vertex0
        {
            /// <summary>
            /// The vertex position.
            /// </summary>
            public Float3 Position;
        }

        /// <summary>
        /// The Vertex Buffer 1 structure format.
        /// </summary>
        public struct Vertex1
        {
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
            /// The lightmap UVs (packed).
            /// </summary>
            public Half2 LightmapUVs;
        }

        /// <summary>
        /// The Vertex Buffer 2 structure format.
        /// </summary>
        public struct Vertex2
        {
            /// <summary>
            /// The vertex color.
            /// </summary>
            public Color32 Color;
        }

        /// <summary>
        /// The raw Vertex Buffer structure format.
        /// </summary>
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
            /// The lightmap UVs.
            /// </summary>
            public Float2 LightmapUVs;

            /// <summary>
            /// The vertex color.
            /// </summary>
            public Color Color;
        }

        /// <summary>
        /// Gets the parent model asset.
        /// </summary>
        public Model ParentModel => (Model)Internal_GetParentModel(__unmanagedPtr);

        /// <summary>
        /// Gets the material slot used by this mesh during rendering.
        /// </summary>
        public MaterialSlot MaterialSlot => ParentModel.MaterialSlots[MaterialSlotIndex];

        /// <summary>
        /// Gets a format of the mesh index buffer.
        /// </summary>
        public PixelFormat IndexBufferFormat => Use16BitIndexBuffer ? PixelFormat.R16_UInt : PixelFormat.R32_UInt;

        /// <summary>
        /// Updates the model mesh vertex and index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// </summary>
        /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
        /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 32-bit stride buffer. Cannot be null.</param>
        /// <param name="normals">The normal vectors (per vertex).</param>
        /// <param name="tangents">The normal vectors (per vertex). Use null to compute them from normal vectors.</param>
        /// <param name="uv">The texture coordinates (per vertex).</param>
        /// <param name="colors">The vertex colors (per vertex).</param>
        public void UpdateMesh(Float3[] vertices, int[] triangles, Float3[] normals = null, Float3[] tangents = null, Float2[] uv = null, Color32[] colors = null)
        {
            if (!ParentModel.IsVirtual)
                throw new InvalidOperationException("Only virtual models can be updated at runtime.");
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

            if (Internal_UpdateMeshUInt(__unmanagedPtr, vertices.Length, triangles.Length / 3, vertices, triangles, normals, tangents, uv, colors))
                throw new Exception("Failed to update mesh data.");
        }

        /// <summary>
        /// Updates the model mesh vertex and index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// </summary>
        /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
        /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 32-bit stride buffer. Cannot be null.</param>
        /// <param name="normals">The normal vectors (per vertex).</param>
        /// <param name="tangents">The normal vectors (per vertex). Use null to compute them from normal vectors.</param>
        /// <param name="uv">The texture coordinates (per vertex).</param>
        /// <param name="colors">The vertex colors (per vertex).</param>
        public void UpdateMesh(List<Float3> vertices, List<int> triangles, List<Float3> normals = null, List<Float3> tangents = null, List<Float2> uv = null, List<Color32> colors = null)
        {
            if (!ParentModel.IsVirtual)
                throw new InvalidOperationException("Only virtual models can be updated at runtime.");
            if (vertices == null)
                throw new ArgumentNullException(nameof(vertices));
            if (triangles == null)
                throw new ArgumentNullException(nameof(triangles));
            if (triangles.Count == 0 || triangles.Count % 3 != 0)
                throw new ArgumentOutOfRangeException(nameof(triangles));
            if (normals != null && normals.Count != vertices.Count)
                throw new ArgumentOutOfRangeException(nameof(normals));
            if (tangents != null && tangents.Count != vertices.Count)
                throw new ArgumentOutOfRangeException(nameof(tangents));
            if (tangents != null && normals == null)
                throw new ArgumentException("If you specify tangents then you need to also provide normals for the mesh.");
            if (uv != null && uv.Count != vertices.Count)
                throw new ArgumentOutOfRangeException(nameof(uv));
            if (colors != null && colors.Count != vertices.Count)
                throw new ArgumentOutOfRangeException(nameof(colors));

            if (Internal_UpdateMeshUInt(__unmanagedPtr, vertices.Count, triangles.Count / 3, Utils.ExtractArrayFromList(vertices), Utils.ExtractArrayFromList(triangles), Utils.ExtractArrayFromList(normals), Utils.ExtractArrayFromList(tangents), Utils.ExtractArrayFromList(uv), Utils.ExtractArrayFromList(colors)))
                throw new Exception("Failed to update mesh data.");
        }

        /// <summary>
        /// Updates the model mesh vertex and index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// </summary>
        /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
        /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 32-bit stride buffer. Cannot be null.</param>
        /// <param name="normals">The normal vectors (per vertex).</param>
        /// <param name="tangents">The normal vectors (per vertex). Use null to compute them from normal vectors.</param>
        /// <param name="uv">The texture coordinates (per vertex).</param>
        /// <param name="colors">The vertex colors (per vertex).</param>
        public void UpdateMesh(Float3[] vertices, uint[] triangles, Float3[] normals = null, Float3[] tangents = null, Float2[] uv = null, Color32[] colors = null)
        {
            if (!ParentModel.IsVirtual)
                throw new InvalidOperationException("Only virtual models can be updated at runtime.");
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

            if (Internal_UpdateMeshUInt(__unmanagedPtr, vertices.Length, triangles.Length / 3, vertices, triangles, normals, tangents, uv, colors))
                throw new Exception("Failed to update mesh data.");
        }

        /// <summary>
        /// Updates the model mesh vertex and index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// </summary>
        /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
        /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 32-bit stride buffer. Cannot be null.</param>
        /// <param name="normals">The normal vectors (per vertex).</param>
        /// <param name="tangents">The normal vectors (per vertex). Use null to compute them from normal vectors.</param>
        /// <param name="uv">The texture coordinates (per vertex).</param>
        /// <param name="colors">The vertex colors (per vertex).</param>
        public void UpdateMesh(List<Float3> vertices, List<uint> triangles, List<Float3> normals = null, List<Float3> tangents = null, List<Float2> uv = null, List<Color32> colors = null)
        {
            if (!ParentModel.IsVirtual)
                throw new InvalidOperationException("Only virtual models can be updated at runtime.");
            if (vertices == null)
                throw new ArgumentNullException(nameof(vertices));
            if (triangles == null)
                throw new ArgumentNullException(nameof(triangles));
            if (triangles.Count == 0 || triangles.Count % 3 != 0)
                throw new ArgumentOutOfRangeException(nameof(triangles));
            if (normals != null && normals.Count != vertices.Count)
                throw new ArgumentOutOfRangeException(nameof(normals));
            if (tangents != null && tangents.Count != vertices.Count)
                throw new ArgumentOutOfRangeException(nameof(tangents));
            if (tangents != null && normals == null)
                throw new ArgumentException("If you specify tangents then you need to also provide normals for the mesh.");
            if (uv != null && uv.Count != vertices.Count)
                throw new ArgumentOutOfRangeException(nameof(uv));
            if (colors != null && colors.Count != vertices.Count)
                throw new ArgumentOutOfRangeException(nameof(colors));

            if (Internal_UpdateMeshUInt(__unmanagedPtr, vertices.Count, triangles.Count / 3, Utils.ExtractArrayFromList(vertices), Utils.ExtractArrayFromList(triangles), Utils.ExtractArrayFromList(normals), Utils.ExtractArrayFromList(tangents), Utils.ExtractArrayFromList(uv), Utils.ExtractArrayFromList(colors)))
                throw new Exception("Failed to update mesh data.");
        }

        /// <summary>
        /// Updates the model mesh vertex and index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// </summary>
        /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
        /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 16-bit stride buffer. Cannot be null.</param>
        /// <param name="normals">The normal vectors (per vertex).</param>
        /// <param name="tangents">The tangent vectors (per vertex). Use null to compute them from normal vectors.</param>
        /// <param name="uv">The texture coordinates (per vertex).</param>
        /// <param name="colors">The vertex colors (per vertex).</param>
        public void UpdateMesh(Float3[] vertices, ushort[] triangles, Float3[] normals = null, Float3[] tangents = null, Float2[] uv = null, Color32[] colors = null)
        {
            if (!ParentModel.IsVirtual)
                throw new InvalidOperationException("Only virtual models can be updated at runtime.");
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

            if (Internal_UpdateMeshUShort(__unmanagedPtr, vertices.Length, triangles.Length / 3, vertices, triangles, normals, tangents, uv, colors))
                throw new Exception("Failed to update mesh data.");
        }

        /// <summary>
        /// Updates the model mesh vertex and index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// </summary>
        /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
        /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 16-bit stride buffer. Cannot be null.</param>
        /// <param name="normals">The normal vectors (per vertex).</param>
        /// <param name="tangents">The tangent vectors (per vertex). Use null to compute them from normal vectors.</param>
        /// <param name="uv">The texture coordinates (per vertex).</param>
        /// <param name="colors">The vertex colors (per vertex).</param>
        public void UpdateMesh(List<Float3> vertices, List<ushort> triangles, List<Float3> normals = null, List<Float3> tangents = null, List<Float2> uv = null, List<Color32> colors = null)
        {
            if (!ParentModel.IsVirtual)
                throw new InvalidOperationException("Only virtual models can be updated at runtime.");
            if (vertices == null)
                throw new ArgumentNullException(nameof(vertices));
            if (triangles == null)
                throw new ArgumentNullException(nameof(triangles));
            if (triangles.Count == 0 || triangles.Count % 3 != 0)
                throw new ArgumentOutOfRangeException(nameof(triangles));
            if (normals != null && normals.Count != vertices.Count)
                throw new ArgumentOutOfRangeException(nameof(normals));
            if (tangents != null && tangents.Count != vertices.Count)
                throw new ArgumentOutOfRangeException(nameof(tangents));
            if (tangents != null && normals == null)
                throw new ArgumentException("If you specify tangents then you need to also provide normals for the mesh.");
            if (uv != null && uv.Count != vertices.Count)
                throw new ArgumentOutOfRangeException(nameof(uv));
            if (colors != null && colors.Count != vertices.Count)
                throw new ArgumentOutOfRangeException(nameof(colors));

            if (Internal_UpdateMeshUShort(__unmanagedPtr, vertices.Count, triangles.Count / 3, Utils.ExtractArrayFromList(vertices), Utils.ExtractArrayFromList(triangles), Utils.ExtractArrayFromList(normals), Utils.ExtractArrayFromList(tangents), Utils.ExtractArrayFromList(uv), Utils.ExtractArrayFromList(colors)))
                throw new Exception("Failed to update mesh data.");
        }

        /// <summary>
        /// Updates the model mesh vertex and index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// [Deprecated on 26.05.2022, expires on 26.05.2024]
        /// </summary>
        /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
        /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 32-bit stride buffer. Cannot be null.</param>
        /// <param name="normals">The normal vectors (per vertex).</param>
        /// <param name="tangents">The normal vectors (per vertex). Use null to compute them from normal vectors.</param>
        /// <param name="uv">The texture coordinates (per vertex).</param>
        /// <param name="colors">The vertex colors (per vertex).</param>
        [Obsolete("Deprecated in 1.4, use overload with Float3 and Float2 parameters")]
        public void UpdateMesh(Vector3[] vertices, int[] triangles, Vector3[] normals = null, Vector3[] tangents = null, Vector2[] uv = null, Color32[] colors = null)
        {
            UpdateMesh(Utils.ConvertCollection(vertices), triangles, Utils.ConvertCollection(normals), Utils.ConvertCollection(tangents), Utils.ConvertCollection(uv), colors);
        }

        /// <summary>
        /// Updates the model mesh vertex and index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// [Deprecated on 26.05.2022, expires on 26.05.2024]
        /// </summary>
        /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
        /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 32-bit stride buffer. Cannot be null.</param>
        /// <param name="normals">The normal vectors (per vertex).</param>
        /// <param name="tangents">The normal vectors (per vertex). Use null to compute them from normal vectors.</param>
        /// <param name="uv">The texture coordinates (per vertex).</param>
        /// <param name="colors">The vertex colors (per vertex).</param>
        [Obsolete("Deprecated in 1.4, use overload with Float3 and Float2 parameters")]
        public void UpdateMesh(List<Vector3> vertices, List<int> triangles, List<Vector3> normals = null, List<Vector3> tangents = null, List<Vector2> uv = null, List<Color32> colors = null)
        {
            UpdateMesh(Utils.ConvertCollection(vertices), triangles, Utils.ConvertCollection(normals), Utils.ConvertCollection(tangents), Utils.ConvertCollection(uv), colors);
        }

        /// <summary>
        /// Updates the model mesh vertex and index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// [Deprecated on 26.05.2022, expires on 26.05.2024]
        /// </summary>
        /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
        /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 32-bit stride buffer. Cannot be null.</param>
        /// <param name="normals">The normal vectors (per vertex).</param>
        /// <param name="tangents">The normal vectors (per vertex). Use null to compute them from normal vectors.</param>
        /// <param name="uv">The texture coordinates (per vertex).</param>
        /// <param name="colors">The vertex colors (per vertex).</param>
        [Obsolete("Deprecated in 1.4, use overload with Float3 and Float2 parameters")]
        public void UpdateMesh(Vector3[] vertices, uint[] triangles, Vector3[] normals = null, Vector3[] tangents = null, Vector2[] uv = null, Color32[] colors = null)
        {
            UpdateMesh(Utils.ConvertCollection(vertices), triangles, Utils.ConvertCollection(normals), Utils.ConvertCollection(tangents), Utils.ConvertCollection(uv), colors);
        }

        /// <summary>
        /// Updates the model mesh vertex and index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// [Deprecated on 26.05.2022, expires on 26.05.2024]
        /// </summary>
        /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
        /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 32-bit stride buffer. Cannot be null.</param>
        /// <param name="normals">The normal vectors (per vertex).</param>
        /// <param name="tangents">The normal vectors (per vertex). Use null to compute them from normal vectors.</param>
        /// <param name="uv">The texture coordinates (per vertex).</param>
        /// <param name="colors">The vertex colors (per vertex).</param>
        [Obsolete("Deprecated in 1.4, use overload with Float3 and Float2 parameters")]
        public void UpdateMesh(List<Vector3> vertices, List<uint> triangles, List<Vector3> normals = null, List<Vector3> tangents = null, List<Vector2> uv = null, List<Color32> colors = null)
        {
            UpdateMesh(Utils.ConvertCollection(vertices), triangles, Utils.ConvertCollection(normals), Utils.ConvertCollection(tangents), Utils.ConvertCollection(uv), colors);
        }

        /// <summary>
        /// Updates the model mesh vertex and index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// [Deprecated on 26.05.2022, expires on 26.05.2024]
        /// </summary>
        /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
        /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 16-bit stride buffer. Cannot be null.</param>
        /// <param name="normals">The normal vectors (per vertex).</param>
        /// <param name="tangents">The tangent vectors (per vertex). Use null to compute them from normal vectors.</param>
        /// <param name="uv">The texture coordinates (per vertex).</param>
        /// <param name="colors">The vertex colors (per vertex).</param>
        [Obsolete("Deprecated in 1.4, use overload with Float3 and Float2 parameters")]
        public void UpdateMesh(Vector3[] vertices, ushort[] triangles, Vector3[] normals = null, Vector3[] tangents = null, Vector2[] uv = null, Color32[] colors = null)
        {
            UpdateMesh(Utils.ConvertCollection(vertices), triangles, Utils.ConvertCollection(normals), Utils.ConvertCollection(tangents), Utils.ConvertCollection(uv), colors);
        }

        /// <summary>
        /// Updates the model mesh vertex and index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// [Deprecated on 26.05.2022, expires on 26.05.2024]
        /// </summary>
        /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
        /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 16-bit stride buffer. Cannot be null.</param>
        /// <param name="normals">The normal vectors (per vertex).</param>
        /// <param name="tangents">The tangent vectors (per vertex). Use null to compute them from normal vectors.</param>
        /// <param name="uv">The texture coordinates (per vertex).</param>
        /// <param name="colors">The vertex colors (per vertex).</param>
        [Obsolete("Deprecated in 1.4, use overload with Float3 and Float2 parameters")]
        public void UpdateMesh(List<Vector3> vertices, List<ushort> triangles, List<Vector3> normals = null, List<Vector3> tangents = null, List<Vector2> uv = null, List<Color32> colors = null)
        {
            UpdateMesh(Utils.ConvertCollection(vertices), triangles, Utils.ConvertCollection(normals), Utils.ConvertCollection(tangents), Utils.ConvertCollection(uv), colors);
        }

        /// <summary>
        /// Updates the model mesh index buffer data.
        /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
        /// Mesh data will be cached and uploaded to the GPU with a delay.
        /// </summary>
        /// <param name="triangles">The mesh index buffer (triangles). Uses 32-bit stride buffer. Cannot be null.</param>
        public void UpdateTriangles(int[] triangles)
        {
            if (!ParentModel.IsVirtual)
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
            if (!ParentModel.IsVirtual)
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
            if (!ParentModel.IsVirtual)
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
            if (!ParentModel.IsVirtual)
                throw new InvalidOperationException("Only virtual models can be updated at runtime.");
            if (triangles == null)
                throw new ArgumentNullException(nameof(triangles));
            if (triangles.Count == 0 || triangles.Count % 3 != 0)
                throw new ArgumentOutOfRangeException(nameof(triangles));

            if (Internal_UpdateTrianglesUShort(__unmanagedPtr, triangles.Count / 3, Utils.ExtractArrayFromList(triangles)))
                throw new Exception("Failed to update mesh data.");
        }

        internal enum InternalBufferType
        {
            VB0 = 0,
            VB1 = 1,
            VB2 = 2,
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
            var result = (Vertex0[])Internal_DownloadBuffer(__unmanagedPtr, forceGpu, typeof(Vertex0), (int)InternalBufferType.VB0);
            if (result == null)
                throw new Exception("Failed to download mesh data.");
            return result;
        }

        /// <summary>
        /// Downloads the second vertex buffer that contains mesh vertices data. To download data from GPU set <paramref name="forceGpu"/> to true and call this method from the thread other than main thread (see <see cref="Platform.IsInMainThread"/>).
        /// </summary>
        /// <param name="forceGpu">If set to <c>true</c> the data will be downloaded from the GPU, otherwise it can be loaded from the drive (source asset file) or from memory (if cached). Downloading mesh from GPU requires this call to be made from the other thread than main thread. Virtual assets are always downloaded from GPU memory due to lack of dedicated storage container for the asset data.</param>
        /// <returns>The gathered data.</returns>
        public Vertex1[] DownloadVertexBuffer1(bool forceGpu = false)
        {
            var result = (Vertex1[])Internal_DownloadBuffer(__unmanagedPtr, forceGpu, typeof(Vertex1), (int)InternalBufferType.VB1);
            if (result == null)
                throw new Exception("Failed to download mesh data.");
            return result;
        }

        /// <summary>
        /// Downloads the third vertex buffer that contains mesh vertices data. To download data from GPU set <paramref name="forceGpu"/> to true and call this method from the thread other than main thread (see <see cref="Platform.IsInMainThread"/>).
        /// </summary>
        /// <remarks>
        /// If mesh has no vertex colors (stored in vertex buffer 2) the returned value is null.
        /// </remarks>
        /// <param name="forceGpu">If set to <c>true</c> the data will be downloaded from the GPU, otherwise it can be loaded from the drive (source asset file) or from memory (if cached). Downloading mesh from GPU requires this call to be made from the other thread than main thread. Virtual assets are always downloaded from GPU memory due to lack of dedicated storage container for the asset data.</param>
        /// <returns>The gathered data or null if mesh has no vertex colors.</returns>
        public Vertex2[] DownloadVertexBuffer2(bool forceGpu = false)
        {
            if (!HasVertexColors)
                return null;
            var result = (Vertex2[])Internal_DownloadBuffer(__unmanagedPtr, forceGpu, typeof(Vertex2), (int)InternalBufferType.VB2);
            if (result == null)
                throw new Exception("Failed to download mesh data.");
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
            // TODO: implement batched data download (3 buffers at once) to reduce stall

            var vb0 = DownloadVertexBuffer0(forceGpu);
            var vb1 = DownloadVertexBuffer1(forceGpu);
            var vb2 = DownloadVertexBuffer2(forceGpu);

            var vertices = vb0.Length;
            var result = new Vertex[vertices];
            for (int i = 0; i < vertices; i++)
            {
                ref var v0 = ref vb0[i];
                ref var v1 = ref vb1[i];
                float bitangentSign = v1.Tangent.A > Mathf.Epsilon ? -1.0f : +1.0f;

                result[i].Position = v0.Position;
                result[i].TexCoord = (Float2)v1.TexCoord;
                result[i].Normal = v1.Normal.ToFloat3() * 2.0f - 1.0f;
                result[i].Tangent = v1.Tangent.ToFloat3() * 2.0f - 1.0f;
                result[i].Bitangent = Float3.Cross(result[i].Normal, result[i].Tangent) * bitangentSign;
                result[i].LightmapUVs = (Float2)v1.LightmapUVs;
                result[i].Color = Color.Black;
            }

            if (vb2 != null)
            {
                for (int i = 0; i < vertices; i++)
                {
                    result[i].Color = vb2[i].Color;
                }
            }

            return result;
        }

        /// <summary>
        /// Downloads the index buffer that contains mesh triangles data. To download data from GPU set <paramref name="forceGpu"/> to true and call this method from the thread other than main thread (see <see cref="Platform.IsInMainThread"/>).
        /// </summary>
        /// <remarks>If mesh index buffer format (see <see cref="IndexBufferFormat"/>) is <see cref="PixelFormat.R16_UInt"/> then it's faster to call .</remarks>
        /// <param name="forceGpu">If set to <c>true</c> the data will be downloaded from the GPU, otherwise it can be loaded from the drive (source asset file) or from memory (if cached). Downloading mesh from GPU requires this call to be made from the other thread than main thread. Virtual assets are always downloaded from GPU memory due to lack of dedicated storage container for the asset data.</param>
        /// <returns>The gathered data.</returns>
        public uint[] DownloadIndexBuffer(bool forceGpu = false)
        {
            var result = (uint[])Internal_DownloadBuffer(__unmanagedPtr, forceGpu, typeof(uint), (int)InternalBufferType.IB32);
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
            var result = (ushort[])Internal_DownloadBuffer(__unmanagedPtr, forceGpu, typeof(ushort), (int)InternalBufferType.IB16);
            if (result == null)
                throw new Exception("Failed to download mesh data.");
            return result;
        }
    }
}
