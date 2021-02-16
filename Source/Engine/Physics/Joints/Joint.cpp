// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Joint.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Core/Log.h"
#include "Engine/Physics/Utilities.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/Actors/IPhysicsActor.h"
#if USE_EDITOR
#include "Engine/Level/Scene/SceneRendering.h"
#endif
#include "Engine/Scripting/Script.h"
#include <ThirdParty/PhysX/extensions/PxJoint.h>

#define PX_PARENT_ACTOR_ID PxJointActorIndex::eACTOR0
#define PX_TARGET_ACTOR_ID PxJointActorIndex::eACTOR1

Joint::Joint(const SpawnParams& params)
    : Actor(params)
    , _joint(nullptr)
    , _breakForce(MAX_float)
    , _breakTorque(MAX_float)
    , _targetAnchor(Vector3::Zero)
    , _targetAnchorRotation(Quaternion::Identity)
    , _enableCollision(true)
{
    Target.Changed.Bind<Joint, &Joint::OnTargetChanged>(this);
}

void Joint::SetBreakForce(float value)
{
    if (Math::NearEqual(value, _breakForce))
        return;

    _breakForce = value;

    if (_joint)
    {
        _joint->setBreakForce(_breakForce, _breakTorque);
    }
}

void Joint::SetBreakTorque(float value)
{
    if (Math::NearEqual(value, _breakTorque))
        return;

    _breakTorque = value;

    if (_joint)
    {
        _joint->setBreakForce(_breakForce, _breakTorque);
    }
}

void Joint::SetEnableCollision(bool value)
{
    if (value == GetEnableCollision())
        return;

    _enableCollision = value;

    if (_joint)
    {
        _joint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, value);
    }
}

void Joint::SetTargetAnchor(const Vector3& value)
{
    if (Vector3::NearEqual(value, _targetAnchor))
        return;

    _targetAnchor = value;

    if (_joint)
    {
        _joint->setLocalPose(PX_TARGET_ACTOR_ID, PxTransform(C2P(_targetAnchor), C2P(_targetAnchorRotation)));
    }
}

void Joint::SetTargetAnchorRotation(const Quaternion& value)
{
    if (Quaternion::NearEqual(value, _targetAnchorRotation))
        return;

    _targetAnchorRotation = value;

    if (_joint)
    {
        _joint->setLocalPose(PX_TARGET_ACTOR_ID, PxTransform(C2P(_targetAnchor), C2P(_targetAnchorRotation)));
    }
}

void Joint::GetCurrentForce(Vector3& linear, Vector3& angular) const
{
    if (_joint && _joint->getConstraint())
    {
        _joint->getConstraint()->getForce(*(PxVec3*)&linear, *(PxVec3*)&angular);
    }
    else
    {
        linear = angular = Vector3::Zero;
    }
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
    JointData data;
    data.Physics = Physics::GetPhysics();
    data.Actor0 = parent ? parent->GetRigidActor() : nullptr;
    data.Actor1 = target ? target->GetRigidActor() : nullptr;
    data.Pos0 = _localTransform.Translation;
    data.Rot0 = _localTransform.Orientation;
    data.Pos1 = _targetAnchor;
    data.Rot1 = _targetAnchorRotation;
    _joint = CreateJoint(data);
    _joint->userData = this;

    // Setup joint properties
    _joint->setBreakForce(_breakForce, _breakTorque);
    _joint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, _enableCollision);
}

void Joint::OnJointBreak()
{
    JointBreak();
}

void Joint::Delete()
{
    // Remove the joint
    Physics::RemoveJoint(this);
    _joint->userData = nullptr;
    _joint->release();
    _joint = nullptr;
}

void Joint::SetActors()
{
    auto parent = dynamic_cast<IPhysicsActor*>(GetParent());
    auto target = dynamic_cast<IPhysicsActor*>(Target.Get());
    ASSERT(parent != nullptr);

    _joint->setActors(parent ? parent->GetRigidActor() : nullptr, target ? target->GetRigidActor() : nullptr);
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

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void Joint::DrawPhysicsDebug(RenderView& view)
{
    DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(GetPosition(), 3.0f), Color::BlueViolet * 0.8f, 0, true);
    if (Target)
    {
        DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(Target->GetPosition() + _targetAnchor, 4.0f), Color::AliceBlue * 0.8f, 0, true);
    }
}

void Joint::OnDebugDrawSelected()
{
    DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(GetPosition(), 3.0f), Color::BlueViolet * 0.8f, 0, false);
    if (Target)
    {
        DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(Target->GetPosition() + _targetAnchor, 4.0f), Color::AliceBlue * 0.8f, 0, false);
    }

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
    GetSceneRendering()->AddPhysicsDebug<Joint, &Joint::DrawPhysicsDebug>(this);

    // Base
    Actor::OnEnable();
}

void Joint::OnDisable()
{
    GetSceneRendering()->RemovePhysicsDebug<Joint, &Joint::DrawPhysicsDebug>(this);

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

    _box = BoundingBox(_transform.Translation, _transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);

    if (_joint)
    {
        _joint->setLocalPose(PX_PARENT_ACTOR_ID, PxTransform(C2P(_localTransform.Translation), C2P(_localTransform.Orientation)));
    }
}
