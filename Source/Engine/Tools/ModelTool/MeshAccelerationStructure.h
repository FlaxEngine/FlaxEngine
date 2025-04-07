// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_MODEL_TOOL

#include "Engine/Core/Math/Triangle.h"
#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Core/Collections/Array.h"

class Model;
class ModelData;

/// <summary>
/// Acceleration Structure utility for robust ray tracing mesh geometry with optimized data structure.
/// </summary>
class FLAXENGINE_API MeshAccelerationStructure
{
private:
    struct Mesh
    {
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

    Array<Mesh, InlinedAllocation<16>> _meshes;
    Array<BVH> _bvh;

    void BuildBVH(int32 node, int32 maxLeafSize, Array<byte>& scratch);
    bool PointQueryBVH(int32 node, const Vector3& point, Real& hitDistance, Vector3& hitPoint, Triangle& hitTriangle) const;
    bool RayCastBVH(int32 node, const Ray& ray, Real& hitDistance, Vector3& hitNormal, Triangle& hitTriangle) const;

public:
    // Adds the model geometry for the build to the structure.
    void Add(Model* model, int32 lodIndex);

    // Adds the model geometry for the build to the structure.
    void Add(const ModelData* modelData, int32 lodIndex, bool copy = false);

    // Adds the triangles geometry for the build to the structure.
    void Add(Float3* vb, int32 vertices, void* ib, int32 indices, bool use16BitIndex, bool copy = false);

    // Builds Bounding Volume Hierarchy (BVH) structure for accelerated geometry queries.
    void BuildBVH(int32 maxLeafSize = 16);

    // Queries the closest triangle.
    bool PointQuery(const Vector3& point, Real& hitDistance, Vector3& hitPoint, Triangle& hitTriangle, Real maxDistance = MAX_Real) const;

    // Ray traces the triangles.
    bool RayCast(const Ray& ray, Real& hitDistance, Vector3& hitNormal, Triangle& hitTriangle, Real maxDistance = MAX_Real) const;
};

#endif
