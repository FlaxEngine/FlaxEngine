// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Core/ISerializable.h"

#define NAV_MESH_PATH_MAX_SIZE 200

/// <summary>
/// The navigation system agent properties container for navmesh building and querying.
/// </summary>
API_STRUCT() struct FLAXENGINE_API NavAgentProperties : ISerializable
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE_MINIMAL(NavAgentProperties);

    /// <summary>
	/// The radius of the agent used for navigation. Agents can't pass through gaps of less than twice the radius.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0)")
    float Radius = 34.0f;

    /// <summary>
	/// The height of the agent used for navigation. Agents can't enter areas with ceilings lower than this value.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10)")
    float Height = 144.0f;

    /// <summary>
	/// The step height used for navigation. Defines the maximum ledge height that is considered to still be traversable by the agent.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20)")
    float StepHeight = 35.0f;

    /// <summary>
    /// The maximum slope (in degrees) that is considered walkable for navigation. Agents can't go up or down slopes higher than this value.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30)")
    float MaxSlopeAngle = 60.0f;

    /// <summary>
    /// The maximum movement speed (units/s).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40)")
    float MaxSpeed = 500.0f;

    /// <summary>
    /// The crowd agent separation weight that defines how aggressive the agent manager should be at avoiding collisions with this agent.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(100)")
    float CrowdSeparationWeight = 2.0f;

    bool operator==(const NavAgentProperties& other) const;

    bool operator!=(const NavAgentProperties& other) const
    {
        return !operator==(other);
    }
};

/// <summary>
/// The navigation mesh properties container for navmesh building.
/// </summary>
API_STRUCT() struct FLAXENGINE_API NavMeshProperties : ISerializable
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE_MINIMAL(NavMeshProperties);

    /// <summary>
    /// The navmesh type name. Identifies different types of the navmeshes, used to sync navmesh properties with settings asset.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0)")
    String Name;

    /// <summary>
    /// The navmesh type color (for debugging).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10)")
    Color Color = Color::Green;

    /// <summary>
    /// The navmesh rotation applied to navigation surface. Used during building to the rotate scene geometry and to revert back result during path finding queries. Can be used to generate navmesh on walls.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20)")
    Quaternion Rotation = Quaternion::Identity;

    /// <summary>
    /// The properties of the agent used to generate walkable navigation surface.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30)")
    NavAgentProperties Agent;

    /// <summary>
    /// The default extents for the nav queries that defines the search distance along each axis (x, y, z). Smaller values prevent queries from snapping to too far locations.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40)")
    Float3 DefaultQueryExtent = Float3(50.0f, 250.0f, 50.0f);

    bool operator==(const NavMeshProperties& other) const;

    bool operator!=(const NavMeshProperties& other) const
    {
        return !operator==(other);
    }
};

/// <summary>
/// The navigation system agents selection mask (from navigation system settings). Uses 1 bit per agent type (up to 32 agents).
/// </summary>
API_STRUCT() struct FLAXENGINE_API NavAgentMask
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(NavAgentMask);

    /// <summary>
    /// The agents selection mask.
    /// </summary>
    API_FIELD() uint32 Mask = MAX_uint32;

    bool IsAgentSupported(int32 agentIndex) const;
    bool IsAgentSupported(const NavAgentProperties& agentProperties) const;
    bool IsNavMeshSupported(const NavMeshProperties& navMeshProperties) const;

    bool operator==(const NavAgentMask& other) const;

    bool operator!=(const NavAgentMask& other) const
    {
        return !operator==(other);
    }
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

/// <summary>
/// The navigation area properties container for navmesh building and navigation runtime.
/// </summary>
API_STRUCT() struct FLAXENGINE_API NavAreaProperties : ISerializable
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE_MINIMAL(NavAreaProperties);

    /// <summary>
    /// The area type name. Identifies different types of the areas.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0)")
    String Name;

    /// <summary>
    /// The area type color (for debugging). Alpha channel is used to blend with navmesh color (alpha 0 to use navmesh color only).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10)")
    Color Color = Color::Transparent;

    /// <summary>
    /// The area id. It must be unique for the project. Valid range 0-63. Value 0 is reserved for Null areas (empty, non-navigable areas).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), Limit(0, 63)")
    byte Id;

    /// <summary>
    /// The cost scale for the area traversal for agents. The higher the cost, the less likely agent wil choose the path that goes over it. For instance, areas that are harder to move like sand should have higher cost for proper path finding.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), Limit(0, float.MaxValue, 0.1f)")
    float Cost = 1;

    bool operator==(const NavAreaProperties& other) const;

    bool operator!=(const NavAreaProperties& other) const
    {
        return !operator==(other);
    }
};
