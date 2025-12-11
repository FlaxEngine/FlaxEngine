// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/MeshAccelerationStructure.hlsl"

META_CB_BEGIN(0, Data)
int3 Resolution;
uint ResolutionSize;
float MaxDistance;
uint VertexStride;
float BackfacesThreshold;
uint TriangleCount;
float3 VoxelToPosMul;
float WorldUnitsPerVoxel;
float3 VoxelToPosAdd;
uint ThreadGroupsX;
META_CB_END

uint GetVoxelIndex(uint3 groupId, uint groupIndex, uint groupSize)
{
    return groupIndex + (groupId.x + groupId.y * ThreadGroupsX) * groupSize;
}

float3 GetVoxelPos(int3 coord)
{
    return float3((float)coord.x, (float)coord.y, (float)coord.z) * VoxelToPosMul + VoxelToPosAdd;
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

#ifdef _CS_Init

#define THREAD_GROUP_SIZE 64

RWTexture3D<unorm half> SDFtex : register(u0);

// Clears SDF texture with the maximum distance.
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void CS_Init(uint3 GroupId : SV_GroupID, uint GroupIndex : SV_GroupIndex)
{
    uint voxelIndex = GetVoxelIndex(GroupId, GroupIndex, THREAD_GROUP_SIZE);
    if (voxelIndex >= ResolutionSize)
        return;
    int3 voxelCoord = GetVoxelCoord(voxelIndex);
    SDFtex[voxelCoord] = 1.0f;
}

#endif

#ifdef _CS_RasterizeTriangles

#define THREAD_GROUP_SIZE 64

RWTexture3D<unorm half> SDFtex : register(u0);
ByteAddressBuffer VertexBuffer : register(t0);
ByteAddressBuffer IndexBuffer : register(t1);
StructuredBuffer<BVHNode> BVHBuffer : register(t2);

// Renders triangle mesh into the SDF texture by writing minimum distance to the triangle into all intersecting voxels.
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void CS_RasterizeTriangles(uint3 GroupId : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID, uint GroupIndex : SV_GroupIndex)
{
    uint voxelIndex = GetVoxelIndex(GroupId, GroupIndex, THREAD_GROUP_SIZE);
    if (voxelIndex >= ResolutionSize)
        return;
    int3 voxelCoord = GetVoxelCoord(voxelIndex);
    float3 voxelPos = GetVoxelPos(voxelCoord);

    // Point query to find the distance to the closest surface
    BVHHit hit;
    PointQueryBVH(BVHBuffers_Init(BVHBuffer, VertexBuffer, IndexBuffer, VertexStride), voxelPos, hit, MaxDistance);
    float sdf = hit.Distance;

    // Raycast triangles around voxel to count triangle backfaces hit
    #define CLOSEST_CACHE_SIZE 6
    float3 closestDirections[CLOSEST_CACHE_SIZE] =
    {
        float3(+1, 0, 0),
        float3(-1, 0, 0),
        float3(0, +1, 0),
        float3(0, -1, 0),
        float3(0, 0, +1),
        float3(0, 0, -1),
    };
    uint hitBackCount = 0;
    uint minBackfaceHitCount = (uint)(CLOSEST_CACHE_SIZE * BackfacesThreshold);
    for (uint i = 0; i < CLOSEST_CACHE_SIZE; i++)
    {
        float3 rayDir = closestDirections[i];
        if (RayCastBVH(BVHBuffers_Init(BVHBuffer, VertexBuffer, IndexBuffer, VertexStride), voxelPos, rayDir, hit, MaxDistance))
        {
            sdf = min(sdf, hit.Distance);
            if (hit.IsBackface)
                hitBackCount++;
        }
    }
    if (hitBackCount >= minBackfaceHitCount)
    {
        // Voxel is inside the geometry so turn it into negative distance to the surface
        sdf *= -1;
    }

    // Pack from range [-MaxDistance; +MaxDistance] to [0; 1]
    sdf = clamp(sdf, -MaxDistance, MaxDistance);
    sdf = (sdf / MaxDistance) * 0.5f + 0.5f;

    SDFtex[voxelCoord] = sdf;
}

#endif
