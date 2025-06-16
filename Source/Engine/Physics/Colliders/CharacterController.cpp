// Copyright (c) Wojciech Figat. All rights reserved.

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
    , _originMode(OriginModes::CapsuleCenter)
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
    if (value == _radius)
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
    if (value == _height)
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
    if (value == _slopeLimit)
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

CharacterController::OriginModes CharacterController::GetOriginMode() const
{
    return _originMode;
}

void CharacterController::SetOriginMode(OriginModes value)
{
    if (_originMode == value)
        return;
    _originMode = value;
    if (_controller)
    {
        DeleteController();
        CreateController();
    }
}

float CharacterController::GetStepOffset() const
{
    return _stepOffset;
}

void CharacterController::SetStepOffset(float value)
{
    if (value == _stepOffset)
        return;
    _stepOffset = value;
    if (_controller)
    {
        float height, radius;
        GetControllerSize(height, radius);
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

void CharacterController::SetAutoGravity(bool value)
{
    _autoGravity = value;
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
    if (_controller && !_isUpdatingTransform)
    {
        // Perform move
        const float deltaTime = Time::GetCurrentSafe()->DeltaTime.GetTotalSeconds();
        result = (CollisionFlags)PhysicsBackend::MoveController(_controller, _shape, displacement, _minMoveDistance, deltaTime);
        _lastFlags = result;

        // Update position
        Vector3 position;
        if (_originMode == OriginModes::Base)
            position = PhysicsBackend::GetControllerBasePosition(_controller);
        else
            position = PhysicsBackend::GetControllerPosition(_controller);
        position -= _center;
        _isUpdatingTransform = true;
        SetPosition(position);
        _isUpdatingTransform = false;
    }
    return result;
}

void CharacterController::Resize(float height, float radius)
{
    const float heightDiff = height - _height;
    const float radiusDiff = radius - _radius;
    if (Math::IsZero(heightDiff) && Math::IsZero(radiusDiff))
        return;
    _height = height;
    _radius = radius;
    if (_controller)
    {
        float centerDiff = heightDiff * 0.5f + radiusDiff;

        // Change physics size
        GetControllerSize(height, radius);
        PhysicsBackend::SetControllerSize(_controller, radius, height);
        Vector3 positionDelta = _upDirection * centerDiff;

        // Change physics position to maintain feet placement (base)
        Vector3 position;
        switch (_originMode)
        {
        case OriginModes::CapsuleCenter:
            position = PhysicsBackend::GetControllerPosition(_controller);
            position += positionDelta;
            _center += positionDelta;
            PhysicsBackend::SetControllerPosition(_controller, position);
            break;
        case OriginModes::Base:
            position = PhysicsBackend::GetControllerBasePosition(_controller);
            position += positionDelta;
            PhysicsBackend::SetControllerBasePosition(_controller, position);
            break;
        }

        // Change actor position
        _isUpdatingTransform = true;
        SetPosition(position - _center);
        _isUpdatingTransform = false;
    }
    UpdateBounds();
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"
#include "Engine/Graphics/RenderView.h"

void CharacterController::DrawPhysicsDebug(RenderView& view)
{
    Quaternion rotation = Quaternion::Euler(90, 0, 0);
    const Vector3 position = GetControllerPosition();
    if (view.Mode == ViewMode::PhysicsColliders)
        DEBUG_DRAW_CAPSULE(position, rotation, _radius, _height, Color::LightYellow, 0, true);
    else
        DEBUG_DRAW_WIRE_CAPSULE(position, rotation, _radius, _height, Color::GreenYellow * 0.8f, 0, true);
}

void CharacterController::OnDebugDrawSelected()
{
    Quaternion rotation = Quaternion::Euler(90, 0, 0);
    const Vector3 position = GetControllerPosition();
    DEBUG_DRAW_WIRE_CAPSULE(position, rotation, _radius, _height, Color::GreenYellow, 0, false);
    if (_controller)
    {
        // Physics backend capsule shape
        float height, radius;
        GetControllerSize(height, radius);
        Vector3 pos = PhysicsBackend::GetControllerPosition(_controller);
        DEBUG_DRAW_WIRE_CAPSULE(pos, rotation, radius, height, Color::Blue.AlphaMultiplied(0.2f), 0, false);
#if 0
        // More technical visuals debugging
        Vector3 base = PhysicsBackend::GetControllerBasePosition(_controller);
        DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(base, 5.0f), Color::Red, 0, false);
        DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(pos, 4.0f), Color::Red, 0, false);
        DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(pos + Vector3(0, height * 0.5f, 0), 2.0f), Color::Red, 0, false);
        DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(pos - Vector3(0, height * 0.5f, 0), 2.0f), Color::Red, 0, false);
        DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(pos + Vector3(0, height * 0.5f, 0), radius), Color::Red.AlphaMultiplied(0.5f), 0, false);
        DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(pos - Vector3(0, height * 0.5f, 0), radius), Color::Red.AlphaMultiplied(0.5f), 0, false);
        DEBUG_DRAW_WIRE_CYLINDER(pos, Quaternion::Identity, radius, height, Color::Red.AlphaMultiplied(0.2f), 0, false);
