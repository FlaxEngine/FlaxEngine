// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Types.h"

struct ActionData;
struct RayCastHit;
class PhysicsSettings;
class PhysicsColliderActor;
class Joint;
class Collider;
class CollisionData;
#if WITH_VEHICLE
class WheeledVehicle;
#endif

/// <summary>
/// Physical simulation scene.
/// </summary>
API_CLASS() class FLAXENGINE_API PhysicsScene : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE(PhysicsScene);
private:
    String _name;
    bool _autoSimulation = true;
    bool _isDuringSimulation = false;
    Vector3 _origin = Vector3::Zero;
    void* _scene = nullptr;

public:
    ~PhysicsScene();

    /// <summary>
    /// Gets the name of the scene.
    /// </summary>
    API_PROPERTY() String GetName() const;

    /// <summary>
    /// Gets the native physics system scene object.
    /// </summary>
    FORCE_INLINE void* GetPhysicsScene() const
    {
        return _scene;
    }

    /// <summary>
    /// Gets the automatic simulation feature that perform physics simulation after on fixed update by auto, otherwise user should do it.
    /// </summary>
    API_PROPERTY() bool GetAutoSimulation() const;

    /// <summary>
    /// Sets the automatic simulation feature that perform physics simulation after on fixed update by auto, otherwise user should do it.
    /// </summary>
    API_PROPERTY() void SetAutoSimulation(bool value);

    /// <summary>
    /// Gets the current gravity force.
    /// </summary>
    API_PROPERTY() Vector3 GetGravity() const;

    /// <summary>
    /// Sets the current gravity force.
    /// </summary>
    API_PROPERTY() void SetGravity(const Vector3& value);

    /// <summary>
    /// Gets the CCD feature enable flag.
    /// </summary>
    API_PROPERTY() bool GetEnableCCD() const;

    /// <summary>
    /// Sets the CCD feature enable flag.
    /// </summary>
    API_PROPERTY() void SetEnableCCD(bool value);

    /// <summary>
    /// Gets the minimum relative velocity required for an object to bounce.
    /// </summary>
    API_PROPERTY() float GetBounceThresholdVelocity();

    /// <summary>
    /// Sets the minimum relative velocity required for an object to bounce.
    /// </summary>
    API_PROPERTY() void SetBounceThresholdVelocity(float value);

    /// <summary>
    /// Gets the current scene origin that defines the center of the simulation (in world). Can be used to run physics simulation relative to the camera.
    /// </summary>
    API_PROPERTY() FORCE_INLINE Vector3 GetOrigin() const
    {
        return _origin;
    }

    /// <summary>
    /// Sets the current scene origin that defines the center of the simulation (in world). Can be used to run physics simulation relative to the camera.
    /// </summary>
    API_PROPERTY() void SetOrigin(const Vector3& value);

#if COMPILE_WITH_PROFILER
    /// <summary>
    /// Gets the physics simulation statistics for the scene.
    /// </summary>
    API_PROPERTY() PhysicsStatistics GetStatistics() const;
#endif

public:
    /// <summary>
    /// Initializes the scene.
    /// </summary>
    /// <param name="name">The name.</param>
    /// <param name="settings">The physics settings.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool Init(const StringView& name, const PhysicsSettings& settings);

    /// <summary>
    /// Called during main engine loop to start physic simulation. Use CollectResults after.
    /// </summary>
    /// <param name="dt">The delta time (in seconds).</param>
    API_FUNCTION() void Simulate(float dt);

    /// <summary>
    /// Checks if physical simulation is running.
    /// </summary>
    API_PROPERTY() bool IsDuringSimulation() const;

    /// <summary>
    /// Called to collect physic simulation results and apply them as well as fire collision events.
    /// </summary>
    API_FUNCTION() void CollectResults();

