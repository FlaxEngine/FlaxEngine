// Copyright (c) Wojciech Figat. All rights reserved.

#include "Navigation.h"
#include "NavigationSettings.h"
#include "NavMeshRuntime.h"
#include "NavMeshBuilder.h"
#include "NavMesh.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/JsonAsset.h"
#include "Engine/Threading/Threading.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#include "Editor/Managed/ManagedEditor.h"
#include "Engine/Level/Level.h"
#include "Engine/Level/Scene/Scene.h"
#endif
#include "Engine/Content/Deprecated.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Serialization/Serialization.h"
#include <ThirdParty/recastnavigation/DetourNavMesh.h>
#include <ThirdParty/recastnavigation/RecastAlloc.h>

namespace
{
    Array<NavMeshRuntime*, InlinedAllocation<16>> NavMeshes;
}

NavMeshRuntime* NavMeshRuntime::Get()
{
    return NavMeshes.Count() != 0 ? NavMeshes[0] : nullptr;
}

NavMeshRuntime* NavMeshRuntime::Get(const StringView& navMeshName)
{
    NavMeshRuntime* result = nullptr;
    for (auto navMesh : NavMeshes)
    {
        if (navMesh->Properties.Name == navMeshName)
        {
            result = navMesh;
            break;
        }
    }
    return result;
}

NavMeshRuntime* NavMeshRuntime::Get(const NavAgentProperties& agentProperties)
{
    NavMeshRuntime* result = nullptr;
    // TODO: maybe build lookup table for agentProperties -> navMesh to improve perf on frequent calls?
    float bestAgentRadiusDiff = -MAX_float;
    float bestAgentHeightDiff = -MAX_float;
    bool bestIsValid = false;
    for (auto navMesh : NavMeshes)
    {
        const auto& navMeshProperties = navMesh->Properties;
        const float agentRadiusDiff = navMeshProperties.Agent.Radius - agentProperties.Radius;
        const float agentHeightDiff = navMeshProperties.Agent.Height - agentProperties.Height;
        const bool isValid = agentRadiusDiff >= 0.0f && agentHeightDiff >= 0.0f;

        // NavMesh must be valid for an agent and be first valid or have better properties than the best matching result so far
        if (isValid
            &&
            (
                !bestIsValid
                ||
                (agentRadiusDiff + agentHeightDiff < bestAgentRadiusDiff + bestAgentHeightDiff)
            )
        )
        {
            result = navMesh;
            bestIsValid = true;
            bestAgentRadiusDiff = agentRadiusDiff;
            bestAgentHeightDiff = agentHeightDiff;
        }
    }
    return result;
}

NavMeshRuntime* NavMeshRuntime::Get(const NavMeshProperties& navMeshProperties, bool createIfMissing)
{
    NavMeshRuntime* result = nullptr;
    for (auto navMesh : NavMeshes)
    {
        if (navMesh->Properties == navMeshProperties)
        {
            result = navMesh;
            break;
        }
    }
    if (!result && createIfMissing)
    {
        // Create a new navmesh
        result = New<NavMeshRuntime>(navMeshProperties);
        NavMeshes.Add(result);
    }
    return result;
}

static_assert(ARRAY_COUNT(NavMeshRuntime::NavAreasCosts) == DT_MAX_AREAS, "Invalid nav areas amount limit.");
float NavMeshRuntime::NavAreasCosts[64];
#if COMPILE_WITH_DEBUG_DRAW
Color NavMeshRuntime::NavAreasColors[64];
#endif

bool NavAgentProperties::operator==(const NavAgentProperties& other) const
{
    return Radius == other.Radius && Height == other.Height && StepHeight == other.StepHeight && MaxSlopeAngle == other.MaxSlopeAngle && MaxSpeed == other.MaxSpeed && CrowdSeparationWeight == other.CrowdSeparationWeight;
}

bool NavAgentMask::IsAgentSupported(int32 agentIndex) const
{
    return (Mask & (1 << agentIndex)) != 0;
}

bool NavAgentMask::IsAgentSupported(const NavAgentProperties& agentProperties) const
{
    auto settings = NavigationSettings::Get();
    for (int32 agentIndex = 0; agentIndex < settings->NavMeshes.Count(); agentIndex++)
    {
        if (settings->NavMeshes[agentIndex].Agent == agentProperties)
        {
            return (Mask & (1 << agentIndex)) != 0;
        }
    }
    return false;
}

bool NavAgentMask::IsNavMeshSupported(const NavMeshProperties& navMeshProperties) const
{
    auto settings = NavigationSettings::Get();
    for (int32 agentIndex = 0; agentIndex < settings->NavMeshes.Count(); agentIndex++)
    {
        if (settings->NavMeshes[agentIndex] == navMeshProperties)
        {
            return (Mask & (1 << agentIndex)) != 0;
        }
    }
    return false;
}

