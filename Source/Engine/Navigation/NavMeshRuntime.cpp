// Copyright (c) Wojciech Figat. All rights reserved.

#include "NavMeshRuntime.h"
#include "NavigationSettings.h"
#include "NavMesh.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Random.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Threading/Threading.h"
#include <ThirdParty/recastnavigation/DetourNavMesh.h>
#include <ThirdParty/recastnavigation/DetourNavMeshQuery.h>
#include <ThirdParty/recastnavigation/RecastAlloc.h>

#define MAX_NODES 2048
#define USE_DATA_LINK 0
#define USE_NAV_MESH_ALLOC 0

#if USE_NAV_MESH_ALLOC
#define GET_NAV_TILE_DATA(tile) \
    const int32 dataSize = (tile).Data.Length(); \
    const auto flags = DT_TILE_FREE_DATA; \
    const auto data = (byte*)dtAlloc(dataSize, DT_ALLOC_PERM); \
    Platform::MemoryCopy(data, (tile).Data.Get(), dataSize)
#else
#define GET_NAV_TILE_DATA(tile) \
    const int32 dataSize = (tile).Data.Length(); \
    const auto flags = 0; \
    const auto data = (tile).Data.Get()
#endif

namespace
{
    FORCE_INLINE void InitFilter(dtQueryFilter& filter)
    {
        Platform::MemoryCopy(filter.m_areaCost, NavMeshRuntime::NavAreasCosts, sizeof(NavMeshRuntime::NavAreasCosts));
        static_assert(sizeof(dtQueryFilter::m_areaCost) == sizeof(NavMeshRuntime::NavAreasCosts), "Invalid navmesh area cost list.");
    }
}

NavMeshRuntime::NavMeshRuntime(const NavMeshProperties& properties)
    : ScriptingObject(SpawnParams(Guid::New(), NavMeshRuntime::TypeInitializer))
    , Properties(properties)
{
    _navMesh = nullptr;
    _navMeshQuery = dtAllocNavMeshQuery();
    _tileSize = 0;
}

NavMeshRuntime::~NavMeshRuntime()
{
    Dispose();
    dtFreeNavMeshQuery(_navMeshQuery);
}

int32 NavMeshRuntime::GetTilesCapacity() const
{
    return _navMesh ? _navMesh->getMaxTiles() : 0;
}

bool NavMeshRuntime::FindDistanceToWall(const Vector3& startPosition, NavMeshHit& hitInfo, float maxDistance) const
{
    ScopeLock lock(Locker);
    const auto query = GetNavMeshQuery();
    if (!query || !_navMesh)
        return false;

    dtQueryFilter filter;
    InitFilter(filter);
    Float3 extent = Properties.DefaultQueryExtent;

    Float3 startPositionNavMesh;
    Float3::Transform(startPosition, Properties.Rotation, startPositionNavMesh);

    dtPolyRef startPoly = 0;
    if (!dtStatusSucceed(query->findNearestPoly(&startPositionNavMesh.X, &extent.X, &filter, &startPoly, nullptr)))
        return false;

    Float3 hitPosition, hitNormal;
    if (!dtStatusSucceed(query->findDistanceToWall(startPoly, &startPositionNavMesh.X, maxDistance, &filter, &hitInfo.Distance, &hitPosition.X, &hitNormal.X)))
        return false;

    Quaternion invRotation;
    Quaternion::Invert(Properties.Rotation, invRotation);
    Vector3::Transform(hitPosition, invRotation, hitInfo.Position);
    Vector3::Transform(hitNormal, invRotation, hitInfo.Normal);

    return true;
}

