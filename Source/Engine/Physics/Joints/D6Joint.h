// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Joint.h"
#include "Limits.h"

/// <summary>
/// Specifies axes that the D6 joint can constrain motion on.
/// </summary>
API_ENUM() enum class D6JointAxis
{
    /// <summary>
    /// Movement on the X axis.
    /// </summary>
    X = 0,

    /// <summary>
    /// Movement on the Y axis.
    /// </summary>
    Y = 1,

    /// <summary>
    /// Movement on the Z axis.
    /// </summary>
    Z = 2,

    /// <summary>
    /// Rotation around the X axis.
    /// </summary>
    Twist = 3,

    /// <summary>
    /// Rotation around the Y axis.
    /// </summary>
    SwingY = 4,

    /// <summary>
    /// Rotation around the Z axis.
    /// </summary>
    SwingZ = 5,

    API_ENUM(Attributes="HideInEditor")
    MAX
};

/// <summary>
/// Specifies type of constraint placed on a specific axis.
/// </summary>
API_ENUM() enum class D6JointMotion
{
    /// <summary>
    /// Axis is immovable.
    /// </summary>
    Locked,

    /// <summary>
    /// Axis will be constrained by the specified limits.
    /// </summary>
    Limited,

    /// <summary>
    /// Axis will not be constrained.
    /// </summary>
    Free,

    API_ENUM(Attributes="HideInEditor")
    MAX
};

/// <summary>
/// Type of drives that can be used for moving or rotating bodies attached to the joint.
/// </summary>
/// <remarks>
/// Each drive is an implicit force-limited damped spring:
/// force = spring * (target position - position) + damping * (targetVelocity - velocity)
/// 
/// Alternatively, the spring may be configured to generate a specified acceleration instead of a force.
/// 
/// A linear axis is affected by drive only if the corresponding drive flag is set.There are two possible models
/// for angular drive : swing / twist, which may be used to drive one or more angular degrees of freedom, or slerp,
/// which may only be used to drive all three angular degrees simultaneously.
/// </remarks>
API_ENUM() enum class D6JointDriveType
{
    /// <summary>
    /// Linear movement on the X axis using the linear drive model.
    /// </summary>
    X = 0,

    /// <summary>
    /// Linear movement on the Y axis using the linear drive model.
    /// </summary>
    Y = 1,

    /// <summary>
    /// Linear movement on the Z axis using the linear drive model.
    /// </summary>
    Z = 2,

    /// <summary>
    /// Rotation around the Y axis using the twist/swing angular drive model. Should not be used together with Slerp mode.
    /// </summary>
    Swing = 3,

    /// <summary>
    /// Rotation around the Z axis using the twist/swing angular drive model. Should not be used together with Slerp mode.
    /// </summary>
    Twist = 4,

    /// <summary>
    /// Rotation using spherical linear interpolation. Uses the SLERP angular drive mode which performs rotation
    /// by interpolating the quaternion values directly over the shortest path (applies to all three axes, which
    /// they all must be unlocked).
    /// </summary>
    Slerp = 5,

    API_ENUM(Attributes="HideInEditor")
    MAX
};

/// <summary>
/// Specifies parameters for a drive that will attempt to move the joint bodies to the specified drive position and velocity.
/// </summary>
API_STRUCT() struct D6JointDrive
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(D6JointDrive);

    /// <summary>
    /// The spring strength. Force proportional to the position error.
    /// </summary>
    API_FIELD() float Stiffness = 0.0f;

    /// <summary>
    /// Damping strength. Force proportional to the velocity error.
    /// </summary>
    API_FIELD() float Damping = 0.0f;

    /// <summary>
    /// The maximum force the drive can apply.
    /// </summary>
    API_FIELD() float ForceLimit = MAX_float;

    /// <summary>
    /// If true the drive will generate acceleration instead of forces. Acceleration drives are easier to tune as they account for the masses of the actors to which the joint is attached.
    /// </summary>
    API_FIELD() bool Acceleration = false;

public:
    bool operator==(const D6JointDrive& other) const
    {
        return Stiffness == other.Stiffness && Damping == other.Damping && ForceLimit == other.ForceLimit && Acceleration == other.Acceleration;
    }
};

/// <summary>
/// Physics joint that is the most customizable type of joint. This joint type can be used to create all other built-in joint 
/// types, and to design your own custom ones, but is less intuitive to use. Allows a specification of a linear 
/// constraint (for example for a slider), twist constraint (rotating around X) and swing constraint (rotating around Y and Z). 
/// It also allows you to constrain limits to only specific axes or completely lock specific axes.
/// </summary>
/// <seealso cref="Joint" />
API_CLASS(Attributes="ActorContextMenu(\"New/Physics/Joints/D6 Joint\"), ActorToolbox(\"Physics\")")
class FLAXENGINE_API D6Joint : public Joint
{
    DECLARE_SCENE_OBJECT(D6Joint);
private:
    D6JointMotion _motion[static_cast<int32>(D6JointAxis::MAX)];
    D6JointDrive _drive[static_cast<int32>(D6JointDriveType::MAX)];
    LimitLinear _limitLinear;
    LimitAngularRange _limitTwist;
    LimitConeRange _limitSwing;

public:
    /// <summary>
    /// Gets the motion type around the specified axis.
    /// </summary>
    /// <remarks>Each axis may independently specify that the degree of freedom is locked (blocking relative movement along or around this axis), limited by the corresponding limit, or free.</remarks>
    /// <param name="axis">The axis the degree of freedom around which the motion type is specified.</param>
    /// <returns>The value.</returns>
    API_FUNCTION() FORCE_INLINE D6JointMotion GetMotion(const D6JointAxis axis) const
    {
        return _motion[static_cast<int32>(axis)];
    }

