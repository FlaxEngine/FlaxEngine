// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"

/// <summary>
/// Controls spring parameters for a physics joint limits. If a limit is soft (body bounces back due to restitution when 
/// the limit is reached) the spring will pull the body back towards the limit using the specified parameters.
/// </summary>
API_STRUCT() struct SpringParameters
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(SpringParameters);

    /// <summary>
    /// The spring strength. Force proportional to the position error.
    /// </summary>
    API_FIELD() float Stiffness;

    /// <summary>
    /// Damping strength. Force proportional to the velocity error.
    /// </summary>
    API_FIELD() float Damping;

public:
    /// <summary>
    /// Constructs a spring with no force.
    /// </summary>
    SpringParameters()
    {
        Stiffness = 0.0f;
        Damping = 0.0f;
    }

    /// <summary>
    /// Constructs a spring.
    /// </summary>
    /// <param name="stiffness">Spring strength. Force proportional to the position error.</param>
    /// <param name="damping">Damping strength. Force proportional to the velocity error.</param>
    SpringParameters(const float stiffness, const float damping)
        : Stiffness(stiffness)
        , Damping(damping)
    {
    }

public:
    bool operator==(const SpringParameters& other) const
    {
        return Stiffness == other.Stiffness && Damping == other.Damping;
    }
};

/// <summary>
/// Represents a joint limit between two distance values. Lower value must be less than the upper value.
/// </summary>
API_STRUCT() struct LimitLinearRange
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(LimitLinearRange);

    /// <summary>
    /// Distance from the limit at which it becomes active. Allows the solver to activate earlier than the limit is reached to avoid breaking the limit.
    /// </summary>
    API_FIELD() float ContactDist = -1.0f;

    /// <summary>
    /// Controls how do objects react when the limit is reached, values closer to zero specify non-elastic collision, while those closer to one specify more elastic (i.e bouncy) collision. Must be in [0, 1] range.
    /// </summary>
    API_FIELD(Attributes="Limit(0.0f, 1.0f)") float Restitution = 0.0f;

    /// <summary>
    /// The spring that controls how are the bodies pulled back towards the limit when they breach it.
    /// </summary>
    API_FIELD() SpringParameters Spring;

    /// <summary>
    /// The lower distance of the limit. Must be less than upper.
    /// </summary>
    API_FIELD() float Lower = 0.0f;

    /// <summary>
    /// The upper distance of the limit. Must be more than lower.
    /// </summary>
    API_FIELD() float Upper = 0.0f;

public:
    /// <summary>
    /// Constructs an empty limit.
    /// </summary>
    LimitLinearRange()
    {
    }

    /// <summary>
    /// Constructs a hard limit. Once the limit is reached the movement of the attached bodies will come to a stop.
    /// </summary>
    /// <param name="lower">The lower distance of the limit. Must be less than upper.</param>
    /// <param name="upper">The upper distance of the limit. Must be more than lower.</param>
    /// <param name="contactDist">Distance from the limit at which it becomes active. Allows the solver to activate earlier than the limit is reached to avoid breaking the limit. Specify -1 for the default.</param>
    LimitLinearRange(const float lower, const float upper, const float contactDist = -1.0f)
        : ContactDist(contactDist)
        , Lower(lower)
        , Upper(upper)
    {
    }

    /// <summary>
    /// Constructs a soft limit. Once the limit is reached the bodies will bounce back according to the restitution parameter and will be pulled back towards the limit by the provided spring.
    /// </summary>
    /// <param name="lower">The lower distance of the limit. Must be less than upper.</param>
    /// <param name="upper">The upper distance of the limit. Must be more than lower.</param>
    /// <param name="spring">The spring that controls how are the bodies pulled back towards the limit when they breach it.</param>
    /// <param name="restitution">Controls how do objects react when the limit is reached, values closer to zero specify non-elastic collision, while those closer to one specify more elastic (i.e bouncy) collision. Must be in [0, 1] range.</param>
    LimitLinearRange(const float lower, const float upper, const SpringParameters& spring, const float restitution = 0.0f)
        : Restitution(restitution)
        , Spring(spring)
        , Lower(lower)
        , Upper(upper)
    {
    }

public:
    bool operator==(const LimitLinearRange& other) const
    {
        return Lower == other.Lower && Upper == other.Upper && ContactDist == other.ContactDist && Restitution == other.Restitution && Spring == other.Spring;
    }
};