bool NavAgentMask::operator==(const NavAgentMask& other) const
{
    return Mask == other.Mask;
}

bool NavAreaProperties::operator==(const NavAreaProperties& other) const
{
    return Name == other.Name && Id == other.Id && Cost == other.Cost;
}

bool NavMeshProperties::operator==(const NavMeshProperties& other) const
{
    return Name == other.Name && Rotation == other.Rotation && Agent == other.Agent && DefaultQueryExtent == other.DefaultQueryExtent;
}

class NavigationService : public EngineService
{
public:
    NavigationService()
        : EngineService(TEXT("Navigation"), 60)
    {
#if COMPILE_WITH_NAV_MESH_BUILDER
        NavMeshBuilder::Init();
#endif
    }

    bool Init() override;
#if COMPILE_WITH_NAV_MESH_BUILDER
    void Update() override;
#endif
    void Dispose() override;
};

NavigationService NavigationServiceInstance;

void* dtAllocDefault(size_t size, dtAllocHint)
{
    return Allocator::Allocate(size);
}

void* rcAllocDefault(size_t size, rcAllocHint)
{
    return Allocator::Allocate(size);
}

NavigationSettings::NavigationSettings()
{
    // Init navmeshes
    NavMeshes.Resize(1);
    auto& navMesh = NavMeshes[0];
    navMesh.Name = TEXT("Default");

    // Init nav areas
    NavAreas.Resize(2);
    auto& areaNull = NavAreas[0];
    areaNull.Name = TEXT("Null");
    areaNull.Color = Color::Transparent;
    areaNull.Id = 0;
    areaNull.Cost = MAX_float;
    auto& areaWalkable = NavAreas[1];
    areaWalkable.Name = TEXT("Walkable");
    areaWalkable.Color = Color::Transparent;
    areaWalkable.Id = 63;
    areaWalkable.Cost = 1;
}

IMPLEMENT_ENGINE_SETTINGS_GETTER(NavigationSettings, Navigation);

void NavigationSettings::Apply()
{
    // Cache areas properties
    for (auto& area : NavAreas)
    {
        if (area.Id < DT_MAX_AREAS)
        {
            NavMeshRuntime::NavAreasCosts[area.Id] = area.Cost;
#if COMPILE_WITH_DEBUG_DRAW
            NavMeshRuntime::NavAreasColors[area.Id] = area.Color;
#endif
        }
    }

#if USE_EDITOR
    if (!Editor::IsPlayMode && Editor::Managed && Editor::Managed->CanAutoBuildNavMesh())
    {
        // Rebuild all navmeshs after apply changes on navigation
        for (auto scene : Level::Scenes)
        {
            Navigation::BuildNavMesh(scene);
        }
    }
#endif
}

#if USE_EDITOR

void NavigationSettings::Serialize(SerializeStream& stream, const void* otherObj)
{
    SERIALIZE_GET_OTHER_OBJ(NavigationSettings);
    SERIALIZE(AutoAddMissingNavMeshes);
    SERIALIZE(AutoRemoveMissingNavMeshes);
    SERIALIZE(CellHeight);
    SERIALIZE(CellSize);
    SERIALIZE(TileSize);
    SERIALIZE(MinRegionArea);
    SERIALIZE(MergeRegionArea);
    SERIALIZE(MaxEdgeLen);
    SERIALIZE(MaxEdgeError);
    SERIALIZE(DetailSamplingDist);
    SERIALIZE(MaxDetailSamplingError);
    SERIALIZE(NavMeshes);
    SERIALIZE(NavAreas);
}

#endif

void NavigationSettings::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    DESERIALIZE(AutoAddMissingNavMeshes);
    DESERIALIZE(AutoRemoveMissingNavMeshes);
    DESERIALIZE(CellHeight);
    DESERIALIZE(CellSize);
    DESERIALIZE(TileSize);
    DESERIALIZE(MinRegionArea);
    DESERIALIZE(MergeRegionArea);
    DESERIALIZE(MaxEdgeLen);
    DESERIALIZE(MaxEdgeError);
    DESERIALIZE(DetailSamplingDist);
    DESERIALIZE(MaxDetailSamplingError);
    if (modifier->EngineBuild >= 6215)
    {
        DESERIALIZE(NavMeshes);
    }
    else
    {
        // [Deprecated on 12.01.2021, expires on 12.01.2022]
        MARK_CONTENT_DEPRECATED();
        float WalkableRadius = 34.0f;
        float WalkableHeight = 144.0f;
        float WalkableMaxClimb = 35.0f;
        float WalkableMaxSlopeAngle = 60.0f;
        DESERIALIZE(WalkableRadius);
        DESERIALIZE(WalkableHeight);
        DESERIALIZE(WalkableMaxClimb);
        DESERIALIZE(WalkableMaxSlopeAngle);
        NavMeshes.Resize(1);
        auto& navMesh = NavMeshes[0];
        navMesh.Name = TEXT("Default");
        navMesh.Agent.Radius = WalkableRadius;
        navMesh.Agent.Height = WalkableHeight;
        navMesh.Agent.StepHeight = WalkableMaxClimb;
        navMesh.Agent.MaxSlopeAngle = WalkableMaxSlopeAngle;
    }
    DESERIALIZE(NavAreas);
}

