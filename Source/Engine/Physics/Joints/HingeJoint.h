// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Joint.h"
#include "Limits.h"

/// <summary>
/// Flags that control hinge joint options.
/// </summary>
API_ENUM(Attributes="Flags") enum class HingeJointFlag
{
    /// <summary>
    /// The none.
    /// </summary>
    None = 0,

    /// <summary>
    /// The joint limit is enabled.
    /// </summary>
    Limit = 0x1,

    /// <summary>
    /// The joint drive is enabled.
    /// </summary>
    Drive = 0x2,
};

DECLARE_ENUM_OPERATORS(HingeJointFlag);

/// <summary>
/// Properties of a drive that drives the joint's angular velocity towards a particular value.
/// </summary>
API_STRUCT() struct HingeJointDrive
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(HingeJointDrive);

    /// <summary>
    /// Target velocity of the joint.
    /// </summary>
    API_FIELD() float Velocity = 0.0f;

    /// <summary>
    /// Maximum torque the drive is allowed to apply.
    /// </summary>
    API_FIELD(Attributes="Limit(0)") float ForceLimit = MAX_float;

    /// <summary>
    /// Scales the velocity of the first body, and its response to drive torque is scaled down.
    /// </summary>
    API_FIELD(Attributes="Limit(0)") float GearRatio = 1.0f;

    /// <summary>
    /// If the joint is moving faster than the drive's target speed, the drive will try to break.
    /// If you don't want the breaking to happen set this to true.
    /// </summary>
    API_FIELD() bool FreeSpin = false;

public:
    bool operator==(const HingeJointDrive& other) const
    {
        return Math::NearEqual(Velocity, other.Velocity) && Math::NearEqual(ForceLimit, other.ForceLimit) && Math::NearEqual(GearRatio, other.GearRatio) && FreeSpin == other.FreeSpin;
    }
};

/// <summary>
/// Physics joint that removes all but a single rotation degree of freedom from its two attached bodies (for example a door hinge).
/// </summary>
/// <seealso cref="Joint" />
API_CLASS(Attributes="ActorContextMenu(\"New/Physics/Joints/Hinge Joint\"), ActorToolbox(\"Physics\")")
class FLAXENGINE_API HingeJoint : public Joint
{
    DECLARE_SCENE_OBJECT(HingeJoint);
private:
    HingeJointFlag _flags;
    LimitAngularRange _limit;
    HingeJointDrive _drive;

public:
    /// <summary>
    /// Gets the joint mode flags. Controls joint behaviour.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(100), EditorDisplay(\"Joint\"), DefaultValue(HingeJointFlag.Limit | HingeJointFlag.Drive)")
    FORCE_INLINE HingeJointFlag GetFlags() const
    {
        return _flags;
    }

    /// <summary>
    /// Sets the joint mode flags. Controls joint behaviour.
    /// </summary>
    API_PROPERTY() void SetFlags(const HingeJointFlag value);

    /// <summary>
    /// Gets the joint limit properties.
    /// </summary>
    /// <remarks>Determines the limit of the joint. Limit constrains the motion to the specified angle range. You must enable the limit flag on the joint in order for this to be recognized.</remarks>
    API_PROPERTY(Attributes="EditorOrder(110), EditorDisplay(\"Joint\")")
    FORCE_INLINE LimitAngularRange GetLimit() const
    {
        return _limit;
    }

    /// <summary>
    /// Sets the joint limit properties.
    /// </summary>
    /// <remarks>Determines the limit of the joint. Limit constrains the motion to the specified angle range. You must enable the limit flag on the joint in order for this to be recognized.</remarks>
    API_PROPERTY() void SetLimit(const LimitAngularRange& value);

    /// <summary>
    /// Gets the joint drive properties.
    /// </summary>
    /// <remarks>Determines the drive properties of the joint. It drives the joint's angular velocity towards a particular value. You must enable the drive flag on the joint in order for the drive to be active.</remarks>
    API_PROPERTY(Attributes="EditorOrder(120), EditorDisplay(\"Joint\")")
    FORCE_INLINE HingeJointDrive GetDrive() const
    {
        return _drive;
    }

    /// <summary>
    /// Sets the joint drive properties.
    /// </summary>
    /// <remarks>Determines the drive properties of the joint. It drives the joint's angular velocity towards a particular value. You must enable the drive flag on the joint in order for the drive to be active.</remarks>
    API_PROPERTY() void SetDrive(const HingeJointDrive& value);

public:
    /// <summary>
    /// Gets the current angle of the joint (in radians, in the range (-Pi, Pi]).
    /// </summary>
    API_PROPERTY() float GetCurrentAngle() const;

    /// <summary>
    /// Gets the current velocity of the joint.
    /// </summary>
    API_PROPERTY() float GetCurrentVelocity() const;

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
