// Copyright (c) Wojciech Figat. All rights reserved.

#include "Joint.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Core/Log.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/PhysicsBackend.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Physics/Actors/IPhysicsActor.h"
#include "Engine/Physics/Actors/RigidBody.h"

#if USE_EDITOR
#include "Engine/Debug/DebugLog.h"
#include "Engine/Level/Scene/SceneRendering.h"
#endif

Joint::Joint(const SpawnParams& params)
    : Actor(params)
    , _joint(nullptr)
    , _breakForce(MAX_float)
    , _breakTorque(MAX_float)
    , LocalPoseActor0()
    , LocalPoseActor1()
{
    Actor0.Changed.Bind<Joint, &Joint::UpdateJointActors>(this);
    Actor1.Changed.Bind<Joint, &Joint::UpdateJointActors>(this);
}

void Joint::SetActors(RigidBody* Actor0, RigidBody* Actor1)
{
    if (_joint)
    {
        PhysicsBackend::SetJointActors
        (
            _joint,
            Actor0 ? Actor0->GetPhysicsActor() : nullptr,
            Actor1 ? Actor1->GetPhysicsActor() : nullptr
        );
    }
}
void Joint::SetLocalPoseActor0(const PhysicsTransform& LocalPose)
{
    if (_joint)
    {
        PhysicsBackend::SetJointLocalPose(_joint, 0, LocalPose);
    }
    LocalPoseActor0 = LocalPose;
}
PhysicsTransform Joint::GetLocalPoseActor0() const
{
    return LocalPoseActor0;
}
void Joint::SetPoseActor0(const PhysicsTransform& InPhysicsTransform)
{
    auto& pt = Actor0 ? PhysicsTransform::WorldToLocal(Actor0->GetTransform(), InPhysicsTransform) : InPhysicsTransform;
    SetLocalPoseActor0(pt);
}
PhysicsTransform Joint::GetPoseActor0() const
{
    if (Actor0) 
        return PhysicsTransform::LocalToWorld(Actor0->GetTransform(), LocalPoseActor0);
    return LocalPoseActor0;
}
void Joint::SetInvMassScaleActor0(float invMassScale)
{
    if (_joint)
    {
        PhysicsBackend::SetJointInvMassScale0(_joint, invMassScale); return;
    }
}
float Joint::GetInvMassScaleActor0() const
{
    if (_joint)
    {
        return PhysicsBackend::GetJointInvMassScale0(_joint);
    }
    return 0;
}
void Joint::SetInvInertiaScaleActor0(float invInertiaScale)
{
    if (_joint)
    {
        PhysicsBackend::SetJointInvInertiaScale0(_joint, invInertiaScale);
        return;
    }
}
float Joint::GetInvInertiaScaleActor0() const
{
    if (_joint)
    {
        return PhysicsBackend::GetJointInvInertiaScale0(_joint);
    }
    return 0;
}
void Joint::SetLocalPoseActor1(const PhysicsTransform& LocalPose)
{
    if (_joint)
    {
        PhysicsBackend::SetJointLocalPose(_joint, 1, LocalPose);
    }
    LocalPoseActor1 = LocalPose;
}
PhysicsTransform Joint::GetLocalPoseActor1() const
{
    return LocalPoseActor1;
}
void Joint::SetPoseActor1(const PhysicsTransform& InPhysicsTransform) 
{
    auto& pt = Actor1 ? PhysicsTransform::WorldToLocal(Actor1->GetTransform(), InPhysicsTransform) : InPhysicsTransform;
    SetLocalPoseActor1(pt);
}
PhysicsTransform Joint::GetPoseActor1() const 
{
    if (Actor1)
        return PhysicsTransform::LocalToWorld(Actor1->GetTransform(), LocalPoseActor1);
    return LocalPoseActor1;
}
void Joint::SetInvMassScaleActor1(float invMassScale)
{
    if (_joint) 
    {
        PhysicsBackend::SetJointInvMassScale1(_joint, invMassScale); return;
    }
}
float Joint::GetInvMassScaleActor1() const 
{
    if (_joint) 
    {
        return PhysicsBackend::GetJointInvMassScale1(_joint);
    } 
    return 0;
}
void Joint::SetInvInertiaScaleActor1(float invInertiaScale) 
{
    if (_joint) {
        PhysicsBackend::SetJointInvInertiaScale1(_joint, invInertiaScale); return;
    }
}
float Joint::GetInvInertiaScaleActor1() const 
{
    if (_joint)
    {
        return PhysicsBackend::GetJointInvInertiaScale1(_joint);
    }
    return 0;
}

