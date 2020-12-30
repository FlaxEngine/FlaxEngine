// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "Physics.h"
#include "Utilities.h"
#include "Actors/PhysicsColliderActor.h"
#include <ThirdParty/PhysX/PxScene.h>
#include <ThirdParty/PhysX/PxQueryFiltering.h>

// Temporary result buffer size
#define HIT_BUFFER_SIZE	128

template<typename HitType>
class DynamicHitBuffer : public PxHitCallback<HitType>
{
private:

    uint32 _count;
    HitType _buffer[HIT_BUFFER_SIZE];

public:

    DynamicHitBuffer()
        : PxHitCallback<HitType>(_buffer, HIT_BUFFER_SIZE)
        , _count(0)
    {
    }

public:

    // Computes the number of any hits in this result, blocking or touching.
    PX_INLINE PxU32 getNbAnyHits() const
    {
        return getNbTouches();
    }

    // Convenience iterator used to access any hits in this result, blocking or touching.
    PX_INLINE const HitType& getAnyHit(const PxU32 index) const
    {
        PX_ASSERT(index < getNbTouches() + PxU32(this->hasBlock));
        return index < getNbTouches() ? getTouches()[index] : this->block;
    }

    PX_INLINE PxU32 getNbTouches() const
    {
        return _count;
    }

    PX_INLINE const HitType* getTouches() const
    {
        return _buffer;
    }

    PX_INLINE const HitType& getTouch(const PxU32 index) const
    {
        PX_ASSERT(index < getNbTouches());
        return _buffer[index];
    }

    PX_INLINE PxU32 getMaxNbTouches() const
    {
        return HIT_BUFFER_SIZE;
    }

protected:

    PxAgain processTouches(const HitType* buffer, PxU32 nbHits) override
    {
        nbHits = Math::Min(nbHits, HIT_BUFFER_SIZE - _count);
        for (PxU32 i = 0; i < nbHits; i++)
        {
            _buffer[_count + i] = buffer[i];
        }
        _count += nbHits;
        return true;
    }

    void finalizeQuery() override
    {
        if (this->hasBlock)
        {
            // Blocking hits go to hits
            processTouches(&this->block, 1);
        }
    }
};

#define SCENE_QUERY_SETUP(blockSingle) if (GetScene() == nullptr) return false; \
		const PxHitFlags hitFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eUV; \
		PxQueryFilterData filterData = PxQueryFilterData(); \
		filterData.flags |=  PxQueryFlag::ePREFILTER; \
		filterData.data.word0 = layerMask; \
		filterData.data.word1 = blockSingle ? 1 : 0; \
		filterData.data.word2 = hitTriggers ? 1 : 0

#define SCENE_QUERY_SETUP_SWEEP_1() SCENE_QUERY_SETUP(true); \
		PxSweepBufferN<1> buffer

#define SCENE_QUERY_SETUP_SWEEP() SCENE_QUERY_SETUP(false); \
		DynamicHitBuffer<PxSweepHit> buffer

#define SCENE_QUERY_SETUP_OVERLAP_1() SCENE_QUERY_SETUP(false); \
		PxOverlapBufferN<1> buffer

#define SCENE_QUERY_SETUP_OVERLAP() SCENE_QUERY_SETUP(false); \
		DynamicHitBuffer<PxOverlapHit> buffer

#define SCENE_QUERY_COLLECT_SINGLE() const auto& hit = buffer.getAnyHit(0); \
		hitInfo.Gather(hit)

#define SCENE_QUERY_COLLECT_ALL() results.Clear(); \
		results.Resize(buffer.getNbAnyHits(), false); \
		for (int32 i = 0; i < results.Count(); i++) \
		{ \
			const auto& hit = buffer.getAnyHit(i); \
			results[i].Gather(hit); \
		}

#define SCENE_QUERY_COLLECT_OVERLAP() results.Clear();  \
		results.Resize(buffer.getNbTouches(), false); \
		for (int32 i = 0; i < results.Count(); i++) \
		{ \
			auto& hitInfo = results[i]; \
			const auto& hit = buffer.getTouch(i); \
			hitInfo = hit.shape ? static_cast<::PhysicsColliderActor*>(hit.shape->userData) : nullptr; \
		}

#define SCENE_QUERY_COLLECT_OVERLAP_COLLIDER() results.Clear();  \
		results.Resize(buffer.getNbTouches(), false); \
		for (int32 i = 0; i < results.Count(); i++) \
		{ \
			auto& hitInfo = results[i]; \
			const auto& hit = buffer.getTouch(i); \
			hitInfo = hit.shape ? static_cast<::Collider*>(hit.shape->userData) : nullptr; \
		}

void RayCastHit::Gather(const PxRaycastHit& hit)
{
    Point = P2C(hit.position);
    Normal = P2C(hit.normal);
    Distance = hit.distance;
    Collider = hit.shape ? static_cast<PhysicsColliderActor*>(hit.shape->userData) : nullptr;
    UV.X = hit.u;
    UV.Y = hit.v;
}

