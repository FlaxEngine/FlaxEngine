// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "TerrainPatch.h"
#include "Terrain.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Physics/PhysicsBackend.h"
#include "Engine/Physics/CollisionCooking.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Level/Level.h"
#include "Engine/Graphics/Async/GPUTask.h"
#include "Engine/Threading/Threading.h"
#if TERRAIN_EDITING
#include "Engine/Core/Math/Packed.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#include "Engine/ContentImporters/AssetsImportingManager.h"
#endif
#endif
#if TERRAIN_EDITING || TERRAIN_UPDATING
#include "Engine/Core/Collections/ArrayExtensions.h"
#endif
#if USE_EDITOR
#include "Engine/Debug/DebugDraw.h"
#endif
#if TERRAIN_USE_PHYSICS_DEBUG
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/DynamicBuffer.h"
#include "Engine/Engine/Units.h"
#endif
#include "Engine/Content/Content.h"
#include "Engine/Content/Assets/RawDataAsset.h"

#define TERRAIN_PATCH_COLLISION_QUANTIZATION ((float)0x7fff)

// [Deprecated on 4.03.2024, expires on 4.03.2029]
struct TerrainCollisionDataHeaderOld
{
    int32 LOD;
    float ScaleXZ;
};

struct TerrainCollisionDataHeader
{
    static constexpr int32 CurrentVersion = 1;
    int32 CheckOldMagicNumber; // Used to detect if loading new header or old one
    int32 Version;
    int32 LOD;
    float ScaleXZ;
};

TerrainPatch::TerrainPatch(const SpawnParams& params)
    : ScriptingObject(params)
{
}

void TerrainPatch::Init(Terrain* terrain, int16 x, int16 z)
{
    ScopeLock lock(_collisionLocker);

    _terrain = terrain;
    _physicsShape = nullptr;
    _physicsActor = nullptr;
    _physicsHeightField = nullptr;
    _x = x;
    _z = z;
    const float size = _terrain->_chunkSize * TERRAIN_UNITS_PER_VERTEX * Terrain::Terrain::ChunksCountEdge;
    _offset = Float3(_x * size, 0.0f, _z * size);
    _yOffset = 0.0f;
    _yHeight = 1.0f;
    for (int32 i = 0; i < Terrain::ChunksCount; i++)
    {
        Chunks[i].Init(this, i % Terrain::Terrain::ChunksCountEdge, i / Terrain::Terrain::ChunksCountEdge);
    }
    Heightmap = nullptr;
    for (int32 i = 0; i < TERRAIN_MAX_SPLATMAPS_COUNT; i++)
    {
        Splatmap[i] = nullptr;
    }
    _heightfield = nullptr;
#if TERRAIN_UPDATING
    _cachedHeightMap.Resize(0);
    _cachedHolesMask.Resize(0);
    _wasHeightModified = false;
    for (int32 i = 0; i < TERRAIN_MAX_SPLATMAPS_COUNT; i++)
    {
        _cachedSplatMap[i].Resize(0);
        _wasSplatmapModified[i] = false;
    }
#endif
#if TERRAIN_USE_PHYSICS_DEBUG
    SAFE_DELETE(_debugLines);
    _debugLinesDirty = true;
#endif
#if USE_EDITOR
    _collisionTriangles.Resize(0);
#endif
    _collisionVertices.Resize(0);
}

TerrainPatch::~TerrainPatch()
{
#if TERRAIN_UPDATING
    SAFE_DELETE(_dataHeightmap);
    for (int32 i = 0; i < TERRAIN_MAX_SPLATMAPS_COUNT; i++)
    {
        SAFE_DELETE(_dataSplatmap[i]);
    }
#endif
}

RawDataAsset* TerrainPatch::GetHeightfield() const
{
    return _heightfield.Get();
}

void TerrainPatch::RemoveLightmap()
{
    for (auto& chunk : Chunks)
    {
        chunk.RemoveLightmap();
    }
}

void TerrainPatch::UpdateBounds()
{
    PROFILE_CPU();
    Chunks[0].UpdateBounds();
    _bounds = Chunks[0]._bounds;
    for (int32 i = 1; i < Terrain::ChunksCount; i++)
    {
        Chunks[i].UpdateBounds();
        BoundingBox::Merge(_bounds, Chunks[i]._bounds, _bounds);
    }
}

void TerrainPatch::UpdateTransform()
{
    PROFILE_CPU();

    // Update physics
    if (_physicsActor)
    {
        const Transform& terrainTransform = _terrain->_transform;
        PhysicsBackend::SetRigidActorPose(_physicsActor, terrainTransform.LocalToWorld(_offset), terrainTransform.Orientation);
    }

    // Update chunks cache
    for (int32 i = 0; i < Terrain::ChunksCount; i++)
    {
        Chunks[i].UpdateTransform();
    }

#if USE_EDITOR
    // We pre-transform vertices to world space
    _collisionTriangles.Resize(0);
#endif
    _collisionVertices.Resize(0);
}

#if TERRAIN_EDITING || TERRAIN_UPDATING

bool IsValidMaterial(const JsonAssetReference<PhysicalMaterial>& e)
{
    return e;
}

struct TerrainDataUpdateInfo
{
    TerrainPatch* Patch;
    int32 ChunkSize;
    int32 VertexCountEdge;
    int32 HeightmapSize;
    int32 HeightmapLength;
    int32 TextureSize;
    float PatchOffset;
    float PatchHeight;
    Color32* SplatMaps[TERRAIN_MAX_SPLATMAPS_COUNT] = {};

    TerrainDataUpdateInfo(TerrainPatch* patch, float patchOffset = 0.0f, float patchHeight = 1.0f)
        : Patch(patch)
        , PatchOffset(patchOffset)
        , PatchHeight(patchHeight)
    {
        ChunkSize = patch->GetTerrain()->GetChunkSize();
        VertexCountEdge = ChunkSize + 1;
        HeightmapSize = ChunkSize * Terrain::ChunksCountEdge + 1;
        HeightmapLength = HeightmapSize * HeightmapSize;
        TextureSize = VertexCountEdge * Terrain::ChunksCountEdge;
    }

    bool UsePhysicalMaterials() const
    {
        return ArrayExtensions::Any<JsonAssetReference<PhysicalMaterial>>(Patch->GetTerrain()->GetPhysicalMaterials(), IsValidMaterial);
    }

    // When using physical materials, then get splatmaps data required for per-triangle material indices
    void GetSplatMaps()
    {
#if TERRAIN_UPDATING
        if (SplatMaps[0])
            return;
        if (UsePhysicalMaterials())
        {
            for (int32 i = 0; i < TERRAIN_MAX_SPLATMAPS_COUNT; i++)
                SplatMaps[i] = Patch->GetSplatMapData(i);
        }
#else
        LOG(Warning, "Splatmaps reading not implemented for physical layers updating.");
#endif
    }
};

// Shared data container for the terrain data updating shared by the normals and collision generation logic
static Array<byte> TerrainUpdateScratchBuffer;

#define GET_TERRAIN_SCRATCH_BUFFER(variable, count, type) \
	TerrainUpdateScratchBuffer.Clear(); \
	TerrainUpdateScratchBuffer.EnsureCapacity((count) * sizeof(type)); \
	auto variable = (type*)TerrainUpdateScratchBuffer.Get()

float AlignHeight(double height, double error)
{
    const double heightCount = height / error;
    const int64 heightCountInt = (int64)heightCount;
    return (float)(heightCountInt * error);
}

FORCE_INLINE void WriteHeight(const TerrainDataUpdateInfo& info, Color32& raw, const float height)
{
    const float normalizedHeight = (height - info.PatchOffset) / info.PatchHeight;
    const uint16 quantizedHeight = (uint16)(normalizedHeight * MAX_uint16);

    raw.R = (uint8)(quantizedHeight & 0xff);
    raw.G = (uint8)((quantizedHeight >> 8) & 0xff);
}

FORCE_INLINE float ReadNormalizedHeight(const Color32& raw)
{
    const uint16 quantizedHeight = raw.R | (raw.G << 8);
    const float normalizedHeight = (float)quantizedHeight / MAX_uint16;
    return normalizedHeight;
}

FORCE_INLINE bool ReadIsHole(const Color32& raw)
{
    return (raw.B + raw.A) >= (int32)(1.9f * MAX_uint8);
}

void CalculateHeightmapRange(Terrain* terrain, TerrainDataUpdateInfo& info, const float* heightmap, float chunkOffsets[Terrain::ChunksCount], float chunkHeights[Terrain::ChunksCount])
{
    PROFILE_CPU_NAMED("Terrain.CalculateRange");

    // Note: terrain heightmap doesn't store raw height values but normalized into per-patch dimensions (height = normHeight * chunkPatch + patchOffset)

    float minPatchHeight = MAX_float;
    float maxPatchHeight = MIN_float;

    for (int32 chunkIndex = 0; chunkIndex < Terrain::ChunksCount; chunkIndex++)
    {
        const int32 chunkX = (chunkIndex % Terrain::ChunksCountEdge) * info.ChunkSize;
        const int32 chunkZ = (chunkIndex / Terrain::ChunksCountEdge) * info.ChunkSize;

        float minHeight = MAX_float;
        float maxHeight = MIN_float;

        for (int32 z = 0; z < info.VertexCountEdge; z++)
        {
            const int32 sz = (chunkZ + z) * info.HeightmapSize;
            for (int32 x = 0; x < info.VertexCountEdge; x++)
            {
                const int32 sx = chunkX + x;
                const float height = heightmap[sz + sx];

                minHeight = Math::Min(minHeight, height);
                maxHeight = Math::Max(maxHeight, height);
            }
        }

        chunkOffsets[chunkIndex] = minHeight;
        chunkHeights[chunkIndex] = Math::Max(maxHeight - minHeight, 1.0f);

        minPatchHeight = Math::Min(minPatchHeight, minHeight);
        maxPatchHeight = Math::Max(maxPatchHeight, maxHeight);
    }

    // Align the patch heightmap range error to reduce artifacts on patch edges (each patch has own height range)
    const double error = 1.0 / MAX_uint16;
    minPatchHeight = AlignHeight(minPatchHeight - error, error);
    maxPatchHeight = AlignHeight(maxPatchHeight + error, error);

    info.PatchOffset = minPatchHeight;
    info.PatchHeight = Math::Max(maxPatchHeight - minPatchHeight, 1.0f);
}

void UpdateHeightMap(const TerrainDataUpdateInfo& info, const float* heightmap, const Int2& modifiedOffset, const Int2& modifiedSize, const byte* data)
{
    PROFILE_CPU_NAMED("Terrain.UpdateHeightMap");

    // TODO: use offset and size to improve performance of the data updating

    const auto heightmapPtr = heightmap;
    const auto ptr = (Color32*)data;

    for (int32 chunkIndex = 0; chunkIndex < Terrain::ChunksCount; chunkIndex++)
    {
        const int32 chunkX = (chunkIndex % Terrain::ChunksCountEdge);
        const int32 chunkZ = (chunkIndex / Terrain::ChunksCountEdge);

        const int32 chunkTextureX = chunkX * info.VertexCountEdge;
        const int32 chunkTextureZ = chunkZ * info.VertexCountEdge;

        const int32 chunkHeightmapX = chunkX * info.ChunkSize;
        const int32 chunkHeightmapZ = chunkZ * info.ChunkSize;

        for (int32 z = 0; z < info.VertexCountEdge; z++)
        {
            const int32 tz = (chunkTextureZ + z) * info.TextureSize;
            const int32 sz = (chunkHeightmapZ + z) * info.HeightmapSize;

            for (int32 x = 0; x < info.VertexCountEdge; x++)
            {
                const int32 tx = chunkTextureX + x;
                const int32 sx = chunkHeightmapX + x;
                const int32 textureIndex = tz + tx;
                const int32 heightmapIndex = sz + sx;

                WriteHeight(info, ptr[textureIndex], heightmapPtr[heightmapIndex]);
            }
        }
    }
}

void UpdateHeightMap(const TerrainDataUpdateInfo& info, const float* heightmap, const byte* data)
{
    UpdateHeightMap(info, heightmap, Int2::Zero, Int2(info.HeightmapSize), data);
}

