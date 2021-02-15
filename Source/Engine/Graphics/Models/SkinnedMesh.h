// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Graphics/RenderTask.h"
#include "ModelInstanceEntry.h"
#include "Types.h"
#include "BlendShape.h"

class GPUBuffer;

/// <summary>
/// Represents part of the skinned model that is made of vertices and can be rendered using custom material, transformation and skeleton bones hierarchy.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API SkinnedMesh : public PersistentScriptingObject
{
DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(SkinnedMesh, PersistentScriptingObject);
protected:

    SkinnedModel* _model;
    int32 _index;
    int32 _lodIndex;
    int32 _materialSlotIndex;
    bool _use16BitIndexBuffer;
    BoundingBox _box;
    BoundingSphere _sphere;
    uint32 _vertices;
    uint32 _triangles;
    GPUBuffer* _vertexBuffer;
    GPUBuffer* _indexBuffer;
    mutable Array<byte> _cachedIndexBuffer;
    mutable Array<byte> _cachedVertexBuffer;

public:

    SkinnedMesh(const SkinnedMesh& other)
        : SkinnedMesh()
    {
#if !!BUILD_RELEASE
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
    /// <returns>The skinned model</returns>
    FORCE_INLINE SkinnedModel* GetSkinnedModel() const
    {
        return _model;
    }

    /// <summary>
    /// Gets the mesh index.
    /// </summary>
    /// <returns>The index</returns>
    FORCE_INLINE int32 GetIndex() const
    {
        return _index;
    }

    /// <summary>
    /// Gets the material slot index.
    /// </summary>
    /// <returns>The material slot index</returns>
    API_PROPERTY() FORCE_INLINE int32 GetMaterialSlotIndex() const
    {
        return _materialSlotIndex;
    }

    /// <summary>
    /// Sets the index of the material slot index.
    /// </summary>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetMaterialSlotIndex(int32 value);

    /// <summary>
    /// Gets the triangle count.
    /// </summary>
    /// <returns>The triangles</returns>
    API_PROPERTY() FORCE_INLINE int32 GetTriangleCount() const
    {
        return _triangles;
    }

    /// <summary>
    /// Gets the vertex count.
    /// </summary>
    /// <returns>The vertices</returns>
    API_PROPERTY() FORCE_INLINE int32 GetVertexCount() const
    {
        return _vertices;
    }

    /// <summary>
    /// Determines whether this mesh is initialized (has vertex and index buffers initialized).
    /// </summary>
    /// <returns>True if this instance is initialized, otherwise false.</returns>
    FORCE_INLINE bool IsInitialized() const
    {
        return _vertexBuffer != nullptr;
    }

    /// <summary>
    /// Determines whether this mesh is using 16 bit index buffer, otherwise it's 32 bit.
    /// </summary>
    /// <returns>True if this mesh is using 16 bit index buffer, otherwise 32 bit index buffer.</returns>
    API_PROPERTY() FORCE_INLINE bool Use16BitIndexBuffer() const
    {
        return _use16BitIndexBuffer;
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
    bool Load(uint32 vertices, uint32 triangles, void* vb0, void* ib, bool use16BitIndexBuffer);

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
    FORCE_INLINE bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, VB0SkinnedElementType* vb, int32* ib)
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
    FORCE_INLINE bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, VB0SkinnedElementType* vb, uint16* ib)
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
    bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, VB0SkinnedElementType* vb, void* ib, bool use16BitIndices);

public:

    /// <summary>
    /// Sets the mesh bounds.
    /// </summary>
    /// <param name="box">The bounding box.</param>
    void SetBounds(const BoundingBox& box)
    {
        _box = box;
        BoundingSphere::FromBox(box, _sphere);
    }

    /// <summary>
    /// Gets the box.
    /// </summary>
    /// <returns>The bounding box.</returns>
    API_PROPERTY() FORCE_INLINE BoundingBox GetBox() const
    {
        return _box;
    }

    /// <summary>
    /// Gets the sphere.
    /// </summary>
    /// <returns>The bounding sphere.</returns>
    API_PROPERTY() FORCE_INLINE BoundingSphere GetSphere() const
    {
        return _sphere;
    }

