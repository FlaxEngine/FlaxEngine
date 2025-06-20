// Copyright (c) Wojciech Figat. All rights reserved.

#include "Collider.h"
#include "Engine/Core/Log.h"
#include "Engine/Level/Scene/Scene.h"
#if USE_EDITOR
#include "Engine/Level/Scene/SceneRendering.h"
#endif
#include "Engine/Physics/PhysicsSettings.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/PhysicsBackend.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Physics/Actors/RigidBody.h"

Collider::Collider(const SpawnParams& params)
    : PhysicsColliderActor(params)
    , _center(Float3::Zero)
    , _isTrigger(false)
    , _shape(nullptr)
    , _staticActor(nullptr)
    , _cachedScale(1.0f)
    , _contactOffset(2.0f)
{
    Material.Loaded.Bind<Collider, &Collider::OnMaterialChanged>(this);
    Material.Unload.Bind<Collider, &Collider::OnMaterialChanged>(this);
    Material.Changed.Bind<Collider, &Collider::OnMaterialChanged>(this);
}

void* Collider::GetPhysicsShape() const
{
    return _shape;
}

void Collider::SetIsTrigger(bool value)
{
    if (value == _isTrigger || !CanBeTrigger())
        return;
    _isTrigger = value;
    if (_shape)
        PhysicsBackend::SetShapeState(_shape, IsActiveInHierarchy(), _isTrigger && CanBeTrigger());
    if (EnumHasAnyFlags(_staticFlags, StaticFlags::Navigation) && _isEnabled)
    {
        if (_isTrigger)
            GetScene()->Navigation.Actors.Remove(this);
        else
            GetScene()->Navigation.Actors.Add(this);
    }
}

void Collider::SetCenter(const Vector3& value)
{
    if (value == _center)
        return;
    _center = value;
    if (_staticActor)
        PhysicsBackend::SetShapeLocalPose(_shape, _center * GetScale(), Quaternion::Identity);
    else if (const RigidBody* rigidBody = GetAttachedRigidBody())
        PhysicsBackend::SetShapeLocalPose(_shape, (_localTransform.Translation + _localTransform.Orientation * _center) * rigidBody->GetScale(), _localTransform.Orientation);
    UpdateBounds();
}

void Collider::SetContactOffset(float value)
{
    value = Math::Clamp(value, 0.0f, 100.0f);
    if (value == _contactOffset)
        return;
    _contactOffset = value;
    if (_shape)
        PhysicsBackend::SetShapeContactOffset(_shape, _contactOffset);
}

bool Collider::RayCast(const Vector3& origin, const Vector3& direction, float& resultHitDistance, float maxDistance) const
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    resultHitDistance = MAX_float;
    if (_shape == nullptr)
        return false;
    return PhysicsBackend::RayCastShape(_shape, _transform.Translation, _transform.Orientation, origin, direction, resultHitDistance, maxDistance);
}

bool Collider::RayCast(const Vector3& origin, const Vector3& direction, RayCastHit& hitInfo, float maxDistance) const
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    if (_shape == nullptr)
        return false;
    return PhysicsBackend::RayCastShape(_shape, _transform.Translation, _transform.Orientation, origin, direction, hitInfo, maxDistance);
}

void Collider::ClosestPoint(const Vector3& point, Vector3& result) const
{
    if (_shape == nullptr)
    {
        result = Vector3::Maximum;
        return;
    }
    Vector3 closestPoint;
    const float distanceSqr = PhysicsBackend::ComputeShapeSqrDistanceToPoint(_shape, _transform.Translation, _transform.Orientation, point, &closestPoint);
    if (distanceSqr > 0.0f)
        result = closestPoint;
    else
        result = point;
}

bool Collider::ContainsPoint(const Vector3& point) const
{
    if (_shape)
    {
        const float distanceSqr = PhysicsBackend::ComputeShapeSqrDistanceToPoint(_shape, _transform.Translation, _transform.Orientation, point);
        return distanceSqr <= 0.0f;
    }
    return false;
}

