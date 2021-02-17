// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "SphereCollider.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Physics/Utilities.h"
#include <PxShape.h>

SphereCollider::SphereCollider(const SpawnParams& params)
    : Collider(params)
    , _radius(50.0f)
{
}

void SphereCollider::SetRadius(const float value)
{
    if (Math::NearEqual(value, _radius))
        return;

    _radius = value;

    UpdateGeometry();
    UpdateBounds();
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void SphereCollider::DrawPhysicsDebug(RenderView& view)
{
    DEBUG_DRAW_WIRE_SPHERE(_sphere, Color::GreenYellow * 0.8f, 0, true);
}

void SphereCollider::OnDebugDrawSelected()
{
    DEBUG_DRAW_WIRE_SPHERE(_sphere, Color::GreenYellow, 0, false);

    // Base
    Collider::OnDebugDrawSelected();
}

#endif

bool SphereCollider::IntersectsItself(const Ray& ray, float& distance, Vector3& normal)
{
    return _sphere.Intersects(ray, distance, normal);
}

void SphereCollider::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Collider::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(SphereCollider);

    SERIALIZE_MEMBER(Radius, _radius);
}

void SphereCollider::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Collider::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Radius, _radius);
}

void SphereCollider::UpdateBounds()
{
    // Cache bounds
    _sphere.Center = _transform.LocalToWorld(_center);
    _sphere.Radius = _radius * _transform.Scale.MaxValue();
    _sphere.GetBoundingBox(_box);
}

void SphereCollider::GetGeometry(PxGeometryHolder& geometry)
{
    const float scaling = _cachedScale.GetAbsolute().MaxValue();
    const float radius = Math::Abs(_radius) * scaling;
    const float minSize = 0.001f;
    const PxSphereGeometry sphere(Math::Max(radius, minSize));
    geometry.storeAny(sphere);
}
