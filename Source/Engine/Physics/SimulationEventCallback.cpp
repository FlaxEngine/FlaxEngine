// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "SimulationEventCallback.h"
#include "Utilities.h"
#include "FlaxEngine.Gen.h"
#include "Colliders/Collider.h"
#include "Joints/Joint.h"
#include "Actors/RigidBody.h"
#include "Engine/Core/Log.h"
#include <ThirdParty/PhysX/extensions/PxJoint.h>
#include <ThirdParty/PhysX/PxShape.h>

namespace
{
    void ClearColliderFromCollection(PhysicsColliderActor* collider, Array<SimulationEventCallback::CollidersPair>& collection)
    {
        for (int32 i = 0; i < collection.Count(); i++)
        {
            if (collection[i].First == collider || collection[i].Second == collider)
            {
                collection.RemoveAt(i--);
                if (collection.IsEmpty())
                    break;
            }
        }
    }

    void ClearColliderFromCollection(PhysicsColliderActor* collider, SimulationEventCallback::CollisionsPool& collection)
    {
        for (auto i = collection.Begin(); i.IsNotEnd(); ++i)
        {
            if (i->Key.First == collider || i->Key.Second == collider)
            {
                collection.Remove(i);
                if (collection.IsEmpty())
                    break;
            }
        }
    }
}

void SimulationEventCallback::CollectResults()
{
    // Generate new collisions
    for (auto i = Collisions.Begin(); i.IsNotEnd(); ++i)
    {
        if (!PrevCollisions.ContainsKey(i->Key))
            NewCollisions.Add(i->Key);
    }

    // Generate removed collisions
    for (auto i = PrevCollisions.Begin(); i.IsNotEnd(); ++i)
    {
        if (!Collisions.ContainsKey(i->Key))
            RemovedCollisions.Add(i->Key);
    }
}

void SimulationEventCallback::SendCollisionEvents()
{
    for (int32 i = 0; i < RemovedCollisions.Count(); i++)
    {
        const auto pair = RemovedCollisions[i];
        auto& c = PrevCollisions[pair];

        pair.First->OnCollisionExit(c);

        c.SwapObjects();
        pair.Second->OnCollisionExit(c);
        c.SwapObjects();
    }
    for (int32 i = 0; i < NewCollisions.Count(); i++)
    {
        const auto pair = NewCollisions[i];
        auto& c = Collisions[pair];

        pair.First->OnCollisionEnter(c);
        c.SwapObjects();
        pair.Second->OnCollisionEnter(c);
        c.SwapObjects();
    }
}

void SimulationEventCallback::SendTriggerEvents()
{
    for (int32 i = 0; i < LostTriggerPairs.Count(); i++)
    {
        const auto c = LostTriggerPairs[i];
        c.First->OnTriggerExit(c.Second);
        c.Second->OnTriggerExit(c.First);
    }
    for (int32 i = 0; i < NewTriggerPairs.Count(); i++)
    {
        const auto c = NewTriggerPairs[i];
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
    ClearColliderFromCollection(collider, Collisions);
    ClearColliderFromCollection(collider, PrevCollisions);
    ClearColliderFromCollection(collider, RemovedCollisions);
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
    {
        return;
    }

    Collision c;
    c.ThisVelocity = c.OtherVelocity = Vector3::Zero;

    // Extract collision pairs
    for (PxU32 pairIndex = 0; pairIndex < nbPairs; pairIndex++)
    {
        const PxContactPair& pair = pairs[pairIndex];
        PxContactStreamIterator i(pair.contactPatches, pair.contactPoints, pair.getInternalFaceIndices(), pair.patchCount, pair.contactCount);

        const PxReal* impulses = pair.contactImpulses;
        //const PxU32 flippedContacts = (pair.flags & PxContactPairFlag::eINTERNAL_CONTACTS_ARE_FLIPPED);
        const PxU32 hasImpulses = (pair.flags & PxContactPairFlag::eINTERNAL_HAS_IMPULSES);
        PxU32 nbContacts = 0;
        PxVec3 totalImpulse = PxVec3(0.0f);

        c.ThisActor = static_cast<PhysicsColliderActor*>(pair.shapes[0]->userData);
        c.OtherActor = static_cast<PhysicsColliderActor*>(pair.shapes[1]->userData);

        while (i.hasNextPatch())
        {
            i.nextPatch();
            while (i.hasNextContact() && nbContacts < COLLISION_NAX_CONTACT_POINTS)
            {
                i.nextContact();

                const PxVec3 point = i.getContactPoint();
                const PxVec3 normal = i.getContactNormal();
                if (hasImpulses)
                    totalImpulse += normal * impulses[nbContacts];

                //PxU32 internalFaceIndex0 = flippedContacts ? iter.getFaceIndex1() : iter.getFaceIndex0();
                //PxU32 internalFaceIndex1 = flippedContacts ? iter.getFaceIndex0() : iter.getFaceIndex1();

                ContactPoint& contact = c.Contacts[nbContacts];
                contact.Point = P2C(point);
                contact.Normal = P2C(normal);
                contact.Separation = i.getSeparation();

                nbContacts++;
            }
        }

        c.ContactsCount = nbContacts;
        c.Impulse = P2C(totalImpulse);
        Collisions[CollidersPair(c.ThisActor, c.OtherActor)] = c;
    }

    // Extract velocities
    PxContactPairExtraDataIterator i(pairHeader.extraDataStream, pairHeader.extraDataStreamSize);
    while (i.nextItemSet())
    {
        if (i.postSolverVelocity)
        {
            const PxVec3 linearVelocityActor0 = i.postSolverVelocity->linearVelocity[0];
            const PxVec3 linearVelocityActor1 = i.postSolverVelocity->linearVelocity[1];

            const PxContactPair& pair = pairs[i.contactPairIndex];
            c.ThisActor = static_cast<PhysicsColliderActor*>(pair.shapes[0]->userData);
            c.OtherActor = static_cast<PhysicsColliderActor*>(pair.shapes[1]->userData);
            Collision& collision = Collisions[CollidersPair(c.ThisActor, c.OtherActor)];

            collision.ThisVelocity = P2C(linearVelocityActor0);
            collision.OtherVelocity = P2C(linearVelocityActor1);
        }
    }
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
