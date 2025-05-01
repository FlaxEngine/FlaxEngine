// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/BinaryAsset.h"
#include "Engine/Core/Math/BoundingBox.h"

class Model;
class ModelBase;
class ModelData;
class MeshBase;

/// <summary>
/// A <see cref="CollisionData"/> storage data type.
/// </summary>
API_ENUM() enum class CollisionDataType
{
    /// <summary>
    /// Nothing.
    /// </summary>
    None = 0,

    /// <summary>
    /// A convex polyhedron represented as a set of vertices and polygonal faces. The number of vertices and faces of a convex mesh is limited to 255.
    /// </summary>
    ConvexMesh = 1,

    /// <summary>
    /// A collision triangle mesh consists of a collection of vertices and the triangle indices.
    /// </summary>
    TriangleMesh = 2,
};

/// <summary>
/// Set of flags used to generate model convex mesh. Allows to customize process.
/// </summary>
API_ENUM(Attributes="Flags") enum class ConvexMeshGenerationFlags
{
    /// <summary>
    /// Nothing.
    /// </summary>
    None = 0,

    /// <summary>
    /// Disables the convex mesh validation to speed-up hull creation. Creating a convex mesh with invalid input data without prior validation may result in undefined behavior.
    /// </summary>
    SkipValidation = 1,

    /// <summary>
    /// Enables plane shifting vertex limit algorithm. 
    ///
    /// Plane shifting is an alternative algorithm for the case when the computed hull has more vertices
    /// than the specified vertex limit. 
    /// 
    /// The default algorithm computes the full hull, and an OBB around the input vertices. This OBB is then sliced
    /// with the hull planes until the vertex limit is reached. The default algorithm requires the vertex limit
    /// to be set to at least 8, and typically produces results that are much better quality than are produced
    /// by plane shifting. 
    /// 
    /// When plane shifting is enabled, the hull computation stops when vertex limit is reached.The hull planes
    /// are then shifted to contain all input vertices, and the new plane intersection points are then used to
    /// generate the final hull with the given vertex limit.Plane shifting may produce sharp edges to vertices
    /// very far away from the input cloud, and does not guarantee that all input vertices are inside the resulting
    /// hull. However, it can be used with a vertex limit as low as 4.
    /// </summary>
    UsePlaneShifting = 2,

    /// <summary>
    /// Inertia tensor computation is faster using SIMD code, but the precision is lower, which may result in incorrect inertia for very thin hulls.
    /// </summary>
    UseFastInteriaComputation = 4,

    /// <summary>
    /// Convex hull input vertices are shifted to be around origin to provide better computation stability.
    /// It is recommended to provide input vertices around the origin, otherwise use this flag to improve numerical stability.
    /// </summary>
    ShiftVertices = 8,

    /// <summary>
    /// If checked, the face remap table is not created. This saves a significant amount of memory, but disabled ability to remap the cooked collision geometry into original mesh using raycast hit info.
    /// </summary>
    SuppressFaceRemapTable = 16,

    /// <summary>
    /// The combination of flags that improve the collision data cooking performance at the cost of quality and features. Recommend for runtime dynamic or deformable objects that need quick collision updates.
    /// </summary>
    FastCook = SkipValidation | UseFastInteriaComputation | SuppressFaceRemapTable,
};

DECLARE_ENUM_OPERATORS(ConvexMeshGenerationFlags);

