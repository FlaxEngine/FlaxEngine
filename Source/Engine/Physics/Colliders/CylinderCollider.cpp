// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "CylinderCollider.h"

CylinderCollider::CylinderCollider(const SpawnParams& params)
    : Collider(params)
    , _radius(20.0f)
    , _height(100.0f)
{
}

void CylinderCollider::SetRadius(const float value)
{
    if (Math::NearEqual(value, _radius))
        return;

    _radius = value;

    UpdateGeometry();
    UpdateBounds();
}

void CylinderCollider::SetHeight(const float value)
{
    if (Math::NearEqual(value, _height))
        return;

    _height = value;

    UpdateGeometry();
    UpdateBounds();
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Physics/Colliders/ColliderColorConfig.h"
#include "Engine/Physics/PhysicsBackend.h"

void CylinderCollider::DrawPhysicsDebug(RenderView& view)
{
    const BoundingSphere sphere(_sphere.Center - view.Origin, _sphere.Radius);
    if (!view.CullingFrustum.Intersects(sphere))
        return;

    Transform T{};
    T.Scale = _transform.Scale;
    PhysicsBackend::GetShapePose(_shape, T.Translation, T.Orientation);

    Quaternion rotation;
    Quaternion::Multiply(T.Orientation, Quaternion::Euler(0, 90, 0), rotation);
    const float scaling = _cachedScale.GetAbsolute().MaxValue();
    const float minSize = 0.001f;
    const float radius = Math::Max(Math::Abs(_radius) * scaling, minSize);
    const float height = Math::Max(Math::Abs(_height) * scaling, minSize);
    if (view.Mode == ViewMode::PhysicsColliders && !GetIsTrigger())
        DEBUG_DRAW_CYLINDER(T.LocalToWorld(_center), rotation, radius, height, _staticActor ? Color::CornflowerBlue : Color::Orchid, 0, true);
    else
       DEBUG_DRAW_WIRE_CYLINDER(T.LocalToWorld(_center), rotation, radius, height, Color::GreenYellow * 0.8f, 0, true);
}

void CylinderCollider::OnDebugDraw()
{
    if (DisplayCollider)
    {
        Quaternion rotation;
        Quaternion::Multiply(_transform.Orientation, Quaternion::Euler(0, 90, 0), rotation);
        const float scaling = _cachedScale.GetAbsolute().MaxValue();
        const float minSize = 0.001f;
        const float radius = Math::Max(Math::Abs(_radius) * scaling, minSize);
        const float height = Math::Max(Math::Abs(_height) * scaling, minSize);
        const Vector3 position = _transform.LocalToWorld(_center);
        if (GetIsTrigger())
        {
            DEBUG_DRAW_WIRE_CYLINDER(position, rotation, radius, height, FlaxEngine::ColliderColors::TriggerColliderOutline, 0, false);
            DEBUG_DRAW_CYLINDER(position, rotation, radius, height, FlaxEngine::ColliderColors::TriggerCollider, 0, true);
        }
        else
        {
            DEBUG_DRAW_WIRE_CYLINDER(position, rotation, radius, height, FlaxEngine::ColliderColors::NormalColliderOutline, 0, false);
            DEBUG_DRAW_CYLINDER(position, rotation, radius, height, FlaxEngine::ColliderColors::NormalCollider, 0, true);
        }
    }

    // Base
    Collider::OnDebugDraw();
}

void CylinderCollider::OnDebugDrawSelected()
{
    Quaternion rotation;
    Quaternion::Multiply(_transform.Orientation, Quaternion::Euler(0, 90, 0), rotation);
    const float scaling = _cachedScale.GetAbsolute().MaxValue();
    const float minSize = 0.001f;
    const float radius = Math::Max(Math::Abs(_radius) * scaling, minSize);
    const float height = Math::Max(Math::Abs(_height) * scaling, minSize);
    const Vector3 position = _transform.LocalToWorld(_center);

    if (!DisplayCollider) 
    {
        if (GetIsTrigger())
        {
            DEBUG_DRAW_WIRE_CYLINDER(position, rotation, radius, height, FlaxEngine::ColliderColors::TriggerColliderOutline, 0, false);
            DEBUG_DRAW_CYLINDER(position, rotation, radius, height, FlaxEngine::ColliderColors::TriggerCollider, 0, true);
        }
        else
        {
            DEBUG_DRAW_WIRE_CYLINDER(position, rotation, radius, height, FlaxEngine::ColliderColors::NormalColliderOutline, 0, false);
            DEBUG_DRAW_CYLINDER(position, rotation, radius, height, FlaxEngine::ColliderColors::NormalCollider, 0, true);
        }
    }
    if (_contactOffset > 0)
    {
        DEBUG_DRAW_WIRE_CYLINDER(position, rotation, radius + _contactOffset, height, Color::Blue.AlphaMultiplied(0.2f), 0, false);
    }

    // Base
    Collider::OnDebugDrawSelected();
}

#endif

bool CylinderCollider::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
    return _orientedBox.Intersects(ray, distance, normal);
}

void CylinderCollider::UpdateBounds()
{
    // Cache bounds
    const float radiusTwice = _radius * 2.0f;
    OrientedBoundingBox::CreateCentered(_center, Vector3(_height + radiusTwice, radiusTwice, radiusTwice), _orientedBox);
    _orientedBox.Transform(_transform);
    _orientedBox.GetBoundingBox(_box);
    BoundingSphere::FromBox(_box, _sphere);
}

void CylinderCollider::GetGeometry(CollisionShape& collision)
{
    collision.SetCylinder(_radius, _height);
}
