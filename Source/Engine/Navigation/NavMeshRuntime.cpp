// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
#define USE_NAV_MESH_ALLOC 1
// TODO: try not using USE_NAV_MESH_ALLOC

#define DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL 50.0f
#define DEFAULT_NAV_QUERY_EXTENT_VERTICAL 250.0f

namespace
{
    FORCE_INLINE void InitFilter(dtQueryFilter& filter)
    {
        Platform::MemoryCopy(filter.m_areaCost, NavMeshRuntime::NavAreasCosts, sizeof(NavMeshRuntime::NavAreasCosts));
    }
}

NavMeshRuntime::NavMeshRuntime(const NavMeshProperties& properties)
    : Properties(properties)
{
    _navMesh = nullptr;
    _navMeshQuery = dtAllocNavMeshQuery();
    _tileSize = 0;
}

NavMeshRuntime::~NavMeshRuntime()
{
    dtFreeNavMesh(_navMesh);
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
    {
        return false;
    }

    dtQueryFilter filter;
    InitFilter(filter);
    Vector3 extent(DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL, DEFAULT_NAV_QUERY_EXTENT_VERTICAL, DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL);

    Vector3 startPositionNavMesh;
    Vector3::Transform(startPosition, Properties.Rotation, startPositionNavMesh);

    dtPolyRef startPoly = 0;
    query->findNearestPoly(&startPositionNavMesh.X, &extent.X, &filter, &startPoly, nullptr);
    if (!startPoly)
    {
        return false;
    }

    if (!dtStatusSucceed(query->findDistanceToWall(startPoly, &startPosition.X, maxDistance, &filter, &hitInfo.Distance, &hitInfo.Position.X, &hitInfo.Normal.X)))
    {
        return false;
    }

    Quaternion invRotation;
    Quaternion::Invert(Properties.Rotation, invRotation);
    Vector3::Transform(hitInfo.Position, invRotation, hitInfo.Position);
    Vector3::Transform(hitInfo.Normal, invRotation, hitInfo.Normal);

    return true;
}

bool NavMeshRuntime::FindPath(const Vector3& startPosition, const Vector3& endPosition, Array<Vector3, HeapAllocation>& resultPath) const
{
    resultPath.Clear();
    ScopeLock lock(Locker);

    const auto query = GetNavMeshQuery();
    if (!query || !_navMesh)
    {
        return false;
    }

    dtQueryFilter filter;
    InitFilter(filter);
    Vector3 extent(DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL, DEFAULT_NAV_QUERY_EXTENT_VERTICAL, DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL);

    Vector3 startPositionNavMesh, endPositionNavMesh;
    Vector3::Transform(startPosition, Properties.Rotation, startPositionNavMesh);
    Vector3::Transform(endPosition, Properties.Rotation, endPositionNavMesh);

    dtPolyRef startPoly = 0;
    query->findNearestPoly(&startPositionNavMesh.X, &extent.X, &filter, &startPoly, nullptr);
    if (!startPoly)
    {
        return false;
    }
    dtPolyRef endPoly = 0;
    query->findNearestPoly(&endPositionNavMesh.X, &extent.X, &filter, &endPoly, nullptr);
    if (!endPoly)
    {
        return false;
    }

    dtPolyRef path[NAV_MESH_PATH_MAX_SIZE];
    int32 pathSize;
    const auto findPathStatus = query->findPath(startPoly, endPoly, &startPositionNavMesh.X, &endPositionNavMesh.X, &filter, path, &pathSize, NAV_MESH_PATH_MAX_SIZE);
    if (dtStatusFailed(findPathStatus))
    {
        return false;
    }

    Quaternion invRotation;
    Quaternion::Invert(Properties.Rotation, invRotation);

    // Check for special case, where path has not been found, and starting polygon was the one closest to the target
    if (pathSize == 1 && dtStatusDetail(findPathStatus, DT_PARTIAL_RESULT))
    {
        // In this case we find a point on starting polygon, that's closest to destination and store it as path end
        resultPath.Resize(2);
        resultPath[0] = startPosition;
        resultPath[1] = startPositionNavMesh;
        query->closestPointOnPolyBoundary(startPoly, &endPositionNavMesh.X, &resultPath[1].X);
        Vector3::Transform(resultPath[1], invRotation, resultPath[1]);
    }
    else
    {
        int straightPathCount = 0;
        resultPath.EnsureCapacity(NAV_MESH_PATH_MAX_SIZE);
        const auto findStraightPathStatus = query->findStraightPath(&startPositionNavMesh.X, &endPositionNavMesh.X, path, pathSize, (float*)resultPath.Get(), nullptr, nullptr, &straightPathCount, resultPath.Capacity(), DT_STRAIGHTPATH_AREA_CROSSINGS);
        if (dtStatusFailed(findStraightPathStatus))
        {
            return false;
        }
        resultPath.Resize(straightPathCount);
        for (auto& pos : resultPath)
            Vector3::Transform(pos, invRotation, pos);
    }

    return true;
}

