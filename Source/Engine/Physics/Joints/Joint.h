// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"
#include "Engine/Physics/Types.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Physics/PhysicsTransform.h"

#include "FlaxEngine.Gen.h"

class IPhysicsActor;

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

    PhysicsTransform LocalConstrainActorA;
    PhysicsTransform LocalConstrainActorB;

    bool _enableCollision = true;
    bool _enableAutoAnchor = false;
    /// <summary>
    /// propery for editor dont use it
    /// </summary>
    /// <returns></returns>
    API_PROPERTY(internal) bool ShowConstrainActorA() const { return  ConstraintActorA.Get() == nullptr; }
public:

    /// <summary>
    /// [todo]
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(0), EditorDisplay(\"Joint\")")
        ScriptingObjectReference<RigidBody> ConstraintActorA;
    /// <summary>
    /// [todo]
    /// </summary>
    API_PROPERTY(Attributes = "EditorOrder(1), EditorDisplay(\"Joint\"),VisibleIf(nameof(ShowConstrainActorA), false)")
    const PhysicsTransform& GetLocalConstrainActorA() const;

    /// <summary>
    /// [todo]
    /// </summary>
    API_PROPERTY() void SetLocalConstrainActorA(const PhysicsTransform& InPhysicsTransform);

    /// <summary>
    /// [todo]
    /// </summary>
    API_FUNCTION() PhysicsTransform GetWorldConstrainActorA() const;

    /// <summary>
    /// [todo]
    /// </summary>
    API_FUNCTION() void SetWorldConstrainActorA(const PhysicsTransform& InPhysicsTransform);


    /// <summary>
    /// [todo]
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(2), DefaultValue(null), EditorDisplay(\"Joint\")")
        ScriptingObjectReference<RigidBody> ConstraintActorB;

    /// <summary>
    /// [todo]
    /// </summary>
    API_PROPERTY(Attributes = "EditorOrder(3), EditorDisplay(\"Joint\"),VisibleIf(nameof(EnableAutoAnchor), true)")
        const PhysicsTransform& GetLocalConstrainActorB() const;

    /// <summary>
    /// [todo]
    /// </summary>
    API_PROPERTY() void SetLocalConstrainActorB(const PhysicsTransform& InPhysicsTransform);

    /// <summary>
    /// [todo]
    /// </summary>
    API_FUNCTION()
        PhysicsTransform GetWorldConstrainActorB() const;
    /// <summary>
    /// [todo]
    /// </summary>
    API_FUNCTION() void SetWorldConstrainActorB(const PhysicsTransform& InPhysicsTransform);;

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
        return LocalConstrainActorB.Translation;
    }

    /// <summary>
    /// [Deprecated in v1.9, removed in v2.0]
    /// </summary>
    API_PROPERTY() void SetTargetAnchor(const Vector3& value);

    /// <summary>
    /// [Deprecated in v1.9, removed in v2.0]
    /// </summary>
    API_PROPERTY() DEPRECATED Quaternion GetTargetAnchorRotation() const
    {
        return LocalConstrainActorB.Orientation;
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
