// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "DistanceJoint.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Physics/Utilities.h"
#include <ThirdParty/PhysX/extensions/PxDistanceJoint.h>

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
    {
        static_cast<PxDistanceJoint*>(_joint)->setDistanceJointFlags(static_cast<PxDistanceJointFlag::Enum>(value));
    }
}

void DistanceJoint::SetMinDistance(float value)
{
    value = Math::Clamp(value, 0.0f, _maxDistance);
    if (Math::NearEqual(value, _minDistance))
        return;

    _minDistance = value;

    if (_joint)
    {
        static_cast<PxDistanceJoint*>(_joint)->setMinDistance(value);
    }
}

void DistanceJoint::SetMaxDistance(float value)
{
    value = Math::Max(_minDistance, value);
    if (Math::NearEqual(value, _maxDistance))
        return;

    _maxDistance = value;

    if (_joint)
    {
        static_cast<PxDistanceJoint*>(_joint)->setMaxDistance(value);
    }
}

void DistanceJoint::SetTolerance(float value)
{
    value = Math::Max(0.1f, value);
    if (Math::NearEqual(value, _tolerance))
        return;

    _tolerance = value;

    if (_joint)
    {
        static_cast<PxDistanceJoint*>(_joint)->setTolerance(value);
    }
}

void DistanceJoint::SetSpringParameters(const SpringParameters& value)
{
    if (value == _spring)
        return;

    _spring = value;

    if (_joint)
    {
        static_cast<PxDistanceJoint*>(_joint)->setStiffness(value.Stiffness);
        static_cast<PxDistanceJoint*>(_joint)->setDamping(value.Damping);
    }
}

float DistanceJoint::GetCurrentDistance() const
{
    return _joint ? static_cast<PxDistanceJoint*>(_joint)->getDistance() : 0.0f;
}

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

PxJoint* DistanceJoint::CreateJoint(JointData& data)
{
    const PxTransform trans0(C2P(data.Pos0), C2P(data.Rot0));
    const PxTransform trans1(C2P(data.Pos1), C2P(data.Rot1));
    auto joint = PxDistanceJointCreate(*data.Physics, data.Actor0, trans0, data.Actor1, trans1);

    joint->setMinDistance(_minDistance);
    joint->setMaxDistance(_maxDistance);
    joint->setTolerance(_tolerance);
    joint->setDistanceJointFlags(static_cast<PxDistanceJointFlag::Enum>(_flags));
    joint->setStiffness(_spring.Stiffness);
    joint->setDamping(_spring.Damping);

    return joint;
}
