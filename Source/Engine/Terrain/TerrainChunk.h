// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Serialization/ISerializable.h"
#include "Engine/Content/Assets/MaterialBase.h"
#include "Engine/Level/Scene/Lightmap.h"

class Terrain;
class TerrainPatch;
struct RenderContext;

/// <summary>
/// Represents a single terrain chunk.
/// </summary>
class FLAXENGINE_API TerrainChunk : public ISerializable
{
    friend Terrain;
    friend TerrainPatch;
    friend TerrainChunk;

private:

    TerrainPatch* _patch;
    uint16 _x, _z;
    Vector4 _heightmapUVScaleBias;
    Matrix _world;
    BoundingBox _bounds;
    Vector3 _boundsCenter;
    float _perInstanceRandom;
    float _yOffset, _yHeight;

    TerrainChunk* _neighbors[4];
    byte _cachedDrawLOD;
    IMaterial* _cachedDrawMaterial;

    void Init(TerrainPatch* patch, uint16 x, uint16 z);

public:

    /// <summary>
    /// The material to override the terrain default one for this chunk.
    /// </summary>
    AssetReference<MaterialBase> OverrideMaterial;

    /// <summary>
    /// The baked lightmap entry info for this chunk.
    /// </summary>
    LightmapEntry Lightmap;

public:

    /// <summary>
    /// Gets the x coordinate.
    /// </summary>
    /// <returns>The x position.</returns>
    FORCE_INLINE int32 GetX() const
    {
        return _x;
    }

    /// <summary>
    /// Gets the z coordinate.
    /// </summary>
    /// <returns>The z position.</returns>
    FORCE_INLINE int32 GetZ() const
    {
        return _z;
    }

    /// <summary>
    /// Gets the patch.
    /// </summary>
    /// <returns>The terrain patch,</returns>
    FORCE_INLINE TerrainPatch* GetPatch() const
    {
        return _patch;
    }

    /// <summary>
    /// Gets the chunk world bounds.
    /// </summary>
    /// <returns>The bounding box.</returns>
    FORCE_INLINE const BoundingBox& GetBounds() const
    {
        return _bounds;
    }

    /// <summary>
    /// Gets the model world matrix transform.
    /// </summary>
    /// <param name="world">The result world matrix.</param>
    FORCE_INLINE void GetWorld(Matrix* world) const
    {
        *world = _world;
    }

    /// <summary>
    /// Gets the scale (in XY) and bias (in ZW) applied to the vertex UVs to get the chunk coordinates.
    /// </summary>
    /// <param name="result">The result.</param>
    FORCE_INLINE void GetHeightmapUVScaleBias(Vector4* result) const
    {
        *result = _heightmapUVScaleBias;
    }

    /// <summary>
    /// Determines whether this chunk has valid lightmap data.
    /// </summary>
    /// <returns><c>true</c> if this chunk has valid lightmap data; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool HasLightmap() const
    {
        return Lightmap.TextureIndex != INVALID_INDEX;
    }

    /// <summary>
    /// Removes the lightmap data from the chunk.
    /// </summary>
    FORCE_INLINE void RemoveLightmap()
    {
        Lightmap.TextureIndex = INVALID_INDEX;
    }

public:

    /// <summary>
    /// Prepares for drawing chunk. Cached LOD and material.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <returns>True if draw chunk, otherwise false.</returns>
    bool PrepareDraw(const RenderContext& renderContext);

    /// <summary>
    /// Draws the chunk (adds the draw call). Must be called after PrepareDraw.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    void Draw(const RenderContext& renderContext) const;

    /// <summary>
    /// Draws the terrain chunk.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="material">The material to use for rendering.</param>
    /// <param name="lodIndex">The LOD index.</param>
    void Draw(const RenderContext& renderContext, MaterialBase* material, int32 lodIndex = 0) const;

    /// <summary>
    /// Determines if there is an intersection between the terrain chunk and a point
    /// </summary>
    /// <param name="ray">The ray.</param>
    /// <param name="distance">The output distance.</param>
    /// <returns>True if chunk intersects with the ray, otherwise false.</returns>
    bool Intersects(const Ray& ray, float& distance);

    /// <summary>
    /// Updates the cached bounds of the chunk.
    /// </summary>
    void UpdateBounds();

    /// <summary>
    /// Updates the cached transform of the chunk.
    /// </summary>
    void UpdateTransform();

    /// <summary>
    /// Caches the neighbor chunks of this chunk.
    /// </summary>
    void CacheNeighbors();

public:

    // [ISerializable]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
