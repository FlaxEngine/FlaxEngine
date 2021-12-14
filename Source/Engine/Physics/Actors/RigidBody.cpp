// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "RigidBody.h"
#include "PxMaterial.h"

#include "Engine/Core/Log.h"
#include "Engine/Physics/Utilities.h"
#include "Engine/Physics/Colliders/Collider.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/PhysicalMaterial.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Serialization/Serialization.h"
#include <ThirdParty/PhysX/extensions/PxRigidBodyExt.h>
#include <ThirdParty/PhysX/PxRigidActor.h>
#include <ThirdParty/PhysX/PxRigidDynamic.h>
#include <ThirdParty/PhysX/PxPhysics.h>

RigidBody::RigidBody(const SpawnParams& params)
    : PhysicsActor(params)
    , _actor(nullptr)
    , _mass(1.0f)
    , _linearDamping(0.01f)
    , _angularDamping(0.05f)
    , _maxAngularVelocity(7.0f)
    , _massScale(1.0f)
    , _centerOfMassOffset(Vector3::Zero)
    , _constraints(RigidbodyConstraints::None)
    , _enableSimulation(true)
    , _isKinematic(false)
    , _useCCD(false)
    , _enableGravity(true)
    , _startAwake(true)
    , _updateMassWhenScaleChanges(false)
    , _overrideMass(false)
{
}

void RigidBody::SetIsKinematic(const bool value)
{
    if (value == GetIsKinematic())
        return;

    _isKinematic = value;

    if (_actor)
        _actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, value);
}

void RigidBody::SetLinearDamping(float value)
{
    if (Math::NearEqual(value, _linearDamping))
        return;

    _linearDamping = value;

    if (_actor)
    {
        _actor->setLinearDamping(value);
    }
}

void RigidBody::SetAngularDamping(float value)
{
    if (Math::NearEqual(value, _angularDamping))
        return;

    _angularDamping = value;

    if (_actor)
    {
        _actor->setAngularDamping(value);
    }
}

void RigidBody::SetEnableSimulation(bool value)
{
    if (value == GetEnableSimulation())
        return;

    _enableSimulation = value;

    if (_actor)
    {
        const bool isActive = _enableSimulation && IsActiveInHierarchy();
        _actor->setActorFlag(PxActorFlag::eDISABLE_SIMULATION, !isActive);

        // Auto wake up
        if (isActive && GetStartAwake())
            WakeUp();
    }
}

void RigidBody::SetUseCCD(bool value)
{
    if (value == GetUseCCD())
        return;

    _useCCD = value;

    if (_actor)
        _actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, value);
}

void RigidBody::SetEnableGravity(bool value)
{
    if (value == GetEnableGravity())
        return;

    _enableGravity = value;

    if (_actor)
    {
        _actor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !value);

        if (value)
            WakeUp();
    }
}

void RigidBody::SetStartAwake(bool value)
{
    _startAwake = value;
}

void RigidBody::SetUpdateMassWhenScaleChanges(bool value)
{
    _updateMassWhenScaleChanges = value;
}

void RigidBody::SetMaxAngularVelocity(float value)
{
    if (Math::NearEqual(value, _maxAngularVelocity))
        return;

    _maxAngularVelocity = value;

    if (_actor)
        _actor->setMaxAngularVelocity(value);
}

bool RigidBody::GetOverrideMass() const
{
    return _overrideMass != 0;
}

void RigidBody::SetOverrideMass(bool value)
{
    if (value == GetOverrideMass())
        return;

    _overrideMass = value;

    UpdateMass();
}

float RigidBody::GetMass() const
{
    return _mass;
}

void RigidBody::SetMass(float value)
{
    if (Math::NearEqual(value, _mass))
        return;

    _mass = value;

    // Auto enable override
    _overrideMass = true;

    UpdateMass();
}

float RigidBody::GetMassScale() const
{
    return _massScale;
}

void RigidBody::SetMassScale(float value)
{
    if (Math::NearEqual(value, _massScale))
        return;

    _massScale = value;
    UpdateMass();
}

void RigidBody::SetCenterOfMassOffset(const Vector3& value)
{
    if (Vector3::NearEqual(value, _centerOfMassOffset))
        return;

    _centerOfMassOffset = value;

    if (_actor)
    {
        PxTransform pose = _actor->getCMassLocalPose();
        pose.p += C2P(_centerOfMassOffset);
        _actor->setCMassLocalPose(pose);
    }
}

void RigidBody::SetConstraints(const RigidbodyConstraints value)
{
    if (value == _constraints)
        return;

    _constraints = value;

    if (_actor)
    {
        _actor->setRigidDynamicLockFlags(static_cast<PxRigidDynamicLockFlag::Enum>(value));
    }
}

