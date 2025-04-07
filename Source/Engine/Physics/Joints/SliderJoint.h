// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Joint.h"
#include "Limits.h"

/// <summary>
/// Flags that control slider joint options.
/// </summary>
API_ENUM(Attributes="Flags") enum class SliderJointFlag
{
    /// <summary>
    /// The none.
    /// </summary>
    None = 0,

    /// <summary>
    /// The joint linear range limit is enabled.
    /// </summary>
    Limit = 0x1,
};

DECLARE_ENUM_OPERATORS(SliderJointFlag);

/// <summary>
/// Physics joint that removes all but a single translational degree of freedom. Bodies are allowed to move along a single axis.
/// </summary>
/// <seealso cref="Joint" />
API_CLASS(Attributes="ActorContextMenu(\"New/Physics/Joints/Slider Joint\"), ActorToolbox(\"Physics\")")
class FLAXENGINE_API SliderJoint : public Joint
{
    DECLARE_SCENE_OBJECT(SliderJoint);
private:
    SliderJointFlag _flags;
    LimitLinearRange _limit;

public:
    /// <summary>
    /// Gets the joint mode flags. Controls joint behaviour.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(100), DefaultValue(SliderJointFlag.Limit), EditorDisplay(\"Joint\")")
    FORCE_INLINE SliderJointFlag GetFlags() const
    {
        return _flags;
    }

    /// <summary>
    /// Sets the joint mode flags. Controls joint behaviour.
    /// </summary>
    API_PROPERTY() void SetFlags(const SliderJointFlag value);

    /// <summary>
    /// Gets the joint limit properties.
    /// </summary>
    /// <remarks>Determines the limit of the joint. Limit constrains the motion to the specified angle range. You must enable the limit flag on the joint in order for this to be recognized.</remarks>
    API_PROPERTY(Attributes="EditorOrder(110), EditorDisplay(\"Joint\")")
    FORCE_INLINE LimitLinearRange GetLimit() const
    {
        return _limit;
    }

    /// <summary>
    /// Sets the joint limit properties.
    /// </summary>
    /// <remarks>Determines a limit that constrains the movement of the joint to a specific minimum and maximum distance. You must enable the limit flag on the joint in order for this to be recognized.</remarks>
    API_PROPERTY() void SetLimit(const LimitLinearRange& value);

public:
    /// <summary>
    /// Gets the current displacement of the joint along its axis.
    /// </summary>
    API_PROPERTY() float GetCurrentPosition() const;

    /// <summary>
    /// Gets the current velocity of the joint along its axis.
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
