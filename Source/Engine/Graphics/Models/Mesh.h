// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "MeshBase.h"
#include "ModelInstanceEntry.h"
#include "Config.h"
#include "Types.h"

class Lightmap;

/// <summary>
/// Represents part of the model that is made of vertices and can be rendered using custom material and transformation.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API Mesh : public MeshBase
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(Mesh, MeshBase);

    /// <summary>
    /// Gets the model owning this mesh.
    /// </summary>
    FORCE_INLINE Model* GetModel() const
    {
        return (Model*)_model;
    }

    /// <summary>
    /// Determines whether this mesh contains valid lightmap texture coordinates data.
    /// </summary>
    API_PROPERTY() bool HasLightmapUVs() const
    {
        return LightmapUVsIndex != -1;
    }

    /// <summary>
    /// Lightmap texture coordinates channel index. Value -1 indicates that channel is not available.
    /// </summary>
    API_FIELD() int32 LightmapUVsIndex = -1;

public:
    /// <summary>
    /// Updates the model mesh (used by the virtual models created with Init rather than Load).
    /// [Deprecated in v1.10]
    /// </summary>
    /// <param name="vertexCount">The amount of vertices in the vertex buffer.</param>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="vb0">The first vertex buffer data.</param>
    /// <param name="vb1">The second vertex buffer data.</param>
    /// <param name="vb2">The third vertex buffer data.</param>
    /// <param name="ib">The index buffer in clockwise order.</param>
    /// <returns>True if failed, otherwise false.</returns>
    DEPRECATED("Use MeshAccessor or UpdateMesh with separate vertex attribute arrays instead.")
    bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0ElementType* vb0, const VB1ElementType* vb1, const VB2ElementType* vb2, const uint32* ib);

    /// <summary>
    /// Updates the model mesh (used by the virtual models created with Init rather than Load).
    /// [Deprecated in v1.10]
    /// </summary>
    /// <param name="vertexCount">The amount of vertices in the vertex buffer.</param>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="vb0">The first vertex buffer data.</param>
    /// <param name="vb1">The second vertex buffer data.</param>
    /// <param name="vb2">The third vertex buffer data.</param>
    /// <param name="ib">The index buffer in clockwise order.</param>
    /// <returns>True if failed, otherwise false.</returns>
    DEPRECATED("Use MeshAccessor or UpdateMesh with separate vertex attribute arrays instead.")
    bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0ElementType* vb0, const VB1ElementType* vb1, const VB2ElementType* vb2, const uint16* ib);

    /// <summary>
    /// Updates the model mesh (used by the virtual models created with Init rather than Load).
    /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
    /// Mesh data will be cached and uploaded to the GPU with a delay.
    /// [Deprecated in v1.10]
    /// </summary>
    /// <param name="vertexCount">The amount of vertices in the vertex buffer.</param>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="vb0">The first vertex buffer data.</param>
    /// <param name="vb1">The second vertex buffer data.</param>
    /// <param name="vb2">The third vertex buffer data.</param>
    /// <param name="ib">The index buffer in clockwise order.</param>
    /// <param name="use16BitIndices">True if index buffer uses 16-bit index buffer, otherwise 32-bit.</param>
    /// <returns>True if failed, otherwise false.</returns>
    DEPRECATED("Use MeshAccessor or UpdateMesh with separate vertex attribute arrays instead.")
    bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0ElementType* vb0, const VB1ElementType* vb1, const VB2ElementType* vb2, const void* ib, bool use16BitIndices);

    /// <summary>
    /// Updates the model mesh (used by the virtual models created with Init rather than Load).
    /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
    /// Mesh data will be cached and uploaded to the GPU with a delay.
    /// </summary>
    /// <param name="vertexCount">The amount of vertices in the vertex buffer.</param>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
    /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 32-bit stride buffer. Cannot be null.</param>
    /// <param name="normals">The normal vectors (per vertex).</param>
    /// <param name="tangents">The normal vectors (per vertex). Use null to compute them from normal vectors.</param>
    /// <param name="uvs">The texture coordinates (per vertex).</param>
    /// <param name="colors">The vertex colors (per vertex).</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, const Float3* vertices, const uint16* triangles, const Float3* normals = nullptr, const Float3* tangents = nullptr, const Float2* uvs = nullptr, const Color32* colors = nullptr);

    /// <summary>
    /// Updates the model mesh (used by the virtual models created with Init rather than Load).
    /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
    /// Mesh data will be cached and uploaded to the GPU with a delay.
    /// </summary>
    /// <param name="vertexCount">The amount of vertices in the vertex buffer.</param>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
    /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 32-bit stride buffer. Cannot be null.</param>
    /// <param name="normals">The normal vectors (per vertex).</param>
    /// <param name="tangents">The normal vectors (per vertex). Use null to compute them from normal vectors.</param>
    /// <param name="uvs">The texture coordinates (per vertex).</param>
    /// <param name="colors">The vertex colors (per vertex).</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, const Float3* vertices, const uint32* triangles, const Float3* normals = nullptr, const Float3* tangents = nullptr, const Float2* uvs = nullptr, const Color32* colors = nullptr);

