// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "D6Joint.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Physics/Utilities.h"
#include "Engine/Physics/Physics.h"
#include <ThirdParty/PhysX/extensions/PxD6Joint.h>

D6Joint::D6Joint(const SpawnParams& params)
    : Joint(params)
{
    for (int32 i = 0; i < static_cast<int32>(D6JointAxis::MAX); i++)
        _motion[i] = D6JointMotion::Locked;
    _limitLinear.Extent = 100.0f;
}

void D6Joint::SetMotion(const D6JointAxis axis, const D6JointMotion value)
{
    if (value == GetMotion(axis))
        return;

    _motion[static_cast<int32>(axis)] = value;

    if (_joint)
    {
        auto joint = static_cast<PxD6Joint*>(_joint);
        joint->setMotion(static_cast<PxD6Axis::Enum>(axis), static_cast<PxD6Motion::Enum>(value));
    }
}

void D6Joint::SetDrive(const D6JointDriveType index, const D6JointDrive& value)
{
    if (value == GetDrive(index))
        return;

    _drive[static_cast<int32>(index)] = value;

    if (_joint)
    {
        auto joint = static_cast<PxD6Joint*>(_joint);
        PxD6JointDrive drive;
        if (value.Acceleration)
            drive.flags = PxD6JointDriveFlag::eACCELERATION;
        drive.stiffness = value.Stiffness;
        drive.damping = value.Damping;
        drive.forceLimit = value.ForceLimit;
        ASSERT_LOW_LAYER(drive.isValid());
        joint->setDrive(static_cast<PxD6Drive::Enum>(index), drive);
    }
}

void D6Joint::SetLimitLinear(const LimitLinear& value)
{
    if (value == _limitLinear)
        return;

    _limitLinear = value;

    if (_joint)
    {
        auto joint = static_cast<PxD6Joint*>(_joint);
        PxJointLinearLimit limit(*Physics::GetTolerancesScale(), Math::Max(value.Extent, ZeroTolerance), value.ContactDist);
        limit.stiffness = value.Spring.Stiffness;
        limit.damping = value.Spring.Damping;
        limit.restitution = value.Restitution;
        ASSERT_LOW_LAYER(limit.isValid());
        joint->setLinearLimit(limit);
    }
}

void D6Joint::SetLimitTwist(const LimitAngularRange& value)
{
    if (value == _limitTwist)
        return;

    _limitTwist = value;

    if (_joint)
    {
        auto joint = static_cast<PxD6Joint*>(_joint);
        PxJointAngularLimitPair limit(value.Lower * DegreesToRadians, Math::Max(value.Upper, value.Lower) * DegreesToRadians, value.ContactDist);
        limit.stiffness = value.Spring.Stiffness;
        limit.damping = value.Spring.Damping;
        limit.restitution = value.Restitution;
        ASSERT_LOW_LAYER(limit.isValid());
        joint->setTwistLimit(limit);
    }
}

void D6Joint::SetLimitSwing(const LimitConeRange& value)
{
    if (value == _limitSwing)
        return;

    _limitSwing = value;

    if (_joint)
    {
        auto joint = static_cast<PxD6Joint*>(_joint);
        PxJointLimitCone limit(Math::Clamp(value.YLimitAngle * DegreesToRadians, ZeroTolerance, PI - ZeroTolerance), Math::Clamp(value.ZLimitAngle * DegreesToRadians, ZeroTolerance, PI - ZeroTolerance), value.ContactDist);
        limit.stiffness = value.Spring.Stiffness;
        limit.damping = value.Spring.Damping;
        limit.restitution = value.Restitution;
        ASSERT_LOW_LAYER(limit.isValid());
        joint->setSwingLimit(limit);
    }
}

Vector3 D6Joint::GetDrivePosition() const
{
    return _joint ? P2C(static_cast<PxD6Joint*>(_joint)->getDrivePosition().p) : Vector3::Zero;
}