PhysicsTransform Joint::GetRelativeTransform() const
{
    auto v = PhysicsTransform();
    if (_joint) PhysicsBackend::GetJointRelativeTransform(_joint, v);
    return v;
}
Vector3 Joint::GetRelativeLinearVelocity() const 
{
    auto v = Vector3();
    if (_joint) PhysicsBackend::GetJointRelativeLinearVelocity(_joint, v);
    return v;
}
Vector3 Joint::GetRelativeAngularVelocity() const
{
    auto v = Vector3();
    if (_joint) PhysicsBackend::GetJointRelativeAngularVelocity(_joint, v);
    return v;
}

void Joint::SetBreakForce(float force, float torque)
{
    _breakForce = force;
    _breakTorque = torque;
    if (_joint)
        PhysicsBackend::SetJointBreakForce(_joint, _breakForce, _breakTorque);
}

void Joint::SetBreakForce(float value)
{
    if (Math::NearEqual(value, _breakForce))
        return;
    _breakForce = value;
    if (_joint)
        PhysicsBackend::SetJointBreakForce(_joint, _breakForce, _breakTorque);
}

void Joint::SetBreakTorque(float value)
{
    if (Math::NearEqual(value, _breakTorque))
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

    if (_enableAutoAnchor)
    {
        if (Actor0 != nullptr)
        {
            SetPoseActor0(GetTransform());
            SetPoseActor1(GetTransform());
        }
    }
}

void* Joint::GetPhysicsImpl() const
{
    return _joint;
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

    if (GetEnableAutoAnchor())
    {
        SetPoseActor0(GetTransform());
        SetPoseActor1(GetTransform());
    }

    // Create joint object
    PhysicsJointDesc desc;
    desc.Joint = this;

    desc.Actor0 = Actor0 ? Actor0->GetPhysicsActor() : nullptr;
    desc.Actor1 = Actor1 ? Actor1->GetPhysicsActor() : nullptr;
    desc.LocalPoseActor0 = LocalPoseActor0;
    desc.LocalPoseActor1 = LocalPoseActor1;

    _joint = CreateJoint(desc);

    // Setup joint properties
    PhysicsBackend::SetJointBreakForce(_joint, _breakForce, _breakTorque);
    PhysicsBackend::SetJointFlags(_joint, _enableCollision ? PhysicsBackend::JointFlags::Collision : PhysicsBackend::JointFlags::None);
}

void Joint::OnJointBreak()
{
    JointBreak(this);
}

void Joint::UpdateJointActors()
{
    if (_joint)
    {
        SetActors(Actor0, Actor1);
        //refresh joints location
        SetLocalPoseActor0(LocalPoseActor0);
        SetLocalPoseActor1(LocalPoseActor1);
    }
}

void Joint::Delete()
{
    PhysicsBackend::RemoveJoint(this);
    PhysicsBackend::DestroyJoint(_joint);
    _joint = nullptr;
}

RigidBody* Joint::FindRigidbody() const
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

void Joint::Attach()
{
    // Check reparenting Joint case
    if (Actor0 == nullptr)
    {
        if (_joint)
        {
            // Remove joint
            Delete();
        }
    }
    else if (_joint)
    {
        UpdateJointActors();
    }
    else
    {
        // Create joint
        Create();
    }
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"
#include "Engine/Graphics/RenderView.h"

void Joint::DrawPhysicsDebug(RenderView& view)
{
    if (view.Mode == ViewMode::PhysicsColliders)
    {
        PhysicsTransform B{};
        PhysicsTransform A{};
        if (_joint) {
            PhysicsBackend::GetJointLocalPose(_joint, 0, A);
            PhysicsBackend::GetJointLocalPose(_joint, 1, B);
        }
        //cashe for cheak sync on A
        PhysicsTransform& fA = LocalPoseActor0;
        PhysicsTransform& pxA = A;
        //cashe for cheak sync on B
        PhysicsTransform& fB = LocalPoseActor1;
        PhysicsTransform& pxB = B;

        bool WarnA = false;
        bool WarnB = false;

        if (fA.Translation != pxA.Translation || fA.Orientation != pxA.Orientation)
        {
            WarnA = true;
        }
        if (fB.Translation != pxB.Translation || fB.Orientation != pxB.Orientation)
        {
            WarnA = true;
        }

        if (Actor0 != nullptr) 
        {
            A = PhysicsTransform::LocalToWorld(Actor0->GetTransform(), A);
        }

        if (Actor1 != nullptr) 
        {
            B = PhysicsTransform::LocalToWorld(Actor1->GetTransform(), B);
        }

        auto p = Vector3::Lerp(A.Translation, B.Translation, 0.5f);
        bool d = Vector3::Distance(view.WorldPosition, p) > 500.0f;

        DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(A.Translation, 3.0f), Color::Red * 0.8f, 0, d);
        DEBUG_DRAW_LINE(A.Translation, B.Translation, Color::Aqua, 0, d);
        DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(B.Translation, 4.0f), Color::Blue * 0.8f, 0, d);
        if (d)
        {
            if (!WarnA && !WarnB)
                return;

            String WarnMsg{ TEXT("Warning joint is not in sync with back end\n") };
            if (WarnA)
            {
                WarnMsg.Append(TEXT("A is off sync\nValues:\n"));
                WarnMsg.Append(A.Translation.ToString());
                WarnMsg.Append(TEXT("\n"));
                WarnMsg.Append(A.Orientation.GetEuler().ToString());
                WarnMsg.Append(TEXT("\n"));
            }
            if (WarnB)
            {
                WarnMsg.Append(TEXT("B is off sync\n"));

                WarnMsg.Append(B.Translation.ToString());
                WarnMsg.Append(TEXT("\n"));
                WarnMsg.Append(B.Orientation.GetEuler().ToString());
                WarnMsg.Append(TEXT("\n"));
            }
            DEBUG_DRAW_TEXT(WarnMsg, GetPosition(), Color::Yellow, 5, 0);

        }
    }
}

