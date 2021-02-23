// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Graphics/RenderTask.h"
#include "ModelInstanceEntry.h"
#include "Config.h"
#include "Types.h"
#if USE_PRECISE_MESH_INTERSECTS
#include "CollisionProxy.h"
#endif

class GPUBuffer;

/// <summary>
/// Represents part of the model that is made of vertices and can be rendered using custom material and transformation.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API Mesh : public PersistentScriptingObject
{
DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(Mesh, PersistentScriptingObject);
protected:

    Model* _model;
    int32 _index;
    int32 _lodIndex;
    int32 _materialSlotIndex;
    bool _use16BitIndexBuffer;
    bool _hasLightmapUVs;
    BoundingBox _box;
    BoundingSphere _sphere;
    uint32 _vertices;
    uint32 _triangles;
    GPUBuffer* _vertexBuffers[3];
    GPUBuffer* _indexBuffer;
#if USE_PRECISE_MESH_INTERSECTS
    CollisionProxy _collisionProxy;
#endif

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
        return _model;
    }

    /// <summary>
    /// Gets the mesh parent LOD index.
    /// </summary>
    FORCE_INLINE int32 GetLODIndex() const
    {
        return _lodIndex;
    }

    /// <summary>
    /// Gets the mesh index.
    /// </summary>
    FORCE_INLINE int32 GetIndex() const
    {
        return _index;
    }

    /// <summary>
    /// Gets the index of the material slot to use during this mesh rendering.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetMaterialSlotIndex() const
    {
        return _materialSlotIndex;
    }

    /// <summary>
    /// Sets the index of the material slot to use during this mesh rendering.
    /// </summary>
    API_PROPERTY() void SetMaterialSlotIndex(int32 value);

    /// <summary>
    /// Gets the triangle count.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetTriangleCount() const
    {
        return _triangles;
    }

    /// <summary>
    /// Gets the vertex count.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetVertexCount() const
    {
        return _vertices;
    }

    /// <summary>
    /// Gets the index buffer.
    /// </summary>
    /// <returns>The buffer.</returns>
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
    /// <returns>True if this instance is initialized, otherwise false.</returns>
    FORCE_INLINE bool IsInitialized() const
    {
        return _vertexBuffers[0] != nullptr;
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
    /// Determines whether this mesh has a vertex colors buffer.
    /// </summary>
    /// <returns>True if this mesh has a vertex colors buffers.</returns>
    API_PROPERTY() FORCE_INLINE bool HasVertexColors() const
    {
        return _vertexBuffers[2] != nullptr && _vertexBuffers[2]->IsAllocated();
    }

    /// <summary>
    /// Determines whether this mesh contains valid lightmap texture coordinates data.
    /// </summary>
    /// <returns>True if this mesh has a vertex colors buffers.</returns>
    API_PROPERTY() FORCE_INLINE bool HasLightmapUVs() const
    {
        return _hasLightmapUVs;
    }

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
    API_PROPERTY() FORCE_INLINE const BoundingBox& GetBox() const
    {
        return _box;
    }

    /// <summary>
    /// Gets the sphere.
    /// </summary>
    /// <returns>The bounding sphere.</returns>
    API_PROPERTY() FORCE_INLINE const BoundingSphere& GetSphere() const
    {
        return _sphere;
    }

#if USE_PRECISE_MESH_INTERSECTS

    /// <summary>
    /// Gets the collision proxy used by the mesh.
    /// </summary>
    /// <returns>The collisions proxy container object reference.</returns>
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
    FORCE_INLINE bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, VB0ElementType* vb0, VB1ElementType* vb1, VB2ElementType* vb2, uint32* ib)
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
    FORCE_INLINE bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, VB0ElementType* vb0, VB1ElementType* vb1, VB2ElementType* vb2, uint16* ib)
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
    bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, VB0ElementType* vb0, VB1ElementType* vb1, VB2ElementType* vb2, void* ib, bool use16BitIndices);

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
    bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, Vector3* vertices, uint16* triangles, Vector3* normals = nullptr, Vector3* tangents = nullptr, Vector2* uvs = nullptr, Color32* colors = nullptr);

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
    bool UpdateMesh(uint32 vertexCount, uint32 triangleCount, Vector3* vertices, uint32* triangles, Vector3* normals = nullptr, Vector3* tangents = nullptr, Vector2* uvs = nullptr, Color32* colors = nullptr);

