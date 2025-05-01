// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "NavMeshData.h"
#include "NavigationTypes.h"

class dtNavMesh;
class dtNavMeshQuery;
class NavMesh;

/// <summary>
/// The navigation mesh tile data.
/// </summary>
class FLAXENGINE_API NavMeshTile
{
public:
    int32 X;
    int32 Y;
    int32 Layer;
    NavMesh* NavMesh;
    BytesContainer Data;
};

/// <summary>
/// The navigation mesh path flags.
/// </summary>
enum class NavMeshPathFlags
{
    // Nothing.
    None = 0,
    // Path is only partially generated, goal is unreachable so path represents the best guess.
    PartialPath = 1,
};

DECLARE_ENUM_OPERATORS(NavMeshPathFlags);

/// <summary>
/// The navigation mesh runtime object that builds the navmesh from all loaded scenes.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API NavMeshRuntime : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(NavMeshRuntime);
public:
    // Gets the first valid navigation mesh runtime. Return null if none created.
    API_FUNCTION() static NavMeshRuntime* Get();

    // Gets the navigation mesh runtime for a given navmesh name. Return null if missing.
    API_FUNCTION() static NavMeshRuntime* Get(const StringView& navMeshName);

    // Gets the navigation mesh runtime for a given agent properties trying to pick the best matching navmesh.
    API_FUNCTION() static NavMeshRuntime* Get(API_PARAM(Ref) const NavAgentProperties& agentProperties);

    // Gets the navigation mesh runtime for a given navmesh properties.
    API_FUNCTION() static NavMeshRuntime* Get(API_PARAM(Ref) const NavMeshProperties& navMeshProperties, bool createIfMissing = false);

    // The lookup table that maps areaId of the navmesh to the current properties (applied by the NavigationSettings). Cached to improve runtime performance.
    static float NavAreasCosts[64];
#if COMPILE_WITH_DEBUG_DRAW
    static Color NavAreasColors[64];
#endif

private:
    dtNavMesh* _navMesh;
    dtNavMeshQuery* _navMeshQuery;
    float _tileSize;
    Array<NavMeshTile> _tiles;

public:
    NavMeshRuntime(const NavMeshProperties& properties);
    ~NavMeshRuntime();

