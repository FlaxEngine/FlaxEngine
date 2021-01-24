// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "CharacterController.h"
#include "Engine/Physics/Utilities.h"
#include "Engine/Physics/Colliders/Collider.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Engine/Time.h"
#include "Engine/Physics/PhysicalMaterial.h"
#if USE_EDITOR
#include "Engine/Level/Scene/SceneRendering.h"
#endif
#include <ThirdParty/PhysX/PxRigidActor.h>
#include <ThirdParty/PhysX/PxRigidDynamic.h>
#include <ThirdParty/PhysX/PxPhysics.h>
#include <ThirdParty/PhysX/characterkinematic/PxController.h>
#include <ThirdParty/PhysX/characterkinematic/PxControllerManager.h>
#include <ThirdParty/PhysX/characterkinematic//PxCapsuleController.h>

CharacterController::CharacterController(const SpawnParams& params)
    : Collider(params)
    , _controller(nullptr)
    , _stepOffset(30.0f)
    , _slopeLimit(45.0f)
    , _radius(50.0f)
    , _height(150.0f)
    , _minMoveDistance(0.0f)
    , _isUpdatingTransform(false)
    , _nonWalkableMode(CharacterController::NonWalkableModes::PreventClimbing)
    , _lastFlags(CollisionFlags::None)
{
}

void CharacterController::SetRadius(const float value)
{
    if (Math::NearEqual(value, _radius))
        return;

    _radius = value;

    UpdateSize();
    UpdateBounds();
}

void CharacterController::SetHeight(const float value)
{
    if (Math::NearEqual(value, _height))
        return;

    _height = value;

    UpdateSize();
    UpdateBounds();
}

void CharacterController::SetSlopeLimit(float value)
{
    value = Math::Clamp(value, 0.0f, 89.0f);
    if (Math::NearEqual(value, _slopeLimit))
        return;

    _slopeLimit = value;

    if (_controller)
        _controller->setSlopeLimit(cosf(value * DegreesToRadians));
}

void CharacterController::SetNonWalkableMode(NonWalkableModes value)
{
    _nonWalkableMode = value;

    if (_controller)
        _controller->setNonWalkableMode(static_cast<PxControllerNonWalkableMode::Enum>(value));
}

void CharacterController::SetStepOffset(float value)
{
    if (Math::NearEqual(value, _stepOffset))
        return;

    _stepOffset = value;

    if (_controller)
        _controller->setStepOffset(value);
}

void CharacterController::SetMinMoveDistance(float value)
{
    _minMoveDistance = Math::Max(value, 0.0f);
}

Vector3 CharacterController::GetVelocity() const
{
    return _controller ? P2C(_controller->getActor()->getLinearVelocity()) : Vector3::Zero;
}

CharacterController::CollisionFlags CharacterController::SimpleMove(const Vector3& speed)
{
    const float deltaTime = Time::GetCurrentSafe()->DeltaTime.GetTotalSeconds();
    Vector3 displacement = speed;
    displacement += Physics::GetGravity() * deltaTime;
    displacement *= deltaTime;

    return Move(displacement);
}

CharacterController::CollisionFlags CharacterController::Move(const Vector3& displacement)
{
    CollisionFlags result = CollisionFlags::None;

    if (_controller)
    {
        const float deltaTime = Time::GetCurrentSafe()->DeltaTime.GetTotalSeconds();
        PxControllerFilters filters;
        filters.mFilterData = &_filterData;
        filters.mFilterCallback = Physics::GetCharacterQueryFilterCallback();
        filters.mFilterFlags = PxQueryFlag::eDYNAMIC | PxQueryFlag::eSTATIC | PxQueryFlag::ePREFILTER;

        result = (CollisionFlags)(byte)_controller->move(C2P(displacement), _minMoveDistance, deltaTime, filters);
        _lastFlags = result;

        SetPosition(P2C(_controller->getPosition()));
    }

    return result;
}

