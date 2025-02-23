// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "CharacterController.h"
#include "Engine/Physics/Colliders/Collider.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/PhysicsBackend.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Engine/Time.h"

#define CC_MIN_SIZE 0.001f

CharacterController::CharacterController(const SpawnParams& params)
    : Collider(params)
    , _controller(nullptr)
    , _stepOffset(30.0f)
    , _slopeLimit(45.0f)
    , _radius(50.0f)
    , _height(150.0f)
    , _minMoveDistance(0.0f)
    , _isUpdatingTransform(false)
    , _upDirection(Vector3::Up)
    , _gravityDisplacement(Vector3::Zero)
    , _nonWalkableMode(NonWalkableModes::PreventClimbing)
    , _lastFlags(CollisionFlags::None)
{
    _contactOffset = 10.0f;
}

float CharacterController::GetRadius() const
{
    return _radius;
}

void CharacterController::SetRadius(const float value)
{
    if (Math::NearEqual(value, _radius))
        return;

    _radius = value;

    UpdateSize();
    UpdateBounds();
}

float CharacterController::GetHeight() const
{
    return _height;
}

void CharacterController::SetHeight(const float value)
{
    if (Math::NearEqual(value, _height))
        return;

    _height = value;

    UpdateSize();
    UpdateBounds();
}

float CharacterController::GetSlopeLimit() const
{
    return _slopeLimit;
}

void CharacterController::SetSlopeLimit(float value)
{
    value = Math::Clamp(value, 0.0f, 89.0f);
    if (Math::NearEqual(value, _slopeLimit))
        return;
    _slopeLimit = value;
    if (_controller)
        PhysicsBackend::SetControllerSlopeLimit(_controller, value);
}

CharacterController::NonWalkableModes CharacterController::GetNonWalkableMode() const
{
    return _nonWalkableMode;
}

void CharacterController::SetNonWalkableMode(NonWalkableModes value)
{
    if (_nonWalkableMode == value)
        return;
    _nonWalkableMode = value;
    if (_controller)
        PhysicsBackend::SetControllerNonWalkableMode(_controller, (int32)value);
}

float CharacterController::GetStepOffset() const
{
    return _stepOffset;
}

void CharacterController::SetStepOffset(float value)
{
    if (Math::NearEqual(value, _stepOffset))
        return;

    _stepOffset = value;

    if (_controller)
    {
        const float scaling = _cachedScale.GetAbsolute().MaxValue();
        const float contactOffset = Math::Max(_contactOffset, ZeroTolerance);
        const float height = Math::Max(Math::Abs(_height) * scaling, CC_MIN_SIZE);
        const float radius = Math::Max(Math::Abs(_radius) * scaling - contactOffset, CC_MIN_SIZE);
        PhysicsBackend::SetControllerStepOffset(_controller, Math::Min(value, height + radius * 2.0f - CC_MIN_SIZE));
    }
}

void CharacterController::SetUpDirection(const Vector3& up)
{
    _upDirection = up;
    if (_controller)
        PhysicsBackend::SetControllerUpDirection(_controller, up);
}

float CharacterController::GetMinMoveDistance() const
{
    return _minMoveDistance;
}

Vector3 CharacterController::GetUpDirection() const
{
    return _controller ? PhysicsBackend::GetControllerUpDirection(_controller) : _upDirection;
}

void CharacterController::SetMinMoveDistance(float value)
{
    _minMoveDistance = Math::Max(value, 0.0f);
}

Vector3 CharacterController::GetVelocity() const
{
    return _controller ? PhysicsBackend::GetRigidDynamicActorLinearVelocity(PhysicsBackend::GetControllerRigidDynamicActor(_controller)) : Vector3::Zero;
}

bool CharacterController::IsGrounded() const
{
    return (static_cast<int>(_lastFlags) & static_cast<int>(CollisionFlags::Below)) != 0;
}

CharacterController::CollisionFlags CharacterController::GetFlags() const
{
    return _lastFlags;
}

CharacterController::CollisionFlags CharacterController::SimpleMove(const Vector3& speed)
{
    const float deltaTime = Time::GetCurrentSafe()->DeltaTime.GetTotalSeconds();
    Vector3 displacement = speed + _gravityDisplacement;
    CollisionFlags result = Move(displacement * deltaTime);
    if ((static_cast<int>(result) & static_cast<int>(CollisionFlags::Below)) != 0)
    {
        // Reset accumulated gravity acceleration when we touch the ground
        _gravityDisplacement = Vector3::Zero;
    }
    else
        _gravityDisplacement += GetPhysicsScene()->GetGravity() * deltaTime;
    return result;
}