bool NavMeshRuntime::FindPath(const Vector3& startPosition, const Vector3& endPosition, Array<Vector3, HeapAllocation>& resultPath, NavMeshPathFlags& resultFlags) const
{
    resultPath.Clear();
    resultFlags = NavMeshPathFlags::None;
    ScopeLock lock(Locker);
    const auto query = GetNavMeshQuery();
    if (!query || !_navMesh)
        return false;

    dtQueryFilter filter;
    InitFilter(filter);
    Float3 extent = Properties.DefaultQueryExtent;

    Float3 startPositionNavMesh, endPositionNavMesh;
    Float3::Transform(startPosition, Properties.Rotation, startPositionNavMesh);
    Float3::Transform(endPosition, Properties.Rotation, endPositionNavMesh);

    dtPolyRef startPoly = 0;
    if (!dtStatusSucceed(query->findNearestPoly(&startPositionNavMesh.X, &extent.X, &filter, &startPoly, nullptr)))
        return false;
    dtPolyRef endPoly = 0;
    if (!dtStatusSucceed(query->findNearestPoly(&endPositionNavMesh.X, &extent.X, &filter, &endPoly, nullptr)))
        return false;

    dtPolyRef path[NAV_MESH_PATH_MAX_SIZE];
    int32 pathSize;
    const auto findPathStatus = query->findPath(startPoly, endPoly, &startPositionNavMesh.X, &endPositionNavMesh.X, &filter, path, &pathSize, NAV_MESH_PATH_MAX_SIZE);
    if (dtStatusFailed(findPathStatus))
    {
        return false;
    }

    Quaternion invRotation;
    Quaternion::Invert(Properties.Rotation, invRotation);

    if (pathSize == 1 && dtStatusDetail(findPathStatus, DT_PARTIAL_RESULT))
    {
        resultFlags |= NavMeshPathFlags::PartialPath;
        // TODO: skip adding 2nd end point if it's not reachable (use navmesh raycast check? or physics check? or local Z distance check?)
        resultPath.Resize(2);
        resultPath[0] = startPosition;
        query->closestPointOnPolyBoundary(startPoly, &endPositionNavMesh.X, &endPositionNavMesh.X);
        resultPath[1] = endPositionNavMesh;
        Vector3::Transform(resultPath[1], invRotation, resultPath[1]);
    }
    else
    {
        int pathPointsCount = 0;
        Float3 pathPoints[NAV_MESH_PATH_MAX_SIZE];
        const auto findStraightPathStatus = query->findStraightPath(&startPositionNavMesh.X, &endPositionNavMesh.X, path, pathSize, (float*)&pathPoints, nullptr, nullptr, &pathPointsCount, NAV_MESH_PATH_MAX_SIZE, DT_STRAIGHTPATH_AREA_CROSSINGS);
        if (dtStatusFailed(findStraightPathStatus))
        {
            return false;
        }
        resultPath.Resize(pathPointsCount);
        for (int32 i = 0; i < pathPointsCount; i++)
        {
            Vector3::Transform(pathPoints[i], invRotation, resultPath[i]);
        }
    }

    return true;
}

bool NavMeshRuntime::TestPath(const Vector3& startPosition, const Vector3& endPosition) const
{
    ScopeLock lock(Locker);
    const auto query = GetNavMeshQuery();
    if (!query || !_navMesh)
        return false;

    dtQueryFilter filter;
    InitFilter(filter);
    Float3 extent = Properties.DefaultQueryExtent;

    Float3 startPositionNavMesh, endPositionNavMesh;
    Float3::Transform(startPosition, Properties.Rotation, startPositionNavMesh);
    Float3::Transform(endPosition, Properties.Rotation, endPositionNavMesh);

    dtPolyRef startPoly = 0;
    if (!dtStatusSucceed(query->findNearestPoly(&startPositionNavMesh.X, &extent.X, &filter, &startPoly, nullptr)))
        return false;
    dtPolyRef endPoly = 0;
    if (!dtStatusSucceed(query->findNearestPoly(&endPositionNavMesh.X, &extent.X, &filter, &endPoly, nullptr)))
        return false;

    dtPolyRef path[NAV_MESH_PATH_MAX_SIZE];
    int32 pathSize;
    const auto findPathStatus = query->findPath(startPoly, endPoly, &startPositionNavMesh.X, &endPositionNavMesh.X, &filter, path, &pathSize, NAV_MESH_PATH_MAX_SIZE);
    if (dtStatusFailed(findPathStatus))
        return false;
    if (dtStatusDetail(findPathStatus, DT_PARTIAL_RESULT))
        return false;

    return true;
}

