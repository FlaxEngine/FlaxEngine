// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PHYSX

#include "SimulationEventCallbackPhysX.h"
#include "Engine/Physics/Colliders/Collider.h"
#include "Engine/Physics/Joints/Joint.h"
#include "Engine/Physics/Actors/RigidBody.h"
#include <ThirdParty/PhysX/extensions/PxJoint.h>
#include <ThirdParty/PhysX/PxShape.h>

namespace
{
    void ClearColliderFromCollection(const PhysicsColliderActor* collider, Array<SimulationEventCallback::CollidersPair>& collection)
    {
        const auto c = collection.Get();
        for (int32 i = 0; i < collection.Count(); i++)
        {
            const SimulationEventCallback::CollidersPair cc = c[i];
            if (cc.First == collider || cc.Second == collider)
            {
                collection.RemoveAt(i--);
                if (collection.IsEmpty())
                    break;
            }
        }
    }
}

void SimulationEventCallback::Clear()
{
    NewCollisions.Clear();
    RemovedCollisions.Clear();

    NewTriggerPairs.Clear();
    LostTriggerPairs.Clear();

    BrokenJoints.Clear();
}

void SimulationEventCallback::SendCollisionEvents()
{
    for (auto& c : RemovedCollisions)
    {
        c.ThisActor->OnCollisionExit(c);
        c.SwapObjects();
        c.ThisActor->OnCollisionExit(c);
        c.SwapObjects();
    }
    for (auto& c : NewCollisions)
    {
        c.ThisActor->OnCollisionEnter(c);
        c.SwapObjects();
        c.ThisActor->OnCollisionEnter(c);
        c.SwapObjects();
    }
}

void SimulationEventCallback::SendTriggerEvents()
{
    for (const auto& c : LostTriggerPairs)
    {
        c.First->OnTriggerExit(c.Second);
        c.Second->OnTriggerExit(c.First);
    }
    for (const auto& c : NewTriggerPairs)
    {
        c.First->OnTriggerEnter(c.Second);
        c.Second->OnTriggerEnter(c.First);
    }
}

void SimulationEventCallback::SendJointEvents()
{
    for (auto* actor : BrokenJoints)
    {
        actor->OnJointBreak();
    }
}

void SimulationEventCallback::OnColliderRemoved(PhysicsColliderActor* collider)
{
    ClearColliderFromCollection(collider, NewTriggerPairs);
    ClearColliderFromCollection(collider, LostTriggerPairs);
}

void SimulationEventCallback::OnJointRemoved(Joint* joint)
{
    BrokenJoints.Remove(joint);
}

void SimulationEventCallback::onConstraintBreak(PxConstraintInfo* constraints, PxU32 count)
{
    for (uint32 i = 0; i < count; i++)
    {
        PxJoint* joint = reinterpret_cast<PxJoint*>(constraints[i].externalReference);
        if (joint->userData)
            BrokenJoints.Add(static_cast<Joint*>(joint->userData));
    }
}

void SimulationEventCallback::onWake(PxActor** actors, PxU32 count)
{
    // Not used
}

void SimulationEventCallback::onSleep(PxActor** actors, PxU32 count)
{
    // Not used
}

void SimulationEventCallback::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
{
    // Skip sending events to removed actors
    if (pairHeader.flags & (PxContactPairHeaderFlag::eREMOVED_ACTOR_0 | PxContactPairHeaderFlag::eREMOVED_ACTOR_1))
        return;

    Collision c;
    PxContactPairExtraDataIterator j(pairHeader.extraDataStream, pairHeader.extraDataStreamSize);

    // Extract collision pairs
    for (PxU32 pairIndex = 0; pairIndex < nbPairs; pairIndex++)
    {
        const PxContactPair& pair = pairs[pairIndex];
        PxContactStreamIterator i(pair.contactPatches, pair.contactPoints, pair.getInternalFaceIndices(), pair.patchCount, pair.contactCount);

        const PxReal* impulses = pair.contactImpulses;
        //const PxU32 flippedContacts = (pair.flags & PxContactPairFlag::eINTERNAL_CONTACTS_ARE_FLIPPED);
        const bool hasImpulses = pair.flags.isSet(PxContactPairFlag::eINTERNAL_HAS_IMPULSES);
        const bool hasPostVelocities = !pair.flags.isSet(PxContactPairFlag::eACTOR_PAIR_LOST_TOUCH);
        PxVec3 totalImpulse(0.0f);

        c.ThisActor = static_cast<PhysicsColliderActor*>(pair.shapes[0]->userData);
        c.OtherActor = static_cast<PhysicsColliderActor*>(pair.shapes[1]->userData);
        if (c.ThisActor == nullptr || c.OtherActor == nullptr)
        {
            // One of the actors was deleted (eg. via RigidBody destroyed by gameplay) then skip processing this collision
            continue;
        }

        // Extract contact points
        c.ContactsCount = 0;
        while (i.hasNextPatch())
        {
            i.nextPatch();
            while (i.hasNextContact() && c.ContactsCount < COLLISION_NAX_CONTACT_POINTS)
            {
                i.nextContact();
                const PxVec3 point = i.getContactPoint();
                const PxVec3 normal = i.getContactNormal();
                if (hasImpulses)
                    totalImpulse += normal * impulses[c.ContactsCount];

                ContactPoint& contact = c.Contacts[c.ContactsCount++];
                contact.Point = P2C(point);
                contact.Normal = P2C(normal);
                contact.Separation = i.getSeparation();
            }
        }
        c.Impulse = P2C(totalImpulse);

        // Extract velocities
        c.ThisVelocity = c.OtherVelocity = Vector3::Zero;
        if (hasPostVelocities && j.nextItemSet())
        {
            ASSERT_LOW_LAYER(j.contactPairIndex == pairIndex);
            if (j.postSolverVelocity)
            {
                c.ThisVelocity = P2C(j.postSolverVelocity->linearVelocity[0]);
                c.OtherVelocity = P2C(j.postSolverVelocity->linearVelocity[1]);
            }
        }

        if (pair.flags & PxContactPairFlag::eACTOR_PAIR_HAS_FIRST_TOUCH)
        {
            NewCollisions.Add(c);
        }
        else if (pair.flags & PxContactPairFlag::eACTOR_PAIR_LOST_TOUCH)
        {
            RemovedCollisions.Add(c);
        }
    }
    //ASSERT(!j.nextItemSet());
}

void SimulationEventCallback::onTrigger(PxTriggerPair* pairs, PxU32 count)
{
    for (PxU32 i = 0; i < count; i++)
    {
        const PxTriggerPair& pair = pairs[i];

        // Ignore pairs when shapes have been deleted
        if (pair.flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
            continue;

        auto trigger = static_cast<PhysicsColliderActor*>(pair.triggerShape->userData);
        auto otherCollider = static_cast<PhysicsColliderActor*>(pair.otherShape->userData);
        ASSERT(trigger && otherCollider);
        CollidersPair collidersPair(trigger, otherCollider);

        if (pair.status & PxPairFlag::eNOTIFY_TOUCH_LOST)
        {
            LostTriggerPairs.Add(collidersPair);
        }
        else
        {
            NewTriggerPairs.Add(collidersPair);
        }
    }
}

void SimulationEventCallback::onAdvance(const PxRigidBody* const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count)
{
    // Not used
}

#endif