public:
    /// <summary>
    /// Load mesh data and Initialize GPU buffers
    /// [Deprecated in v1.10]
    /// </summary>
    /// <param name="vertices">Amount of vertices in the vertex buffer</param>
    /// <param name="triangles">Amount of triangles in the index buffer</param>
    /// <param name="vb0">Vertex buffer 0 data</param>
    /// <param name="vb1">Vertex buffer 1 data</param>
    /// <param name="vb2">Vertex buffer 2 data (may be null if not used)</param>
    /// <param name="ib">Index buffer data</param>
    /// <param name="use16BitIndexBuffer">True if use 16 bit indices for the index buffer (true: uint16, false: uint32).</param>
    /// <returns>True if cannot load data, otherwise false.</returns>
    DEPRECATED("Use Init intead.") bool Load(uint32 vertices, uint32 triangles, const void* vb0, const void* vb1, const void* vb2, const void* ib, bool use16BitIndexBuffer);

public:
    /// <summary>
    /// Draws the mesh.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="material">The material to use for rendering.</param>
    /// <param name="world">The world transformation of the model.</param>
    /// <param name="flags">The object static flags.</param>
    /// <param name="receiveDecals">True if rendered geometry can receive decals, otherwise false.</param>
    /// <param name="drawModes">The draw passes to use for rendering this object.</param>
    /// <param name="perInstanceRandom">The random per-instance value (normalized to range 0-1).</param>
    /// <param name="sortOrder">Object sorting key.</param>
    API_FUNCTION() void Draw(API_PARAM(Ref) const RenderContext& renderContext, MaterialBase* material, API_PARAM(Ref) const Matrix& world, StaticFlags flags = StaticFlags::None, bool receiveDecals = true, DrawPass drawModes = DrawPass::Default, float perInstanceRandom = 0.0f, int8 sortOrder = 0) const;

    /// <summary>
    /// Draws the mesh.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="info">The packed drawing info data.</param>
    /// <param name="lodDitherFactor">The LOD transition dither factor.</param>
    void Draw(const RenderContext& renderContext, const DrawInfo& info, float lodDitherFactor) const;

    /// <summary>
    /// Draws the mesh.
    /// </summary>
    /// <param name="renderContextBatch">The rendering context batch.</param>
    /// <param name="info">The packed drawing info data.</param>
    /// <param name="lodDitherFactor">The LOD transition dither factor.</param>
    void Draw(const RenderContextBatch& renderContextBatch, const DrawInfo& info, float lodDitherFactor) const;

public:
    // [MeshBase]
    bool Init(uint32 vertices, uint32 triangles, const Array<const void*, FixedAllocation<3>>& vbData, const void* ibData, bool use16BitIndexBuffer, Array<GPUVertexLayout*, FixedAllocation<3>> vbLayout) override;
    void Release() override;

private:
    // Internal bindings
#if !COMPILE_WITHOUT_CSHARP
    API_FUNCTION(NoProxy) bool UpdateMeshUInt(int32 vertexCount, int32 triangleCount, const MArray* verticesObj, const MArray* trianglesObj, const MArray* normalsObj, const MArray* tangentsObj, const MArray* uvObj, const MArray* colorsObj);
    API_FUNCTION(NoProxy) bool UpdateMeshUShort(int32 vertexCount, int32 triangleCount, const MArray* verticesObj, const MArray* trianglesObj, const MArray* normalsObj, const MArray* tangentsObj, const MArray* uvObj, const MArray* colorsObj);
    API_FUNCTION(NoProxy) MArray* DownloadBuffer(bool forceGpu, MTypeObject* resultType, int32 typeI);
#if USE_EDITOR
    API_FUNCTION(NoProxy) Array<Vector3> GetCollisionProxyPoints() const;
#endif
#endif
};
