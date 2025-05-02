// Copyright (c) Wojciech Figat. All rights reserved.

#include "TerrainChunk.h"
#include "Engine/Serialization/Serialization.h"
#include "TerrainPatch.h"
#include "Terrain.h"
#include "TerrainManager.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Core/Math/OrientedBoundingBox.h"
#include "Engine/Level/Scene/Scene.h"
#if USE_EDITOR
#include "Engine/Level/Prefabs/PrefabManager.h"
#endif

TerrainChunk::TerrainChunk(const SpawnParams& params)
    : ScriptingObject(params)
{
}

void TerrainChunk::Init(TerrainPatch* patch, uint16 x, uint16 z)
{
    // Initialize chunk properties
    _patch = patch;
    _x = x;
    _z = z;
    _yOffset = 0;
    _yHeight = 1;
    _heightmapUVScaleBias = Float4(1.0f, 1.0f, _x, _z) * (1.0f / Terrain::ChunksCountEdge);
    _perInstanceRandom = (_patch->_terrain->_id.C ^ _x ^ _z) * (1.0f / (float)MAX_uint32);
    OverrideMaterial = nullptr;
}

bool TerrainChunk::PrepareDraw(const RenderContext& renderContext)
{
    // Calculate LOD
    int32 lod;
    const int32 forcedLod = _patch->_terrain->_forcedLod;
    const int32 lodCount = _patch->Heightmap.Get()->StreamingTexture()->TotalMipLevels();
    const int32 minStreamedLod = lodCount - _patch->Heightmap.Get()->GetTexture()->ResidentMipLevels();
    if (forcedLod >= 0)
    {
        lod = forcedLod;
    }
    else
    {
        const int32 lodBias = _patch->_terrain->_lodBias;
        const float lodDistribution = _patch->_terrain->_lodDistribution;
        const float chunkEdgeSize = (_patch->_terrain->_chunkSize * TERRAIN_UNITS_PER_VERTEX);

        // Calculate chunk distance to view
        const auto lodView = (renderContext.LodProxyView ? renderContext.LodProxyView : &renderContext.View);
        const float distance = Float3::Distance(_sphere.Center - lodView->Origin, lodView->Position);
        lod = (int32)Math::Pow(distance / chunkEdgeSize, lodDistribution);
        lod += lodBias;

        //lod = 0;
        //lod = 10;
        //lod = (_x + _z + Terrain::ChunksCountEdge * (_patch->_x + _patch->_z));
        //lod = (int32)Vector2::Distance(Vector2(2, 2), Vector2(_patch->_x, _patch->_z) * Terrain::ChunksCountEdge + Vector2(_x, _z));
        //lod = (int32)(Vector3::Distance(_bounds.GetCenter(), view.Position) / 10000.0f);
    }
    lod = Math::Clamp(lod, minStreamedLod, lodCount - 1);

    // Pick a material
    auto material = OverrideMaterial.Get();
    if (!material || !material->IsLoaded())
    {
        material = _patch->_terrain->Material.Get();
        if (!material || !material->IsLoaded())
            material = TerrainManager::GetDefaultTerrainMaterial();
    }
    if (!material || !material->IsReady() || !material->IsTerrain())
        return false;

    // Cache data
    _cachedDrawLOD = lod;
    _cachedDrawMaterial = material;
    return true;
}

