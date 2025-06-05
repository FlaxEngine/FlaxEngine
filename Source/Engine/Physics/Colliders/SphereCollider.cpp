// Copyright (c) Wojciech Figat. All rights reserved.

#include "SphereCollider.h"

SphereCollider::SphereCollider(const SpawnParams& params)
    : Collider(params)
    , _radius(50.0f)
{
}

void SphereCollider::SetRadius(const float value)
{
    if (value == _radius)
        return;

    _radius = value;

    UpdateGeometry();
    UpdateBounds();
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"
#include "Engine/Graphics/RenderView.h"

void SphereCollider::DrawPhysicsDebug(RenderView& view)
{
    const BoundingSphere sphere(_sphere.Center - view.Origin, _sphere.Radius);
    if (!view.CullingFrustum.Intersects(sphere))
        return;
    if (view.Mode == ViewMode::PhysicsColliders && !GetIsTrigger())
        DEBUG_DRAW_SPHERE(_sphere, _staticActor ? Color::CornflowerBlue : Color::Orchid, 0, true);
    else
        DEBUG_DRAW_WIRE_SPHERE(_sphere, Color::GreenYellow * 0.8f, 0, true);
}

void SphereCollider::OnDebugDrawSelected()
{
    DEBUG_DRAW_WIRE_SPHERE(_sphere, Color::GreenYellow, 0, false);

    if (_contactOffset > 0)
    {
        BoundingSphere contactBounds = _sphere;
        contactBounds.Radius += _contactOffset;
        DEBUG_DRAW_WIRE_SPHERE(contactBounds, Color::Blue.AlphaMultiplied(0.2f), 0, false);
    }

    // Base
    Collider::OnDebugDrawSelected();
}

#endif

bool SphereCollider::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
    return _sphere.Intersects(ray, distance, normal);
}

void SphereCollider::UpdateBounds()
{
    // Cache bounds
    _sphere.Center = _transform.LocalToWorld(_center);
    _sphere.Radius = _radius * _transform.Scale.MaxValue();
    _sphere.GetBoundingBox(_box);
}

void SphereCollider::GetGeometry(CollisionShape& collision)
{
    const float radius = Math::Abs(_radius) * _cachedScale;
    const float minSize = 0.001f;
    collision.SetSphere(Math::Max(radius, minSize));
}
