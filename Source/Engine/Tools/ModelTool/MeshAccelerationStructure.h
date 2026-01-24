// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_MODEL_TOOL

#include "Engine/Core/Math/Triangle.h"
#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Core/Collections/Array.h"

class Model;
class ModelData;
class GPUBuffer;

/// <summary>
/// Acceleration Structure utility for robust ray tracing mesh geometry with optimized data structure.
/// </summary>
class FLAXENGINE_API MeshAccelerationStructure
{
private:
    struct Mesh
    {
        Model* Asset;
        BytesContainer IndexBuffer, VertexBuffer;
        int32 Indices, Vertices;
        bool Use16BitIndexBuffer;
        BoundingBox Bounds;
    };

    struct BVH
    {
        BoundingBox Bounds;

        union
        {
            struct
            {
                uint32 IsLeaf : 1;
                uint16 TriangleCount : 15;
                uint16 MeshIndex : 16;
                uint32 TriangleIndex;
            } Leaf;

            struct
            {
                uint32 IsLeaf : 1;
                uint32 ChildrenCount : 31;
                int32 ChildIndex;
            } Node;
        };
    };

    struct BVHBuild
    {
        int32 MaxLeafSize, MaxDepth;
        int32 NodeDepth = 0;
        int32 MaxNodeDepth = 0;
        int32 MaxNodeTriangles = 0;
        Array<byte> Scratch;
    };

    Array<Mesh, InlinedAllocation<16>> _meshes;
    Array<BVH> _bvh;

    void BuildBVH(int32 node, BVHBuild& build);
    bool PointQueryBVH(int32 node, const Vector3& point, Real& hitDistance, Vector3& hitPoint, Triangle& hitTriangle) const;
    bool RayCastBVH(int32 node, const Ray& ray, Real& hitDistance, Vector3& hitNormal, Triangle& hitTriangle) const;

public:
    ~MeshAccelerationStructure();

    // Adds the model geometry for the build to the structure.
    void Add(Model* model, int32 lodIndex);

    // Adds the model geometry for the build to the structure.
    void Add(const ModelData* modelData, int32 lodIndex, bool copy = false);

    // Adds the triangles geometry for the build to the structure.
    void Add(Float3* vb, int32 vertices, void* ib, int32 indices, bool use16BitIndex, bool copy = false);

    // Merges all added meshes into a single mesh (to reduce number of BVH nodes). Required for GPU BVH build.
    void MergeMeshes(bool force16BitIndexBuffer = false);

    // Builds Bounding Volume Hierarchy (BVH) structure for accelerated geometry queries.
    void BuildBVH(int32 maxLeafSize = 16, int32 maxDepth = 0);

    // Queries the closest triangle.
    bool PointQuery(const Vector3& point, Real& hitDistance, Vector3& hitPoint, Triangle& hitTriangle, Real maxDistance = MAX_Real) const;

    // Ray traces the triangles.
    bool RayCast(const Ray& ray, Real& hitDistance, Vector3& hitNormal, Triangle& hitTriangle, Real maxDistance = MAX_Real) const;

public:
    struct GPU
    {
        GPUBuffer* BVHBuffer;
        GPUBuffer* VertexBuffer;
        GPUBuffer* IndexBuffer;
        bool Valid;

        GPU()
            : BVHBuffer(nullptr)
            , VertexBuffer(nullptr)
            , IndexBuffer(nullptr)
        {
        }

        GPU(GPU&& other) noexcept
            : BVHBuffer(other.BVHBuffer)
            , VertexBuffer(other.VertexBuffer)
            , IndexBuffer(other.IndexBuffer)
        {
            other.BVHBuffer = nullptr;
            other.VertexBuffer = nullptr;
            other.IndexBuffer = nullptr;
        }

        GPU& operator=(GPU other)
        {
            Swap(*this, other);
            return *this;
        }

        ~GPU();

        operator bool() const;
    };

    // Converts the acceleration structure data to GPU format for raytracing inside a shader.
    GPU ToGPU();
};

#endif
