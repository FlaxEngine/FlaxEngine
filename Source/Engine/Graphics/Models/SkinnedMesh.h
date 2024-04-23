// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "MeshBase.h"
#include "Types.h"
#include "BlendShape.h"

/// <summary>
/// Represents part of the skinned model that is made of vertices and can be rendered using custom material, transformation and skeleton bones hierarchy.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API SkinnedMesh : public MeshBase
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(SkinnedMesh, MeshBase);
protected:
    GPUBuffer* _vertexBuffer = nullptr;
    GPUBuffer* _indexBuffer = nullptr;
    mutable Array<byte> _cachedIndexBuffer;
    mutable Array<byte> _cachedVertexBuffer;
    mutable int32 _cachedIndexBufferCount;

public:
    SkinnedMesh(const SkinnedMesh& other)
        : SkinnedMesh()
    {
#if !BUILD_RELEASE
        CRASH; // Not used
#endif
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="SkinnedMesh"/> class.
    /// </summary>
    ~SkinnedMesh();

public:
    /// <summary>
    /// Gets the skinned model owning this mesh.
    /// </summary>
    FORCE_INLINE SkinnedModel* GetSkinnedModel() const
    {
        return (SkinnedModel*)_model;
    }

    /// <summary>
    /// Determines whether this mesh is initialized (has vertex and index buffers initialized).
    /// </summary>
    FORCE_INLINE bool IsInitialized() const
    {
        return _vertexBuffer != nullptr;
    }

    /// <summary>
    /// Blend shapes used by this mesh.
    /// </summary>
    Array<BlendShape> BlendShapes;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="SkinnedMesh"/> class.
    /// </summary>
    /// <param name="model">The model.</param>
    /// <param name="lodIndex">The model LOD index.</param>
    /// <param name="index">The mesh index.</param>
    /// <param name="materialSlotIndex">The material slot index to use.</param>
    /// <param name="box">The bounding box.</param>
    /// <param name="sphere">The bounding sphere.</param>
    void Init(SkinnedModel* model, int32 lodIndex, int32 index, int32 materialSlotIndex, const BoundingBox& box, const BoundingSphere& sphere);

    /// <summary>
    /// Load mesh data and Initialize GPU buffers
    /// </summary>
    /// <param name="vertices">Amount of vertices in the vertex buffer</param>
    /// <param name="triangles">Amount of triangles in the index buffer</param>
    /// <param name="vb0">Vertex buffer data</param>
    /// <param name="ib">Index buffer data</param>
    /// <param name="use16BitIndexBuffer">True if use 16 bit indices for the index buffer (true: uint16, false: uint32).</param>
    /// <returns>True if cannot load data, otherwise false.</returns>
    bool Load(uint32 vertices, uint32 triangles, const void* vb0, const void* ib, bool use16BitIndexBuffer);

    /// <summary>
    /// Unloads the mesh data (vertex buffers and cache). The opposite to Load.
    /// </summary>
    void Unload();

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
    /// Determines if there is an intersection between the mesh and a ray in given world
    /// </summary>
    /// <param name="ray">The ray to test</param>
    /// <param name="world">World to transform box</param>
    /// <param name="distance">When the method completes and returns true, contains the distance of the intersection (if any valid).</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
    /// <returns>True whether the two objects intersected</returns>
    bool Intersects(const Ray& ray, const Matrix& world, Real& distance, Vector3& normal) const;

    /// <summary>
    /// Determines if there is an intersection between the mesh and a ray in given world
    /// </summary>
    /// <param name="ray">The ray to test</param>
    /// <param name="transform">Instance transformation</param>
    /// <param name="distance">When the method completes and returns true, contains the distance of the intersection (if any valid).</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
    /// <returns>True whether the two objects intersected</returns>
    bool Intersects(const Ray& ray, const Transform& transform, Real& distance, Vector3& normal) const;

public:
    /// <summary>
    /// Draws the mesh. Binds vertex and index buffers and invokes the draw call.
    /// </summary>
    /// <param name="context">The GPU context.</param>
    void Render(GPUContext* context) const;

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
    bool DownloadDataGPU(MeshBufferType type, BytesContainer& result) const override;
    Task* DownloadDataGPUAsync(MeshBufferType type, BytesContainer& result) const override;
    bool DownloadDataCPU(MeshBufferType type, BytesContainer& result, int32& count) const override;

private:
    // Internal bindings
    API_FUNCTION(NoProxy) ScriptingObject* GetParentModel();
#if !COMPILE_WITHOUT_CSHARP
    API_FUNCTION(NoProxy) bool UpdateMeshUInt(const MArray* verticesObj, const MArray* trianglesObj, const MArray* blendIndicesObj, const MArray* blendWeightsObj, const MArray* normalsObj, const MArray* tangentsObj, const MArray* uvObj);
    API_FUNCTION(NoProxy) bool UpdateMeshUShort(const MArray* verticesObj, const MArray* trianglesObj, const MArray* blendIndicesObj, const MArray* blendWeightsObj, const MArray* normalsObj, const MArray* tangentsObj, const MArray* uvObj);
    API_FUNCTION(NoProxy) MArray* DownloadBuffer(bool forceGpu, MTypeObject* resultType, int32 typeI);
#endif
};