void UpdateSplatMap(const TerrainDataUpdateInfo& info, const Color32* splatMap, const Int2& modifiedOffset, const Int2& modifiedSize, const byte* data)
{
    PROFILE_CPU_NAMED("Terrain.UpdateSplatMap");

    // TODO: use offset and size to improve performance of the data updating

    const auto splatPtr = splatMap;
    const auto ptr = (Color32*)data;
    for (int32 chunkIndex = 0; chunkIndex < Terrain::ChunksCount; chunkIndex++)
    {
        const int32 chunkX = (chunkIndex % Terrain::ChunksCountEdge);
        const int32 chunkZ = (chunkIndex / Terrain::ChunksCountEdge);

        const int32 chunkTextureX = chunkX * info.VertexCountEdge;
        const int32 chunkTextureZ = chunkZ * info.VertexCountEdge;

        const int32 chunkHeightmapX = chunkX * info.ChunkSize;
        const int32 chunkHeightmapZ = chunkZ * info.ChunkSize;

        for (int32 z = 0; z < info.VertexCountEdge; z++)
        {
            const int32 tz = (chunkTextureZ + z) * info.TextureSize;
            const int32 sz = (chunkHeightmapZ + z) * info.HeightmapSize;

            for (int32 x = 0; x < info.VertexCountEdge; x++)
            {
                const int32 tx = chunkTextureX + x;
                const int32 sx = chunkHeightmapX + x;
                const int32 textureIndex = tz + tx;
                const int32 heightmapIndex = sz + sx;

                ptr[textureIndex] = splatPtr[heightmapIndex];
            }
        }
    }
}

void UpdateSplatMap(const TerrainDataUpdateInfo& info, const Color32* splatMap, const byte* data)
{
    UpdateSplatMap(info, splatMap, Int2::Zero, Int2(info.HeightmapSize), data);
}

void UpdateNormalsAndHoles(const TerrainDataUpdateInfo& info, const float* heightmap, const byte* holesMask, const Int2& modifiedOffset, const Int2& modifiedSize, const byte* data)
{
    PROFILE_CPU_NAMED("Terrain.CalculateNormals");

    // Expand the area for the normals to prevent issues on the edges (for the averaged normals)
    const Int2 modifiedEnd = modifiedOffset + modifiedSize;
    const Int2 normalsStart = Int2::Max(Int2::Zero, modifiedOffset - 1);
    const Int2 normalsEnd = Int2::Min(info.HeightmapSize, modifiedEnd + 1);
    const Int2 normalsSize = normalsEnd - normalsStart;

    // Prepare memory
    const int32 normalsLength = normalsSize.X * normalsSize.Y;
    GET_TERRAIN_SCRATCH_BUFFER(normalsPerVertex, normalsLength, Float3);

    // Clear normals (for accumulation pass)
    Platform::MemoryClear(normalsPerVertex, normalsLength * sizeof(Float3));

    // Calculate per-quad normals and apply them to nearby vertices
    for (int32 z = normalsStart.Y; z < normalsEnd.Y - 1; z++)
    {
        for (int32 x = normalsStart.X; x < normalsEnd.X - 1; x++)
        {
            // Get four vertices from the quad
#define GET_VERTEX(a, b) \
	int32 i##a##b = (z + (b) - normalsStart.Y) * normalsSize.X + (x + (a) - normalsStart.X); \
	int32 h##a##b = (z + (b)) * info.HeightmapSize + (x + (a)); \
	Float3 v##a##b; v##a##b.X = (x + (a)) * TERRAIN_UNITS_PER_VERTEX; \
	v##a##b.Y = heightmap[h##a##b]; \
	v##a##b.Z = (z + (b)) * TERRAIN_UNITS_PER_VERTEX
            GET_VERTEX(0, 0);
            GET_VERTEX(1, 0);
            GET_VERTEX(0, 1);
            GET_VERTEX(1, 1);
#undef GET_VERTEX

            // TODO: use SIMD for those calculations

            // Calculate normals for quad two vertices
            Float3 n0 = Float3::Normalize((v00 - v01) ^ (v01 - v10));
            Float3 n1 = Float3::Normalize((v11 - v10) ^ (v10 - v01));
            Float3 n2 = n0 + n1;

            // Apply normal to each vertex using it
            normalsPerVertex[i00] += n1;
            normalsPerVertex[i01] += n2;
            normalsPerVertex[i10] += n2;
            normalsPerVertex[i11] += n0;
        }
    }

    // Smooth normals
    for (int32 z = 1; z < normalsSize.Y - 1; z++)
    {
        for (int32 x = 1; x < normalsSize.X - 1; x++)
        {
            // Get four normals for the nearby quads
#define GET_NORMAL(a, b) \
	int32 i##a##b = (z + (b - 1)) * normalsSize.X + (x + (a - 1)); \
	Float3 n##a##b = Float3::NormalizeFast(normalsPerVertex[i##a##b])
            GET_NORMAL(0, 0);
            GET_NORMAL(1, 0);
            GET_NORMAL(0, 1);
            GET_NORMAL(1, 1);
            GET_NORMAL(2, 0);
            GET_NORMAL(2, 1);
            GET_NORMAL(0, 2);
            GET_NORMAL(1, 2);
            GET_NORMAL(2, 2);
#undef GET_VERTEX

            // TODO: use SIMD for those calculations

            /*
             * The current vertex is (11). Calculate average for the nearby vertices.
             * 00   01   02
             * 10  (11)  12
             * 20   21   22
             */

            const Float3 avg = (n00 + n01 + n02 + n10 + n11 + n12 + n20 + n21 + n22) * (1.0f / 9.0f);

            // Smooth normals by performing interpolation to average for nearby quads
            normalsPerVertex[i11] = Float3::Lerp(n11, avg, 0.6f);
        }
    }

    // Write back to the data container
    const auto ptr = (Color32*)data;
    for (int32 chunkIndex = 0; chunkIndex < Terrain::ChunksCount; chunkIndex++)
    {
        const int32 chunkX = (chunkIndex % Terrain::ChunksCountEdge);
        const int32 chunkZ = (chunkIndex / Terrain::ChunksCountEdge);

        const int32 chunkTextureX = chunkX * info.VertexCountEdge;
        const int32 chunkTextureZ = chunkZ * info.VertexCountEdge;

        const int32 chunkHeightmapX = chunkX * info.ChunkSize;
        const int32 chunkHeightmapZ = chunkZ * info.ChunkSize;

        // Skip unmodified chunks
        if (chunkHeightmapX >= modifiedEnd.X || chunkHeightmapX + info.ChunkSize < modifiedOffset.X ||
            chunkHeightmapZ >= modifiedEnd.Y || chunkHeightmapZ + info.ChunkSize < modifiedOffset.Y)
            continue;

        // TODO: adjust loop range to reduce iterations count for edge cases (skip checking unmodified samples)
        for (int32 z = 0; z < info.VertexCountEdge; z++)
        {
            // Skip unmodified columns
            const int32 dz = chunkHeightmapZ + z - modifiedOffset.Y;
            if (dz < 0 || dz >= modifiedSize.Y)
                continue;
            const int32 hz = (chunkHeightmapZ + z) * info.HeightmapSize;
            const int32 sz = (chunkHeightmapZ + z - normalsStart.Y) * normalsSize.X;
            const int32 tz = (chunkTextureZ + z) * info.TextureSize;

            // TODO: adjust loop range to reduce iterations count for edge cases (skip checking unmodified samples)
            for (int32 x = 0; x < info.VertexCountEdge; x++)
            {
                // Skip unmodified rows
                const int32 dx = chunkHeightmapX + x - modifiedOffset.X;
                if (dx < 0 || dx >= modifiedSize.X)
                    continue;
                const int32 hx = chunkHeightmapX + x;
                const int32 sx = chunkHeightmapX + x - normalsStart.X;
                const int32 tx = chunkTextureX + x;

                const int32 textureIndex = tz + tx;
                const int32 heightmapIndex = hz + hx;
                const int32 normalIndex = sz + sx;
#if BUILD_DEBUG
                ASSERT(normalIndex >= 0 && normalIndex < normalsLength);
#endif
                Float3 normal = Float3::NormalizeFast(normalsPerVertex[normalIndex]) * 0.5f + 0.5f;

                if (holesMask && !holesMask[heightmapIndex])
                    normal = Float3::One;

                ptr[textureIndex].B = (uint8)(normal.X * MAX_uint8);
                ptr[textureIndex].A = (uint8)(normal.Z * MAX_uint8);
            }
        }
    }
}

void UpdateNormalsAndHoles(const TerrainDataUpdateInfo& info, const float* heightmap, const byte* holesMask, const byte* data)
{
    UpdateNormalsAndHoles(info, heightmap, holesMask, Int2::Zero, Int2(info.HeightmapSize), data);
}

bool GenerateMips(TextureBase::InitData* initData)
{
    PROFILE_CPU_NAMED("Terrain.GenerateMips");

    for (int32 mipIndex = 1; mipIndex < initData->Mips.Count(); mipIndex++)
    {
        if (initData->GenerateMip(mipIndex, false))
        {
            LOG(Warning, "Failed to generate heightmap texture mip maps.");
            return true;
        }
    }

    return false;
}

void FixMips(const TerrainDataUpdateInfo& info, TextureBase::InitData* initData, int32 pixelStride)
{
    PROFILE_CPU_NAMED("Terrain.FixMips");

    for (int32 mipIndex = 1; mipIndex < initData->Mips.Count(); mipIndex++)
    {
        auto& mip = initData->Mips[mipIndex];
        auto& mipHigher = initData->Mips[mipIndex - 1];
        byte* mipData = mip.Data.Get();
        const byte* mipDataHigher = mipHigher.Data.Get();
        const int32 vertexCountEdgeMip = info.VertexCountEdge >> mipIndex;
        const int32 textureSizeMip = info.TextureSize >> mipIndex;
        const int32 vertexCountEdgeMipHigher = vertexCountEdgeMip << 1;
        const int32 textureSizeMipHigher = textureSizeMip << 1;

        // Make heightmap values on left edge the same as the left edge of the chunk on the higher LOD
        for (int32 chunkX = 0; chunkX < Terrain::ChunksCountEdge; chunkX++)
        {
            for (int32 chunkZ = 0; chunkZ < Terrain::ChunksCountEdge; chunkZ++)
            {
                const int32 chunkTextureX = chunkX * vertexCountEdgeMip;
                const int32 chunkTextureZ = chunkZ * vertexCountEdgeMip;

                const int32 chunkTextureXHigher = chunkX * vertexCountEdgeMipHigher;
                const int32 chunkTextureZHigher = chunkZ * vertexCountEdgeMipHigher;

                // Exclude patch edges
                int32 z = 0, zCount = vertexCountEdgeMip;
                int32 x = 0, xCount = vertexCountEdgeMip;
                if (chunkX == 0)
                    x = 1;
                else if (chunkX == Terrain::ChunksCountEdge - 1)
                    xCount--;
                if (chunkZ == 0)
                    z = 1;
                else if (chunkZ == Terrain::ChunksCountEdge - 1)
                    zCount--;

                for (; z < zCount; z++)
                {
                    const int32 textureIndex = (chunkTextureZ + z) * textureSizeMip + chunkTextureX;

                    const int32 zHigher = (int32)(((float)z / vertexCountEdgeMip) * vertexCountEdgeMipHigher);
                    const int32 textureIndexHigherMip = (chunkTextureZHigher + zHigher) * textureSizeMipHigher + chunkTextureXHigher;
                    const byte* higherMipData = mipDataHigher + textureIndexHigherMip * pixelStride;

                    Platform::MemoryCopy(mipData + textureIndex * pixelStride, higherMipData, pixelStride);
                }

                for (; x < xCount; x++)
                {
                    const int32 textureIndex = chunkTextureZ * textureSizeMip + chunkTextureX + x;

                    const int32 xHigher = (int32)(((float)x / vertexCountEdgeMip) * vertexCountEdgeMipHigher);
                    const int32 textureIndexHigherMip = chunkTextureZHigher * textureSizeMipHigher + chunkTextureXHigher + xHigher;
                    const byte* higherMipData = mipDataHigher + textureIndexHigherMip * pixelStride;

                    Platform::MemoryCopy(mipData + textureIndex * pixelStride, higherMipData, pixelStride);
                }
            }
        }
    }
}

