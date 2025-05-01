// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "MeshBase.h"
#include "BlendShape.h"

/// <summary>
/// Represents part of the skinned model that is made of vertices and can be rendered using custom material, transformation and skeleton bones hierarchy.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API SkinnedMesh : public MeshBase
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(SkinnedMesh, MeshBase);

    /// <summary>
    /// Gets the skinned model owning this mesh.
    /// </summary>
    FORCE_INLINE SkinnedModel* GetSkinnedModel() const
    {
        return (SkinnedModel*)_model;
    }

    /// <summary>
    /// Blend shapes used by this mesh.
    /// </summary>
    Array<BlendShape> BlendShapes;

public:
    /// <summary>
    /// Load mesh data and Initialize GPU buffers
    /// [Deprecated in v1.10]
    /// </summary>
    /// <param name="vertices">Amount of vertices in the vertex buffer</param>
    /// <param name="triangles">Amount of triangles in the index buffer</param>
    /// <param name="vb0">Vertex buffer data</param>
    /// <param name="ib">Index buffer data</param>
    /// <param name="use16BitIndexBuffer">True if use 16 bit indices for the index buffer (true: uint16, false: uint32).</param>
    /// <returns>True if cannot load data, otherwise false.</returns>
    DEPRECATED("Use Init intead.") bool Load(uint32 vertices, uint32 triangles, const void* vb0, const void* ib, bool use16BitIndexBuffer);

    /// <summary>
    /// Updates the model mesh (used by the virtual models created with Init rather than Load).
    /// [Deprecated in v1.10]
    /// </summary>
    /// <param name="vertexCount">The amount of vertices in the vertex buffer.</param>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="vb">The vertex buffer data.</param>
    /// <param name="ib">The index buffer in clockwise order.</param>
    /// <returns>True if failed, otherwise false.</returns>
    DEPRECATED("Use MeshAccessor or UpdateMesh with separate vertex attribute arrays instead.")
    bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0SkinnedElementType* vb, const int32* ib);

    /// <summary>
    /// Updates the model mesh (used by the virtual models created with Init rather than Load).
    /// [Deprecated in v1.10]
    /// </summary>
    /// <param name="vertexCount">The amount of vertices in the vertex buffer.</param>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="vb">The vertex buffer data.</param>
    /// <param name="ib">The index buffer in clockwise order.</param>
    /// <returns>True if failed, otherwise false.</returns>
    DEPRECATED("Use MeshAccessor or UpdateMesh with separate vertex attribute arrays instead.")
    bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0SkinnedElementType* vb, const uint32* ib);

    /// <summary>
    /// Updates the model mesh (used by the virtual models created with Init rather than Load).
    /// [Deprecated in v1.10]
    /// </summary>
    /// <param name="vertexCount">The amount of vertices in the vertex buffer.</param>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="vb">The vertex buffer data.</param>
    /// <param name="ib">The index buffer, clockwise order.</param>
    /// <returns>True if failed, otherwise false.</returns>
    DEPRECATED("Use MeshAccessor or UpdateMesh with separate vertex attribute arrays instead.")
    bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0SkinnedElementType* vb, const uint16* ib);

    /// <summary>
    /// Updates the model mesh (used by the virtual models created with Init rather than Load).
    /// [Deprecated in v1.10]
    /// </summary>
    /// <param name="vertexCount">The amount of vertices in the vertex buffer.</param>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="vb">The vertex buffer data.</param>
    /// <param name="ib">The index buffer in clockwise order.</param>
    /// <param name="use16BitIndices">True if index buffer uses 16-bit index buffer, otherwise 32-bit.</param>
    /// <returns>True if failed, otherwise false.</returns>
    DEPRECATED("Use MeshAccessor or Load with separate vertex attribute arrays instead.")
    bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0SkinnedElementType* vb, const void* ib, bool use16BitIndices);

    /// <summary>
    /// Updates the model mesh (used by the virtual models created with Init rather than Load).
    /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
    /// Mesh data will be cached and uploaded to the GPU with a delay.
    /// </summary>
    /// <param name="vertexCount">The amount of vertices in the vertex buffer.</param>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
    /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 32-bit stride buffer. Cannot be null.</param>
    /// <param name="blendIndices">The skeletal bones indices to use for skinning.</param>
    /// <param name="blendWeights">The skeletal bones weights to use for skinning (matches blendIndices).</param>
    /// <param name="normals">The normal vectors (per vertex).</param>
    /// <param name="tangents">The normal vectors (per vertex). Use null to compute them from normal vectors.</param>
    /// <param name="uvs">The texture coordinates (per vertex).</param>
    /// <param name="colors">The vertex colors (per vertex).</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, const Float3* vertices, const uint16* triangles, const Int4* blendIndices, const Float4* blendWeights, const Float3* normals = nullptr, const Float3* tangents = nullptr, const Float2* uvs = nullptr, const Color32* colors = nullptr);

    /// <summary>
    /// Updates the model mesh (used by the virtual models created with Init rather than Load).
    /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
    /// Mesh data will be cached and uploaded to the GPU with a delay.
    /// </summary>
    /// <param name="vertexCount">The amount of vertices in the vertex buffer.</param>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="vertices">The mesh vertices positions. Cannot be null.</param>
    /// <param name="triangles">The mesh index buffer (clockwise triangles). Uses 32-bit stride buffer. Cannot be null.</param>
    /// <param name="blendIndices">The skeletal bones indices to use for skinning.</param>
    /// <param name="blendWeights">The skeletal bones weights to use for skinning (matches blendIndices).</param>
    /// <param name="normals">The normal vectors (per vertex).</param>
    /// <param name="tangents">The normal vectors (per vertex). Use null to compute them from normal vectors.</param>
    /// <param name="uvs">The texture coordinates (per vertex).</param>
    /// <param name="colors">The vertex colors (per vertex).</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, const Float3* vertices, const uint32* triangles, const Int4* blendIndices, const Float4* blendWeights, const Float3* normals = nullptr, const Float3* tangents = nullptr, const Float2* uvs = nullptr, const Color32* colors = nullptr);

public:
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
    void Release() override;

private:
    // Internal bindings
#if !COMPILE_WITHOUT_CSHARP
    API_FUNCTION(NoProxy) bool UpdateMeshUInt(int32 vertexCount, int32 triangleCount, const MArray* verticesObj, const MArray* trianglesObj, const MArray* blendIndicesObj, const MArray* blendWeightsObj, const MArray* normalsObj, const MArray* tangentsObj, const MArray* uvObj, const MArray* colorsObj);
    API_FUNCTION(NoProxy) bool UpdateMeshUShort(int32 vertexCount, int32 triangleCount, const MArray* verticesObj, const MArray* trianglesObj, const MArray* blendIndicesObj, const MArray* blendWeightsObj, const MArray* normalsObj, const MArray* tangentsObj, const MArray* uvObj, const MArray* colorsObj);
    API_FUNCTION(NoProxy) MArray* DownloadBuffer(bool forceGpu, MTypeObject* resultType, int32 typeI);
#endif
};
