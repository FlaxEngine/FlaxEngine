// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"
#include "Engine/Physics/Types.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Physics/PhysicsTransform.h"

#include "FlaxEngine.Gen.h"

class IPhysicsActor;

#define Export(...)
#define Property
#define Getter
#define Setter
#define EditorOrder(order)
#define EditorDisplay(Category)
#define VisibleIf(var,swich)

/// <summary>
/// A base class for all Joint types. Joints constrain how two rigidbodies move relative to one another (for example a door hinge).
/// One of the bodies in the joint must always be movable (non-kinematic and non-static).
/// </summary>
/// <remarks>
/// Joint constraint is created between the parent physic actor (rigidbody, character controller, etc.) and the specified target actor.
/// </remarks>
/// <seealso cref="Actor" />
API_CLASS(Abstract) class FLAXENGINE_API Joint : public Actor
{
    DECLARE_SCENE_OBJECT_ABSTRACT(Joint);
protected:
    void* _joint;
    float _breakForce;
    float _breakTorque;

    PhysicsTransform LocalPoseActor0;
    PhysicsTransform LocalPoseActor1;

    bool _enableCollision = true;
    bool _enableAutoAnchor = false;
public:
#pragma region Actor0
    /// <summary>
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(0), EditorDisplay(\"Joint\")")
        ScriptingObjectReference<RigidBody> Actor0;

    /// <summary>
    /// Set the joint local pose for an actor.
    /// This is the relative pose which locates the joint frame relative to the actor.
    /// </summary>
    /// <param name="LocalPose">localPose the local pose for the actor this joint.</param>
    API_PROPERTY(Attributes = "EditorOrder(1), EditorDisplay(\"Joint\"),VisibleIf(nameof(EnableAutoAnchor), true)")
        void SetLocalPoseActor0(const PhysicsTransform& LocalPose);

    /// <summary>
    /// Get the joint local pose for an actor.
    /// </summary>
    /// <returns>The local pose for this joint</returns>
    API_PROPERTY()
        PhysicsTransform GetLocalPoseActor0() const;

    /// <summary>
    /// Set the joint local pose for an actor.
    /// This is the relative pose which locates the joint frame relative to the actor.
    /// </summary>
    /// <param name="LocalPose">localPose the local pose for the actor this joint.</param>
    API_PROPERTY(Attributes = "HideInEditor")
        void SetPoseActor0(const PhysicsTransform& LocalPose);

    /// <summary>
    /// Get the joint local pose for an actor.
    /// </summary>
    /// <returns>The local pose for this joint</returns>
    API_PROPERTY()
        PhysicsTransform GetPoseActor0() const;

    /// <summary>
    /// set the inverse mass scale for actor0.
    /// </summary>
    /// <param name="invMassScale">invMassScale the scale to apply to the inverse mass of actor 0 for resolving this constraint</param>
    API_PROPERTY(Attributes = "HideInEditor")
        void SetInvMassScaleActor0(float invMassScale);

    /// <summary>
    /// get the inverse mass scale for actor0.
    /// </summary>
    /// <returns>inverse mass scale for actor0</returns>
    API_PROPERTY()
        float GetInvMassScaleActor0() const;

    /// <summary>
    /// set the inverse inertia scale for actor0.
    /// </summary>
    /// <param name="invInertiaScale">invInertiaScale the scale to apply to the inverse inertia of actor0 for resolving this constraint.</param>
    API_PROPERTY(Attributes = "HideInEditor")
        void SetInvInertiaScaleActor0(float invInertiaScale);

    /// <summary>
    /// Get the inverse inertia scale for actor0.
    /// </summary>
    /// <returns>inverse inertia scale for actor0</returns>
    API_PROPERTY()
        float GetInvInertiaScaleActor0() const;
#pragma endregion
#pragma region Actor1
    /// <summary>
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(2), EditorDisplay(\"Joint\")")
        ScriptingObjectReference<RigidBody> Actor1;

    /// <summary>
    /// Set the joint local pose for an actor.
    /// This is the relative pose which locates the joint frame relative to the actor.
    /// </summary>
    /// <param name="LocalPose">localPose the local pose for the actor this joint.</param>
    API_PROPERTY(Attributes = "EditorOrder(3), EditorDisplay(\"Joint\"),VisibleIf(nameof(EnableAutoAnchor), true)")
        void SetLocalPoseActor1(const PhysicsTransform& LocalPose);

    /// <summary>
    /// Get the joint local pose for an actor.
    /// </summary>
    /// <returns>The local pose for this joint</returns>
    API_PROPERTY() PhysicsTransform GetLocalPoseActor1() const;

    /// <summary>
    /// Set the joint local pose for an actor.
    /// This is the relative pose which locates the joint frame relative to the actor.
    /// </summary>
    /// <param name="LocalPose">localPose the local pose for the actor this joint.</param>
    API_PROPERTY(Attributes = "HideInEditor") void SetPoseActor1(const PhysicsTransform& LocalPose);

    /// <summary>
    /// Get the joint local pose for an actor.
    /// </summary>
    /// <returns>The local pose for this joint</returns>
    API_PROPERTY() PhysicsTransform GetPoseActor1() const;

    /// <summary>
    /// set the inverse mass scale for actor0.
    /// </summary>
    /// <param name="invMassScale">invMassScale the scale to apply to the inverse mass of actor 0 for resolving this constraint</param>
    API_PROPERTY(Attributes = "HideInEditor") void SetInvMassScaleActor1(float invMassScale);

    /// <summary>
    /// get the inverse mass scale for actor0.
    /// </summary>
    /// <returns>inverse mass scale for actor0</returns>
    API_PROPERTY() float GetInvMassScaleActor1() const;

    /// <summary>
    /// set the inverse inertia scale for actor0.
    /// </summary>
    /// <param name="invInertiaScale">invInertiaScale the scale to apply to the inverse inertia of actor0 for resolving this constraint.</param>
    API_PROPERTY(Attributes = "HideInEditor") void SetInvInertiaScaleActor1(float invInertiaScale);

    /// <summary>
    /// Get the inverse inertia scale for actor0.
    /// </summary>
    /// <returns>inverse inertia scale for actor0</returns>
    API_PROPERTY() float GetInvInertiaScaleActor1() const;
#pragma endregion
public:
    /// <summary>
    /// Gets the break force. Determines the maximum force the joint can apply before breaking. Broken joints no longer participate in physics simulation.
    /// </summary>
    API_PROPERTY(Attributes = "EditorOrder(10), DefaultValue(float.MaxValue), EditorDisplay(\"Joint\"), ValueCategory(Utils.ValueCategory.Force)")
        FORCE_INLINE float GetBreakForce() const
    {
        return _breakForce;
    }

    /// <summary>
    /// Sets the break force. Determines the maximum force the joint can apply before breaking. Broken joints no longer participate in physics simulation.
    /// </summary>
    API_PROPERTY() void SetBreakForce(float value);

    /// <summary>
    /// Gets the break torque. Determines the maximum torque the joint can apply before breaking. Broken joints no longer participate in physics simulation.
    /// </summary>
    API_PROPERTY(Attributes = "EditorOrder(20), DefaultValue(float.MaxValue), EditorDisplay(\"Joint\"), ValueCategory(Utils.ValueCategory.Torque)")
        FORCE_INLINE float GetBreakTorque() const
    {
        return _breakTorque;
    }

    /// <summary>
    /// Sets the break torque. Determines the maximum torque the joint can apply before breaking. Broken joints no longer participate in physics simulation.
    /// </summary>
    API_PROPERTY() void SetBreakTorque(float value);

    /// <summary>
    /// Determines whether collision between the two bodies managed by the joint are enabled.
    /// </summary>
    API_PROPERTY(Attributes = "EditorOrder(30), DefaultValue(true), EditorDisplay(\"Joint\")")
        FORCE_INLINE bool GetEnableCollision() const
    {
        return _enableCollision;
    }

    /// <summary>
    /// Determines whether collision between the two bodies managed by the joint are enabled.
    /// </summary>
    API_PROPERTY() void SetEnableCollision(bool value);

    /// <summary>
    /// Determines whether use automatic target anchor position and rotation based on the joint world-space frame (computed when creating joint).
    /// </summary>
    API_PROPERTY(Attributes = "EditorOrder(39), DefaultValue(false), EditorDisplay(\"Joint\")")
        bool GetEnableAutoAnchor() const;

    /// <summary>
    /// Determines whether use automatic target anchor position and rotation based on the joint world-space frame (computed when creating joint).
    /// </summary>
    API_PROPERTY() void SetEnableAutoAnchor(bool value);

public:
    /// <summary>
    /// Gets the native physics backend object.
    /// </summary>
    void* GetPhysicsImpl() const;

    /// <summary>
    /// Gets the current force applied by the solver to maintain all constraints.
    /// </summary>
    /// <param name="linear">The result linear force.</param>
    /// <param name="angular">The result angular force.</param>
    API_FUNCTION() void GetCurrentForce(API_PARAM(Out) Vector3& linear, API_PARAM(Out) Vector3& angular) const;

    /// <summary>
    /// Creates native join object.
    /// </summary>
    void Create();

    /// <summary>
    /// Occurs when a joint gets broken during simulation.
    /// </summary>
    API_EVENT() Delegate<ScriptingObjectReference<Joint>> JointBreak;

    /// <summary>
    /// Called by the physics system when joint gets broken.
    /// </summary>
    virtual void OnJointBreak();

public:

    /// <summary>
    /// Set the break force for this joint.
    /// if the constraint force or torque on the joint exceeds the specified values, the joint will break,
    /// at which point it will not constrain the two actors 
    /// The force and torque are measured in the joint frame of the first actor
    /// </summary>
    /// <param name="force">force the maximum force the joint can apply before breaking.</param>
    /// <param name="torque">torque the maximum torque the joint can apply before breaking.</param>
    API_FUNCTION() void SetBreakForce(float force, float torque);

    /// <summary>
    /// Set the actors for this joint.
    /// An actor may be null to indicate the world frame. At most one of the actors may be null.
    /// </summary>
    /// <param name="Actor0">actor0 the first actor.</param>
    /// <param name="Actor1">actor1 the second actor.</param>
    API_FUNCTION() void SetActors(RigidBody* Actor0, RigidBody* Actor1);

    /// <summary>
    /// Get the relative pose for this joint
    /// </summary>
    /// <returns>Pose of the joint frame of actor1 relative to actor0</returns>
    API_FUNCTION() PhysicsTransform GetRelativeTransform() const;

    /// <summary>
    /// Get the relative linear velocity of the joint
    /// </summary>
    /// <returns>
    /// This function returns the linear velocity of the origin of the constraint frame of actor1, relative to the origin of the constraint
    /// frame of actor0.The value is returned in the constraint frame of actor0
    /// </returns>
    API_FUNCTION() Vector3 GetRelativeLinearVelocity() const;

    /// <summary>
    /// get the relative angular velocity of the joint
    /// </summary>
    /// <returns>
    /// This function returns the angular velocity of actor1 relative to actor0. The value is returned in the constraint frame of actor0
    /// </returns>
    API_FUNCTION() Vector3 GetRelativeAngularVelocity() const;

protected:
    virtual void* CreateJoint(const struct PhysicsJointDesc& desc) = 0;
#if USE_EDITOR
    virtual void DrawPhysicsDebug(RenderView& view);
#endif
private:
    void UpdateJointActors();

    void Delete();
    /// <summary>
    /// looks for rigid body in the parents
    /// </summary>
    RigidBody* FindRigidbody() const;
    void Attach();
public:
    // [Actor]
#if USE_EDITOR
    void OnDebugDrawSelected() override;
#endif
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;

protected:
    // [Actor]
    void BeginPlay(SceneBeginData* data) override;
    void EndPlay() override;
#if USE_EDITOR
    void OnEnable() override;
    void OnDisable() override;
#endif
    void OnActiveInTreeChanged() override;
    void OnParentChanged() override;
    void OnParentChangedInHierarchy() override;
    void OnTransformChanged() override;

public:
#if (FLAXENGINE_VERSION_MAJOR != 2 && FLAXENGINE_VERSION_MINOR >= 0)

    /// <summary>
    /// [Deprecated in v1.9, removed in v2.0]
    /// </summary>
    API_PROPERTY() DEPRECATED Vector3 GetTargetAnchor() const
    {
        return LocalPoseActor1.Translation;
    }

    /// <summary>
    /// [Deprecated in v1.9, removed in v2.0]
    /// </summary>
    API_PROPERTY() DEPRECATED void SetTargetAnchor(const Vector3& value);

    /// <summary>
    /// [Deprecated in v1.9, removed in v2.0]
    /// </summary>
    API_PROPERTY() DEPRECATED Quaternion GetTargetAnchorRotation() const
    {
        return LocalPoseActor1.Orientation;
    }

    /// <summary>
    /// [Deprecated in v1.9, removed in v2.0]
    /// </summary>
    API_PROPERTY() DEPRECATED void SetTargetAnchorRotation(const Quaternion& value);

    /// <summary>
    /// [Deprecated in v1.9, removed in v2.0]
    /// </summary>
    API_FIELD() DEPRECATED ScriptingObjectReference<Actor> Target;

    /// <summary>
    /// [Deprecated in v1.9, removed in v2.0]
    /// </summary>
    API_FUNCTION() DEPRECATED void SetJointLocation(const Vector3& location);

    /// <summary>
    /// [Deprecated in v1.9, removed in v2.0]
    /// </summary>
    API_FUNCTION() DEPRECATED void SetJointOrientation(const Quaternion& orientation);
#else
#ifndef STRING2
#define STRING2(x) #x
#define STRING(x) STRING2(x)
#endif
#pragma message ( __FILE__ "(" STRING(__LINE__) "):" "[Code Mantening] Remove Deprecated code")
#endif
};
