// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Collider.h"
#include "Engine/Core/Log.h"
#if USE_EDITOR
#include "Engine/Level/Scene/SceneRendering.h"
#endif
#include "Engine/Serialization/Serialization.h"
#include "Engine/Physics/Utilities.h"
#include "Engine/Physics/PhysicsSettings.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/PhysicalMaterial.h"
#include "Engine/Physics/Actors/RigidBody.h"
#include <ThirdParty/PhysX/geometry/PxGeometryQuery.h>
#include <ThirdParty/PhysX/PxShape.h>
#include <ThirdParty/PhysX/PxPhysics.h>
#include <ThirdParty/PhysX/PxFiltering.h>
#include <ThirdParty/PhysX/PxRigidDynamic.h>
#include <ThirdParty/PhysX/PxRigidStatic.h>
#include <ThirdParty/PhysX/PxScene.h>

Collider::Collider(const SpawnParams& params)
    : PhysicsColliderActor(params)
    , _center(Vector3::Zero)
    , _isTrigger(false)
    , _shape(nullptr)
    , _staticActor(nullptr)
    , _cachedScale(1.0f)
    , _contactOffset(10.0f)
{
    Material.Changed.Bind<Collider, &Collider::OnMaterialChanged>(this);
}

void Collider::SetIsTrigger(bool value)
{
    if (value == _isTrigger || !CanBeTrigger())
        return;

    _isTrigger = value;

    if (_shape)
    {
        const bool isTrigger = _isTrigger && CanBeTrigger();
        const PxShapeFlags shapeFlags = GetShapeFlags(isTrigger, IsActiveInHierarchy());
        _shape->setFlags(shapeFlags);
    }
}

void Collider::SetCenter(const Vector3& value)
{
    if (Vector3::NearEqual(value, _center))
        return;

    _center = value;

    if (_staticActor)
    {
        _shape->setLocalPose(PxTransform(C2P(_center)));
    }
    else if (const RigidBody* rigidBody = GetAttachedRigidBody())
    {
        _shape->setLocalPose(PxTransform(C2P((_localTransform.Translation + _localTransform.Orientation * _center) * rigidBody->GetScale()), C2P(_localTransform.Orientation)));
    }

    UpdateBounds();
}

void Collider::SetContactOffset(float value)
{
    value = Math::Clamp(value, 0.0f, 100.0f);
    if (Math::NearEqual(value, _contactOffset))
        return;

    _contactOffset = value;

    if (_shape)
    {
        _shape->setContactOffset(Math::Max(_shape->getRestOffset() + ZeroTolerance, _contactOffset));
    }
}

bool Collider::RayCast(const Vector3& origin, const Vector3& direction, float& resultHitDistance, float maxDistance) const
{
    resultHitDistance = MAX_float;
    if (_shape == nullptr)
        return false;

    // Prepare data
    const PxTransform trans(C2P(_transform.Translation), C2P(_transform.Orientation));
    const PxHitFlags hitFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eFACE_INDEX | PxHitFlag::eUV;

    // Perform raycast test
    PxRaycastHit hit;
    if (PxGeometryQuery::raycast(C2P(origin), C2P(direction), _shape->getGeometry().any(), trans, maxDistance, hitFlags, 1, &hit) != 0)
    {
        resultHitDistance = hit.distance;
        return true;
    }

    return false;
}

bool Collider::RayCast(const Vector3& origin, const Vector3& direction, RayCastHit& hitInfo, float maxDistance) const
{
    if (_shape == nullptr)
        return false;

    // Prepare data
    const PxTransform trans(C2P(_transform.Translation), C2P(_transform.Orientation));
    const PxHitFlags hitFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eUV;
    PxRaycastHit hit;

    // Perform raycast test
    if (PxGeometryQuery::raycast(C2P(origin), C2P(direction), _shape->getGeometry().any(), trans, maxDistance, hitFlags, 1, &hit) == 0)
        return false;

    // Gather results
    hitInfo.Gather(hit);
    return true;
}

void Collider::ClosestPoint(const Vector3& position, Vector3& result) const
{
    if (_shape == nullptr)
    {
        result = Vector3::Maximum;
        return;
    }

    // Prepare data
    const PxTransform trans(C2P(_transform.Translation), C2P(_transform.Orientation));
    PxVec3 closestPoint;

    // Compute distance between a point and a geometry object
    const float distanceSqr = PxGeometryQuery::pointDistance(C2P(position), _shape->getGeometry().any(), trans, &closestPoint);
    if (distanceSqr > 0.0f)
    {
        // Use calculated point
        result = P2C(closestPoint);
    }
    else
    {
        // Fallback to the input location
        result = position;
    }
}

bool Collider::ContainsPoint(const Vector3& point) const
{
    if (_shape)
    {
        const PxTransform trans(C2P(_transform.Translation), C2P(_transform.Orientation));
        const float distanceSqr = PxGeometryQuery::pointDistance(C2P(point), _shape->getGeometry().any(), trans);
        return distanceSqr <= 0.0f;
    }
    return false;
}

