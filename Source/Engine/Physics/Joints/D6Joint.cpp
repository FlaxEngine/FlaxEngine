// Copyright (c) Wojciech Figat. All rights reserved.

#include "D6Joint.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Physics/PhysicsBackend.h"

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
        PhysicsBackend::SetD6JointMotion(_joint, axis, value);
}

void D6Joint::SetDrive(const D6JointDriveType index, const D6JointDrive& value)
{
    if (value == GetDrive(index))
        return;
    _drive[static_cast<int32>(index)] = value;
    if (_joint)
        PhysicsBackend::SetD6JointDrive(_joint, index, value);
}

void D6Joint::SetLimitLinear(const LimitLinear& value)
{
    if (value == _limitLinear)
        return;
    _limitLinear = value;
    if (_joint)
        PhysicsBackend::SetD6JointLimitLinear(_joint, value);
}

void D6Joint::SetLimitTwist(const LimitAngularRange& value)
{
    if (value == _limitTwist)
        return;
    _limitTwist = value;
    if (_joint)
        PhysicsBackend::SetD6JointLimitTwist(_joint, value);
}

void D6Joint::SetLimitSwing(const LimitConeRange& value)
{
    if (value == _limitSwing)
        return;
    _limitSwing = value;
    if (_joint)
        PhysicsBackend::SetD6JointLimitSwing(_joint, value);
}

Vector3 D6Joint::GetDrivePosition() const
{
    return _joint ? PhysicsBackend::GetD6JointDrivePosition(_joint) : Vector3::Zero;
}

void D6Joint::SetDrivePosition(const Vector3& value)
{
    if (_joint)
        PhysicsBackend::SetD6JointDrivePosition(_joint, value);
}

Quaternion D6Joint::GetDriveRotation() const
{
    return _joint ? PhysicsBackend::GetD6JointDriveRotation(_joint) : Quaternion::Identity;
}

void D6Joint::SetDriveRotation(const Quaternion& value)
{
    if (_joint)
        PhysicsBackend::SetD6JointDriveRotation(_joint, value);
}

Vector3 D6Joint::GetDriveLinearVelocity() const
{
    Vector3 linear = Vector3::Zero, angular;
    if (_joint)
        PhysicsBackend::GetD6JointDriveVelocity(_joint, linear, angular);
    return linear;
}

void D6Joint::SetDriveLinearVelocity(const Vector3& value)
{
    if (_joint)
    {
        Vector3 linear, angular;
        PhysicsBackend::GetD6JointDriveVelocity(_joint, linear, angular);
        PhysicsBackend::SetD6JointDriveVelocity(_joint, value, angular);
    }
}

Vector3 D6Joint::GetDriveAngularVelocity() const
{
    Vector3 linear, angular = Vector3::Zero;
    if (_joint)
        PhysicsBackend::GetD6JointDriveVelocity(_joint, linear, angular);
    return angular;
}

void D6Joint::SetDriveAngularVelocity(const Vector3& value)
{
    if (_joint)
    {
        Vector3 linear, angular;
        PhysicsBackend::GetD6JointDriveVelocity(_joint, linear, angular);
        PhysicsBackend::SetD6JointDriveVelocity(_joint, linear, value);
    }
}

float D6Joint::GetCurrentTwist() const
{
    return _joint ? PhysicsBackend::GetD6JointTwist(_joint) : 0.0f;
}

float D6Joint::GetCurrentSwingY() const
{
    return _joint ? PhysicsBackend::GetD6JointSwingY(_joint) : 0.0f;
}

float D6Joint::GetCurrentSwingZ() const
{
    return _joint ? PhysicsBackend::GetD6JointSwingZ(_joint) : 0.0f;
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
    const Quaternion xRot = Quaternion::LookRotation(Vector3::UnitX, Vector3::UnitY);
    const Quaternion sourceRotation = GetOrientation() * xRot;
    const Vector3 target = GetTargetPosition();
    const Quaternion targetRotation = GetTargetOrientation() * xRot;
    const float swingSize = 15.0f;
    const float twistSize = 9.0f;
    const Color swingColor = Color::Green.AlphaMultiplied(0.6f);
    const Color twistColor = Color::Yellow.AlphaMultiplied(0.5f);
    const float arrowSize = swingSize / 100.0f * 0.5f;
    DEBUG_DRAW_WIRE_ARROW(target, targetRotation, arrowSize, arrowSize * 0.5f, Color::Red, 0, false);
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
        DEBUG_DRAW_ARC(source, sourceRotation * Quaternion::RotationYawPitchRoll(0, 0, lower), twistSize, upper - lower, twistColor, 0, false);
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

void* D6Joint::CreateJoint(const PhysicsJointDesc& desc)
{
    void* joint = PhysicsBackend::CreateD6Joint(desc);
    for (int32 i = 0; i < static_cast<int32>(D6JointAxis::MAX); i++)
        PhysicsBackend::SetD6JointMotion(joint, (D6JointAxis)i, _motion[i]);
    for (int32 i = 0; i < static_cast<int32>(D6JointAxis::MAX); i++)
        PhysicsBackend::SetD6JointDrive(joint, (D6JointDriveType)i, _drive[i]);
    PhysicsBackend::SetD6JointLimitLinear(joint, _limitLinear);
    PhysicsBackend::SetD6JointLimitTwist(joint, _limitTwist);
    PhysicsBackend::SetD6JointLimitSwing(joint, _limitSwing);
    return joint;
}
