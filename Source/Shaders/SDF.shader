// Copyright (c) Wojciech Figat. All rights reserved.

// Mesh SDF generation based on https://github.com/GPUOpen-Effects/TressFX

#include "./Flax/Common.hlsl"
#include "./Flax/ThirdParty/TressFX/TressFXSDF.hlsl"

#define THREAD_GROUP_SIZE 64

META_CB_BEGIN(0, Data)
int3 Resolution;
uint ResolutionSize;
float MaxDistance;
uint VertexStride;
bool Index16bit;
uint TriangleCount;
float3 VoxelToPosMul;
float WorldUnitsPerVoxel;
float3 VoxelToPosAdd;
uint ThreadGroupsX;
META_CB_END

RWBuffer<uint> SDF : register(u0);

uint GetVoxelIndex(uint3 groupId, uint groupIndex)
{
    return groupIndex + (groupId.x + groupId.y * ThreadGroupsX) * THREAD_GROUP_SIZE;
}

int3 ClampVoxelCoord(int3 coord)
{
    return clamp(coord, 0, Resolution - 1);
}

int GetVoxelIndex(int3 coord)
{
    return Resolution.x * Resolution.y * coord.z + Resolution.x * coord.y + coord.x;
}

float3 GetVoxelPos(int3 coord)
{
    return float3((float)coord.x, (float)coord.y, (float)coord.z) * VoxelToPosMul + VoxelToPosAdd;
}

int3 GetVoxelCoord(float3 pos)
{
    pos = (pos - VoxelToPosAdd) / VoxelToPosMul;
    return int3((int)pos.x, (int)pos.y, (int)pos.z);
}

int3 GetVoxelCoord(uint index)
{
    uint sizeX = (uint)Resolution.x;
    uint sizeY = (uint)(Resolution.x * Resolution.y);
    uint coordZ = index / sizeY;
    uint coordXY = index % sizeY;
    uint coordY = coordXY / sizeX;
    uint coordX = coordXY % sizeX;
    return int3((int)coordX, (int)coordY, (int)coordZ);
}

// Clears SDF texture with the initial distance.
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void CS_Init(uint3 GroupId : SV_GroupID, uint GroupIndex : SV_GroupIndex)
{
    uint voxelIndex = GetVoxelIndex(GroupId, GroupIndex);
    if (voxelIndex >= ResolutionSize)
        return;
    float distance = MaxDistance * 10.0f; // Start with a very large value
    SDF[voxelIndex] = FloatFlip3(distance);
}

// Unpacks SDF texture into distances stores as normal float value (FloatFlip3 is used for interlocked operations on uint).
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void CS_Resolve(uint3 GroupId : SV_GroupID, uint GroupIndex : SV_GroupIndex)
{
    uint voxelIndex = GetVoxelIndex(GroupId, GroupIndex);
    if (voxelIndex >= ResolutionSize)
        return;
    SDF[voxelIndex] = IFloatFlip3(SDF[voxelIndex]);
}

#ifdef _CS_RasterizeTriangle

ByteAddressBuffer VertexBuffer : register(t0);
ByteAddressBuffer IndexBuffer : register(t1);

uint LoadIndex(uint i)
{
    if (Index16bit)
    {
        uint index = IndexBuffer.Load((i >> 1u) << 2u);
        index = (i & 1u) == 1u ? (index >> 16) : index;
        return index & 0xffff;
    }
    return IndexBuffer.Load(i << 2u);
}

float3 LoadVertex(uint i)
{
    return asfloat(VertexBuffer.Load3(i * VertexStride));
}

