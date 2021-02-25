// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Navigation.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Engine/EngineService.h"
#include "NavMeshBuilder.h"
#include "NavMesh.h"
#include <ThirdParty/recastnavigation/RecastAlloc.h>
#include <ThirdParty/recastnavigation/DetourNavMesh.h>
#include <ThirdParty/recastnavigation/DetourNavMeshQuery.h>

#define DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL 50.0f
#define DEFAULT_NAV_QUERY_EXTENT_VERTICAL 250.0f

NavMesh* _navMesh = nullptr;

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

NavMesh* Navigation::GetNavMesh()
{
    return _navMesh;
}

void* dtAllocDefault(size_t size, dtAllocHint)
{
    return Allocator::Allocate(size);
}

void* rcAllocDefault(size_t size, rcAllocHint)
{
    return Allocator::Allocate(size);
}

bool NavigationService::Init()
{
    // Link memory allocation calls to use engine default allocator
    dtAllocSetCustom(dtAllocDefault, Allocator::Free);
    rcAllocSetCustom(rcAllocDefault, Allocator::Free);

    // Create global nav mesh
    _navMesh = New<NavMesh>();

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
    if (_navMesh)
    {
        _navMesh->Dispose();
        Delete(_navMesh);
        _navMesh = nullptr;
    }
}

bool Navigation::FindDistanceToWall(const Vector3& startPosition, NavMeshHit& hitInfo, float maxDistance)
{
    ScopeLock lock(_navMesh->Locker);

    const auto query = _navMesh->GetNavMeshQuery();
    if (!query || !_navMesh->GetNavMesh())
    {
        return false;
    }

    dtQueryFilter filter;
    Vector3 extent(DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL, DEFAULT_NAV_QUERY_EXTENT_VERTICAL, DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL);

    dtPolyRef startPoly = 0;
    query->findNearestPoly(&startPosition.X, &extent.X, &filter, &startPoly, nullptr);
    if (!startPoly)
    {
        return false;
    }

    return dtStatusSucceed(query->findDistanceToWall(startPoly, &startPosition.X, maxDistance, &filter, &hitInfo.Distance, &hitInfo.Position.X, &hitInfo.Normal.X));
}

bool Navigation::FindPath(const Vector3& startPosition, const Vector3& endPosition, Array<Vector3, HeapAllocation>& resultPath)
{
    resultPath.Clear();
    ScopeLock lock(_navMesh->Locker);

    const auto query = _navMesh->GetNavMeshQuery();
    if (!query || !_navMesh->GetNavMesh())
    {
        return false;
    }

    dtQueryFilter filter;
    Vector3 extent(DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL, DEFAULT_NAV_QUERY_EXTENT_VERTICAL, DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL);

    dtPolyRef startPoly = 0;
    query->findNearestPoly(&startPosition.X, &extent.X, &filter, &startPoly, nullptr);
    if (!startPoly)
    {
        return false;
    }
    dtPolyRef endPoly = 0;
    query->findNearestPoly(&endPosition.X, &extent.X, &filter, &endPoly, nullptr);
    if (!endPoly)
    {
        return false;
    }

    dtPolyRef path[NAV_MESH_PATH_MAX_SIZE];
    int32 pathSize;
    const auto findPathStatus = query->findPath(startPoly, endPoly, &startPosition.X, &endPosition.X, &filter, path, &pathSize, NAV_MESH_PATH_MAX_SIZE);
    if (dtStatusFailed(findPathStatus))
    {
        return false;
    }

    // Check for special case, where path has not been found, and starting polygon was the one closest to the target
    if (pathSize == 1 && dtStatusDetail(findPathStatus, DT_PARTIAL_RESULT))
    {
        // In this case we find a point on starting polygon, that's closest to destination and store it as path end
        resultPath.Resize(2);
        resultPath[0] = startPosition;
        resultPath[1] = startPosition;
        query->closestPointOnPolyBoundary(startPoly, &endPosition.X, &resultPath[1].X);
    }
    else
    {
        int straightPathCount = 0;
        resultPath.EnsureCapacity(NAV_MESH_PATH_MAX_SIZE);
        const auto findStraightPathStatus = query->findStraightPath(&startPosition.X, &endPosition.X, path, pathSize, (float*)resultPath.Get(), nullptr, nullptr, &straightPathCount, resultPath.Capacity(), DT_STRAIGHTPATH_AREA_CROSSINGS);
        if (dtStatusFailed(findStraightPathStatus))
        {
            return false;
        }
        resultPath.Resize(straightPathCount);
    }

    return true;
}