bool Collider::ComputePenetration(const Collider* colliderA, const Collider* colliderB, Vector3& direction, float& distance)
{
    direction = Vector3::Zero;
    distance = 0.0f;
    CHECK_RETURN(colliderA && colliderB, false);
    void* shapeA = colliderA->GetPhysicsShape();
    void* shapeB = colliderB->GetPhysicsShape();
    if (!shapeA || !shapeB)
        return false;
    return PhysicsBackend::ComputeShapesPenetration(shapeA, shapeB, colliderA->GetPosition(), colliderA->GetOrientation(), colliderB->GetPosition(), colliderB->GetOrientation(), direction, distance);
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
        return dynamic_cast<RigidBody*>(GetParent());
    }
    return nullptr;
}

void Collider::OnEnable()
{
    if (EnumHasAnyFlags(_staticFlags, StaticFlags::Navigation) && !_isTrigger)
        GetScene()->Navigation.Actors.Add(this);
#if USE_EDITOR
    GetSceneRendering()->AddPhysicsDebug<Collider, &Collider::DrawPhysicsDebug>(this);
#endif

    PhysicsColliderActor::OnEnable();
}

void Collider::OnDisable()
{
    PhysicsColliderActor::OnDisable();

    if (EnumHasAnyFlags(_staticFlags, StaticFlags::Navigation) && !_isTrigger)
        GetScene()->Navigation.Actors.Remove(this);
#if USE_EDITOR
    GetSceneRendering()->RemovePhysicsDebug<Collider, &Collider::DrawPhysicsDebug>(this);
#endif
}

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
    PhysicsBackend::AttachShape(_shape, rigidBody->GetPhysicsActor());
    _cachedLocalPosePos = (_localTransform.Translation + _localTransform.Orientation * _center) * rigidBody->GetScale();
    _cachedLocalPoseRot = _localTransform.Orientation;
    PhysicsBackend::SetShapeLocalPose(_shape, _cachedLocalPosePos, _cachedLocalPoseRot);
    if (rigidBody->IsDuringPlay())
    {
        rigidBody->UpdateBounds();
        rigidBody->UpdateMass();
    }
}

void Collider::UpdateLayerBits()
{
    // Own layer ID
    const uint32 mask0 = GetLayerMask();

    // Own layer mask
    const uint32 mask1 = Physics::LayerMasks[GetLayer()];

    ASSERT(_shape);
    PhysicsBackend::SetShapeFilterMask(_shape, mask0, mask1);
}

void Collider::CreateShape()
{
    ASSERT(_shape == nullptr);

    // Setup shape geometry
    _cachedScale = GetScale().GetAbsolute().MaxValue();
    CollisionShape shape;
    GetGeometry(shape);

    // Create shape
    const bool isTrigger = _isTrigger && CanBeTrigger();
    _shape = PhysicsBackend::CreateShape(this, shape, Material, IsActiveInHierarchy(), isTrigger);
    PhysicsBackend::SetShapeContactOffset(_shape, _contactOffset);
    UpdateLayerBits();
}

