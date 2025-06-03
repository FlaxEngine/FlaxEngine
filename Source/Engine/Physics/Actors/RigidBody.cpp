// Copyright (c) Wojciech Figat. All rights reserved.

#include "RigidBody.h"
#include "Engine/Core/Log.h"
#include "Engine/Physics/Colliders/Collider.h"
#include "Engine/Physics/PhysicsBackend.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Serialization/Serialization.h"

RigidBody::RigidBody(const SpawnParams& params)
    : Actor(params)
    , _actor(nullptr)
    , _cachedScale(1.0f)
    , _mass(1.0f)
    , _linearDamping(0.01f)
    , _angularDamping(0.05f)
    , _maxAngularVelocity(7.0f)
    , _massScale(1.0f)
    , _centerOfMassOffset(Float3::Zero)
    , _constraints(RigidbodyConstraints::None)
    , _enableSimulation(true)
    , _isKinematic(false)
    , _useCCD(false)
    , _enableGravity(true)
    , _startAwake(true)
    , _updateMassWhenScaleChanges(false)
    , _overrideMass(false)
    , _isUpdatingTransform(false)
{
}

void RigidBody::SetIsKinematic(const bool value)
{
    if (value == GetIsKinematic())
        return;
    _isKinematic = value;
    if (_actor)
    {
        PhysicsBackend::SetRigidDynamicActorFlag(_actor, PhysicsBackend::RigidDynamicFlags::Kinematic, value);
        if (!value && _isActive && _startAwake)
            WakeUp();
    }
}

void RigidBody::SetLinearDamping(float value)
{
    if (value == _linearDamping)
        return;
    _linearDamping = value;
    if (_actor)
        PhysicsBackend::SetRigidDynamicActorLinearDamping(_actor, _linearDamping);
}

void RigidBody::SetAngularDamping(float value)
{
    if (value == _angularDamping)
        return;
    _angularDamping = value;
    if (_actor)
        PhysicsBackend::SetRigidDynamicActorAngularDamping(_actor, _angularDamping);
}

