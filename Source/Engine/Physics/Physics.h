// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Quaternion.h"
#include "Types.h"

/// <summary>
/// Physics simulation system.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API Physics
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(Physics);

    /// <summary>
    /// The default physics scene.
    /// </summary>
    API_FIELD(ReadOnly) static PhysicsScene* DefaultScene;

    /// <summary>
    /// List with all physics scenes (readonly).
    /// </summary>
    API_FIELD(ReadOnly) static Array<PhysicsScene*, HeapAllocation> Scenes;

    /// <summary>
    /// Finds an existing <see cref="PhysicsScene"/> or creates it if it does not exist.
    /// </summary>
    API_FUNCTION() static PhysicsScene* FindOrCreateScene(const StringView& name);

    /// <summary>
    /// Finds an existing scene.
    /// </summary>
    API_FUNCTION() static PhysicsScene* FindScene(const StringView& name);

public:
    /// <summary>
    /// The automatic simulation feature. True if perform physics simulation after on fixed update by auto, otherwise user should do it.
    /// </summary>
    API_PROPERTY() static bool GetAutoSimulation();

    /// <summary>
    /// Gets the current gravity force.
    /// </summary>
    API_PROPERTY(Attributes="DebugCommand") static Vector3 GetGravity();

    /// <summary>
    /// Sets the current gravity force.
    /// </summary>
    API_PROPERTY() static void SetGravity(const Vector3& value);

    /// <summary>
    /// Gets the CCD feature enable flag.
    /// </summary>
    API_PROPERTY() static bool GetEnableCCD();

    /// <summary>
    /// Sets the CCD feature enable flag.
    /// </summary>
    API_PROPERTY() static void SetEnableCCD(bool value);

    /// <summary>
    /// Gets the minimum relative velocity required for an object to bounce.
    /// </summary>
    API_PROPERTY() static float GetBounceThresholdVelocity();

    /// <summary>
    /// Sets the minimum relative velocity required for an object to bounce.
    /// </summary>
    API_PROPERTY() static void SetBounceThresholdVelocity(float value);

    /// <summary>
    /// The collision layers masks. Used to define layer-based collision detection.
    /// </summary>
    static uint32 LayerMasks[32];
public:
    /// <summary>
    /// Called during main engine loop to start physic simulation. Use CollectResults after.
    /// </summary>
    /// <param name="dt">The delta time (in seconds).</param>
    API_FUNCTION() static void Simulate(float dt);

    /// <summary>
    /// Called during main engine loop to collect physic simulation results and apply them as well as fire collision events.
    /// </summary>
    API_FUNCTION() static void CollectResults();

    /// <summary>
    /// Checks if physical simulation is running.
    /// </summary>
    API_PROPERTY() static bool IsDuringSimulation();

    /// <summary>
    /// Flushes any latent physics actions (eg. object destroy, actor add/remove to the scene, etc.).
    /// </summary>
    API_FUNCTION() static void FlushRequests();