FORCE_INLINE byte GetPhysicalMaterial(const Color32& raw, const TerrainDataUpdateInfo& info, int32 chunkZ, int32 chunkX, int32 z, int32 x)
{
    byte result = 0;
    if (ReadIsHole(raw))
    {
        // Hole
        result = (uint8)PhysicsBackend::HeightFieldMaterial::Hole;
    }
    else if (info.SplatMaps[0])
    {
        // Use the layer with the highest influence (splatmap data is Mip0 so convert x/z coords back to LOD0)
        uint8 layer = 0;
        uint8 layerWeight = 0;
        const int32 splatmapTextureIndex = (chunkZ * info.ChunkSize + z) * info.HeightmapSize + chunkX * info.ChunkSize + x;
        ASSERT(splatmapTextureIndex < info.HeightmapLength);
        for (int32 splatIndex = 0; splatIndex < TERRAIN_MAX_SPLATMAPS_COUNT; splatIndex++)
        {
            for (int32 channelIndex = 0; channelIndex < 4; channelIndex++)
            {
                // Assume splatmap data pitch matches the row size and shift by channel index to simply sample at R chanel
                const Color32* splatmap = (const Color32*)((const byte*)info.SplatMaps[splatIndex] + channelIndex);
                const uint8 splat = splatmap[splatmapTextureIndex].R;
                if (splat > layerWeight)
                {
                    layer = splatIndex * 4 + channelIndex;
                    layerWeight = splat;
                    if (layerWeight == MAX_uint8)
                        break;
                }
            }
            if (layerWeight == MAX_uint8)
                break;
        }
        result = layer;
    }
    return result;
}

bool CookCollision(TerrainDataUpdateInfo& info, TextureBase::InitData* initData, int32 collisionLod, Array<byte>* collisionData)
{
#if COMPILE_WITH_PHYSICS_COOKING
    info.GetSplatMaps();
    PROFILE_CPU_NAMED("Terrain.CookCollision");

    // Prepare data
    const int32 collisionLOD = Math::Clamp<int32>(collisionLod, 0, initData->Mips.Count() - 1);
    const int32 collisionLODInv = (int32)Math::Pow(2.0f, (float)collisionLOD);
    const int32 heightFieldChunkSize = ((info.ChunkSize + 1) >> collisionLOD) - 1;
    const int32 heightFieldSize = heightFieldChunkSize * Terrain::ChunksCountEdge + 1;
    const int32 heightFieldLength = heightFieldSize * heightFieldSize;
    GET_TERRAIN_SCRATCH_BUFFER(heightFieldData, heightFieldLength, PhysicsBackend::HeightFieldSample);
    PhysicsBackend::HeightFieldSample sample;
    Platform::MemoryClear(&sample, sizeof(PhysicsBackend::HeightFieldSample));
    Platform::MemoryClear(heightFieldData, sizeof(PhysicsBackend::HeightFieldSample) * heightFieldLength);

    // Setup terrain collision information
    const auto& mip = initData->Mips[collisionLOD];
    const int32 vertexCountEdgeMip = info.VertexCountEdge >> collisionLOD;
    const int32 textureSizeMip = info.TextureSize >> collisionLOD;
    for (int32 chunkX = 0; chunkX < Terrain::ChunksCountEdge; chunkX++)
    {
        const int32 chunkTextureX = chunkX * vertexCountEdgeMip;
        const int32 chunkStartX = chunkX * heightFieldChunkSize;
        for (int32 chunkZ = 0; chunkZ < Terrain::ChunksCountEdge; chunkZ++)
        {
            const int32 chunkTextureZ = chunkZ * vertexCountEdgeMip;
            const int32 chunkStartZ = chunkZ * heightFieldChunkSize;
            for (int32 z = 0; z < vertexCountEdgeMip; z++)
            {
                const int32 heightmapZ = chunkStartZ + z;
                for (int32 x = 0; x < vertexCountEdgeMip; x++)
                {
                    const int32 heightmapX = chunkStartX + x;

                    const int32 textureIndex = (chunkTextureZ + z) * textureSizeMip + chunkTextureX + x;
                    const Color32 raw = mip.Data.Get<Color32>()[textureIndex];
                    sample.Height = int16(TERRAIN_PATCH_COLLISION_QUANTIZATION * ReadNormalizedHeight(raw));
                    sample.MaterialIndex0 = sample.MaterialIndex1 = GetPhysicalMaterial(raw, info, chunkZ, chunkX, z * collisionLODInv, x * collisionLODInv);

                    const int32 dstIndex = (heightmapX * heightFieldSize) + heightmapZ;
                    heightFieldData[dstIndex] = sample;
                }
            }
        }
    }

    // Cook height field
    MemoryWriteStream outputStream;
    if (CollisionCooking::CookHeightField(heightFieldSize, heightFieldSize, heightFieldData, outputStream))
        return true;

    // Write results
    collisionData->Resize(sizeof(TerrainCollisionDataHeader) + outputStream.GetPosition(), false);
    const auto header = (TerrainCollisionDataHeader*)collisionData->Get();
    header->CheckOldMagicNumber = MAX_int32;
    header->Version = TerrainCollisionDataHeader::CurrentVersion;
    header->LOD = collisionLOD;
    header->ScaleXZ = (float)info.HeightmapSize / heightFieldSize;
    Platform::MemoryCopy(collisionData->Get() + sizeof(TerrainCollisionDataHeader), outputStream.GetHandle(), outputStream.GetPosition());

    return false;

#else
    LOG(Warning, "Collision cooking is disabled.");
    return true;
#endif
}

bool ModifyCollision(TerrainDataUpdateInfo& info, TextureBase::InitData* initData, int32 collisionLod, const Int2& modifiedOffset, const Int2& modifiedSize, void* heightField)
{
    info.GetSplatMaps();
    PROFILE_CPU_NAMED("Terrain.ModifyCollision");

    // Prepare data
    const Vector2 modifiedOffsetRatio((float)modifiedOffset.X / info.HeightmapSize, (float)modifiedOffset.Y / info.HeightmapSize);
    const Vector2 modifiedSizeRatio((float)modifiedSize.X / info.HeightmapSize, (float)modifiedSize.Y / info.HeightmapSize);
    const int32 collisionLOD = Math::Clamp<int32>(collisionLod, 0, initData->Mips.Count() - 1);
    const int32 collisionLODInv = (int32)Math::Pow(2.0f, (float)collisionLOD);
    const int32 heightFieldChunkSize = ((info.ChunkSize + 1) >> collisionLOD) - 1;
    const int32 heightFieldSize = heightFieldChunkSize * Terrain::ChunksCountEdge + 1;
    const Int2 samplesOffset(Vector2::Floor(modifiedOffsetRatio * (float)heightFieldSize));
    Int2 samplesSize(Vector2::Ceil(modifiedSizeRatio * (float)heightFieldSize));
    samplesSize.X = Math::Max(samplesSize.X, 1);
    samplesSize.Y = Math::Max(samplesSize.Y, 1);
    Int2 samplesEnd = samplesOffset + samplesSize;
    samplesEnd.X = Math::Min(samplesEnd.X, heightFieldSize);
    samplesEnd.Y = Math::Min(samplesEnd.Y, heightFieldSize);

    // Allocate data
    const int32 heightFieldDataLength = samplesSize.X * samplesSize.Y;
    GET_TERRAIN_SCRATCH_BUFFER(heightFieldData, info.HeightmapLength, PhysicsBackend::HeightFieldSample);
    PhysicsBackend::HeightFieldSample sample;
    Platform::MemoryClear(&sample, sizeof(PhysicsBackend::HeightFieldSample));
    Platform::MemoryClear(heightFieldData, sizeof(PhysicsBackend::HeightFieldSample) * heightFieldDataLength);

    // Setup terrain collision information
    const auto& mip = initData->Mips[collisionLOD];
    const int32 vertexCountEdgeMip = info.VertexCountEdge >> collisionLOD;
    const int32 textureSizeMip = info.TextureSize >> collisionLOD;
    for (int32 chunkX = 0; chunkX < Terrain::ChunksCountEdge; chunkX++)
    {
        const int32 chunkTextureX = chunkX * vertexCountEdgeMip;
        const int32 chunkStartX = chunkX * heightFieldChunkSize;
        if (chunkStartX >= samplesEnd.X || chunkStartX + vertexCountEdgeMip < samplesOffset.X)
            continue; // Skip unmodified chunks

        for (int32 chunkZ = 0; chunkZ < Terrain::ChunksCountEdge; chunkZ++)
        {
            const int32 chunkTextureZ = chunkZ * vertexCountEdgeMip;
            const int32 chunkStartZ = chunkZ * heightFieldChunkSize;
            if (chunkStartZ >= samplesEnd.Y || chunkStartZ + vertexCountEdgeMip < samplesOffset.Y)
                continue; // Skip unmodified chunks

            // TODO: adjust loop range to reduce iterations count for edge cases (skip checking unmodified samples)
            for (int32 z = 0; z < vertexCountEdgeMip; z++)
            {
                const int32 heightmapZ = chunkStartZ + z;
                const int32 heightmapLocalZ = heightmapZ - samplesOffset.Y;
                if (heightmapLocalZ < 0 || heightmapLocalZ >= samplesSize.Y)
                    continue; // Skip unmodified columns

                // TODO: adjust loop range to reduce iterations count for edge cases (skip checking unmodified samples)
                for (int32 x = 0; x < vertexCountEdgeMip; x++)
                {
                    const int32 heightmapX = chunkStartX + x;
                    const int32 heightmapLocalX = heightmapX - samplesOffset.X;
                    if (heightmapLocalX < 0 || heightmapLocalX >= samplesSize.X)
                        continue; // Skip unmodified rows

                    const int32 textureIndex = (chunkTextureZ + z) * textureSizeMip + chunkTextureX + x;
                    const Color32 raw = mip.Data.Get<Color32>()[textureIndex];
                    sample.Height = int16(TERRAIN_PATCH_COLLISION_QUANTIZATION * ReadNormalizedHeight(raw));
                    sample.MaterialIndex0 = sample.MaterialIndex1 = GetPhysicalMaterial(raw, info, chunkZ, chunkX, z * collisionLODInv, x * collisionLODInv);

                    const int32 dstIndex = (heightmapLocalX * samplesSize.Y) + heightmapLocalZ;
                    heightFieldData[dstIndex] = sample;
                }
            }
        }
    }

    // Update height field range
    if (PhysicsBackend::ModifyHeightField(heightField, samplesOffset.Y, samplesOffset.X, samplesSize.Y, samplesSize.X, heightFieldData))
    {
        LOG(Warning, "Height Field collision modification failed.");
        return true;
    }

    return false;
}

#endif

#if TERRAIN_EDITING

