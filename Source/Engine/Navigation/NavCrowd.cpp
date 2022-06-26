// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "NavCrowd.h"
#include "NavMesh.h"
#include "NavMeshRuntime.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include <ThirdParty/recastnavigation/DetourCrowd.h>

NavCrowd::NavCrowd(const SpawnParams& params)
    : ScriptingObject(params)
{
    _crowd = dtAllocCrowd();
}

NavCrowd::~NavCrowd()
{
    dtFreeCrowd(_crowd);
}

bool NavCrowd::Init(float maxAgentRadius, int32 maxAgents, NavMesh* navMesh)
{
    NavMeshRuntime* navMeshRuntime = navMesh ? navMesh->GetRuntime() : NavMeshRuntime::Get();
    return Init(maxAgentRadius, maxAgents, navMeshRuntime);
}

bool NavCrowd::Init(float maxAgentRadius, int32 maxAgents, NavMeshRuntime* navMesh)
{
    if (!_crowd || !navMesh)
        return true;
    return !_crowd->init(maxAgents, maxAgentRadius, navMesh->GetNavMesh());
}

int32 NavCrowd::AddAgent(const Vector3& position, const NavAgentProperties& properties)
{
    const Float3 pos = position;
    dtCrowdAgentParams agentParams;
    InitCrowdAgentParams(agentParams, properties);
    return _crowd->addAgent(pos.Raw, &agentParams);
}

Vector3 NavCrowd::GetAgentPosition(int32 id) const
{
    Vector3 result = Vector3::Zero;
    const dtCrowdAgent* agent = _crowd->getAgent(id);
    if (agent && agent->state != DT_CROWDAGENT_STATE_INVALID)
    {
        result = Float3(agent->npos);
    }
    return result;
}

Vector3 NavCrowd::GetAgentVelocity(int32 id) const
{
    Vector3 result = Vector3::Zero;
    const dtCrowdAgent* agent = _crowd->getAgent(id);
    if (agent && agent->state != DT_CROWDAGENT_STATE_INVALID)
    {
        result = Float3(agent->vel);
    }
    return result;
}

void NavCrowd::SetAgentProperties(int32 id, const NavAgentProperties& properties)
{
    dtCrowdAgentParams agentParams;
    InitCrowdAgentParams(agentParams, properties);
    _crowd->updateAgentParameters(id, &agentParams);
}

void NavCrowd::SetAgentMoveTarget(int32 id, const Vector3& position)
{
    const float* extent = _crowd->getQueryExtents();
    const dtQueryFilter* filter = _crowd->getFilter(0);
    const Float3 pointNavMesh = position;
    dtPolyRef startPoly = 0;
    Float3 nearestPt = pointNavMesh;
    _crowd->getNavMeshQuery()->findNearestPoly(pointNavMesh.Raw, extent, filter, &startPoly, &nearestPt.X);
    _crowd->requestMoveTarget(id, startPoly, nearestPt.Raw);
}

void NavCrowd::SetAgentMoveVelocity(int32 id, const Vector3& velocity)
{
    const Float3 v = velocity;
    _crowd->requestMoveVelocity(id, v.Raw);
}

void NavCrowd::ResetAgentMove(int32 id)
{
    _crowd->resetMoveTarget(id);
}

void NavCrowd::RemoveAgent(int32 id)
{
    _crowd->removeAgent(id);
}

void NavCrowd::Update(float dt)
{
    PROFILE_CPU();
    _crowd->update(Math::Max(dt, ZeroTolerance), nullptr);
}

void NavCrowd::InitCrowdAgentParams(dtCrowdAgentParams& agentParams, const NavAgentProperties& properties)
{
    agentParams.radius = properties.Radius;
    agentParams.height = properties.Height;
    agentParams.maxAcceleration = 10000.0f;
    agentParams.maxSpeed = properties.MaxSpeed;
    agentParams.collisionQueryRange = properties.Radius * 12.0f;
    agentParams.pathOptimizationRange = properties.Radius * 30.0f;
    agentParams.separationWeight = properties.CrowdSeparationWeight;
    agentParams.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OPTIMIZE_TOPO | DT_CROWD_OBSTACLE_AVOIDANCE;
    if (agentParams.separationWeight > 0.001f)
        agentParams.updateFlags |= DT_CROWD_SEPARATION;
    agentParams.obstacleAvoidanceType = 0;
    agentParams.queryFilterType = 0;
    agentParams.userData = this;
}
