// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Physics.h"
#include "Utilities.h"
#include "CollisionData.h"
#include "PhysicsScene.h"
#include "Actors/PhysicsColliderActor.h"

bool Physics::RayCast(const Vector3& origin, const Vector3& direction, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->RayCast(origin, direction, maxDistance, layerMask, hitTriggers);
}

bool Physics::RayCast(const Vector3& origin, const Vector3& direction, RayCastHit& hitInfo, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->RayCast(origin, direction, hitInfo, maxDistance, layerMask, hitTriggers);
}

bool Physics::RayCastAll(const Vector3& origin, const Vector3& direction, Array<RayCastHit>& results, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->RayCastAll(origin, direction, results, maxDistance, layerMask, hitTriggers);
}

bool Physics::BoxCast(const Vector3& center, const Vector3& halfExtents, const Vector3& direction, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->BoxCast(center, halfExtents, direction, rotation, maxDistance, layerMask, hitTriggers);
}

bool Physics::BoxCast(const Vector3& center, const Vector3& halfExtents, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->BoxCast(center, halfExtents, direction, hitInfo, rotation, maxDistance, layerMask, hitTriggers);
}

bool Physics::BoxCastAll(const Vector3& center, const Vector3& halfExtents, const Vector3& direction, Array<RayCastHit>& results, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->BoxCastAll(center, halfExtents, direction, results, rotation, maxDistance, layerMask, hitTriggers);
}

bool Physics::SphereCast(const Vector3& center, const float radius, const Vector3& direction, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->SphereCast(center, radius, direction, maxDistance, layerMask, hitTriggers);
}

bool Physics::SphereCast(const Vector3& center, const float radius, const Vector3& direction, RayCastHit& hitInfo, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->SphereCast(center, radius, direction, hitInfo, maxDistance, layerMask, hitTriggers);
}

bool Physics::SphereCastAll(const Vector3& center, const float radius, const Vector3& direction, Array<RayCastHit>& results, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->SphereCastAll(center, radius, direction, results, maxDistance, layerMask, hitTriggers);
}

bool Physics::CapsuleCast(const Vector3& center, const float radius, const float height, const Vector3& direction, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->CapsuleCast(center, radius, height, direction, rotation, maxDistance, layerMask, hitTriggers);
}

bool Physics::CapsuleCast(const Vector3& center, const float radius, const float height, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->CapsuleCast(center, radius, height, direction, hitInfo, rotation, maxDistance, layerMask, hitTriggers);
}

bool Physics::CapsuleCastAll(const Vector3& center, const float radius, const float height, const Vector3& direction, Array<RayCastHit>& results, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->CapsuleCastAll(center, radius, height, direction, results, rotation, maxDistance, layerMask, hitTriggers);
}

bool Physics::ConvexCast(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->ConvexCast(center, convexMesh, scale, direction, rotation, maxDistance, layerMask, hitTriggers);
}

bool Physics::ConvexCast(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->ConvexCast(center, convexMesh, scale, direction, hitInfo, rotation, maxDistance, layerMask, hitTriggers);
}

bool Physics::ConvexCastAll(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, Array<RayCastHit>& results, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->ConvexCastAll(center, convexMesh, scale, direction, results, rotation, maxDistance, layerMask, hitTriggers);
}

bool Physics::CheckBox(const Vector3& center, const Vector3& halfExtents, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->CheckBox(center, halfExtents, rotation, layerMask, hitTriggers);
}

bool Physics::CheckSphere(const Vector3& center, const float radius, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->CheckSphere(center, radius, layerMask, hitTriggers);
}

bool Physics::CheckCapsule(const Vector3& center, const float radius, const float height, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->CheckCapsule(center, radius, height, rotation, layerMask, hitTriggers);
}

bool Physics::CheckConvex(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->CheckConvex(center, convexMesh, scale, rotation, layerMask, hitTriggers);
}

bool Physics::OverlapBox(const Vector3& center, const Vector3& halfExtents, Array<Collider*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->OverlapBox(center, halfExtents, results, rotation, layerMask, hitTriggers);
}

bool Physics::OverlapSphere(const Vector3& center, const float radius, Array<Collider*>& results, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->OverlapSphere(center, radius, results, layerMask, hitTriggers);
}

bool Physics::OverlapCapsule(const Vector3& center, const float radius, const float height, Array<Collider*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->OverlapCapsule(center, radius, height, results, rotation, layerMask, hitTriggers);
}

bool Physics::OverlapConvex(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, Array<Collider*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->OverlapConvex(center, convexMesh, scale, results, rotation, layerMask, hitTriggers);
}

bool Physics::OverlapBox(const Vector3& center, const Vector3& halfExtents, Array<PhysicsColliderActor*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->OverlapBox(center, halfExtents, results, rotation, layerMask, hitTriggers);
}

bool Physics::OverlapSphere(const Vector3& center, const float radius, Array<PhysicsColliderActor*>& results, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->OverlapSphere(center, radius, results, layerMask, hitTriggers);
}

bool Physics::OverlapCapsule(const Vector3& center, const float radius, const float height, Array<PhysicsColliderActor*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->OverlapCapsule(center, radius, height, results, rotation, layerMask, hitTriggers);
}

bool Physics::OverlapConvex(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, Array<PhysicsColliderActor*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->OverlapConvex(center, convexMesh, scale, results, rotation, layerMask, hitTriggers);
}
