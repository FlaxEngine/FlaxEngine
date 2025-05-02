// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_PHYSX

#include "Types.h"
#include "Engine/Physics/Collisions.h"
#include "Engine/Physics/Colliders/Collider.h"
#include "Engine/Core/Types/Pair.h"
#include <ThirdParty/PhysX/PxSimulationEventCallback.h>

/// <summary>
/// Default implementation of the PxSimulationEventCallback to send physics events to the other engine services.
/// </summary>
class SimulationEventCallback : public PxSimulationEventCallback
{
public:
    typedef Pair<PhysicsColliderActor*, PhysicsColliderActor*> CollidersPair;

    /// <summary>
    /// The new collisions (for enter event).
    /// </summary>
    Array<Collision> NewCollisions;

    /// <summary>
    /// The old collisions (for exit event).
    /// </summary>
    Array<Collision> RemovedCollisions;

    /// <summary>
    /// The new trigger pairs (for enter event).
    /// </summary>
    Array<CollidersPair> NewTriggerPairs;

    /// <summary>
    /// The removed trigger pairs (for exit event).
    /// </summary>
    Array<CollidersPair> LostTriggerPairs;

    /// <summary>
    /// The broken joints collection.
    /// </summary>
    Array<Joint*> BrokenJoints;

public:
    /// <summary>
    /// Clears the data.
    /// </summary>
    void Clear();

    /// <summary>
    /// Sends the collision events to the managed objects.
    /// </summary>
    void SendCollisionEvents();

    /// <summary>
    /// Sends the trigger events to the managed objects.
    /// </summary>
    void SendTriggerEvents();

    /// <summary>
    /// Sends the joint events to the managed objects.
    /// </summary>
    void SendJointEvents();

    /// <summary>
    /// Called when collider gets removed so all cached events should be removed for this object.
    /// Prevents sending events and using deleted objects.
    /// </summary>
    /// <param name="collider">The collider.</param>
    void OnColliderRemoved(PhysicsColliderActor* collider);

    /// <summary>
    /// Called when joint gets removed so all cached events should be removed for this object.
    /// Prevents sending events and using deleted objects.
    /// </summary>
    /// <param name="joint">The joint.</param>
    void OnJointRemoved(Joint* joint);

public:
    // [PxSimulationEventCallback]
    void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override;
    void onWake(PxActor** actors, PxU32 count) override;
    void onSleep(PxActor** actors, PxU32 count) override;
    void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;
    void onTrigger(PxTriggerPair* pairs, PxU32 count) override;
    void onAdvance(const PxRigidBody* const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) override;
};

#endif
