// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "SphericalJoint.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Physics/Utilities.h"
#include <ThirdParty/PhysX/extensions/PxSphericalJoint.h>

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
    {
        auto joint = static_cast<PxSphericalJoint*>(_joint);
        joint->setSphericalJointFlag(PxSphericalJointFlag::eLIMIT_ENABLED, (_flags & SphericalJointFlag::Limit) != 0);
    }
}

void SphericalJoint::SetLimit(const LimitConeRange& value)
{
    if (_limit == value)
        return;

    _limit = value;

    if (_joint)
    {
        auto joint = static_cast<PxSphericalJoint*>(_joint);
        PxJointLimitCone limit(value.YLimitAngle * DegreesToRadians, value.ZLimitAngle * DegreesToRadians, value.ContactDist);
        limit.stiffness = value.Spring.Stiffness;
        limit.damping = value.Spring.Damping;
        limit.restitution = value.Restitution;
        joint->setLimitCone(limit);
    }
}

void SphericalJoint::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Joint::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(SphericalJoint);

    SERIALIZE_MEMBER(Flags, _flags);
    SERIALIZE_MEMBER(ContactDist, _limit.ContactDist);
    SERIALIZE_MEMBER(Restitution, _limit.Restitution);
    SERIALIZE_MEMBER(Stiffness, _limit.Spring.Stiffness);
    SERIALIZE_MEMBER(Damping, _limit.Spring.Damping);
    SERIALIZE_MEMBER(YLimitAngle, _limit.YLimitAngle);
    SERIALIZE_MEMBER(ZLimitAngle, _limit.ZLimitAngle);
}

void SphericalJoint::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Joint::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Flags, _flags);
    DESERIALIZE_MEMBER(ContactDist, _limit.ContactDist);
    DESERIALIZE_MEMBER(Restitution, _limit.Restitution);
    DESERIALIZE_MEMBER(Stiffness, _limit.Spring.Stiffness);
    DESERIALIZE_MEMBER(Damping, _limit.Spring.Damping);
    DESERIALIZE_MEMBER(YLimitAngle, _limit.YLimitAngle);
    DESERIALIZE_MEMBER(ZLimitAngle, _limit.ZLimitAngle);
}

PxJoint* SphericalJoint::CreateJoint(JointData& data)
{
    const PxTransform trans0(C2P(data.Pos0), C2P(data.Rot0));
    const PxTransform trans1(C2P(data.Pos1), C2P(data.Rot1));
    auto joint = PxSphericalJointCreate(*data.Physics, data.Actor0, trans0, data.Actor1, trans1);

    int32 flags = 0;
    if (_flags & SphericalJointFlag::Limit)
        flags |= PxSphericalJointFlag::eLIMIT_ENABLED;
    joint->setSphericalJointFlags(static_cast<PxSphericalJointFlag::Enum>(flags));
    PxJointLimitCone limit(_limit.YLimitAngle * DegreesToRadians, _limit.ZLimitAngle * DegreesToRadians, _limit.ContactDist);
    limit.stiffness = _limit.Spring.Stiffness;
    limit.damping = _limit.Spring.Damping;
    limit.restitution = _limit.Restitution;
    joint->setLimitCone(limit);

    return joint;
}
