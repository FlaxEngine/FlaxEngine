// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "SliderJoint.h"
#include "Engine/Physics/PhysicsBackend.h"

SliderJoint::SliderJoint(const SpawnParams& params)
    : Joint(params)
    , _flags(SliderJointFlag::Limit)
{
    _limit.Lower = -50.0f;
    _limit.Upper = 50.0f;
}

void SliderJoint::SetFlags(const SliderJointFlag value)
{
    if (_flags == value)
        return;
    _flags = value;
    if (_joint)
        PhysicsBackend::SetSliderJointFlags(_joint, value);
}

void SliderJoint::SetLimit(const LimitLinearRange& value)
{
    if (_limit == value)
        return;
    _limit = value;
    if (_joint)
        PhysicsBackend::SetSliderJointLimit(_joint, value);
}

float SliderJoint::GetCurrentPosition() const
{
    return _joint ? PhysicsBackend::GetSliderJointPosition(_joint) : 0.0f;
}

float SliderJoint::GetCurrentVelocity() const
{
    return _joint ? PhysicsBackend::GetSliderJointVelocity(_joint) : 0.0f;
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void SliderJoint::OnDebugDrawSelected()
{
    const Vector3 source = GetPosition();
    const Vector3 normal = GetOrientation() * Vector3::Right;
    float min = -100.0f, max = 100.0f;
    if (EnumHasAnyFlags(_flags, SliderJointFlag::Limit))
    {
        min = _limit.Lower;
        max = _limit.Upper;
        if (max < min)
            Swap(min, max);
    }
    DEBUG_DRAW_LINE(source + normal * min, source + normal * max, Color::Green * 0.6f, 0, false);

    // Base
    Joint::OnDebugDrawSelected();
}

#endif

void* SliderJoint::CreateJoint(const PhysicsJointDesc& desc)
{
    void* joint = PhysicsBackend::CreateSliderJoint(desc);
    PhysicsBackend::SetSliderJointFlags(joint, _flags);
    PhysicsBackend::SetSliderJointLimit(joint, _limit);
    return joint;
}