bool NavMeshRuntime::FindClosestPoint(const Vector3& point, Vector3& result) const
{
    ScopeLock lock(Locker);
    const auto query = GetNavMeshQuery();
    if (!query || !_navMesh)
        return false;

    dtQueryFilter filter;
    InitFilter(filter);
    Float3 extent = Properties.DefaultQueryExtent;

    Float3 pointNavMesh;
    Float3::Transform(point, Properties.Rotation, pointNavMesh);

    dtPolyRef startPoly = 0;
    Float3 nearestPt;
    if (!dtStatusSucceed(query->findNearestPoly(&pointNavMesh.X, &extent.X, &filter, &startPoly, &nearestPt.X)))
        return false;

    Quaternion invRotation;
    Quaternion::Invert(Properties.Rotation, invRotation);
    Vector3::Transform(nearestPt, invRotation, result);

    return true;
}

bool NavMeshRuntime::FindRandomPoint(Vector3& result) const
{
    ScopeLock lock(Locker);
    const auto query = GetNavMeshQuery();
    if (!query || !_navMesh)
        return false;

    dtQueryFilter filter;
    InitFilter(filter);

    dtPolyRef randomPoly = 0;
    Float3 randomPt;
    if (!dtStatusSucceed(query->findRandomPoint(&filter, Random::Rand, &randomPoly, &randomPt.X)))
        return false;

    Quaternion invRotation;
    Quaternion::Invert(Properties.Rotation, invRotation);
    Vector3::Transform(randomPt, invRotation, result);

    return true;
}

bool NavMeshRuntime::FindRandomPointAroundCircle(const Vector3& center, float radius, Vector3& result) const
{
    ScopeLock lock(Locker);
    const auto query = GetNavMeshQuery();
    if (!query || !_navMesh)
        return false;

    dtQueryFilter filter;
    InitFilter(filter);
    Float3 extent(radius);

    Float3 centerNavMesh;
    Float3::Transform(center, Properties.Rotation, centerNavMesh);

    dtPolyRef centerPoly = 0;
    if (!dtStatusSucceed(query->findNearestPoly(&centerNavMesh.X, &extent.X, &filter, &centerPoly, nullptr)))
        return false;

    dtPolyRef randomPoly = 0;
    Float3 randomPt;
    if (!dtStatusSucceed(query->findRandomPointAroundCircle(centerPoly, &centerNavMesh.X, radius, &filter, Random::Rand, &randomPoly, &randomPt.X)))
        return false;

    Quaternion invRotation;
    Quaternion::Invert(Properties.Rotation, invRotation);
    Vector3::Transform(randomPt, invRotation, result);

    return true;
}

bool NavMeshRuntime::RayCast(const Vector3& startPosition, const Vector3& endPosition, NavMeshHit& hitInfo) const
{
    ScopeLock lock(Locker);
    const auto query = GetNavMeshQuery();
    if (!query || !_navMesh)
        return false;

    dtQueryFilter filter;
    InitFilter(filter);
    Float3 extent = Properties.DefaultQueryExtent;

    Float3 startPositionNavMesh, endPositionNavMesh;
    Float3::Transform(startPosition, Properties.Rotation, startPositionNavMesh);
    Float3::Transform(endPosition, Properties.Rotation, endPositionNavMesh);

    dtPolyRef startPoly = 0;
    if (!dtStatusSucceed(query->findNearestPoly(&startPositionNavMesh.X, &extent.X, &filter, &startPoly, nullptr)))
        return false;

    dtRaycastHit hit;
    hit.path = nullptr;
    hit.maxPath = 0;
    const bool result = dtStatusSucceed(query->raycast(startPoly, &startPositionNavMesh.X, &endPositionNavMesh.X, &filter, 0, &hit));
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
    hitInfo.Normal = *(Float3*)&hit.hitNormal;

    return result;
}

void NavMeshRuntime::SetTileSize(float tileSize)
{
    ScopeLock lock(Locker);

    // Skip if the same or invalid
    if (_tileSize == tileSize || tileSize < 1)
        return;

    // Dispose the existing mesh (its invalid)
    if (_navMesh)
    {
        Dispose();
    }

    _tileSize = tileSize;
}

