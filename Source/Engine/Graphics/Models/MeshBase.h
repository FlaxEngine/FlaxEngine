// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Graphics/Enums.h"
#include "Engine/Graphics/Models/Types.h"
#include "Engine/Level/Types.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Config.h"
#if MODEL_USE_PRECISE_MESH_INTERSECTS
#include "CollisionProxy.h"
#endif

struct GeometryDrawStateData;
struct RenderContext;
struct RenderContextBatch;
class Task;
class ModelBase;
class Lightmap;
class GPUBuffer;
class SkinnedMeshDrawData;
class BlendShapesInstance;

/// <summary>
/// Base class for mesh objects.
/// </summary>
API_CLASS(Abstract, NoSpawn) class FLAXENGINE_API MeshBase : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(MeshBase);
    friend class Model;
    friend class SkinnedModel;

protected:
    ModelBase* _model = nullptr;
    BoundingBox _box = BoundingBox::Zero;
    BoundingSphere _sphere = BoundingSphere::Empty;

    int32 _index = 0;
    int32 _lodIndex = 0;
    uint32 _vertices = 0;
    uint32 _triangles = 0;
    int32 _materialSlotIndex = 0;
    bool _use16BitIndexBuffer = false;
    bool _hasBounds = false;

    GPUBuffer* _vertexBuffers[MODEL_MAX_VB] = {};
    GPUBuffer* _indexBuffer = nullptr;

    mutable BytesContainer _cachedVertexBuffers[MODEL_MAX_VB];
    mutable GPUVertexLayout* _cachedVertexLayouts[MODEL_MAX_VB] = {};
    mutable BytesContainer _cachedIndexBuffer;
    mutable int32 _cachedIndexBufferCount = 0, _cachedVertexBufferCount = 0;

#if MODEL_USE_PRECISE_MESH_INTERSECTS
    CollisionProxy _collisionProxy;
#endif

    void Link(ModelBase* model, int32 lodIndex, int32 index)
    {
        _model = model;
        _lodIndex = lodIndex;
        _index = index;
    }

    explicit MeshBase(const SpawnParams& params)
        : ScriptingObject(params)
    {
    }

public:
    ~MeshBase();

    /// <summary>
    /// Gets the model owning this mesh.
    /// </summary>
    API_PROPERTY() FORCE_INLINE ModelBase* GetModelBase() const
    {
        return _model;
    }

    /// <summary>
    /// Gets the mesh parent LOD index.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetLODIndex() const
    {
        return _lodIndex;
    }

    /// <summary>
    /// Gets the mesh index.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetIndex() const
    {
        return _index;
    }

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
    /// Gets the box.
    /// </summary>
    API_PROPERTY() FORCE_INLINE const BoundingBox& GetBox() const
    {
        return _box;
    }

    /// <summary>
    /// Gets the sphere.
    /// </summary>
    API_PROPERTY() FORCE_INLINE const BoundingSphere& GetSphere() const
    {
        return _sphere;
    }

    /// <summary>
    /// Determines whether this mesh is using 16 bit index buffer, otherwise it's 32 bit.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool Use16BitIndexBuffer() const
    {
        return _use16BitIndexBuffer;
    }

#if MODEL_USE_PRECISE_MESH_INTERSECTS
    /// <summary>
    /// Gets the collision proxy used by the mesh.
    /// </summary>
    FORCE_INLINE const CollisionProxy& GetCollisionProxy() const
    {
        return _collisionProxy;
    }
#endif

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
    /// Sets the mesh bounds.
    /// </summary>
    /// <param name="box">The bounding box.</param>
    API_FUNCTION() void SetBounds(API_PARAM(ref) const BoundingBox& box);

    /// <summary>
    /// Sets the mesh bounds.
    /// </summary>
    /// <param name="box">The bounding box.</param>
    /// <param name="sphere">The bounding sphere.</param>
    API_FUNCTION() void SetBounds(API_PARAM(ref) const BoundingBox& box, API_PARAM(ref) const BoundingSphere& sphere);

    /// <summary>
    /// Gets the index buffer.
    /// </summary>
    API_FUNCTION() FORCE_INLINE GPUBuffer* GetIndexBuffer() const
    {
        return _indexBuffer;
    }

    /// <summary>
    /// Gets the vertex buffer.
    /// </summary>
    /// <param name="index">The bind slot index.</param>
    /// <returns>The buffer or null if not used.</returns>
    API_FUNCTION() FORCE_INLINE GPUBuffer* GetVertexBuffer(int32 index) const
    {
        return _vertexBuffers[index];
    }

    /// <summary>
    /// Gets the vertex buffers layout. Made out of all buffers used by this mesh.
    /// </summary>
    /// <returns>The vertex layout.</returns>
    API_PROPERTY() GPUVertexLayout* GetVertexLayout() const;

