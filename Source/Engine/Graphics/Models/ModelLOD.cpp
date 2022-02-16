// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "ModelLOD.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Serialization/MemoryReadStream.h"

bool ModelLOD::Load(MemoryReadStream& stream)
{
    // Load LOD for each mesh
    _verticesCount = 0;
    for (int32 i = 0; i < Meshes.Count(); i++)
    {
        // #MODEL_DATA_FORMAT_USAGE
        uint32 vertices;
        stream.ReadUint32(&vertices);
        _verticesCount += vertices;
        uint32 triangles;
        stream.ReadUint32(&triangles);
        uint32 indicesCount = triangles * 3;
        bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
        uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
        if (vertices == 0 || triangles == 0)
            return true;
        auto vb0 = stream.Read<VB0ElementType>(vertices);
        auto vb1 = stream.Read<VB1ElementType>(vertices);
        bool hasColors = stream.ReadBool();
        VB2ElementType18* vb2 = nullptr;
        if (hasColors)
        {
            vb2 = stream.Read<VB2ElementType18>(vertices);
        }
        auto ib = stream.Read<byte>(indicesCount * ibStride);

        // Setup GPU resources
        if (Meshes[i].Load(vertices, triangles, vb0, vb1, vb2, ib, use16BitIndexBuffer))
        {
            LOG(Warning, "Cannot initialize mesh {0}. Vertices: {1}, triangles: {2}", i, vertices, triangles);
            return true;
        }
    }

    return false;
}

void ModelLOD::Unload()
{
    // Unload LOD for each mesh
    for (int32 i = 0; i < Meshes.Count(); i++)
    {
        Meshes[i].Unload();
    }
}

void ModelLOD::Dispose()
{
    _model = nullptr;
    ScreenSize = 0.0f;
    Meshes.Resize(0);
}

bool ModelLOD::Intersects(const Ray& ray, const Matrix& world, float& distance, Vector3& normal, Mesh** mesh)
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

BoundingBox ModelLOD::GetBox(const Matrix& world) const
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

BoundingBox ModelLOD::GetBox(const Matrix& world, int32 meshIndex) const
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

BoundingBox ModelLOD::GetBox() const
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