void RayCastHit::Gather(const PxSweepHit& hit)
{
    Point = P2C(hit.position);
    Normal = P2C(hit.normal);
    Distance = hit.distance;
    Collider = hit.shape ? static_cast<PhysicsColliderActor*>(hit.shape->userData) : nullptr;
    UV = Vector2::Zero;
}

class QueryFilter : public PxQueryFilterCallback
{
public:

    PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags) override
    {
        // Early out to avoid crashing
        if (!shape)
            return PxQueryHitType::eNONE;

        // Check mask
        const PxFilterData shapeFilter = shape->getQueryFilterData();
        if ((filterData.word0 & shapeFilter.word0) == 0)
        {
            return PxQueryHitType::eNONE;
        }

        // Check if skip triggers
        const bool hitTriggers = filterData.word2 != 0;
        if (!hitTriggers && shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE)
            return PxQueryHitType::eNONE;

        const bool blockSingle = filterData.word1 != 0;
        return blockSingle ? PxQueryHitType::eBLOCK : PxQueryHitType::eTOUCH;
    }

    PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit) override
    {
        // Not used
        return PxQueryHitType::eNONE;
    }
};

class CharacterQueryFilter : public PxQueryFilterCallback
{
public:

    PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags) override
    {
        // Early out to avoid crashing
        if (!shape)
            return PxQueryHitType::eNONE;

        // Let triggers through
        if (PxFilterObjectIsTrigger(shape->getFlags()))
            return PxQueryHitType::eNONE;

        // Trigger the contact callback for pairs (A,B) where the filtermask of A contains the ID of B and vice versa
        const PxFilterData shapeFilter = shape->getQueryFilterData();
        if (filterData.word0 & shapeFilter.word1)
            return PxQueryHitType::eBLOCK;

        return PxQueryHitType::eNONE;
    }

    PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit) override
    {
        // Not used
        return PxQueryHitType::eNONE;
    }
};

PxQueryFilterCallback* Physics::GetQueryFilterCallback()
{
    static QueryFilter Filter;
    return &Filter;
}

PxQueryFilterCallback* Physics::GetCharacterQueryFilterCallback()
{
    static CharacterQueryFilter Filter;
    return &Filter;
}

bool Physics::RayCast(const Vector3& origin, const Vector3& direction, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    // Prepare data
    SCENE_QUERY_SETUP(true);
    PxRaycastBuffer buffer;

    // Perform raycast test
    return GetScene()->raycast(C2P(origin), C2P(direction), maxDistance, buffer, hitFlags, filterData, GetQueryFilterCallback());
}

bool Physics::RayCast(const Vector3& origin, const Vector3& direction, RayCastHit& hitInfo, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    // Prepare data
    SCENE_QUERY_SETUP(true);
    PxRaycastBuffer buffer;

    // Perform raycast test
    if (!GetScene()->raycast(C2P(origin), C2P(direction), maxDistance, buffer, hitFlags, filterData, GetQueryFilterCallback()))
        return false;

    // Collect results
    SCENE_QUERY_COLLECT_SINGLE();

    return true;
}

bool Physics::RayCastAll(const Vector3& origin, const Vector3& direction, Array<RayCastHit>& results, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    // Prepare data
    SCENE_QUERY_SETUP(false);
    DynamicHitBuffer<PxRaycastHit> buffer;

    // Perform raycast test
    if (!GetScene()->raycast(C2P(origin), C2P(direction), maxDistance, buffer, hitFlags, filterData, GetQueryFilterCallback()))
        return false;

    // Collect results
    SCENE_QUERY_COLLECT_ALL();

    return true;
}

bool Physics::BoxCast(const Vector3& center, const Vector3& halfExtents, const Vector3& direction, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    // Prepare data
    SCENE_QUERY_SETUP_SWEEP_1();
    const PxTransform pose(C2P(center), C2P(rotation));
    const PxBoxGeometry geometry(C2P(halfExtents));

    // Perform sweep test
    return GetScene()->sweep(geometry, pose, C2P(direction), maxDistance, buffer, hitFlags, filterData, GetQueryFilterCallback());
}

bool Physics::BoxCast(const Vector3& center, const Vector3& halfExtents, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    // Prepare data
    SCENE_QUERY_SETUP_SWEEP_1();
    const PxTransform pose(C2P(center), C2P(rotation));
    const PxBoxGeometry geometry(C2P(halfExtents));

    // Perform sweep test
    if (!GetScene()->sweep(geometry, pose, C2P(direction), maxDistance, buffer, hitFlags, filterData, GetQueryFilterCallback()))
        return false;

    // Collect results
    SCENE_QUERY_COLLECT_SINGLE();

    return true;
}

bool Physics::BoxCastAll(const Vector3& center, const Vector3& halfExtents, const Vector3& direction, Array<RayCastHit>& results, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    // Prepare data
    SCENE_QUERY_SETUP_SWEEP();
    const PxTransform pose(C2P(center), C2P(rotation));
    const PxBoxGeometry geometry(C2P(halfExtents));

    // Perform sweep test
    if (!GetScene()->sweep(geometry, pose, C2P(direction), maxDistance, buffer, hitFlags, filterData, GetQueryFilterCallback()))
        return false;

    // Collect results
    SCENE_QUERY_COLLECT_ALL();

    return true;
}

