// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/JsonAssetReference.h"
#include "Engine/Content/Assets/MaterialBase.h"
#include "Engine/Physics/Actors/PhysicsColliderActor.h"
#include "Engine/Physics/Actors/IPhysicsDebug.h"

class Terrain;
class TerrainChunk;
class TerrainPatch;
class TerrainManager;
class PhysicalMaterial;
struct RayCastHit;
struct RenderView;

// Maximum amount of levels of detail for the terrain chunks
#define TERRAIN_MAX_LODS 8

// Amount of units per terrain geometry vertex (can be adjusted per terrain instance using non-uniform scale factor)
#define TERRAIN_UNITS_PER_VERTEX 100.0f

// Enable/disable terrain editing and changing at runtime. If your game doesn't use procedural terrain in game then disable this option to reduce build size.
#define TERRAIN_EDITING 1

// Enable/disable terrain heightmap samples modification and gather. Used by the editor to modify the terrain with the brushes.
#define TERRAIN_UPDATING 1

// Enable/disable terrain physics collision drawing
#define TERRAIN_USE_PHYSICS_DEBUG (USE_EDITOR && 1)

// Terrain splatmaps amount limit. Each splatmap can hold up to 4 layer weights.
#define TERRAIN_MAX_SPLATMAPS_COUNT 2

/// <summary>
/// Represents a single terrain object.
/// </summary>
/// <seealso cref="Actor" />
/// <seealso cref="PhysicsColliderActor" />
API_CLASS(Sealed) class FLAXENGINE_API Terrain : public PhysicsColliderActor
#if USE_EDITOR
    , public IPhysicsDebug
