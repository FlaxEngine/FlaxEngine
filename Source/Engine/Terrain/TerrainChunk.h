// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Core/ISerializable.h"
#include "Engine/Content/Assets/MaterialBase.h"
#include "Engine/Level/Scene/Lightmap.h"

class Terrain;
class TerrainPatch;
struct RenderContext;

/// <summary>
/// Represents a single terrain chunk.
/// </summary>
API_CLASS(Sealed, NoSpawn) class FLAXENGINE_API TerrainChunk : public ScriptingObject, public ISerializable
{
    DECLARE_SCRIPTING_TYPE(TerrainChunk);
    friend Terrain;
    friend TerrainPatch;
    friend TerrainChunk;

private:
    TerrainPatch* _patch;
    uint16 _x, _z;
    Float4 _heightmapUVScaleBias;
    Transform _transform;
    BoundingBox _bounds;
    BoundingSphere _sphere;
    float _perInstanceRandom;
    float _yOffset, _yHeight;

    TerrainChunk* _neighbors[4] = {};
    byte _cachedDrawLOD = 0;
    IMaterial* _cachedDrawMaterial = nullptr;

    void Init(TerrainPatch* patch, uint16 x, uint16 z);

public:
    /// <summary>
    /// The material to override the terrain default one for this chunk.
    /// </summary>
    API_FIELD() AssetReference<MaterialBase> OverrideMaterial;

    /// <summary>
    /// The baked lightmap entry info for this chunk.
    /// </summary>
    LightmapEntry Lightmap;

public:
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
    /// Gets the patch.
    /// </summary>
    API_FUNCTION() FORCE_INLINE TerrainPatch* GetPatch() const
    {
        return _patch;
    }

    /// <summary>
    /// Gets the chunk world bounds.
    /// </summary>
    API_FUNCTION() FORCE_INLINE const BoundingBox& GetBounds() const
    {
        return _bounds;
    }

    /// <summary>
    /// Gets the chunk transformation (world to local).
    /// </summary>
    API_FUNCTION() FORCE_INLINE const Transform& GetTransform() const
    {
        return _transform;
    }

    /// <summary>
    /// Gets the scale (in XY) and bias (in ZW) applied to the vertex UVs to get the chunk coordinates.
    /// </summary>
    API_FUNCTION() FORCE_INLINE const Float4& GetHeightmapUVScaleBias() const
    {
        return _heightmapUVScaleBias;
    }

    /// <summary>
    /// Determines whether this chunk has valid lightmap data.
    /// </summary>
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
    API_FUNCTION() void Draw(API_PARAM(Ref) const RenderContext& renderContext, MaterialBase* material, int32 lodIndex = 0) const;

    /// <summary>
    /// Determines if there is an intersection between the terrain chunk and a point
    /// </summary>
    /// <param name="ray">The ray.</param>
    /// <param name="distance">The output distance.</param>
    /// <returns>True if chunk intersects with the ray, otherwise false.</returns>
    API_FUNCTION() bool Intersects(const Ray& ray, API_PARAM(Out) Real& distance);

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