// Renders triangle mesh into the SDF texture by writing minimum distance to the triangle into all intersecting voxels.
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void CS_RasterizeTriangle(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    uint triangleIndex = DispatchThreadId.x;
    if (triangleIndex >= TriangleCount)
        return;

    // Load triangle
    triangleIndex *= 3;
    uint i0 = LoadIndex(triangleIndex + 0);
    uint i1 = LoadIndex(triangleIndex + 1);
    uint i2 = LoadIndex(triangleIndex + 2);
    float3 v0 = LoadVertex(i0);
    float3 v1 = LoadVertex(i1);
    float3 v2 = LoadVertex(i2);

    // Project triangle into SDF voxels
    float3 vMargin = float3(WorldUnitsPerVoxel, WorldUnitsPerVoxel, WorldUnitsPerVoxel);
    float3 vMin = min(min(v0, v1), v2) - vMargin;
    float3 vMax = max(max(v0, v1), v2) + vMargin;
    int3 voxelMargin = int3(1, 1, 1);
    int3 voxelMin = GetVoxelCoord(vMin) - voxelMargin;
    int3 voxelMax = GetVoxelCoord(vMax) + voxelMargin;
    voxelMin = ClampVoxelCoord(voxelMin);
    voxelMax = ClampVoxelCoord(voxelMax);

    // Rasterize into SDF voxels
    for (int z = voxelMin.z; z <= voxelMax.z; z++)
    {
        for (int y = voxelMin.y; y <= voxelMax.y; y++)
        {
            for (int x = voxelMin.x; x <= voxelMax.x; x++)
            {
                int3 voxelCoord = int3(x, y, z);
                int voxelIndex = GetVoxelIndex(voxelCoord);
                float3 voxelPos = GetVoxelPos(voxelCoord);
                float distance = SignedDistancePointToTriangle(voxelPos, v0, v1, v2);
                if (distance < -10.0f) // TODO: find a better way to reject negative distance from degenerate triangles that break SDF shape
                    distance = abs(distance);
                InterlockedMin(SDF[voxelIndex], FloatFlip3(distance));
            }
        }
    }
}

#endif

#if defined(_CS_FloodFill) || defined(_CS_Encode)

Buffer<uint> InSDF : register(t0);

float GetVoxel(int voxelIndex)
{
    return asfloat(InSDF[voxelIndex]);
}

float GetVoxel(int3 coord)
{
    coord = ClampVoxelCoord(coord);
    int voxelIndex = GetVoxelIndex(coord);
    return GetVoxel(voxelIndex);
}

float CombineSDF(float sdf, int3 nearbyCoord, float nearbyDistance)
{
    // Sample nearby voxel
    float sdfNearby = GetVoxel(nearbyCoord);

    // Include distance to that nearby voxel
    if (sdfNearby < 0.0f)
        nearbyDistance *= -1;
    sdfNearby += nearbyDistance;

    if (sdfNearby > MaxDistance)
    {
        // Ignore if nearby sample is invalid (see CS_Init)
    }
    else if (sdf > MaxDistance)
    {
        // Use nearby sample if current one is invalid (see CS_Init)
        sdf = sdfNearby;
    }
    else
    {
        // Use distance closer to 0
        sdf = sdf >= 0 ? min(sdf, sdfNearby) : max(sdf, sdfNearby);
    }

    return sdf;
}

// Fills the voxels with minimum distances to the triangles.
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void CS_FloodFill(uint3 GroupId : SV_GroupID, uint GroupIndex : SV_GroupIndex)
{
    uint voxelIndex = GetVoxelIndex(GroupId, GroupIndex);
    if (voxelIndex >= ResolutionSize)
        return;
    float sdf = GetVoxel(voxelIndex);

    // Skip if the distance is already so small that we know that triangle is nearby
    if (abs(sdf) > WorldUnitsPerVoxel * 1.2f)
    {
        int3 voxelCoord = GetVoxelCoord(voxelIndex);
        int3 offset = int3(-1, 0, 1);

        // Sample nearby voxels
        float nearbyDistance = WorldUnitsPerVoxel;
        sdf = CombineSDF(sdf, voxelCoord + offset.zyy, nearbyDistance);
        sdf = CombineSDF(sdf, voxelCoord + offset.yzy, nearbyDistance);
        sdf = CombineSDF(sdf, voxelCoord + offset.yyz, nearbyDistance);
        sdf = CombineSDF(sdf, voxelCoord + offset.xyy, nearbyDistance);
        sdf = CombineSDF(sdf, voxelCoord + offset.yxy, nearbyDistance);
        sdf = CombineSDF(sdf, voxelCoord + offset.yyx, nearbyDistance);
    }

    SDF[voxelIndex] = asuint(sdf);
}

RWTexture3D<half> SDFtex : register(u1);

// Encodes SDF values into the packed format with normalized distances.
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void CS_Encode(uint3 GroupId : SV_GroupID, uint GroupIndex : SV_GroupIndex)
{
    uint voxelIndex = GetVoxelIndex(GroupId, GroupIndex);
    if (voxelIndex >= ResolutionSize)
        return;
    float sdf = GetVoxel(voxelIndex);
    sdf = min(sdf, MaxDistance);

    // Pack from range [-MaxDistance; +MaxDistance] to [0; 1]
    sdf = (sdf / MaxDistance) * 0.5f + 0.5f;

    int3 voxelCoord = GetVoxelCoord(voxelIndex);
    SDFtex[voxelCoord] = sdf;
}

#endif
