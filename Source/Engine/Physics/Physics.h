// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Types.h"

class PhysicsColliderActor;
class Joint;
class Collider;
class CollisionData;

/// <summary>
/// Raycast hit result data.
/// </summary>
API_STRUCT() struct RayCastHit
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(RayCastHit);

    /// <summary>
    /// The collider that was hit.
    /// </summary>
    API_FIELD() PhysicsColliderActor* Collider = nullptr;

    /// <summary>
    /// The normal of the surface the ray hit.
    /// </summary>
    API_FIELD() Vector3 Normal;

    /// <summary>
    /// The distance from the ray's origin to the hit location.
    /// </summary>
    API_FIELD() float Distance;

    /// <summary>
    /// The point in the world space where ray hit the collider.
    /// </summary>
    API_FIELD() Vector3 Point;

    /// <summary>
    /// The index of the face that was hit. Valid only for convex mesh (polygon index), triangle mesh (triangle index) and height field (triangle index).
    /// </summary>
    /// <seealso cref="CollisionData.GetModelTriangle" />
    API_FIELD() uint32 FaceIndex;

    /// <summary>
    /// The barycentric coordinates of hit triangle. Valid only for triangle mesh and height field.
    /// </summary>
    API_FIELD() Vector2 UV;

public:

    /// <summary>
    /// Gathers the data from the specified hit (PhysX).
    /// </summary>
    /// <param name="hit">The hit.</param>
    void Gather(const PxRaycastHit& hit);

    /// <summary>
    /// Gathers the data from the specified hit (PhysX).
    /// </summary>
    /// <param name="hit">The hit.</param>
    void Gather(const PxSweepHit& hit);
};

/// <summary>
/// Physics simulation system.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API Physics
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(Physics);

    /// <summary>
    /// Gets the master physics object.
    /// </summary>
    static PxPhysics* GetPhysics();

#if COMPILE_WITH_PHYSICS_COOKING

    /// <summary>
    /// Gets physics cooking object
    /// </summary>
    static PxCooking* GetCooking();

#endif

    /// <summary>
    /// Gets PhysX scene object
    /// </summary>
    static PxScene* GetScene();

    /// <summary>
    /// Gets PhysX characters controller manager object
    /// </summary>
    static PxControllerManager* GetControllerManager();

    /// <summary>
    /// Gets the physics tolerances scale.
    /// </summary>
    static PxTolerancesScale* GetTolerancesScale();

    /// <summary>
    /// Gets the default query filter callback used for the scene queries.
    /// </summary>
    static PxQueryFilterCallback* GetQueryFilterCallback();

    /// <summary>
    /// Gets the default query filter callback used for the character controller collisions detection.
    /// </summary>
    static PxQueryFilterCallback* GetCharacterQueryFilterCallback();

    /// <summary>
    /// Gets the default controller filter callback used for the character controller collisions detection.
    /// </summary>
    static PxControllerFilterCallback* GetCharacterControllerFilterCallback();

    /// <summary>
    /// Gets the default physical material.
    /// </summary>
    static PxMaterial* GetDefaultMaterial();

public:

    /// <summary>
    /// The automatic simulation feature. True if perform physics simulation after on fixed update by auto, otherwise user should do it.
    /// </summary>
    API_FIELD() static bool AutoSimulation;

    /// <summary>
    /// Gets the current gravity force.
    /// </summary>
    API_PROPERTY() static Vector3 GetGravity();

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
    /// Checks if physical simulation is running
    /// </summary>
    /// <returns>True if simulation is active, otherwise false</returns>
    API_PROPERTY() static bool IsDuringSimulation();

public:

    /// <summary>
    /// Flushes the async requests to add/remove actors, remove materials, etc..
    /// </summary>
    static void FlushRequests();

    /// <summary>
    /// Removes the material (using safe async request).
    /// </summary>
    /// <param name="material">The material.</param>
    static void RemoveMaterial(PxMaterial* material);

    /// <summary>
    /// Removes the physX object via calling release() on it (using safe async request).
    /// </summary>
    /// <param name="obj">The obj.</param>
    static void RemoveObject(PxBase* obj);

    /// <summary>
    /// Adds the actor (using safe async request).
    /// </summary>
    /// <param name="actor">The actor.</param>
    static void AddActor(PxActor* actor);

    /// <summary>
    /// Adds the actor (using safe async request).
    /// </summary>
    /// <param name="actor">The actor.</param>
    /// <param name="putToSleep">If set to <c>true</c> will put actor to sleep after spawning.</param>
    static void AddActor(PxRigidDynamic* actor, bool putToSleep = false);

    /// <summary>
    /// Removes the actor (using safe async request).
    /// </summary>
    /// <param name="actor">The actor.</param>
    static void RemoveActor(PxActor* actor);

    /// <summary>
    /// Marks that collider has been removed (all collision events should be cleared to prevent leaks of using removed object).
    /// </summary>
    /// <param name="collider">The collider.</param>
    static void RemoveCollider(PhysicsColliderActor* collider);

    /// <summary>
    /// Marks that joint has been removed (all collision events should be cleared to prevent leaks of using removed object).
    /// </summary>
    /// <param name="joint">The joint.</param>
    static void RemoveJoint(Joint* joint);
};
