// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "NavMeshRuntime.h"
#include "NavigationScene.h"
#include "Engine/Core/Log.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Threading/Threading.h"
#include <ThirdParty/recastnavigation/DetourNavMesh.h>
#include <ThirdParty/recastnavigation/DetourNavMeshQuery.h>

#define MAX_NODES 2048
#define USE_DATA_LINK 0
#define USE_NAV_MESH_ALLOC 1

NavMeshRuntime::NavMeshRuntime()
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

    LOG(Info, "Resizing navmesh from {0} to {1} tiles capacity", capacity, newCapacity);

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
        LOG(Fatal, "Failed to initialize nav mesh.");
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
        LOG(Fatal, "Navmesh init failed.");
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
            LOG(Warning, "Could not add tile to navmesh.");
        }
    }
}

void NavMeshRuntime::AddTiles(NavigationScene* scene)
{
    // Skip if no data
    ASSERT(scene);
    if (scene->Data.Tiles.IsEmpty())
        return;
    auto& data = scene->Data;

    PROFILE_CPU_NAMED("NavMeshRuntime.AddTiles");

    ScopeLock lock(Locker);

    // Validate data (must match navmesh) or init navmesh to match the tiles options
    if (_navMesh)
    {
        if (Math::NotNearEqual(data.TileSize, _tileSize))
        {
            LOG(Warning, "Cannot add navigation scene tiles to the navmesh. Navmesh tile size: {0}, input tiles size: {1}", _tileSize, data.TileSize);
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
        AddTileInternal(scene, tileData);
    }
}

void NavMeshRuntime::AddTile(NavigationScene* scene, NavMeshTileData& tileData)
{
    ASSERT(scene);
    auto& data = scene->Data;

    PROFILE_CPU_NAMED("NavMeshRuntime.AddTile");

    ScopeLock lock(Locker);

    // Validate data (must match navmesh) or init navmesh to match the tiles options
    if (_navMesh)
    {
        if (Math::NotNearEqual(data.TileSize, _tileSize))
        {
            LOG(Warning, "Cannot add navigation scene tile to the navmesh. Navmesh tile size: {0}, input tile size: {1}", _tileSize, data.TileSize);
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
    AddTileInternal(scene, tileData);
}

bool IsTileFromScene(const NavMeshRuntime* navMesh, const NavMeshTile& tile, void* customData)
{
    return tile.Scene == (NavigationScene*)customData;
}

void NavMeshRuntime::RemoveTiles(NavigationScene* scene)
{
    RemoveTiles(IsTileFromScene, scene);
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
        LOG(Warning, "Failed to remove tile from navmesh.");
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
                LOG(Warning, "Missing navmesh tile at {0}x{1}, layer: {2}", tile.X, tile.Y, tile.Layer);
            }
            else
            {
                if (dtStatusFailed(_navMesh->removeTile(tileRef, nullptr, nullptr)))
                {
                    LOG(Warning, "Failed to remove tile from navmesh.");
                }
            }

            _tiles.RemoveAt(i--);
        }
    }
}

void NavMeshRuntime::Dispose()
{
    if (_navMesh)
    {
        dtFreeNavMesh(_navMesh);
        _navMesh = nullptr;
    }
    _tiles.Resize(0);
}

void NavMeshRuntime::AddTileInternal(NavigationScene* scene, NavMeshTileData& tileData)
{
    // Check if that tile has been added to navmesh
    NavMeshTile* tile = nullptr;
    const auto tileRef = _navMesh->getTileRefAt(tileData.PosX, tileData.PosY, tileData.Layer);
    if (tileRef)
    {
        // Remove any existing tile at that location
        _navMesh->removeTile(tileRef, nullptr, nullptr);

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
    else
    {
        // Add tile
        tile = &_tiles.AddOne();
    }
    ASSERT(tile);

    // Copy tile properties
    tile->Scene = scene;
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
        LOG(Warning, "Could not add tile to navmesh.");
    }
}
