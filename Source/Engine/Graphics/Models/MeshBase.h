// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Graphics/Enums.h"
#include "Engine/Graphics/Models/Types.h"
#include "Engine/Level/Types.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Config.h"
#if USE_PRECISE_MESH_INTERSECTS
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

protected:
    ModelBase* _model;
    BoundingBox _box;
    BoundingSphere _sphere;

    int32 _index;
    int32 _lodIndex;
    uint32 _vertices;
    uint32 _triangles;
    int32 _materialSlotIndex;
    bool _use16BitIndexBuffer;

    GPUBuffer* _vertexBuffers[3] = {};
    GPUBuffer* _indexBuffer = nullptr;

    mutable Array<byte> _cachedVertexBuffers[3];
    mutable Array<byte> _cachedIndexBuffer;
    mutable int32 _cachedIndexBufferCount;

#if USE_PRECISE_MESH_INTERSECTS
    CollisionProxy _collisionProxy;
#endif

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

#if USE_PRECISE_MESH_INTERSECTS
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
    void SetBounds(const BoundingBox& box);

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
    /// <param name="index">The bind slot index.</param>
    /// <returns>The buffer or null if not used.</returns>
    FORCE_INLINE GPUBuffer* GetVertexBuffer(int32 index) const
    {
        return _vertexBuffers[index];
    }

public:
    /// <summary>
    /// Unloads the mesh data (vertex buffers and cache). The opposite to Load.
    /// </summary>
    void Unload();

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
    /// <returns>True if failed, otherwise false</returns>
    bool DownloadDataGPU(MeshBufferType type, BytesContainer& result) const;

    /// <summary>
    /// Extracts mesh buffer data from GPU in the async task.
    /// </summary>
    /// <param name="type">Buffer type</param>
    /// <param name="result">The result data</param>
    /// <returns>Created async task used to gather the buffer data.</returns>
    Task* DownloadDataGPUAsync(MeshBufferType type, BytesContainer& result) const;

    /// <summary>
    /// Extract mesh buffer data from CPU. Cached internally.
    /// </summary>
    /// <param name="type">Buffer type</param>
    /// <param name="result">The result data</param>
    /// <param name="count">The amount of items inside the result buffer.</param>
    /// <returns>True if failed, otherwise false</returns>
    virtual bool DownloadDataCPU(MeshBufferType type, BytesContainer& result, int32& count) const = 0;

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
#endif
};
