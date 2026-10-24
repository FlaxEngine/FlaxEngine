// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MODEL_TOOL

#include "Engine/Tools/ModelTool/MeshAccelerationStructure.h"
#include "Engine/Core/Math/Triangle.h"
#include "Engine/Core/Collections/Array.h"
#include <ThirdParty/catch2/catch.hpp>

namespace
{
    // Finds the triangle closest to the point (merges the meshes as for the GPU BVH, then builds the BVH).
    Triangle FindTriangle(MeshAccelerationStructure& structure, bool force16BitIndexBuffer, const Vector3& point, Real& distance)
    {
        structure.MergeMeshes(force16BitIndexBuffer);
        structure.BuildBVH();
        Vector3 hitPoint;
        Triangle triangle;
        if (!structure.PointQuery(point, distance, hitPoint, triangle))
            distance = MAX_Real;
        return triangle;
    }
}

TEST_CASE("MeshAccelerationStructure")
{
    SECTION("Test MergeMeshes with 32-bit indices")
    {
        // A mesh of 70,002 vertices with one triangle that uses the vertex 69,999 (above the 16-bit range)
        Array<Float3> vertices;
        vertices.Resize(70002);
        for (Float3& v : vertices)
            v = Float3::Zero;
        vertices[0] = Float3(100, 0, 0);
        vertices[1] = Float3(101, 0, 0);
        vertices[69999] = Float3(100, 1, 0);
        uint32 indices[3] = { 0, 1, 69999 };
        Float3 otherVertices[3] = { Float3(-10, 0, 0), Float3(-11, 0, 0), Float3(-10, 1, 0) };
        uint16 otherIndices[3] = { 0, 1, 2 };

        MeshAccelerationStructure structure;
        structure.Add(vertices.Get(), vertices.Count(), indices, 3, false, true);
        structure.Add(otherVertices, 3, otherIndices, 3, true, true);
        Real distance;
        Triangle triangle = FindTriangle(structure, true, Vector3(100.5f, 0.2f, 0), distance);
        CHECK(distance < 0.001f);
        CHECK(triangle.V2 == Vector3(100, 1, 0));
    }
    SECTION("Test MergeMeshes with 16-bit indices")
    {
        // Two meshes with 16-bit indices: the merged mesh has 32-bit indices, and it must say so
        Float3 verticesA[3] = { Float3(100, 0, 0), Float3(101, 0, 0), Float3(100, 1, 0) };
        Float3 verticesB[3] = { Float3(-10, 0, 0), Float3(-11, 0, 0), Float3(-10, 1, 0) };
        uint16 indices[3] = { 0, 1, 2 };

        MeshAccelerationStructure structure;
        structure.Add(verticesA, 3, indices, 3, true, true);
        structure.Add(verticesB, 3, indices, 3, true, true);
        Real distance;
        Triangle triangle = FindTriangle(structure, false, Vector3(-10.5f, 0.2f, 0), distance);
        CHECK(distance < 0.001f);
        CHECK(triangle.V2 == Vector3(-10, 1, 0));
    }
}

#endif