void NavMeshRuntime::EnsureCapacity(int32 tilesToAddCount)
{
    ScopeLock lock(Locker);
    const int32 newTilesCount = _tiles.Count() + tilesToAddCount;
    const int32 capacity = GetTilesCapacity();
    if (newTilesCount <= capacity)
        return;
    PROFILE_CPU_NAMED("NavMeshRuntime.EnsureCapacity");

    // Navmesh tiles capacity growing rule
    int32 newCapacity = capacity ? capacity : 32;
    while (newCapacity < newTilesCount)
        newCapacity = Math::RoundUpToPowerOf2(newCapacity + 1);

    LOG(Info, "Resizing navmesh {2} from {0} to {1} tiles capacity", capacity, newCapacity, Properties.Name);

    // Ensure to have size assigned
    ASSERT(_tileSize != 0);

    // Prepare parameters
    dtNavMeshParams params;
    params.orig[0] = 0.0f;
    params.orig[1] = 0.0f;
    params.orig[2] = 0.0f;
    params.tileWidth = _tileSize;
    params.tileHeight = _tileSize;
    params.maxTiles = newCapacity;
    const int32 tilesBits = (int32)Math::Log2((float)Math::RoundUpToPowerOf2(params.maxTiles));
    params.maxPolys = 1 << (22 - tilesBits);

    // Initialize nav mesh
    if (!_navMesh)
        _navMesh = dtAllocNavMesh();
    if (dtStatusFailed(_navMesh->init(&params)))
    {
        LOG(Error, "Navmesh {0} init failed", Properties.Name);
        return;
    }
    if (dtStatusFailed(_navMeshQuery->init(_navMesh, MAX_NODES)))
    {
        LOG(Error, "Navmesh query {0} init failed", Properties.Name);
    }

    // Prepare tiles container
    _tiles.EnsureCapacity(newCapacity);

    // Restore previous tiles
    for (auto& tile : _tiles)
    {
        GET_NAV_TILE_DATA(tile);
        const auto result = _navMesh->addTile(data, dataSize, flags, 0, nullptr);
        if (dtStatusFailed(result))
        {
            LOG(Warning, "Could not add tile ({2}x{3}, layer {4}) to navmesh {0} (error: {1})", Properties.Name, result & ~DT_FAILURE, tile.X, tile.Y, tile.Layer);
#if USE_NAV_MESH_ALLOC
            dtFree(data);
#endif
        }
    }
}

void NavMeshRuntime::AddTiles(NavMesh* navMesh)
{
    ASSERT(navMesh);
    if (navMesh->Data.Tiles.IsEmpty())
        return;
    auto& data = navMesh->Data;
    PROFILE_CPU_NAMED("NavMeshRuntime.AddTiles");
    ScopeLock lock(Locker);

    // Validate data (must match navmesh) or init navmesh to match the tiles options
    if (_navMesh)
    {
        if (Math::NotNearEqual(data.TileSize, _tileSize))
        {
            LOG(Warning, "Cannot add navigation scene tiles to the navmesh {2}. Navmesh tile size: {0}, input tiles size: {1}", _tileSize, data.TileSize, Properties.Name);
            return;
        }
    }
    else
    {
        _tileSize = data.TileSize;
    }

    // Ensure to have space for new tiles
    EnsureCapacity(data.Tiles.Count());

    // Add new tiles
    for (auto& tileData : data.Tiles)
    {
        AddTileInternal(navMesh, tileData);
    }
}

void NavMeshRuntime::AddTile(NavMesh* navMesh, NavMeshTileData& tileData)
{
    ASSERT(navMesh);
    auto& data = navMesh->Data;
    PROFILE_CPU_NAMED("NavMeshRuntime.AddTile");
    ScopeLock lock(Locker);

    // Validate data (must match navmesh) or init navmesh to match the tiles options
    if (_navMesh)
    {
        if (Math::NotNearEqual(data.TileSize, _tileSize))
        {
            LOG(Warning, "Cannot add navigation scene tile to the navmesh {2}. Navmesh tile size: {0}, input tile size: {1}", _tileSize, data.TileSize, Properties.Name);
            return;
        }
    }
    else
    {
        _tileSize = data.TileSize;
    }

    // Ensure to have space for new tile
    EnsureCapacity(1);

    // Add new tile
    AddTileInternal(navMesh, tileData);
}