bool Collider::ComputePenetration(const Collider* colliderA, const Collider* colliderB, Vector3& direction, float& distance)
{
    direction = Vector3::Zero;
    distance = 0.0f;

    CHECK_RETURN(colliderA && colliderB, false);
    const PxShape* shapeA = colliderA->GetPxShape();
    const PxShape* shapeB = colliderB->GetPxShape();
    if (!shapeA || !shapeB)
        return false;

    const PxTransform poseA(C2P(colliderA->GetPosition()), C2P(colliderA->GetOrientation()));
    const PxTransform poseB(C2P(colliderB->GetPosition()), C2P(colliderB->GetOrientation()));

    return PxGeometryQuery::computePenetration(
        C2P(direction),
        distance,
        shapeA->getGeometry().any(),
        poseA,
        shapeB->getGeometry().any(),
        poseB
    );
}

bool Collider::IsAttached() const
{
    return _shape && _shape->getActor() != nullptr;
}

bool Collider::CanAttach(RigidBody* rigidBody) const
{
    return true;
}

bool Collider::CanBeTrigger() const
{
    return true;
}

RigidBody* Collider::GetAttachedRigidBody() const
{
    if (_shape && _staticActor == nullptr)
    {
        auto actor = _shape->getActor();
        if (actor && actor->is<PxRigidDynamic>())
            return static_cast<RigidBody*>(actor->userData);
    }
    return nullptr;
}

#if USE_EDITOR

void Collider::OnEnable()
{
    GetSceneRendering()->AddPhysicsDebug<Collider, &Collider::DrawPhysicsDebug>(this);

    // Base
    Actor::OnEnable();
}

void Collider::OnDisable()
{
    // Base
    Actor::OnDisable();

    GetSceneRendering()->RemovePhysicsDebug<Collider, &Collider::DrawPhysicsDebug>(this);
}

#endif

void Collider::Attach(RigidBody* rigidBody)
{
    ASSERT(CanAttach(rigidBody));

    // Remove static body if used
    if (_staticActor)
        RemoveStaticActor();

    // Create shape if missing
    if (_shape == nullptr)
        CreateShape();

    // Attach
    rigidBody->GetPhysXRigidActor()->attachShape(*_shape);
    _shape->setLocalPose(PxTransform(C2P((_localTransform.Translation + _localTransform.Orientation * _center) * rigidBody->GetScale()), C2P(_localTransform.Orientation)));
    if (rigidBody->IsDuringPlay())
        rigidBody->UpdateBounds();
}

void Collider::UpdateScale()
{
    const Vector3 scale = GetScale();
    if (Vector3::NearEqual(_cachedScale, scale))
        return;

    // Recreate shape geometry
    UpdateGeometry();
}

void Collider::UpdateLayerBits()
{
    ASSERT(_shape);

    PxFilterData filterData;

    // Own layer ID
    filterData.word0 = GetLayerMask();

    // Own layer mask
    filterData.word1 = Physics::LayerMasks[GetLayer()];

    _shape->setSimulationFilterData(filterData);
    _shape->setQueryFilterData(filterData);
}

void Collider::CreateShape()
{
    // Setup shape geometry
    _cachedScale = GetScale();
    PxGeometryHolder geometry;
    GetGeometry(geometry);

    // Create shape
    const bool isTrigger = _isTrigger && CanBeTrigger();
    const PxShapeFlags shapeFlags = GetShapeFlags(isTrigger, IsActiveInHierarchy());
    PxMaterial* material = Physics::GetDefaultMaterial();
    if (Material && !Material->WaitForLoaded() && Material->Instance)
    {
        material = ((PhysicalMaterial*)Material->Instance)->GetPhysXMaterial();
    }
    ASSERT(_shape == nullptr);
    _shape = CPhysX->createShape(geometry.any(), *material, true, shapeFlags);
    ASSERT(_shape);
    _shape->userData = this;
    _shape->setContactOffset(Math::Max(_shape->getRestOffset() + ZeroTolerance, _contactOffset));
    UpdateLayerBits();
}

void Collider::UpdateGeometry()
{
    if (_shape == nullptr)
        return;

    // Setup shape geometry
    _cachedScale = GetScale();
    PxGeometryHolder geometry;
    GetGeometry(geometry);

    // Recreate shape if geometry has different type
    if (_shape->getGeometryType() != geometry.getType())
    {
        // Detach from the actor
        auto actor = _shape->getActor();
        if (actor)
            actor->detachShape(*_shape);

        // Release shape
        Physics::RemoveCollider(this);
        _shape->release();
        _shape = nullptr;

        // Recreate shape
        CreateShape();

        // Reattach again (only if can, see CanAttach function)
        if (actor)
        {
            const auto rigidBody = dynamic_cast<RigidBody*>(GetParent());
            if (_staticActor != nullptr || (rigidBody && CanAttach(rigidBody)))
            {
                actor->attachShape(*_shape);
            }
            else
            {
                // Be static triangle mesh
                CreateStaticActor();
            }
        }

        return;
    }

    // Update shape
    _shape->setGeometry(geometry.any());
}

