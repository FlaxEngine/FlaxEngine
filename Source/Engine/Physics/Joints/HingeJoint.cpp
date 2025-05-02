// Copyright (c) Wojciech Figat. All rights reserved.

#include "HingeJoint.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Physics/PhysicsBackend.h"

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
        PhysicsBackend::SetHingeJointFlags(_joint, value, _drive.FreeSpin);
}

void HingeJoint::SetLimit(const LimitAngularRange& value)
{
    if (_limit == value)
        return;
    _limit = value;
    if (_joint)
        PhysicsBackend::SetHingeJointLimit(_joint, value);
}

void HingeJoint::SetDrive(const HingeJointDrive& value)
{
    if (_drive == value)
        return;
    _drive = value;
    if (_joint)
        PhysicsBackend::SetHingeJointDrive(_joint, value);
}

float HingeJoint::GetCurrentAngle() const
{
    return _joint ? PhysicsBackend::GetHingeJointAngle(_joint) : 0.0f;
}

float HingeJoint::GetCurrentVelocity() const
{
    return _joint ? PhysicsBackend::GetHingeJointVelocity(_joint) : 0.0f;
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"
#include "Engine/Core/Math/Color.h"

void HingeJoint::OnDebugDrawSelected()
{
    const Vector3 source = GetPosition();
    const Vector3 target = GetTargetPosition();
    const Quaternion xRotation = Quaternion::LookRotation(Vector3::UnitX, Vector3::UnitY);
    const Quaternion sourceRotation = GetOrientation() * xRotation;
    const Quaternion targetRotation = GetTargetOrientation() * xRotation;
    const float size = 15.0f;
    const Color color = Color::Green.AlphaMultiplied(0.6f);
    const float arrowSize = size / 100.0f * 0.5f;
    DEBUG_DRAW_WIRE_ARROW(source, sourceRotation, arrowSize, arrowSize * 0.5f, Color::Red, 0, false);
    DEBUG_DRAW_WIRE_ARROW(target, targetRotation, arrowSize, arrowSize * 0.5f, Color::Blue, 0, false);
    if (EnumHasAnyFlags(_flags, HingeJointFlag::Limit))
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

void* HingeJoint::CreateJoint(const PhysicsJointDesc& desc)
{
    void* joint = PhysicsBackend::CreateHingeJoint(desc);
    PhysicsBackend::SetHingeJointFlags(joint, _flags, _drive.FreeSpin);
    PhysicsBackend::SetHingeJointLimit(joint, _limit);
    PhysicsBackend::SetHingeJointDrive(joint, _drive);
    return joint;
}