    /// <summary>
    /// Sets the motion type around the specified axis.
    /// </summary>
    /// <remarks>Each axis may independently specify that the degree of freedom is locked (blocking relative movement along or around this axis), limited by the corresponding limit, or free.</remarks>
    /// <param name="axis">The axis the degree of freedom around which the motion type is specified.</param>
    /// <param name="value">The value.</param>
    API_FUNCTION() void SetMotion(const D6JointAxis axis, const D6JointMotion value);

    /// <summary>
    /// Gets the drive parameters for the specified drive type.
    /// </summary>
    /// <param name="index">The type of drive being specified.</param>
    /// <returns>The value.</returns>
    API_FUNCTION() FORCE_INLINE D6JointDrive GetDrive(const D6JointDriveType index) const
    {
        return _drive[static_cast<int32>(index)];
    }

    /// <summary>
    /// Sets the drive parameters for the specified drive type.
    /// </summary>
    /// <param name="index">The type of drive being specified.</param>
    /// <param name="value">The value.</param>
    API_FUNCTION() void SetDrive(const D6JointDriveType index, const D6JointDrive& value);

public:
    /// <summary>
    /// Determines the linear limit used for constraining translation degrees of freedom.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(200), EditorDisplay(\"Joint\")")
    FORCE_INLINE LimitLinear GetLimitLinear() const
    {
        return _limitLinear;
    }

    /// <summary>
    /// Determines the linear limit used for constraining translation degrees of freedom.
    /// </summary>
    API_PROPERTY() void SetLimitLinear(const LimitLinear& value);

    /// <summary>
    /// Determines the angular limit used for constraining the twist (rotation around X) degree of freedom.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(210), EditorDisplay(\"Joint\")")
    FORCE_INLINE LimitAngularRange GetLimitTwist() const
    {
        return _limitTwist;
    }

    /// <summary>
    /// Determines the angular limit used for constraining the twist (rotation around X) degree of freedom.
    /// </summary>
    API_PROPERTY() void SetLimitTwist(const LimitAngularRange& value);

    /// <summary>
    /// Determines the cone limit used for constraining the swing (rotation around Y and Z) degree of freedom.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(220), EditorDisplay(\"Joint\")")
    FORCE_INLINE LimitConeRange GetLimitSwing() const
    {
        return _limitSwing;
    }

    /// <summary>
    /// Determines the cone limit used for constraining the swing (rotation around Y and Z) degree of freedom.
    /// </summary>
    API_PROPERTY() void SetLimitSwing(const LimitConeRange& value);

public:
    /// <summary>
    /// Gets the drive's target position relative to the joint's first body.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor") Vector3 GetDrivePosition() const;

    /// <summary>
    /// Sets the drive's target position relative to the joint's first body.
    /// </summary>
    API_PROPERTY() void SetDrivePosition(const Vector3& value);

    /// <summary>
    /// Gets the drive's target rotation relative to the joint's first body.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor") Quaternion GetDriveRotation() const;

    /// <summary>
    /// Sets the drive's target rotation relative to the joint's first body.
    /// </summary>
    API_PROPERTY() void SetDriveRotation(const Quaternion& value);

    /// <summary>
    /// Gets the drive's target linear velocity.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor") Vector3 GetDriveLinearVelocity() const;

    /// <summary>
    /// Sets the drive's target linear velocity.
    /// </summary>
    API_PROPERTY() void SetDriveLinearVelocity(const Vector3& value);

    /// <summary>
    /// Gets the drive's target angular velocity.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor") Vector3 GetDriveAngularVelocity() const;

    /// <summary>
    /// Sets the drive's target angular velocity.
    /// </summary>
    API_PROPERTY() void SetDriveAngularVelocity(const Vector3& value);

public:
    /// <summary>
    /// Gets the twist angle of the joint (in the range (-2*Pi, 2*Pi]).
    /// </summary>
    API_PROPERTY() float GetCurrentTwist() const;

    /// <summary>
    /// Gets the current swing angle of the joint from the Y axis.
    /// </summary>
    API_PROPERTY() float GetCurrentSwingY() const;

    /// <summary>
    /// Gets the current swing angle of the joint from the Z axis.
    /// </summary>
    API_PROPERTY() float GetCurrentSwingZ() const;

public:
    // [Joint]
#if USE_EDITOR
    void OnDebugDrawSelected() override;
#endif
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;

protected:
    // [Joint]
    void* CreateJoint(const PhysicsJointDesc& desc) override;
};