#endif
    }

    // Base
    Collider::OnDebugDrawSelected();
}

#endif

void CharacterController::CreateController()
{
    // Create controller
    ASSERT(_controller == nullptr && _shape == nullptr);
    _cachedScale = GetScale().GetAbsolute().MaxValue();
    float height, radius;
    GetControllerSize(height, radius);
    Vector3 position = _center;
    if (_originMode == OriginModes::Base)
        position += _upDirection * (_height * 0.5f + _radius);
    position = _transform.LocalToWorld(position);
    _controller = PhysicsBackend::CreateController(GetPhysicsScene()->GetPhysicsScene(), this, this, _contactOffset, position, _slopeLimit, (int32)_nonWalkableMode, Material, radius, height, _stepOffset, _shape);

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
        float height, radius;
        GetControllerSize(height, radius);
        PhysicsBackend::SetControllerSize(_controller, radius, height);
    }
}

Vector3 CharacterController::GetControllerPosition() const
{
    Vector3 position = _center;
    if (_originMode == OriginModes::Base)
        position += _upDirection * (_height * 0.5f + _radius);
    position = _transform.LocalToWorld(position);
    return position;
}

void CharacterController::GetControllerSize(float& height, float& radius) const
{
    // Use absolute values including scale
    height = Math::Abs(_height) * _cachedScale;
    radius = Math::Abs(_radius) * _cachedScale;

    // Exclude contact offset around the capsule (otherwise character floats in the air)
    radius = radius - Math::Max(_contactOffset, 0.0f);

    // Prevent too small controllers
    height = Math::Max(height, CC_MIN_SIZE);
    radius = Math::Max(radius, CC_MIN_SIZE);
}

void CharacterController::CreateShape()
{
    // Not used
}

void CharacterController::UpdateBounds()
{
    _cachedScale = GetScale().GetAbsolute().MaxValue();
    float height, radius;
    GetControllerSize(height, radius);
    const Vector3 position = GetControllerPosition();
    const Vector3 extent(radius, height * 0.5f + radius, radius);
    _box = BoundingBox(position - extent, position + extent);
    BoundingSphere::FromBox(_box, _sphere);
}

void CharacterController::AddMovement(const Vector3& translation, const Quaternion& rotation)
{
    Vector3 displacement = translation;
    if (_autoGravity)
    {
        // Apply gravity
        const float deltaTime = Time::GetCurrentSafe()->DeltaTime.GetTotalSeconds();
        displacement += GetPhysicsScene()->GetGravity() * deltaTime;
    }

    Move(displacement);

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

void CharacterController::SetCenter(const Vector3& value)
{
    if (value == _center)
        return;
    Vector3 delta = value - _center;
    _center = value;
    if (_controller)
    {
        // Change physics position while maintaining actor placement
        Vector3 position = PhysicsBackend::GetControllerPosition(_controller);
        position += _upDirection * delta;
        PhysicsBackend::SetControllerPosition(_controller, position);
    }
}

void CharacterController::OnActiveTransformChanged()
{
    if (!_shape)
        return;

    // Change actor transform (but with locking)
    ASSERT(!_isUpdatingTransform);
    _isUpdatingTransform = true;
    Vector3 position;
    if (_originMode == OriginModes::Base)
        position = PhysicsBackend::GetControllerBasePosition(_controller);
    else
        position = PhysicsBackend::GetControllerPosition(_controller);
    position -= _center;
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
    _cachedScale = GetScale().GetAbsolute().MaxValue();
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
        if (_originMode == OriginModes::Base)
            PhysicsBackend::SetControllerBasePosition(_controller, position);
        else
            PhysicsBackend::SetControllerPosition(_controller, position);
        const float scale = GetScale().GetAbsolute().MaxValue();
        if (_cachedScale != scale)
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