bool NavMeshRuntime::TestPath(const Vector3& startPosition, const Vector3& endPosition) const
{
    ScopeLock lock(Locker);

    const auto query = GetNavMeshQuery();
    if (!query || !_navMesh)
    {
        return false;
    }

    dtQueryFilter filter;
    InitFilter(filter);
    Vector3 extent(DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL, DEFAULT_NAV_QUERY_EXTENT_VERTICAL, DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL);

    Vector3 startPositionNavMesh, endPositionNavMesh;
    Vector3::Transform(startPosition, Properties.Rotation, startPositionNavMesh);
    Vector3::Transform(endPosition, Properties.Rotation, endPositionNavMesh);

    dtPolyRef startPoly = 0;
    query->findNearestPoly(&startPositionNavMesh.X, &extent.X, &filter, &startPoly, nullptr);
    if (!startPoly)
    {
        return false;
    }
    dtPolyRef endPoly = 0;
    query->findNearestPoly(&endPositionNavMesh.X, &extent.X, &filter, &endPoly, nullptr);
    if (!endPoly)
    {
        return false;
    }

    dtPolyRef path[NAV_MESH_PATH_MAX_SIZE];
    int32 pathSize;
    const auto findPathStatus = query->findPath(startPoly, endPoly, &startPositionNavMesh.X, &endPositionNavMesh.X, &filter, path, &pathSize, NAV_MESH_PATH_MAX_SIZE);
    if (dtStatusFailed(findPathStatus))
    {
        return false;
    }

    if (dtStatusDetail(findPathStatus, DT_PARTIAL_RESULT))
    {
        return false;
    }

    return true;
}

bool NavMeshRuntime::ProjectPoint(const Vector3& point, Vector3& result) const
{
    ScopeLock lock(Locker);

    const auto query = GetNavMeshQuery();
    if (!query || !_navMesh)
    {
        return false;
    }

    dtQueryFilter filter;
    InitFilter(filter);
    Vector3 extent(DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL, DEFAULT_NAV_QUERY_EXTENT_VERTICAL, DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL);

    Vector3 pointNavMesh;
    Vector3::Transform(point, Properties.Rotation, pointNavMesh);

    dtPolyRef startPoly = 0;
    query->findNearestPoly(&pointNavMesh.X, &extent.X, &filter, &startPoly, &result.X);
    if (!startPoly)
    {
        return false;
    }

    Quaternion invRotation;
    Quaternion::Invert(Properties.Rotation, invRotation);
    Vector3::Transform(result, invRotation, result);

    return true;
}

bool NavMeshRuntime::FindRandomPoint(Vector3& result) const
{
    ScopeLock lock(Locker);

    const auto query = GetNavMeshQuery();
    if (!query || !_navMesh)
    {
        return false;
    }

    dtQueryFilter filter;
    InitFilter(filter);

    dtPolyRef randomPoly = 0;
    query->findRandomPoint(&filter, Random::Rand, &randomPoly, &result.X);
    if (!randomPoly)
    {
        return false;
    }

    Quaternion invRotation;
    Quaternion::Invert(Properties.Rotation, invRotation);
    Vector3::Transform(result, invRotation, result);

    return true;
}

bool NavMeshRuntime::FindRandomPointAroundCircle(const Vector3& center, float radius, Vector3& result) const
{
    ScopeLock lock(Locker);

    const auto query = GetNavMeshQuery();
    if (!query || !_navMesh)
    {
        return false;
    }

    dtQueryFilter filter;
    InitFilter(filter);
    Vector3 extent(DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL, DEFAULT_NAV_QUERY_EXTENT_VERTICAL, DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL);

    Vector3 centerNavMesh;
    Vector3::Transform(center, Properties.Rotation, centerNavMesh);

    dtPolyRef centerPoly = 0;
    query->findNearestPoly(&centerNavMesh.X, &extent.X, &filter, &centerPoly, nullptr);
    if (!centerPoly)
    {
        return false;
    }

    dtPolyRef randomPoly = 0;
    query->findRandomPointAroundCircle(centerPoly, &centerNavMesh.X, radius, &filter, Random::Rand, &randomPoly, &result.X);
    if (!randomPoly)
    {
        return false;
    }

    Quaternion invRotation;
    Quaternion::Invert(Properties.Rotation, invRotation);
    Vector3::Transform(result, invRotation, result);

    return true;
}

bool NavMeshRuntime::RayCast(const Vector3& startPosition, const Vector3& endPosition, NavMeshHit& hitInfo) const
{
    ScopeLock lock(Locker);

    const auto query = GetNavMeshQuery();
    if (!query || !_navMesh)
    {
        return false;
    }

    dtQueryFilter filter;
    InitFilter(filter);
    Vector3 extent(DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL, DEFAULT_NAV_QUERY_EXTENT_VERTICAL, DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL);

    Vector3 startPositionNavMesh, endPositionNavMesh;
    Vector3::Transform(startPosition, Properties.Rotation, startPositionNavMesh);
    Vector3::Transform(endPosition, Properties.Rotation, endPositionNavMesh);

    dtPolyRef startPoly = 0;
    query->findNearestPoly(&startPositionNavMesh.X, &extent.X, &filter, &startPoly, nullptr);
    if (!startPoly)
    {
        return false;
    }

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
    hitInfo.Normal = *(Vector3*)&hit.hitNormal;

    return result;
}

