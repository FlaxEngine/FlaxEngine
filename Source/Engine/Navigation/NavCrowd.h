// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObject.h"
#include "NavigationTypes.h"

class NavMesh;
class NavMeshRuntime;
class dtCrowd;
struct dtCrowdAgentParams;

/// <summary>
/// Navigation steering behaviors system for a group of agents. Handles avoidance between agents by using an adaptive RVO sampling calculation.
/// </summary>
API_CLASS() class FLAXENGINE_API NavCrowd : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE(NavCrowd);
private:
    dtCrowd* _crowd;

public:
    ~NavCrowd();

    /// <summary>
    /// Initializes the crowd.
    /// </summary>
    /// <param name="maxAgentRadius">The maximum radius of any agent that will be added to the crowd.</param>
    /// <param name="maxAgents"> maximum number of agents the crowd can manage.</param>
    /// <param name="navMesh">The navigation mesh to use for crowd movement planning. Use null to pick the first navmesh.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool Init(float maxAgentRadius = 100.0f, int32 maxAgents = 25, NavMesh* navMesh = nullptr);

    /// <summary>
    /// Initializes the crowd.
    /// </summary>
    /// <param name="agentProperties">The properties of the agents that will be added to the crowd.</param>
    /// <param name="maxAgents"> maximum number of agents the crowd can manage.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool Init(const NavAgentProperties& agentProperties, int32 maxAgents = 25);

    /// <summary>
    /// Initializes the crowd.
    /// </summary>
    /// <param name="maxAgentRadius">The maximum radius of any agent that will be added to the crowd.</param>
    /// <param name="maxAgents"> maximum number of agents the crowd can manage.</param>
    /// <param name="navMesh">The navigation mesh to use for crowd movement planning.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool Init(float maxAgentRadius, int32 maxAgents, NavMeshRuntime* navMesh);

    /// <summary>
    /// Adds a new agent to the crowd.
    /// </summary>
    /// <param name="position">The agent position.</param>
    /// <param name="properties">The agent properties.</param>
    /// <returns>The agent unique ID or -1 if failed to add it (eg. too many active agents).</returns>
    API_FUNCTION() int32 AddAgent(const Vector3& position, const NavAgentProperties& properties);

    /// <summary>
    /// Gets the agent current position.
    /// </summary>
    /// <param name="id">The agent ID.</param>
    /// <returns>The agent current position.</returns>
    API_FUNCTION() Vector3 GetAgentPosition(int32 id) const;

    /// <summary>
    /// Sets the agent current position.
    /// </summary>
    /// <param name="id">The agent ID.</param>
    /// <param name="position">The agent position.</param>
    API_FUNCTION() void SetAgentPosition(int32 id, const Vector3& position);

    /// <summary>
    /// Gets the agent current velocity (direction * speed).
    /// </summary>
    /// <param name="id">The agent ID.</param>
    /// <returns>The agent current velocity (direction * speed).</returns>
    API_FUNCTION() Vector3 GetAgentVelocity(int32 id) const;

    /// <summary>
    /// Sets the agent current velocity (direction * speed).
    /// </summary>
    /// <param name="id">The agent ID.</param>
    /// <param name="velocity">The agent velocity (direction * speed).</param>
    API_FUNCTION() void SetAgentVelocity(int32 id, const Vector3& velocity);

    /// <summary>
    /// Updates the agent properties.
    /// </summary>
    /// <param name="id">The agent ID.</param>
    /// <param name="properties">The agent properties.</param>
    API_FUNCTION() void SetAgentProperties(int32 id, const NavAgentProperties& properties);

    /// <summary>
    /// Updates the agent movement target position.
    /// </summary>
    /// <param name="id">The agent ID.</param>
    /// <param name="position">The agent target position.</param>
    API_FUNCTION() void SetAgentMoveTarget(int32 id, const Vector3& position);

    /// <summary>
    /// Updates the agent movement target velocity (direction * speed).
    /// </summary>
    /// <param name="id">The agent ID.</param>
    /// <param name="velocity">The agent target velocity (direction * speed).</param>
    API_FUNCTION() void SetAgentMoveVelocity(int32 id, const Vector3& velocity);

    /// <summary>
    /// Resets any movement request for the specified agent.
    /// </summary>
    /// <param name="id">The agent ID.</param>
    API_FUNCTION() void ResetAgentMove(int32 id);

    /// <summary>
    /// Removes the agent of the given ID.
    /// </summary>
    API_FUNCTION() void RemoveAgent(int32 id);

    /// <summary>
    /// Updates the steering and positions of all agents.
    /// </summary>
    /// <param name="dt">The simulation update delta time (in seconds).</param>
    API_FUNCTION() void Update(float dt);

private:
    void InitCrowdAgentParams(dtCrowdAgentParams& agentParams, const NavAgentProperties& properties);
};
