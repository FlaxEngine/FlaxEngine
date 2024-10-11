// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Joint.h"
#include "Limits.h"

/// <summary>
/// Controls distance joint options.
/// </summary>
API_ENUM(Attributes="Flags") enum class DistanceJointFlag
{
    /// <summary>
    /// The none limits.
    /// </summary>
    None = 0,

    /// <summary>
    /// The minimum distance limit.
    /// </summary>
    MinDistance = 0x1,

    /// <summary>
    /// Uses the maximum distance limit.
    /// </summary>
    MaxDistance = 0x2,

    /// <summary>
    /// Uses the spring when maintaining limits
    /// </summary>
    Spring = 0x4,
};

DECLARE_ENUM_OPERATORS(DistanceJointFlag);

/// <summary>
/// Physics joint that maintains an upper or lower (or both) bound on the distance between two bodies.
/// </summary>
/// <seealso cref="Joint" />
API_CLASS(Attributes="ActorContextMenu(\"New/Physics/Joints/Distance Joint\"), ActorToolbox(\"Physics\")")
class FLAXENGINE_API DistanceJoint : public Joint
{
    DECLARE_SCENE_OBJECT(DistanceJoint);
private:
    DistanceJointFlag _flags;
    float _minDistance;
    float _maxDistance;
    float _tolerance;
    SpringParameters _spring;

public:
    /// <summary>
    /// Gets the joint mode flags. Controls joint behaviour.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(100), EditorDisplay(\"Joint\"), DefaultValue(DistanceJointFlag.MinDistance | DistanceJointFlag.MaxDistance)")
    FORCE_INLINE DistanceJointFlag GetFlags() const
    {
        return _flags;
    }

    /// <summary>
    /// Sets the joint mode flags. Controls joint behaviour.
    /// </summary>
    API_PROPERTY() void SetFlags(DistanceJointFlag value);

    /// <summary>
    /// Gets the allowed minimum distance for the joint.
    /// </summary>
    /// <remarks>Used only when DistanceJointFlag.MinDistance flag is set. The minimum distance must be no more than the maximum distance. Default: 0, Range: [0, float.MaxValue].</remarks>
    API_PROPERTY(Attributes="EditorOrder(110), DefaultValue(0.0f), Limit(0.0f), EditorDisplay(\"Joint\"), ValueCategory(Utils.ValueCategory.Distance)")
    FORCE_INLINE float GetMinDistance() const
    {
        return _minDistance;
    }

    /// <summary>
    /// Sets the allowed minimum distance for the joint.
    /// </summary>
    /// <remarks>Used only when DistanceJointFlag.MinDistance flag is set. The minimum distance must be no more than the maximum distance. Default: 0, Range: [0, float.MaxValue].</remarks>
    API_PROPERTY() void SetMinDistance(float value);

    /// <summary>
    /// Gets the allowed maximum distance for the joint.
    /// </summary>
    /// <remarks>Used only when DistanceJointFlag.MaxDistance flag is set. The maximum distance must be no less than the minimum distance. Default: 0, Range: [0, float.MaxValue].</remarks>
    API_PROPERTY(Attributes="EditorOrder(120), DefaultValue(10.0f), Limit(0.0f), EditorDisplay(\"Joint\"), ValueCategory(Utils.ValueCategory.Distance)")
    FORCE_INLINE float GetMaxDistance() const
    {
        return _maxDistance;
    }

    /// <summary>
    /// Sets the allowed maximum distance for the joint.
    /// </summary>
    /// <remarks>Used only when DistanceJointFlag.MaxDistance flag is set. The maximum distance must be no less than the minimum distance. Default: 0, Range: [0, float.MaxValue].</remarks>
    API_PROPERTY() void SetMaxDistance(float value);

    /// <summary>
    /// Gets the error tolerance of the joint.
    /// </summary>
    /// <remarks>The distance beyond the joint's [min, max] range before the joint becomes active. Default: 25, Range: [0.1, float.MaxValue].</remarks>
    API_PROPERTY(Attributes="EditorOrder(130), DefaultValue(25.0f), Limit(0.0f), EditorDisplay(\"Joint\")")
    FORCE_INLINE float GetTolerance() const
    {
        return _tolerance;
    }

    /// <summary>
    /// Sets the error tolerance of the joint.
    /// </summary>
    /// <remarks>The distance beyond the joint's [min, max] range before the joint becomes active. Default: 25, Range: [0.1, float.MaxValue]. </remarks>
    API_PROPERTY() void SetTolerance(float value);

    /// <summary>
    /// Gets the spring parameters.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(140), EditorDisplay(\"Joint\")")
    FORCE_INLINE SpringParameters GetSpringParameters() const
    {
        return _spring;
    }

    /// <summary>
    /// Sets the spring parameters.
    /// </summary>
    API_PROPERTY() void SetSpringParameters(const SpringParameters& value);

public:
    /// <summary>
    /// Gets the current distance of the joint.
    /// </summary>
    API_PROPERTY() float GetCurrentDistance() const;

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
