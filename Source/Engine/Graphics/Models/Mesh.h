// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "MeshBase.h"
#include "ModelInstanceEntry.h"
#include "Config.h"
#include "Types.h"
#if USE_PRECISE_MESH_INTERSECTS
#include "CollisionProxy.h"
#endif

class Lightmap;

/// <summary>
/// Represents part of the model that is made of vertices and can be rendered using custom material and transformation.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API Mesh : public MeshBase
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(Mesh, MeshBase);
protected:
    bool _hasLightmapUVs;
    GPUBuffer* _vertexBuffers[3] = {};
    GPUBuffer* _indexBuffer = nullptr;
#if USE_PRECISE_MESH_INTERSECTS
    CollisionProxy _collisionProxy;
#endif
    mutable Array<byte> _cachedVertexBuffer[3];
    mutable Array<byte> _cachedIndexBuffer;
    mutable int32 _cachedIndexBufferCount;

public:
    Mesh(const Mesh& other)
        : Mesh()
    {
#if !BUILD_RELEASE
        CRASH; // Not used
#endif
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="Mesh"/> class.
    /// </summary>
    ~Mesh();

public:
    /// <summary>
    /// Gets the model owning this mesh.
    /// </summary>
    FORCE_INLINE Model* GetModel() const
    {
        return (Model*)_model;
    }

    /// <summary>
    /// Gets the index buffer.
    /// </summary>
    FORCE_INLINE GPUBuffer* GetIndexBuffer() const
    {
        return _indexBuffer;
    }

    /// <summary>
    /// Gets the vertex buffer.
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns>The buffer.</returns>
    FORCE_INLINE GPUBuffer* GetVertexBuffer(int32 index) const
    {
        return _vertexBuffers[index];
    }

    /// <summary>
    /// Determines whether this mesh is initialized (has vertex and index buffers initialized).
    /// </summary>
    FORCE_INLINE bool IsInitialized() const
    {
        return _vertexBuffers[0] != nullptr;
    }

    /// <summary>
    /// Determines whether this mesh has a vertex colors buffer.
    /// </summary>
    API_PROPERTY() bool HasVertexColors() const;

    /// <summary>
    /// Determines whether this mesh contains valid lightmap texture coordinates data.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool HasLightmapUVs() const
    {
        return _hasLightmapUVs;
    }

#if USE_PRECISE_MESH_INTERSECTS
    /// <summary>
    /// Gets the collision proxy used by the mesh.
    /// </summary>
    FORCE_INLINE const CollisionProxy& GetCollisionProxy() const
    {
        return _collisionProxy;
    }
#endif

public:
    /// <summary>
    /// Updates the model mesh (used by the virtual models created with Init rather than Load).
    /// </summary>
    /// <param name="vertexCount">The amount of vertices in the vertex buffer.</param>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="vb0">The first vertex buffer data.</param>
    /// <param name="vb1">The second vertex buffer data.</param>
    /// <param name="vb2">The third vertex buffer data.</param>
    /// <param name="ib">The index buffer in clockwise order.</param>
    /// <returns>True if failed, otherwise false.</returns>
    FORCE_INLINE bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0ElementType* vb0, const VB1ElementType* vb1, const VB2ElementType* vb2, const uint32* ib)
    {
        return UpdateMesh(vertexCount, triangleCount, vb0, vb1, vb2, ib, false);
    }

    /// <summary>
    /// Updates the model mesh (used by the virtual models created with Init rather than Load).
    /// </summary>
    /// <param name="vertexCount">The amount of vertices in the vertex buffer.</param>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="vb0">The first vertex buffer data.</param>
    /// <param name="vb1">The second vertex buffer data.</param>
    /// <param name="vb2">The third vertex buffer data.</param>
    /// <param name="ib">The index buffer in clockwise order.</param>
    /// <returns>True if failed, otherwise false.</returns>
    FORCE_INLINE bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0ElementType* vb0, const VB1ElementType* vb1, const VB2ElementType* vb2, const uint16* ib)
    {
        return UpdateMesh(vertexCount, triangleCount, vb0, vb1, vb2, ib, true);
    }

    /// <summary>
    /// Updates the model mesh (used by the virtual models created with Init rather than Load).
    /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
    /// Mesh data will be cached and uploaded to the GPU with a delay.
    /// </summary>
    /// <param name="vertexCount">The amount of vertices in the vertex buffer.</param>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="vb0">The first vertex buffer data.</param>
    /// <param name="vb1">The second vertex buffer data.</param>
    /// <param name="vb2">The third vertex buffer data.</param>
    /// <param name="ib">The index buffer in clockwise order.</param>
    /// <param name="use16BitIndices">True if index buffer uses 16-bit index buffer, otherwise 32-bit.</param>
    /// <returns>True if failed, otherwise false.</returns>
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
    /// Updates the model mesh index buffer (used by the virtual models created with Init rather than Load).
    /// </summary>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="ib">The index buffer.</param>
    /// <returns>True if failed, otherwise false.</returns>
    FORCE_INLINE bool UpdateTriangles(uint32 triangleCount, const uint32* ib)
    {
        return UpdateTriangles(triangleCount, ib, false);
    }

    /// <summary>
    /// Updates the model mesh index buffer (used by the virtual models created with Init rather than Load).
    /// </summary>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="ib">The index buffer.</param>
    /// <returns>True if failed, otherwise false.</returns>
    FORCE_INLINE bool UpdateTriangles(uint32 triangleCount, const uint16* ib)
    {
        return UpdateTriangles(triangleCount, ib, true);
    }

    /// <summary>
    /// Updates the model mesh index buffer (used by the virtual models created with Init rather than Load).
    /// </summary>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="ib">The index buffer.</param>
    /// <param name="use16BitIndices">True if index buffer uses 16-bit index buffer, otherwise 32-bit.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool UpdateTriangles(uint32 triangleCount, const void* ib, bool use16BitIndices);