void Collider::CreateStaticActor()
{
    ASSERT(_staticActor == nullptr);

    const PxTransform trans(C2P(_transform.Translation), C2P(_transform.Orientation));
    _staticActor = CPhysX->createRigidStatic(trans);
    ASSERT(_staticActor);
    _staticActor->userData = this;

    // Reset local pos of the shape and link it to the actor
    _shape->setLocalPose(PxTransform(C2P(_center)));
    _staticActor->attachShape(*_shape);

    Physics::AddActor(_staticActor);
}

void Collider::RemoveStaticActor()
{
    ASSERT(_staticActor != nullptr);

    Physics::RemoveActor(_staticActor);
    _staticActor = nullptr;
}

#if USE_EDITOR

void Collider::DrawPhysicsDebug(RenderView& view)
{
}

#endif

void Collider::OnMaterialChanged()
{
    // Update the shape material
    if (_shape)
    {
        PxMaterial* material = Physics::GetDefaultMaterial();
        if (Material && !Material->WaitForLoaded() && Material->Instance)
        {
            material = ((PhysicalMaterial*)Material->Instance)->GetPhysXMaterial();
        }
        _shape->setMaterials(&material, 1);
    }
}

void Collider::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    PhysicsColliderActor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(Collider);

    SERIALIZE_MEMBER(IsTrigger, _isTrigger);
    SERIALIZE_MEMBER(Center, _center);
    SERIALIZE_MEMBER(ContactOffset, _contactOffset);
    SERIALIZE(Material);
}

void Collider::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    PhysicsColliderActor::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(IsTrigger, _isTrigger);
    DESERIALIZE_MEMBER(Center, _center);
    DESERIALIZE_MEMBER(ContactOffset, _contactOffset);
    DESERIALIZE(Material);
}

void Collider::BeginPlay(SceneBeginData* data)
{
    // Check if has no shape created (it means no rigidbody requested it but also collider may be spawned at runtime)
    if (_shape == nullptr)
    {
        CreateShape();

        // Check if parent is a rigidbody
        const auto rigidBody = dynamic_cast<RigidBody*>(GetParent());
        if (rigidBody && CanAttach(rigidBody))
        {
            // Attach to the rigidbody
            Attach(rigidBody);
        }
        else
        {
            // Be a static collider
            CreateStaticActor();
        }
    }

    // Base
    PhysicsColliderActor::BeginPlay(data);
}

void Collider::EndPlay()
{
    if (_shape)
    {
        // Detach from the actor
        auto actor = _shape->getActor();
        if (actor)
            actor->detachShape(*_shape);

        // Check if was using a static actor and cleanup it
        if (_staticActor)
        {
            RemoveStaticActor();
        }

        // Release shape
        Physics::RemoveCollider(this);
        _shape->release();
        _shape = nullptr;
    }

    // Base
    PhysicsColliderActor::EndPlay();
}

void Collider::OnActiveInTreeChanged()
{
    // Base
    PhysicsColliderActor::OnActiveInTreeChanged();

    if (_shape)
    {
        const bool isTrigger = _isTrigger && CanBeTrigger();
        const PxShapeFlags shapeFlags = GetShapeFlags(isTrigger, IsActiveInHierarchy());
        _shape->setFlags(shapeFlags);

        auto rigidBody = GetAttachedRigidBody();
        if (rigidBody)
        {
            rigidBody->UpdateMass();

            // TODO: maybe wake up only if one ore more shapes attached is active?
            //if (rigidBody->GetStartAwake())
            //	rigidBody->WakeUp();
        }
    }
}

void Collider::OnParentChanged()
{
    // Base
    PhysicsColliderActor::OnParentChanged();

    // Check reparenting collider case
    if (_shape)
    {
        // Detach from the actor
        auto actor = _shape->getActor();
        if (actor)
            actor->detachShape(*_shape);

        // Check if the new parent is a rigidbody
        const auto rigidBody = dynamic_cast<RigidBody*>(GetParent());
        if (rigidBody && CanAttach(rigidBody))
        {
            // Attach to the rigidbody (will remove static actor if it's n use)
            Attach(rigidBody);
        }
        else
        {
            // Use static actor (if not created yet)
            if (_staticActor == nullptr)
                CreateStaticActor();
        }
    }
}

void Collider::OnTransformChanged()
{
    // Base
    PhysicsColliderActor::OnTransformChanged();

    if (_staticActor)
    {
        _staticActor->setGlobalPose(PxTransform(C2P(_transform.Translation), C2P(_transform.Orientation)));
    }
    else if (const RigidBody* rigidBody = GetAttachedRigidBody())
    {
        _shape->setLocalPose(PxTransform(C2P((_localTransform.Translation + _localTransform.Orientation * _center) * rigidBody->GetScale()), C2P(_localTransform.Orientation)));
    }

    UpdateScale();
    UpdateBounds();
}

void Collider::OnLayerChanged()
{
    // Base
    PhysicsColliderActor::OnLayerChanged();

    if (_shape)
        UpdateLayerBits();
}
