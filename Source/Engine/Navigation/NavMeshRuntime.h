// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Platform/CriticalSection.h"
#include "NavMeshData.h"

class NavigationScene;
class dtNavMesh;
class dtNavMeshQuery;

class NavMeshTile
{
public:

    int32 X;
    int32 Y;
    int32 Layer;
    NavigationScene* Scene;
    BytesContainer Data;
};

class NavMeshRuntime
{
public:

    static NavMeshRuntime* Get();

private:

    dtNavMesh* _navMesh;
    dtNavMeshQuery* _navMeshQuery;
    float _tileSize;
    Array<NavMeshTile> _tiles;

public:

    NavMeshRuntime();
    ~NavMeshRuntime();

public:

    /// <summary>
    /// The object locker.
    /// </summary>
    CriticalSection Locker;

    /// <summary>
    /// Gets the size of the tile (in world-units). Returns zero if not initialized yet.
    /// </summary>
    FORCE_INLINE float GetTileSize() const
    {
        return _tileSize;
    }

    dtNavMesh* GetNavMesh() const
    {
        return _navMesh;
    }

    dtNavMeshQuery* GetNavMeshQuery() const
    {
        return _navMeshQuery;
    }

    int32 GetTilesCapacity() const;

public:

    /// <summary>
    /// Sets the size of the tile (if not assigned). Disposes the mesh if added tiles have different size.
    /// </summary>
    /// <param name="tileSize">The size of the tile.</param>
    void SetTileSize(float tileSize);

    /// <summary>
    /// Ensures the navmesh capacity for adding new tiles. Performs resizing if needed.
    /// </summary>
    /// <param name="tilesToAddCount">The new tiles amount.</param>
    void EnsureCapacity(int32 tilesToAddCount);

    /// <summary>
    /// Adds the tiles from the given scene to the runtime navmesh.
    /// </summary>
    /// <param name="scene">The navigation scene.</param>
    void AddTiles(NavigationScene* scene);

    /// <summary>
    /// Adds the tile from the given scene to the runtime navmesh.
    /// </summary>
    /// <param name="scene">The navigation scene.</param>
    /// <param name="tileData">The tile data.</param>
    void AddTile(NavigationScene* scene, NavMeshTileData& tileData);

    /// <summary>
    /// Removes all the tiles from the navmesh that has been added from the given navigation scene.
    /// </summary>
    /// <param name="scene">The scene.</param>
    void RemoveTiles(NavigationScene* scene);

    /// <summary>
    /// Removes the tile from the navmesh.
    /// </summary>
    /// <param name="x">The tile X coordinate.</param>
    /// <param name="y">The tile Y coordinate.</param>
    /// <param name="layer">The tile layer.</param>
    void RemoveTile(int32 x, int32 y, int32 layer);

    /// <summary>
    /// Removes all the tiles that custom prediction callback marks.
    /// </summary>
    /// <param name="prediction">The prediction callback, returns true for tiles to remove and false for tiles to preserve.</param>
    /// <param name="userData">The user data passed to the callback method.</param>
    void RemoveTiles(bool (*prediction)(const NavMeshRuntime* navMesh, const NavMeshTile& tile, void* customData), void* userData);

    /// <summary>
    /// Releases the navmesh.
    /// </summary>
    void Dispose();

private:

    void AddTileInternal(NavigationScene* scene, NavMeshTileData& tileData);
};
