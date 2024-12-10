// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Terrain.h"
#include "TerrainChunk.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Level/Scene/Lightmap.h"
#include "Engine/Content/Assets/RawDataAsset.h"

struct RayCastHit;
class TerrainMaterialShader;

/// <summary>
/// Represents single terrain patch made of 16 terrain chunks.
/// </summary>
API_CLASS(Sealed, NoSpawn) class FLAXENGINE_API TerrainPatch : public ScriptingObject, public ISerializable
{
    DECLARE_SCRIPTING_TYPE(TerrainPatch);
    friend Terrain;
    friend TerrainPatch;
    friend TerrainChunk;

private:
    Terrain* _terrain;
    int16 _x, _z;
    float _yOffset, _yHeight;
    BoundingBox _bounds;
    Float3 _offset;
    AssetReference<RawDataAsset> _heightfield;
    void* _physicsShape;
    void* _physicsActor;
    void* _physicsHeightField;
    CriticalSection _collisionLocker;
    float _collisionScaleXZ;
#if TERRAIN_UPDATING
    Array<float> _cachedHeightMap;
    Array<byte> _cachedHolesMask;
    Array<Color32> _cachedSplatMap[TERRAIN_MAX_SPLATMAPS_COUNT];
    bool _wasHeightModified;
    bool _wasSplatmapModified[TERRAIN_MAX_SPLATMAPS_COUNT];
#if TERRAIN_USE_PHYSICS_DEBUG
    bool _debugLinesDirty = true;
    class GPUBuffer* _debugLines = nullptr;
#endif
    TextureBase::InitData* _dataHeightmap = nullptr;
    TextureBase::InitData* _dataSplatmap[TERRAIN_MAX_SPLATMAPS_COUNT] = {};
#endif
#if USE_EDITOR
    Array<Vector3> _collisionTriangles; // TODO: large-worlds
#endif
    Array<Float3> _collisionVertices; // TODO: large-worlds

    void Init(Terrain* terrain, int16 x, int16 z);

public:
    /// <summary>
    /// Finalizes an instance of the <see cref="TerrainPatch"/> class.
    /// </summary>
    ~TerrainPatch();

public:
    /// <summary>
    /// The chunks contained within the patch. Organized in 4x4 square.
    /// </summary>
    TerrainChunk Chunks[Terrain::ChunksCount];

    /// <summary>
    /// The heightmap texture.
    /// </summary>
    API_FIELD() AssetReference<Texture> Heightmap;

    /// <summary>
    /// The splatmap textures.
    /// </summary>
    AssetReference<Texture> Splatmap[TERRAIN_MAX_SPLATMAPS_COUNT];

public:
    /// <summary>
    /// Gets the Y axis heightmap offset from terrain origin.
    /// </summary>
    API_FUNCTION() FORCE_INLINE float GetOffsetY() const
    {
        return _yOffset;
    }

    /// <summary>
    /// Gets the Y axis heightmap height.
    /// </summary>
    API_FUNCTION() FORCE_INLINE float GetHeightY() const
    {
        return _yHeight;
    }

    /// <summary>
    /// Gets the x coordinate.
    /// </summary>
    API_FUNCTION() FORCE_INLINE int32 GetX() const
    {
        return _x;
    }

    /// <summary>
    /// Gets the z coordinate.
    /// </summary>
    API_FUNCTION() FORCE_INLINE int32 GetZ() const
    {
        return _z;
    }

    /// <summary>
    /// Gets the terrain.
    /// </summary>
    API_FUNCTION() FORCE_INLINE Terrain* GetTerrain() const
    {
        return _terrain;
    }

    /// <summary>
    /// Gets the chunk at the given index.
    /// </summary>
    /// <param name="index">The chunk zero-based index.</param>
    /// <returns>The chunk.</returns>
    API_FUNCTION() TerrainChunk* GetChunk(int32 index)
    {
        if (index < 0 || index >= Terrain::ChunksCount)
            return nullptr;
        return &Chunks[index];
    }

    /// <summary>
    /// Gets the chunk at the given location.
    /// </summary>
    /// <param name="chunkCoord">The chunk location (x and z).</param>
    /// <returns>The chunk.</returns>
    API_FUNCTION() TerrainChunk* GetChunk(API_PARAM(Ref) const Int2& chunkCoord)
    {
        return GetChunk(chunkCoord.Y * Terrain::ChunksCountEdge + chunkCoord.X);
    }

    /// <summary>
    /// Gets the chunk at the given location.
    /// </summary>
    /// <param name="x">The chunk location x.</param>
    /// <param name="z">The chunk location z.</param>
    /// <returns>The chunk.</returns>
    API_FUNCTION() TerrainChunk* GetChunk(int32 x, int32 z)
    {
        return GetChunk(z * Terrain::ChunksCountEdge + x);
    }

    /// <summary>
    /// Gets the heightfield collision data asset.
    /// </summary>
    /// <returns>The heightfield data asset.</returns>
    API_PROPERTY() RawDataAsset* GetHeightfield() const;

    /// <summary>
    /// Gets the splatmap assigned to this patch.
    /// </summary>
    /// <param name="index">The zero-based index of the splatmap.</param>
    /// <returns>The splatmap texture.</returns>
    API_FUNCTION() AssetReference<Texture> GetSplatmap(int32 index)
    {
        if (index < 0 || index >= TERRAIN_MAX_SPLATMAPS_COUNT)
            return nullptr;
        return Splatmap[index];
    }

    /// <summary>
    /// Sets a splatmap to this patch.
    /// </summary>
    /// <param name="index">The zero-based index of the splatmap.</param>
    /// <param name="splatMap">Splatmap texture.</param>
    API_FUNCTION() void SetSplatmap(int32 index, const AssetReference<Texture>& splatMap)
    {
        if (index >= 0 && index < TERRAIN_MAX_SPLATMAPS_COUNT)
            Splatmap[index] = splatMap;
    }

    /// <summary>
    /// Gets the patch world bounds.
    /// </summary>
    API_FUNCTION() FORCE_INLINE const BoundingBox& GetBounds() const
    {
        return _bounds;
    }

public:
    /// <summary>
    /// Removes the lightmap data from the terrain patch.
    /// </summary>
    void RemoveLightmap();

    /// <summary>
    /// Updates the cached bounds of the patch and child chunks.
    /// </summary>
    void UpdateBounds();

    /// <summary>
    /// Updates the cached transform of the patch and child chunks.
    /// </summary>
    void UpdateTransform();

#if TERRAIN_EDITING
    /// <summary>
    /// Initializes the patch heightmap and collision to the default flat level.
    /// </summary>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool InitializeHeightMap();

    /// <summary>
    /// Setups the terrain patch using the specified heightmap data.
    /// </summary>
    /// <param name="heightMapLength">The height map array length. It must match the terrain descriptor, so it should be (chunkSize*4+1)^2. Patch is a 4 by 4 square made of chunks. Each chunk has chunkSize quads on edge.</param>
    /// <param name="heightMap">The height map. Each array item contains a height value.</param>
    /// <param name="holesMask">The holes mask (optional). Normalized to 0-1 range values with holes mask per-vertex. Must match the heightmap dimensions.</param>
    /// <param name="forceUseVirtualStorage">If set to <c>true</c> patch will use virtual storage by force. Otherwise it can use normal texture asset storage on drive (valid only during Editor). Runtime-created terrain can only use virtual storage (in RAM).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool SetupHeightMap(int32 heightMapLength, API_PARAM(Ref) const float* heightMap, API_PARAM(Ref) const byte* holesMask = nullptr, bool forceUseVirtualStorage = false);

    /// <summary>
    /// Setups the terrain patch layer weights using the specified splatmaps data.
    /// </summary>
    /// <param name="index">The zero-based index of the splatmap texture.</param>
    /// <param name="splatMapLength">The splatmap map array length. It must match the terrain descriptor, so it should be (chunkSize*4+1)^2. Patch is a 4 by 4 square made of chunks. Each chunk has chunkSize quads on edge.</param>
    /// <param name="splatMap">The splat map. Each array item contains 4 layer weights.</param>
    /// <param name="forceUseVirtualStorage">If set to <c>true</c> patch will use virtual storage by force. Otherwise it can use normal texture asset storage on drive (valid only during Editor). Runtime-created terrain can only use virtual storage (in RAM).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool SetupSplatMap(int32 index, int32 splatMapLength, API_PARAM(Ref) const Color32* splatMap, bool forceUseVirtualStorage = false);
#endif

#if TERRAIN_UPDATING
    /// <summary>
    /// Gets the raw pointer to the heightmap data.
    /// </summary>
    /// <returns>The heightmap data.</returns>
    API_FUNCTION() float* GetHeightmapData();

    /// <summary>
    /// Clears cache of the heightmap data.
    /// </summary>
    API_FUNCTION() void ClearHeightmapCache();

    /// <summary>
    /// Gets the raw pointer to the holes mask data.
    /// </summary>
    /// <returns>The holes mask data.</returns>
    API_FUNCTION() byte* GetHolesMaskData();

    /// <summary>
    /// Clears cache of the holes mask data.
    /// </summary>
    API_FUNCTION() void ClearHolesMaskCache();

    /// <summary>
    /// Gets the raw pointer to the splat map data.
    /// </summary>
    /// <param name="index">The zero-based index of the splatmap texture.</param>
    /// <returns>The splat map data.</returns>
    API_FUNCTION() Color32* GetSplatMapData(int32 index);

    /// <summary>
    /// Clears cache of the splat map data.
    /// </summary>
    API_FUNCTION() void ClearSplatMapCache();

    /// <summary>
    /// Clears all caches.
    /// </summary>
    API_FUNCTION() void ClearCache();

    /// <summary>
    /// Modifies the terrain patch heightmap with the given samples.
    /// </summary>
    /// <param name="samples">The samples. The array length is size.X*size.Y.</param>
    /// <param name="modifiedOffset">The offset from the first row and column of the heightmap data (offset destination x and z start position).</param>
    /// <param name="modifiedSize">The size of the heightmap to modify (x and z). Amount of samples in each direction.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool ModifyHeightMap(API_PARAM(Ref) const float* samples, API_PARAM(Ref) const Int2& modifiedOffset, API_PARAM(Ref) const Int2& modifiedSize);

    /// <summary>
    /// Modifies the terrain patch holes mask with the given samples.
    /// </summary>
    /// <param name="samples">The samples. The array length is size.X*size.Y.</param>
    /// <param name="modifiedOffset">The offset from the first row and column of the holes map data (offset destination x and z start position).</param>
    /// <param name="modifiedSize">The size of the holes map to modify (x and z). Amount of samples in each direction.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool ModifyHolesMask(API_PARAM(Ref) const byte* samples, API_PARAM(Ref) const Int2& modifiedOffset, API_PARAM(Ref) const Int2& modifiedSize);

    /// <summary>
    /// Modifies the terrain patch splat map (layers mask) with the given samples.
    /// </summary>
    /// <param name="index">The zero-based index of the splatmap texture.</param>
    /// <param name="samples">The samples. The array length is size.X*size.Y.</param>
    /// <param name="modifiedOffset">The offset from the first row and column of the splat map data (offset destination x and z start position).</param>
    /// <param name="modifiedSize">The size of the splat map to modify (x and z). Amount of samples in each direction.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool ModifySplatMap(int32 index, API_PARAM(Ref) const Color32* samples, API_PARAM(Ref) const Int2& modifiedOffset, API_PARAM(Ref) const Int2& modifiedSize);

private:
    bool UpdateHeightData(struct TerrainDataUpdateInfo& info, const Int2& modifiedOffset, const Int2& modifiedSize, bool wasHeightRangeChanged, bool wasHeightChanged);
    void SaveHeightData();
    void CacheHeightData();
    void SaveSplatData();
    void SaveSplatData(int32 index);
    void CacheSplatData();
#endif

public:
    /// <summary>
    /// Performs a raycast against this terrain collision shape.
    /// </summary>
    /// <param name="origin">The origin of the ray.</param>
    /// <param name="direction">The normalized direction of the ray.</param>
    /// <param name="resultHitDistance">The raycast result hit position distance from the ray origin. Valid only if raycast hits anything.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <returns>True if ray hits an object, otherwise false.</returns>
    API_FUNCTION() bool RayCast(const Vector3& origin, const Vector3& direction, API_PARAM(Out) float& resultHitDistance, float maxDistance = MAX_float) const;

    /// <summary>
    /// Performs a raycast against this terrain collision shape.
    /// </summary>
    /// <param name="origin">The origin of the ray.</param>
    /// <param name="direction">The normalized direction of the ray.</param>
    /// <param name="resultHitDistance">The raycast result hit position distance from the ray origin. Valid only if raycast hits anything.</param>
    /// <param name="resultHitNormal">The raycast result hit position normal vector. Valid only if raycast hits anything.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <returns>True if ray hits an object, otherwise false.</returns>
    API_FUNCTION() bool RayCast(const Vector3& origin, const Vector3& direction, API_PARAM(Out) float& resultHitDistance, API_PARAM(Out) Vector3& resultHitNormal, float maxDistance = MAX_float) const;

    /// <summary>
    /// Performs a raycast against this terrain collision shape. Returns the hit chunk.
    /// </summary>
    /// <param name="origin">The origin of the ray.</param>
    /// <param name="direction">The normalized direction of the ray.</param>
    /// <param name="resultHitDistance">The raycast result hit position distance from the ray origin. Valid only if raycast hits anything.</param>
    /// <param name="resultChunk">The raycast result hit chunk. Valid only if raycast hits anything.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <returns>True if ray hits an object, otherwise false.</returns>
    API_FUNCTION() bool RayCast(const Vector3& origin, const Vector3& direction, API_PARAM(Out) float& resultHitDistance, API_PARAM(Out) TerrainChunk*& resultChunk, float maxDistance = MAX_float) const;

    /// <summary>
    /// Performs a raycast against terrain collision, returns results in a RaycastHit structure.
    /// </summary>
    /// <param name="origin">The origin of the ray.</param>
    /// <param name="direction">The normalized direction of the ray.</param>
    /// <param name="hitInfo">The result hit information. Valid only when method returns true.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <returns>True if ray hits an object, otherwise false.</returns>
    API_FUNCTION() bool RayCast(const Vector3& origin, const Vector3& direction, API_PARAM(Out) RayCastHit& hitInfo, float maxDistance = MAX_float) const;

    /// <summary>
    /// Gets a point on the terrain collider that is closest to a given location. Can be used to find a hit location or position to apply explosion force or any other special effects.
    /// </summary>
    /// <param name="position">The position to find the closest point to it.</param>
    /// <param name="result">The result point on the collider that is closest to the specified location.</param>
    API_FUNCTION() void ClosestPoint(API_PARAM(Ref) const Vector3& position, API_PARAM(Out) Vector3& result) const;

#if USE_EDITOR
    /// <summary>
    /// Updates the patch data after manual deserialization called at runtime (eg. by editor undo).
    /// </summary>
    void UpdatePostManualDeserialization();
#endif

public:
#if USE_EDITOR
    /// <summary>
    /// Gets the collision mesh triangles array (3 vertices per triangle in linear list). Cached internally to reuse data.
    /// </summary>
    /// <returns>The collision triangles vertices list (in world-space).</returns>
    const Array<Vector3>& GetCollisionTriangles();

    /// <summary>
    /// Gets the collision mesh triangles array (3 vertices per triangle in linear list) that intersect with the given bounds.
    /// </summary>
    /// <param name="bounds">The world-space bounds to find terrain triangles that intersect with it.</param>
    /// <param name="result">The result triangles that intersect with the given bounds (in world-space).</param>
    void GetCollisionTriangles(API_PARAM(Ref) const BoundingSphere& bounds, API_PARAM(Out) Array<Vector3>& result);
#endif

    /// <summary>
    /// Extracts the collision data geometry into list of triangles.
    /// </summary>
    /// <param name="vertexBuffer">The output vertex buffer.</param>
    /// <param name="indexBuffer">The output index buffer.</param>
    API_FUNCTION() void ExtractCollisionGeometry(API_PARAM(Out) Array<Float3>& vertexBuffer, API_PARAM(Out) Array<int32>& indexBuffer);

private:
    /// <summary>
    /// Determines whether this patch has created collision representation.
    /// </summary>
    FORCE_INLINE bool HasCollision() const
    {
        return _physicsShape != nullptr;
    }

    /// <summary>
    /// Creates the collision object for the patch.
    /// </summary>
    void CreateCollision();

    /// <summary>
    /// Creates the height field from the collision data and caches height field XZ scale parameter.
    /// </summary>
    /// <returns>True if failed, otherwise false.</returns>
    bool CreateHeightField();

    /// <summary>
    /// Updates the collision geometry scale for the patch. Called when terrain actor scale gets changed.
    /// </summary>
    void UpdateCollisionScale() const;

    /// <summary>
    /// Deletes the collision object from the patch.
    /// </summary>
    void DestroyCollision();

#if TERRAIN_USE_PHYSICS_DEBUG
    void CacheDebugLines();
    void DrawPhysicsDebug(RenderView& view);
#endif

    /// <summary>
    /// Updates the collision heightfield.
    /// </summary>
    /// <returns>True if failed, otherwise false.</returns>
    bool UpdateCollision();

    void OnPhysicsSceneChanged(PhysicsScene* previous);

public:
    // [ISerializable]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
