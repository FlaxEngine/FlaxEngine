// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "PhysicsActor.h"
#include "../PhysicsBackend.h"

PhysicsActor::PhysicsActor(const SpawnParams& params)
    : Actor(params)
    , _cachedScale(1.0f)
    , _isUpdatingTransform(false)
{
}

void PhysicsActor::OnActiveTransformChanged()
{
    // Change actor transform (but with locking)
    ASSERT(!_isUpdatingTransform);
    _isUpdatingTransform = true;
    Transform transform;
    PhysicsBackend::GetRigidActorPose(GetPhysicsActor(), transform.Translation, transform.Orientation);
    transform.Scale = _transform.Scale;
    if (_parent)
    {
        _parent->GetTransform().WorldToLocal(transform, _localTransform);
    }
    else
    {
        _localTransform = transform;
    }
    OnTransformChanged();
    _isUpdatingTransform = false;
}

void PhysicsActor::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    UpdateBounds();
}

void PhysicsActor::UpdateBounds()
{
    void* actor = GetPhysicsActor();
    if (actor)
        PhysicsBackend::GetActorBounds(actor, _box);
    else
        _box = BoundingBox(_transform.Translation);
    BoundingSphere::FromBox(_box, _sphere);
}

bool PhysicsActor::IntersectsItself(const Ray& ray, float& distance, Vector3& normal)
{
    return _box.Intersects(ray, distance, normal);
}