bool TerrainPatch::SetupHeightMap(int32 heightMapLength, const float* heightMap, const byte* holesMask, bool forceUseVirtualStorage)
{
    PROFILE_CPU_NAMED("Terrain.Setup");
    if (heightMap == nullptr)
    {
        LOG(Warning, "Cannot create terrain without a heightmap specified.");
        return true;
    }
    TerrainDataUpdateInfo info(this);
    if (heightMapLength != info.HeightmapLength)
    {
        LOG(Warning, "Invalid heightmap length. Terrain of chunk size equal {0} uses heightmap of size {1}x{1} (heightmap array length must be {2}). Input heightmap has length {3}.", info.ChunkSize, info.HeightmapSize, info.HeightmapLength, heightMapLength);
        return true;
    }
    const PixelFormat pixelFormat = PixelFormat::R8G8B8A8_UNorm;

    // Input heightmap data overlaps on chunk edges but it needs to be duplicated for chunks (each chunk has own scale-bias for height values normalization)
    const int32 pixelStride = PixelFormatExtensions::SizeInBytes(pixelFormat);
    const int32 lodCount = Math::Min<int32>(_terrain->_lodCount, MipLevelsCount(info.VertexCountEdge) - 2);

    // Process heightmap to get per-patch height normalization values
    float chunkOffsets[Terrain::ChunksCount];
    float chunkHeights[Terrain::ChunksCount];
    CalculateHeightmapRange(_terrain, info, heightMap, chunkOffsets, chunkHeights);

    // Prepare
#if USE_EDITOR
    const bool useVirtualStorage = Editor::IsPlayMode || forceUseVirtualStorage;
#else
    const bool useVirtualStorage = true;
#endif
#if USE_EDITOR
    String heightMapPath, heightFieldPath;
    if (!useVirtualStorage)
    {
        if (!_terrain->GetScene())
        {
            LOG(Error, "Cannot create non-virtual terrain. Add terrain actor to scene first (needs scene folder path for target assets location).");
            return true;
        }
        const String cacheDir = _terrain->GetScene()->GetDataFolderPath() / TEXT("Terrain/") / _terrain->GetID().ToString(Guid::FormatType::N);

        // Prepare asset paths for the non-virtual assets
        heightMapPath = cacheDir + String::Format(TEXT("_{0:2}_{1:2}_Heightmap.{2}"), _x, _z, ASSET_FILES_EXTENSION);
        heightFieldPath = cacheDir + String::Format(TEXT("_{0:2}_{1:2}_Heightfield.{2}"), _x, _z, ASSET_FILES_EXTENSION);
    }
#endif

    // Create heightmap texture data source container
    auto initData = New<TextureBase::InitData>();
    initData->Format = pixelFormat;
    initData->Width = info.TextureSize;
    initData->Height = info.TextureSize;
    initData->ArraySize = 1;
    initData->Mips.Resize(lodCount);

    // Allocate top mip data
    {
        PROFILE_CPU_NAMED("Terrain.AllocateHeightmap");
        auto& mip = initData->Mips[0];
        mip.RowPitch = info.TextureSize * pixelStride;
        mip.SlicePitch = mip.RowPitch * info.TextureSize;
        mip.Data.Allocate(mip.SlicePitch);
    }

    // Create heightmap LOD0 data
    {
        const auto mipLOD0Data = initData->Mips[0].Data.Get();
        UpdateHeightMap(info, heightMap, mipLOD0Data);
        UpdateNormalsAndHoles(info, heightMap, holesMask, mipLOD0Data);
    }

    // Downscale mip data for all lower LODs
    if (GenerateMips(initData))
    {
        Delete(initData);
        return true;
    }

    // Fix generated mip maps to keep the same values for chunk edges (reduce cracks on continuous LOD transitions)
    FixMips(info, initData, pixelStride);

    // Save the heightmap data to the asset
    if (useVirtualStorage)
    {
        // Check if texture is missing or it is not virtual
        Texture* texture = Heightmap.Get();
        if (texture == nullptr || !texture->IsVirtual())
        {
            // Create new virtual texture
            texture = Content::CreateVirtualAsset<Texture>();
            if (texture == nullptr)
            {
                LOG(Warning, "Failed to create virtual heightmap texture.");
                return true;
            }
            Heightmap = texture;
        }

        // Initialize the texture (data will be streamed)
        if (texture->Init(initData))
        {
            Delete(initData);
            LOG(Warning, "Failed to initialize virtual heightmap texture.");
            return true;
        }
    }
#if COMPILE_WITH_ASSETS_IMPORTER
    else
    {
        // Import data to the asset file
        Guid id = Guid::New();
        if (AssetsImportingManager::Create(AssetsImportingManager::CreateTextureAsInitDataTag, heightMapPath, id, initData))
        {
            LOG(Error, "Cannot import generated heightmap texture asset.");
            return true;
        }
        Heightmap = Content::LoadAsync<Texture>(id);
        if (Heightmap == nullptr)
        {
            LOG(Error, "Cannot load generated heightmap texture asset.");
            return true;
        }
    }
#else
    else
    {
        // Not supported
        CRASH;
    }
#endif

    // Prepare collision data destination container
    Array<byte> tmpData;
    Array<byte>* collisionData;
    if (useVirtualStorage)
    {
        // Check if asset is missing or it is not virtual
        RawDataAsset* collision = _heightfield.Get();
        if (collision == nullptr || !collision->IsVirtual())
        {
            // Create new virtual container
            collision = Content::CreateVirtualAsset<RawDataAsset>();
            if (collision == nullptr)
            {
                LOG(Warning, "Failed to create virtual heightfield container.");
                return true;
            }
            _heightfield = collision;
        }

        // Write directly to the virtual asset storage
        collisionData = &collision->Data;
    }
    else
    {
        // Write to the temporary array (that is later imported to the asset)
        collisionData = &tmpData;
    }

    // Generate physics backend height field data for the runtime
    if (CookCollision(info, initData, _terrain->_collisionLod, collisionData))
    {
        return true;
    }

#if COMPILE_WITH_ASSETS_IMPORTER
    if (!useVirtualStorage)
    {
        // Import data to the asset file
        Guid id = Guid::New();
        BytesContainer bytesContainer;
        bytesContainer.Link(tmpData.Get(), tmpData.Count());
        if (AssetsImportingManager::Create(AssetsImportingManager::CreateRawDataTag, heightFieldPath, id, &bytesContainer))
        {
            LOG(Error, "Cannot import generated heightfield collision asset.");
            return true;
        }
        _heightfield = Content::LoadAsync<RawDataAsset>(id);
        if (_heightfield == nullptr)
        {
            LOG(Error, "Cannot load generated heightfield collision asset.");
            return true;
        }
    }
#endif

    // Update data
    _yOffset = info.PatchOffset;
    _yHeight = info.PatchHeight;
    for (int32 chunkIndex = 0; chunkIndex < Terrain::ChunksCount; chunkIndex++)
    {
        auto& chunk = Chunks[chunkIndex];
        chunk._yOffset = chunkOffsets[chunkIndex];
        chunk._yHeight = chunkHeights[chunkIndex];
        chunk.UpdateTransform();
    }
    UpdateCollision();
    _terrain->UpdateBounds();
    _terrain->UpdateLayerBits();

#if TERRAIN_UPDATING
    // Invalidate cache
    _cachedHeightMap.Resize(0);
    _cachedHolesMask.Resize(0);
    _wasHeightModified = false;
#endif

    return false;
}

bool TerrainPatch::SetupSplatMap(int32 index, int32 splatMapLength, const Color32* splatMap, bool forceUseVirtualStorage)
{
    PROFILE_CPU_NAMED("Terrain.SetupSplatMap");
    CHECK_RETURN(index >= 0 && index < TERRAIN_MAX_SPLATMAPS_COUNT, true);
    if (splatMap == nullptr)
    {
        LOG(Warning, "Cannot create terrain without any splatmap specified.");
        return true;
    }
    TerrainDataUpdateInfo info(this, _yOffset, _yHeight);
    if (splatMapLength != info.HeightmapLength)
    {
        LOG(Warning, "Invalid splatmap length. Terrain of chunk size equal {0} uses heightmap of size {1}x{1} (heightmap array length must be {2}). Input heightmap has length {3}.", info.ChunkSize, info.HeightmapSize, info.HeightmapLength, splatMapLength);
        return true;
    }
    const PixelFormat pixelFormat = PixelFormat::R8G8B8A8_UNorm;

    // Ensure that terrain has a valid heightmap
    if (Heightmap == nullptr)
    {
        if (InitializeHeightMap() || Heightmap == nullptr)
        {
            LOG(Warning, "Cannot modify splatmap without valid heightmap loaded.");
            return true;
        }
    }

    // Input splatmap data overlaps on chunk edges but it needs to be duplicated for chunks
    const int32 pixelStride = PixelFormatExtensions::SizeInBytes(pixelFormat);
    const int32 lodCount = Math::Min<int32>(_terrain->_lodCount, MipLevelsCount(info.VertexCountEdge) - 2);

    // Prepare
#if USE_EDITOR
    const bool useVirtualStorage = Editor::IsPlayMode || forceUseVirtualStorage;
#else
    const bool useVirtualStorage = true;
#endif
#if USE_EDITOR
    String splatMapPath;
    if (!useVirtualStorage)
    {
        if (!_terrain->GetScene())
        {
            LOG(Error, "Cannot create non-virtual terrain. Add terrain actor to scene first (needs scene folder path for target assets location).");
            return true;
        }
        const String cacheDir = _terrain->GetScene()->GetDataFolderPath() / TEXT("Terrain/") / _terrain->GetID().ToString(Guid::FormatType::N);

        // Prepare asset path for the non-virtual assets
        splatMapPath = cacheDir + String::Format(TEXT("_{0:2}_{1:2}_Splatmap{3}.{2}"), _x, _z, ASSET_FILES_EXTENSION, index);
    }
#endif

    // Create heightmap texture data source container
    auto initData = New<TextureBase::InitData>();
    initData->Format = pixelFormat;
    initData->Width = info.TextureSize;
    initData->Height = info.TextureSize;
    initData->ArraySize = 1;
    initData->Mips.Resize(lodCount);

    // Allocate top mip data
    {
        PROFILE_CPU_NAMED("Terrain.AllocateSplatmap");
        auto& mip = initData->Mips[0];
        mip.RowPitch = info.TextureSize * pixelStride;
        mip.SlicePitch = mip.RowPitch * info.TextureSize;
        mip.Data.Allocate(mip.SlicePitch);
    }

    // Create splatmap LOD0 data
    {
        const auto mipLOD0Data = initData->Mips[0].Data.Get();
        UpdateSplatMap(info, splatMap, mipLOD0Data);
    }

    // Downscale mip data for all lower LODs
    if (GenerateMips(initData))
    {
        Delete(initData);
        return true;
    }

    // Fix generated mip maps to keep the same values for chunk edges (reduce cracks on continuous LOD transitions)
    FixMips(info, initData, pixelStride);

    // Save the splatmap data to the asset
    auto& splatmapAsset = Splatmap[index];
    if (useVirtualStorage)
    {
        // Check if texture is missing or it is not virtual
        Texture* texture = splatmapAsset.Get();
        if (texture == nullptr || !texture->IsVirtual())
        {
            // Create new virtual texture
            texture = Content::CreateVirtualAsset<Texture>();
            if (texture == nullptr)
            {
                LOG(Warning, "Failed to create virtual splatmap texture.");
                return true;
            }
            splatmapAsset = texture;
        }

        // Initialize the texture (data will be streamed)
        if (texture->Init(initData))
        {
            Delete(initData);
            LOG(Warning, "Failed to initialize virtual splatmap texture.");
            return true;
        }
    }
#if COMPILE_WITH_ASSETS_IMPORTER
    else
    {
        // Import data to the asset file
        Guid id = Guid::New();
        if (AssetsImportingManager::Create(AssetsImportingManager::CreateTextureAsInitDataTag, splatMapPath, id, initData))
        {
            LOG(Error, "Cannot import generated splatmap texture asset.");
            return true;
        }
        splatmapAsset = Content::LoadAsync<Texture>(id);
        if (splatmapAsset == nullptr)
        {
            LOG(Error, "Cannot load generated splatmap texture asset.");
            return true;
        }
    }
#else
    else
    {
        // Not supported
        CRASH;
    }
#endif

#if TERRAIN_UPDATING
    // Invalidate cache
    _cachedSplatMap[index].Resize(0);
    _wasSplatmapModified[index] = false;
#endif

    return false;
}

#endif

bool TerrainPatch::InitializeHeightMap()
{
    PROFILE_CPU_NAMED("Terrain.InitializeHeightMap");
    const auto heightmapSize = _terrain->GetChunkSize() * Terrain::ChunksCountEdge + 1;
    Array<float> heightmap;
    heightmap.Resize(heightmapSize * heightmapSize);
    heightmap.SetAll(0.0f);
    return SetupHeightMap(heightmap.Count(), heightmap.Get());
}

#if TERRAIN_UPDATING

float* TerrainPatch::GetHeightmapData()
{
    PROFILE_CPU_NAMED("Terrain.GetHeightmapData");
    if (_cachedHeightMap.HasItems())
        return _cachedHeightMap.Get();
    CacheHeightData();
    return _cachedHeightMap.Get();
}

void TerrainPatch::ClearHeightmapCache()
{
    PROFILE_CPU_NAMED("Terrain.ClearHeightmapCache");
    _cachedHeightMap.Clear();
}

byte* TerrainPatch::GetHolesMaskData()
{
    PROFILE_CPU_NAMED("Terrain.GetHolesMaskData");
    if (_cachedHolesMask.HasItems())
        return _cachedHolesMask.Get();
    CacheHeightData();
    return _cachedHolesMask.Get();
}

void TerrainPatch::ClearHolesMaskCache()
{
    PROFILE_CPU_NAMED("Terrain.ClearHolesMaskCache");
    _cachedHolesMask.Clear();
}

Color32* TerrainPatch::GetSplatMapData(int32 index)
{
    CHECK_RETURN(index >= 0 && index < TERRAIN_MAX_SPLATMAPS_COUNT, nullptr);
    PROFILE_CPU_NAMED("Terrain.GetSplatMapData");
    if (_cachedSplatMap[index].HasItems())
        return _cachedSplatMap[index].Get();
    CacheSplatData();
    return _cachedSplatMap[index].Get();
}

void TerrainPatch::ClearSplatMapCache()
{
    PROFILE_CPU_NAMED("Terrain.ClearSplatMapCache");
    for (int32 i = 0; i < TERRAIN_MAX_SPLATMAPS_COUNT; i++)
        _cachedSplatMap[i].Clear();
}

void TerrainPatch::ClearCache()
{
    ClearHeightmapCache();
    ClearHolesMaskCache();
    ClearSplatMapCache();
}

