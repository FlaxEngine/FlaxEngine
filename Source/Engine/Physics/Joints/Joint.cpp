// Copyright (c) Wojciech Figat. All rights reserved.

#include "Joint.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Core/Log.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/PhysicsBackend.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Physics/Actors/IPhysicsActor.h"
#if USE_EDITOR
#include "Engine/Level/Scene/SceneRendering.h"
#endif

Joint::Joint(const SpawnParams& params)
    : Actor(params)
    , _joint(nullptr)
    , _breakForce(MAX_float)
    , _breakTorque(MAX_float)
    , _targetAnchor(Vector3::Zero)
    , _targetAnchorRotation(Quaternion::Identity)
{
    Target.Changed.Bind<Joint, &Joint::OnTargetChanged>(this);
}

void Joint::SetBreakForce(float value)
{
    if (value == _breakForce)
        return;
    _breakForce = value;
    if (_joint)
        PhysicsBackend::SetJointBreakForce(_joint, _breakForce, _breakTorque);
}

void Joint::SetBreakTorque(float value)
{
    if (value == _breakTorque)
        return;
    _breakTorque = value;
    if (_joint)
        PhysicsBackend::SetJointBreakForce(_joint, _breakForce, _breakTorque);
}

void Joint::SetEnableCollision(bool value)
{
    if (value == GetEnableCollision())
        return;
    _enableCollision = value;
    if (_joint)
        PhysicsBackend::SetJointFlags(_joint, _enableCollision ? PhysicsBackend::JointFlags::Collision : PhysicsBackend::JointFlags::None);
}

bool Joint::GetEnableAutoAnchor() const
{
    return _enableAutoAnchor;
}

void Joint::SetEnableAutoAnchor(bool value)
{
    _enableAutoAnchor = value;
}

void Joint::SetTargetAnchor(const Vector3& value)
{
    if (value == _targetAnchor)
        return;
    _targetAnchor = value;
    if (_joint && !_enableAutoAnchor)
        PhysicsBackend::SetJointActorPose(_joint, _targetAnchor, _targetAnchorRotation, 1);
}

void Joint::SetTargetAnchorRotation(const Quaternion& value)
{
    if (Quaternion::NearEqual(value, _targetAnchorRotation))
        return;
    _targetAnchorRotation = value;
    if (_joint && !_enableAutoAnchor)
        PhysicsBackend::SetJointActorPose(_joint, _targetAnchor, _targetAnchorRotation, 1);
}

void* Joint::GetPhysicsImpl() const
{
    return _joint;
}

void Joint::SetJointLocation(const Vector3& location)
{
    if (GetParent())
    {
        SetLocalPosition(GetParent()->GetTransform().WorldToLocal(location));
    }
    if (Target)
    {
        SetTargetAnchor(Target->GetTransform().WorldToLocal(location));
    }
}

FORCE_INLINE Quaternion WorldToLocal(const Quaternion& world, const Quaternion& orientation)
{
    Quaternion rot;
    const Quaternion invRotation = world.Conjugated();
    Quaternion::Multiply(invRotation, orientation, rot);
    rot.Normalize();
    return rot;
}

void Joint::SetJointOrientation(const Quaternion& orientation)
{
    if (GetParent())
    {
        SetLocalOrientation(WorldToLocal(GetParent()->GetOrientation(), orientation));
    }
    if (Target)
    {
        SetTargetAnchorRotation(WorldToLocal(Target->GetOrientation(), orientation));
    }
}

void Joint::GetCurrentForce(Vector3& linear, Vector3& angular) const
{
    linear = angular = Vector3::Zero;
    if (_joint)
        PhysicsBackend::GetJointForce(_joint, linear, angular);
}

void Joint::Create()
{
    ASSERT(_joint == nullptr);

    auto parent = dynamic_cast<IPhysicsActor*>(GetParent());
    auto target = dynamic_cast<IPhysicsActor*>(Target.Get());
    if (parent == nullptr)
    {
        // Skip creation if joint is link to the not supported actor
        return;
    }

    // Create joint object
    PhysicsJointDesc desc;
    desc.Joint = this;
    desc.Actor0 = parent->GetPhysicsActor();
    desc.Actor1 = target ? target->GetPhysicsActor() : nullptr;
    desc.Pos0 = _localTransform.Translation;
    desc.Rot0 = _localTransform.Orientation;
    desc.Pos1 = _targetAnchor;
    desc.Rot1 = _targetAnchorRotation;
    if (_enableAutoAnchor && target)
    {
        // Place target anchor at the joint location
        desc.Pos1 = Target->GetTransform().WorldToLocal(GetPosition());
        desc.Rot1 = WorldToLocal(Target->GetOrientation(), GetOrientation());
    }
    _joint = CreateJoint(desc);

    // Setup joint properties
    PhysicsBackend::SetJointBreakForce(_joint, _breakForce, _breakTorque);
    PhysicsBackend::SetJointFlags(_joint, _enableCollision ? PhysicsBackend::JointFlags::Collision : PhysicsBackend::JointFlags::None);
}

void Joint::OnJointBreak()
{
    JointBreak();
}

void Joint::Delete()
{
    PhysicsBackend::RemoveJoint(this);
    PhysicsBackend::DestroyJoint(_joint);
    _joint = nullptr;
}