bool IsTileFromScene(const NavMeshRuntime* navMesh, const NavMeshTile& tile, void* customData)
{
    return tile.NavMesh == (NavMesh*)customData;
}

void NavMeshRuntime::RemoveTiles(NavMesh* navMesh)
{
    RemoveTiles(IsTileFromScene, navMesh);
}

void NavMeshRuntime::RemoveTile(int32 x, int32 y, int32 layer)
{
    ScopeLock lock(Locker);
    if (!_navMesh)
        return;
    PROFILE_CPU_NAMED("NavMeshRuntime.RemoveTile");

    const auto tileRef = _navMesh->getTileRefAt(x, y, layer);
    if (tileRef == 0)
        return;

    if (dtStatusFailed(_navMesh->removeTile(tileRef, nullptr, nullptr)))
    {
        LOG(Warning, "Failed to remove tile ({1}x{2}, layer {3}) from navmesh {0}", Properties.Name, x, y, layer);
    }

    for (int32 i = 0; i < _tiles.Count(); i++)
    {
        auto& tile = _tiles[i];
        if (tile.X == x && tile.Y == y && tile.Layer == layer)
        {
            _tiles.RemoveAt(i);
            break;
        }
    }
}

void NavMeshRuntime::RemoveTiles(bool (*prediction)(const NavMeshRuntime* navMesh, const NavMeshTile& tile, void* customData), void* userData)
{
    ScopeLock lock(Locker);
    ASSERT(prediction);
    if (!_navMesh)
        return;
    PROFILE_CPU_NAMED("NavMeshRuntime.RemoveTiles");

    for (int32 i = 0; i < _tiles.Count(); i++)
    {
        auto& tile = _tiles[i];
        if (prediction(this, tile, userData))
        {
            const auto tileRef = _navMesh->getTileRefAt(tile.X, tile.Y, tile.Layer);
            if (tileRef == 0)
            {
                LOG(Warning, "Missing navmesh {3} tile at {0}x{1}, layer: {2}", tile.X, tile.Y, tile.Layer, Properties.Name);
            }
            else
            {
                if (dtStatusFailed(_navMesh->removeTile(tileRef, nullptr, nullptr)))
                {
                    LOG(Warning, "Failed to remove tile ({1}x{2}, layer {3}) from navmesh {0}", Properties.Name, tile.X, tile.Y, tile.Layer);
                }
            }

            _tiles.RemoveAt(i--);
        }
    }
}

#if COMPILE_WITH_DEBUG_DRAW

#include "Engine/Debug/DebugDraw.h"
#include "Engine/Core/Math/Matrix.h"

void DrawPoly(NavMeshRuntime* navMesh, const Matrix& navMeshToWorld, const dtMeshTile& tile, const dtPoly& poly)
{
    const unsigned int ip = (unsigned int)(&poly - tile.polys);
    const dtPolyDetail& pd = tile.detailMeshes[ip];
    const Color& areaColor = NavMeshRuntime::NavAreasColors[poly.getArea()];
    const Color color = Color::Lerp(navMesh->Properties.Color, areaColor, areaColor.A);
    const float drawOffsetY = 10.0f + ((float)GetHash(color) / (float)MAX_uint32) * 10.0f; // Apply some offset to prevent Z-fighting for different navmeshes
    const Color fillColor = color * 0.5f;
    const Color edgesColor = Color::FromHSV(color.ToHSV() + Float3(20.0f, 0, -0.1f), color.A);

    for (int i = 0; i < pd.triCount; i++)
    {
        Float3 v[3];
        const unsigned char* t = &tile.detailTris[(pd.triBase + i) * 4];

        for (int k = 0; k < 3; k++)
        {
            if (t[k] < poly.vertCount)
            {
                v[k] = *(Float3*)&tile.verts[poly.verts[t[k]] * 3];
            }
            else
            {
                v[k] = *(Float3*)&tile.detailVerts[(pd.vertBase + t[k] - poly.vertCount) * 3];
            }
        }

        v[0].Y += drawOffsetY;
        v[1].Y += drawOffsetY;
        v[2].Y += drawOffsetY;

        Float3::Transform(v[0], navMeshToWorld, v[0]);
        Float3::Transform(v[1], navMeshToWorld, v[1]);
        Float3::Transform(v[2], navMeshToWorld, v[2]);

        DEBUG_DRAW_TRIANGLE(v[0], v[1], v[2], fillColor, 0, true);
    }

    for (int k = 0; k < pd.triCount; k++)
    {
        const unsigned char* t = &tile.detailTris[(pd.triBase + k) * 4];
        Float3 v[3];

        for (int m = 0; m < 3; m++)
        {
            if (t[m] < poly.vertCount)
                v[m] = *(Float3*)&tile.verts[poly.verts[t[m]] * 3];
            else
                v[m] = *(Float3*)&tile.detailVerts[(pd.vertBase + (t[m] - poly.vertCount)) * 3];
        }

        v[0].Y += drawOffsetY;
        v[1].Y += drawOffsetY;
        v[2].Y += drawOffsetY;

        Float3::Transform(v[0], navMeshToWorld, v[0]);
        Float3::Transform(v[1], navMeshToWorld, v[1]);
        Float3::Transform(v[2], navMeshToWorld, v[2]);

        for (int m = 0, n = 2; m < 3; n = m++)
        {
            // Skip inner detail edges
            if (((t[3] >> (n * 2)) & 0x3) == 0)
                continue;

            DEBUG_DRAW_LINE(v[n], v[m], edgesColor, 0, true);
        }
    }
}