void TerrainChunk::Draw(const RenderContext& renderContext) const
{
    const int32 lod = _cachedDrawLOD;
    const int32 minLod = Math::Max(lod + 1, 0);
    const int32 chunkSize = _patch->_terrain->GetChunkSize();

    // Setup draw call
    DrawCall drawCall;
    if (TerrainManager::GetChunkGeometry(drawCall, chunkSize, lod))
        return;
    if (!_neighbors[0])
        const_cast<TerrainChunk*>(this)->CacheNeighbors();
    drawCall.InstanceCount = 1;
    drawCall.Material = _cachedDrawMaterial;
    renderContext.View.GetWorldMatrix(_transform, drawCall.World);
    drawCall.ObjectPosition = drawCall.World.GetTranslation();
    drawCall.ObjectRadius = (float)_sphere.Radius;
    drawCall.Terrain.Patch = _patch;
    drawCall.Terrain.HeightmapUVScaleBias = _heightmapUVScaleBias;
    drawCall.Terrain.OffsetUV = Vector2((float)(_patch->_x * Terrain::ChunksCountEdge + _x), (float)(_patch->_z * Terrain::ChunksCountEdge + _z));
    drawCall.Terrain.CurrentLOD = (float)lod;
    drawCall.Terrain.ChunkSizeNextLOD = (float)(((chunkSize + 1) >> (lod + 1)) - 1);
    drawCall.Terrain.TerrainChunkSizeLOD0 = TERRAIN_UNITS_PER_VERTEX * chunkSize;
    // TODO: try using SIMD clamping for 4 chunks at once
    drawCall.Terrain.NeighborLOD.X = (float)Math::Clamp<int32>(_neighbors[0]->_cachedDrawLOD, lod, minLod);
    drawCall.Terrain.NeighborLOD.Y = (float)Math::Clamp<int32>(_neighbors[1]->_cachedDrawLOD, lod, minLod);
    drawCall.Terrain.NeighborLOD.Z = (float)Math::Clamp<int32>(_neighbors[2]->_cachedDrawLOD, lod, minLod);
    drawCall.Terrain.NeighborLOD.W = (float)Math::Clamp<int32>(_neighbors[3]->_cachedDrawLOD, lod, minLod);
    const auto scene = _patch->_terrain->GetScene();
    const auto flags = _patch->_terrain->_staticFlags;
    if ((flags & StaticFlags::Lightmap) != StaticFlags::None && scene)
    {
        drawCall.Terrain.Lightmap = scene->LightmapsData.GetReadyLightmap(Lightmap.TextureIndex);
        drawCall.Terrain.LightmapUVsArea = Lightmap.UVsArea;
    }
    else
    {
        drawCall.Terrain.Lightmap = nullptr;
        drawCall.Terrain.LightmapUVsArea = Rectangle::Empty;
    }
    drawCall.WorldDeterminantSign = RenderTools::GetWorldDeterminantSign(drawCall.World);
    drawCall.PerInstanceRandom = _perInstanceRandom;
#if USE_EDITOR
    if (renderContext.View.Mode == ViewMode::LightmapUVsDensity)
        drawCall.Surface.LODDitherFactor = 1.0f; // See LightmapUVsDensityMaterialShader
#endif

    // Add half-texel offset for heightmap sampling in vertex shader
    //const float lodHeightmapSize = Math::Max(1, drawCall.TerrainData.Heightmap->Width() >> lod);
    //const float halfTexelOffset = 0.5f / lodHeightmapSize;
    //drawCall.TerrainData.HeightmapUVScaleBias.Z += halfTexelOffset;
    //drawCall.TerrainData.HeightmapUVScaleBias.W += halfTexelOffset;

    // Submit draw call
    const DrawPass drawModes = _patch->_terrain->DrawModes & renderContext.View.Pass & drawCall.Material->GetDrawModes();
    if (drawModes != DrawPass::None)
        renderContext.List->AddDrawCall(renderContext, drawModes, flags, drawCall, true);
}

