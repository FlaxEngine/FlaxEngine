// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"
#include "Engine/Physics/Collisions.h"

struct RayCastHit;
struct Collision;

/// <summary>
/// A base class for all physical collider actors.
/// </summary>
/// <seealso cref="Actor" />
API_CLASS(Abstract) class FLAXENGINE_API PhysicsColliderActor : public Actor
{
    DECLARE_SCENE_OBJECT_ABSTRACT(PhysicsColliderActor);
public:
    /// <summary>
    /// Occurs when a collision start gets registered for this collider (it collides with something).
    /// </summary>
    API_EVENT() Delegate<const Collision&> CollisionEnter;

    /// <summary>
    /// Occurs when a collision end gets registered for this collider (it ends colliding with something).
    /// </summary>
    API_EVENT() Delegate<const Collision&> CollisionExit;

    /// <summary>
    /// Occurs when a trigger touching start gets registered for this collider (the other collider enters it and triggers the event).
    /// </summary>
    API_EVENT() Delegate<PhysicsColliderActor*> TriggerEnter;

    /// <summary>
    /// Occurs when a trigger touching end gets registered for this collider (the other collider enters it and triggers the event).
    /// </summary>
    API_EVENT() Delegate<PhysicsColliderActor*> TriggerExit;

public:
    /// <summary>
    /// Gets the attached rigid body.
    /// </summary>
    /// <returns>The rigid body or null.</returns>
    API_PROPERTY() virtual RigidBody* GetAttachedRigidBody() const = 0;

    /// <summary>
    /// Performs a raycast against this collider shape.
    /// </summary>
    /// <param name="origin">The origin of the ray.</param>
    /// <param name="direction">The normalized direction of the ray.</param>
    /// <param name="resultHitDistance">The raycast result hit position distance from the ray origin. Valid only if raycast hits anything.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <returns>True if ray hits an object, otherwise false.</returns>
    API_FUNCTION(Sealed) virtual bool RayCast(const Vector3& origin, const Vector3& direction, API_PARAM(Out) float& resultHitDistance, float maxDistance = MAX_float) const = 0;

    /// <summary>
    /// Performs a raycast against this collider, returns results in a RaycastHit structure.
    /// </summary>
    /// <param name="origin">The origin of the ray.</param>
    /// <param name="direction">The normalized direction of the ray.</param>
    /// <param name="hitInfo">The result hit information. Valid only when method returns true.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <returns>True if ray hits an object, otherwise false.</returns>
    API_FUNCTION(Sealed) virtual bool RayCast(const Vector3& origin, const Vector3& direction, API_PARAM(Out) RayCastHit& hitInfo, float maxDistance = MAX_float) const = 0;

    /// <summary>
    /// Gets a point on the collider that is closest to a given location. Can be used to find a hit location or position to apply explosion force or any other special effects.
    /// </summary>
    /// <param name="point">The position to find the closest point to it.</param>
    /// <param name="result">The result point on the collider that is closest to the specified location.</param>
    API_FUNCTION(Sealed) virtual void ClosestPoint(const Vector3& point, API_PARAM(Out) Vector3& result) const = 0;

    /// <summary>
    /// Checks if a point is inside the collider.
    /// </summary>
    /// <param name="point">The point to check if is contained by the collider shape (in world-space).</param>
    /// <returns>True if collider shape contains a given point, otherwise false.</returns>
    API_FUNCTION(Sealed) virtual bool ContainsPoint(const Vector3& point) const = 0;

public:
    /// <summary>
    /// Called when a collision start gets registered for this collider (it collides with something).
    /// </summary>
    /// <param name="c">The collision info.</param>
    API_FUNCTION() virtual void OnCollisionEnter(const Collision& c);

    /// <summary>
    /// Called when a collision end gets registered for this collider (it ends colliding with something).
    /// </summary>
    /// <param name="c">The collision info.</param>
    API_FUNCTION() virtual void OnCollisionExit(const Collision& c);

    /// <summary>
    /// Called when a trigger touching start gets registered for this collider (the other collider enters it and triggers the event).
    /// </summary>
    /// <param name="c">The other collider.</param>
    API_FUNCTION() virtual void OnTriggerEnter(PhysicsColliderActor* c);

    /// <summary>
    /// Called when a trigger touching end gets registered for this collider (the other collider enters it and triggers the event).
    /// </summary>
    /// <param name="c">The other collider.</param>
    API_FUNCTION() virtual void OnTriggerExit(PhysicsColliderActor* c);
};