public:
    /// <summary>
    /// Performs a line between two points in the scene.
    /// </summary>
    /// <param name="start">The start position of the line.</param>
    /// <param name="end">The end position of the line.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if ray hits an matching object, otherwise false.</returns>
    API_FUNCTION() static bool LineCast(const Vector3& start, const Vector3& end, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Performs a line between two points in the scene.
    /// </summary>
    /// <param name="start">The start position of the line.</param>
    /// <param name="end">The end position of the line.</param>
    /// <param name="hitInfo">The result hit information. Valid only when method returns true.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if ray hits an matching object, otherwise false.</returns>
    API_FUNCTION() static bool LineCast(const Vector3& start, const Vector3& end, API_PARAM(Out) RayCastHit& hitInfo, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    // <summary>
    /// Performs a line between two points in the scene, returns all hitpoints infos.
    /// </summary>
    /// <param name="start">The origin of the ray.</param>
    /// <param name="end">The end position of the line.</param>
    /// <param name="results">The result hits. Valid only when method returns true.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if ray hits an matching object, otherwise false.</returns>
    API_FUNCTION() static bool LineCastAll(const Vector3& start, const Vector3& end, API_PARAM(Out) Array<RayCastHit, HeapAllocation>& results, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Performs a raycast against objects in the scene.
    /// </summary>
    /// <param name="origin">The origin of the ray.</param>
    /// <param name="direction">The normalized direction of the ray.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if ray hits an matching object, otherwise false.</returns>
    API_FUNCTION() static bool RayCast(const Vector3& origin, const Vector3& direction, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Performs a raycast against objects in the scene, returns results in a RayCastHit structure.
    /// </summary>
    /// <param name="origin">The origin of the ray.</param>
    /// <param name="direction">The normalized direction of the ray.</param>
    /// <param name="hitInfo">The result hit information. Valid only when method returns true.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if ray hits an matching object, otherwise false.</returns>
    API_FUNCTION() static bool RayCast(const Vector3& origin, const Vector3& direction, API_PARAM(Out) RayCastHit& hitInfo, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Performs a raycast against objects in the scene, returns results in a RayCastHit structure.
    /// </summary>
    /// <param name="origin">The origin of the ray.</param>
    /// <param name="direction">The normalized direction of the ray.</param>
    /// <param name="results">The result hits. Valid only when method returns true.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if ray hits an matching object, otherwise false.</returns>
    API_FUNCTION() static bool RayCastAll(const Vector3& origin, const Vector3& direction, API_PARAM(Out) Array<RayCastHit, HeapAllocation>& results, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Performs a sweep test against objects in the scene using a box geometry.
    /// </summary>
    /// <param name="center">The box center.</param>
    /// <param name="halfExtents">The half size of the box in each direction.</param>
    /// <param name="direction">The normalized direction in which cast a box.</param>
    /// <param name="rotation">The box rotation.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if box hits an matching object, otherwise false.</returns>
    API_FUNCTION() static bool BoxCast(const Vector3& center, const Vector3& halfExtents, const Vector3& direction, const Quaternion& rotation = Quaternion::Identity, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Performs a sweep test against objects in the scene using a box geometry.
    /// </summary>
    /// <param name="center">The box center.</param>
    /// <param name="halfExtents">The half size of the box in each direction.</param>
    /// <param name="direction">The normalized direction in which cast a box.</param>
    /// <param name="hitInfo">The result hit information. Valid only when method returns true.</param>
    /// <param name="rotation">The box rotation.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if box hits an matching object, otherwise false.</returns>
    API_FUNCTION() static bool BoxCast(const Vector3& center, const Vector3& halfExtents, const Vector3& direction, API_PARAM(Out) RayCastHit& hitInfo, const Quaternion& rotation = Quaternion::Identity, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Performs a sweep test against objects in the scene using a box geometry.
    /// </summary>
    /// <param name="center">The box center.</param>
    /// <param name="halfExtents">The half size of the box in each direction.</param>
    /// <param name="direction">The normalized direction in which cast a box.</param>
    /// <param name="results">The result hits. Valid only when method returns true.</param>
    /// <param name="rotation">The box rotation.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if box hits an matching object, otherwise false.</returns>
    API_FUNCTION() static bool BoxCastAll(const Vector3& center, const Vector3& halfExtents, const Vector3& direction, API_PARAM(Out) Array<RayCastHit, HeapAllocation>& results, const Quaternion& rotation = Quaternion::Identity, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Performs a sweep test against objects in the scene using a sphere geometry.
    /// </summary>
    /// <param name="center">The sphere center.</param>
    /// <param name="radius">The radius of the sphere.</param>
    /// <param name="direction">The normalized direction in which cast a sphere.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if sphere hits an matching object, otherwise false.</returns>
    API_FUNCTION() static bool SphereCast(const Vector3& center, float radius, const Vector3& direction, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Performs a sweep test against objects in the scene using a sphere geometry.
    /// </summary>
    /// <param name="center">The sphere center.</param>
    /// <param name="radius">The radius of the sphere.</param>
    /// <param name="direction">The normalized direction in which cast a sphere.</param>
    /// <param name="hitInfo">The result hit information. Valid only when method returns true.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if sphere hits an matching object, otherwise false.</returns>
    API_FUNCTION() static bool SphereCast(const Vector3& center, float radius, const Vector3& direction, API_PARAM(Out) RayCastHit& hitInfo, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Performs a sweep test against objects in the scene using a sphere geometry.
    /// </summary>
    /// <param name="center">The sphere center.</param>
    /// <param name="radius">The radius of the sphere.</param>
    /// <param name="direction">The normalized direction in which cast a sphere.</param>
    /// <param name="results">The result hits. Valid only when method returns true.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if sphere hits an matching object, otherwise false.</returns>
    API_FUNCTION() static bool SphereCastAll(const Vector3& center, float radius, const Vector3& direction, API_PARAM(Out) Array<RayCastHit, HeapAllocation>& results, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Performs a sweep test against objects in the scene using a capsule geometry.
    /// </summary>
    /// <param name="center">The capsule center.</param>
    /// <param name="radius">The radius of the capsule.</param>
    /// <param name="height">The height of the capsule, excluding the top and bottom spheres.</param>
    /// <param name="direction">The normalized direction in which cast a capsule.</param>
    /// <param name="rotation">The capsule rotation.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if capsule hits an matching object, otherwise false.</returns>
    API_FUNCTION() static bool CapsuleCast(const Vector3& center, float radius, float height, const Vector3& direction, const Quaternion& rotation = Quaternion::Identity, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Performs a sweep test against objects in the scene using a capsule geometry.
    /// </summary>
    /// <param name="center">The capsule center.</param>
    /// <param name="radius">The radius of the capsule.</param>
    /// <param name="height">The height of the capsule, excluding the top and bottom spheres.</param>
    /// <param name="direction">The normalized direction in which cast a capsule.</param>
    /// <param name="hitInfo">The result hit information. Valid only when method returns true.</param>
    /// <param name="rotation">The capsule rotation.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if capsule hits an matching object, otherwise false.</returns>
    API_FUNCTION() static bool CapsuleCast(const Vector3& center, float radius, float height, const Vector3& direction, API_PARAM(Out) RayCastHit& hitInfo, const Quaternion& rotation = Quaternion::Identity, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Performs a sweep test against objects in the scene using a capsule geometry.
    /// </summary>
    /// <param name="center">The capsule center.</param>
    /// <param name="radius">The radius of the capsule.</param>
    /// <param name="height">The height of the capsule, excluding the top and bottom spheres.</param>
    /// <param name="direction">The normalized direction in which cast a capsule.</param>
    /// <param name="results">The result hits. Valid only when method returns true.</param>
    /// <param name="rotation">The capsule rotation.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if capsule hits an matching object, otherwise false.</returns>
    API_FUNCTION() static bool CapsuleCastAll(const Vector3& center, float radius, float height, const Vector3& direction, API_PARAM(Out) Array<RayCastHit, HeapAllocation>& results, const Quaternion& rotation = Quaternion::Identity, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Performs a sweep test against objects in the scene using a convex mesh.
    /// </summary>
    /// <param name="center">The convex mesh center.</param>
    /// <param name="convexMesh">Collision data of the convex mesh.</param>
    /// <param name="scale">The scale of the convex mesh.</param>
    /// <param name="direction">The normalized direction in which cast a convex mesh.</param>
    /// <param name="rotation">The convex mesh rotation.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if convex mesh hits an matching object, otherwise false.</returns>
    API_FUNCTION() static bool ConvexCast(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, const Quaternion& rotation = Quaternion::Identity, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Performs a sweep test against objects in the scene using a convex mesh.
    /// </summary>
    /// <param name="center">The convex mesh center.</param>
    /// <param name="convexMesh">Collision data of the convex mesh.</param>
    /// <param name="scale">The scale of the convex mesh.</param>
    /// <param name="direction">The normalized direction in which cast a convex mesh.</param>
    /// <param name="hitInfo">The result hit information. Valid only when method returns true.</param>
    /// <param name="rotation">The convex mesh rotation.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if convex mesh hits an matching object, otherwise false.</returns>
    API_FUNCTION() static bool ConvexCast(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, API_PARAM(Out) RayCastHit& hitInfo, const Quaternion& rotation = Quaternion::Identity, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Performs a sweep test against objects in the scene using a convex mesh.
    /// </summary>
    /// <param name="center">The convex mesh center.</param>
    /// <param name="convexMesh">Collision data of the convex mesh.</param>
    /// <param name="scale">The scale of the convex mesh.</param>
    /// <param name="direction">The normalized direction in which cast a convex mesh.</param>
    /// <param name="results">The result hits. Valid only when method returns true.</param>
    /// <param name="rotation">The convex mesh rotation.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if convex mesh hits an matching object, otherwise false.</returns>
    API_FUNCTION() static bool ConvexCastAll(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, API_PARAM(Out) Array<RayCastHit, HeapAllocation>& results, const Quaternion& rotation = Quaternion::Identity, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Checks whether the given box overlaps with other colliders or not.
    /// </summary>
    /// <param name="center">The box center.</param>
    /// <param name="halfExtents">The half size of the box in each direction.</param>
    /// <param name="rotation">The box rotation.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if box overlaps any matching object, otherwise false.</returns>
    API_FUNCTION() static bool CheckBox(const Vector3& center, const Vector3& halfExtents, const Quaternion& rotation = Quaternion::Identity, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Checks whether the given sphere overlaps with other colliders or not.
    /// </summary>
    /// <param name="center">The sphere center.</param>
    /// <param name="radius">The radius of the sphere.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if sphere overlaps any matching object, otherwise false.</returns>
    API_FUNCTION() static bool CheckSphere(const Vector3& center, float radius, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Checks whether the given capsule overlaps with other colliders or not.
    /// </summary>
    /// <param name="center">The capsule center.</param>
    /// <param name="radius">The radius of the capsule.</param>
    /// <param name="height">The height of the capsule, excluding the top and bottom spheres.</param>
    /// <param name="rotation">The capsule rotation.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if capsule overlaps any matching object, otherwise false.</returns>
    API_FUNCTION() static bool CheckCapsule(const Vector3& center, float radius, float height, const Quaternion& rotation = Quaternion::Identity, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Checks whether the given convex mesh overlaps with other colliders or not.
    /// </summary>
    /// <param name="center">The convex mesh center.</param>
    /// <param name="convexMesh">Collision data of the convex mesh.</param>
    /// <param name="scale">The scale of the convex mesh.</param>
    /// <param name="rotation">The convex mesh rotation.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if convex mesh overlaps any matching object, otherwise false.</returns>
    API_FUNCTION() static bool CheckConvex(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Quaternion& rotation = Quaternion::Identity, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Finds all colliders touching or inside of the given box.
    /// </summary>
    /// <param name="center">The box center.</param>
    /// <param name="halfExtents">The half size of the box in each direction.</param>
    /// <param name="rotation">The box rotation.</param>
    /// <param name="results">The result colliders that overlap with the given box. Valid only when method returns true.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if box overlaps any matching object, otherwise false.</returns>
    API_FUNCTION() static bool OverlapBox(const Vector3& center, const Vector3& halfExtents, API_PARAM(Out) Array<Collider*, HeapAllocation>& results, const Quaternion& rotation = Quaternion::Identity, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Finds all colliders touching or inside of the given sphere.
    /// </summary>
    /// <param name="center">The sphere center.</param>
    /// <param name="radius">The radius of the sphere.</param>
    /// <param name="results">The result colliders that overlap with the given sphere. Valid only when method returns true.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if sphere overlaps any matching object, otherwise false.</returns>
    API_FUNCTION() static bool OverlapSphere(const Vector3& center, float radius, API_PARAM(Out) Array<Collider*, HeapAllocation>& results, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Finds all colliders touching or inside of the given capsule.
    /// </summary>
    /// <param name="center">The capsule center.</param>
    /// <param name="radius">The radius of the capsule.</param>
    /// <param name="height">The height of the capsule, excluding the top and bottom spheres.</param>
    /// <param name="results">The result colliders that overlap with the given capsule. Valid only when method returns true.</param>
    /// <param name="rotation">The capsule rotation.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if capsule overlaps any matching object, otherwise false.</returns>
    API_FUNCTION() static bool OverlapCapsule(const Vector3& center, float radius, float height, API_PARAM(Out) Array<Collider*, HeapAllocation>& results, const Quaternion& rotation = Quaternion::Identity, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Finds all colliders touching or inside of the given convex mesh.
    /// </summary>
    /// <param name="center">The convex mesh center.</param>
    /// <param name="convexMesh">Collision data of the convex mesh.</param>
    /// <param name="scale">The scale of the convex mesh.</param>
    /// <param name="results">The result colliders that overlap with the given convex mesh. Valid only when method returns true.</param>
    /// <param name="rotation">The convex mesh rotation.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if convex mesh overlaps any matching object, otherwise false.</returns>
    API_FUNCTION() static bool OverlapConvex(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, API_PARAM(Out) Array<Collider*, HeapAllocation>& results, const Quaternion& rotation = Quaternion::Identity, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Finds all colliders touching or inside of the given box.
    /// </summary>
    /// <param name="center">The box center.</param>
    /// <param name="halfExtents">The half size of the box in each direction.</param>
    /// <param name="rotation">The box rotation.</param>
    /// <param name="results">The result colliders that overlap with the given box. Valid only when method returns true.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if box overlaps any matching object, otherwise false.</returns>
    API_FUNCTION() static bool OverlapBox(const Vector3& center, const Vector3& halfExtents, API_PARAM(Out) Array<PhysicsColliderActor*, HeapAllocation>& results, const Quaternion& rotation = Quaternion::Identity, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Finds all colliders touching or inside of the given sphere.
    /// </summary>
    /// <param name="center">The sphere center.</param>
    /// <param name="radius">The radius of the sphere.</param>
    /// <param name="results">The result colliders that overlap with the given sphere. Valid only when method returns true.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if sphere overlaps any matching object, otherwise false.</returns>
    API_FUNCTION() static bool OverlapSphere(const Vector3& center, float radius, API_PARAM(Out) Array<PhysicsColliderActor*, HeapAllocation>& results, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Finds all colliders touching or inside of the given capsule.
    /// </summary>
    /// <param name="center">The capsule center.</param>
    /// <param name="radius">The radius of the capsule.</param>
    /// <param name="height">The height of the capsule, excluding the top and bottom spheres.</param>
    /// <param name="results">The result colliders that overlap with the given capsule. Valid only when method returns true.</param>
    /// <param name="rotation">The capsule rotation.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if capsule overlaps any matching object, otherwise false.</returns>
    API_FUNCTION() static bool OverlapCapsule(const Vector3& center, float radius, float height, API_PARAM(Out) Array<PhysicsColliderActor*, HeapAllocation>& results, const Quaternion& rotation = Quaternion::Identity, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Finds all colliders touching or inside of the given convex mesh.
    /// </summary>
    /// <param name="center">The convex mesh center.</param>
    /// <param name="convexMesh">Collision data of the convex mesh.</param>
    /// <param name="scale">The scale of the convex mesh.</param>
    /// <param name="results">The result colliders that overlap with the given convex mesh. Valid only when method returns true.</param>
    /// <param name="rotation">The convex mesh rotation.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if convex mesh overlaps any matching object, otherwise false.</returns>
    API_FUNCTION() static bool OverlapConvex(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, API_PARAM(Out) Array<PhysicsColliderActor*, HeapAllocation>& results, const Quaternion& rotation = Quaternion::Identity, uint32 layerMask = MAX_uint32, bool hitTriggers = true);
};
