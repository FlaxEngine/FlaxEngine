// Copyright (c) Wojciech Figat. All rights reserved.

#include "MeshCollider.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Math/Ray.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/PhysicsScene.h"
#if USE_EDITOR || !BUILD_RELEASE
#include "Engine/Debug/DebugLog.h"
#endif

MeshCollider::MeshCollider(const SpawnParams& params)
    : Collider(params)
{
    CollisionData.Changed.Bind<MeshCollider, &MeshCollider::OnCollisionDataChanged>(this);
    CollisionData.Loaded.Bind<MeshCollider, &MeshCollider::OnCollisionDataLoaded>(this);
}

void MeshCollider::OnCollisionDataChanged()
{
    // This should not be called during physics simulation, if it happened use write lock on physx scene
    ASSERT(!GetScene() || !GetPhysicsScene()->IsDuringSimulation());

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
#if USE_EDITOR || !BUILD_RELEASE
    if (type == CollisionDataType::TriangleMesh)
    {
        LOG(Warning, "Cannot attach '{0}' using Triangle Mesh collider '{1}' to Rigid Body (not supported)", GetNamePath(), CollisionData->ToString());
    }
#endif
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
#include "Engine/Graphics/RenderView.h"

void MeshCollider::DrawPhysicsDebug(RenderView& view)
{
    if (CollisionData && CollisionData->IsLoaded())
    {
        const BoundingSphere sphere(_sphere.Center - view.Origin, _sphere.Radius);
        if (!view.CullingFrustum.Intersects(sphere))
            return;
        if (view.Mode == ViewMode::PhysicsColliders && !GetIsTrigger())
        {
            Array<Float3>* vertexBuffer;
            Array<int32>* indexBuffer;
            CollisionData->GetDebugTriangles(vertexBuffer, indexBuffer);
            DEBUG_DRAW_TRIANGLES_EX2(*vertexBuffer, *indexBuffer, _transform.GetWorld(), _staticActor ? Color::CornflowerBlue : Color::Orchid, 0, true);
        }
        else
        {
            DEBUG_DRAW_LINES(CollisionData->GetDebugLines(), _transform.GetWorld(), Color::GreenYellow * 0.8f, 0, true);
        }
    }
}

void MeshCollider::OnDebugDrawSelected()
{
    if (CollisionData && CollisionData->IsLoaded())
    {
        const auto& debugLines = CollisionData->GetDebugLines();
        DEBUG_DRAW_LINES(Span<Float3>(debugLines.Get(), debugLines.Count()), _transform.GetWorld(), Color::GreenYellow, 0, false);
    }

    // Base
    Collider::OnDebugDrawSelected();
}

#endif

bool MeshCollider::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
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

void MeshCollider::GetGeometry(CollisionShape& collision)
{
    // Prepare scale
    Float3 scale = _transform.Scale;
    const float minSize = 0.001f;
    Float3 scaleAbs = scale.GetAbsolute();
    if (scaleAbs.X < minSize)
        scale.X = Math::Sign(scale.X) * minSize;
    if (scaleAbs.Y < minSize)
        scale.Y = Math::Sign(scale.Y) * minSize;
    if (scaleAbs.Z < minSize)
        scale.Z = Math::Sign(scale.Z) * minSize;

    // Setup shape (based on type)
    CollisionDataType type = CollisionDataType::None;
    if (CollisionData && CollisionData->IsLoaded())
        type = CollisionData->GetOptions().Type;
    if (type == CollisionDataType::ConvexMesh)
        collision.SetConvexMesh(CollisionData->GetConvex(), scale.Raw);
    else if (type == CollisionDataType::TriangleMesh)
        collision.SetTriangleMesh(CollisionData->GetTriangle(), scale.Raw);
    else
        collision.SetSphere(minSize);
}