public:
    /// <summary>
    /// Performs a line between two points in the scene.
    /// </summary>
    /// <param name="start">The start position of the line.</param>
    /// <param name="end">The end position of the line.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if ray hits an matching object, otherwise false.</returns>
    API_FUNCTION() bool LineCast(const Vector3& start, const Vector3& end, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Performs a line between two points in the scene.
    /// </summary>
    /// <param name="start">The start position of the line.</param>
    /// <param name="end">The end position of the line.</param>
    /// <param name="hitInfo">The result hit information. Valid only when method returns true.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if ray hits an matching object, otherwise false.</returns>
    API_FUNCTION() bool LineCast(const Vector3& start, const Vector3& end, API_PARAM(Out) RayCastHit& hitInfo, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    // <summary>
    /// Performs a line between two points in the scene, returns all hitpoints infos.
    /// </summary>
    /// <param name="start">The origin of the ray.</param>
    /// <param name="end">The normalized direction of the ray.</param>
    /// <param name="results">The result hits. Valid only when method returns true.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if ray hits an matching object, otherwise false.</returns>
    API_FUNCTION() bool LineCastAll(const Vector3& start, const Vector3& end, API_PARAM(Out) Array<RayCastHit, HeapAllocation>& results, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Performs a raycast against objects in the scene.
    /// </summary>
    /// <param name="origin">The origin of the ray.</param>
    /// <param name="direction">The normalized direction of the ray.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if ray hits an matching object, otherwise false.</returns>
    API_FUNCTION() bool RayCast(const Vector3& origin, const Vector3& direction, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool RayCast(const Vector3& origin, const Vector3& direction, API_PARAM(Out) RayCastHit& hitInfo, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool RayCastAll(const Vector3& origin, const Vector3& direction, API_PARAM(Out) Array<RayCastHit, HeapAllocation>& results, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool BoxCast(const Vector3& center, const Vector3& halfExtents, const Vector3& direction, const Quaternion& rotation = Quaternion::Identity, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool BoxCast(const Vector3& center, const Vector3& halfExtents, const Vector3& direction, API_PARAM(Out) RayCastHit& hitInfo, const Quaternion& rotation = Quaternion::Identity, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool BoxCastAll(const Vector3& center, const Vector3& halfExtents, const Vector3& direction, API_PARAM(Out) Array<RayCastHit, HeapAllocation>& results, const Quaternion& rotation = Quaternion::Identity, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool SphereCast(const Vector3& center, float radius, const Vector3& direction, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool SphereCast(const Vector3& center, float radius, const Vector3& direction, API_PARAM(Out) RayCastHit& hitInfo, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool SphereCastAll(const Vector3& center, float radius, const Vector3& direction, API_PARAM(Out) Array<RayCastHit, HeapAllocation>& results, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool CapsuleCast(const Vector3& center, float radius, float height, const Vector3& direction, const Quaternion& rotation = Quaternion::Identity, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool CapsuleCast(const Vector3& center, float radius, float height, const Vector3& direction, API_PARAM(Out) RayCastHit& hitInfo, const Quaternion& rotation = Quaternion::Identity, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool CapsuleCastAll(const Vector3& center, float radius, float height, const Vector3& direction, API_PARAM(Out) Array<RayCastHit, HeapAllocation>& results, const Quaternion& rotation = Quaternion::Identity, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool ConvexCast(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, const Quaternion& rotation = Quaternion::Identity, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool ConvexCast(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, API_PARAM(Out) RayCastHit& hitInfo, const Quaternion& rotation = Quaternion::Identity, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool ConvexCastAll(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, API_PARAM(Out) Array<RayCastHit, HeapAllocation>& results, const Quaternion& rotation = Quaternion::Identity, float maxDistance = MAX_float, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Checks whether the given box overlaps with other colliders or not.
    /// </summary>
    /// <param name="center">The box center.</param>
    /// <param name="halfExtents">The half size of the box in each direction.</param>
    /// <param name="rotation">The box rotation.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if box overlaps any matching object, otherwise false.</returns>
    API_FUNCTION() bool CheckBox(const Vector3& center, const Vector3& halfExtents, const Quaternion& rotation = Quaternion::Identity, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Checks whether the given sphere overlaps with other colliders or not.
    /// </summary>
    /// <param name="center">The sphere center.</param>
    /// <param name="radius">The radius of the sphere.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if sphere overlaps any matching object, otherwise false.</returns>
    API_FUNCTION() bool CheckSphere(const Vector3& center, float radius, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool CheckCapsule(const Vector3& center, float radius, float height, const Quaternion& rotation = Quaternion::Identity, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool CheckConvex(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Quaternion& rotation = Quaternion::Identity, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool OverlapBox(const Vector3& center, const Vector3& halfExtents, API_PARAM(Out) Array<Collider*, HeapAllocation>& results, const Quaternion& rotation = Quaternion::Identity, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Finds all colliders touching or inside of the given sphere.
    /// </summary>
    /// <param name="center">The sphere center.</param>
    /// <param name="radius">The radius of the sphere.</param>
    /// <param name="results">The result colliders that overlap with the given sphere. Valid only when method returns true.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if sphere overlaps any matching object, otherwise false.</returns>
    API_FUNCTION() bool OverlapSphere(const Vector3& center, float radius, API_PARAM(Out) Array<Collider*, HeapAllocation>& results, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool OverlapCapsule(const Vector3& center, float radius, float height, API_PARAM(Out) Array<Collider*, HeapAllocation>& results, const Quaternion& rotation = Quaternion::Identity, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool OverlapConvex(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, API_PARAM(Out) Array<Collider*, HeapAllocation>& results, const Quaternion& rotation = Quaternion::Identity, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool OverlapBox(const Vector3& center, const Vector3& halfExtents, API_PARAM(Out) Array<PhysicsColliderActor*, HeapAllocation>& results, const Quaternion& rotation = Quaternion::Identity, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

    /// <summary>
    /// Finds all colliders touching or inside of the given sphere.
    /// </summary>
    /// <param name="center">The sphere center.</param>
    /// <param name="radius">The radius of the sphere.</param>
    /// <param name="results">The result colliders that overlap with the given sphere. Valid only when method returns true.</param>
    /// <param name="layerMask">The layer mask used to filter the results.</param>
    /// <param name="hitTriggers">If set to <c>true</c> triggers will be hit, otherwise will skip them.</param>
    /// <returns>True if sphere overlaps any matching object, otherwise false.</returns>
    API_FUNCTION() bool OverlapSphere(const Vector3& center, float radius, API_PARAM(Out) Array<PhysicsColliderActor*, HeapAllocation>& results, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool OverlapCapsule(const Vector3& center, float radius, float height, API_PARAM(Out) Array<PhysicsColliderActor*, HeapAllocation>& results, const Quaternion& rotation = Quaternion::Identity, uint32 layerMask = MAX_uint32, bool hitTriggers = true);

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
    API_FUNCTION() bool OverlapConvex(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, API_PARAM(Out) Array<PhysicsColliderActor*, HeapAllocation>& results, const Quaternion& rotation = Quaternion::Identity, uint32 layerMask = MAX_uint32, bool hitTriggers = true);
};