/// <summary>
/// The collision data asset cooking options.
/// </summary>
API_STRUCT() struct CollisionDataOptions
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(CollisionDataOptions);

    /// <summary>
    /// The data type.
    /// </summary>
    API_FIELD() CollisionDataType Type = CollisionDataType::None;

    /// <summary>
    /// The source model asset id.
    /// </summary>
    API_FIELD() Guid Model = Guid::Empty;

    /// <summary>
    /// The source model LOD index.
    /// </summary>
    API_FIELD() int32 ModelLodIndex = 0;

    /// <summary>
    /// The cooked collision bounds.
    /// </summary>
    API_FIELD() BoundingBox Box = BoundingBox::Zero;

    /// <summary>
    /// The convex generation flags.
    /// </summary>
    API_FIELD() ConvexMeshGenerationFlags ConvexFlags = ConvexMeshGenerationFlags::None;

    /// <summary>
    /// The convex vertices limit (maximum amount).
    /// </summary>
    API_FIELD() int32 ConvexVertexLimit = 0;

    /// <summary>
    /// The source model material slots mask. One bit per-slot. Can be used to exclude particular material slots from collision cooking.
    /// </summary>
    API_FIELD() uint32 MaterialSlotsMask = MAX_uint32;
};

/// <summary>
/// Represents a physics mesh that can be used with a MeshCollider. Physics mesh can be a generic triangle mesh or a convex mesh.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API CollisionData : public BinaryAsset
{
    DECLARE_BINARY_ASSET_HEADER(CollisionData, 1);
public:
    /// <summary>
    /// A raw structure stored in the binary asset. It has fixed size so it's easier to add new parameters to it. It's loaded and changed into Options structure used at runtime.
    /// </summary>
    struct SerializedOptions
    {
        CollisionDataType Type;
        Guid Model;
        int32 ModelLodIndex;
        ConvexMeshGenerationFlags ConvexFlags;
        int32 ConvexVertexLimit;
        uint32 MaterialSlotsMask;
        byte Padding[92];
    };

    static_assert(sizeof(SerializedOptions) == 128, "Invalid collision data options size. Change the padding.");

private:
    CollisionDataOptions _options;
    void* _convexMesh;
    void* _triangleMesh;

public:
    /// <summary>
    /// Gets the options.
    /// </summary>
    API_PROPERTY() FORCE_INLINE const CollisionDataOptions& GetOptions() const
    {
        return _options;
    }

    /// <summary>
    /// Gets the convex mesh object (valid only if asset is loaded and has cooked convex data).
    /// </summary>
    FORCE_INLINE void* GetConvex() const
    {
        return _convexMesh;
    }

    /// <summary>
    /// Gets the triangle mesh object (valid only if asset is loaded and has cooked triangle data).
    /// </summary>
    FORCE_INLINE void* GetTriangle() const
    {
        return _triangleMesh;
    }

public:
#if COMPILE_WITH_PHYSICS_COOKING

    /// <summary>
    /// Cooks the mesh collision data and updates the virtual asset.
    /// </summary>
    /// <remarks>
    /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
    /// </remarks>
    /// <param name="type">The collision data type.</param>
    /// <param name="model">The source model. If model is virtual then this method cannot be called from the main thread.</param>
    /// <param name="modelLodIndex">The source model LOD index.</param>
    /// <param name="materialSlotsMask">The source model material slots mask. One bit per-slot. Can be used to exclude particular material slots from collision cooking.</param>
    /// <param name="convexFlags">The convex mesh generation flags.</param>
    /// <param name="convexVertexLimit">The convex mesh vertex limit. Use values in range [8;255]</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool CookCollision(CollisionDataType type, ModelBase* model, int32 modelLodIndex = 0, uint32 materialSlotsMask = MAX_uint32, ConvexMeshGenerationFlags convexFlags = ConvexMeshGenerationFlags::None, int32 convexVertexLimit = 255);

    /// <summary>
    /// Cooks the mesh collision data and updates the virtual asset. action cannot be performed on a main thread.
    /// </summary>
    /// <remarks>
    /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
    /// </remarks>
    /// <param name="type">The collision data type.</param>
    /// <param name="vertices">The source geometry vertex buffer with vertices positions. Cannot be empty.</param>
    /// <param name="triangles">The source data index buffer (triangles list). Uses 32-bit stride buffer. Cannot be empty. Length must be multiple of 3 (as 3 vertices build a triangle).</param>
    /// <param name="convexFlags">The convex mesh generation flags.</param>
    /// <param name="convexVertexLimit">The convex mesh vertex limit. Use values in range [8;255]</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool CookCollision(CollisionDataType type, const Span<Float3>& vertices, const Span<uint32>& triangles, ConvexMeshGenerationFlags convexFlags = ConvexMeshGenerationFlags::None, int32 convexVertexLimit = 255);

    /// <summary>
    /// Cooks the mesh collision data and updates the virtual asset. action cannot be performed on a main thread.
    /// </summary>
    /// <remarks>
    /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
    /// </remarks>
    /// <param name="type">The collision data type.</param>
    /// <param name="vertices">The source geometry vertex buffer with vertices positions. Cannot be empty.</param>
    /// <param name="triangles">The source data index buffer (triangles list). Uses 32-bit stride buffer. Cannot be empty. Length must be multiple of 3 (as 3 vertices build a triangle).</param>
    /// <param name="convexFlags">The convex mesh generation flags.</param>
    /// <param name="convexVertexLimit">The convex mesh vertex limit. Use values in range [8;255]</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool CookCollision(CollisionDataType type, const Span<Float3>& vertices, const Span<int32>& triangles, ConvexMeshGenerationFlags convexFlags = ConvexMeshGenerationFlags::None, int32 convexVertexLimit = 255);

    /// <summary>
    /// Cooks the mesh collision data and updates the virtual asset. action cannot be performed on a main thread.
    /// </summary>
    /// <remarks>
    /// Can be used only for virtual assets (see <see cref="Asset.IsVirtual"/> and <see cref="Content.CreateVirtualAsset{T}"/>).
    /// </remarks>
    /// <param name="type">The collision data type.</param>
    /// <param name="modelData">The custom geometry.</param>
    /// <param name="convexFlags">The convex mesh generation flags.</param>
    /// <param name="convexVertexLimit">The convex mesh vertex limit. Use values in range [8;255]</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool CookCollision(CollisionDataType type, ModelData* modelData, ConvexMeshGenerationFlags convexFlags, int32 convexVertexLimit);

#endif

    /// <summary>
    /// Extracts the triangle index of the original mesh data used for cooking this collision data. Can be used to get vertex attributes of the triangle mesh hit by the raycast.
    /// </summary>
    /// <remarks>Supported only for collision data built as triangle mesh and without <see cref="ConvexMeshGenerationFlags.SuppressFaceRemapTable"/> flag set.</remarks>
    /// <param name="faceIndex">The face index of the collision mesh.</param>
    /// <param name="mesh">The result source mesh used to build this collision data (can be null if collision data was cooked using custom geometry without source Model set).</param>
    /// <param name="meshTriangleIndex">The result triangle index of the source geometry used to build this collision data.</param>
    /// <returns>True if got a valid triangle index, otherwise false.</returns>
    API_FUNCTION() bool GetModelTriangle(uint32 faceIndex, API_PARAM(Out) MeshBase*& mesh, API_PARAM(Out) uint32& meshTriangleIndex) const;

    /// <summary>
    /// Extracts the collision data geometry into list of triangles.
    /// </summary>
    /// <param name="vertexBuffer">The output vertex buffer.</param>
    /// <param name="indexBuffer">The output index buffer.</param>
    API_FUNCTION() void ExtractGeometry(API_PARAM(Out) Array<Float3>& vertexBuffer, API_PARAM(Out) Array<int32>& indexBuffer) const;

public:
    // MeshCollider is drawing debug view of the collision data, allow to share it across instances
#if USE_EDITOR
private:
    bool _hasMissingDebugLines = true;
    Array<Float3> _debugLines;
    Array<Float3> _debugVertexBuffer;
    Array<int32> _debugIndexBuffer;
public:
    const Array<Float3>& GetDebugLines();
    void GetDebugTriangles(Array<Float3>*& vertexBuffer, Array<int32>*& indexBuffer);
#endif

private:
    LoadResult load(const SerializedOptions* options, byte* dataPtr, int32 dataSize);

protected:
    // [BinaryAsset]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;
};