void D6Joint::SetDrivePosition(const Vector3& value)
{
    if (_joint)
    {
        auto joint = static_cast<PxD6Joint*>(_joint);
        PxTransform t = joint->getDrivePosition();
        t.p = C2P(value);
        joint->setDrivePosition(t);
    }
}

Quaternion D6Joint::GetDriveRotation() const
{
    return _joint ? P2C(static_cast<PxD6Joint*>(_joint)->getDrivePosition().q) : Quaternion::Identity;
}

void D6Joint::SetDriveRotation(const Quaternion& value)
{
    if (_joint)
    {
        auto joint = static_cast<PxD6Joint*>(_joint);
        PxTransform t = joint->getDrivePosition();
        t.q = C2P(value);
        joint->setDrivePosition(t);
    }
}

Vector3 D6Joint::GetDriveLinearVelocity() const
{
    if (_joint)
    {
        PxVec3 linear, angular;
        static_cast<PxD6Joint*>(_joint)->getDriveVelocity(linear, angular);
        return P2C(linear);
    }
    return Vector3::Zero;
}

void D6Joint::SetDriveLinearVelocity(const Vector3& value)
{
    if (_joint)
    {
        PxVec3 linear, angular;
        auto joint = static_cast<PxD6Joint*>(_joint);
        joint->getDriveVelocity(linear, angular);
        linear = C2P(value);
        joint->setDriveVelocity(linear, angular);
    }
}

Vector3 D6Joint::GetDriveAngularVelocity() const
{
    if (_joint)
    {
        PxVec3 linear, angular;
        static_cast<PxD6Joint*>(_joint)->getDriveVelocity(linear, angular);
        return P2C(angular);
    }
    return Vector3::Zero;
}

void D6Joint::SetDriveAngularVelocity(const Vector3& value)
{
    if (_joint)
    {
        PxVec3 linear, angular;
        auto joint = static_cast<PxD6Joint*>(_joint);
        joint->getDriveVelocity(linear, angular);
        angular = C2P(value);
        joint->setDriveVelocity(linear, angular);
    }
}

float D6Joint::GetCurrentTwist() const
{
    return _joint ? static_cast<PxD6Joint*>(_joint)->getTwistAngle() : 0.0f;
}

float D6Joint::GetCurrentSwingYAngle() const
{
    return _joint ? static_cast<PxD6Joint*>(_joint)->getSwingYAngle() : 0.0f;
}