public:

    /// <summary>
    /// Determines if there is an intersection between the mesh and a ray in given world
    /// </summary>
    /// <param name="ray">The ray to test</param>
    /// <param name="world">World to transform box</param>
    /// <param name="distance">When the method completes and returns true, contains the distance of the intersection (if any valid).</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
    /// <returns>True whether the two objects intersected</returns>
    bool Intersects(const Ray& ray, const Matrix& world, float& distance, Vector3& normal) const;

    /// <summary>
    /// Retrieves the eight corners of the bounding box.
    /// </summary>
    /// <param name="corners">An array of points representing the eight corners of the bounding box.</param>
    FORCE_INLINE void GetCorners(Vector3 corners[8]) const
    {
        _box.GetCorners(corners);
    }

public:

    /// <summary>
    /// Model instance drawing packed data.
    /// </summary>
    struct DrawInfo
    {
        /// <summary>
        /// The instance buffer to use during model rendering
        /// </summary>
        ModelInstanceEntries* Buffer;

        /// <summary>
        /// The skinning.
        /// </summary>
        SkinnedMeshDrawData* Skinning;

        /// <summary>
        /// The blend shapes.
        /// </summary>
        BlendShapesInstance* BlendShapes;

        /// <summary>
        /// The world transformation of the model.
        /// </summary>
        Matrix* World;

        /// <summary>
        /// The instance drawing state data container. Used for LOD transition handling and previous world transformation matrix updating. 
        /// </summary>
        GeometryDrawStateData* DrawState;

        /// <summary>
        /// The object draw modes.
        /// </summary>
        DrawPass DrawModes;

        /// <summary>
        /// The bounds of the model (used to select a proper LOD during rendering).
        /// </summary>
        BoundingSphere Bounds;

        /// <summary>
        /// The per-instance random value.
        /// </summary>
        float PerInstanceRandom;

        /// <summary>
        /// The LOD bias value.
        /// </summary>
        char LODBias;

        /// <summary>
        /// The forced LOD to use. Value -1 disables this feature.
        /// </summary>
        char ForcedLOD;
    };

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

public:

    /// <summary>
    /// Extract mesh buffer data from the GPU (cannot be called from the main thread!).
    /// </summary>
    /// <param name="type">Buffer type</param>
    /// <param name="result">The result data</param>
    /// <returns>True if failed, otherwise false</returns>
    bool DownloadDataGPU(MeshBufferType type, BytesContainer& result) const;

    /// <summary>
    /// Extracts mesh buffer data from the GPU in the async task.
    /// </summary>
    /// <param name="type">Buffer type</param>
    /// <param name="result">The result data</param>
    /// <returns>Created async task used to gather the buffer data.</returns>
    Task* DownloadDataAsyncGPU(MeshBufferType type, BytesContainer& result) const;

    /// <summary>
    /// Extract mesh buffer data from the CPU.
    /// </summary>
    /// <param name="type">Buffer type</param>
    /// <param name="result">The result data</param>
    /// <returns>True if failed, otherwise false</returns>
    bool DownloadDataCPU(MeshBufferType type, BytesContainer& result) const;

private:

    // Internal bindings
    API_FUNCTION(NoProxy) ScriptingObject* GetParentModel();
    API_FUNCTION(NoProxy) bool UpdateMeshInt(MonoArray* verticesObj, MonoArray* trianglesObj, MonoArray* blendIndicesObj, MonoArray* blendWeightsObj, MonoArray* normalsObj, MonoArray* tangentsObj, MonoArray* uvObj);
    API_FUNCTION(NoProxy) bool UpdateMeshUShort(MonoArray* verticesObj, MonoArray* trianglesObj, MonoArray* blendIndicesObj, MonoArray* blendWeightsObj, MonoArray* normalsObj, MonoArray* tangentsObj, MonoArray* uvObj);
    API_FUNCTION(NoProxy) bool DownloadBuffer(bool forceGpu, MonoArray* resultObj, int32 typeI);
};