void Collider::UpdateGeometry()
{
    if (_shape == nullptr)
        return;

    // Setup shape geometry
    _cachedScale = GetScale().GetAbsolute().MaxValue();
    CollisionShape shape;
    GetGeometry(shape);

    // Recreate shape if geometry has different type
    if (PhysicsBackend::GetShapeType(_shape) != shape.Type)
    {
        // Detach from the actor
        void* actor = PhysicsBackend::GetShapeActor(_shape);
        if (actor)
            PhysicsBackend::DetachShape(_shape, actor);

        // Release shape
        PhysicsBackend::RemoveCollider(this);
        PhysicsBackend::DestroyShape(_shape);
        _shape = nullptr;

        // Recreate shape
        CreateShape();

        // Reattach again (only if can, see CanAttach function)
        if (actor)
        {
            const auto rigidBody = dynamic_cast<RigidBody*>(GetParent());
            if (_staticActor != nullptr || (rigidBody && CanAttach(rigidBody)))
            {
                PhysicsBackend::AttachShape(_shape, actor);
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
    PhysicsBackend::SetShapeGeometry(_shape, shape);
}

void Collider::CreateStaticActor()
{
    ASSERT(_staticActor == nullptr);
    void* scene = GetPhysicsScene()->GetPhysicsScene();
    _staticActor = PhysicsBackend::CreateRigidStaticActor(nullptr, _transform.Translation, _transform.Orientation, scene);

    // Reset local pos of the shape and link it to the actor
    PhysicsBackend::SetShapeLocalPose(_shape, _center * GetScale(), Quaternion::Identity);
    PhysicsBackend::AttachShape(_shape, _staticActor);

    PhysicsBackend::AddSceneActor(scene, _staticActor);
}

void Collider::RemoveStaticActor()
{
    ASSERT(_staticActor != nullptr);
    void* scene = GetPhysicsScene()->GetPhysicsScene();
    PhysicsBackend::RemoveSceneActor(scene, _staticActor);
    PhysicsBackend::DestroyActor(_staticActor);
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
        PhysicsBackend::SetShapeMaterial(_shape, Material);
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
    // Base
    PhysicsColliderActor::EndPlay();

    if (_shape)
    {
        // Detach from the actor
        void* actor = PhysicsBackend::GetShapeActor(_shape);
        RigidBody* rigidBody = GetAttachedRigidBody();
        if (actor)
            PhysicsBackend::DetachShape(_shape, actor);
        if (rigidBody)
        {
            rigidBody->OnColliderChanged(this);
        }
        else if (_staticActor)
        {
            RemoveStaticActor();
        }

        // Release shape
        PhysicsBackend::RemoveCollider(this);
        PhysicsBackend::DestroyShape(_shape);
        _shape = nullptr;
    }
}

void Collider::OnActiveInTreeChanged()
{
    // Base
    PhysicsColliderActor::OnActiveInTreeChanged();

    if (_shape)
    {
        PhysicsBackend::SetShapeState(_shape, IsActiveInHierarchy(), _isTrigger && CanBeTrigger());

        auto rigidBody = GetAttachedRigidBody();
        if (rigidBody)
        {
            rigidBody->OnColliderChanged(this);
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
        void* actor = PhysicsBackend::GetShapeActor(_shape);
        RigidBody* rigidBody = GetAttachedRigidBody();
        if (actor)
            PhysicsBackend::DetachShape(_shape, actor);
        if (rigidBody)
        {
            rigidBody->OnColliderChanged(this);
        }

        // Check if the new parent is a rigidbody
        rigidBody = dynamic_cast<RigidBody*>(GetParent());
        if (rigidBody && CanAttach(rigidBody))
        {
            // Attach to the rigidbody
            Attach(rigidBody);
        }
        else
        {
            // Use static actor (if not created yet)
            if (_staticActor == nullptr)
                CreateStaticActor();
            else
                PhysicsBackend::AttachShape(_shape, _staticActor);
        }
    }
}

void Collider::OnTransformChanged()
{
    // Base
    PhysicsColliderActor::OnTransformChanged();

    if (_staticActor)
    {
        PhysicsBackend::SetRigidActorPose(_staticActor, _transform.Translation, _transform.Orientation);
    }
    else if (const RigidBody* rigidBody = GetAttachedRigidBody())
    {
        const Vector3 localPosePos = (_localTransform.Translation + _localTransform.Orientation * _center) * rigidBody->GetScale();
        if (_cachedLocalPosePos != localPosePos || _cachedLocalPoseRot != _localTransform.Orientation)
        {
            _cachedLocalPosePos = localPosePos;
            _cachedLocalPoseRot = _localTransform.Orientation;
            PhysicsBackend::SetShapeLocalPose(_shape, localPosePos, _cachedLocalPoseRot);
        }
    }

    const float scale = GetScale().GetAbsolute().MaxValue();
    if (_cachedScale != scale)
        UpdateGeometry();
    UpdateBounds();
}

void Collider::OnLayerChanged()
{
    // Base
    PhysicsColliderActor::OnLayerChanged();

    if (_shape)
        UpdateLayerBits();
}

void Collider::OnStaticFlagsChanged()
{
    PhysicsColliderActor::OnStaticFlagsChanged();

    if (!_isTrigger && _isEnabled)
    {
        if (EnumHasAnyFlags(_staticFlags, StaticFlags::Navigation))
            GetScene()->Navigation.Actors.AddUnique(this);
        else
            GetScene()->Navigation.Actors.Remove(this);
    }
}

void Collider::OnPhysicsSceneChanged(PhysicsScene* previous)
{
    PhysicsColliderActor::OnPhysicsSceneChanged(previous);

    if (_staticActor != nullptr)
    {
        PhysicsBackend::RemoveSceneActor(previous->GetPhysicsScene(), _staticActor, true);
        void* scene = GetPhysicsScene()->GetPhysicsScene();
        PhysicsBackend::AddSceneActor(scene, _staticActor);
    }
}