float D6Joint::GetCurrentSwingZAngle() const
{
    return _joint ? static_cast<PxD6Joint*>(_joint)->getSwingZAngle() : 0.0f;
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

float GetAngle(float angle, D6JointMotion motion)
{
    switch (motion)
    {
    case D6JointMotion::Limited:
        return angle * DegreesToRadians;
    case D6JointMotion::Free:
        return PI;
    default:
        return 0.0f;
    }
}

void D6Joint::OnDebugDrawSelected()
{
    const Vector3 source = GetPosition();
    const Vector3 target = GetTargetPosition();
    const Quaternion sourceRotation = GetOrientation();
    const Quaternion targetRotation = GetTargetOrientation();
    const float swingSize = 15.0f;
    const float twistSize = 9.0f;
    const Color swingColor = Color::Green.AlphaMultiplied(0.6f);
    const Color twistColor = Color::Yellow.AlphaMultiplied(0.5f);
    DebugDraw::DrawWireArrow(source, sourceRotation, swingSize / 100.0f * 0.5f, Color::Red, 0, false);
    if (_motion[(int32)D6JointAxis::SwingY] == D6JointMotion::Locked && _motion[(int32)D6JointAxis::SwingZ] == D6JointMotion::Locked)
    {
        // Swing is locked
    }
    else if (_motion[(int32)D6JointAxis::SwingY] == D6JointMotion::Free && _motion[(int32)D6JointAxis::SwingZ] == D6JointMotion::Free)
    {
        // Swing is free
        DEBUG_DRAW_SPHERE(BoundingSphere(source, swingSize), swingColor, 0, false);
    }
    else
    {
        // Swing is limited
        const float angleY = GetAngle(_limitSwing.YLimitAngle, _motion[(int32)D6JointAxis::SwingY]);
        const float angleZ = GetAngle(_limitSwing.ZLimitAngle, _motion[(int32)D6JointAxis::SwingZ]);
        DEBUG_DRAW_CONE(source, sourceRotation, swingSize, angleY, angleZ, swingColor, 0, false);
    }
    if (_motion[(int32)D6JointAxis::Twist] == D6JointMotion::Locked)
    {
        // Twist is locked
    }
    else if (_motion[(int32)D6JointAxis::Twist] == D6JointMotion::Free)
    {
        // Twist is free
        DEBUG_DRAW_ARC(source, sourceRotation, twistSize, TWO_PI, twistColor, 0, false);
    }
    else
    {
        // Twist is limited
        const float lower = _limitTwist.Lower * DegreesToRadians;
        const float upper = Math::Max(lower, _limitTwist.Upper * DegreesToRadians);
        DEBUG_DRAW_ARC(source, sourceRotation * Quaternion::Euler(0, 0, lower), twistSize, upper - lower, twistColor, 0, false);
    }

    // Base
    Joint::OnDebugDrawSelected();
}

#endif

void D6Joint::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Joint::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(D6Joint);

    char motionLabel[8] = "Motion?";
    for (int32 i = 0; i < 6; i++)
    {
        if (!other || _motion[i] != other->_motion[i])
        {
            motionLabel[6] = '0' + i;
            stream.Key(motionLabel, ARRAY_COUNT(motionLabel) - 1);
            stream.Enum(_motion[i]);
        }
    }
    static_assert(ARRAY_COUNT(_motion) == 6, "Invalid motion array size");

    //

    char driveLabel_Stiffness[17] = "Drive?.Stiffness";
    char driveLabel_Damping[17] = "Drive?.Damping";
    char driveLabel_ForceLimit[18] = "Drive?.ForceLimit";
    char driveLabel_Acceleration[20] = "Drive?.Acceleration";
    for (int32 i = 0; i < 6; i++)
    {
        if (!other || _drive[i].Stiffness != other->_drive[i].Stiffness)
        {
            driveLabel_Stiffness[5] = '0' + i;
            stream.Key(driveLabel_Stiffness, ARRAY_COUNT(driveLabel_Stiffness) - 1);
            stream.Float(_drive[i].Stiffness);
        }

        if (!other || _drive[i].Damping != other->_drive[i].Damping)
        {
            driveLabel_Damping[5] = '0' + i;
            stream.Key(driveLabel_Damping, ARRAY_COUNT(driveLabel_Damping) - 1);
            stream.Float(_drive[i].Damping);
        }

        if (!other || _drive[i].ForceLimit != other->_drive[i].ForceLimit)
        {
            driveLabel_ForceLimit[5] = '0' + i;
            stream.Key(driveLabel_ForceLimit, ARRAY_COUNT(driveLabel_ForceLimit) - 1);
            stream.Float(_drive[i].ForceLimit);
        }

        if (!other || _drive[i].Acceleration != other->_drive[i].Acceleration)
        {
            driveLabel_Acceleration[5] = '0' + i;
            stream.Key(driveLabel_Acceleration, ARRAY_COUNT(driveLabel_Acceleration) - 1);
            stream.Bool(_drive[i].Acceleration);
        }
    }
    static_assert(ARRAY_COUNT(_drive) == 6, "Invalid drive array size");

    //

    SERIALIZE_MEMBER(LimitLinear.Extent, _limitLinear.Extent);
    SERIALIZE_MEMBER(LimitLinear.Restitution, _limitLinear.Restitution);
    SERIALIZE_MEMBER(LimitLinear.ContactDist, _limitLinear.ContactDist);
    SERIALIZE_MEMBER(LimitLinear.Stiffness, _limitLinear.Spring.Stiffness);
    SERIALIZE_MEMBER(LimitLinear.Damping, _limitLinear.Spring.Damping);

    //

    SERIALIZE_MEMBER(LimitTwist.Lower, _limitTwist.Lower);
    SERIALIZE_MEMBER(LimitTwist.Upper, _limitTwist.Upper);
    SERIALIZE_MEMBER(LimitTwist.Restitution, _limitTwist.Restitution);
    SERIALIZE_MEMBER(LimitTwist.ContactDist, _limitTwist.ContactDist);
    SERIALIZE_MEMBER(LimitTwist.Stiffness, _limitTwist.Spring.Stiffness);
    SERIALIZE_MEMBER(LimitTwist.Damping, _limitTwist.Spring.Damping);

    //

    SERIALIZE_MEMBER(LimitSwing.YLimitAngle, _limitSwing.YLimitAngle);
    SERIALIZE_MEMBER(LimitSwing.ZLimitAngle, _limitSwing.ZLimitAngle);
    SERIALIZE_MEMBER(LimitSwing.Restitution, _limitSwing.Restitution);
    SERIALIZE_MEMBER(LimitSwing.ContactDist, _limitSwing.ContactDist);
    SERIALIZE_MEMBER(LimitSwing.Stiffness, _limitSwing.Spring.Stiffness);
    SERIALIZE_MEMBER(LimitSwing.Damping, _limitSwing.Spring.Damping);
}

