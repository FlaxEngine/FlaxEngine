// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "SkinnedModelLOD.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Serialization/MemoryReadStream.h"

bool SkinnedModelLOD::HasAnyMeshInitialized() const
{
    // Note: we initialize all meshes at once so the last one can be used to check it.
    return Meshes.HasItems() && Meshes.Last().IsInitialized();
}

bool SkinnedModelLOD::Load(MemoryReadStream& stream)
{
    // Load LOD for each mesh
    for (int32 i = 0; i < Meshes.Count(); i++)
    {
        auto& mesh = Meshes[i];

        // #MODEL_DATA_FORMAT_USAGE
        uint32 vertices;
        stream.ReadUint32(&vertices);
        uint32 triangles;
        stream.ReadUint32(&triangles);
        uint16 blendShapesCount;
        stream.ReadUint16(&blendShapesCount);
        if (blendShapesCount != mesh.BlendShapes.Count())
        {
            LOG(Warning, "Cannot initialize mesh {0}. Incorect blend shapes amount: {1} (expected: {2})", i, blendShapesCount, mesh.BlendShapes.Count());
            return true;
        }
        for (auto& blendShape : mesh.BlendShapes)
        {
            blendShape.UseNormals = stream.ReadBool();
            stream.ReadUint32(&blendShape.MinVertexIndex);
            stream.ReadUint32(&blendShape.MaxVertexIndex);
            uint32 blendShapeVertices;
            stream.ReadUint32(&blendShapeVertices);
            blendShape.Vertices.Resize(blendShapeVertices);
            stream.ReadBytes(blendShape.Vertices.Get(), blendShape.Vertices.Count() * sizeof(BlendShapeVertex));
        }
        const uint32 indicesCount = triangles * 3;
        const bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
        const uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
        if (vertices == 0 || triangles == 0)
            return true;
        const auto vb0 = stream.Read<VB0SkinnedElementType>(vertices);
        const auto ib = stream.Read<byte>(indicesCount * ibStride);

        // Setup GPU resources
        if (mesh.Load(vertices, triangles, vb0, ib, use16BitIndexBuffer))
        {
            LOG(Warning, "Cannot initialize mesh {0}. Vertices: {1}, triangles: {2}", i, vertices, triangles);
            return true;
        }
    }

    return false;
}

void SkinnedModelLOD::Unload()
{
    // Unload LOD for each mesh
    for (int32 i = 0; i < Meshes.Count(); i++)
    {
        Meshes[i].Unload();
    }
}

void SkinnedModelLOD::Dispose()
{
    _model = nullptr;
    ScreenSize = 0.0f;
    Meshes.Resize(0);
}

bool SkinnedModelLOD::Intersects(const Ray& ray, const Matrix& world, float& distance, Vector3& normal, SkinnedMesh** mesh)
{
    // Check all meshes
    bool result = false;
    float closest = MAX_float;
    Vector3 closestNormal = Vector3::Up;
    for (int32 i = 0; i < Meshes.Count(); i++)
    {
        // Test intersection with mesh and check if is closer than previous
        float dst;
        Vector3 nrm;
        if (Meshes[i].Intersects(ray, world, dst, nrm) && dst < closest)
        {
            result = true;
            *mesh = &Meshes[i];
            closest = dst;
            closestNormal = nrm;
        }
    }

    distance = closest;
    normal = closestNormal;
    return result;
}

BoundingBox SkinnedModelLOD::GetBox(const Matrix& world) const
{
    // Find minimum and maximum points of all the meshes
    Vector3 tmp, min = Vector3::Maximum, max = Vector3::Minimum;
    Vector3 corners[8];
    for (int32 j = 0; j < Meshes.Count(); j++)
    {
        const auto& mesh = Meshes[j];
        mesh.GetCorners(corners);

        for (int32 i = 0; i < 8; i++)
        {
            Vector3::Transform(corners[i], world, tmp);
            min = Vector3::Min(min, tmp);
            max = Vector3::Max(max, tmp);
        }
    }

    return BoundingBox(min, max);
}

BoundingBox SkinnedModelLOD::GetBox(const Matrix& world, int32 meshIndex) const
{
    // Find minimum and maximum points of the mesh
    Vector3 tmp, min = Vector3::Maximum, max = Vector3::Minimum;
    Vector3 corners[8];
    const auto& mesh = Meshes[meshIndex];
    mesh.GetCorners(corners);
    for (int32 i = 0; i < 8; i++)
    {
        Vector3::Transform(corners[i], world, tmp);
        min = Vector3::Min(min, tmp);
        max = Vector3::Max(max, tmp);
    }

    return BoundingBox(min, max);
}

BoundingBox SkinnedModelLOD::GetBox() const
{
    // Find minimum and maximum points of the mesh in given world
    Vector3 min = Vector3::Maximum, max = Vector3::Minimum;
    Vector3 corners[8];
    for (int32 j = 0; j < Meshes.Count(); j++)
    {
        Meshes[j].GetCorners(corners);

        for (int32 i = 0; i < 8; i++)
        {
            min = Vector3::Min(min, corners[i]);
            max = Vector3::Max(max, corners[i]);
        }
    }

    return BoundingBox(min, max);
}