/// <summary>
/// Represents a joint limit between zero a single distance value.
/// </summary>
API_STRUCT() struct LimitLinear
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(LimitLinear);

    /// <summary>
    /// Distance from the limit at which it becomes active. Allows the solver to activate earlier than the limit is reached to avoid breaking the limit.
    /// </summary>
    API_FIELD() float ContactDist = -1.0f;

    /// <summary>
    /// Controls how do objects react when the limit is reached, values closer to zero specify non-elastic collision, while those closer to one specify more elastic (i.e bouncy) collision. Must be in [0, 1] range.
    /// </summary>
    API_FIELD(Attributes="Limit(0.0f, 1.0f)") float Restitution = 0.0f;

    /// <summary>
    /// The spring that controls how are the bodies pulled back towards the limit when they breach it.
    /// </summary>
    API_FIELD() SpringParameters Spring;

    /// <summary>
    /// The distance at which the limit becomes active.
    /// </summary>
    API_FIELD() float Extent = 0.0f;

public:
    /// <summary>
    /// Constructs an empty limit.
    /// </summary>
    LimitLinear()
    {
    }

    /// <summary>
    /// Constructs a hard limit. Once the limit is reached the movement of the attached bodies will come to a stop.
    /// </summary>
    /// <param name="extent">The distance at which the limit becomes active.</param>
    /// <param name="contactDist">The distance from the limit at which it becomes active. Allows the solver to activate earlier than the limit is reached to avoid breaking the limit. Specify -1 for the default.</param>
    LimitLinear(const float extent, const float contactDist = -1.0f)
        : ContactDist(contactDist)
        , Extent(extent)
    {
    }

    /// <summary>
    /// Constructs a soft limit. Once the limit is reached the bodies will bounce back according to the restitution parameter and will be pulled back towards the limit by the provided spring.
    /// </summary>
    /// <param name="extent">The distance at which the limit becomes active.</param>
    /// <param name="spring">The spring that controls how are the bodies pulled back towards the limit when they breach it.</param>
    /// <param name="restitution">Controls how do objects react when the limit is reached, values closer to zero specify non-elastic collision, while those closer to one specify more elastic (i.e bouncy) collision. Must be in [0, 1] range.</param>
    LimitLinear(const float extent, const SpringParameters& spring, const float restitution = 0.0f)
        : Restitution(restitution)
        , Spring(spring)
        , Extent(extent)
    {
    }

public:
    bool operator==(const LimitLinear& other) const
    {
        return Extent == other.Extent && ContactDist == other.ContactDist && Restitution == other.Restitution && Spring == other.Spring;
    }
};

/// <summary>
/// Represents a joint limit between two angles.
/// </summary>
API_STRUCT() struct LimitAngularRange
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(LimitAngularRange);

    /// <summary>
    /// Distance from the limit at which it becomes active. Allows the solver to activate earlier than the limit is reached to avoid breaking the limit.
    /// </summary>
    API_FIELD() float ContactDist = -1.0f;

    /// <summary>
    /// Controls how do objects react when the limit is reached, values closer to zero specify non-elastic collision, while those closer to one specify more elastic (i.e bouncy) collision. Must be in [0, 1] range.
    /// </summary>
    API_FIELD(Attributes="Limit(0.0f, 1.0f)") float Restitution = 0.0f;

    /// <summary>
    /// The spring that controls how are the bodies pulled back towards the limit when they breach it.
    /// </summary>
    API_FIELD() SpringParameters Spring;

    /// <summary>
    /// Lower angle of the limit (in degrees). Must be less than upper.
    /// </summary>
    API_FIELD() float Lower = 0.0f;

    /// <summary>
    /// Upper angle of the limit (in degrees). Must be less than lower.
    /// </summary>
    API_FIELD() float Upper = 0.0f;

public:
    /// <summary>
    /// Constructs an empty limit.
    /// </summary>
    LimitAngularRange()
    {
    }

    /// <summary>
    /// Constructs a hard limit. Once the limit is reached the movement of the attached bodies will come to a stop.
    /// </summary>
    /// <param name="lower">The lower angle of the limit (in degrees). Must be less than upper.</param>
    /// <param name="upper">The upper angle of the limit (in degrees). Must be more than lower.</param>
    /// <param name="contactDist">Distance from the limit at which it becomes active. Allows the solver to activate earlier than the limit is reached to avoid breaking the limit. Specify -1 for the default.</param>
    LimitAngularRange(const float lower, const float upper, const float contactDist = -1.0f)
        : ContactDist(contactDist)
        , Lower(lower)
        , Upper(upper)
    {
    }

    /// <summary>
    /// Constructs a soft limit. Once the limit is reached the bodies will bounce back according to the restitution parameter and will be pulled back towards the limit by the provided spring.
    /// </summary>
    /// <param name="lower">The lower angle of the limit. Must be less than upper.</param>
    /// <param name="upper">The upper angle of the limit. Must be more than lower.</param>
    /// <param name="spring">The spring that controls how are the bodies pulled back towards the limit when they breach it.</param>
    /// <param name="restitution">Controls how do objects react when the limit is reached, values closer to zero specify non-elastic collision, while those closer to one specify more elastic (i.e bouncy) collision. Must be in [0, 1] range.</param>
    LimitAngularRange(const float lower, const float upper, const SpringParameters& spring, const float restitution = 0.0f)
        : Restitution(restitution)
        , Spring(spring)
        , Lower(lower)
        , Upper(upper)
    {
    }