bool NavigationService::Init()
{
    // Link memory allocation calls to use engine default allocator
    dtAllocSetCustom(dtAllocDefault, Allocator::Free);
    rcAllocSetCustom(rcAllocDefault, Allocator::Free);

    return false;
}

#if COMPILE_WITH_NAV_MESH_BUILDER

void NavigationService::Update()
{
    NavMeshBuilder::Update();
}

#endif

void NavigationService::Dispose()
{
    // Release nav meshes
    for (auto navMesh : NavMeshes)
    {
        navMesh->Dispose();
        Delete(navMesh);
    }
    NavMeshes.Clear();
    NavMeshes.ClearDelete();
}

bool Navigation::FindDistanceToWall(const Vector3& startPosition, NavMeshHit& hitInfo, float maxDistance)
{
    if (NavMeshes.IsEmpty())
        return false;
    return NavMeshes.First()->FindDistanceToWall(startPosition, hitInfo, maxDistance);
}

bool Navigation::FindPath(const Vector3& startPosition, const Vector3& endPosition, Array<Vector3, HeapAllocation>& resultPath)
{
    if (NavMeshes.IsEmpty())
        return false;
    return NavMeshes.First()->FindPath(startPosition, endPosition, resultPath);
}

bool Navigation::TestPath(const Vector3& startPosition, const Vector3& endPosition)
{
    if (NavMeshes.IsEmpty())
        return false;
    return NavMeshes.First()->TestPath(startPosition, endPosition);
}

bool Navigation::FindClosestPoint(const Vector3& point, Vector3& result)
{
    if (NavMeshes.IsEmpty())
        return false;
    return NavMeshes.First()->FindClosestPoint(point, result);
}

bool Navigation::FindRandomPoint(Vector3& result)
{
    if (NavMeshes.IsEmpty())
        return false;
    return NavMeshes.First()->FindRandomPoint(result);
}

bool Navigation::FindRandomPointAroundCircle(const Vector3& center, float radius, Vector3& result)
{
    if (NavMeshes.IsEmpty())
        return false;
    return NavMeshes.First()->FindRandomPointAroundCircle(center, radius, result);
}

bool Navigation::RayCast(const Vector3& startPosition, const Vector3& endPosition, NavMeshHit& hitInfo)
{
    if (NavMeshes.IsEmpty())
        return false;
    return NavMeshes.First()->RayCast(startPosition, endPosition, hitInfo);
}

#if COMPILE_WITH_NAV_MESH_BUILDER

bool Navigation::IsBuildingNavMesh()
{
    return NavMeshBuilder::IsBuildingNavMesh();
}

float Navigation::GetNavMeshBuildingProgress()
{
    return NavMeshBuilder::GetNavMeshBuildingProgress();
}

void Navigation::BuildNavMesh(Scene* scene, float timeoutMs)
{
    NavMeshBuilder::Build(scene, timeoutMs);
}

void Navigation::BuildNavMesh(Scene* scene, const BoundingBox& dirtyBounds, float timeoutMs)
{
    NavMeshBuilder::Build(scene, dirtyBounds, timeoutMs);
}

#endif

#if COMPILE_WITH_DEBUG_DRAW

void Navigation::DrawNavMesh()
{
    for (auto navMesh : NavMeshes)
    {
#if USE_EDITOR
        // Skip drawing if any of the navmeshes on scene has disabled ShowDebugDraw option
        bool skip = false;
        for (auto scene : Level::Scenes)
        {
            for (auto e : scene->Navigation.Meshes)
            {
                if (e->Properties == navMesh->Properties)
                {
                    if (!e->ShowDebugDraw)
                    {
                        skip = true;
                    }
                    break;
                }
            }
        }
        if (skip)
            continue;
#endif

        navMesh->DebugDraw();
    }
}

#endif