public:
    /// <summary>
    /// The object locker.
    /// </summary>
    CriticalSection Locker;

    /// <summary>
    /// The navigation mesh properties.
    /// </summary>
    API_FIELD(ReadOnly) NavMeshProperties Properties;

    /// <summary>
    /// Gets the size of the tile (in world-units). Returns zero if not initialized yet.
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetTileSize() const
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
    /// Finds the distance from the specified start position to the nearest polygon wall.
    /// </summary>
    /// <param name="startPosition">The start position.</param>
    /// <param name="hitInfo">The result hit information. Valid only when query succeed.</param>
    /// <param name="maxDistance">The maximum distance to search for wall (search radius).</param>
    /// <returns>True if ray hits an matching object, otherwise false.</returns>
    API_FUNCTION() bool FindDistanceToWall(const Vector3& startPosition, NavMeshHit& hitInfo, float maxDistance = MAX_float) const;

    /// <summary>
    /// Finds the path between the two positions presented as a list of waypoints stored in the corners array.
    /// </summary>
    /// <param name="startPosition">The start position.</param>
    /// <param name="endPosition">The end position.</param>
    /// <param name="resultPath">The result path.</param>
    /// <returns>True if found valid path between given two points (it may be partial), otherwise false if failed.</returns>
    API_FUNCTION() bool FindPath(const Vector3& startPosition, const Vector3& endPosition, API_PARAM(Out) Array<Vector3, HeapAllocation>& resultPath) const
    {
        NavMeshPathFlags flags;
        return FindPath(startPosition, endPosition, resultPath, flags);
    }

    /// <summary>
    /// Finds the path between the two positions presented as a list of waypoints stored in the corners array.
    /// </summary>
    /// <param name="startPosition">The start position.</param>
    /// <param name="endPosition">The end position.</param>
    /// <param name="resultPath">The result path.</param>
    /// <param name="resultFlags">The result path flags.</param>
    /// <returns>True if found valid path between given two points (it may be partial), otherwise false if failed.</returns>
    bool FindPath(const Vector3& startPosition, const Vector3& endPosition, API_PARAM(Out) Array<Vector3, HeapAllocation>& resultPath, NavMeshPathFlags& resultFlags) const;

    /// <summary>
    /// Tests the path between the two positions (non-partial).
    /// </summary>
    /// <param name="startPosition">The start position.</param>
    /// <param name="endPosition">The end position.</param>
    /// <returns>True if found valid path between given two points, otherwise false if failed.</returns>
    API_FUNCTION() bool TestPath(const Vector3& startPosition, const Vector3& endPosition) const;

    /// <summary>
    /// Finds the nearest point on a nav mesh surface.
    /// </summary>
    /// <param name="point">The source point.</param>
    /// <param name="result">The result position on the navmesh (valid only if method returns true).</param>
    /// <returns>True if found valid location on the navmesh, otherwise false.</returns>
    API_FUNCTION() bool FindClosestPoint(const Vector3& point, API_PARAM(Out) Vector3& result) const;

    /// <summary>
    /// Projects the point to nav mesh surface (finds the nearest polygon).
    /// [Deprecated in v1.8]
    /// </summary>
    /// <param name="point">The source point.</param>
    /// <param name="result">The result position on the navmesh (valid only if method returns true).</param>
    /// <returns>True if found valid location on the navmesh, otherwise false.</returns>
    API_FUNCTION() DEPRECATED("Use FindClosestPoint instead") bool ProjectPoint(const Vector3& point, API_PARAM(Out) Vector3& result) const
    {
        return FindClosestPoint(point, result);
    }

    /// <summary>
    /// Finds random location on nav mesh.
    /// </summary>
    /// <param name="result">The result position on the navmesh (valid only if method returns true).</param>
    /// <returns>True if found valid location on the navmesh, otherwise false.</returns>
    API_FUNCTION() bool FindRandomPoint(API_PARAM(Out) Vector3& result) const;

    /// <summary>
    /// Finds random location on nav mesh within the reach of specified location.
    /// </summary>
    /// <param name="center">The source point to find random location around it.</param>
    /// <param name="radius">The search distance for a random point. Maximum distance for a result point from the center of the circle.</param>
    /// <param name="result">The result position on the navmesh (valid only if method returns true).</param>
    /// <returns>True if found valid location on the navmesh, otherwise false.</returns>
    API_FUNCTION() bool FindRandomPointAroundCircle(const Vector3& center, float radius, API_PARAM(Out) Vector3& result) const;

    /// <summary>
    /// Casts a 'walkability' ray along the surface of the navigation mesh from the start position toward the end position.
    /// </summary>
    /// <param name="startPosition">The start position.</param>
    /// <param name="endPosition">The end position.</param>
    /// <param name="hitInfo">The result hit information. Valid only when query succeed.</param>
    /// <returns>True if ray hits an matching object, otherwise false.</returns>
    API_FUNCTION() bool RayCast(const Vector3& startPosition, const Vector3& endPosition, API_PARAM(Out) NavMeshHit& hitInfo) const;

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
    /// <param name="navMesh">The navigation mesh.</param>
    void AddTiles(NavMesh* navMesh);

    /// <summary>
    /// Adds the tile from the given scene to the runtime navmesh.
    /// </summary>
    /// <param name="navMesh">The navigation mesh.</param>
    /// <param name="tileData">The tile data.</param>
    void AddTile(NavMesh* navMesh, NavMeshTileData& tileData);

    /// <summary>
    /// Removes all the tiles from the navmesh that has been added from the given navigation scene.
    /// </summary>
    /// <param name="navMesh">The navigation mesh.</param>
    void RemoveTiles(NavMesh* navMesh);

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

#if COMPILE_WITH_DEBUG_DRAW
    void DebugDraw();
#endif

    /// <summary>
    /// Releases the navmesh.
    /// </summary>
    void Dispose();

private:
    void AddTileInternal(NavMesh* navMesh, NavMeshTileData& tileData);
};