#endif
{
    DECLARE_SCENE_OBJECT(Terrain);
    friend Terrain;
    friend TerrainPatch;
    friend TerrainChunk;

    /// <summary>
    /// Various defines regarding terrain configuration.
    /// </summary>
    API_ENUM() enum Config
    {
        /// <summary>
        /// The maximum allowed amount of chunks per patch.
        /// </summary>
        ChunksCount = 16,

        /// <summary>
        /// The maximum allowed amount of chunks per chunk.
        /// </summary>
        ChunksCountEdge = 4,
    };

private:
    char _lodBias;
    char _forcedLod;
    char _collisionLod;
    byte _lodCount;
    uint16 _chunkSize;
    int32 _sceneRenderingKey = -1;
    float _scaleInLightmap;
    float _lodDistribution;
    Vector3 _boundsExtent;
    Float3 _cachedScale;
    Array<TerrainPatch*, InlinedAllocation<64>> _patches;
    Array<JsonAssetReference<PhysicalMaterial>, FixedAllocation<8>> _physicalMaterials;

public:
    /// <summary>
    /// Finalizes an instance of the <see cref="Terrain"/> class.
    /// </summary>
    ~Terrain();

public:
    /// <summary>
    /// The default material used for terrain rendering (chunks can override this).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(100), DefaultValue(null), EditorDisplay(\"Terrain\")")
    AssetReference<MaterialBase> Material;

    /// <summary>
    /// The draw passes to use for rendering this object.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(115), DefaultValue(DrawPass.Default), EditorDisplay(\"Terrain\")")
    DrawPass DrawModes = DrawPass::Default;

public:
    /// <summary>
    /// Gets the terrain Level Of Detail bias value. Allows to increase or decrease rendered terrain quality.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(50), DefaultValue(0), Limit(-100, 100, 0.1f), EditorDisplay(\"Terrain\", \"LOD Bias\")")
    FORCE_INLINE int32 GetLODBias() const
    {
        return static_cast<int32>(_lodBias);
    }

    /// <summary>
    /// Sets the terrain Level Of Detail bias value. Allows to increase or decrease rendered terrain quality.
    /// </summary>
    API_PROPERTY() void SetLODBias(int32 value)
    {
        _lodBias = static_cast<char>(Math::Clamp(value, -100, 100));
    }

    /// <summary>
    /// Gets the terrain forced Level Of Detail index. Allows to bind the given chunks LOD to show. Value -1 disables this feature.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(60), DefaultValue(-1), Limit(-1, 100, 0.1f), EditorDisplay(\"Terrain\", \"Forced LOD\")")
    FORCE_INLINE int32 GetForcedLOD() const
    {
        return static_cast<int32>(_forcedLod);
    }

    /// <summary>
    /// Sets the terrain forced Level Of Detail index. Allows to bind the given chunks LOD to show. Value -1 disables this feature.
    /// </summary>
    API_PROPERTY() void SetForcedLOD(int32 value)
    {
        _forcedLod = static_cast<char>(Math::Clamp(value, -1, TERRAIN_MAX_LODS));
    }

    /// <summary>
    /// Gets the terrain LODs distribution parameter. Adjusts terrain chunks transitions distances. Use lower value to increase terrain quality or higher value to increase performance.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(70), DefaultValue(0.6f), Limit(0, 5, 0.01f), EditorDisplay(\"Terrain\", \"LOD Distribution\")")
    FORCE_INLINE float GetLODDistribution() const
    {
        return _lodDistribution;
    }

    /// <summary>
    /// Sets the terrain LODs distribution parameter. Adjusts terrain chunks transitions distances. Use lower value to increase terrain quality or higher value to increase performance.
    /// </summary>
    API_PROPERTY() void SetLODDistribution(float value);

    /// <summary>
    /// Gets the terrain scale in lightmap (applied to all the chunks). Use value higher than 1 to increase baked lighting resolution.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(110), DefaultValue(0.1f), Limit(0, 10000, 0.1f), EditorDisplay(\"Terrain\", \"Scale In Lightmap\")")
    FORCE_INLINE float GetScaleInLightmap() const
    {
        return _scaleInLightmap;
    }

    /// <summary>
    /// Sets the terrain scale in lightmap (applied to all the chunks). Use value higher than 1 to increase baked lighting resolution.
    /// </summary>
    API_PROPERTY() void SetScaleInLightmap(float value);

    /// <summary>
    /// Gets the terrain chunks bounds extent. Values used to expand terrain chunks bounding boxes. Use it when your terrain material is performing vertex offset operations on a GPU.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(120), EditorDisplay(\"Terrain\")")
    FORCE_INLINE Vector3 GetBoundsExtent() const
    {
        return _boundsExtent;
    }

    /// <summary>
    /// Sets the terrain chunks bounds extent. Values used to expand terrain chunks bounding boxes. Use it when your terrain material is performing vertex offset operations on a GPU.
    /// </summary>
    API_PROPERTY() void SetBoundsExtent(const Vector3& value);

    /// <summary>
    /// Gets the terrain geometry LOD index used for collision.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(500), DefaultValue(-1), Limit(-1, 100, 0.1f), EditorDisplay(\"Collision\", \"Collision LOD\")")
    FORCE_INLINE int32 GetCollisionLOD() const
    {
        return _collisionLod;
    }

    /// <summary>
    /// Sets the terrain geometry LOD index used for collision.
    /// </summary>
    API_PROPERTY() void SetCollisionLOD(int32 value);

    /// <summary>
    /// Gets the list with physical materials used to define the terrain collider physical properties - each for terrain layer (layer index matches index in this array).
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(520), EditorDisplay(\"Collision\"), Collection(MinCount=8, MaxCount=8)")
    FORCE_INLINE const Array<JsonAssetReference<PhysicalMaterial>, FixedAllocation<8>>& GetPhysicalMaterials() const
    {
        return _physicalMaterials;
    }

    /// <summary>
    /// Sets the list with physical materials used to define the terrain collider physical properties - each for terrain layer (layer index matches index in this array).
    /// </summary>
    API_PROPERTY()
    void SetPhysicalMaterials(const Array<JsonAssetReference<PhysicalMaterial>, FixedAllocation<8>>& value);

    /// <summary>
    /// Gets the physical material used to define the terrain collider physical properties.
    /// [Deprecated on 16.02.2024, expires on 16.02.2026]
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoSerialize")
    DEPRECATED("Use PhysicalMaterials instead.") FORCE_INLINE JsonAssetReference<PhysicalMaterial>& GetPhysicalMaterial()
    {
        return _physicalMaterials[0];
    }

    /// <summary>
    /// Sets the physical materials used to define the terrain collider physical properties.
    /// [Deprecated on 16.02.2024, expires on 16.02.2026]
    /// </summary>
    DEPRECATED("Use PhysicalMaterials instead.") API_PROPERTY()
    void SetPhysicalMaterial(const JsonAssetReference<PhysicalMaterial>& value)
    {
        for (auto& e : _physicalMaterials)
            e = value;
    }

    /// <summary>
    /// Gets the terrain Level Of Detail count.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetLODCount() const
    {
        return static_cast<int32>(_lodCount);
    }

    /// <summary>
    /// Gets the terrain chunk vertices amount per edge (square).
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetChunkSize() const
    {
        return static_cast<int32>(_chunkSize);
    }

    /// <summary>
    /// Gets the terrain patches count. Each patch contains 16 chunks arranged into a 4x4 square.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetPatchesCount() const
    {
        return _patches.Count();
    }

    /// <summary>
    /// Checks if terrain has the patch at the given coordinates.
    /// </summary>
    /// <param name="patchCoord">The patch location (x and z).</param>
    /// <returns>True if has patch added, otherwise false.</returns>
    API_FUNCTION() FORCE_INLINE bool HasPatch(API_PARAM(Ref) const Int2& patchCoord) const
    {
        return GetPatch(patchCoord) != nullptr;
    }

    /// <summary>
    /// Gets the patch at the given location.
    /// </summary>
    /// <param name="patchCoord">The patch location (x and z).</param>
    /// <returns>The patch.</returns>
    API_FUNCTION() TerrainPatch* GetPatch(API_PARAM(Ref) const Int2& patchCoord) const;

    /// <summary>
    /// Gets the patch at the given location.
    /// </summary>
    /// <param name="x">The patch location x.</param>
    /// <param name="z">The patch location z.</param>
    /// <returns>The patch.</returns>
    API_FUNCTION() TerrainPatch* GetPatch(int32 x, int32 z) const;

    /// <summary>
    /// Gets the zero-based index of the terrain patch in the terrain patches collection.
    /// </summary>
    /// <param name="patchCoord">The patch location (x and z).</param>
    /// <returns>The zero-based index of the terrain patch in the terrain patches collection. Returns -1 if patch coordinates are invalid.</returns>
    API_FUNCTION() int32 GetPatchIndex(API_PARAM(Ref) const Int2& patchCoord) const;

    /// <summary>
    /// Gets the patch at the given index.
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns>The patch.</returns>
    API_FUNCTION() FORCE_INLINE TerrainPatch* GetPatch(int32 index) const
    {
        return _patches[index];
    }

    /// <summary>
    /// Gets the terrain patch coordinates (x and z) at the given index.
    /// </summary>
    /// <param name="patchIndex">The zero-based index of the terrain patch in the terrain patches collection.</param>
    /// <param name="patchCoord">The patch location (x and z).</param>
    API_FUNCTION() void GetPatchCoord(int32 patchIndex, API_PARAM(Out) Int2& patchCoord) const;

    /// <summary>
    /// Gets the terrain patch world bounds at the given index.
    /// </summary>
    /// <param name="patchIndex">The zero-based index of the terrain patch in the terrain patches collection.</param>
    /// <param name="bounds">The patch world bounds.</param>
    API_FUNCTION() void GetPatchBounds(int32 patchIndex, API_PARAM(Out) BoundingBox& bounds) const;

    /// <summary>
    /// Gets the terrain chunk world bounds at the given index.
    /// </summary>
    /// <param name="patchIndex">The zero-based index of the terrain patch in the terrain patches collection.</param>
    /// <param name="chunkIndex">The zero-based index of the terrain chunk in the terrain patch chunks collection (in range 0-15).</param>
    /// <param name="bounds">The chunk world bounds.</param>
    API_FUNCTION() void GetChunkBounds(int32 patchIndex, int32 chunkIndex, API_PARAM(Out) BoundingBox& bounds) const;

    /// <summary>
    /// Gets the chunk material that overrides the terrain default one.
    /// </summary>
    /// <param name="patchCoord">The patch coordinates (x and z).</param>
    /// <param name="chunkCoord">The chunk coordinates (x and z).</param>
    /// <returns>The value.</returns>
    API_FUNCTION() MaterialBase* GetChunkOverrideMaterial(API_PARAM(Ref) const Int2& patchCoord, API_PARAM(Ref) const Int2& chunkCoord) const;

    /// <summary>
    /// Sets the chunk material to override the terrain default one.
    /// </summary>
    /// <param name="patchCoord">The patch coordinates (x and z).</param>
    /// <param name="chunkCoord">The chunk coordinates (x and z).</param>
    /// <param name="value">The value to set.</param>
    API_FUNCTION() void SetChunkOverrideMaterial(API_PARAM(Ref) const Int2& patchCoord, API_PARAM(Ref) const Int2& chunkCoord, MaterialBase* value);

#if TERRAIN_EDITING

    /// <summary>
    /// Setups the terrain patch using the specified heightmap data.
    /// </summary>
    /// <param name="patchCoord">The patch coordinates (x and z).</param>
    /// <param name="heightMapLength">The height map array length. It must match the terrain descriptor, so it should be (chunkSize*4+1)^2. Patch is a 4 by 4 square made of chunks. Each chunk has chunkSize quads on edge.</param>
    /// <param name="heightMap">The height map. Each array item contains a height value.</param>
    /// <param name="holesMask">The holes mask (optional). Normalized to 0-1 range values with holes mask per-vertex. Must match the heightmap dimensions.</param>
    /// <param name="forceUseVirtualStorage">If set to <c>true</c> patch will use virtual storage by force. Otherwise it can use normal texture asset storage on drive (valid only during Editor). Runtime-created terrain can only use virtual storage (in RAM).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool SetupPatchHeightMap(API_PARAM(Ref) const Int2& patchCoord, int32 heightMapLength, const float* heightMap, const byte* holesMask = nullptr, bool forceUseVirtualStorage = false);

    /// <summary>
    /// Setups the terrain patch layer weights using the specified splatmaps data.
    /// </summary>
    /// <param name="patchCoord">The patch coordinates (x and z).</param>
    /// <param name="index">The zero-based index of the splatmap texture.</param>
    /// <param name="splatMapLength">The splatmap map array length. It must match the terrain descriptor, so it should be (chunkSize*4+1)^2. Patch is a 4 by 4 square made of chunks. Each chunk has chunkSize quads on edge.</param>
    /// <param name="splatMap">The splat map. Each array item contains 4 layer weights.</param>
    /// <param name="forceUseVirtualStorage">If set to <c>true</c> patch will use virtual storage by force. Otherwise it can use normal texture asset storage on drive (valid only during Editor). Runtime-created terrain can only use virtual storage (in RAM).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool SetupPatchSplatMap(API_PARAM(Ref) const Int2& patchCoord, int32 index, int32 splatMapLength, const Color32* splatMap, bool forceUseVirtualStorage = false);

#endif

public:
#if TERRAIN_EDITING
    /// <summary>
    /// Setups the terrain. Clears the existing data.
    /// </summary>
    /// <param name="lodCount">The LODs count. The actual amount of LODs may be lower due to provided chunk size (each LOD has 4 times less quads).</param>
    /// <param name="chunkSize">The size of the chunk (amount of quads per edge for the highest LOD). Must be power of two minus one (eg. 63 or 127).</param>
    API_FUNCTION() void Setup(int32 lodCount = 6, int32 chunkSize = 127);

    /// <summary>
    /// Adds the patches to the terrain (clears existing ones).
    /// </summary>
    /// <param name="numberOfPatches">The number of patches (x and z axis).</param>
    API_FUNCTION() void AddPatches(API_PARAM(Ref) const Int2& numberOfPatches);

    /// <summary>
    /// Adds the patch.
    /// </summary>
    /// <param name="patchCoord">The patch location (x and z).</param>
    API_FUNCTION() void AddPatch(API_PARAM(Ref) const Int2& patchCoord);

    /// <summary>
    /// Removes the patch.
    /// </summary>
    /// <param name="patchCoord">The patch location (x and z).</param>
    API_FUNCTION() void RemovePatch(API_PARAM(Ref) const Int2& patchCoord);
#endif

    /// <summary>
    /// Updates the cached bounds of the actor. Updates the cached world bounds for every patch and chunk.
    /// </summary>
    void UpdateBounds();

    /// <summary>
    /// Caches the neighbor chunks of this terrain.
    /// </summary>
    void CacheNeighbors();

    /// <summary>
    /// Updates the collider shapes collisions/queries layer mask bits.
    /// </summary>
    void UpdateLayerBits();

    /// <summary>
    /// Removes the lightmap data from the terrain.
    /// </summary>
    void RemoveLightmap();

public:
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
    /// Performs a raycast against this terrain collision shape. Returns the hit chunk.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="resultHitDistance">The raycast result hit position distance from the ray origin. Valid only if raycast hits anything.</param>
    /// <param name="resultPatchCoord">The raycast result hit terrain patch coordinates (x and z). Valid only if raycast hits anything.</param>
    /// <param name="resultChunkCoord">The raycast result hit terrain chunk coordinates (relative to the patch, x and z). Valid only if raycast hits anything.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <returns>True if ray hits an object, otherwise false.</returns>
    API_FUNCTION() bool RayCast(const Ray& ray, API_PARAM(Out) float& resultHitDistance, API_PARAM(Out) Int2& resultPatchCoord, API_PARAM(Out) Int2& resultChunkCoord, float maxDistance = MAX_float) const;

    /// <summary>
    /// Draws the terrain patch.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="patchCoord">The patch location (x and z).</param>
    /// <param name="material">The material to use for rendering.</param>
    /// <param name="lodIndex">The LOD index.</param>
    API_FUNCTION() void DrawPatch(API_PARAM(Ref) const RenderContext& renderContext, API_PARAM(Ref) const Int2& patchCoord, MaterialBase* material, int32 lodIndex = 0) const;

    /// <summary>
    /// Draws the terrain chunk.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="patchCoord">The patch location (x and z).</param>
    /// <param name="chunkCoord">The chunk location (x and z).</param>
    /// <param name="material">The material to use for rendering.</param>
    /// <param name="lodIndex">The LOD index.</param>
    API_FUNCTION() void DrawChunk(API_PARAM(Ref) const RenderContext& renderContext, API_PARAM(Ref) const Int2& patchCoord, API_PARAM(Ref) const Int2& chunkCoord, MaterialBase* material, int32 lodIndex = 0) const;

private:
    ImplementPhysicsDebug;
    bool DrawSetup(RenderContext& renderContext);
    void DrawImpl(RenderContext& renderContext, HashSet<TerrainChunk*, class RendererAllocation>& drawnChunks);

public:
    // [PhysicsColliderActor]
    void Draw(RenderContextBatch& renderContextBatch) override;
    void Draw(RenderContext& renderContext) override;
#if USE_EDITOR
    void OnDebugDrawSelected() override;
#endif
    void OnLayerChanged() override;
    bool IntersectsItself(const Ray& ray, Real& distance, Vector3& normal) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    RigidBody* GetAttachedRigidBody() const override;
    bool RayCast(const Vector3& origin, const Vector3& direction, float& resultHitDistance, float maxDistance = MAX_float) const final;
    bool RayCast(const Vector3& origin, const Vector3& direction, RayCastHit& hitInfo, float maxDistance = MAX_float) const final;
    void ClosestPoint(const Vector3& point, Vector3& result) const final;
    bool ContainsPoint(const Vector3& point) const final;

protected:
    // [PhysicsColliderActor]
    void OnEnable() override;
    void OnDisable() override;
    void OnTransformChanged() override;
    void OnActiveInTreeChanged() override;
    void OnPhysicsSceneChanged(PhysicsScene* previous) override;
    void BeginPlay(SceneBeginData* data) override;
    void EndPlay() override;
};
