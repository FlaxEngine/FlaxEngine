// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "MeshBase.h"
#include "BlendShape.h"

/// <summary>
/// Represents part of the skinned model that is made of vertices and can be rendered using custom material, transformation and skeleton bones hierarchy.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API SkinnedMesh : public MeshBase
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(SkinnedMesh, MeshBase);

public:
    SkinnedMesh(const SkinnedMesh& other)
        : SkinnedMesh()
    {
#if !BUILD_RELEASE
        CRASH; // Not used
#endif
    }

public:
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

public:
    /// <summary>
    /// Updates the model mesh (used by the virtual models created with Init rather than Load).
    /// </summary>
    /// <param name="vertexCount">The amount of vertices in the vertex buffer.</param>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="vb">The vertex buffer data.</param>
    /// <param name="ib">The index buffer in clockwise order.</param>
    /// <returns>True if failed, otherwise false.</returns>
    FORCE_INLINE bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0SkinnedElementType* vb, const int32* ib)
    {
        return UpdateMesh(vertexCount, triangleCount, vb, ib, false);
    }

    /// <summary>
    /// Updates the model mesh (used by the virtual models created with Init rather than Load).
    /// </summary>
    /// <param name="vertexCount">The amount of vertices in the vertex buffer.</param>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="vb">The vertex buffer data.</param>
    /// <param name="ib">The index buffer in clockwise order.</param>
    /// <returns>True if failed, otherwise false.</returns>
    FORCE_INLINE bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0SkinnedElementType* vb, const uint32* ib)
    {
        return UpdateMesh(vertexCount, triangleCount, vb, ib, false);
    }

    /// <summary>
    /// Updates the model mesh (used by the virtual models created with Init rather than Load).
    /// </summary>
    /// <param name="vertexCount">The amount of vertices in the vertex buffer.</param>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="vb">The vertex buffer data.</param>
    /// <param name="ib">The index buffer, clockwise order.</param>
    /// <returns>True if failed, otherwise false.</returns>
    FORCE_INLINE bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0SkinnedElementType* vb, const uint16* ib)
    {
        return UpdateMesh(vertexCount, triangleCount, vb, ib, true);
    }

    /// <summary>
    /// Updates the model mesh (used by the virtual models created with Init rather than Load).
    /// </summary>
    /// <param name="vertexCount">The amount of vertices in the vertex buffer.</param>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="vb">The vertex buffer data.</param>
    /// <param name="ib">The index buffer in clockwise order.</param>
    /// <param name="use16BitIndices">True if index buffer uses 16-bit index buffer, otherwise 32-bit.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0SkinnedElementType* vb, const void* ib, bool use16BitIndices);

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
    bool DownloadDataCPU(MeshBufferType type, BytesContainer& result, int32& count) const override;

private:
    // Internal bindings
#if !COMPILE_WITHOUT_CSHARP
    API_FUNCTION(NoProxy) bool UpdateMeshUInt(const MArray* verticesObj, const MArray* trianglesObj, const MArray* blendIndicesObj, const MArray* blendWeightsObj, const MArray* normalsObj, const MArray* tangentsObj, const MArray* uvObj);
    API_FUNCTION(NoProxy) bool UpdateMeshUShort(const MArray* verticesObj, const MArray* trianglesObj, const MArray* blendIndicesObj, const MArray* blendWeightsObj, const MArray* normalsObj, const MArray* tangentsObj, const MArray* uvObj);
    API_FUNCTION(NoProxy) MArray* DownloadBuffer(bool forceGpu, MTypeObject* resultType, int32 typeI);
#endif
};
