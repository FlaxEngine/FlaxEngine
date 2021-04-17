// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "PhysicsActor.h"
#include "Engine/Physics/Utilities.h"
#include "Engine/Physics/Physics.h"
#include <ThirdParty/PhysX/PxActor.h>
#include <ThirdParty/PhysX/PxRigidActor.h>

PhysicsActor::PhysicsActor(const SpawnParams& params)
    : Actor(params)
    , _cachedScale(1.0f)
    , _isUpdatingTransform(false)
{
}

void PhysicsActor::OnActiveTransformChanged(const PxTransform& transform)
{
    // Change actor transform (but with locking)
    ASSERT(!_isUpdatingTransform);
    _isUpdatingTransform = true;
    //SetTransform(Transform(P2C(transform.p), P2C(transform.q), GetScale()));
    {
        Transform v;
        v.Translation = P2C(transform.p);
        v.Orientation = P2C(transform.q);
        v.Scale = _transform.Scale;

        if (_parent)
        {
            _parent->GetTransform().WorldToLocal(v, _localTransform);
        }
        else
        {
            _localTransform = v;
        }
        OnTransformChanged();
    }
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
    const auto actor = GetPhysXActor();
    const float boundsScale = 1.02f;
    if (actor && actor->getScene() != nullptr)
    {
        if (actor->is<PxRigidActor>())
        {
            const auto rigidActor = (PxRigidActor*)actor;
            if (rigidActor->getNbShapes() != 0)
            {
                _box = P2C(actor->getWorldBounds(boundsScale));
            }
            else
            {
                _box = BoundingBox(_transform.Translation);
            }
        }
        else
        {
            _box = P2C(actor->getWorldBounds(boundsScale));
        }
    }
    else
    {
        _box = BoundingBox(_transform.Translation);
    }
    BoundingSphere::FromBox(_box, _sphere);
}

bool PhysicsActor::IntersectsItself(const Ray& ray, float& distance, Vector3& normal)
{
    return _box.Intersects(ray, distance, normal);
}