Vector3 RigidBody::GetLinearVelocity() const
{
    return _actor ? P2C(_actor->getLinearVelocity()) : Vector3::Zero;
}

void RigidBody::SetLinearVelocity(const Vector3& value) const
{
    if (_actor)
        _actor->setLinearVelocity(C2P(value), GetStartAwake());
}

Vector3 RigidBody::GetAngularVelocity() const
{
    return _actor ? P2C(_actor->getAngularVelocity()) : Vector3::Zero;
}

void RigidBody::SetAngularVelocity(const Vector3& value) const
{
    if (_actor)
        _actor->setAngularVelocity(C2P(value), GetStartAwake());
}

float RigidBody::GetMaxDepenetrationVelocity() const
{
    return _actor ? _actor->getMaxDepenetrationVelocity() : 0;
}

void RigidBody::SetMaxDepenetrationVelocity(const float value) const
{
    if (_actor)
        _actor->setMaxDepenetrationVelocity(value);
}

float RigidBody::GetSleepThreshold() const
{
    return _actor ? _actor->getSleepThreshold() : 0;
}

void RigidBody::SetSleepThreshold(const float value) const
{
    if (_actor)
        _actor->setSleepThreshold(value);
}

Vector3 RigidBody::GetCenterOfMass() const
{
    return _actor ? P2C(_actor->getCMassLocalPose().p) : Vector3::Zero;
}

bool RigidBody::IsSleeping() const
{
    return _actor ? _actor->isSleeping() : false;
}

void RigidBody::Sleep() const
{
    if (_actor && GetEnableSimulation() && !GetIsKinematic() && IsActiveInHierarchy() && _actor->getScene())
    {
        _actor->putToSleep();
    }
}

void RigidBody::WakeUp() const
{
    if (_actor && GetEnableSimulation() && !GetIsKinematic() && IsActiveInHierarchy() && _actor->getScene())
    {
        _actor->wakeUp();
    }
}

void RigidBody::UpdateMass()
{
    if (_actor == nullptr)
        return;

    if (_overrideMass)
    {
        // Use fixed mass
        PxRigidBodyExt::setMassAndUpdateInertia(*_actor, Math::Max(_mass * _massScale, 0.001f));
    }
    else
    {
        // Calculate per-shape densities (convert kg/m^3 into engine units)
        const float minDensity = 0.08375f; // Hydrogen density
        const float defaultDensity = 1000.0f; // Water density
        Array<float, InlinedAllocation<32>> densities;
        for (uint32 i = 0; i < _actor->getNbShapes(); i++)
        {
            PxShape* shape;
            _actor->getShapes(&shape, 1, i);
            if (shape->getFlags() & PxShapeFlag::eSIMULATION_SHAPE)
            {
                float density = defaultDensity;
                PxMaterial* material;
                if (shape->getMaterials(&material, 1, 0) == 1)
                {
                    if (const auto mat = (PhysicalMaterial*)material->userData)
                    {
                        density = Math::Max(mat->Density, minDensity);
                    }
                }
                densities.Add(KgPerM3ToKgPerCm3(density));
            }
        }
        if (densities.IsEmpty())
            densities.Add(KgPerM3ToKgPerCm3(defaultDensity));

        // Auto calculated mass
        PxRigidBodyExt::updateMassAndInertia(*_actor, densities.Get(), densities.Count());
        _mass = _actor->getMass();
        const float massScale = Math::Max(_massScale, 0.001f);
        if (!Math::IsOne(massScale))
        {
            _mass *= massScale;
            _actor->setMass(_mass);
        }
    }
}

void RigidBody::AddForce(const Vector3& force, ForceMode mode) const
{
    if (_actor && GetEnableSimulation())
    {
        _actor->addForce(C2P(force), static_cast<PxForceMode::Enum>(mode));
    }
}

void RigidBody::AddForceAtPosition(const Vector3& force, const Vector3& position, ForceMode mode) const
{
    if (_actor && GetEnableSimulation())
    {
        PxRigidBodyExt::addForceAtPos(*_actor, C2P(force), C2P(position), static_cast<PxForceMode::Enum>(mode));
    }
}

void RigidBody::AddRelativeForce(const Vector3& force, ForceMode mode) const
{
    AddForce(Vector3::Transform(force, _transform.Orientation), mode);
}

void RigidBody::AddTorque(const Vector3& torque, ForceMode mode) const
{
    if (_actor && GetEnableSimulation())
    {
        _actor->addTorque(C2P(torque), static_cast<PxForceMode::Enum>(mode));
    }
}