public:
    /// <summary>
    /// Initializes the mesh buffers.
    /// </summary>
    /// <param name="vertices">Amount of vertices in the vertex buffer.</param>
    /// <param name="triangles">Amount of triangles in the index buffer.</param>
    /// <param name="vbData">Array with pointers to vertex buffers initial data (layout defined by <paramref name="vbLayout"/>).</param>
    /// <param name="ibData">Pointer to index buffer data. Data is uint16 or uint32 depending on <paramref name="use16BitIndexBuffer"/> value.</param>
    /// <param name="use16BitIndexBuffer">True to use 16-bit indices for the index buffer (true: uint16, false: uint32).</param>
    /// <param name="vbLayout">Layout descriptors for the vertex buffers attributes (one for each vertex buffer).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION(Sealed) virtual bool Init(uint32 vertices, uint32 triangles, const Array<const void*, FixedAllocation<MODEL_MAX_VB>>& vbData, const void* ibData, bool use16BitIndexBuffer, Array<GPUVertexLayout*, FixedAllocation<MODEL_MAX_VB>> vbLayout);

    /// <summary>
    /// Releases the mesh data (GPU buffers and local cache).
    /// </summary>
    virtual void Release();

    /// <summary>
    /// Unloads the mesh data (vertex buffers and cache). The opposite to Load.
    /// [Deprecated in v1.10]
    /// </summary>
    DEPRECATED("Use Release instead.") void Unload()
    {
        Release();
    }

public:
    /// <summary>
    /// Updates the model mesh index buffer.
    /// </summary>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="ib">The index buffer.</param>
    /// <returns>True if failed, otherwise false.</returns>
    FORCE_INLINE bool UpdateTriangles(uint32 triangleCount, const uint32* ib)
    {
        return UpdateTriangles(triangleCount, ib, false);
    }

    /// <summary>
    /// Updates the model mesh index buffer.
    /// </summary>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="ib">The index buffer.</param>
    /// <returns>True if failed, otherwise false.</returns>
    FORCE_INLINE bool UpdateTriangles(uint32 triangleCount, const uint16* ib)
    {
        return UpdateTriangles(triangleCount, ib, true);
    }

    /// <summary>
    /// Updates the model mesh index buffer.
    /// </summary>
    /// <param name="triangleCount">The amount of triangles in the index buffer.</param>
    /// <param name="ib">The index buffer.</param>
    /// <param name="use16BitIndices">True if index buffer uses 16-bit index buffer, otherwise 32-bit.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool UpdateTriangles(uint32 triangleCount, const void* ib, bool use16BitIndices);

public:
    /// <summary>
    /// Determines if there is an intersection between the mesh and a ray in given world.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="world">The mesh instance transformation.</param>
    /// <param name="distance">When the method completes and returns true, contains the distance of the intersection (if any valid).</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
    /// <returns>True whether the two objects intersected, otherwise false.</returns>
    bool Intersects(const Ray& ray, const Matrix& world, Real& distance, Vector3& normal) const;

    /// <summary>
    /// Determines if there is an intersection between the mesh and a ray in given world
    /// </summary>
    /// <param name="ray">The ray to test</param>
    /// <param name="transform">The mesh instance transformation.</param>
    /// <param name="distance">When the method completes and returns true, contains the distance of the intersection (if any valid).</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
    /// <returns>True whether the two objects intersected, otherwise false.</returns>
    bool Intersects(const Ray& ray, const Transform& transform, Real& distance, Vector3& normal) const;

