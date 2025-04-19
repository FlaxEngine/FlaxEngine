// Copyright (c) Wojciech Figat. All rights reserved.

#include "DistanceJoint.h"
#include "Engine/Physics/PhysicsBackend.h"
#include "Engine/Serialization/Serialization.h"

DistanceJoint::DistanceJoint(const SpawnParams& params)
    : Joint(params)
    , _flags(DistanceJointFlag::MinDistance | DistanceJointFlag::MaxDistance)
    , _minDistance(0.0f)
    , _maxDistance(10.0f)
    , _tolerance(25.0f)
{
}

void DistanceJoint::SetFlags(DistanceJointFlag value)
{
    if (_flags == value)
        return;
    _flags = value;
    if (_joint)
        PhysicsBackend::SetDistanceJointFlags(_joint, value);
}

void DistanceJoint::SetMinDistance(float value)
{
    value = Math::Clamp(value, 0.0f, _maxDistance);
    if (value == _minDistance)
        return;
    _minDistance = value;
    if (_joint)
        PhysicsBackend::SetDistanceJointMinDistance(_joint, value);
}

void DistanceJoint::SetMaxDistance(float value)
{
    value = Math::Max(_minDistance, value);
    if (value == _maxDistance)
        return;
    _maxDistance = value;
    if (_joint)
        PhysicsBackend::SetDistanceJointMaxDistance(_joint, value);
}

void DistanceJoint::SetTolerance(float value)
{
    value = Math::Max(0.1f, value);
    if (value == _tolerance)
        return;
    _tolerance = value;
    if (_joint)
        PhysicsBackend::SetDistanceJointTolerance(_joint, value);
}

void DistanceJoint::SetSpringParameters(const SpringParameters& value)
{
    if (value == _spring)
        return;
    _spring = value;
    if (_joint)
        PhysicsBackend::SetDistanceJointSpring(_joint, value);
}

float DistanceJoint::GetCurrentDistance() const
{
    return _joint ? PhysicsBackend::GetDistanceJointDistance(_joint) : 0.0f;
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void DistanceJoint::OnDebugDrawSelected()
{
    const Vector3 source = GetPosition();
    const Vector3 target = GetTargetPosition();
    Vector3 dir = target - source;
    const float len = (float)dir.Length();
    dir *= 1.0f / len;
    Vector3 start = source, end = target;
    float max = 0, min = 0;
    if (EnumHasAnyFlags(_flags, DistanceJointFlag::MinDistance))
    {
        min = Math::Min(_minDistance, len);
        start += dir * min;
        DEBUG_DRAW_LINE(source, start, Color::Red * 0.6f, 0, false);
    }
    if (EnumHasAnyFlags(_flags, DistanceJointFlag::MaxDistance))
    {
        max = Math::Min(_maxDistance, len - min);
        end -= dir * max;
        DEBUG_DRAW_LINE(end, target, Color::Red * 0.6f, 0, false);
    }
    DEBUG_DRAW_LINE(start, end, Color::Green * 0.6f, 0, false);

    // Base
    Joint::OnDebugDrawSelected();
}

#endif

void DistanceJoint::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Joint::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(DistanceJoint);

    SERIALIZE_MEMBER(Flags, _flags);
    SERIALIZE_MEMBER(MinDistance, _minDistance);
    SERIALIZE_MEMBER(MaxDistance, _maxDistance);
    SERIALIZE_MEMBER(Tolerance, _tolerance);
    SERIALIZE_MEMBER(Stiffness, _spring.Stiffness);
    SERIALIZE_MEMBER(Damping, _spring.Damping);
}

void DistanceJoint::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Joint::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Flags, _flags);
    DESERIALIZE_MEMBER(MinDistance, _minDistance);
    DESERIALIZE_MEMBER(MaxDistance, _maxDistance);
    DESERIALIZE_MEMBER(Tolerance, _tolerance);
    DESERIALIZE_MEMBER(Stiffness, _spring.Stiffness);
    DESERIALIZE_MEMBER(Damping, _spring.Damping);
}

void* DistanceJoint::CreateJoint(const PhysicsJointDesc& desc)
{
    void* joint = PhysicsBackend::CreateDistanceJoint(desc);
    PhysicsBackend::SetDistanceJointFlags(joint, _flags);
    PhysicsBackend::SetDistanceJointMinDistance(joint, _minDistance);
    PhysicsBackend::SetDistanceJointMaxDistance(joint, _maxDistance);
    PhysicsBackend::SetDistanceJointTolerance(joint, _tolerance);
    PhysicsBackend::SetDistanceJointSpring(joint, _spring);
    return joint;
}
