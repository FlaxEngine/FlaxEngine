// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
    , LocalConstrainActorA()
    , LocalConstrainActorB()
{
    ConstraintActorA.Changed.Bind<Joint, &Joint::UpdateJointActors>(this);
    ConstraintActorB.Changed.Bind<Joint, &Joint::UpdateJointActors>(this);
}

const PhysicsTransform& Joint::GetLocalConstrainActorA() const
{
    return LocalConstrainActorA;
};
const PhysicsTransform& Joint::GetLocalConstrainActorB() const
{
    return LocalConstrainActorB;
};
PhysicsTransform        Joint::GetWorldConstrainActorA() const
{
    if (ConstraintActorA)
        return PhysicsTransform::LocalToWorld(ConstraintActorA->GetTransform(), LocalConstrainActorA);
    return LocalConstrainActorA;
};
PhysicsTransform        Joint::GetWorldConstrainActorB() const
{
    if (ConstraintActorB)
        return PhysicsTransform::LocalToWorld(ConstraintActorB->GetTransform(), LocalConstrainActorB);
    return LocalConstrainActorB;
};

void                    Joint::SetLocalConstrainActorA(const PhysicsTransform& InPhysicsTransform)
{
    if (_joint)
    {
        PhysicsBackend::SetJointActorPose(_joint, LocalConstrainActorA.Translation, LocalConstrainActorA.Orientation, 1);
    }
    LocalConstrainActorA = InPhysicsTransform;
};
void                    Joint::SetLocalConstrainActorB(const PhysicsTransform& InPhysicsTransform)
{
    if (_joint)
    {
        PhysicsBackend::SetJointActorPose(_joint, LocalConstrainActorB.Translation, LocalConstrainActorB.Orientation, 1);
    }
    LocalConstrainActorB = InPhysicsTransform;
};

void                    Joint::SetWorldConstrainActorA(const PhysicsTransform& InPhysicsTransform)
{
    auto& pt = ConstraintActorA ? PhysicsTransform::WorldToLocal(ConstraintActorA->GetTransform(), InPhysicsTransform) : InPhysicsTransform;
    SetLocalConstrainActorA(pt);
};
void                    Joint::SetWorldConstrainActorB(const PhysicsTransform& InPhysicsTransform)
{
    auto& pt = ConstraintActorB ? PhysicsTransform::WorldToLocal(ConstraintActorB->GetTransform(), InPhysicsTransform) : InPhysicsTransform;
    SetLocalConstrainActorB(pt);
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
        if (ConstraintActorB != nullptr)
        {
            SetWorldConstrainActorB(GetTransform());
            SetWorldConstrainActorA(GetTransform());
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

    if (ConstraintActorA == nullptr)
        ConstraintActorA = FindRigidbody();
    if (GetEnableAutoAnchor()) 
    {
        if (ConstraintActorB != nullptr)
        {
            SetWorldConstrainActorB(GetTransform());
            SetWorldConstrainActorA(GetTransform());
        }
    }

    // Create joint object
    PhysicsJointDesc desc;
    desc.Joint = this;

    desc.Actor0 = ConstraintActorA ? ConstraintActorA->GetPhysicsActor() : nullptr;
    desc.Actor1 = ConstraintActorB ? ConstraintActorB->GetPhysicsActor() : nullptr;
    desc.Pos0 = LocalConstrainActorA.Translation;
    desc.Rot0 = LocalConstrainActorA.Orientation;
    desc.Pos1 = LocalConstrainActorB.Translation;
    desc.Rot1 = LocalConstrainActorB.Orientation;

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
        PhysicsBackend::SetJointActors
        (
            _joint,
            ConstraintActorB ? ConstraintActorB->GetPhysicsActor() : nullptr,
            ConstraintActorB ? ConstraintActorB->GetPhysicsActor() : nullptr
        );
        //refresh joints location
        SetLocalConstrainActorA(LocalConstrainActorA);
        SetLocalConstrainActorB(LocalConstrainActorB);
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
    if (ConstraintActorA == nullptr)
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
            PhysicsBackend::GetJointActorPose(_joint, A.Translation, A.Orientation, 0);
            PhysicsBackend::GetJointActorPose(_joint, B.Translation, B.Orientation, 1);
        }
        //cashe for cheak sync on A
        PhysicsTransform& fA = LocalConstrainActorA;
        PhysicsTransform& pxA = A;
        //cashe for cheak sync on B
        PhysicsTransform& fB = LocalConstrainActorB;
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

        if (ConstraintActorA != nullptr) 
        {
            A = PhysicsTransform::LocalToWorld(ConstraintActorA->GetTransform(), A);
        }

        if (ConstraintActorB != nullptr) 
        {
            B = PhysicsTransform::LocalToWorld(ConstraintActorB->GetTransform(), B);
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
    auto wcaa = GetWorldConstrainActorA();
    auto wcab = GetWorldConstrainActorB();

    DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(wcaa.Translation, 3.0f), Color::BlueViolet * 0.8f, 0, false);
    DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(wcab.Translation, 4.0f), Color::AliceBlue * 0.8f, 0, false);

    // Base
    Actor::OnDebugDrawSelected();
}

#endif

void Joint::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(Joint);

    SERIALIZE_MEMBER(Source, ConstraintActorA);
    SERIALIZE_MEMBER(SourceAnchor, LocalConstrainActorA.Translation);
    SERIALIZE_MEMBER(SourceAnchorRotation, LocalConstrainActorA.Orientation);

    SERIALIZE_MEMBER(Target, ConstraintActorB);
    SERIALIZE_MEMBER(TargetAnchor, LocalConstrainActorB.Translation);
    SERIALIZE_MEMBER(TargetAnchorRotation, LocalConstrainActorB.Orientation);

    SERIALIZE_MEMBER(BreakForce, _breakForce);
    SERIALIZE_MEMBER(BreakTorque, _breakTorque);
    SERIALIZE_MEMBER(EnableCollision, _enableCollision);
    SERIALIZE_MEMBER(EnableAutoAnchor, _enableAutoAnchor);
}

void Joint::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Source, ConstraintActorA);
    DESERIALIZE_MEMBER(SourceAnchor, LocalConstrainActorA.Translation);
    DESERIALIZE_MEMBER(SourceAnchorRotation, LocalConstrainActorA.Orientation);

    DESERIALIZE_MEMBER(Target, ConstraintActorB);
    DESERIALIZE_MEMBER(TargetAnchor, LocalConstrainActorB.Translation);
    DESERIALIZE_MEMBER(TargetAnchorRotation, LocalConstrainActorB.Orientation);

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
}

void Joint::SetTargetAnchorRotation(const Quaternion& value)
{

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
