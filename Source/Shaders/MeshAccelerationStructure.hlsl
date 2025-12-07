// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __MESH_ACCELERATION_STRUCTURE__
#define __MESH_ACCELERATION_STRUCTURE__

#include "./Flax/Collisions.hlsl"

// This must match MeshAccelerationStructure::ToGPU
#define BVH_STACK_SIZE 32

struct BVHNode
{
    float3 BoundsMin;
    uint Index;
    float3 BoundsMax;
    int Count; // Negative for non-leaf nodes
};

// Pass all data via separate params (SPIR-V doesn't support buffers in structures)
#define BVHBuffers_Param StructuredBuffer<BVHNode> BVHBuffer, ByteAddressBuffer VertexBuffer, ByteAddressBuffer IndexBuffer, uint VertexStride
#define BVHBuffers_Init(BVHBuffer, VertexBuffer, IndexBuffer, VertexStride) BVHBuffer, VertexBuffer, IndexBuffer, VertexStride
#define BVHBuffers_Pass BVHBuffers_Init(BVHBuffer, VertexBuffer, IndexBuffer, VertexStride)

struct BVHHit
{
    float Distance;
    bool IsBackface;
};

float3 LoadVertexBVH(BVHBuffers_Param, uint index)
{
    int addr = index << 2u;
    uint vertexIndex = IndexBuffer.Load(addr);
    return asfloat(VertexBuffer.Load3(vertexIndex * VertexStride));
}

// [https://tavianator.com/2011/ray_box.html]
float RayTestBoxBVH(float3 rayPos, float3 rayDir, float3 boxMin, float3 boxMax)
{
    float3 rayInvDir = rcp(rayDir);
    float3 tMin = (boxMin - rayPos) * rayInvDir;
    float3 tMax = (boxMax - rayPos) * rayInvDir;
    float3 t1 = min(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float3 t2 = max(tMin, tMax);
    float tFar = min(min(t2.x, t2.y), t2.z);
    bool hit = tFar >= tNear && tFar > 0;
    return hit ? max(tNear, 0) : -1;
}

// Performs raytracing against the BVH acceleration structure to find the closest intersection with a triangle.
bool RayCastBVH(BVHBuffers_Param, float3 rayPos, float3 rayDir, out BVHHit hit, float maxDistance = 1000000.0f)
{
    hit = (BVHHit)0;
    hit.Distance = maxDistance;
    
    // Stack-based recursion, starts from root node
    uint stack[BVH_STACK_SIZE];
    uint stackCount = 1;
    stack[0] = 0;

    bool result = false;
    LOOP
    while (stackCount > 0)
    {
        BVHNode node = BVHBuffer[stack[--stackCount]];

        // Raytrace bounds
        float boundsHit = RayTestBoxBVH(rayPos, rayDir, node.BoundsMin, node.BoundsMax);
        BRANCH
        if (boundsHit >= 0 && boundsHit < hit.Distance)
        {
            BRANCH
            if (node.Count > 0) // Is leaf?
            {
                // Ray cast along all triangles in the leaf
                uint indexStart = node.Index;
                uint indexEnd = indexStart + node.Count;
                for (uint i = indexStart; i < indexEnd;)
                {
                    // Load triangle
                    float3 v0 = LoadVertexBVH(BVHBuffers_Pass, i++);
                    float3 v1 = LoadVertexBVH(BVHBuffers_Pass, i++);
                    float3 v2 = LoadVertexBVH(BVHBuffers_Pass, i++);

                    // Raytrace triangle
                    float distance;
                    if (RayIntersectsTriangle(rayPos, rayDir, v0, v1, v2, distance) && distance < hit.Distance)
                    {
                        float3 n = normalize(cross(v1 - v0, v2 - v0));
                        hit.Distance = distance;
                        hit.IsBackface = dot(rayDir, n) > 0;
                        result = true;
                    }
                }
            }
            else
            {
                // Push children onto the stack to be tested
                stack[stackCount++] = node.Index + 0;
                stack[stackCount++] = node.Index + 1;
            }
        }
    }
    return result;
}

// Performs a query against the BVH acceleration structure to find the closest distance to a triangle from a given point.
bool PointQueryBVH(BVHBuffers_Param, float3 pos, out BVHHit hit, float maxDistance = 1000000.0f)
{
    hit = (BVHHit)0;
    hit.Distance = maxDistance;

    // Stack-based recursion, starts from root node
    uint stack[BVH_STACK_SIZE];
    uint stackCount = 1;
    stack[0] = 0;

    bool result = false;
    LOOP
    while (stackCount > 0)
    {
        BVHNode node = BVHBuffer[stack[--stackCount]];

        // Skip too far nodes
        if (PointDistanceBox(node.BoundsMin, node.BoundsMax, pos) >= hit.Distance)
            continue;

        BRANCH
        if (node.Count > 0) // Is leaf?
        {
            // Find the closest triangles in the leaf
            uint indexStart = node.Index;
            uint indexEnd = indexStart + node.Count;
            for (uint i = indexStart; i < indexEnd;)
            {
                // Load triangle
                float3 v0 = LoadVertexBVH(BVHBuffers_Pass, i++);
                float3 v1 = LoadVertexBVH(BVHBuffers_Pass, i++);
                float3 v2 = LoadVertexBVH(BVHBuffers_Pass, i++);

                // Check triangle
                float distance = sqrt(DistancePointToTriangle2(pos, v0, v1, v2));
                if (distance < hit.Distance)
                {
                    hit.Distance = distance;
                    result = true;
                }
            }
        }
        else
        {
            // Push children onto the stack to be tested
            stack[stackCount++] = node.Index + 0;
            stack[stackCount++] = node.Index + 1;
        }
    }
    return result;
}

#endif