void TerrainPatch::CacheHeightData()
{
    PROFILE_CPU_NAMED("Terrain.CacheHeightData");
    const TerrainDataUpdateInfo info(this);

    // Ensure that heightmap data is all loaded
    // TODO: disable streaming for heightmap texture if it's being modified by the editor
    if (Heightmap->WaitForLoaded())
    {
        LOG(Error, "Failed to load patch heightmap data.");
        return;
    }

    // Get the LOD0 mip map data and extract the heightmap
    auto lock = Heightmap->LockData();
    BytesContainer mipLOD0;
    Heightmap->GetMipDataWithLoading(0, mipLOD0);
    if (mipLOD0.IsInvalid())
    {
        LOG(Error, "Failed to get patch heightmap data.");
        return;
    }

    // Allocate data
    _cachedHeightMap.Resize(info.HeightmapLength);
    _cachedHolesMask.Resize(info.HeightmapLength);
    _wasHeightModified = false;

    // Extract heightmap data and denormalize it to get the pure height field
    const float patchOffset = _yOffset;
    const float patchHeight = _yHeight;
    const auto heightmapPtr = _cachedHeightMap.Get();
    const auto holesMaskPtr = _cachedHolesMask.Get();
    for (int32 chunkIndex = 0; chunkIndex < Terrain::ChunksCount; chunkIndex++)
    {
        const int32 chunkTextureX = Chunks[chunkIndex]._x * info.VertexCountEdge;
        const int32 chunkTextureZ = Chunks[chunkIndex]._z * info.VertexCountEdge;

        const int32 chunkHeightmapX = Chunks[chunkIndex]._x * info.ChunkSize;
        const int32 chunkHeightmapZ = Chunks[chunkIndex]._z * info.ChunkSize;

        for (int32 z = 0; z < info.VertexCountEdge; z++)
        {
            const int32 tz = (chunkTextureZ + z) * info.TextureSize;
            const int32 sz = (chunkHeightmapZ + z) * info.HeightmapSize;

            for (int32 x = 0; x < info.VertexCountEdge; x++)
            {
                const int32 tx = chunkTextureX + x;
                const int32 sx = chunkHeightmapX + x;
                const int32 textureIndex = tz + tx;
                const int32 heightmapIndex = sz + sx;

                const Color32 raw = mipLOD0.Get<Color32>()[textureIndex];
                const float normalizedHeight = ReadNormalizedHeight(raw);
                const float height = (normalizedHeight * patchHeight) + patchOffset;
                const bool isHole = ReadIsHole(raw);

                heightmapPtr[heightmapIndex] = height;
                holesMaskPtr[heightmapIndex] = isHole ? 0 : 255;
            }
        }
    }
}

void TerrainPatch::CacheSplatData()
{
    PROFILE_CPU_NAMED("Terrain.CacheSplatData");
    const TerrainDataUpdateInfo info(this);

    // Cache all the splatmaps
    for (int32 index = 0; index < TERRAIN_MAX_SPLATMAPS_COUNT; index++)
    {
        // Allocate data
        _cachedSplatMap[index].Resize(info.HeightmapLength);
        _wasSplatmapModified[index] = false;

        // Skip if has missing splatmap asset
        if (Splatmap[index] == nullptr)
        {
            // Initialize splatmap (fill with the first layer if it's the first splatmap)
            const auto fillColor = index == 0 ? Color32(255, 0, 0, 0) : Color32::Transparent;
            _cachedSplatMap[index].SetAll(fillColor);
            continue;
        }

        // Ensure that splatmap data is all loaded
        // TODO: disable streaming for heightmap texture if it's being modified by the editor
        if (Splatmap[index]->WaitForLoaded())
        {
            LOG(Error, "Failed to load patch splatmap data.");
            continue;
        }

        // Get the LOD0 mip map data and extract the splatmap
        auto lock = Splatmap[index]->LockData();
        BytesContainer mipLOD0;
        Splatmap[index]->GetMipDataWithLoading(0, mipLOD0);
        if (mipLOD0.IsInvalid())
        {
            LOG(Error, "Failed to get patch splatmap data.");
            continue;
        }

        // Extract splatmap data
        const auto splatMapPtr = static_cast<Color32*>(_cachedSplatMap[index].Get());
        for (int32 chunkIndex = 0; chunkIndex < Terrain::ChunksCount; chunkIndex++)
        {
            const int32 chunkTextureX = Chunks[chunkIndex]._x * info.VertexCountEdge;
            const int32 chunkTextureZ = Chunks[chunkIndex]._z * info.VertexCountEdge;

            const int32 chunkHeightmapX = Chunks[chunkIndex]._x * info.ChunkSize;
            const int32 chunkHeightmapZ = Chunks[chunkIndex]._z * info.ChunkSize;

            for (int32 z = 0; z < info.VertexCountEdge; z++)
            {
                const int32 tz = (chunkTextureZ + z) * info.TextureSize;
                const int32 sz = (chunkHeightmapZ + z) * info.HeightmapSize;

                for (int32 x = 0; x < info.VertexCountEdge; x++)
                {
                    const int32 tx = chunkTextureX + x;
                    const int32 sx = chunkHeightmapX + x;
                    const int32 textureIndex = tz + tx;
                    const int32 heightmapIndex = sz + sx;

                    splatMapPtr[heightmapIndex] = mipLOD0.Get<Color32>()[textureIndex];
                }
            }
        }
    }
}

bool TerrainPatch::ModifyHeightMap(const float* samples, const Int2& modifiedOffset, const Int2& modifiedSize)
{
    // Validate input samples range
    TerrainDataUpdateInfo info(this);
    if (samples == nullptr)
    {
        LOG(Warning, "Missing heightmap samples data.");
        return true;
    }
    if (modifiedOffset.X < 0 || modifiedOffset.Y < 0 ||
        modifiedSize.X <= 0 || modifiedSize.Y <= 0 ||
        modifiedOffset.X + modifiedSize.X > info.HeightmapSize ||
        modifiedOffset.Y + modifiedSize.Y > info.HeightmapSize)
    {
        LOG(Warning, "Invalid heightmap samples range.");
        return true;
    }
    PROFILE_CPU_NAMED("Terrain.ModifyHeightMap");

    // Check if has no heightmap
    if (Heightmap == nullptr)
    {
        // Initialize with flat heightmap data
        if (InitializeHeightMap())
        {
            LOG(Error, "Failed to initialize patch heightmap for modification.");
            return true;
        }
    }

    // Get the current data to modify it
    float* heightMap = GetHeightmapData();
    if (samples == heightMap)
    {
        LOG(Warning, "Updating terrain with its own data. Oh god xD");
    }

    // Modify heightmap data
    {
        PROFILE_CPU_NAMED("Terrain.WrtieCache");
        for (int32 z = 0; z < modifiedSize.Y; z++)
        {
            // TODO: use batches row mem copy

            for (int32 x = 0; x < modifiedSize.X; x++)
            {
                heightMap[(z + modifiedOffset.Y) * info.HeightmapSize + (x + modifiedOffset.X)] = samples[z * modifiedSize.X + x];
            }
        }
    }

    // Process heightmap to get per-patch height normalization values
    float chunkOffsets[Terrain::ChunksCount];
    float chunkHeights[Terrain::ChunksCount];
    CalculateHeightmapRange(_terrain, info, heightMap, chunkOffsets, chunkHeights);
    // TODO: maybe calculate chunk ranges for only modified chunks
    const bool wasHeightRangeChanged = Math::NotNearEqual(_yOffset, info.PatchOffset) || Math::NotNearEqual(_yHeight, info.PatchHeight);

    // Check if has allocated texture
    if (_dataHeightmap)
    {
        auto holesMask = GetHolesMaskData();
        const auto data = _dataHeightmap->Mips[0].Data.Get();

        // Update the heightmap storage
        if (wasHeightRangeChanged)
        {
            // Slower path that updates the whole heightmap (height range has been modified)
            UpdateHeightMap(info, heightMap, data);
        }
        else
        {
            // Faster path that updates only modified samples range
            UpdateHeightMap(info, heightMap, modifiedOffset, modifiedSize, data);
        }

        // Calculate per heightmap vertex smooth normal vectors
        UpdateNormalsAndHoles(info, heightMap, holesMask, modifiedOffset, modifiedSize, data);
    }

    // Update all the stuff
    _yOffset = info.PatchOffset;
    _yHeight = info.PatchHeight;
    for (int32 chunkIndex = 0; chunkIndex < Terrain::ChunksCount; chunkIndex++)
    {
        auto& chunk = Chunks[chunkIndex];
        chunk._yOffset = chunkOffsets[chunkIndex];
        chunk._yHeight = chunkHeights[chunkIndex];
        chunk.UpdateTransform();
    }
    _terrain->UpdateBounds();
    return UpdateHeightData(info, modifiedOffset, modifiedSize, wasHeightRangeChanged, true);
}

bool TerrainPatch::ModifyHolesMask(const byte* samples, const Int2& modifiedOffset, const Int2& modifiedSize)
{
    // Validate input samples range
    TerrainDataUpdateInfo info(this, _yOffset, _yHeight);
    if (samples == nullptr)
    {
        LOG(Warning, "Missing holes mask samples data.");
        return true;
    }
    if (modifiedOffset.X < 0 || modifiedOffset.Y < 0 ||
        modifiedSize.X <= 0 || modifiedSize.Y <= 0 ||
        modifiedOffset.X + modifiedSize.X > info.HeightmapSize ||
        modifiedOffset.Y + modifiedSize.Y > info.HeightmapSize)
    {
        LOG(Warning, "Invalid holes mask samples range.");
        return true;
    }
    PROFILE_CPU_NAMED("Terrain.ModifyHolesMask");

    // Check if has no heightmap
    if (Heightmap == nullptr)
    {
        // Initialize with flat heightmap data
        if (InitializeHeightMap())
        {
            LOG(Error, "Failed to initialize patch heightmap for modification.");
            return true;
        }
    }

    // Get the current data to modify it
    auto holesMask = GetHolesMaskData();
    if (samples == holesMask)
    {
        LOG(Warning, "Updating terrain with its own data. Oh god xD");
    }

    // Modify holes mask data
    {
        PROFILE_CPU_NAMED("Terrain.WrtieCache");
        for (int32 z = 0; z < modifiedSize.Y; z++)
        {
            // TODO: use batches row mem copy

            for (int32 x = 0; x < modifiedSize.X; x++)
            {
                holesMask[(z + modifiedOffset.Y) * info.HeightmapSize + (x + modifiedOffset.X)] = samples[z * modifiedSize.X + x];
            }
        }
    }

    // Check if has allocated texture
    if (_dataHeightmap)
    {
        float* heightMap = GetHeightmapData();
        const auto data = _dataHeightmap->Mips[0].Data.Get();

        // Calculate per heightmap vertex smooth normal vectors and update holes mask
        UpdateNormalsAndHoles(info, heightMap, holesMask, modifiedOffset, modifiedSize, data);
    }

    // Update all the stuff
    return UpdateHeightData(info, modifiedOffset, modifiedSize, false, true);
}

