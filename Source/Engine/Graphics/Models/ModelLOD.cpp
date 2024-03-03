// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "ModelLOD.h"
#include "MeshDeformation.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Transform.h"
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
        auto vb0 = stream.Move<VB0ElementType>(vertices);
        auto vb1 = stream.Move<VB1ElementType>(vertices);
        bool hasColors = stream.ReadBool();
        VB2ElementType18* vb2 = nullptr;
        if (hasColors)
        {
            vb2 = stream.Move<VB2ElementType18>(vertices);
        }
        auto ib = stream.Move<byte>(indicesCount * ibStride);

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

bool ModelLOD::Intersects(const Ray& ray, const Matrix& world, Real& distance, Vector3& normal, Mesh** mesh)
{
    bool result = false;
    Real closest = MAX_Real;
    Vector3 closestNormal = Vector3::Up;
    for (int32 i = 0; i < Meshes.Count(); i++)
    {
        Real dst;
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

bool ModelLOD::Intersects(const Ray& ray, const Transform& transform, Real& distance, Vector3& normal, Mesh** mesh)
{
    bool result = false;
    Real closest = MAX_Real;
    Vector3 closestNormal = Vector3::Up;
    for (int32 i = 0; i < Meshes.Count(); i++)
    {
        Real dst;
        Vector3 nrm;
        if (Meshes[i].Intersects(ray, transform, dst, nrm) && dst < closest)
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
    Vector3 tmp, min = Vector3::Maximum, max = Vector3::Minimum;
    Vector3 corners[8];
    for (int32 meshIndex = 0; meshIndex < Meshes.Count(); meshIndex++)
    {
        const auto& mesh = Meshes[meshIndex];
        mesh.GetBox().GetCorners(corners);
        for (int32 i = 0; i < 8; i++)
        {
            Vector3::Transform(corners[i], world, tmp);
            min = Vector3::Min(min, tmp);
            max = Vector3::Max(max, tmp);
        }
    }
    return BoundingBox(min, max);
}

BoundingBox ModelLOD::GetBox(const Transform& transform, const MeshDeformation* deformation) const
{
    Vector3 tmp, min = Vector3::Maximum, max = Vector3::Minimum;
    Vector3 corners[8];
    for (int32 meshIndex = 0; meshIndex < Meshes.Count(); meshIndex++)
    {
        const auto& mesh = Meshes[meshIndex];
        BoundingBox box = mesh.GetBox();
        if (deformation)
            deformation->GetBounds(_lodIndex, meshIndex, box);
        box.GetCorners(corners);
        for (int32 i = 0; i < 8; i++)
        {
            transform.LocalToWorld(corners[i], tmp);
            min = Vector3::Min(min, tmp);
            max = Vector3::Max(max, tmp);
        }
    }
    return BoundingBox(min, max);
}

BoundingBox ModelLOD::GetBox() const
{
    Vector3 min = Vector3::Maximum, max = Vector3::Minimum;
    Vector3 corners[8];
    for (int32 meshIndex = 0; meshIndex < Meshes.Count(); meshIndex++)
    {
        Meshes[meshIndex].GetBox().GetCorners(corners);
        for (int32 i = 0; i < 8; i++)
        {
            min = Vector3::Min(min, corners[i]);
            max = Vector3::Max(max, corners[i]);
        }
    }
    return BoundingBox(min, max);
}