bool Navigation::ProjectPoint(const Vector3& point, Vector3& result)
{
    ScopeLock lock(_navMesh->Locker);

    const auto query = _navMesh->GetNavMeshQuery();
    if (!query || !_navMesh->GetNavMesh())
    {
        return false;
    }

    dtQueryFilter filter;
    Vector3 extent(DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL, DEFAULT_NAV_QUERY_EXTENT_VERTICAL, DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL);

    dtPolyRef startPoly = 0;
    query->findNearestPoly(&point.X, &extent.X, &filter, &startPoly, &result.X);
    if (!startPoly)
    {
        return false;
    }

    return true;
}

bool Navigation::RayCast(const Vector3& startPosition, const Vector3& endPosition, NavMeshHit& hitInfo)
{
    ScopeLock lock(_navMesh->Locker);

    const auto query = _navMesh->GetNavMeshQuery();
    if (!query || !_navMesh->GetNavMesh())
    {
        return false;
    }

    dtQueryFilter filter;
    Vector3 extent(DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL, DEFAULT_NAV_QUERY_EXTENT_VERTICAL, DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL);

    dtPolyRef startPoly = 0;
    query->findNearestPoly(&startPosition.X, &extent.X, &filter, &startPoly, nullptr);
    if (!startPoly)
    {
        return false;
    }

    dtRaycastHit hit;
    hit.path = nullptr;
    hit.maxPath = 0;
    const bool result = dtStatusSucceed(query->raycast(startPoly, &startPosition.X, &endPosition.X, &filter, 0, &hit));
    if (hit.t >= MAX_float)
    {
        hitInfo.Position = endPosition;
        hitInfo.Distance = 0;
    }
    else
    {
        hitInfo.Position = startPosition + (endPosition - startPosition) * hit.t;
        hitInfo.Distance = hit.t;
    }
    hitInfo.Normal = *(Vector3*)&hit.hitNormal;

    return result;
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

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void DrawPoly(const dtMeshTile& tile, const dtPoly& poly)
{
    const unsigned int ip = (unsigned int)(&poly - tile.polys);
    const dtPolyDetail& pd = tile.detailMeshes[ip];
    const float DrawOffsetY = 20.0f;

    for (int i = 0; i < pd.triCount; i++)
    {
        Vector3 v[3];
        const unsigned char* t = &tile.detailTris[(pd.triBase + i) * 4];

        for (int k = 0; k < 3; k++)
        {
            if (t[k] < poly.vertCount)
            {
                v[k] = *(Vector3*)&tile.verts[poly.verts[t[k]] * 3];
            }
            else
            {
                v[k] = *(Vector3*)&tile.detailVerts[(pd.vertBase + t[k] - poly.vertCount) * 3];
            }
        }

        v[0].Y += DrawOffsetY;
        v[1].Y += DrawOffsetY;
        v[2].Y += DrawOffsetY;

        DEBUG_DRAW_TRIANGLE(v[0], v[1], v[2], Color::Green * 0.5f, 0, true);
    }

    for (int k = 0; k < pd.triCount; k++)
    {
        const unsigned char* t = &tile.detailTris[(pd.triBase + k) * 4];
        Vector3 v[3];

        for (int m = 0; m < 3; m++)
        {
            if (t[m] < poly.vertCount)
                v[m] = *(Vector3*)&tile.verts[poly.verts[t[m]] * 3];
            else
                v[m] = *(Vector3*)&tile.detailVerts[(pd.vertBase + (t[m] - poly.vertCount)) * 3];
        }

        v[0].Y += DrawOffsetY;
        v[1].Y += DrawOffsetY;
        v[2].Y += DrawOffsetY;

        for (int m = 0, n = 2; m < 3; n = m++)
        {
            // Skip inner detail edges
            if (((t[3] >> (n * 2)) & 0x3) == 0)
                continue;

            DEBUG_DRAW_LINE(v[n], v[m], Color::YellowGreen, 0, true);
        }
    }
}

void Navigation::DrawNavMesh()
{
    if (!_navMesh)
        return;

    ScopeLock lock(_navMesh->Locker);

    const dtNavMesh* dtNavMesh = _navMesh->GetNavMesh();
    const int tilesCount = dtNavMesh ? dtNavMesh->getMaxTiles() : 0;
    if (tilesCount == 0)
        return;

    for (int tileIndex = 0; tileIndex < tilesCount; tileIndex++)
    {
        const dtMeshTile* tile = dtNavMesh->getTile(tileIndex);
        if (!tile->header)
            continue;

        //DebugDraw::DrawWireBox(*(BoundingBox*)&tile->header->bmin[0], Color::CadetBlue);

        for (int i = 0; i < tile->header->polyCount; i++)
        {
            const dtPoly* poly = &tile->polys[i];
            if (poly->getType() != DT_POLYTYPE_GROUND)
                continue;

            DrawPoly(*tile, *poly);
        }
    }
}

#endif