public:
    bool operator==(const LimitAngularRange& other) const
    {
        return Lower == other.Lower && Upper == other.Upper && ContactDist == other.ContactDist && Restitution == other.Restitution && Spring == other.Spring;
    }
};

/// <summary>
/// Represents a joint limit that constraints movement to within an elliptical cone.
/// </summary>
API_STRUCT() struct LimitConeRange
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(LimitConeRange);

    /// <summary>
    /// Distance from the limit at which it becomes active. Allows the solver to activate earlier than the limit is reached to avoid breaking the limit.
    /// </summary>
    API_FIELD() float ContactDist = -1.0f;

    /// <summary>
    /// Controls how do objects react when the limit is reached, values closer to zero specify non-elastic collision, while those closer to one specify more elastic (i.e bouncy) collision. Must be in [0, 1] range.
    /// </summary>
    API_FIELD(Attributes="Limit(0.0f, 1.0f)") float Restitution = 0.0f;

    /// <summary>
    /// The spring that controls how are the bodies pulled back towards the limit when they breach it.
    /// </summary>
    API_FIELD() SpringParameters Spring;

    /// <summary>
    /// The Y angle of the cone (in degrees). Movement is constrained between 0 and this angle on the Y axis.
    /// </summary>
    API_FIELD(Attributes="Limit(0.0f, 180.0f)") float YLimitAngle = 90.0f;

    /// <summary>
    /// The Z angle of the cone (in degrees). Movement is constrained between 0 and this angle on the Z axis.
    /// </summary>
    API_FIELD(Attributes="Limit(0.0f, 180.0f)") float ZLimitAngle = 90.0f;

public:
    /// <summary>
    /// Constructs a limit with a 45 degree cone.
    /// </summary>
    LimitConeRange()
    {
    }

    /// <summary>
    /// Constructs a hard limit. Once the limit is reached the movement of the attached bodies will come to a stop.
    /// </summary>
    /// <param name="yLimitAngle">The Y angle of the cone (in degrees). Movement is constrained between 0 and this angle on the Y axis.</param>
    /// <param name="zLimitAngle">The Z angle of the cone (in degrees). Movement is constrained between 0 and this angle on the Z axis.</param>
    /// <param name="contactDist">Distance from the limit at which it becomes active. Allows the solver to activate earlier than the limit is reached to avoid breaking the limit. Specify -1 for the default.</param>
    LimitConeRange(const float yLimitAngle, const float zLimitAngle, const float contactDist = -1.0f)
        : ContactDist(contactDist)
        , YLimitAngle(yLimitAngle)
        , ZLimitAngle(zLimitAngle)
    {
    }

    /// <summary>
    /// Constructs a soft limit. Once the limit is reached the bodies will bounce back according to the restitution parameter and will be pulled back towards the limit by the provided spring.
    /// </summary>
    /// <param name="yLimitAngle">The Y angle of the cone (in degrees). Movement is constrained between 0 and this angle on the Y axis.</param>
    /// <param name="zLimitAngle">The Z angle of the cone (in degrees). Movement is constrained between 0 and this angle on the Z axis.</param>
    /// <param name="spring">The spring that controls how are the bodies pulled back towards the limit when they breach it.</param>
    /// <param name="restitution">Controls how do objects react when the limit is reached, values closer to zero specify non-elastic collision, while those closer to one specify more elastic (i.e bouncy) collision. Must be in [0, 1] range.</param>
    LimitConeRange(const float yLimitAngle, const float zLimitAngle, const SpringParameters& spring, const float restitution = 0.0f)
        : Restitution(restitution)
        , Spring(spring)
        , YLimitAngle(yLimitAngle)
        , ZLimitAngle(zLimitAngle)
    {
    }

public:
    bool operator==(const LimitConeRange& other) const
    {
        return YLimitAngle == other.YLimitAngle && ZLimitAngle == other.ZLimitAngle && ContactDist == other.ContactDist && Restitution == other.Restitution && Spring == other.Spring;
    }
};