public:

    /// <summary>
    /// Updates the model mesh index buffer (used by the virtual models created with Init rather than Load).
    /// </summary>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="ib">The index buffer.</param>
    /// <returns>True if failed, otherwise false.</returns>
    FORCE_INLINE bool UpdateTriangles(uint32 triangleCount, uint32* ib)
    {
        return UpdateTriangles(triangleCount, ib, false);
    }

    /// <summary>
    /// Updates the model mesh index buffer (used by the virtual models created with Init rather than Load).
    /// </summary>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="ib">The index buffer.</param>
    /// <returns>True if failed, otherwise false.</returns>
    FORCE_INLINE bool UpdateTriangles(uint32 triangleCount, uint16* ib)
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
    bool UpdateTriangles(uint32 triangleCount, void* ib, bool use16BitIndices);

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
    bool Load(uint32 vertices, uint32 triangles, void* vb0, void* vb1, void* vb2, void* ib, bool use16BitIndexBuffer);

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
    /// Gets the draw call geometry for this mesh. Sets the index and vertex buffers.
    /// </summary>
    /// <param name="drawCall">The draw call.</param>
    void GetDrawCallGeometry(DrawCall& drawCall) const;

    /// <summary>
    /// Model instance drawing packed data.
    /// </summary>
    struct DrawInfo
    {
        /// <summary>
        /// The instance buffer to use during model rendering.
        /// </summary>
        ModelInstanceEntries* Buffer;

        /// <summary>
        /// The world transformation of the model.
        /// </summary>
        Matrix* World;

        /// <summary>
        /// The instance drawing state data container. Used for LOD transition handling and previous world transformation matrix updating. 
        /// </summary>
        GeometryDrawStateData* DrawState;

        /// <summary>
        /// The lightmap.
        /// </summary>
        const Lightmap* Lightmap;

        /// <summary>
        /// The lightmap UVs.
        /// </summary>
        const Rectangle* LightmapUVs;

        /// <summary>
        /// The model instance vertex colors buffers (per-lod all meshes packed in a single allocation, array length equal to model lods count).
        /// </summary>
        GPUBuffer** VertexColors;

        /// <summary>
        /// The object static flags.
        /// </summary>
        StaticFlags Flags;

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
    /// <param name="material">The material to use for rendering.</param>
    /// <param name="world">The world transformation of the model.</param>
    /// <param name="flags">The object static flags.</param>
    /// <param name="receiveDecals">True if rendered geometry can receive decals, otherwise false.</param>
    /// <param name="drawModes">The draw passes to use for rendering this object.</param>
    /// <param name="perInstanceRandom">The random per-instance value (normalized to range 0-1).</param>
    API_FUNCTION() void Draw(API_PARAM(Ref) const RenderContext& renderContext, MaterialBase* material, API_PARAM(Ref) const Matrix& world, StaticFlags flags = StaticFlags::None, bool receiveDecals = true, DrawPass drawModes = DrawPass::Default, float perInstanceRandom = 0.0f) const;

    /// <summary>
    /// Draws the mesh.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="info">The packed drawing info data.</param>
    /// <param name="lodDitherFactor">The LOD transition dither factor.</param>
    void Draw(const RenderContext& renderContext, const DrawInfo& info, float lodDitherFactor) const;

public:

    /// <summary>
    /// Extract mesh buffer data (cannot be called from the main thread!).
    /// </summary>
    /// <param name="type">Buffer type</param>
    /// <param name="result">The result data</param>
    /// <returns>True if failed, otherwise false</returns>
    bool ExtractData(MeshBufferType type, BytesContainer& result) const;

    /// <summary>
    /// Extracts mesh buffer data in the async task.
    /// </summary>
    /// <param name="type">Buffer type</param>
    /// <param name="result">The result data</param>
    /// <returns>Created async task used to gather the buffer data.</returns>
    Task* ExtractDataAsync(MeshBufferType type, BytesContainer& result) const;

private:

    // Internal bindings
    API_FUNCTION(NoProxy) ScriptingObject* GetParentModel();
    API_FUNCTION(NoProxy) bool UpdateMeshInt(int32 vertexCount, int32 triangleCount, MonoArray* verticesObj, MonoArray* trianglesObj, MonoArray* normalsObj, MonoArray* tangentsObj, MonoArray* uvObj, MonoArray* colorsObj);
    API_FUNCTION(NoProxy) bool UpdateMeshUShort(int32 vertexCount, int32 triangleCount, MonoArray* verticesObj, MonoArray* trianglesObj, MonoArray* normalsObj, MonoArray* tangentsObj, MonoArray* uvObj, MonoArray* colorsObj);
    API_FUNCTION(NoProxy) bool UpdateTrianglesInt(int32 triangleCount, MonoArray* trianglesObj);
    API_FUNCTION(NoProxy) bool UpdateTrianglesUShort(int32 triangleCount, MonoArray* trianglesObj);
    API_FUNCTION(NoProxy) bool DownloadBuffer(bool forceGpu, MonoArray* resultObj, int32 typeI);
};