PxRigidDynamic* CharacterController::GetPhysXRigidActor() const
{
    return _controller ? _controller->getActor() : nullptr;
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void CharacterController::DrawPhysicsDebug(RenderView& view)
{
    const float scaling = _cachedScale.GetAbsolute().MaxValue();
    const float minSize = 0.001f;
    const float radius = Math::Max(Math::Abs(_radius) * scaling, minSize);
    const float height = Math::Max(Math::Abs(_height) * scaling, minSize);
    DEBUG_DRAW_WIRE_TUBE(_transform.LocalToWorld(_center), Quaternion::Euler(90, 0, 0), radius, height, Color::GreenYellow * 0.8f, 0, true);
}

void CharacterController::OnDebugDrawSelected()
{
    const float scaling = _cachedScale.GetAbsolute().MaxValue();
    const float minSize = 0.001f;
    const float radius = Math::Max(Math::Abs(_radius) * scaling, minSize);
    const float height = Math::Max(Math::Abs(_height) * scaling, minSize);
    DEBUG_DRAW_WIRE_TUBE(_transform.LocalToWorld(_center), Quaternion::Euler(90, 0, 0), radius, height, Color::GreenYellow, 0, false);

    // Base
    Collider::OnDebugDrawSelected();
}

#endif

void CharacterController::CreateActor()
{
    ASSERT(_controller == nullptr && _shape == nullptr);

    PxCapsuleControllerDesc desc;
    desc.userData = this;
    desc.contactOffset = Math::Max(_contactOffset, ZeroTolerance);
    desc.position = PxExtendedVec3(_transform.Translation.X, _transform.Translation.Y, _transform.Translation.Z);
    desc.slopeLimit = Math::Cos(_slopeLimit * DegreesToRadians);
    desc.nonWalkableMode = static_cast<PxControllerNonWalkableMode::Enum>(_nonWalkableMode);
    desc.climbingMode = PxCapsuleClimbingMode::eEASY;
    desc.stepOffset = _stepOffset;
    if (Material && !Material->WaitForLoaded())
        desc.material = ((PhysicalMaterial*)Material->Instance)->GetPhysXMaterial();
    else
        desc.material = Physics::GetDefaultMaterial();
    _cachedScale = GetScale();
    const float scaling = _cachedScale.GetAbsolute().MaxValue();
    const float minSize = 0.001f;
    desc.height = Math::Max(Math::Abs(_height) * scaling, minSize);
    desc.radius = Math::Max(Math::Abs(_radius) * scaling - desc.contactOffset, minSize);

    // Create controller
    _controller = (PxCapsuleController*)Physics::GetControllerManager()->createController(desc);
    ASSERT(_controller);
    const auto actor = _controller->getActor();
    ASSERT(actor && actor->getNbShapes() == 1);
    actor->getShapes(&_shape, 1);
    actor->userData = this;
    _shape->userData = this;

    // Apply additional shape properties
    _shape->setLocalPose(PxTransform(C2P(_center)));
    UpdateLayerBits();

    // Update cached data
    UpdateBounds();
}

void CharacterController::UpdateSize() const
{
    if (_controller)
    {
        const float scaling = _cachedScale.GetAbsolute().MaxValue();
        const float minSize = 0.001f;
        const float radius = Math::Max(Math::Abs(_radius) * scaling - Math::Max(_contactOffset, ZeroTolerance), minSize);
        const float height = Math::Max(Math::Abs(_height) * scaling, minSize);

        _controller->setRadius(radius);
        _controller->resize(height);
    }
}

void CharacterController::CreateShape()
{
    // Not used
}

void CharacterController::UpdateBounds()
{
    const auto actor = GetPhysXRigidActor();
    const float boundsScale = 1.03f;
    if (actor)
        _box = P2C(actor->getWorldBounds(boundsScale));
    else
        _box = BoundingBox(_transform.Translation, _transform.Translation);
    BoundingSphere::FromBox(_box, _sphere);
}

void CharacterController::AddMovement(const Vector3& translation, const Quaternion& rotation)
{
    Move(translation);

    if (!rotation.IsIdentity())
    {
        SetOrientation(GetOrientation() * rotation);
    }
}

bool CharacterController::CanAttach(RigidBody* rigidBody) const
{
    return false;
}

RigidBody* CharacterController::GetAttachedRigidBody() const
{
    return nullptr;
}

void CharacterController::OnActiveTransformChanged(const PxTransform& transform)
{
    // Change actor transform (but with locking)
    ASSERT(!_isUpdatingTransform);
    _isUpdatingTransform = true;
    Transform t = _transform;
    t.Translation = P2C(transform.p);
    SetTransform(t);
    _isUpdatingTransform = false;

    UpdateBounds();
}

PxRigidActor* CharacterController::GetRigidActor()
{
    return _shape ? _shape->getActor() : nullptr;
}

void CharacterController::UpdateGeometry()
{
    // Check if has no character created
    if (_shape == nullptr)
        return;

    // Setup shape geometry
    _cachedScale = GetScale();
    UpdateSize();
}

void CharacterController::UpdateLayerBits()
{
    // Base
    Collider::UpdateLayerBits();

    // Cache filter data
    _filterData = _shape->getSimulationFilterData();
}

void CharacterController::BeginPlay(SceneBeginData* data)
{
    CreateActor();

    // Skip collider base
    Actor::BeginPlay(data);
}

void CharacterController::EndPlay()
{
    // Skip collider base
    Actor::EndPlay();

    // Remove controller
    if (_controller)
    {
        _shape->userData = nullptr;
        _controller->getActor()->userData = nullptr;
        _controller->release();
        _controller = nullptr;
    }
    _shape = nullptr;
}

#if USE_EDITOR

void CharacterController::OnEnable()
{
    GetSceneRendering()->AddPhysicsDebug<CharacterController, &CharacterController::DrawPhysicsDebug>(this);

    // Base
    Collider::OnEnable();
}

void CharacterController::OnDisable()
{
    GetSceneRendering()->RemovePhysicsDebug<CharacterController, &CharacterController::DrawPhysicsDebug>(this);

    // Base
    Collider::OnDisable();
}

#endif

void CharacterController::OnActiveInTreeChanged()
{
    // Skip collider base
    Actor::OnActiveInTreeChanged();

    // Clear velocities and the forces on disabled
    if (!IsActiveInHierarchy() && _controller)
    {
        // TODO: sleep actor? clear forces?
    }
}

void CharacterController::OnParentChanged()
{
    // Skip collider base
    Actor::OnParentChanged();
}

void CharacterController::OnTransformChanged()
{
    // Skip collider base
    Actor::OnTransformChanged();

    // Update physics
    if (!_isUpdatingTransform && _controller)
    {
        const PxExtendedVec3 pos(_transform.Translation.X, _transform.Translation.Y, _transform.Translation.Z);
        _controller->setPosition(pos);

        UpdateScale();
        UpdateBounds();
    }
}

void CharacterController::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Collider::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(CharacterController);

    SERIALIZE_MEMBER(StepOffset, _stepOffset);
    SERIALIZE_MEMBER(SlopeLimit, _slopeLimit);
    SERIALIZE_MEMBER(NonWalkableMode, _nonWalkableMode);
    SERIALIZE_MEMBER(Radius, _radius);
    SERIALIZE_MEMBER(Height, _height);
    SERIALIZE_MEMBER(MinMoveDistance, _minMoveDistance);
}

void CharacterController::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Collider::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(StepOffset, _stepOffset);
    DESERIALIZE_MEMBER(SlopeLimit, _slopeLimit);
    DESERIALIZE_MEMBER(NonWalkableMode, _nonWalkableMode);
    DESERIALIZE_MEMBER(Radius, _radius);
    DESERIALIZE_MEMBER(Height, _height);
    DESERIALIZE_MEMBER(MinMoveDistance, _minMoveDistance);
}
