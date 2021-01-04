// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "Collisions.h"
#include "Engine/Core/Types/Pair.h"
#include "Colliders/Collider.h"
#include <ThirdParty/PhysX/PxSimulationEventCallback.h>

/// <summary>
/// Default implementation of the PxSimulationEventCallback to send physics events to the other engine services.
/// </summary>
/// <seealso cref="PxSimulationEventCallback" />
class SimulationEventCallback : public PxSimulationEventCallback
{
public:

    typedef Pair<PhysicsColliderActor*, PhysicsColliderActor*> CollidersPair;
    typedef Dictionary<CollidersPair, Collision> CollisionsPool;

    /// <summary>
    /// The collected collisions.
    /// </summary>
    CollisionsPool Collisions;

    /// <summary>
    /// The previous step collisions.
    /// </summary>
    CollisionsPool PrevCollisions;

    /// <summary>
    /// The new collisions (for enter event).
    /// </summary>
    Array<CollidersPair> NewCollisions;

    /// <summary>
    /// The old collisions (for exit event).
    /// </summary>
    Array<CollidersPair> RemovedCollisions;

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
    void Clear()
    {
        PrevCollisions = Collisions;
        Collisions.Clear();

        NewCollisions.Clear();
        RemovedCollisions.Clear();

        NewTriggerPairs.Clear();
        LostTriggerPairs.Clear();

        BrokenJoints.Clear();
    }

    /// <summary>
    /// Generates the new/old/removed collisions and a valid trigger pairs.
    /// </summary>
    void CollectResults();

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