void Joint::OnDebugDrawSelected()
{
    auto p0 = GetPoseActor0();
    auto p1 = GetPoseActor1();
    if (Actor0) 
    {
        DEBUG_DRAW_WIRE_BOX(Actor0->GetBox(), Color::BlueViolet * 0.8f, 0, false);
    }
    if (Actor1) 
    {
        DEBUG_DRAW_WIRE_BOX(Actor1->GetBox(), Color::AliceBlue * 0.8f, 0, false);
    }
    DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(p0.Translation, 3.0f), Color::BlueViolet * 0.8f, 0, false);
    DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(p1.Translation, 4.0f), Color::AliceBlue * 0.8f, 0, false);
    DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(GetRelativeTransform().Translation, 4.0f), Color::AliceBlue * 0.8f, 0, false);

    // Base
    Actor::OnDebugDrawSelected();
}

#endif

void Joint::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(Joint);

    SERIALIZE_MEMBER(Source, Actor0);
    SERIALIZE_MEMBER(SourceAnchor, LocalPoseActor0.Translation);
    SERIALIZE_MEMBER(SourceAnchorRotation, LocalPoseActor0.Orientation);

    SERIALIZE_MEMBER(Target, Actor1);
    SERIALIZE_MEMBER(TargetAnchor, LocalPoseActor1.Translation);
    SERIALIZE_MEMBER(TargetAnchorRotation, LocalPoseActor1.Orientation);

    SERIALIZE_MEMBER(BreakForce, _breakForce);
    SERIALIZE_MEMBER(BreakTorque, _breakTorque);
    SERIALIZE_MEMBER(EnableCollision, _enableCollision);
    SERIALIZE_MEMBER(EnableAutoAnchor, _enableAutoAnchor);
}

void Joint::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Source, Actor0);
    DESERIALIZE_MEMBER(SourceAnchor, LocalPoseActor0.Translation);
    DESERIALIZE_MEMBER(SourceAnchorRotation, LocalPoseActor0.Orientation);

    DESERIALIZE_MEMBER(Target, Actor1);
    DESERIALIZE_MEMBER(TargetAnchor, LocalPoseActor1.Translation);
    DESERIALIZE_MEMBER(TargetAnchorRotation, LocalPoseActor1.Orientation);

    DESERIALIZE_MEMBER(BreakForce, _breakForce);
    DESERIALIZE_MEMBER(BreakTorque, _breakTorque);
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
            Attach();
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

    Attach();
}
void Joint::OnParentChangedInHierarchy()
{
    Actor::OnParentChangedInHierarchy();

    if (!IsDuringPlay())
        return;

    Attach();
}

void Joint::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    // TODO: this could track only local transform changed

    _box = BoundingBox(_transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);
}

#pragma region Deprecated
#if (FLAXENGINE_VERSION_MAJOR != 2 && (FLAXENGINE_VERSION_MINOR >= 0))

void Joint::SetTargetAnchor(const Vector3& value)
{
    LocalPoseActor1.Translation = value;
    SetLocalPoseActor1(LocalPoseActor0);
}

void Joint::SetTargetAnchorRotation(const Quaternion& value)
{
    LocalPoseActor1.Orientation = value;
    SetLocalPoseActor1(LocalPoseActor1);
}

void Joint::SetJointLocation(const Vector3& location)
{
}
void Joint::SetJointOrientation(const Quaternion& orientation)
{
}
#else
#ifndef STRING2
#define STRING2(x) #x
#define STRING(x) STRING2(x)
#endif
#pragma message ( __FILE__ "(" STRING(__LINE__) "):" "[Code Mantening] Remove Deprecated code")
#endif
#pragma endregion