void TerrainChunk::Draw(const RenderContext& renderContext, MaterialBase* material, int32 lodIndex) const
{
    if (_patch->Heightmap == nullptr || !_patch->Heightmap->IsLoaded())
        return;
    if (!material || !material->IsReady() || !material->IsTerrain())
        return;

    const int32 lodCount = _patch->Heightmap.Get()->StreamingTexture()->TotalMipLevels();
    const int32 lod = Math::Clamp(lodIndex, 0, lodCount);
    const int32 chunkSize = _patch->_terrain->_chunkSize;

    // Setup draw call
    DrawCall drawCall;
    if (TerrainManager::GetChunkGeometry(drawCall, chunkSize, lod))
        return;
    drawCall.InstanceCount = 1;
    drawCall.Material = material;
    renderContext.View.GetWorldMatrix(_transform, drawCall.World);
    drawCall.ObjectPosition = drawCall.World.GetTranslation();
    drawCall.ObjectRadius = (float)_sphere.Radius;
    drawCall.Terrain.Patch = _patch;
    drawCall.Terrain.HeightmapUVScaleBias = _heightmapUVScaleBias;
    drawCall.Terrain.OffsetUV = Vector2((float)(_patch->_x * Terrain::ChunksCountEdge + _x), (float)(_patch->_z * Terrain::ChunksCountEdge + _z));
    drawCall.Terrain.CurrentLOD = (float)lod;
    drawCall.Terrain.ChunkSizeNextLOD = (float)(((chunkSize + 1) >> (lod + 1)) - 1);
    drawCall.Terrain.TerrainChunkSizeLOD0 = TERRAIN_UNITS_PER_VERTEX * chunkSize;
    drawCall.Terrain.NeighborLOD.X = (float)lod;
    drawCall.Terrain.NeighborLOD.Y = (float)lod;
    drawCall.Terrain.NeighborLOD.Z = (float)lod;
    drawCall.Terrain.NeighborLOD.W = (float)lod;
    const auto scene = _patch->_terrain->GetScene();
    const auto flags = _patch->_terrain->_staticFlags;
    if ((flags & StaticFlags::Lightmap) != StaticFlags::None && scene)
    {
        drawCall.Terrain.Lightmap = scene->LightmapsData.GetReadyLightmap(Lightmap.TextureIndex);
        drawCall.Terrain.LightmapUVsArea = Lightmap.UVsArea;
    }
    else
    {
        drawCall.Terrain.Lightmap = nullptr;
        drawCall.Terrain.LightmapUVsArea = Rectangle::Empty;
    }
    drawCall.WorldDeterminantSign = RenderTools::GetWorldDeterminantSign(drawCall.World);
    drawCall.PerInstanceRandom = _perInstanceRandom;
#if USE_EDITOR
    if (renderContext.View.Mode == ViewMode::LightmapUVsDensity)
        drawCall.Surface.LODDitherFactor = 1.0f; // See LightmapUVsDensityMaterialShader
#endif

    // Add half-texel offset for heightmap sampling in vertex shader
    //const float lodHeightmapSize = Math::Max(1, drawCall.TerrainData.Heightmap->Width() >> lod);
    //const float halfTexelOffset = 0.5f / lodHeightmapSize;
    //drawCall.TerrainData.HeightmapUVScaleBias.Z += halfTexelOffset;
    //drawCall.TerrainData.HeightmapUVScaleBias.W += halfTexelOffset;

    // Submit draw call
    const DrawPass drawModes = _patch->_terrain->DrawModes & renderContext.View.Pass & drawCall.Material->GetDrawModes();
    if (drawModes != DrawPass::None)
        renderContext.List->AddDrawCall(renderContext, drawModes, flags, drawCall, true);
}

bool TerrainChunk::Intersects(const Ray& ray, Real& distance)
{
    return _bounds.Intersects(ray, distance);
}

void TerrainChunk::UpdateBounds()
{
    const Vector3 boundsExtent = _patch->_terrain->_boundsExtent;
    const float size = (float)_patch->_terrain->_chunkSize * TERRAIN_UNITS_PER_VERTEX;
    const Transform terrainTransform = _patch->_terrain->_transform;

    Transform localTransform;
    localTransform.Translation = _patch->_offset + Vector3(_x * size, _yOffset, _z * size);
    localTransform.Orientation = Quaternion::Identity;
    localTransform.Scale = Vector3(size, _yHeight, size);
    localTransform = terrainTransform.LocalToWorld(localTransform);

    OrientedBoundingBox obb(Vector3::Zero, Vector3::One);
    obb.Transform(localTransform);
    obb.GetBoundingBox(_bounds);
    BoundingSphere::FromBox(_bounds, _sphere);

    _bounds.Minimum -= boundsExtent;
    _bounds.Maximum += boundsExtent;
}

