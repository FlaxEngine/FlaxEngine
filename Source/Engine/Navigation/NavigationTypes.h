// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Quaternion.h"

/// <summary>
/// The navigation system agent properties container for navmesh building and querying.
/// </summary>
API_STRUCT() struct FLAXENGINE_API NavAgentProperties
{
DECLARE_SCRIPTING_TYPE_MINIMAL(NavAgentProperties);

    /// <summary>
	/// The radius of the agent used for navigation. Agents can't pass through gaps of less than twice the radius.
    /// </summary>
    API_FIELD() float Radius = 34.0f;

    /// <summary>
	/// The height of the agent used for navigation. Agents can't enter areas with ceilings lower than this value.
    /// </summary>
    API_FIELD() float Height = 144.0f;

    /// <summary>
	/// The step height used for navigation. Defines the maximum ledge height that is considered to still be traversable by the agent.
    /// </summary>
    API_FIELD() float StepHeight = 35.0f;

    /// <summary>
    /// The maximum slope (in degrees) that is considered walkable for navigation. Agents can't go up or down slopes higher than this value.
    /// </summary>
    API_FIELD() float MaxSlopeAngle = 60.0f;
};

/// <summary>
/// The navigation mesh properties container for navmesh building.
/// </summary>
API_STRUCT() struct FLAXENGINE_API NavMeshProperties
{
DECLARE_SCRIPTING_TYPE_MINIMAL(NavMeshProperties);

    /// <summary>
    /// The navmesh type name (for debugging).
    /// </summary>
    API_FIELD() String Name;

    /// <summary>
    /// The navmesh type color (for debugging).
    /// </summary>
    API_FIELD() Color Color;

    /// <summary>
    /// The navmesh rotation applied to navigation surface. Used during building to the rotate scene geometry and to revert back result during path finding queries. Can be used to generate navmesh on walls.
    /// </summary>
    API_FIELD() Quaternion Rotation;

    /// <summary>
    /// The properties of the agent used to generate walkable navigation surface.
    /// </summary>
    API_FIELD() NavAgentProperties Agent;
};

/// <summary>
/// The result information for navigation mesh queries.
/// </summary>
API_STRUCT() struct FLAXENGINE_API NavMeshHit
{
DECLARE_SCRIPTING_TYPE_MINIMAL(NavMeshHit);

    /// <summary>
    /// The hit point position.
    /// </summary>
    API_FIELD() Vector3 Position;

    /// <summary>
    /// The distance to hit point (from the query origin).
    /// </summary>
    API_FIELD() float Distance;

    /// <summary>
    /// The hit point normal vector.
    /// </summary>
    API_FIELD() Vector3 Normal;
};
