// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "CapsuleCollider.h"
#include "Engine/Serialization/Serialization.h"

CapsuleCollider::CapsuleCollider(const SpawnParams& params)
    : Collider(params)
    , _radius(20.0f)
    , _height(100.0f)
    , _direction(ColliderOrientationDirection::YAxis)
{
    SetDirection(_direction);
}

void CapsuleCollider::SetRadius(const float value)
{
    if (Math::NearEqual(value, _radius))
        return;

    _radius = value;

    UpdateGeometry();
    UpdateBounds();
}

void CapsuleCollider::SetHeight(const float value)
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

void CapsuleCollider::DrawPhysicsDebug(RenderView& view)
{
    const BoundingSphere sphere(_sphere.Center - view.Origin, _sphere.Radius);
    if (!view.CullingFrustum.Intersects(sphere))
        return;
    Quaternion collRot;
    Quaternion::Multiply( _colliderOrientation, Quaternion::Euler(0, 90, 0), collRot);
    Quaternion rot;
    Quaternion::Multiply(_transform.Orientation, collRot, rot);
    const float scaling = _cachedScale.GetAbsolute().MaxValue();
    const float minSize = 0.001f;
    const float radius = Math::Max(Math::Abs(_radius) * scaling, minSize);
    const float height = Math::Max(Math::Abs(_height) * scaling, minSize);
    if (view.Mode == ViewMode::PhysicsColliders && !GetIsTrigger())
        DEBUG_DRAW_TUBE(_transform.LocalToWorld(_center), rot, radius, height, _staticActor ? Color::CornflowerBlue : Color::Orchid, 0, true);
    else
        DEBUG_DRAW_WIRE_TUBE(_transform.LocalToWorld(_center), rot, radius, height, Color::GreenYellow * 0.8f, 0, true);
}

void CapsuleCollider::SetDirection(ColliderOrientationDirection value)
{
    _direction = value;
    switch (value)
    {
    case ColliderOrientationDirection::XAxis:
        SetColliderOrientation(Quaternion::Identity);
        break;
    case ColliderOrientationDirection::YAxis:
        SetColliderOrientation(Quaternion::Euler(0, 0, 90));
        break;
    case ColliderOrientationDirection::ZAxis:
        SetColliderOrientation(Quaternion::Euler(0, 90, 0));
        break;
    default: ;
    }
}

void CapsuleCollider::OnDebugDrawSelected()
{
    Quaternion collRot;
    Quaternion::Multiply( _colliderOrientation, Quaternion::Euler(0, 90, 0), collRot);
    Quaternion rot;
    Quaternion::Multiply(_transform.Orientation, collRot, rot);
    const float scaling = _cachedScale.GetAbsolute().MaxValue();
    const float minSize = 0.001f;
    const float radius = Math::Max(Math::Abs(_radius) * scaling, minSize);
    const float height = Math::Max(Math::Abs(_height) * scaling, minSize);
    DEBUG_DRAW_WIRE_TUBE(_transform.LocalToWorld(_center), rot, radius, height, Color::GreenYellow, 0, false);

    // Base
    Collider::OnDebugDrawSelected();
}

#endif

bool CapsuleCollider::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
    return _orientedBox.Intersects(ray, distance, normal);
}

void CapsuleCollider::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Collider::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(CapsuleCollider);

    SERIALIZE_MEMBER(Radius, _radius);
    SERIALIZE_MEMBER(Height, _height);
    SERIALIZE(_direction);
}

void CapsuleCollider::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Collider::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Radius, _radius);
    DESERIALIZE_MEMBER(Height, _height);
    DESERIALIZE(_direction);
    SetDirection(_direction);
}

void CapsuleCollider::UpdateBounds()
{
    // Cache bounds
    const float radiusTwice = _radius * 2.0f;
    OrientedBoundingBox::CreateCentered(_center, Vector3(_height + radiusTwice, radiusTwice, radiusTwice), _orientedBox);
    Transform transform = _transform;
    Quaternion rot;
    Quaternion::Multiply(transform.Orientation, _colliderOrientation, rot);
    transform.Orientation = rot;
    _orientedBox.Transform(transform);
    _orientedBox.GetBoundingBox(_box);
    BoundingSphere::FromBox(_box, _sphere);
}

void CapsuleCollider::GetGeometry(CollisionShape& collision)
{
    const float scaling = _cachedScale.GetAbsolute().MaxValue();
    const float minSize = 0.001f;
    const float radius = Math::Max(Math::Abs(_radius) * scaling, minSize);
    const float height = Math::Max(Math::Abs(_height) * scaling, minSize);
    collision.SetCapsule(radius, height * 0.5f);
}