bool TerrainPatch::ModifySplatMap(int32 index, const Color32* samples, const Int2& modifiedOffset, const Int2& modifiedSize)
{
    ASSERT(index >= 0 && index < TERRAIN_MAX_SPLATMAPS_COUNT);

    // Ensure that terrain has a valid heightmap
    if (Heightmap == nullptr)
    {
        if (InitializeHeightMap() || Heightmap == nullptr)
        {
            LOG(Warning, "Cannot modify splatmap without valid heightmap loaded.");
            return true;
        }
    }

    // Validate input samples range
    TerrainDataUpdateInfo info(this, _yOffset, _yHeight);
    if (samples == nullptr)
    {
        LOG(Warning, "Missing splatmap samples data.");
        return true;
    }
    if (modifiedOffset.X < 0 || modifiedOffset.Y < 0 ||
        modifiedSize.X <= 0 || modifiedSize.Y <= 0 ||
        modifiedOffset.X + modifiedSize.X > info.HeightmapSize ||
        modifiedOffset.Y + modifiedSize.Y > info.HeightmapSize)
    {
        LOG(Warning, "Invalid heightmap samples range.");
        return true;
    }
    PROFILE_CPU_NAMED("Terrain.ModifySplatMap");

    // Get the current data to modify it
    Color32* splatMap = GetSplatMapData(index);
    if (samples == splatMap)
    {
        LOG(Warning, "Updating terrain with its own data. Oh god xD");
    }

    // Modify splat map data
    {
        PROFILE_CPU_NAMED("Terrain.WrtieCache");
        for (int32 z = 0; z < modifiedSize.Y; z++)
        {
            // TODO: use batches row mem copy

            for (int32 x = 0; x < modifiedSize.X; x++)
            {
                splatMap[(z + modifiedOffset.Y) * info.HeightmapSize + (x + modifiedOffset.X)] = samples[z * modifiedSize.X + x];
            }
        }
    }

    // Initialize data container if need to
    auto& splatmap = Splatmap[index];
    auto& dataSplatmap = _dataSplatmap[index];
    if (dataSplatmap == nullptr)
    {
        PROFILE_CPU_NAMED("Terrain.InitDataStorage");
        if (Heightmap->WaitForLoaded())
        {
            LOG(Error, "Failed to load heightmap.");
            return true;
        }

        // Use heightmap properties to match texture size and mip maps count
        const auto heightmap = Heightmap->StreamingTexture();
        const int32 textureSize = heightmap->TotalWidth();
        const int32 lodCount = heightmap->TotalMipLevels();

        // Prepare storage for splatmap saving to file and uploading to GPU
        dataSplatmap = New<TextureBase::InitData>();
        dataSplatmap->Format = PixelFormat::R8G8B8A8_UNorm;
        dataSplatmap->Width = textureSize;
        dataSplatmap->Height = textureSize;
        dataSplatmap->ArraySize = 1;
        dataSplatmap->Mips.Resize(lodCount);

        // Initialize top mip container
        auto& mip = dataSplatmap->Mips[0];
        mip.RowPitch = textureSize * sizeof(Color32);
        mip.SlicePitch = mip.RowPitch * textureSize;
        mip.Data.Allocate(mip.SlicePitch);
    }

    // Update splat map storage data
    const bool hasSplatmap = splatmap;
    const auto splatmapData = dataSplatmap->Mips[0].Data.Get();
    if (hasSplatmap)
        UpdateSplatMap(info, splatMap, modifiedOffset, modifiedSize, splatmapData);
    else
        UpdateSplatMap(info, splatMap, splatmapData);

    // Downscale mip data for all lower LODs
    if (GenerateMips(dataSplatmap))
    {
        return true;
    }

    // Fix generated mip maps to keep the same values for chunk edges (reduce cracks on continuous LOD transitions)
    FixMips(info, dataSplatmap, sizeof(Color32));

    // Update the resource (upload data to the GPU or create a new splatmap asset if missing)
    if (hasSplatmap)
    {
        // Ensure that splatmap data is all loaded
        if (splatmap->WaitForLoaded())
        {
            LOG(Error, "Failed to load patch splatmap data.");
            return true;
        }

        // Update terrain texture (on a GPU)
        for (int32 mipIndex = 0; mipIndex < dataSplatmap->Mips.Count(); mipIndex++)
        {
            auto t = splatmap->GetTexture();
            if (!t->IsAllocated())
            {
                LOG(Warning, "Failed to update splatmap texture. It's not allocated.");
                continue;
            }
            auto task = t->UploadMipMapAsync(dataSplatmap->Mips[mipIndex].Data, mipIndex);
            if (task)
                task->Start();
        }
    }
    else
    {
#if USE_EDITOR
        const bool useVirtualStorage = Editor::IsPlayMode || Heightmap->IsVirtual();
#else
        const bool useVirtualStorage = true;
#endif

        // Save the splatmap data to the asset
        if (useVirtualStorage)
        {
            // Create new virtual texture
            auto texture = Content::CreateVirtualAsset<Texture>();
            if (texture == nullptr)
            {
                LOG(Warning, "Failed to create virtual splatmap texture.");
                return true;
            }
            splatmap = texture;

            // Initialize the texture (data will be streamed)
            if (texture->Init(dataSplatmap))
            {
                LOG(Warning, "Failed to initialize virtual splatmap texture.");
                return true;
            }
        }
#if COMPILE_WITH_ASSETS_IMPORTER
        else
        {
            // Prepare asset path for the non-virtual asset
            const String cacheDir = String(StringUtils::GetDirectoryName(Heightmap->GetPath())) / _terrain->GetID().ToString(Guid::FormatType::N);
            const String splatMapPath = cacheDir + String::Format(TEXT("_{0:2}_{1:2}_Splatmap{3}.{2}"), _x, _z, ASSET_FILES_EXTENSION, index);

            // Import data to the asset file
            Guid id = Guid::New();
            if (AssetsImportingManager::Create(AssetsImportingManager::CreateTextureAsInitDataTag, splatMapPath, id, dataSplatmap))
            {
                LOG(Error, "Cannot import generated splatmap texture asset.");
                return true;
            }
            splatmap = Content::LoadAsync<Texture>(id);
            if (splatmap == nullptr)
            {
                LOG(Error, "Cannot load generated splatmap texture asset.");
                return true;
            }
        }
#else
        else
        {
            // Not supported
            CRASH;
        }
#endif
    }

    // Mark as modified (need to save texture data during scene saving)
    _wasSplatmapModified[index] = true;

    // Note: if terrain is using virtual storage then it won't be updated, we could synchronize that data...

    // TODO: disable splatmap dynamic streaming - data on a GPU was modified and we don't want to override it with the old data stored in the asset container

    // Update heightfield to reflect physical materials layering
    if (info.UsePhysicalMaterials() && HasCollision())
    {
        UpdateHeightData(info, modifiedOffset, modifiedSize, false, false);
    }

    return false;
}

bool TerrainPatch::UpdateHeightData(TerrainDataUpdateInfo& info, const Int2& modifiedOffset, const Int2& modifiedSize, bool wasHeightRangeChanged, bool wasHeightChanged)
{
    PROFILE_CPU();
    float* heightMap = GetHeightmapData();
    byte* holesMask = GetHolesMaskData();
    ASSERT(heightMap && holesMask);

    // Prepare data for the uploading to GPU
    ASSERT(Heightmap);
    auto texture = Heightmap->GetTexture();
    ASSERT(texture->ResidentMipLevels() > 0);
    const int32 textureSize = texture->Width();
    const PixelFormat pixelFormat = texture->Format();
    const int32 pixelStride = PixelFormatExtensions::SizeInBytes(pixelFormat);
    const int32 lodCount = texture->MipLevels();
    if (_dataHeightmap == nullptr)
    {
        // Setup
        _dataHeightmap = New<TextureBase::InitData>();
        _dataHeightmap->Format = pixelFormat;
        _dataHeightmap->Width = textureSize;
        _dataHeightmap->Height = textureSize;
        _dataHeightmap->ArraySize = 1;
        _dataHeightmap->Mips.Resize(lodCount);

        // Allocate top level mip
        auto& mip = _dataHeightmap->Mips[0];
        mip.RowPitch = textureSize * pixelStride;
        mip.SlicePitch = mip.RowPitch * textureSize;
        mip.Data.Allocate(mip.SlicePitch);

        // Generate full data on first usage (need to get valid normals and update the whole heightmap region)
        UpdateHeightMap(info, heightMap, mip.Data.Get());
        UpdateNormalsAndHoles(info, heightMap, holesMask, mip.Data.Get());
    }

    // Downscale mip data for all lower LODs
    if (GenerateMips(_dataHeightmap))
        return true;

    // Fix generated mip maps to keep the same values for chunk edges (reduce cracks on continuous LOD transitions)
    FixMips(info, _dataHeightmap, pixelStride);

    // Update terrain texture (on a GPU)
    for (int32 mipIndex = 0; mipIndex < _dataHeightmap->Mips.Count(); mipIndex++)
    {
        auto task = texture->UploadMipMapAsync(_dataHeightmap->Mips[mipIndex].Data, mipIndex);
        if (task)
            task->Start();
    }

#if 1
    if (wasHeightRangeChanged)
    {
        // When min-max height range has been changed for the patch let's update it all, it's faster to cook collision and rebuild shape rather than modify all the samples
        if (_heightfield->WaitForLoaded())
        {
            LOG(Error, "Failed to load patch heightfield data.");
            return true;
        }
        const auto collisionData = &_heightfield->Data;
        if (CookCollision(info, _dataHeightmap, _terrain->_collisionLod, collisionData))
            return true;
        UpdateCollision();
    }
    else
    {
        ScopeLock lock(_collisionLocker);
        if (ModifyCollision(info, _dataHeightmap, _terrain->_collisionLod, modifiedOffset, modifiedSize, _physicsHeightField))
            return true;
        if (wasHeightChanged)
            UpdateCollisionScale();
    }
#else
	// Modify heightfield samples (without cooking collision which is done on a separate async task)
	if (HasCollision() && _physicsHeightField)
	{
        ScopeLock lock(_collisionLocker);
		if (wasHeightRangeChanged)
		{
			if (ModifyCollision(info, _dataHeightmap, _terrain->_collisionLod, Int2::Zero, Int2(info.HeightmapSize), _physicsHeightField))
				return true;
		}
		else
		{
			if (ModifyCollision(info, _dataHeightmap, _terrain->_collisionLod, modifiedOffset, modifiedSize, _physicsHeightField))
				return true;
		}

		UpdateCollisionScale();
	}
#endif

    // Mark as modified (need to save texture data during scene saving)
    _wasHeightModified = true;

    if (!wasHeightChanged)
        return false;

    // Invalidate cache
#if TERRAIN_USE_PHYSICS_DEBUG
    _debugLinesDirty = true;
#endif
#if USE_EDITOR
    _collisionTriangles.Resize(0);
#endif
    _collisionVertices.Resize(0);

    // Note: if terrain is using virtual storage then it won't be updated, we could synchronize that data...

    // TODO: disable heightmap dynamic streaming - data on a GPU was modified and we don't want to override it with the old data stored in the asset container

    return false;
}

void TerrainPatch::SaveHeightData()
{
#if USE_EDITOR
    // Skip if was not modified or cannot be saved
    if (!_wasHeightModified ||
        Heightmap == nullptr ||
        _heightfield == nullptr ||
        Heightmap->IsVirtual() ||
        _heightfield->IsVirtual() ||
        _dataHeightmap == nullptr)
    {
        return;
    }
    PROFILE_CPU_NAMED("Terrain.Save");
    TerrainDataUpdateInfo info(this, _yOffset, _yHeight);

    // Save heightmap to asset
    if (Heightmap->WaitForLoaded())
    {
        LOG(Error, "Failed to load patch heightmap data.");
        return;
    }
    if (Heightmap->Save(String::Empty, _dataHeightmap))
    {
        LOG(Error, "Failed to save heightmap data to asset.");
        return;
    }

    // Generate physics backend height field data for the runtime
    if (_heightfield->WaitForLoaded())
    {
        LOG(Error, "Failed to load patch heightfield data.");
        return;
    }
    const auto collisionData = &_heightfield->Data;
    if (CookCollision(info, _dataHeightmap, _terrain->_collisionLod, collisionData))
    {
        return;
    }

    // Save heightfield to asset
    if (_heightfield->Save())
    {
        LOG(Error, "Failed to save heightfield data to asset.");
        return;
    }

    // Clear flag
    _wasHeightModified = false;
#endif
}

void TerrainPatch::SaveSplatData()
{
#if USE_EDITOR
    for (int32 i = 0; i < TERRAIN_MAX_SPLATMAPS_COUNT; i++)
        SaveSplatData(i);
#endif
}

void TerrainPatch::SaveSplatData(int32 index)
{
#if USE_EDITOR
    ASSERT(index >= 0 && index < TERRAIN_MAX_SPLATMAPS_COUNT);

    // Skip if was not modified or cannot be saved
    if (!_wasSplatmapModified[index] ||
        Splatmap[index] == nullptr ||
        Splatmap[index]->IsVirtual() ||
        _dataSplatmap[index] == nullptr)
    {
        return;
    }
    PROFILE_CPU_NAMED("Terrain.Save");

    // Save splatmap to asset
    if (Splatmap[index]->WaitForLoaded())
    {
        LOG(Error, "Failed to load patch splatmap data.");
        return;
    }
    if (Splatmap[index]->Save(String::Empty, _dataSplatmap[index]))
    {
        LOG(Error, "Failed to save splatmap data to asset.");
        return;
    }

    // Clear flag
    _wasSplatmapModified[index] = false;
#endif
}

#endif

bool TerrainPatch::UpdateCollision()
{
    PROFILE_CPU();
    ScopeLock lock(_collisionLocker);

    // Update collision
    if (HasCollision())
    {
        // Invalidate cache
#if TERRAIN_USE_PHYSICS_DEBUG
        _debugLinesDirty = true;
#endif
#if USE_EDITOR
        _collisionTriangles.Resize(0);
#endif
        _collisionVertices.Resize(0);

        // Recreate height field
        PhysicsBackend::DestroyObject(_physicsHeightField);
        _physicsHeightField = nullptr;
        if (CreateHeightField())
        {
            LOG(Error, "Failed to create terrain collision height field.");
            return true;
        }

        // Update physics (will link new height field into shape geometry container)
        UpdateCollisionScale();
    }
    else
    {
        CreateCollision();
    }

    return false;
}