void NavMeshRuntime::DebugDraw()
{
    ScopeLock lock(Locker);
    const dtNavMesh* dtNavMesh = GetNavMesh();
    const int tilesCount = dtNavMesh ? dtNavMesh->getMaxTiles() : 0;
    if (tilesCount == 0)
        return;
    Matrix worldToNavMesh, navMeshToWorld;
    Matrix::RotationQuaternion(Properties.Rotation, worldToNavMesh);
    Matrix::Invert(worldToNavMesh, navMeshToWorld);

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

            DrawPoly(this, navMeshToWorld, *tile, *poly);
        }
    }
}

#endif

void NavMeshRuntime::Dispose()
{
    if (_navMesh)
    {
        dtFreeNavMesh(_navMesh);
        _navMesh = nullptr;
    }
    _tiles.Resize(0);
}

void NavMeshRuntime::AddTileInternal(NavMesh* navMesh, NavMeshTileData& tileData)
{
    // Check if that tile has been added to navmesh
    NavMeshTile* tile = nullptr;
    const auto tileRef = _navMesh->getTileRefAt(tileData.PosX, tileData.PosY, tileData.Layer);
    if (tileRef)
    {
        // Remove any existing tile at that location
        if (dtStatusFailed(_navMesh->removeTile(tileRef, nullptr, nullptr)))
        {
            LOG(Warning, "Failed to remove tile from navmesh {0}", Properties.Name);
        }

        // Reuse tile data container
        for (int32 i = 0; i < _tiles.Count(); i++)
        {
            auto& e = _tiles[i];
            if (e.X == tileData.PosX && e.Y == tileData.PosY && e.Layer == tileData.Layer)
            {
                tile = &e;
                break;
            }
        }
    }
    if (!tile)
    {
        // Add tile
        tile = &_tiles.AddOne();
    }

    // Copy tile properties
    tile->NavMesh = navMesh;
    tile->X = tileData.PosX;
    tile->Y = tileData.PosY;
    tile->Layer = tileData.Layer;
#if USE_DATA_LINK
	tile->Data.Link(tileData.Data);
#else
    tile->Data.Copy(tileData.Data);
#endif

    // Add tile to navmesh
    GET_NAV_TILE_DATA(*tile);
    const auto result = _navMesh->addTile(data, dataSize, flags, 0, nullptr);
    if (dtStatusFailed(result))
    {
        LOG(Warning, "Could not add tile ({2}x{3}, layer {4}) to navmesh {0} (error: {1})", Properties.Name, result & ~DT_FAILURE, tileData.PosX, tileData.PosY, tileData.Layer);
#if USE_NAV_MESH_ALLOC
        dtFree(data);
#endif
    }
}