void D6Joint::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Joint::Deserialize(stream, modifier);

    char motionLabel[8] = "Motion?";
    for (int32 i = 0; i < 6; i++)
    {
        motionLabel[6] = '0' + i;
        JsonTools::GetEnum(_motion[i], stream, motionLabel);
    }
    static_assert(ARRAY_COUNT(_motion) == 6, "Invalid motion array size");

    //

    char driveLabel_Stiffness[17] = "Drive?.Stiffness";
    char driveLabel_Damping[17] = "Drive?.Damping";
    char driveLabel_ForceLimit[18] = "Drive?.ForceLimit";
    char driveLabel_Acceleration[20] = "Drive?.Acceleration";
    for (int32 i = 0; i < 6; i++)
    {
        driveLabel_Stiffness[5] = '0' + i;
        driveLabel_Damping[5] = '0' + i;
        driveLabel_ForceLimit[5] = '0' + i;
        driveLabel_Acceleration[5] = '0' + i;

        JsonTools::GetFloat(_drive[i].Stiffness, stream, driveLabel_Stiffness);
        JsonTools::GetFloat(_drive[i].Damping, stream, driveLabel_Damping);
        JsonTools::GetFloat(_drive[i].ForceLimit, stream, driveLabel_ForceLimit);
        JsonTools::GetBool(_drive[i].Acceleration, stream, driveLabel_Acceleration);
    }
    static_assert(ARRAY_COUNT(_drive) == 6, "Invalid drive array size");

    //

    JsonTools::GetFloat(_limitLinear.Extent, stream, "LimitLinear.Extent");
    JsonTools::GetFloat(_limitLinear.Restitution, stream, "LimitLinear.Restitution");
    JsonTools::GetFloat(_limitLinear.ContactDist, stream, "LimitLinear.ContactDist");
    JsonTools::GetFloat(_limitLinear.Spring.Stiffness, stream, "LimitLinear.Stiffness");
    JsonTools::GetFloat(_limitLinear.Spring.Damping, stream, "LimitLinear.Damping");

    //

    JsonTools::GetFloat(_limitTwist.Lower, stream, "LimitTwist.Lower");
    JsonTools::GetFloat(_limitTwist.Upper, stream, "LimitTwist.Upper");
    JsonTools::GetFloat(_limitTwist.Restitution, stream, "LimitTwist.Restitution");
    JsonTools::GetFloat(_limitTwist.ContactDist, stream, "LimitTwist.ContactDist");
    JsonTools::GetFloat(_limitTwist.Spring.Stiffness, stream, "LimitTwist.Stiffness");
    JsonTools::GetFloat(_limitTwist.Spring.Damping, stream, "LimitTwist.Damping");

    //

    JsonTools::GetFloat(_limitSwing.YLimitAngle, stream, "LimitSwing.YLimitAngle");
    JsonTools::GetFloat(_limitSwing.ZLimitAngle, stream, "LimitSwing.ZLimitAngle");
    JsonTools::GetFloat(_limitSwing.Restitution, stream, "LimitSwing.Restitution");
    JsonTools::GetFloat(_limitSwing.ContactDist, stream, "LimitSwing.ContactDist");
    JsonTools::GetFloat(_limitSwing.Spring.Stiffness, stream, "LimitSwing.Stiffness");
    JsonTools::GetFloat(_limitSwing.Spring.Damping, stream, "LimitSwing.Damping");
}