bool TerrainPatch::RayCast(const Vector3& origin, const Vector3& direction, float& resultHitDistance, float maxDistance) const
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    if (_physicsShape == nullptr)
        return false;
    Vector3 shapePos;
    Quaternion shapeRot;
    PhysicsBackend::GetShapePose(_physicsShape, shapePos, shapeRot);
    return PhysicsBackend::RayCastShape(_physicsShape, shapePos, shapeRot, origin, direction, resultHitDistance, maxDistance);
}

bool TerrainPatch::RayCast(const Vector3& origin, const Vector3& direction, float& resultHitDistance, Vector3& resultHitNormal, float maxDistance) const
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    if (_physicsShape == nullptr)
        return false;
    Vector3 shapePos;
    Quaternion shapeRot;
    PhysicsBackend::GetShapePose(_physicsShape, shapePos, shapeRot);
    RayCastHit hit;
    if (PhysicsBackend::RayCastShape(_physicsShape, shapePos, shapeRot, origin, direction, hit, maxDistance))
    {
        resultHitDistance = hit.Distance;
        resultHitNormal = hit.Normal;
        return true;
    }
    return false;
}

bool TerrainPatch::RayCast(const Vector3& origin, const Vector3& direction, float& resultHitDistance, TerrainChunk*& resultChunk, float maxDistance) const
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    if (_physicsShape == nullptr)
        return false;
    Vector3 shapePos;
    Quaternion shapeRot;
    PhysicsBackend::GetShapePose(_physicsShape, shapePos, shapeRot);

    // Perform raycast test
    float hitDistance;
    if (PhysicsBackend::RayCastShape(_physicsShape, shapePos, shapeRot, origin, direction, hitDistance, maxDistance))
    {
        // Find hit chunk
        resultChunk = nullptr;
        const auto hitPoint = origin + direction * hitDistance;
        for (int32 chunkIndex = 0; chunkIndex < Terrain::ChunksCount; chunkIndex++)
        {
            const auto box = Chunks[chunkIndex]._bounds;
            if (box.Minimum.X <= hitPoint.X && box.Maximum.X >= hitPoint.X &&
                box.Minimum.Z <= hitPoint.Z && box.Maximum.Z >= hitPoint.Z)
            {
                resultChunk = (TerrainChunk*)&Chunks[chunkIndex];
                break;
            }
        }

        // This should never happen but in that case just skip hit
        if (resultChunk == nullptr)
            return false;

        resultHitDistance = hitDistance;
        return true;
    }

    return false;
}

bool TerrainPatch::RayCast(const Vector3& origin, const Vector3& direction, RayCastHit& hitInfo, float maxDistance) const
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    if (_physicsShape == nullptr)
        return false;
    Vector3 shapePos;
    Quaternion shapeRot;
    PhysicsBackend::GetShapePose(_physicsShape, shapePos, shapeRot);
    return PhysicsBackend::RayCastShape(_physicsShape, shapePos, shapeRot, origin, direction, hitInfo, maxDistance);
}

void TerrainPatch::ClosestPoint(const Vector3& position, Vector3& result) const
{
    if (_physicsShape == nullptr)
    {
        result = Vector3::Maximum;
        return;
    }
    Vector3 shapePos;
    Quaternion shapeRot;
    PhysicsBackend::GetShapePose(_physicsShape, shapePos, shapeRot);
    Vector3 closestPoint;
    const float distanceSqr = PhysicsBackend::ComputeShapeSqrDistanceToPoint(_physicsShape, shapePos, shapeRot, position, &closestPoint);
    if (distanceSqr > 0.0f)
        result = closestPoint;
    else
        result = position;
}

#if USE_EDITOR

void TerrainPatch::UpdatePostManualDeserialization()
{
    // Update data
    for (int32 chunkIndex = 0; chunkIndex < Terrain::ChunksCount; chunkIndex++)
    {
        auto& chunk = Chunks[chunkIndex];
        chunk.UpdateTransform();
    }
    _terrain->UpdateBounds();

    ScopeLock lock(_collisionLocker);

    // Update collision
    if (HasCollision())
    {
        // Invalidate cache
#if TERRAIN_USE_PHYSICS_DEBUG
        _debugLinesDirty = true;
#endif
#if USE_EDITOR
        _collisionTriangles.Resize(0);
#endif
        _collisionVertices.Resize(0);

        // Recreate height field
        PhysicsBackend::DestroyObject(_physicsHeightField);
        _physicsHeightField = nullptr;
        if (CreateHeightField())
        {
            LOG(Error, "Failed to create terrain collision height field.");
            return;
        }

        // Update physics (will link new height field into shape geometry container)
        UpdateCollisionScale();
    }
    else
    {
        CreateCollision();
    }
}

#endif

void TerrainPatch::CreateCollision()
{
    PROFILE_CPU();
    ASSERT(!HasCollision());
    if (CreateHeightField())
        return;
    ASSERT(_physicsHeightField);

    // Create geometry
    const Transform terrainTransform = _terrain->_transform;
    CollisionShape shape;
    const float rowScale = Math::Abs(terrainTransform.Scale.X) * _collisionScaleXZ;
    const float heightScale = Math::Abs(terrainTransform.Scale.Y) * _yHeight / TERRAIN_PATCH_COLLISION_QUANTIZATION;
    const float columnScale = Math::Abs(terrainTransform.Scale.Z) * _collisionScaleXZ;
    shape.SetHeightField(_physicsHeightField, heightScale, rowScale, columnScale);

    // Create shape
    JsonAsset* materials[8];
    for (int32 i = 0; i < 8; i++)
        materials[i] = _terrain->GetPhysicalMaterials()[i];
    _physicsShape = PhysicsBackend::CreateShape(_terrain, shape, ToSpan(materials, 8), _terrain->IsActiveInHierarchy(), false);
    PhysicsBackend::SetShapeLocalPose(_physicsShape, Vector3(0, _yOffset * terrainTransform.Scale.Y, 0), Quaternion::Identity);

    // Create static actor
    void* scene = _terrain->GetPhysicsScene()->GetPhysicsScene();
    _physicsActor = PhysicsBackend::CreateRigidStaticActor(nullptr, terrainTransform.LocalToWorld(_offset), terrainTransform.Orientation, scene);
    PhysicsBackend::AttachShape(_physicsShape, _physicsActor);
    if (_terrain->IsDuringPlay())
        PhysicsBackend::AddSceneActor(scene, _physicsActor);
}

bool TerrainPatch::CreateHeightField()
{
    PROFILE_CPU();
    ASSERT(_physicsHeightField == nullptr);

    // Skip if height field data is missing but warn on loading failed
    if (_heightfield == nullptr)
        return true;
    if (_heightfield->WaitForLoaded() || _heightfield->Data.IsEmpty())
    {
        LOG(Warning, "Cannot create terrain collision. Failed to load heightfield data for terrain {0} patch {1}x{2}.", _terrain->ToString(), _x, _z);
        return true;
    }

    // Check if the cooked collision matches the engine version
    auto collisionHeader = (TerrainCollisionDataHeader*)_heightfield->Data.Get();
    if (collisionHeader->CheckOldMagicNumber != MAX_int32 || collisionHeader->Version != TerrainCollisionDataHeader::CurrentVersion)
    {
        // Reset height map
        PROFILE_CPU_NAMED("ResetHeightMap");
        const float* data = GetHeightmapData();
        return SetupHeightMap(_cachedHeightMap.Count(), data);
    }

    // Create heightfield object from the data
    _collisionScaleXZ = collisionHeader->ScaleXZ * TERRAIN_UNITS_PER_VERTEX;
    _physicsHeightField = PhysicsBackend::CreateHeightField(_heightfield->Data.Get() + sizeof(TerrainCollisionDataHeader), _heightfield->Data.Count() - sizeof(TerrainCollisionDataHeader));
    if (_physicsHeightField == nullptr)
    {
        LOG(Error, "Failed to create terrain collision height field.");
        return true;
    }

    return false;
}

void TerrainPatch::UpdateCollisionScale() const
{
    PROFILE_CPU();
    ASSERT(HasCollision());

    // Create geometry
    const Transform terrainTransform = _terrain->_transform;
    CollisionShape geometry;
    const float rowScale = Math::Abs(terrainTransform.Scale.X) * _collisionScaleXZ;
    const float heightScale = Math::Abs(terrainTransform.Scale.Y) * _yHeight / TERRAIN_PATCH_COLLISION_QUANTIZATION;
    const float columnScale = Math::Abs(terrainTransform.Scale.Z) * _collisionScaleXZ;
    geometry.SetHeightField(_physicsHeightField, heightScale, rowScale, columnScale);

    // Update shape
    PhysicsBackend::SetShapeGeometry(_physicsShape, geometry);
    PhysicsBackend::SetShapeLocalPose(_physicsShape, Vector3(0, _yOffset * terrainTransform.Scale.Y, 0), Quaternion::Identity);
}

void TerrainPatch::DestroyCollision()
{
    PROFILE_CPU();
    ScopeLock lock(_collisionLocker);
    ASSERT(HasCollision());

    void* scene = _terrain->GetPhysicsScene()->GetPhysicsScene();
    PhysicsBackend::RemoveCollider(_terrain);
    PhysicsBackend::RemoveSceneActor(scene, _physicsActor);
    PhysicsBackend::DestroyActor(_physicsActor);
    PhysicsBackend::DestroyShape(_physicsShape);
    PhysicsBackend::DestroyObject(_physicsHeightField);

    _physicsActor = nullptr;
    _physicsShape = nullptr;
    _physicsHeightField = nullptr;
#if TERRAIN_USE_PHYSICS_DEBUG
    _debugLinesDirty = true;
    SAFE_DELETE(_debugLines);
#endif
#if USE_EDITOR
    _collisionTriangles.Resize(0);
#endif
    _collisionVertices.Resize(0);
}

#if TERRAIN_USE_PHYSICS_DEBUG

void TerrainPatch::CacheDebugLines()
{
    PROFILE_CPU();
    ASSERT(_physicsHeightField);
    _debugLinesDirty = false;
    if (!_debugLines)
        _debugLines = GPUDevice::Instance->CreateBuffer(TEXT("Terrain.DebugLines"));

    int32 rows, cols;
    PhysicsBackend::GetHeightFieldSize(_physicsHeightField, rows, cols);
    const int32 count = (rows - 1) * (cols - 1) * 6 + (cols + rows - 2) * 2;
    typedef DebugDraw::Vertex Vertex;
    if (_debugLines->GetElementsCount() != count)
    {
        if (_debugLines->Init(GPUBufferDescription::Vertex(sizeof(Vertex), count)))
            return;
    }
    Array<Vertex> debugLines;
    debugLines.Resize(count);
    auto* data = debugLines.Get();
    const Color32 color(Color::GreenYellow * 0.8f);

#define GET_VERTEX(x, y) const Vertex v##x##y = { Float3((float)(row + (x)), PhysicsBackend::GetHeightFieldHeight(_physicsHeightField, row + (x), col + (y)) / TERRAIN_PATCH_COLLISION_QUANTIZATION, (float)(col + (y))), color }

    for (int32 row = 0; row < rows - 1; row++)
    {
        for (int32 col = 0; col < cols - 1; col++)
        {
            // Skip holes
            const auto sample = PhysicsBackend::GetHeightFieldSample(_physicsHeightField, row, col);
            if (sample.MaterialIndex0 == (uint8)PhysicsBackend::HeightFieldMaterial::Hole)
            {
                for (int32 i = 0; i < 6; i++)
                    *data++ = Vertex { Float3::Zero, Color32::Black };
                continue;
            }

            GET_VERTEX(0, 0);
            GET_VERTEX(0, 1);
            GET_VERTEX(1, 0);
            GET_VERTEX(1, 1);

            *data++ = v00;
            *data++ = v01;

            *data++ = v00;
            *data++ = v10;

            *data++ = v00;
            *data++ = v11;
        }
    }

    for (int32 row = 0; row < rows - 1; row++)
    {
        const int32 col = cols - 1;
        GET_VERTEX(0, 0);
        GET_VERTEX(1, 0);

        *data++ = v00;
        *data++ = v10;
    }

    for (int32 col = 0; col < cols - 1; col++)
    {
        const int32 row = rows - 1;
        GET_VERTEX(0, 0);
        GET_VERTEX(0, 1);

        *data++ = v00;
        *data++ = v01;
    }

#undef GET_VERTEX

    _debugLines->SetData(debugLines.Get(), _debugLines->GetSize());
}

