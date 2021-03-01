// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "NavigationTypes.h"

class Scene;

/// <summary>
/// The navigation service used for path finding and agents navigation system.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API Navigation
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(Navigation);
public:

    /// <summary>
    /// Finds the distance from the specified start position to the nearest polygon wall.
    /// </summary>
    /// <param name="startPosition">The start position.</param>
    /// <param name="hitInfo">The result hit information. Valid only when query succeed.</param>
    /// <param name="maxDistance">The maximum distance to search for wall (search radius).</param>
    /// <returns>True if ray hits an matching object, otherwise false.</returns>
    API_FUNCTION() static bool FindDistanceToWall(const Vector3& startPosition, API_PARAM(Out) NavMeshHit& hitInfo, float maxDistance = MAX_float);

    /// <summary>
    /// Finds the path between the two positions presented as a list of waypoints stored in the corners array.
    /// </summary>
    /// <param name="startPosition">The start position.</param>
    /// <param name="endPosition">The end position.</param>
    /// <param name="resultPath">The result path.</param>
    /// <returns>True if found valid path between given two points (it may be partial), otherwise false if failed.</returns>
    API_FUNCTION() static bool FindPath(const Vector3& startPosition, const Vector3& endPosition, API_PARAM(Out) Array<Vector3, HeapAllocation>& resultPath);

    /// <summary>
    /// Tests the path between the two positions (non-partial).
    /// </summary>
    /// <param name="startPosition">The start position.</param>
    /// <param name="endPosition">The end position.</param>
    /// <returns>True if found valid path between given two points, otherwise false if failed.</returns>
    API_FUNCTION() static bool TestPath(const Vector3& startPosition, const Vector3& endPosition);

    /// <summary>
    /// Projects the point to nav mesh surface (finds the nearest polygon).
    /// </summary>
    /// <param name="point">The source point.</param>
    /// <param name="result">The result position on the navmesh (valid only if method returns true).</param>
    /// <returns>True if found valid location on the navmesh, otherwise false.</returns>
    API_FUNCTION() static bool ProjectPoint(const Vector3& point, API_PARAM(Out) Vector3& result);

    /// <summary>
    /// Finds random location on nav mesh.
    /// </summary>
    /// <param name="result">The result position on the navmesh (valid only if method returns true).</param>
    /// <returns>True if found valid location on the navmesh, otherwise false.</returns>
    API_FUNCTION() static bool FindRandomPoint(API_PARAM(Out) Vector3& result);

    /// <summary>
    /// Finds random location on nav mesh within the reach of specified location.
    /// </summary>
    /// <param name="center">The source point to find random location around it.</param>
    /// <param name="radius">The search distance for a random point. Maximum distance for a result point from the center of the circle.</param>
    /// <param name="result">The result position on the navmesh (valid only if method returns true).</param>
    /// <returns>True if found valid location on the navmesh, otherwise false.</returns>
    API_FUNCTION() static bool FindRandomPointAroundCircle(const Vector3& center, float radius, API_PARAM(Out) Vector3& result);

    /// <summary>
    /// Casts a 'walkability' ray along the surface of the navigation mesh from the start position toward the end position.
    /// </summary>
    /// <param name="startPosition">The start position.</param>
    /// <param name="endPosition">The end position.</param>
    /// <param name="hitInfo">The result hit information. Valid only when query succeed.</param>
    /// <returns>True if ray hits an matching object, otherwise false.</returns>
    API_FUNCTION() static bool RayCast(const Vector3& startPosition, const Vector3& endPosition, API_PARAM(Out) NavMeshHit& hitInfo);

public:

#if COMPILE_WITH_NAV_MESH_BUILDER

    /// <summary>
    /// Returns true if navigation system is during navmesh building (any request is valid or async task active).
    /// </summary>
    API_PROPERTY() static bool IsBuildingNavMesh();

    /// <summary>
    /// Gets the navmesh building progress (normalized to range 0-1).
    /// </summary>
    API_PROPERTY() static float GetNavMeshBuildingProgress();

    /// <summary>
    /// Builds the Nav Mesh for the given scene (discards all its tiles).
    /// </summary>
    /// <remarks>
    /// Requests are enqueued till the next game scripts update. Actual navmesh building in done via Thread Pool tasks in a background to prevent game thread stalls.
    /// </remarks>
    /// <param name="scene">The scene.</param>
    /// <param name="timeoutMs">The timeout to wait before building Nav Mesh (in milliseconds).</param>
    API_FUNCTION() static void BuildNavMesh(Scene* scene, float timeoutMs = 50);

    /// <summary>
    /// Builds the Nav Mesh for the given scene (builds only the tiles overlapping the given bounding box).
    /// </summary>
    /// <remarks>
    /// Requests are enqueued till the next game scripts update. Actual navmesh building in done via Thread Pool tasks in a background to prevent game thread stalls.
    /// </remarks>
    /// <param name="scene">The scene.</param>
    /// <param name="dirtyBounds">The bounds in world-space to build overlapping tiles.</param>
    /// <param name="timeoutMs">The timeout to wait before building Nav Mesh (in milliseconds).</param>
    API_FUNCTION() static void BuildNavMesh(Scene* scene, const BoundingBox& dirtyBounds, float timeoutMs = 50);

#endif

#if COMPILE_WITH_DEBUG_DRAW

    /// <summary>
    /// Draws the navigation for all the scenes (uses DebugDraw interface).
    /// </summary>
    static void DrawNavMesh();

#endif
};