bool Physics::SphereCast(const Vector3& center, const float radius, const Vector3& direction, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    // Prepare data
    SCENE_QUERY_SETUP_SWEEP_1();
    const PxTransform pose(C2P(center));
    const PxSphereGeometry geometry(radius);

    // Perform sweep test
    return GetScene()->sweep(geometry, pose, C2P(direction), maxDistance, buffer, hitFlags, filterData, GetQueryFilterCallback());
}

bool Physics::SphereCast(const Vector3& center, const float radius, const Vector3& direction, RayCastHit& hitInfo, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    // Prepare data
    SCENE_QUERY_SETUP_SWEEP_1();
    const PxTransform pose(C2P(center));
    const PxSphereGeometry geometry(radius);

    // Perform sweep test
    if (!GetScene()->sweep(geometry, pose, C2P(direction), maxDistance, buffer, hitFlags, filterData, GetQueryFilterCallback()))
        return false;

    // Collect results
    SCENE_QUERY_COLLECT_SINGLE();

    return true;
}

bool Physics::SphereCastAll(const Vector3& center, const float radius, const Vector3& direction, Array<RayCastHit>& results, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    // Prepare data
    SCENE_QUERY_SETUP_SWEEP();
    const PxTransform pose(C2P(center));
    const PxSphereGeometry geometry(radius);

    // Perform sweep test
    if (!GetScene()->sweep(geometry, pose, C2P(direction), maxDistance, buffer, hitFlags, filterData, GetQueryFilterCallback()))
        return false;

    // Collect results
    SCENE_QUERY_COLLECT_ALL();

    return true;
}

bool Physics::CheckBox(const Vector3& center, const Vector3& halfExtents, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    // Prepare data
    SCENE_QUERY_SETUP_OVERLAP_1();
    const PxTransform pose(C2P(center), C2P(rotation));
    const PxBoxGeometry geometry(C2P(halfExtents));

    // Perform overlap test
    return GetScene()->overlap(geometry, pose, buffer, filterData, GetQueryFilterCallback());
}

bool Physics::CheckSphere(const Vector3& center, const float radius, uint32 layerMask, bool hitTriggers)
{
    // Prepare data
    SCENE_QUERY_SETUP_OVERLAP_1();
    const PxTransform pose(C2P(center));
    const PxSphereGeometry geometry(radius);

    // Perform overlap test
    return GetScene()->overlap(geometry, pose, buffer, filterData, GetQueryFilterCallback());
}

bool Physics::OverlapBox(const Vector3& center, const Vector3& halfExtents, Array<Collider*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    // Prepare data
    SCENE_QUERY_SETUP_OVERLAP();
    const PxTransform pose(C2P(center), C2P(rotation));
    const PxBoxGeometry geometry(C2P(halfExtents));

    // Perform overlap test
    if (!GetScene()->overlap(geometry, pose, buffer, filterData, GetQueryFilterCallback()))
        return false;

    // Collect results
    SCENE_QUERY_COLLECT_OVERLAP_COLLIDER();

    return true;
}

bool Physics::OverlapSphere(const Vector3& center, const float radius, Array<Collider*>& results, uint32 layerMask, bool hitTriggers)
{
    // Prepare data
    SCENE_QUERY_SETUP_OVERLAP();
    const PxTransform pose(C2P(center));
    const PxSphereGeometry geometry(radius);

    // Perform overlap test
    if (!GetScene()->overlap(geometry, pose, buffer, filterData, GetQueryFilterCallback()))
        return false;

    // Collect results
    SCENE_QUERY_COLLECT_OVERLAP_COLLIDER();

    return true;
}

bool Physics::OverlapBox(const Vector3& center, const Vector3& halfExtents, Array<PhysicsColliderActor*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    // Prepare data
    SCENE_QUERY_SETUP_OVERLAP();
    const PxTransform pose(C2P(center), C2P(rotation));
    const PxBoxGeometry geometry(C2P(halfExtents));

    // Perform overlap test
    if (!GetScene()->overlap(geometry, pose, buffer, filterData, GetQueryFilterCallback()))
        return false;

    // Collect results
    SCENE_QUERY_COLLECT_OVERLAP();

    return true;
}

bool Physics::OverlapSphere(const Vector3& center, const float radius, Array<PhysicsColliderActor*>& results, uint32 layerMask, bool hitTriggers)
{
    // Prepare data
    SCENE_QUERY_SETUP_OVERLAP();
    const PxTransform pose(C2P(center));
    const PxSphereGeometry geometry(radius);

    // Perform overlap test
    if (!GetScene()->overlap(geometry, pose, buffer, filterData, GetQueryFilterCallback()))
        return false;

    // Collect results
    SCENE_QUERY_COLLECT_OVERLAP();

    return true;
}