void TerrainChunk::UpdateTransform()
{
    const float size = _patch->_terrain->_chunkSize * TERRAIN_UNITS_PER_VERTEX;
    Transform terrainTransform = _patch->_terrain->_transform;
    Transform localTransform;
    localTransform.Translation = _patch->_offset + Vector3(_x * size, _patch->_yOffset, _z * size);
    localTransform.Orientation = Quaternion::Identity;
    localTransform.Scale = Vector3(1.0f, _patch->_yHeight, 1.0f);
    _transform = terrainTransform.LocalToWorld(localTransform);
}

void TerrainChunk::CacheNeighbors()
{
    // Cache per-chunk neighbors (for morph transition)
    // Fallback to this chunk if none existing on the edge

    // 0: bottom
    _neighbors[0] = this;
    if (_z > 0)
    {
        _neighbors[0] = &_patch->Chunks[(_z - 1) * Terrain::ChunksCountEdge + _x];
    }
    else
    {
        const auto patch = _patch->_terrain->GetPatch(_patch->_x, _patch->_z - 1);
        if (patch)
            _neighbors[0] = &patch->Chunks[(Terrain::ChunksCountEdge - 1) * Terrain::ChunksCountEdge + _x];
    }

    // 1: left
    _neighbors[1] = this;
    if (_x > 0)
    {
        _neighbors[1] = &_patch->Chunks[_z * Terrain::ChunksCountEdge + (_x - 1)];
    }
    else
    {
        const auto patch = _patch->_terrain->GetPatch(_patch->_x - 1, _patch->_z);
        if (patch)
            _neighbors[1] = &patch->Chunks[_z * Terrain::ChunksCountEdge + (Terrain::ChunksCountEdge - 1)];
    }

    // 2: right 
    _neighbors[2] = this;
    if (_x < Terrain::ChunksCountEdge - 1)
    {
        _neighbors[2] = &_patch->Chunks[_z * Terrain::ChunksCountEdge + (_x + 1)];
    }
    else
    {
        const auto patch = _patch->_terrain->GetPatch(_patch->_x + 1, _patch->_z);
        if (patch)
            _neighbors[2] = &patch->Chunks[_z * Terrain::ChunksCountEdge];
    }

    // 3: top
    _neighbors[3] = this;
    if (_z < Terrain::ChunksCountEdge - 1)
    {
        _neighbors[3] = &_patch->Chunks[(_z + 1) * Terrain::ChunksCountEdge + _x];
    }
    else
    {
        const auto patch = _patch->_terrain->GetPatch(_patch->_x, _patch->_z + 1);
        if (patch)
            _neighbors[3] = &patch->Chunks[_x];
    }
}

void TerrainChunk::Serialize(SerializeStream& stream, const void* otherObj)
{
    SERIALIZE_GET_OTHER_OBJ(TerrainChunk);

    SERIALIZE_MEMBER(Offset, _yOffset);
    SERIALIZE_MEMBER(Height, _yHeight);
    SERIALIZE_MEMBER(Material, OverrideMaterial);

    if (HasLightmap()
#if USE_EDITOR
        && !PrefabManager::IsCreatingPrefab
#endif
    )
    {
        stream.JKEY("LightmapIndex");
        stream.Int(Lightmap.TextureIndex);

        stream.JKEY("LightmapArea");
        stream.Rectangle(Lightmap.UVsArea);
    }
}

void TerrainChunk::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    DESERIALIZE_MEMBER(Offset, _yOffset);
    DESERIALIZE_MEMBER(Height, _yHeight);
    DESERIALIZE_MEMBER(Material, OverrideMaterial);
    DESERIALIZE_MEMBER(LightmapIndex, Lightmap.TextureIndex);
    DESERIALIZE_MEMBER(LightmapArea, Lightmap.UVsArea);
}