void TerrainPatch::DrawPhysicsDebug(RenderView& view)
{
#if COMPILE_WITH_DEBUG_DRAW
    const BoundingBox bounds(_bounds.Minimum - view.Origin, _bounds.Maximum - view.Origin);
    if (!_physicsShape || !view.CullingFrustum.Intersects(bounds))
        return;
    if (view.Mode == ViewMode::PhysicsColliders)
    {
        DEBUG_DRAW_TRIANGLES(GetCollisionTriangles(), Color::DarkOliveGreen, 0, true);
    }
    else
    {
        BoundingSphere sphere;
        BoundingSphere::FromBox(bounds, sphere);
        if (Vector3::Distance(sphere.Center, view.Position) - sphere.Radius < METERS_TO_UNITS(500))
        {
            if (!_debugLines || _debugLinesDirty)
                CacheDebugLines();
            const Transform terrainTransform = _terrain->_transform;
            const Transform localTransform(Vector3(0, _yOffset, 0), Quaternion::Identity, Vector3(_collisionScaleXZ, _yHeight, _collisionScaleXZ));
            const Matrix world = localTransform.GetWorld() * terrainTransform.GetWorld();
            DebugDraw::DrawLines(_debugLines, world);
        }
    }
#endif
}

#endif

#if USE_EDITOR

const Array<Vector3>& TerrainPatch::GetCollisionTriangles()
{
    ScopeLock lock(_collisionLocker);
    if (!_physicsShape || _collisionTriangles.HasItems())
        return _collisionTriangles;
    PROFILE_CPU();

    int32 rows, cols;
    PhysicsBackend::GetHeightFieldSize(_physicsHeightField, rows, cols);

    _collisionTriangles.Resize((rows - 1) * (cols - 1) * 6);
    Vector3* data = _collisionTriangles.Get();

#define GET_VERTEX(x, y) Vector3 v##x##y((float)(row + (x)), PhysicsBackend::GetHeightFieldHeight(_physicsHeightField, row + (x), col + (y)) / TERRAIN_PATCH_COLLISION_QUANTIZATION, (float)(col + (y))); Vector3::Transform(v##x##y, world, v##x##y)

    const float size = _terrain->_chunkSize * TERRAIN_UNITS_PER_VERTEX * Terrain::Terrain::ChunksCountEdge;
    const Transform terrainTransform = _terrain->_transform;
    Transform localTransform(Vector3(_x * size, _yOffset, _z * size), Quaternion::Identity, Vector3(_collisionScaleXZ, _yHeight, _collisionScaleXZ));
    const Matrix world = localTransform.GetWorld() * terrainTransform.GetWorld();

    for (int32 row = 0; row < rows - 1; row++)
    {
        for (int32 col = 0; col < cols - 1; col++)
        {
            // Skip holes
            const auto sample = PhysicsBackend::GetHeightFieldSample(_physicsHeightField, row, col);
            if (sample.MaterialIndex0 == (uint8)PhysicsBackend::HeightFieldMaterial::Hole)
            {
                for (int32 i = 0; i < 6; i++)
                    *data++ = Vector3::Zero;
                continue;
            }

            GET_VERTEX(0, 0);
            GET_VERTEX(0, 1);
            GET_VERTEX(1, 0);
            GET_VERTEX(1, 1);

            *data++ = v00;
            *data++ = v11;
            *data++ = v10;

            *data++ = v00;
            *data++ = v01;
            *data++ = v11;
        }
    }

#undef GET_VERTEX

    return _collisionTriangles;
}

void TerrainPatch::GetCollisionTriangles(const BoundingSphere& bounds, Array<Vector3>& result)
{
    PROFILE_CPU();
    result.Clear();

    // Skip if no intersection with patch
    if (!CollisionsHelper::BoxIntersectsSphere(GetBounds(), bounds) || !_physicsHeightField)
        return;

    // Prepare
    const auto& triangles = GetCollisionTriangles();
    const float size = _terrain->_chunkSize * TERRAIN_UNITS_PER_VERTEX * Terrain::Terrain::ChunksCountEdge;
    Transform transform;
    transform.Translation = _offset + Vector3(0, _yOffset, 0);
    transform.Orientation = Quaternion::Identity;
    transform.Scale = Vector3(1.0f, _yHeight, 1.0f);
    transform = _terrain->_transform.LocalToWorld(transform);
    Matrix world;
    transform.GetWorld(world);
    Matrix invWorld;
    Matrix::Invert(world, invWorld);

    // Project bounds to terrain surface XZ plane to find the heightfield range that might intersect with the brush
    BoundingBox box;
    BoundingBox::FromSphere(bounds, box);
    Vector3 min, max;
    Vector3::Transform(box.Minimum, invWorld, min);
    Vector3::Transform(box.Maximum, invWorld, max);
    {
        Vector3 t = min;
        Vector3::Min(t, max, min);
        Vector3::Max(t, max, max);
    }

    // Normalize bounds and map to actual triangles buffer
    int32 rows, cols;
    PhysicsBackend::GetHeightFieldSize(_physicsHeightField, rows, cols);
    int32 startRow = (int32)Math::Floor(min.X / size * rows);
    int32 startCol = (int32)Math::Floor(min.Z / size * cols);
    int32 endRow = (int32)Math::Ceil(max.X / size * rows);
    int32 endCol = (int32)Math::Ceil(max.Z / size * cols);

    // Normalize bounds to patch borders
    startRow = Math::Clamp(startRow, 0, rows - 2);
    startCol = Math::Clamp(startCol, 0, cols - 2);
    endRow = Math::Clamp(endRow, 0, rows - 2);
    endCol = Math::Clamp(endCol, 0, cols - 2);

    // Shortcut: row=x, col=z

    // Check every triangle from the given range
    for (int32 row = startRow; row <= endRow; row++)
    {
        for (int32 col = startCol; col <= endCol; col++)
        {
            int32 index = (row * (cols - 1) + col) * 6;
            Vector3 t0 = triangles[index + 0];
            Vector3 t1 = triangles[index + 1];
            Vector3 t2 = triangles[index + 2];

#if 0
            DebugDraw::DrawLine(t0, t2, Color::Red, 1.0f, false);
            DebugDraw::DrawLine(t1, t2, Color::Red, 1.0f, false);
            DebugDraw::DrawLine(t0, t1, Color::Red, 1.0f, false);
#endif

            // Check if triangles intersects with the bounds
            if (CollisionsHelper::SphereIntersectsTriangle(bounds, t0, t1, t2))
            {
                result.Add(t0);
                result.Add(t1);
                result.Add(t2);
            }

            t0 = triangles[index + 3];
            t1 = triangles[index + 4];
            t2 = triangles[index + 5];

#if 0
            DebugDraw::DrawLine(t0, t2, Color::Red, 1.0f, false);
            DebugDraw::DrawLine(t1, t2, Color::Red, 1.0f, false);
            DebugDraw::DrawLine(t0, t1, Color::Red, 1.0f, false);
#endif

            // Check if triangles intersects with the bounds
            if (CollisionsHelper::SphereIntersectsTriangle(bounds, t0, t1, t2))
            {
                result.Add(t0);
                result.Add(t1);
                result.Add(t2);
            }
        }
    }
}

#endif

void TerrainPatch::ExtractCollisionGeometry(Array<Float3>& vertexBuffer, Array<int32>& indexBuffer)
{
    PROFILE_CPU();
    vertexBuffer.Clear();
    indexBuffer.Clear();

    ScopeLock lock(_collisionLocker);
    if (!_physicsShape)
        return;

    int32 rows, cols;
    PhysicsBackend::GetHeightFieldSize(_physicsHeightField, rows, cols);

    // Cache pre-transformed collision heightfield vertices locations
    if (_collisionVertices.IsEmpty())
    {
        // Prevent race conditions
        ScopeLock sceneLock(Level::ScenesLock);
        if (_collisionVertices.IsEmpty())
        {
            const float size = _terrain->_chunkSize * TERRAIN_UNITS_PER_VERTEX * Terrain::Terrain::ChunksCountEdge;
            const Transform terrainTransform = _terrain->_transform;
            const Transform localTransform(Vector3(_x * size, _yOffset, _z * size), Quaternion::Identity, Float3(_collisionScaleXZ, _yHeight, _collisionScaleXZ));
            const Matrix world = localTransform.GetWorld() * terrainTransform.GetWorld();

            const int32 vertexCount = rows * cols;
            _collisionVertices.Resize(vertexCount);
            Float3* vb = _collisionVertices.Get();
            for (int32 row = 0; row < rows; row++)
            {
                for (int32 col = 0; col < cols; col++)
                {
                    Float3 v((float)row, PhysicsBackend::GetHeightFieldHeight(_physicsHeightField, row, col) / TERRAIN_PATCH_COLLISION_QUANTIZATION, (float)col);
                    Float3::Transform(v, world, v);
                    *vb++ = v;
                }
            }
        }
    }

    // Copy vertex buffer
    vertexBuffer.Add(_collisionVertices);

    // Generate index buffer
    const int32 indexCount = (rows - 1) * (cols - 1) * 6;
    indexBuffer.Resize(indexCount);
    int32* ib = indexBuffer.Get();
    for (int32 row = 0; row < rows - 1; row++)
    {
        for (int32 col = 0; col < cols - 1; col++)
        {
#define GET_INDEX(x, y) *ib++ = (col + (y)) + (row + (x)) * cols

            GET_INDEX(0, 0);
            GET_INDEX(1, 1);
            GET_INDEX(1, 0);

            GET_INDEX(0, 0);
            GET_INDEX(0, 1);
            GET_INDEX(1, 1);

#undef GET_INDEX
        }
    }
}

void TerrainPatch::Serialize(SerializeStream& stream, const void* otherObj)
{
    SERIALIZE_GET_OTHER_OBJ(TerrainPatch);

    SERIALIZE_MEMBER(X, _x);
    SERIALIZE_MEMBER(Z, _z);
    SERIALIZE_MEMBER(Offset, _yOffset);
    SERIALIZE_MEMBER(Height, _yHeight);
    SERIALIZE_MEMBER(Heightmap, Heightmap);
    SERIALIZE_MEMBER(Splatmap0, Splatmap[0]);
    SERIALIZE_MEMBER(Splatmap1, Splatmap[1]);
    static_assert(ARRAY_COUNT(Splatmap) == 2, "Please update the code above to match the maximum terrain splatmaps amount.");
    SERIALIZE_MEMBER(Heightfield, _heightfield);

    stream.JKEY("Chunks");
    stream.StartArray();
    for (int32 i = 0; i < Terrain::Terrain::ChunksCount; i++)
    {
        stream.StartObject();
        Chunks[i].Serialize(stream, other ? &other->Chunks[i] : nullptr);
        stream.EndObject();
    }
    stream.EndArray();

#if TERRAIN_UPDATING
    SaveHeightData();
    SaveSplatData();
#endif
}

void TerrainPatch::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    DESERIALIZE_MEMBER(X, _x);
    DESERIALIZE_MEMBER(Z, _z);
    DESERIALIZE_MEMBER(Offset, _yOffset);
    DESERIALIZE_MEMBER(Height, _yHeight);
    DESERIALIZE_MEMBER(Heightmap, Heightmap);
    DESERIALIZE_MEMBER(Splatmap0, Splatmap[0]);
    DESERIALIZE_MEMBER(Splatmap1, Splatmap[1]);
    static_assert(ARRAY_COUNT(Splatmap) == 2, "Please update the code above to match the maximum terrain splatmaps amount.");
    DESERIALIZE_MEMBER(Heightfield, _heightfield);

    // Update offset (x or/and z may be modified)
    const float size = _terrain->_chunkSize * TERRAIN_UNITS_PER_VERTEX * Terrain::ChunksCountEdge;
    _offset = Vector3(_x * size, 0.0f, _z * size);

    auto member = stream.FindMember("Chunks");
    if (member != stream.MemberEnd() && member->value.IsArray())
    {
        auto& chunksData = member->value;
        const auto chunksCount = Math::Min<int32>((int32)chunksData.Size(), Terrain::ChunksCount);
        for (int32 i = 0; i < chunksCount; i++)
        {
            Chunks[i].Deserialize(chunksData[i], modifier);
        }
    }
}

void TerrainPatch::OnPhysicsSceneChanged(PhysicsScene* previous)
{
    PhysicsBackend::RemoveSceneActor(previous->GetPhysicsScene(), _physicsActor, true);
    void* scene = _terrain->GetPhysicsScene()->GetPhysicsScene();
    PhysicsBackend::AddSceneActor(scene, _physicsActor);
}