public:
    /// <summary>
    /// Initializes instance of the <see cref="Mesh"/> class.
    /// </summary>
    /// <param name="model">The model.</param>
    /// <param name="lodIndex">The LOD index.</param>
    /// <param name="index">The mesh index.</param>
    /// <param name="materialSlotIndex">The material slot index to use.</param>
    /// <param name="box">The bounding box.</param>
    /// <param name="sphere">The bounding sphere.</param>
    /// <param name="hasLightmapUVs">The lightmap UVs flag.</param>
    void Init(Model* model, int32 lodIndex, int32 index, int32 materialSlotIndex, const BoundingBox& box, const BoundingSphere& sphere, bool hasLightmapUVs);

    /// <summary>
    /// Load mesh data and Initialize GPU buffers
    /// </summary>
    /// <param name="vertices">Amount of vertices in the vertex buffer</param>
    /// <param name="triangles">Amount of triangles in the index buffer</param>
    /// <param name="vb0">Vertex buffer 0 data</param>
    /// <param name="vb1">Vertex buffer 1 data</param>
    /// <param name="vb2">Vertex buffer 2 data (may be null if not used)</param>
    /// <param name="ib">Index buffer data</param>
    /// <param name="use16BitIndexBuffer">True if use 16 bit indices for the index buffer (true: uint16, false: uint32).</param>
    /// <returns>True if cannot load data, otherwise false.</returns>
    bool Load(uint32 vertices, uint32 triangles, const void* vb0, const void* vb1, const void* vb2, const void* ib, bool use16BitIndexBuffer);

    /// <summary>
    /// Unloads the mesh data (vertex buffers and cache). The opposite to Load.
    /// </summary>
    void Unload();

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
    /// <param name="transform">The instance transformation.</param>
    /// <param name="distance">When the method completes and returns true, contains the distance of the intersection (if any valid).</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
    /// <returns>True whether the two objects intersected</returns>
    bool Intersects(const Ray& ray, const Transform& transform, Real& distance, Vector3& normal) const;

public:
    /// <summary>
    /// Gets the draw call geometry for this mesh. Sets the index and vertex buffers.
    /// </summary>
    /// <param name="drawCall">The draw call.</param>
    void GetDrawCallGeometry(DrawCall& drawCall) const;

    /// <summary>
    /// Draws the mesh. Binds vertex and index buffers and invokes the draw call.
    /// </summary>
    /// <param name="context">The GPU context.</param>
    void Render(GPUContext* context) const;

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
    bool DownloadDataGPU(MeshBufferType type, BytesContainer& result) const override;
    Task* DownloadDataGPUAsync(MeshBufferType type, BytesContainer& result) const override;
    bool DownloadDataCPU(MeshBufferType type, BytesContainer& result, int32& count) const override;

private:
    // Internal bindings
    API_FUNCTION(NoProxy) ScriptingObject* GetParentModel();
#if !COMPILE_WITHOUT_CSHARP
    API_FUNCTION(NoProxy) bool UpdateMeshUInt(int32 vertexCount, int32 triangleCount, const MArray* verticesObj, const MArray* trianglesObj, const MArray* normalsObj, const MArray* tangentsObj, const MArray* uvObj, const MArray* colorsObj);
    API_FUNCTION(NoProxy) bool UpdateMeshUShort(int32 vertexCount, int32 triangleCount, const MArray* verticesObj, const MArray* trianglesObj, const MArray* normalsObj, const MArray* tangentsObj, const MArray* uvObj, const MArray* colorsObj);
    API_FUNCTION(NoProxy) bool UpdateTrianglesUInt(int32 triangleCount, const MArray* trianglesObj);
    API_FUNCTION(NoProxy) bool UpdateTrianglesUShort(int32 triangleCount, const MArray* trianglesObj);
    API_FUNCTION(NoProxy) MArray* DownloadBuffer(bool forceGpu, MTypeObject* resultType, int32 typeI);
#endif
};
