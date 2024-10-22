// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "PhysicsColliderActor.h"
#include "RigidBody.h"

PhysicsColliderActor::PhysicsColliderActor(const SpawnParams& params)
    : Actor(params)
{
    _name = TEXT("Physics Collider Actor");
}

void PhysicsColliderActor::OnCollisionEnter(const Collision& c)
{
    CollisionEnter(c);

    auto rigidBody = GetAttachedRigidBody();
    if (rigidBody)
        rigidBody->OnCollisionEnter(c);
}

void PhysicsColliderActor::OnCollisionExit(const Collision& c)
{
    CollisionExit(c);

    auto rigidBody = GetAttachedRigidBody();
    if (rigidBody)
        rigidBody->OnCollisionExit(c);
}

void PhysicsColliderActor::OnTriggerEnter(PhysicsColliderActor* c)
{
    TriggerEnter(c);

    auto rigidBody = GetAttachedRigidBody();
    if (rigidBody)
        rigidBody->OnTriggerEnter(c);
}

void PhysicsColliderActor::OnTriggerExit(PhysicsColliderActor* c)
{
    TriggerExit(c);

    auto rigidBody = GetAttachedRigidBody();
    if (rigidBody)
        rigidBody->OnTriggerExit(c);
}
