// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Vector3.h"

class PhysicsColliderActor;

/// <summary>
/// Contains a contact point data for the collision location.
/// </summary>
API_STRUCT(NoDefault) struct FLAXENGINE_API ContactPoint
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(ContactPoint);

    /// <summary>
    /// The contact point location in the world space.
    /// </summary>
    API_FIELD() Vector3 Point;

    /// <summary>
    /// The separation value (negative implies penetration).
    /// </summary>
    API_FIELD() float Separation;

    /// <summary>
    /// The contact normal.
    /// </summary>
    API_FIELD() Vector3 Normal;
};

template<>
struct TIsPODType<ContactPoint>
{
    enum { Value = true };
};

// The maximum amount of the contact points to be stored within a single collision data (higher amount will be skipped).
#define COLLISION_NAX_CONTACT_POINTS 8

/// <summary>
/// Contains a collision information passed to the OnCollisionEnter/OnCollisionExit events.
/// </summary>
API_STRUCT(NoDefault) struct FLAXENGINE_API Collision
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(Collision);

    /// <summary>
    /// The first collider (this instance).
    /// </summary>
    API_FIELD() PhysicsColliderActor* ThisActor;

    /// <summary>
    /// The second collider (other instance).
    /// </summary>
    API_FIELD() PhysicsColliderActor* OtherActor;

    /// <summary>
    /// The total impulse applied to this contact pair to resolve the collision.
    /// </summary>
    /// <remarks>The total impulse is obtained by summing up impulses applied at all contact points in this collision pair.</remarks>
    API_FIELD() Vector3 Impulse;

    /// <summary>
    /// The linear velocity of the first colliding object (this instance).
    /// </summary>
    API_FIELD() Vector3 ThisVelocity;

    /// <summary>
    /// The linear velocity of the second colliding object (other instance).
    /// </summary>
    API_FIELD() Vector3 OtherVelocity;

    /// <summary>
    /// The amount of valid contact points (less or equal to COLLISION_NAX_CONTACT_POINTS).
    /// </summary>
    API_FIELD() int32 ContactsCount;

    /// <summary>
    /// The contacts locations.
    /// </summary>
    API_FIELD(Internal, NoArray) ContactPoint Contacts[COLLISION_NAX_CONTACT_POINTS];

public:
    /// <summary>
    /// Gets the relative linear velocity of the two colliding objects.
    /// </summary>
    /// <remarks>Can be used to detect stronger collisions. </remarks>
    Vector3 GetRelativeVelocity() const
    {
        return ThisVelocity - OtherVelocity;
    }

    /// <summary>
    /// Swaps the colliding objects (swaps A with B).
    /// </summary>
    void SwapObjects()
    {
        Swap(ThisActor, OtherActor);
        Swap(ThisVelocity, OtherVelocity);
    }
};
