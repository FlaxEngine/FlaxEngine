// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
    if (Vector3::NearEqual(value, _center))
        return;
    _center = value;
    if (_staticActor)
    {
        PhysicsBackend::SetShapeLocalPose(_shape, _center, Quaternion::Identity);
    }
    else if (CalculateShapeTransform())
    {
        PhysicsBackend::SetShapeLocalPose(_shape, _cachedLocalPosePos, _cachedLocalPoseRot);
    }
    UpdateBounds();
}

void Collider::SetContactOffset(float value)
{
    value = Math::Clamp(value, 0.0f, 100.0f);
    if (Math::NearEqual(value, _contactOffset))
        return;
    Internal_SetContactOffset(value);
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
        return AttachedTo;
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
    if (AttachedTo != rigidBody) 
    {
        AttachedTo = rigidBody;
    }
    if (CalculateShapeTransform())
    {
        PhysicsBackend::SetShapeLocalPose(_shape, _cachedLocalPosePos, _cachedLocalPoseRot);
    }
    rigidBody->AttachedColliders.Add(this);
    rigidBody->OnColliderChanged(this);
}
void Collider::Detach()
{
    void* actor = PhysicsBackend::GetShapeActor(_shape);
    RigidBody* rigidBody = GetAttachedRigidBody();
    if (actor)
        PhysicsBackend::DetachShape(_shape, actor);
    if (rigidBody)
    {
        rigidBody->OnColliderChanged(this);
        rigidBody->AttachedColliders.Remove(this);
        AttachedTo = nullptr;
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
    _cachedScale = GetScale();
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
    _cachedScale = GetScale();
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
            const auto rigidBody = GetAttachedRigidBody();
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
    PhysicsBackend::SetShapeLocalPose(_shape, _center, Quaternion::Identity);
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

RigidBody* Collider::GetAttachmentRigidBody()
{
    RigidBody* rb = nullptr;
    Actor* p = GetParent();
    while (p)
    {
        RigidBody* crb = dynamic_cast<RigidBody*>(p);
        if (crb)
        {
            rb = crb;
            break;
        }
        p = p->GetParent();
    }
    return rb;
}

void Collider::BeginPlay(SceneBeginData* data)
{
    // Check if has no shape created (it means no rigidbody requested it but also collider may be spawned at runtime)
    if (_shape == nullptr)
    {
        CreateShape();

        // Check if parent is a rigidbody
        const auto rigidBody = GetAttachmentRigidBody();
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
        Detach();

        if (_staticActor && GetAttachedRigidBody() == nullptr)
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
        Detach();

        // Check if the new parent is a rigidbody
        RigidBody* rigidBody = GetAttachmentRigidBody();
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
    else if (CalculateShapeTransform())
    {
        PhysicsBackend::SetShapeLocalPose(_shape, _cachedLocalPosePos, _cachedLocalPoseRot);
    }

    const Float3 scale = GetScale();
    if (!Float3::NearEqual(_cachedScale, scale))
        UpdateGeometry();
    UpdateBounds();
}

void Collider::OnParentChangedInHierarchy()
{
    Actor::OnParentChangedInHierarchy();
    OnParentChanged();
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

bool Collider::CalculateShapeTransform()
{
    const RigidBody* rigidBody = GetAttachedRigidBody();
    if (rigidBody == nullptr)
        return false;

    const Transform& T = rigidBody->GetTransform().WorldToLocal(GetTransform());
    const Vector3 localPosePos = (T.Translation + T.Orientation * _center) * rigidBody->GetScale();
    if (_cachedLocalPosePos != localPosePos || _cachedLocalPoseRot != T.Orientation)
    {
        _cachedLocalPosePos = localPosePos;
        _cachedLocalPoseRot = T.Orientation;
        return true;
    }
    return false;
}

void Collider::Internal_SetContactOffset(float value)
{
    _contactOffset = value;
    if (_shape)
        PhysicsBackend::SetShapeContactOffset(_shape, _contactOffset);
}