void RigidBody::AddRelativeTorque(const Vector3& torque, ForceMode mode) const
{
    AddTorque(Vector3::Transform(torque, _transform.Orientation), mode);
}

void RigidBody::SetSolverIterationCounts(int32 minPositionIters, int32 minVelocityIters) const
{
    if (_actor)
    {
        _actor->setSolverIterationCounts(Math::Clamp(minPositionIters, 1, 255), Math::Clamp(minVelocityIters, 1, 255));
    }
}

void RigidBody::ClosestPoint(const Vector3& position, Vector3& result) const
{
    Vector3 tmp;
    float minDistanceSqr = MAX_float;
    result = Vector3::Maximum;
    for (int32 i = 0; i < Children.Count(); i++)
    {
        const auto collider = dynamic_cast<Collider*>(Children[i]);
        if (collider && collider->GetAttachedRigidBody() == this)
        {
            collider->ClosestPoint(position, tmp);
            const auto dstSqr = Vector3::DistanceSquared(position, tmp);
            if (dstSqr < minDistanceSqr)
            {
                minDistanceSqr = dstSqr;
                result = tmp;
            }
        }
    }
}

void RigidBody::OnCollisionEnter(const Collision& c)
{
    CollisionEnter(c);
}

void RigidBody::OnCollisionExit(const Collision& c)
{
    CollisionExit(c);
}

void RigidBody::OnTriggerEnter(PhysicsColliderActor* c)
{
    TriggerEnter(c);
}

void RigidBody::OnTriggerExit(PhysicsColliderActor* c)
{
    TriggerExit(c);
}

void RigidBody::OnColliderChanged(Collider* c)
{
    UpdateMass();

    // TODO: maybe wake up only if one ore more shapes attached is active?
    //if (GetStartAwake())
    //	WakeUp();
}

void RigidBody::CreateActor()
{
    ASSERT(_actor == nullptr);

    // Create rigid body
    const PxTransform trans(C2P(_transform.Translation), C2P(_transform.Orientation));
    _actor = CPhysX->createRigidDynamic(trans);
    _actor->userData = this;

    // Setup flags
#if WITH_PVD
    PxActorFlags actorFlags = PxActorFlag::eVISUALIZATION;
#else
    PxActorFlags actorFlags = static_cast<PxActorFlags>(0);
#endif
    const bool isActive = _enableSimulation && IsActiveInHierarchy();
    if (!isActive)
        actorFlags |= PxActorFlag::eDISABLE_SIMULATION;
    if (!_enableGravity)
        actorFlags |= PxActorFlag::eDISABLE_GRAVITY;
    _actor->setActorFlags(actorFlags);
    if (_useCCD)
        _actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
    if (_isKinematic)
        _actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);

    // Apply properties
    _actor->setLinearDamping(_linearDamping);
    _actor->setAngularDamping(_angularDamping);
    _actor->setMaxAngularVelocity(_maxAngularVelocity);
    _actor->setRigidDynamicLockFlags(static_cast<PxRigidDynamicLockFlag::Enum>(_constraints));

    // Find colliders to attach
    for (int32 i = 0; i < Children.Count(); i++)
    {
        auto collider = dynamic_cast<Collider*>(Children[i]);
        if (collider && collider->CanAttach(this))
        {
            collider->Attach(this);
        }
    }

    // Setup mass (calculate or use overriden value)
    UpdateMass();

    // Apply the Center Of Mass offset
    if (!_centerOfMassOffset.IsZero())
    {
        PxTransform pose = _actor->getCMassLocalPose();
        pose.p += C2P(_centerOfMassOffset);
        _actor->setCMassLocalPose(pose);
    }

    // Register actor
    const bool putToSleep = !_startAwake && GetEnableSimulation() && !GetIsKinematic() && IsActiveInHierarchy();
    GetPhysicsScene()->AddActor(_actor, putToSleep);

    // Update cached data
    UpdateBounds();
}

void RigidBody::UpdateScale()
{
    const Vector3 scale = GetScale();
    if (Vector3::NearEqual(_cachedScale, scale))
        return;

    _cachedScale = scale;

    // Check if update mass
    if (_updateMassWhenScaleChanges && !_overrideMass)
        UpdateMass();
}

