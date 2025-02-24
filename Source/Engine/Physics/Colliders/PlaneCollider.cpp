// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "PlaneCollider.h"

PlaneCollider::PlaneCollider(const SpawnParams& params) : Collider(params){}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Physics/Colliders/ColliderColorConfig.h"
#include "Engine/Physics/PhysicsBackend.h"

void PlaneCollider::DrawPhysicsDebug(RenderView& view)
{
    const BoundingSphere sphere(_sphere.Center - view.Origin, _sphere.Radius);
    if (!view.CullingFrustum.Intersects(sphere))
        return;

    Transform T{};
    T.Scale = _transform.Scale;
    PhysicsBackend::GetShapePose(_shape, T.Translation, T.Orientation);

    auto Box = OrientedBoundingBox(Vector3(-MaxBoundingBox, -MaxBoundingBox, -MaxBoundingBox), Vector3(0, MaxBoundingBox, MaxBoundingBox));
    Box.Transform(T);

    if (view.Mode == ViewMode::PhysicsColliders && !GetIsTrigger())
        DEBUG_DRAW_BOX(Box, _staticActor ? Color::CornflowerBlue : Color::Orchid, 0, true);
    else
        DEBUG_DRAW_BOX(Box, Color::GreenYellow * 0.8f, 0, true);
}

void PlaneCollider::OnDebugDraw()
{
    if (DisplayCollider)
    {
        if (GetIsTrigger())
        {
            DEBUG_DRAW_BOX(_orientedBox, FlaxEngine::ColliderColors::TriggerCollider, 0, true);
        }
        else
        {
            DEBUG_DRAW_BOX(_orientedBox, FlaxEngine::ColliderColors::NormalCollider, 0, true);
        }
    }

    // Base
    Collider::OnDebugDraw();
}

void PlaneCollider::OnDebugDrawSelected()
{
    if (!DisplayCollider) 
    {
        if (GetIsTrigger())
        {
            DEBUG_DRAW_BOX(_orientedBox, FlaxEngine::ColliderColors::TriggerCollider, 0, true);
        }
        else
        {
            DEBUG_DRAW_BOX(_orientedBox, FlaxEngine::ColliderColors::NormalCollider, 0, true);
        }
    }
    // Base
    Collider::OnDebugDrawSelected();
}

#endif

bool PlaneCollider::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
    return _orientedBox.Intersects(ray, distance, normal);
}

void PlaneCollider::Internal_SetContactOffset(float value)
{
    _contactOffset = value;
    UpdateGeometry();
    UpdateBounds();
}

void PlaneCollider::UpdateBounds()
{
    // Cache bounds
    //px plane is extending on Z and Y
    _orientedBox = OrientedBoundingBox(Vector3(-MaxBoundingBox, -MaxBoundingBox, -MaxBoundingBox), Vector3(0, MaxBoundingBox, MaxBoundingBox));
    _orientedBox.Transform(_transform);
    _orientedBox.GetBoundingBox(_box);
    BoundingSphere::FromBox(_box, _sphere);
}

void PlaneCollider::GetGeometry(CollisionShape& collision)
{
    collision.SetPlane();
}