void RigidBody::SetEnableSimulation(bool value)
{
    if (value == GetEnableSimulation())
        return;
    _enableSimulation = value;
    if (_actor)
    {
        const bool isActive = _enableSimulation && IsActiveInHierarchy();
        PhysicsBackend::SetActorFlag(_actor, PhysicsBackend::ActorFlags::NoSimulation, !isActive);
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
        PhysicsBackend::SetRigidDynamicActorFlag(_actor, PhysicsBackend::RigidDynamicFlags::CCD, value);
}

void RigidBody::SetEnableGravity(bool value)
{
    if (value == GetEnableGravity())
        return;
    _enableGravity = value;
    if (_actor)
    {
        PhysicsBackend::SetActorFlag(_actor, PhysicsBackend::ActorFlags::NoGravity, !value);
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
    if (value == _maxAngularVelocity)
        return;
    _maxAngularVelocity = value;
    if (_actor)
        PhysicsBackend::SetRigidDynamicActorMaxAngularVelocity(_actor, _maxAngularVelocity);
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
    if (value == _mass)
        return;
    _mass = value;
    _overrideMass = true;
    UpdateMass();
}

float RigidBody::GetMassScale() const
{
    return _massScale;
}

void RigidBody::SetMassScale(float value)
{
    if (value == _massScale)
        return;
    _massScale = value;
    UpdateMass();
}

void RigidBody::SetCenterOfMassOffset(const Float3& value)
{
    if (value == _centerOfMassOffset)
        return;
    _centerOfMassOffset = value;
    if (_actor)
        PhysicsBackend::SetRigidDynamicActorCenterOfMassOffset(_actor, _centerOfMassOffset);
}

void RigidBody::SetConstraints(const RigidbodyConstraints value)
{
    if (value == _constraints)
        return;
    _constraints = value;
    if (_actor)
        PhysicsBackend::SetRigidDynamicActorConstraints(_actor, value);
}

Vector3 RigidBody::GetLinearVelocity() const
{
    return _actor ? PhysicsBackend::GetRigidDynamicActorLinearVelocity(_actor) : Vector3::Zero;
}

void RigidBody::SetLinearVelocity(const Vector3& value) const
{
    if (_actor)
        PhysicsBackend::SetRigidDynamicActorLinearVelocity(_actor, value, GetStartAwake());
}

Vector3 RigidBody::GetAngularVelocity() const
{
    return _actor ? PhysicsBackend::GetRigidDynamicActorAngularVelocity(_actor) : Vector3::Zero;
}

void RigidBody::SetAngularVelocity(const Vector3& value) const
{
    if (_actor)
        PhysicsBackend::SetRigidDynamicActorAngularVelocity(_actor, value, GetStartAwake());
}

float RigidBody::GetMaxDepenetrationVelocity() const
{
    return _actor ? PhysicsBackend::GetRigidDynamicActorMaxDepenetrationVelocity(_actor) : 0;
}

void RigidBody::SetMaxDepenetrationVelocity(const float value) const
{
    if (_actor)
        PhysicsBackend::SetRigidDynamicActorMaxDepenetrationVelocity(_actor, value);
}

float RigidBody::GetSleepThreshold() const
{
    return _actor ? PhysicsBackend::GetRigidDynamicActorSleepThreshold(_actor) : 0;
}

void RigidBody::SetSleepThreshold(const float value) const
{
    if (_actor)
        PhysicsBackend::SetRigidDynamicActorSleepThreshold(_actor, value);
}

Vector3 RigidBody::GetCenterOfMass() const
{
    return _actor ? PhysicsBackend::GetRigidDynamicActorCenterOfMass(_actor) : Vector3::Zero;
}

bool RigidBody::IsSleeping() const
{
    return _actor ? PhysicsBackend::GetRigidDynamicActorIsSleeping(_actor) : false;
}

void RigidBody::Sleep() const
{
    if (_actor && GetEnableSimulation() && !GetIsKinematic() && IsActiveInHierarchy())
        PhysicsBackend::RigidDynamicActorSleep(_actor);
}

void RigidBody::WakeUp() const
{
    if (_actor && GetEnableSimulation() && !GetIsKinematic() && IsActiveInHierarchy())
        PhysicsBackend::RigidDynamicActorWakeUp(_actor);
}

void RigidBody::UpdateMass()
{
    if (_actor)
        PhysicsBackend::UpdateRigidDynamicActorMass(_actor, _mass, _massScale, _overrideMass == 0);
}

void RigidBody::AddForce(const Vector3& force, ForceMode mode) const
{
    if (_actor && GetEnableSimulation())
        PhysicsBackend::AddRigidDynamicActorForce(_actor, force, mode);
}

void RigidBody::AddForceAtPosition(const Vector3& force, const Vector3& position, ForceMode mode) const
{
    if (_actor && GetEnableSimulation())
        PhysicsBackend::AddRigidDynamicActorForceAtPosition(_actor, force, position, mode);
}

void RigidBody::AddRelativeForce(const Vector3& force, ForceMode mode) const
{
    AddForce(Vector3::Transform(force, _transform.Orientation), mode);
}

void RigidBody::AddTorque(const Vector3& torque, ForceMode mode) const
{
    if (_actor && GetEnableSimulation())
        PhysicsBackend::AddRigidDynamicActorTorque(_actor, torque, mode);
}

void RigidBody::AddRelativeTorque(const Vector3& torque, ForceMode mode) const
{
    AddTorque(Vector3::Transform(torque, _transform.Orientation), mode);
}

void RigidBody::SetSolverIterationCounts(int32 minPositionIters, int32 minVelocityIters) const
{
    if (_actor)
        PhysicsBackend::SetRigidDynamicActorSolverIterationCounts(_actor, minPositionIters, minVelocityIters);
}

void RigidBody::ClosestPoint(const Vector3& position, Vector3& result) const
{
    Vector3 tmp;
    Real minDistanceSqr = MAX_Real;
    result = Vector3::Maximum;
    for (int32 i = 0; i < Children.Count(); i++)
    {
        const auto collider = dynamic_cast<Collider*>(Children[i]);
        if (collider && collider->GetAttachedRigidBody() == this)
        {
            collider->ClosestPoint(position, tmp);
            const Real dstSqr = Vector3::DistanceSquared(position, tmp);
            if (dstSqr < minDistanceSqr)
            {
                minDistanceSqr = dstSqr;
                result = tmp;
            }
        }
    }
}

void RigidBody::AddMovement(const Vector3& translation, const Quaternion& rotation)
{
    // filter rotation according to constraints
    Quaternion allowedRotation;
    if (EnumHasAllFlags(GetConstraints(), RigidbodyConstraints::LockRotation))
        allowedRotation = Quaternion::Identity;
    else
    {
        Float3 euler = rotation.GetEuler();
        if (EnumHasAnyFlags(GetConstraints(), RigidbodyConstraints::LockRotationX))
            euler.X = 0;
        if (EnumHasAnyFlags(GetConstraints(), RigidbodyConstraints::LockRotationY))
            euler.Y = 0;
        if (EnumHasAnyFlags(GetConstraints(), RigidbodyConstraints::LockRotationZ))
            euler.Z = 0;
        allowedRotation = Quaternion::Euler(euler);
    }

    // filter translation according to the constraints
    auto allowedTranslation = translation;
    if (EnumHasAllFlags(GetConstraints(), RigidbodyConstraints::LockPosition))
        allowedTranslation = Vector3::Zero;
    else
    {
        if (EnumHasAnyFlags(GetConstraints(), RigidbodyConstraints::LockPositionX))
            allowedTranslation.X = 0;
        if (EnumHasAnyFlags(GetConstraints(), RigidbodyConstraints::LockPositionY))
            allowedTranslation.Y = 0;
        if (EnumHasAnyFlags(GetConstraints(), RigidbodyConstraints::LockPositionZ))
            allowedTranslation.Z = 0;
    }
    Transform t;
    t.Translation = _transform.Translation + allowedTranslation;
    t.Orientation = _transform.Orientation * allowedRotation;
    t.Scale = _transform.Scale;
    SetTransform(t);
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

void RigidBody::UpdateBounds()
{
    void* actor = _actor;
    if (actor && PhysicsBackend::GetRigidActorShapesCount(actor) != 0)
        PhysicsBackend::GetActorBounds(actor, _box);
    else
        _box = BoundingBox(_transform.Translation);
    BoundingSphere::FromBox(_box, _sphere);
}

void RigidBody::UpdateScale()
{
    const Float3 scale = GetScale();
    if (_cachedScale == scale)
        return;
    _cachedScale = scale;

    // Check if update mass
    if (_updateMassWhenScaleChanges && !_overrideMass)
        UpdateMass();
}

void RigidBody::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

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
    Actor::Deserialize(stream, modifier);

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

void* RigidBody::GetPhysicsActor() const
{
    return _actor;
}

void RigidBody::OnActiveTransformChanged()
{
    // Change actor transform (but with locking)
    ASSERT(!_isUpdatingTransform);
    _isUpdatingTransform = true;
    Transform transform = _transform;
    PhysicsBackend::GetRigidActorPose(_actor, transform.Translation, transform.Orientation);
    if (transform.Translation.IsNanOrInfinity() || transform.Orientation.IsNanOrInfinity())
    {
        LOG(Error, "GetRigidActorPose retuned NaN/Inf transformation");
        transform = _transform;
    }
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

void RigidBody::BeginPlay(SceneBeginData* data)
{
    // Create rigid body
    ASSERT(_actor == nullptr);
    void* scene = GetPhysicsScene()->GetPhysicsScene();
    _actor = PhysicsBackend::CreateRigidDynamicActor(this, _transform.Translation, _transform.Orientation, scene);

    // Apply properties
    auto actorFlags = PhysicsBackend::ActorFlags::None;
    if (!_enableSimulation || !IsActiveInHierarchy())
        actorFlags |= PhysicsBackend::ActorFlags::NoSimulation;
    if (!_enableGravity)
        actorFlags |= PhysicsBackend::ActorFlags::NoGravity;
    PhysicsBackend::SetActorFlags(_actor, actorFlags);
    auto rigidBodyFlags = PhysicsBackend::RigidDynamicFlags::None;
    if (_isKinematic)
        rigidBodyFlags |= PhysicsBackend::RigidDynamicFlags::Kinematic;
    if (_useCCD)
        rigidBodyFlags |= PhysicsBackend::RigidDynamicFlags::CCD;
    PhysicsBackend::SetRigidDynamicActorFlags(_actor, rigidBodyFlags);
    PhysicsBackend::SetRigidDynamicActorLinearDamping(_actor, _linearDamping);
    PhysicsBackend::SetRigidDynamicActorAngularDamping(_actor, _angularDamping);
    PhysicsBackend::SetRigidDynamicActorMaxAngularVelocity(_actor, _maxAngularVelocity);
    PhysicsBackend::SetRigidDynamicActorConstraints(_actor, _constraints);

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
        PhysicsBackend::SetRigidDynamicActorCenterOfMassOffset(_actor, _centerOfMassOffset);

    // Register actor
    PhysicsBackend::AddSceneActor(scene, _actor);
    const bool putToSleep = !_startAwake && GetEnableSimulation() && !GetIsKinematic() && IsActiveInHierarchy();
    if (putToSleep)
        PhysicsBackend::AddSceneActorAction(scene, _actor, PhysicsBackend::ActionType::Sleep);

    // Update cached data
    UpdateBounds();

    // Base
    Actor::BeginPlay(data);
}

void RigidBody::EndPlay()
{
    // Base
    Actor::EndPlay();

    if (_actor)
    {
        // Remove actor
        void* scene = GetPhysicsScene()->GetPhysicsScene();
        PhysicsBackend::RemoveSceneActor(scene, _actor);
        PhysicsBackend::DestroyActor(_actor);
        _actor = nullptr;
    }
}

void RigidBody::OnActiveInTreeChanged()
{
    // Base
    Actor::OnActiveInTreeChanged();

    if (_actor)
    {
        const bool isActive = _enableSimulation && IsActiveInHierarchy();
        PhysicsBackend::SetActorFlag(_actor, PhysicsBackend::ActorFlags::NoSimulation, !isActive);

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
    // Base
    Actor::OnTransformChanged();

    // Update physics is not during physics state synchronization
    if (!_isUpdatingTransform && _actor)
    {
        const bool kinematic = GetIsKinematic() && GetEnableSimulation();
        PhysicsBackend::SetRigidActorPose(_actor, _transform.Translation, _transform.Orientation, kinematic, true);
        UpdateScale();
    }

    UpdateBounds();
}

void RigidBody::OnPhysicsSceneChanged(PhysicsScene* previous)
{
    PhysicsBackend::RemoveSceneActor(previous->GetPhysicsScene(), _actor, true);
    void* scene = GetPhysicsScene()->GetPhysicsScene();
    PhysicsBackend::AddSceneActor(scene, _actor);
    const bool putToSleep = !_startAwake && GetEnableSimulation() && !GetIsKinematic() && IsActiveInHierarchy();
    if (putToSleep)
        PhysicsBackend::AddSceneActorAction(scene, _actor, PhysicsBackend::ActionType::Sleep);
}
