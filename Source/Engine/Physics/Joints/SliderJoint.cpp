// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "SliderJoint.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Physics/Utilities.h"
#include "Engine/Physics/Physics.h"
#include <ThirdParty/PhysX/extensions/PxPrismaticJoint.h>

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
    {
        auto joint = static_cast<PxPrismaticJoint*>(_joint);
        joint->setPrismaticJointFlag(PxPrismaticJointFlag::eLIMIT_ENABLED, (_flags & SliderJointFlag::Limit) != 0);
    }
}

void SliderJoint::SetLimit(const LimitLinearRange& value)
{
    if (_limit == value)
        return;

    _limit = value;

    if (_joint)
    {
        auto joint = static_cast<PxPrismaticJoint*>(_joint);
        PxJointLinearLimitPair limit(*Physics::GetTolerancesScale(), value.Lower, value.Upper, value.ContactDist);
        limit.stiffness = value.Spring.Stiffness;
        limit.damping = value.Spring.Damping;
        limit.restitution = value.Restitution;
        ASSERT_LOW_LAYER(limit.isValid());
        joint->setLimit(limit);
    }
}

float SliderJoint::GetCurrentPosition() const
{
    return _joint ? static_cast<PxPrismaticJoint*>(_joint)->getPosition() : 0.0f;
}

float SliderJoint::GetCurrentVelocity() const
{
    return _joint ? static_cast<PxPrismaticJoint*>(_joint)->getVelocity() : 0.0f;
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void SliderJoint::OnDebugDrawSelected()
{
    const Vector3 source = GetPosition();
    const Vector3 normal = GetOrientation() * Vector3::Right;
    float min = -100.0f, max = 100.0f;
    if (_flags & SliderJointFlag::Limit)
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

void SliderJoint::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Joint::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(SliderJoint);

    SERIALIZE_MEMBER(Flags, _flags);
    SERIALIZE_MEMBER(ContactDist, _limit.ContactDist);
    SERIALIZE_MEMBER(Restitution, _limit.Restitution);
    SERIALIZE_MEMBER(Stiffness, _limit.Spring.Stiffness);
    SERIALIZE_MEMBER(Damping, _limit.Spring.Damping);
    SERIALIZE_MEMBER(LowerLimit, _limit.Lower);
    SERIALIZE_MEMBER(UpperLimit, _limit.Upper);
}

void SliderJoint::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Joint::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Flags, _flags);
    DESERIALIZE_MEMBER(ContactDist, _limit.ContactDist);
    DESERIALIZE_MEMBER(Restitution, _limit.Restitution);
    DESERIALIZE_MEMBER(Stiffness, _limit.Spring.Stiffness);
    DESERIALIZE_MEMBER(Damping, _limit.Spring.Damping);
    DESERIALIZE_MEMBER(LowerLimit, _limit.Lower);
    DESERIALIZE_MEMBER(UpperLimit, _limit.Upper);
}

PxJoint* SliderJoint::CreateJoint(JointData& data)
{
    const PxTransform trans0(C2P(data.Pos0), C2P(data.Rot0));
    const PxTransform trans1(C2P(data.Pos1), C2P(data.Rot1));
    auto joint = PxPrismaticJointCreate(*data.Physics, data.Actor0, trans0, data.Actor1, trans1);

    int32 flags = 0;
    if (_flags & SliderJointFlag::Limit)
        flags |= PxPrismaticJointFlag::eLIMIT_ENABLED;
    joint->setPrismaticJointFlags(static_cast<PxPrismaticJointFlag::Enum>(flags));
    PxJointLinearLimitPair limit(*Physics::GetTolerancesScale(), _limit.Lower, _limit.Upper, _limit.ContactDist);
    limit.stiffness = _limit.Spring.Stiffness;
    limit.damping = _limit.Spring.Damping;
    limit.restitution = _limit.Restitution;
    joint->setLimit(limit);

    return joint;
}
