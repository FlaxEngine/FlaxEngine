// Copyright (c) Wojciech Figat. All rights reserved.

#include "NavCrowd.h"
#include "NavMesh.h"
#include "NavMeshRuntime.h"
#include "Engine/Core/Log.h"
#include "Engine/Level/Level.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Threading/Threading.h"
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

bool NavCrowd::Init(const NavAgentProperties& agentProperties, int32 maxAgents)
{
    NavMeshRuntime* navMeshRuntime = NavMeshRuntime::Get(agentProperties);
#if !BUILD_RELEASE
    if (!navMeshRuntime)
    {
        if (NavMeshRuntime::Get())
            LOG(Error, "Cannot create crowd. Failed to find a navmesh that matches a given agent properties.");
        else
            LOG(Error, "Cannot create crowd. No navmesh is loaded.");
    }
#endif
    return Init(agentProperties.Radius * 3.0f, maxAgents, navMeshRuntime);
}

bool NavCrowd::Init(float maxAgentRadius, int32 maxAgents, NavMeshRuntime* navMesh)
{
    if (!_crowd || !navMesh)
        return true;
    PROFILE_CPU();

    // This can happen on game start when no navmesh is loaded yet (eg. navmesh tiles data is during streaming) so wait for navmesh
    if (navMesh->GetNavMesh() == nullptr)
    {
        PROFILE_CPU_NAMED("WaitForNavMesh");
        if (IsInMainThread())
        {
            // Wait for any navmesh data load
            for (const Scene* scene : Level::Scenes)
            {
                for (const NavMesh* actor : scene->Navigation.Meshes)
                {
                    if (actor->DataAsset)
                    {
                        actor->DataAsset->WaitForLoaded();
                        if (navMesh->GetNavMesh())
                            break;
                    }
                }
                if (navMesh->GetNavMesh())
                    break;
            }
        }
        else
        {
            while (navMesh->GetNavMesh() == nullptr)
                Platform::Sleep(1);
        }
        if (navMesh->GetNavMesh() == nullptr)
        {
            LOG(Error, "Cannot create crowd. Navmesh is not yet laoded.");
            return true;
        }
    }

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
    if (agent)
    {
        result = Float3(agent->npos);
    }
    return result;
}

void NavCrowd::SetAgentPosition(int32 id, const Vector3& position)
{
    dtCrowdAgent* agent = _crowd->getEditableAgent(id);
    if (agent)
    {
        *(Float3*)agent->npos = Float3(position);
    }
}

Vector3 NavCrowd::GetAgentVelocity(int32 id) const
{
    Vector3 result = Vector3::Zero;
    const dtCrowdAgent* agent = _crowd->getAgent(id);
    if (agent)
    {
        result = Float3(agent->vel);
    }
    return result;
}

void NavCrowd::SetAgentVelocity(int32 id, const Vector3& velocity)
{
    dtCrowdAgent* agent = _crowd->getEditableAgent(id);
    if (agent)
    {
        *(Float3*)agent->vel = Float3(velocity);
    }
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
    CHECK(id >= 0 && id < _crowd->getAgentCount());
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