public:
    /// <summary>
    /// Extracts mesh buffer data from a GPU. Cannot be called from the main thread.
    /// </summary>
    /// <param name="type">Buffer type</param>
    /// <param name="result">The result data</param>
    /// <param name="layout">The result layout of the vertex buffer (optional).</param>
    /// <returns>True if failed, otherwise false</returns>
    bool DownloadDataGPU(MeshBufferType type, BytesContainer& result, GPUVertexLayout** layout = nullptr) const;

    /// <summary>
    /// Extracts mesh buffer data from GPU in the async task.
    /// </summary>
    /// <param name="type">Buffer type</param>
    /// <param name="result">The result data</param>
    /// <param name="layout">The result layout of the vertex buffer (optional).</param>
    /// <returns>Created async task used to gather the buffer data.</returns>
    Task* DownloadDataGPUAsync(MeshBufferType type, BytesContainer& result, GPUVertexLayout** layout = nullptr) const;

    /// <summary>
    /// Extract mesh buffer data from CPU. Cached internally.
    /// </summary>
    /// <param name="type">Buffer type</param>
    /// <param name="result">The result data</param>
    /// <param name="count">The amount of items inside the result buffer.</param>
    /// <param name="layout">The result layout of the vertex buffer (optional).</param>
    /// <returns>True if failed, otherwise false</returns>
    bool DownloadDataCPU(MeshBufferType type, BytesContainer& result, int32& count, GPUVertexLayout** layout = nullptr) const;

    /// <summary>
    /// Extracts mesh buffers data.
    /// </summary>
    /// <param name="types">List of buffers to load.</param>
    /// <param name="buffers">The result mesh buffers.</param>
    /// <param name="layouts">The result layouts of the vertex buffers.</param>
    /// <param name="forceGpu">If set to <c>true</c> the data will be downloaded from the GPU, otherwise it can be loaded from the drive (source asset file) or from memory (if cached). Downloading mesh from GPU requires this call to be made from the other thread than main thread. Virtual assets are always downloaded from GPU memory due to lack of dedicated storage container for the asset data.</param>
    /// <returns>True if failed, otherwise false</returns>
    bool DownloadData(Span<MeshBufferType> types, Array<BytesContainer, FixedAllocation<4>>& buffers, Array<GPUVertexLayout*, FixedAllocation<4>>& layouts, bool forceGpu = false) const;

public:
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
        /// The instance deformation utility.
        /// </summary>
        MeshDeformation* Deformation;

        union
        {
            struct
            {
                /// <summary>
                /// The skinning.
                /// </summary>
                SkinnedMeshDrawData* Skinning;
            };

            struct
            {
                /// <summary>
                /// The lightmap.
                /// </summary>
                const Lightmap* Lightmap;

                /// <summary>
                /// The lightmap UVs.
                /// </summary>
                const Rectangle* LightmapUVs;
            };
        };

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

        /// <summary>
        /// The object sorting key.
        /// </summary>
        int8 SortOrder;

#if USE_EDITOR
        float LightmapScale = -1.0f;
#endif
    };

    /// <summary>
    /// Gets the draw call geometry for this mesh. Sets the index and vertex buffers.
    /// </summary>
    /// <param name="drawCall">The draw call.</param>
    void GetDrawCallGeometry(struct DrawCall& drawCall) const;

    /// <summary>
    /// Draws the mesh. Binds vertex and index buffers and invokes the draw call.
    /// </summary>
    /// <param name="context">The GPU context.</param>
    void Render(GPUContext* context) const;

private:
    // Internal bindings
    API_FUNCTION(NoProxy) ScriptingObject* GetParentModel() const;
#if !COMPILE_WITHOUT_CSHARP
    API_FUNCTION(NoProxy) bool UpdateTrianglesUInt(int32 triangleCount, const MArray* trianglesObj);
    API_FUNCTION(NoProxy) bool UpdateTrianglesUShort(int32 triangleCount, const MArray* trianglesObj);
    API_FUNCTION(NoProxy) MArray* DownloadIndexBuffer(bool forceGpu, MTypeObject* resultType, bool use16Bit);
    API_FUNCTION(NoProxy) bool DownloadData(int32 count, MeshBufferType* types, API_PARAM(Out) BytesContainer& buffer0, API_PARAM(Out) BytesContainer& buffer1, API_PARAM(Out) BytesContainer& buffer2, API_PARAM(Out) BytesContainer& buffer3, API_PARAM(Out) GPUVertexLayout*& layout0, API_PARAM(Out) GPUVertexLayout*& layout1, API_PARAM(Out) GPUVertexLayout*& layout2, API_PARAM(Out) GPUVertexLayout*& layout3, bool forceGpu) const;
#endif
};
