// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"
#include "Engine/Physics/Collisions.h"

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
