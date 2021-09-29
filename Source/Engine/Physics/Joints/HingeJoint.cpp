// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "HingeJoint.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Physics/Utilities.h"
#include <ThirdParty/PhysX/extensions/PxRevoluteJoint.h>

HingeJoint::HingeJoint(const SpawnParams& params)
    : Joint(params)
    , _flags(HingeJointFlag::Limit | HingeJointFlag::Drive)
{
    _limit.Lower = -90.0f;
    _limit.Upper = 90.0f;
}

void HingeJoint::SetFlags(const HingeJointFlag value)
{
    if (_flags == value)
        return;

    _flags = value;

    if (_joint)
    {
        auto joint = static_cast<PxRevoluteJoint*>(_joint);
        joint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, (_flags & HingeJointFlag::Limit) != 0);
        joint->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, (_flags & HingeJointFlag::Drive) != 0);
    }
}

void HingeJoint::SetLimit(const LimitAngularRange& value)
{
    if (_limit == value)
        return;

    _limit = value;

    if (_joint)
    {
        auto joint = static_cast<PxRevoluteJoint*>(_joint);
        PxJointAngularLimitPair limit(value.Lower * DegreesToRadians, value.Upper * DegreesToRadians, value.ContactDist);
        limit.stiffness = value.Spring.Stiffness;
        limit.damping = value.Spring.Damping;
        limit.restitution = value.Restitution;
        joint->setLimit(limit);
    }
}

void HingeJoint::SetDrive(const HingeJointDrive& value)
{
    if (_drive == value)
        return;

    _drive = value;

    if (_joint)
    {
        auto joint = static_cast<PxRevoluteJoint*>(_joint);
        joint->setDriveVelocity(Math::Max(value.Velocity, 0.0f));
        joint->setDriveForceLimit(Math::Max(value.ForceLimit, 0.0f));
        joint->setDriveGearRatio(Math::Max(value.GearRatio, 0.0f));
        joint->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_FREESPIN, value.FreeSpin);
    }
}

float HingeJoint::GetCurrentAngle() const
{
    return _joint ? static_cast<PxRevoluteJoint*>(_joint)->getAngle() : 0.0f;
}

float HingeJoint::GetCurrentVelocity() const
{
    return _joint ? static_cast<PxRevoluteJoint*>(_joint)->getVelocity() : 0.0f;
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void HingeJoint::OnDebugDrawSelected()
{
    const Vector3 source = GetPosition();
    const Vector3 target = GetTargetPosition();
    const Quaternion xRotation = Quaternion::LookRotation(Vector3::UnitX, Vector3::UnitY);
    const Quaternion sourceRotation = GetOrientation() * xRotation;
    const Quaternion targetRotation = GetTargetOrientation() * xRotation;
    const float size = 15.0f;
    const Color color = Color::Green.AlphaMultiplied(0.6f);
    DebugDraw::DrawWireArrow(source, sourceRotation, size / 100.0f * 0.5f, Color::Red, 0, false);
    DebugDraw::DrawWireArrow(target, targetRotation, size / 100.0f * 0.5f, Color::Blue, 0, false);
    if (_flags & HingeJointFlag::Limit)
    {
        const float upper = Math::Max(_limit.Upper, _limit.Lower);
        const float range = Math::Abs(upper - _limit.Lower);
        DEBUG_DRAW_ARC(source, sourceRotation * Quaternion::Euler(0, 0, _limit.Lower - 90.0f), size, range * DegreesToRadians, color, 0, false);
        DEBUG_DRAW_WIRE_ARC(source, sourceRotation * Quaternion::Euler(0, 0, upper - 90.0f), size, (360.0f - range) * DegreesToRadians, Color::Red.AlphaMultiplied(0.6f), 0, false);
    }
    else
    {
        DEBUG_DRAW_ARC(source, sourceRotation, size, TWO_PI, color, 0, false);
    }
    DEBUG_DRAW_LINE(source, target, Color::Green * 0.6f, 0, false);

    // Base
    Joint::OnDebugDrawSelected();
}

#endif

void HingeJoint::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Joint::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(HingeJoint);

    SERIALIZE_MEMBER(Flags, _flags);
    SERIALIZE_MEMBER(ContactDist, _limit.ContactDist);
    SERIALIZE_MEMBER(Restitution, _limit.Restitution);
    SERIALIZE_MEMBER(Stiffness, _limit.Spring.Stiffness);
    SERIALIZE_MEMBER(Damping, _limit.Spring.Damping);
    SERIALIZE_MEMBER(LowerLimit, _limit.Lower);
    SERIALIZE_MEMBER(UpperLimit, _limit.Upper);
    SERIALIZE_MEMBER(Velocity, _drive.Velocity);
    SERIALIZE_MEMBER(ForceLimit, _drive.ForceLimit);
    SERIALIZE_MEMBER(GearRatio, _drive.GearRatio);
    SERIALIZE_MEMBER(FreeSpin, _drive.FreeSpin);
}

void HingeJoint::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
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
    DESERIALIZE_MEMBER(Velocity, _drive.Velocity);
    DESERIALIZE_MEMBER(ForceLimit, _drive.ForceLimit);
    DESERIALIZE_MEMBER(GearRatio, _drive.GearRatio);
    DESERIALIZE_MEMBER(FreeSpin, _drive.FreeSpin);
}

PxJoint* HingeJoint::CreateJoint(JointData& data)
{
    const PxTransform trans0(C2P(data.Pos0), C2P(data.Rot0));
    const PxTransform trans1(C2P(data.Pos1), C2P(data.Rot1));
    auto joint = PxRevoluteJointCreate(*data.Physics, data.Actor0, trans0, data.Actor1, trans1);

    int32 flags = 0;
    if (_flags & HingeJointFlag::Limit)
        flags |= PxRevoluteJointFlag::eLIMIT_ENABLED;
    if (_flags & HingeJointFlag::Drive)
        flags |= PxRevoluteJointFlag::eDRIVE_ENABLED;
    if (_drive.FreeSpin)
        flags |= PxRevoluteJointFlag::eDRIVE_FREESPIN;
    joint->setRevoluteJointFlags(static_cast<PxRevoluteJointFlag::Enum>(flags));
    joint->setDriveVelocity(_drive.Velocity);
    joint->setDriveForceLimit(_drive.ForceLimit);
    joint->setDriveGearRatio(_drive.GearRatio);
    PxJointAngularLimitPair limit(_limit.Lower * DegreesToRadians, _limit.Upper * DegreesToRadians, _limit.ContactDist);
    limit.stiffness = _limit.Spring.Stiffness;
    limit.damping = _limit.Spring.Damping;
    limit.restitution = _limit.Restitution;
    joint->setLimit(limit);

    return joint;
}