void NavMeshRuntime::SetTileSize(float tileSize)
{
    ScopeLock lock(Locker);

    // Skip if the same or invalid
    if (Math::NearEqual(_tileSize, tileSize) || tileSize < 1)
        return;

    // Dispose the existing mesh (its invalid)
    if (_navMesh)
    {
        dtFreeNavMesh(_navMesh);
        _navMesh = nullptr;
        _tiles.Clear();
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
    int32 newCapacity = 0;
    if (capacity)
    {
        while (newCapacity < newTilesCount)
        {
            newCapacity = Math::RoundUpToPowerOf2(newCapacity);
        }
    }
    else
    {
        newCapacity = 32;
    }

    LOG(Info, "Resizing navmesh {2} from {0} to {1} tiles capacity", capacity, newCapacity, Properties.Name);

    // Ensure to have size assigned
    ASSERT(_tileSize != 0);

    // Fre previous data (if any)
    if (_navMesh)
    {
        dtFreeNavMesh(_navMesh);
    }

    // Allocate new navmesh
    _navMesh = dtAllocNavMesh();
    if (dtStatusFailed(_navMeshQuery->init(_navMesh, MAX_NODES)))
    {
        LOG(Error, "Failed to initialize navmesh {0}.", Properties.Name);
    }

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
    if (dtStatusFailed(_navMesh->init(&params)))
    {
        LOG(Error, "Navmesh {0} init failed.", Properties.Name);
        return;
    }

    // Prepare tiles container
    _tiles.EnsureCapacity(newCapacity);

    // Restore previous tiles
    for (auto& tile : _tiles)
    {
        const int32 dataSize = tile.Data.Length();
#if USE_NAV_MESH_ALLOC
        const auto flags = DT_TILE_FREE_DATA;
        const auto data = (byte*)dtAlloc(dataSize, DT_ALLOC_PERM);
        Platform::MemoryCopy(data, tile.Data.Get(), dataSize);
#else
		const auto flags = 0;
		const auto data = tile.Data.Get();
#endif
        if (dtStatusFailed(_navMesh->addTile(data, dataSize, flags, 0, nullptr)))
        {
            LOG(Warning, "Could not add tile to navmesh {0}.", Properties.Name);
        }
    }
}

void NavMeshRuntime::AddTiles(NavMesh* navMesh)
{
    // Skip if no data
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

    // Skip if no data
    if (!_navMesh)
        return;

    PROFILE_CPU_NAMED("NavMeshRuntime.RemoveTile");

    const auto tileRef = _navMesh->getTileRefAt(x, y, layer);
    if (tileRef == 0)
    {
        return;
    }

    if (dtStatusFailed(_navMesh->removeTile(tileRef, nullptr, nullptr)))
    {
        LOG(Warning, "Failed to remove tile from navmesh {0}.", Properties.Name);
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

void NavMeshRuntime::RemoveTiles(bool (* prediction)(const NavMeshRuntime* navMesh, const NavMeshTile& tile, void* customData), void* userData)
{
    ScopeLock lock(Locker);

    // Skip if no data
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
                    LOG(Warning, "Failed to remove tile from navmesh {0}.", Properties.Name);
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
    const Color edgesColor = Color::FromHSV(color.ToHSV() + Vector3(20.0f, 0, -0.1f), color.A);

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

        v[0].Y += drawOffsetY;
        v[1].Y += drawOffsetY;
        v[2].Y += drawOffsetY;

        Vector3::Transform(v[0], navMeshToWorld, v[0]);
        Vector3::Transform(v[1], navMeshToWorld, v[1]);
        Vector3::Transform(v[2], navMeshToWorld, v[2]);

        DEBUG_DRAW_TRIANGLE(v[0], v[1], v[2], fillColor, 0, true);
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

        v[0].Y += drawOffsetY;
        v[1].Y += drawOffsetY;
        v[2].Y += drawOffsetY;

        Vector3::Transform(v[0], navMeshToWorld, v[0]);
        Vector3::Transform(v[1], navMeshToWorld, v[1]);
        Vector3::Transform(v[2], navMeshToWorld, v[2]);

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
            LOG(Warning, "Failed to remove tile from navmesh {0}.", Properties.Name);
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
    const int32 dataSize = tile->Data.Length();
#if USE_NAV_MESH_ALLOC
    const auto flags = DT_TILE_FREE_DATA;
    const auto data = (byte*)dtAlloc(dataSize, DT_ALLOC_PERM);
    Platform::MemoryCopy(data, tile->Data.Get(), dataSize);
#else
	const auto flags = 0;
	const auto data = tile->Data.Get();
#endif
    if (dtStatusFailed(_navMesh->addTile(data, dataSize, flags, 0, nullptr)))
    {
        LOG(Warning, "Could not add tile to navmesh {0}.", Properties.Name);
    }
}