PxJoint* D6Joint::CreateJoint(JointData& data)
{
    const PxTransform trans0(C2P(data.Pos0), C2P(data.Rot0));
    const PxTransform trans1(C2P(data.Pos1), C2P(data.Rot1));
    auto joint = PxD6JointCreate(*data.Physics, data.Actor0, trans0, data.Actor1, trans1);

    for (int32 i = 0; i < static_cast<int32>(D6JointAxis::MAX); i++)
    {
        joint->setMotion(static_cast<PxD6Axis::Enum>(i), static_cast<PxD6Motion::Enum>(_motion[i]));
    }

    for (int32 i = 0; i < static_cast<int32>(D6JointAxis::MAX); i++)
    {
        const auto& value = _drive[i];
        PxD6JointDrive drive;
        if (value.Acceleration)
            drive.flags = PxD6JointDriveFlag::eACCELERATION;
        drive.stiffness = value.Stiffness;
        drive.damping = value.Damping;
        drive.forceLimit = value.ForceLimit;
        ASSERT_LOW_LAYER(drive.isValid());
        joint->setDrive(static_cast<PxD6Drive::Enum>(i), drive);
    }

    {
        const auto& value = _limitLinear;
        PxJointLinearLimit limit(*Physics::GetTolerancesScale(), Math::Max(value.Extent, ZeroTolerance), value.ContactDist);
        limit.stiffness = value.Spring.Stiffness;
        limit.damping = value.Spring.Damping;
        limit.restitution = value.Restitution;
        ASSERT_LOW_LAYER(limit.isValid());
        joint->setDistanceLimit(limit);
    }

    {
        const auto& value = _limitTwist;
        PxJointAngularLimitPair limit(value.Lower * DegreesToRadians, Math::Max(value.Upper, value.Lower) * DegreesToRadians, value.ContactDist);
        limit.stiffness = value.Spring.Stiffness;
        limit.damping = value.Spring.Damping;
        limit.restitution = value.Restitution;
        ASSERT_LOW_LAYER(limit.isValid());
        joint->setTwistLimit(limit);
    }

    {
        const auto& value = _limitSwing;
        PxJointLimitCone limit(Math::Clamp(value.YLimitAngle * DegreesToRadians, ZeroTolerance, PI - ZeroTolerance), Math::Clamp(value.ZLimitAngle * DegreesToRadians, ZeroTolerance, PI -ZeroTolerance), value.ContactDist);
        limit.stiffness = value.Spring.Stiffness;
        limit.damping = value.Spring.Damping;
        limit.restitution = value.Restitution;
        ASSERT_LOW_LAYER(limit.isValid());
        joint->setSwingLimit(limit);
    }

    return joint;
}
