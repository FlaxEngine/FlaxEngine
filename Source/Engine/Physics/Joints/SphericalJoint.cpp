// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "SphericalJoint.h"
#include "Engine/Physics/PhysicsBackend.h"

SphericalJoint::SphericalJoint(const SpawnParams& params)
    : Joint(params)
    , _flags(SphericalJointFlag::Limit)
{
}

void SphericalJoint::SetFlags(const SphericalJointFlag value)
{
    if (_flags == value)
        return;
    _flags = value;
    if (_joint)
        PhysicsBackend::SetSphericalJointFlags(_joint, value);
}

void SphericalJoint::SetLimit(const LimitConeRange& value)
{
    if (_limit == value)
        return;
    _limit = value;
    if (_joint)
        PhysicsBackend::SetSphericalJointLimit(_joint, value);
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"
#include "Engine/Core/Math/Color.h"

void SphericalJoint::OnDebugDrawSelected()
{
    const Vector3 source = GetPosition();
    const Vector3 target = GetTargetPosition();
    const float size = 15.0f;
    const Color color = Color::Green.AlphaMultiplied(0.6f);
    DEBUG_DRAW_WIRE_ARROW(source, GetOrientation(), size / 100.0f * 0.5f, Color::Red, 0, false);
    if (EnumHasAnyFlags(_flags, SphericalJointFlag::Limit))
    {
        DEBUG_DRAW_CONE(source, GetOrientation(), size, _limit.YLimitAngle * DegreesToRadians, _limit.ZLimitAngle * DegreesToRadians, color, 0, false);
    }
    else
    {
        DEBUG_DRAW_SPHERE(BoundingSphere(source, size), color, 0, false);
    }
    DEBUG_DRAW_LINE(source, target, Color::Green * 0.6f, 0, false);

    // Base
    Joint::OnDebugDrawSelected();
}

#endif

void* SphericalJoint::CreateJoint(const PhysicsJointDesc& desc)
{
    void* joint = PhysicsBackend::CreateSphericalJoint(desc);
    PhysicsBackend::SetSphericalJointFlags(joint, _flags);
    PhysicsBackend::SetSphericalJointLimit(joint, _limit);
    return joint;
}