void Joint::SetActors()
{
    auto parent = dynamic_cast<IPhysicsActor*>(GetParent());
    auto target = dynamic_cast<IPhysicsActor*>(Target.Get());
    ASSERT(parent != nullptr);
    PhysicsBackend::SetJointActors(_joint, parent->GetPhysicsActor(), target ? target->GetPhysicsActor() : nullptr);
}

void Joint::OnTargetChanged()
{
    // Validate type
    const auto target = dynamic_cast<IPhysicsActor*>(Target.Get());
    if (Target && target == nullptr)
    {
        LOG(Error, "Invalid actor. Cannot use it as joint target. Rigidbodies and character controllers are supported. Object: {0}", Target.Get()->ToString());
        Target = nullptr;
    }
    else if (_joint)
    {
        SetActors();
    }
}

Vector3 Joint::GetTargetPosition() const
{
    Vector3 position = _targetAnchor;
    if (Target)
    {
        if (_enableAutoAnchor)
            position = Target->GetTransform().WorldToLocal(GetPosition());
        position = Target->GetOrientation() * position + Target->GetPosition();
    }
    return position;
}

Quaternion Joint::GetTargetOrientation() const
{
    Quaternion rotation = _targetAnchorRotation;
    if (Target)
    {
        if (_enableAutoAnchor)
            rotation = WorldToLocal(Target->GetOrientation(), GetOrientation());
        rotation = Target->GetOrientation() * rotation;
    }
    return rotation;
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"
#include "Engine/Graphics/RenderView.h"

void Joint::DrawPhysicsDebug(RenderView& view)
{
    if (view.Mode == ViewMode::PhysicsColliders)
    {
        DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(GetPosition(), 3.0f), Color::BlueViolet * 0.8f, 0, true);
        DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(GetTargetPosition(), 4.0f), Color::AliceBlue * 0.8f, 0, true);
    }
}

void Joint::OnDebugDrawSelected()
{
    DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(GetPosition(), 3.0f), Color::BlueViolet * 0.8f, 0, false);
    DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(GetTargetPosition(), 4.0f), Color::AliceBlue * 0.8f, 0, false);

    // Base
    Actor::OnDebugDrawSelected();
}

#endif

void Joint::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(Joint);

    SERIALIZE(Target);
    SERIALIZE_MEMBER(BreakForce, _breakForce);
    SERIALIZE_MEMBER(BreakTorque, _breakTorque);
    SERIALIZE_MEMBER(TargetAnchor, _targetAnchor);
    SERIALIZE_MEMBER(TargetAnchorRotation, _targetAnchorRotation);
    SERIALIZE_MEMBER(EnableCollision, _enableCollision);
    SERIALIZE_MEMBER(EnableAutoAnchor, _enableAutoAnchor);
}

void Joint::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE(Target);
    DESERIALIZE_MEMBER(BreakForce, _breakForce);
    DESERIALIZE_MEMBER(BreakTorque, _breakTorque);
    DESERIALIZE_MEMBER(TargetAnchor, _targetAnchor);
    DESERIALIZE_MEMBER(TargetAnchorRotation, _targetAnchorRotation);
    DESERIALIZE_MEMBER(EnableCollision, _enableCollision);
    DESERIALIZE_MEMBER(EnableAutoAnchor, _enableAutoAnchor);
}

void Joint::BeginPlay(SceneBeginData* data)
{
    // Base
    Actor::BeginPlay(data);

    // Create joint object only if it's enabled (otherwise it will be created in OnActiveInTreeChanged)
    if (IsActiveInHierarchy() && !_joint)
    {
        // Register for later init (joints are created after full beginPlay so all rigidbodies are created and can be linked)
        data->JointsToCreate.Add(this);
    }
}

void Joint::EndPlay()
{
    if (_joint)
    {
        Delete();
    }

    // Base
    Actor::EndPlay();
}

#if USE_EDITOR

void Joint::OnEnable()
{
    GetSceneRendering()->AddPhysicsDebug(this);

    // Base
    Actor::OnEnable();
}

void Joint::OnDisable()
{
    GetSceneRendering()->RemovePhysicsDebug(this);

    // Base
    Actor::OnDisable();
}

#endif

void Joint::OnActiveInTreeChanged()
{
    // Base
    Actor::OnActiveInTreeChanged();

    if (_joint)
    {
        // Enable/disable joint
        if (IsActiveInHierarchy())
            SetActors();
        else
            Delete();
    }
    // Joint object may not be created if actor is disabled on play mode begin (late init feature)
    else if (IsDuringPlay())
    {
        Create();
    }
}

void Joint::OnParentChanged()
{
    // Base
    Actor::OnParentChanged();

    if (!IsDuringPlay())
        return;

    // Check reparenting Joint case
    const auto parent = dynamic_cast<IPhysicsActor*>(GetParent());
    if (parent == nullptr)
    {
        if (_joint)
        {
            // Remove joint
            Delete();
        }
    }
    else if (_joint)
    {
        // Change target actor
        SetActors();
    }
    else
    {
        // Create joint
        Create();
    }
}

void Joint::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    // TODO: this could track only local transform changed

    _box = BoundingBox(_transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);
    if (_joint)
        PhysicsBackend::SetJointActorPose(_joint, _localTransform.Translation, _localTransform.Orientation, 0);
}