void RigidBody::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    PhysicsActor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(RigidBody);

    SERIALIZE_BIT_MEMBER(OverrideMass, _overrideMass);
    SERIALIZE_MEMBER(Mass, _mass);

    SERIALIZE_MEMBER(LinearDamping, _linearDamping);
    SERIALIZE_MEMBER(AngularDamping, _angularDamping);
    SERIALIZE_MEMBER(MaxAngularVelocity, _maxAngularVelocity);
    SERIALIZE_MEMBER(CenterOfMassOffset, _centerOfMassOffset);
    SERIALIZE_MEMBER(MassScale, _massScale);
    SERIALIZE_MEMBER(Constraints, _constraints);

    SERIALIZE_BIT_MEMBER(EnableSimulation, _enableSimulation);
    SERIALIZE_BIT_MEMBER(IsKinematic, _isKinematic);
    SERIALIZE_BIT_MEMBER(UseCCD, _useCCD);
    SERIALIZE_BIT_MEMBER(EnableGravity, _enableGravity);
    SERIALIZE_BIT_MEMBER(StartAwake, _startAwake);
    SERIALIZE_BIT_MEMBER(UpdateMassWhenScaleChanges, _updateMassWhenScaleChanges);
}

void RigidBody::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    PhysicsActor::Deserialize(stream, modifier);

    DESERIALIZE_BIT_MEMBER(OverrideMass, _overrideMass);
    DESERIALIZE_MEMBER(Mass, _mass);

    DESERIALIZE_MEMBER(LinearDamping, _linearDamping);
    DESERIALIZE_MEMBER(AngularDamping, _angularDamping);
    DESERIALIZE_MEMBER(MaxAngularVelocity, _maxAngularVelocity);
    DESERIALIZE_MEMBER(CenterOfMassOffset, _centerOfMassOffset);
    DESERIALIZE_MEMBER(MassScale, _massScale);
    DESERIALIZE_MEMBER(Constraints, _constraints);

    DESERIALIZE_BIT_MEMBER(EnableSimulation, _enableSimulation);
    DESERIALIZE_BIT_MEMBER(IsKinematic, _isKinematic);
    DESERIALIZE_BIT_MEMBER(UseCCD, _useCCD);
    DESERIALIZE_BIT_MEMBER(EnableGravity, _enableGravity);
    DESERIALIZE_BIT_MEMBER(StartAwake, _startAwake);
    DESERIALIZE_BIT_MEMBER(UpdateMassWhenScaleChanges, _updateMassWhenScaleChanges);
}

PxActor* RigidBody::GetPhysXActor()
{
    return (PxActor*)_actor;
}

PxRigidActor* RigidBody::GetRigidActor()
{
    return (PxRigidActor*)_actor;
}

void RigidBody::BeginPlay(SceneBeginData* data)
{
    CreateActor();

    // Base
    PhysicsActor::BeginPlay(data);
}

void RigidBody::EndPlay()
{
    // Detach all the shapes
    PxShape* shapes[8];
    while (_actor && _actor->getNbShapes() > 0)
    {
        const uint32 count = _actor->getShapes(shapes, 8, 0);
        for (uint32 i = 0; i < count; i++)
        {
            _actor->detachShape(*shapes[i], false);
        }
    }

    // Base
    PhysicsActor::EndPlay();

    if (_actor)
    {
        // Remove actor
        GetPhysicsScene()->RemoveActor(_actor);
        _actor = nullptr;
    }
}

void RigidBody::OnActiveInTreeChanged()
{
    // Base
    PhysicsActor::OnActiveInTreeChanged();

    if (_actor)
    {
        const bool isActive = _enableSimulation && IsActiveInHierarchy();
        _actor->setActorFlag(PxActorFlag::eDISABLE_SIMULATION, !isActive);

        // Auto wake up
        if (isActive && GetStartAwake())
            WakeUp();
            // Clear velocities and the forces on disabled
        else if (!IsActiveInHierarchy())
            Sleep();
    }
}

void RigidBody::OnTransformChanged()
{
    // Update physics is not during physics state synchronization
    if (!_isUpdatingTransform && _actor)
    {
        // Base (skip PhysicsActor call to optimize)
        Actor::OnTransformChanged();

        const PxTransform trans(C2P(_transform.Translation), C2P(_transform.Orientation));
        if (GetIsKinematic() && GetEnableSimulation())
            _actor->setKinematicTarget(trans);
        else
            _actor->setGlobalPose(trans, true);

        UpdateScale();
        UpdateBounds();
    }
    else
    {
        // Base
        PhysicsActor::OnTransformChanged();
    }
}

void RigidBody::OnPhysicsSceneChanged(PhysicsScene* previous)
{
    ASSERT(previous);

    PhysicsActor::OnPhysicsSceneChanged(previous);

    previous->UnlinkActor(_actor);

    const bool putToSleep = !_startAwake && GetEnableSimulation() && !GetIsKinematic() && IsActiveInHierarchy();
    GetPhysicsScene()->AddActor(_actor, putToSleep);
}
