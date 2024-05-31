// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "ConeCollider.h"

ConeCollider::ConeCollider(const SpawnParams& params)
    : Collider(params)
    , _radius(20.0f)
    , _height(100.0f)
    , _axis(Y)
{
}

void ConeCollider::SetAxis(ColliderAxis value)
{
    if (value == _axis)
        return;

    _axis = value;

    UpdateGeometry();
    UpdateBounds();
}

void ConeCollider::SetRadius(const float value)
{
    if (Math::NearEqual(value, _radius))
        return;

    //[todo] maybe log a warning ? in dev builds and not to any cheeks or clamping in release
    auto SafeValue = Math::Clamp(value, 0.0001f, 100000.0f);// if this gets out of range the physx will throw or crash :D

    _radius = SafeValue;

    UpdateGeometry();
    UpdateBounds();
}

void ConeCollider::SetHeight(const float value)
{
    if (Math::NearEqual(value, _height))
        return;

    //[todo] maybe log a warning ? in dev builds and not to any cheeks or clamping in release
    auto SafeValue = Math::Clamp(value, 0.0001f, 100000.0f);// if this gets out of range the physx will throw or crash :D

    _height = SafeValue;

    UpdateGeometry();
    UpdateBounds();
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Physics/Colliders/ColliderColorConfig.h"
#include "Engine/Physics/PhysicsBackend.h"
namespace 
{
    void DrawWireCone(const Vector3& position, const Quaternion& orientation, float radius, float height, const Color& color, float duration, bool depthTest)
    {
        auto up = orientation * Vector3::Up;
        auto n = (up * (height * 0.5f));
        auto top = position + n;
        auto bottom = position - n;
        auto right = orientation * Vector3(radius, 0, 0);
        auto foroward = orientation * Vector3(0, 0, radius);
        DebugDraw::DrawCircle(bottom, up, radius, color, duration, depthTest);
        DebugDraw::DrawLine(top, bottom + right, color, duration, depthTest);
        DebugDraw::DrawLine(top, bottom - right, color, duration, depthTest);
        DebugDraw::DrawLine(top, bottom + foroward, color, duration, depthTest);
        DebugDraw::DrawLine(top, bottom - foroward, color, duration, depthTest);
    }
}

void ConeCollider::DrawPhysicsDebug(RenderView& view)
{
    const BoundingSphere sphere(_sphere.Center - view.Origin, _sphere.Radius);
    if (!view.CullingFrustum.Intersects(sphere))
        return;

    Transform T{};
    T.Scale = _transform.Scale;
    PhysicsBackend::GetShapePose(_shape, T.Translation, T.Orientation);

    Quaternion rotation;
    //[todo] use of the GetQuaternionOffset(),_radius,_height. is incorent in this place
    //pull the axis value from back end some how

    //[todo] show text the same joins show it, when actor transform and backend are off sync with back end
    //in this place or in the base class

    Quaternion::Multiply(T.Orientation, GetQuaternionOffset(), rotation);
    const float scaling = _cachedScale.GetAbsolute().MaxValue();
    const float minSize = 0.001f;
    const float radius = Math::Max(Math::Abs(_radius) * scaling, minSize);
    const float height = Math::Max(Math::Abs(_height) * scaling, minSize);

    //todo

    //if (view.Mode == ViewMode::PhysicsColliders && !GetIsTrigger())
    //    DEBUG_DRAW_CYLINDER(T.LocalToWorld(_center), rotation, radius, height, _staticActor ? Color::CornflowerBlue : Color::Orchid, 0, true);
    //else
    //   DEBUG_DRAW_WIRE_CYLINDER(T.LocalToWorld(_center), rotation, radius, height, Color::GreenYellow * 0.8f, 0, true);
}

void ConeCollider::OnDebugDraw()
{
    if (DisplayCollider)
    {
        Quaternion rotation;
        Quaternion::Multiply(_transform.Orientation, GetQuaternionOffset(), rotation);
        const float scaling = _cachedScale.GetAbsolute().MaxValue();
        const float minSize = 0.001f;
        const float radius = Math::Max(Math::Abs(_radius) * scaling, minSize);
        const float height = Math::Max(Math::Abs(_height) * scaling, minSize);
        const Vector3 position = _transform.LocalToWorld(_center);
        const float angle = Math::Atan2(radius, height);

        if (GetIsTrigger())
        {
            DrawWireCone(position, rotation, radius, height, FlaxEngine::ColliderColors::TriggerColliderOutline, 0, false);
            //todo
            //DEBUG_DRAW_CONE(position, rotation, radius, angle, angle, FlaxEngine::ColliderColors::TriggerCollider, 0, true);
        }
        else
        {
            DrawWireCone(position, rotation, radius, height, FlaxEngine::ColliderColors::NormalColliderOutline, 0, false);
            //todo
            //DEBUG_DRAW_CONE(position, rotation, radius, angle, angle, FlaxEngine::ColliderColors::NormalCollider, 0, true);
        }
    }

    // Base
    Collider::OnDebugDraw();
}

void ConeCollider::OnDebugDrawSelected()
{
    Quaternion rotation;
    Quaternion::Multiply(_transform.Orientation, GetQuaternionOffset(), rotation);
    const float scaling = _cachedScale.GetAbsolute().MaxValue();
    const float minSize = 0.001f;
    const float radius = Math::Max(Math::Abs(_radius) * scaling, minSize);
    const float height = Math::Max(Math::Abs(_height) * scaling, minSize);
    const Vector3 position = _transform.LocalToWorld(_center);

    if (!DisplayCollider) 
    {
        Quaternion rotation;
        Quaternion::Multiply(_transform.Orientation, GetQuaternionOffset(), rotation);
        const float scaling = _cachedScale.GetAbsolute().MaxValue();
        const float minSize = 0.001f;
        const float radius = Math::Max(Math::Abs(_radius) * scaling, minSize);
        const float height = Math::Max(Math::Abs(_height) * scaling, minSize);
        const Vector3 position = _transform.LocalToWorld(_center);
        const float angle = Math::Atan2(radius, height);


        if (GetIsTrigger())
        {
            DrawWireCone(position, rotation, radius, height, FlaxEngine::ColliderColors::TriggerColliderOutline, 0, false);
            //todo
            //DEBUG_DRAW_CONE(position, rotation, radius, angle, angle, FlaxEngine::ColliderColors::TriggerCollider, 0, true);
        }
        else
        {
            DrawWireCone(position, rotation, radius, height, FlaxEngine::ColliderColors::NormalColliderOutline, 0, false);
            //todo
            //DEBUG_DRAW_CONE(position, rotation, radius, angle, angle, FlaxEngine::ColliderColors::NormalCollider, 0, true);
        }
    }
    if (_contactOffset > 0)
    {
        //todo
        //DEBUG_DRAW_WIRE_CYLINDER(position, rotation, radius + _contactOffset, height, Color::Blue.AlphaMultiplied(0.2f), 0, false);
    }

    // Base
    Collider::OnDebugDrawSelected();
}

#endif

bool ConeCollider::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
    return _orientedBox.Intersects(ray, distance, normal);
}

Quaternion ConeCollider::GetQuaternionOffset()
{
    switch (GetAxis())
    {
    case ColliderAxis::X:
        return Quaternion::Euler(0, 0, 90);
    case ColliderAxis::Y:
        return Quaternion::Identity;
    case ColliderAxis::Z:
        return Quaternion::Euler(90, 0, 0);
    default:
        break;
    }
    //just in case the axis are invalid
    //[Todo] error here ? (dont use assert in this place,allowing for softlock's of editor or a game will be a bad)
    return Quaternion::Identity;
}

void ConeCollider::Internal_SetContactOffset(float value)
{
    _contactOffset = value;
    UpdateGeometry();
    UpdateBounds();
}

void ConeCollider::UpdateBounds()
{
    // Cache bounds
    const float radiusTwice = _radius * 2.0f;
    OrientedBoundingBox::CreateCentered(_center, Vector3(_height + radiusTwice, radiusTwice, radiusTwice), _orientedBox);
    _orientedBox.Transform(_transform);
    _orientedBox.GetBoundingBox(_box);
    BoundingSphere::FromBox(_box, _sphere);
}

void ConeCollider::GetGeometry(CollisionShape& collision)
{
    collision.SetCone(_radius, _height,(int)_axis,_contactOffset);
}
