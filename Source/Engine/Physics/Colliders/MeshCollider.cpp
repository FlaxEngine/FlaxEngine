// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "MeshCollider.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Physics/Utilities.h"
#include "Engine/Physics/Physics.h"

MeshCollider::MeshCollider(const SpawnParams& params)
    : Collider(params)
{
    CollisionData.Changed.Bind<MeshCollider, &MeshCollider::OnCollisionDataChanged>(this);
    CollisionData.Loaded.Bind<MeshCollider, &MeshCollider::OnCollisionDataLoaded>(this);
}

void MeshCollider::OnCollisionDataChanged()
{
    // This should not be called during physics simulation, if it happened use write lock on physx scene
    ASSERT(!Physics::IsDuringSimulation());

    if (CollisionData)
    {
        // Ensure that collision asset is loaded (otherwise objects might fall though collider that is not yet loaded on play begin)
        CollisionData->WaitForLoaded();
    }

    UpdateGeometry();
    UpdateBounds();
}

void MeshCollider::OnCollisionDataLoaded()
{
    UpdateGeometry();
    UpdateBounds();
}

bool MeshCollider::CanAttach(RigidBody* rigidBody) const
{
    CollisionDataType type = CollisionDataType::None;
    if (CollisionData && CollisionData->IsLoaded())
        type = CollisionData->GetOptions().Type;
    return type != CollisionDataType::TriangleMesh;
}

bool MeshCollider::CanBeTrigger() const
{
    CollisionDataType type = CollisionDataType::None;
    if (CollisionData && CollisionData->IsLoaded())
        type = CollisionData->GetOptions().Type;
    return type != CollisionDataType::TriangleMesh;
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void MeshCollider::DrawPhysicsDebug(RenderView& view)
{
    if (CollisionData && CollisionData->IsLoaded())
    {
        const auto& debugLines = CollisionData->GetDebugLines();
        DEBUG_DRAW_LINES(Span<Vector3>(debugLines.Get(), debugLines.Count()), _transform.GetWorld(), Color::GreenYellow * 0.8f, 0, true);
    }
}

void MeshCollider::OnDebugDrawSelected()
{
    if (CollisionData && CollisionData->IsLoaded())
    {
        const auto& debugLines = CollisionData->GetDebugLines();
        DEBUG_DRAW_LINES(Span<Vector3>(debugLines.Get(), debugLines.Count()), _transform.GetWorld(), Color::GreenYellow, 0, false);
    }

    // Base
    Collider::OnDebugDrawSelected();
}

#endif

bool MeshCollider::IntersectsItself(const Ray& ray, float& distance, Vector3& normal)
{
    // Use detailed hit
    if (_shape)
    {
        RayCastHit hitInfo;
        if (!RayCast(ray.Position, ray.Direction, hitInfo))
            return false;
        distance = hitInfo.Distance;
        normal = hitInfo.Normal;
        return true;
    }

    // Fallback to AABB
    return _box.Intersects(ray, distance, normal);
}

void MeshCollider::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Collider::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(MeshCollider);

    SERIALIZE(CollisionData);
}

void MeshCollider::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Collider::Deserialize(stream, modifier);

    DESERIALIZE(CollisionData);
}

void MeshCollider::UpdateBounds()
{
    // Cache bounds
    BoundingBox box;
    if (CollisionData && CollisionData->IsLoaded())
        box = CollisionData->GetOptions().Box;
    else
        box = BoundingBox::Zero;
    BoundingBox::Transform(box, _transform.GetWorld(), _box);
    BoundingSphere::FromBox(_box, _sphere);
}

void MeshCollider::GetGeometry(PxGeometryHolder& geometry)
{
    // Prepare scale
    Vector3 scale = _cachedScale;
    scale.Absolute();
    const float minSize = 0.001f;
    scale = Vector3::Max(scale, minSize);

    // Setup shape (based on type)
    CollisionDataType type = CollisionDataType::None;
    if (CollisionData && CollisionData->IsLoaded())
        type = CollisionData->GetOptions().Type;
    if (type == CollisionDataType::ConvexMesh)
    {
        // Convex mesh
        PxConvexMeshGeometry convexMesh;
        convexMesh.scale.scale = C2P(scale);
        convexMesh.convexMesh = CollisionData->GetConvex();
        geometry.storeAny(convexMesh);
    }
    else if (type == CollisionDataType::TriangleMesh)
    {
        // Triangle mesh
        PxTriangleMeshGeometry triangleMesh;
        triangleMesh.scale.scale = C2P(scale);
        triangleMesh.triangleMesh = CollisionData->GetTriangle();
        geometry.storeAny(triangleMesh);
    }
    else
    {
        // Dummy geometry
        const PxSphereGeometry sphere(minSize);
        geometry.storeAny(sphere);
    }
}
