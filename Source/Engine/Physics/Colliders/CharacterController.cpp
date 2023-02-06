// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "CharacterController.h"
#include "Engine/Physics/Colliders/Collider.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/PhysicsBackend.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Engine/Time.h"

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
    , _nonWalkableMode(NonWalkableModes::PreventClimbing)
    , _lastFlags(CollisionFlags::None)
{
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
        const float minSize = 0.001f;
        const float height = Math::Max(Math::Abs(_height) * scaling, minSize);
        const float radius = Math::Max(Math::Abs(_radius) * scaling - contactOffset, minSize);
        PhysicsBackend::SetControllerStepOffset(_controller, Math::Min(value, height + radius * 2.0f - minSize));
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
    Vector3 displacement = speed;
    displacement += GetPhysicsScene()->GetGravity() * deltaTime;
    displacement *= deltaTime;
    return Move(displacement);
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
    const float minSize = 0.001f;
    const float radius = Math::Max(Math::Abs(_radius) * scaling, minSize);
    const float height = Math::Max(Math::Abs(_height) * scaling, minSize);
    const Vector3 position = _transform.LocalToWorld(_center);
    if (view.Mode == ViewMode::PhysicsColliders)
        DEBUG_DRAW_TUBE(position, Quaternion::Euler(90, 0, 0), radius, height, Color::LightYellow, 0, true);
    else
        DEBUG_DRAW_WIRE_TUBE(position, Quaternion::Euler(90, 0, 0), radius, height, Color::GreenYellow * 0.8f, 0, true);
}

void CharacterController::OnDebugDrawSelected()
{
    const float scaling = _cachedScale.GetAbsolute().MaxValue();
    const float minSize = 0.001f;
    const float radius = Math::Max(Math::Abs(_radius) * scaling, minSize);
    const float height = Math::Max(Math::Abs(_height) * scaling, minSize);
    const Vector3 position = _transform.LocalToWorld(_center);
    DEBUG_DRAW_WIRE_TUBE(position, Quaternion::Euler(90, 0, 0), radius, height, Color::GreenYellow, 0, false);

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
    _controller = PhysicsBackend::CreateController(GetPhysicsScene()->GetPhysicsScene(), this, this, _contactOffset, position, _slopeLimit, (int32)_nonWalkableMode, Material.Get(), Math::Abs(_radius) * scaling, Math::Abs(_height) * scaling, _stepOffset, _shape);

    // Setup
    PhysicsBackend::SetControllerUpDirection(_shape, _upDirection);
    PhysicsBackend::SetShapeLocalPose(_shape, _center, Quaternion::Identity);
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
        const float minSize = 0.001f;
        const float radius = Math::Max(Math::Abs(_radius) * scaling - Math::Max(_contactOffset, ZeroTolerance), minSize);
        const float height = Math::Max(Math::Abs(_height) * scaling, minSize);
        PhysicsBackend::SetControllerSize(_controller, radius, height);
    }
}

void CharacterController::CreateShape()
{
    // Not used
}

void CharacterController::UpdateBounds()
{
    void* actor = _shape ? PhysicsBackend::GetShapeActor(_shape) : nullptr;
    if (actor)
        PhysicsBackend::GetActorBounds(actor, _box);
    else
        _box = BoundingBox(_transform.Translation);
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
    Transform transform;
    PhysicsBackend::GetRigidActorPose(PhysicsBackend::GetShapeActor(_shape), transform.Translation, transform.Orientation);
    transform.Translation -= _center;
    transform.Orientation = _transform.Orientation;
    transform.Scale = _transform.Scale;
    SetTransform(transform);
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
    SERIALIZE_MEMBER(UpDirection, _upDirection);
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
    DESERIALIZE_MEMBER(UpDirection, _upDirection);
}
