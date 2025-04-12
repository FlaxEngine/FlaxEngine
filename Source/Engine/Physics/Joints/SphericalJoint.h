// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Joint.h"
#include "Limits.h"

/// <summary>
/// Flags that control spherical joint options.
/// </summary>
API_ENUM(Attributes="Flags") enum class SphericalJointFlag
{
    /// <summary>
    /// The none.
    /// </summary>
    None = 0,

    /// <summary>
    /// The joint cone range limit is enabled.
    /// </summary>
    Limit = 0x1,
};

DECLARE_ENUM_OPERATORS(SphericalJointFlag);

/// <summary>
/// Physics joint that removes all translational degrees of freedom but allows all rotational degrees of freedom. 
/// Essentially this ensures that the anchor points of the two bodies are always coincident. Bodies are allowed to 
/// rotate around the anchor points, and their rotation can be limited by an elliptical cone.
/// </summary>
/// <seealso cref="Joint" />
API_CLASS(Attributes="ActorContextMenu(\"New/Physics/Joints/Spherical Joint\"), ActorToolbox(\"Physics\")")
class FLAXENGINE_API SphericalJoint : public Joint
{
    DECLARE_SCENE_OBJECT(SphericalJoint);
private:
    SphericalJointFlag _flags;
    LimitConeRange _limit;

public:
    /// <summary>
    /// Gets the joint mode flags. Controls joint behaviour.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(100), DefaultValue(SphericalJointFlag.Limit), EditorDisplay(\"Joint\")")
    FORCE_INLINE SphericalJointFlag GetFlags() const
    {
        return _flags;
    }

    /// <summary>
    /// Sets the joint mode flags. Controls joint behaviour.
    /// </summary>
    API_PROPERTY() void SetFlags(const SphericalJointFlag value);

    /// <summary>
    /// Gets the joint limit properties.
    /// </summary>
    /// <remarks>Determines the limit of the joint. Limit constrains the motion to the specified angle range. You must enable the limit flag on the joint in order for this to be recognized.</remarks>
    API_PROPERTY(Attributes="EditorOrder(110), EditorDisplay(\"Joint\")")
    FORCE_INLINE LimitConeRange GetLimit() const
    {
        return _limit;
    }

    /// <summary>
    /// Sets the joint limit properties.
    /// </summary>
    /// <remarks>Determines a limit that constrains the movement of the joint to a specific minimum and maximum distance. You must enable the limit flag on the joint in order for this to be recognized.</remarks>
    API_PROPERTY() void SetLimit(const LimitConeRange& value);

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