CharacterController::CollisionFlags CharacterController::Move(const Vector3& displacement)
{
    CollisionFlags result = CollisionFlags::None;
    if (_controller)
    {
        const float deltaTime = Time::GetCurrentSafe()->DeltaTime.GetTotalSeconds();
        result = (CollisionFlags)PhysicsBackend::MoveController(_controller, _shape, displacement, _minMoveDistance, deltaTime);
        _lastFlags = result;
        Vector3 position = PhysicsBackend::GetControllerPosition(_controller) - _center;
        SetPosition(position);
    }
    return result;
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"
#include "Engine/Graphics/RenderView.h"

void CharacterController::DrawPhysicsDebug(RenderView& view)
{
    const float scaling = _cachedScale.GetAbsolute().MaxValue();
    const float radius = Math::Max(Math::Abs(_radius) * scaling, CC_MIN_SIZE);
    const float height = Math::Max(Math::Abs(_height) * scaling, CC_MIN_SIZE);
    const Vector3 position = _transform.LocalToWorld(_center);
    if (view.Mode == ViewMode::PhysicsColliders)
        DEBUG_DRAW_CAPSULE(position, Quaternion::Euler(90, 0, 0), radius, height, Color::LightYellow, 0, true);
    else
        DEBUG_DRAW_WIRE_CAPSULE(position, Quaternion::Euler(90, 0, 0), radius, height, Color::GreenYellow * 0.8f, 0, true);
}

void CharacterController::OnDebugDrawSelected()
{
    const float scaling = _cachedScale.GetAbsolute().MaxValue();
    const float radius = Math::Max(Math::Abs(_radius) * scaling, CC_MIN_SIZE);
    const float height = Math::Max(Math::Abs(_height) * scaling, CC_MIN_SIZE);
    const Vector3 position = _transform.LocalToWorld(_center);
    DEBUG_DRAW_WIRE_CAPSULE(position, Quaternion::Euler(90, 0, 0), radius, height, Color::GreenYellow, 0, false);

    // Base
    Collider::OnDebugDrawSelected();
}

#endif

void CharacterController::CreateController()
{
    // Create controller
    ASSERT(_controller == nullptr && _shape == nullptr);
    _cachedScale = GetScale();
    const float scaling = _cachedScale.GetAbsolute().MaxValue();
    const Vector3 position = _transform.LocalToWorld(_center);
    _controller = PhysicsBackend::CreateController(GetPhysicsScene()->GetPhysicsScene(), this, this, _contactOffset, position, _slopeLimit, (int32)_nonWalkableMode, Material, Math::Abs(_radius) * scaling, Math::Abs(_height) * scaling, _stepOffset, _shape);

    // Setup
    PhysicsBackend::SetControllerUpDirection(_controller, _upDirection);
    PhysicsBackend::SetShapeLocalPose(_shape, Vector3::Zero, Quaternion::Identity);
    UpdateLayerBits();
    UpdateBounds();
}

void CharacterController::DeleteController()
{
    if (_controller)
    {
        PhysicsBackend::DestroyController(_controller);
        _controller = nullptr;
        _shape = nullptr;
    }
}

void CharacterController::UpdateSize() const
{
    if (_controller)
    {
        const float scaling = _cachedScale.GetAbsolute().MaxValue();
        const float radius = Math::Max(Math::Abs(_radius) * scaling - Math::Max(_contactOffset, ZeroTolerance), CC_MIN_SIZE);
        const float height = Math::Max(Math::Abs(_height) * scaling, CC_MIN_SIZE);
        PhysicsBackend::SetControllerSize(_controller, radius, height);
    }
}

void CharacterController::CreateShape()
{
    // Not used
}

void CharacterController::UpdateBounds()
{
    const float scaling = GetScale().GetAbsolute().MaxValue();
    const float radius = Math::Max(Math::Abs(_radius) * scaling, CC_MIN_SIZE);
    const float height = Math::Max(Math::Abs(_height) * scaling, CC_MIN_SIZE);
    const Vector3 position = _transform.LocalToWorld(_center);
    const Vector3 extent(radius, height * 0.5f + radius, radius);
    _box = BoundingBox(position - extent, position + extent);
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

void CharacterController::OnActiveTransformChanged()
{
    if (!_shape)
        return;

    // Change actor transform (but with locking)
    ASSERT(!_isUpdatingTransform);
    _isUpdatingTransform = true;
    const Vector3 position = PhysicsBackend::GetControllerPosition(_controller) - _center;
    SetPosition(position);
    _isUpdatingTransform = false;

    UpdateBounds();
}

void* CharacterController::GetPhysicsActor() const
{
    return _shape ? PhysicsBackend::GetShapeActor(_shape) : nullptr;
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

void CharacterController::GetGeometry(CollisionShape& collision)
{
    // Unused
}

void CharacterController::BeginPlay(SceneBeginData* data)
{
    if (IsActiveInHierarchy())
        CreateController();

    // Skip collider base
    Actor::BeginPlay(data);
}

void CharacterController::EndPlay()
{
    // Skip collider base
    Actor::EndPlay();

    // Remove controller
    DeleteController();
}

void CharacterController::OnActiveInTreeChanged()
{
    // Skip collider base
    Actor::OnActiveInTreeChanged();
}

void CharacterController::OnEnable()
{
    if (_controller == nullptr)
        CreateController();

    Collider::OnEnable();
}

void CharacterController::OnDisable()
{
    Collider::OnDisable();

    DeleteController();
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
    const Vector3 position = _transform.LocalToWorld(_center);
    if (!_isUpdatingTransform && _controller)
    {
        PhysicsBackend::SetControllerPosition(_controller, position);
        const Float3 scale = GetScale();
        if (!Float3::NearEqual(_cachedScale, scale))
            UpdateGeometry();
        UpdateBounds();
    }
    else if (!_controller)
    {
        _box = BoundingBox(position);
        BoundingSphere::FromBox(_box, _sphere);
    }
}

void CharacterController::OnPhysicsSceneChanged(PhysicsScene* previous)
{
    Collider::OnPhysicsSceneChanged(previous);

    DeleteController();
    CreateController();
}
