// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "PhysicsActor.h"
#include "Engine/Physics/Collisions.h"

class PhysicsColliderActor;

/// <summary>
/// Physics simulation driven object.
/// </summary>
/// <seealso cref="PhysicsActor" />
API_CLASS() class FLAXENGINE_API RigidBody : public PhysicsActor
{
DECLARE_SCENE_OBJECT(RigidBody);
protected:

    PxRigidDynamic* _actor;
    Vector3 _cachedScale;

    float _mass;
    float _linearDamping;
    float _angularDamping;
    float _maxAngularVelocity;
    float _massScale;
    Vector3 _centerOfMassOffset;
    RigidbodyConstraints _constraints;

    int32 _enableSimulation : 1;
    int32 _isKinematic : 1;
    int32 _useCCD : 1;
    int32 _enableGravity : 1;
    int32 _startAwake : 1;
    int32 _updateMassWhenScaleChanges : 1;
    int32 _overrideMass : 1;

public:

    /// <summary>
    /// Enables kinematic mode for the rigidbody.
    /// </summary>
    /// <remarks>
    /// Kinematic rigidbodies are special dynamic actors that are not influenced by forces(such as gravity), and have no momentum.
    /// They are considered to have infinite mass and can push regular dynamic actors out of the way.
    /// Kinematics will not collide with static or other kinematic objects.
    /// <para>
    /// Kinematic rigidbodies are great for moving platforms or characters, where direct motion control is desired.
    /// </para>
    /// <para>
    /// Kinematic rigidbodies are incompatible with CCD.
    /// </para>
    /// </remarks>
    /// <returns>The value.</returns>
    API_PROPERTY(Attributes="EditorOrder(10), DefaultValue(false), EditorDisplay(\"Rigid Body\")")
    FORCE_INLINE bool GetIsKinematic() const
    {
        return _isKinematic != 0;
    }

    /// <summary>
    /// Enables kinematic mode for the rigidbody.
    /// </summary>
    /// <remarks>
    /// Kinematic rigidbodies are special dynamic actors that are not influenced by forces(such as gravity), and have no momentum.
    /// They are considered to have infinite mass and can push regular dynamic actors out of the way.
    /// Kinematics will not collide with static or other kinematic objects.
    /// <para>
    /// Kinematic rigidbodies are great for moving platforms or characters, where direct motion control is desired.
    /// </para>
    /// <para>
    /// Kinematic rigidbodies are incompatible with CCD.
    /// </para>
    /// </remarks>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetIsKinematic(const bool value);

    /// <summary>
    /// Gets the 'drag' force added to reduce linear movement.
    /// </summary>
    /// <remarks>
    /// Linear damping can be used to slow down an object. The higher the drag the more the object slows down.
    /// </remarks>
    /// <returns>The value.</returns>
    API_PROPERTY(Attributes="EditorOrder(60), DefaultValue(0.01f), Limit(0), EditorDisplay(\"Rigid Body\")")
    FORCE_INLINE float GetLinearDamping() const
    {
        return _linearDamping;
    }

    /// <summary>
    /// Sets the 'drag' force added to reduce linear movement.
    /// </summary>
    /// <remarks>
    /// Linear damping can be used to slow down an object. The higher the drag the more the object slows down.
    /// </remarks>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetLinearDamping(float value);

    /// <summary>
    /// Gets the 'drag' force added to reduce angular movement.
    /// </summary>
    /// <remarks>
    /// Angular damping can be used to slow down the rotation of an object. The higher the drag the more the rotation slows down.
    /// </remarks>
    /// <returns>The value.</returns>
    API_PROPERTY(Attributes="EditorOrder(70), DefaultValue(0.05f), Limit(0), EditorDisplay(\"Rigid Body\")")
    FORCE_INLINE float GetAngularDamping() const
    {
        return _angularDamping;
    }

    /// <summary>
    /// Sets the 'drag' force added to reduce angular movement.
    /// </summary>
    /// <remarks>
    /// Angular damping can be used to slow down the rotation of an object. The higher the drag the more the rotation slows down.
    /// </remarks>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetAngularDamping(float value);

    /// <summary>
    /// If true simulation and collisions detection will be enabled for the rigidbody.
    /// </summary>
    /// <returns>The value.</returns>
    API_PROPERTY(Attributes="EditorOrder(20), DefaultValue(true), EditorDisplay(\"Rigid Body\")")
    FORCE_INLINE bool GetEnableSimulation() const
    {
        return _enableSimulation != 0;
    }

    /// <summary>
    /// If true simulation and collisions detection will be enabled for the rigidbody.
    /// </summary>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetEnableSimulation(bool value);

    /// <summary>
    /// If true Continuous Collision Detection (CCD) will be used for this component.
    /// </summary>
    /// <returns>The value.</returns>
    API_PROPERTY(Attributes="EditorOrder(30), DefaultValue(false), EditorDisplay(\"Rigid Body\", \"Use CCD\")")
    FORCE_INLINE bool GetUseCCD() const
    {
        return _useCCD != 0;
    }

    /// <summary>
    /// If true Continuous Collision Detection (CCD) will be used for this component.
    /// </summary>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetUseCCD(const bool value);

    /// <summary>
    /// If object should have the force of gravity applied.
    /// </summary>
    /// <returns>The value.</returns>
    API_PROPERTY(Attributes="EditorOrder(40), DefaultValue(true), EditorDisplay(\"Rigid Body\")")
    FORCE_INLINE bool GetEnableGravity() const
    {
        return _enableGravity != 0;
    }

    /// <summary>
    /// If object should have the force of gravity applied.
    /// </summary>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetEnableGravity(bool value);

    /// <summary>
    /// If object should start awake, or if it should initially be sleeping.
    /// </summary>
    /// <returns>The value.</returns>
    API_PROPERTY(Attributes="EditorOrder(50), DefaultValue(true), EditorDisplay(\"Rigid Body\")")
    FORCE_INLINE bool GetStartAwake() const
    {
        return _startAwake != 0;
    }

    /// <summary>
    /// If object should start awake, or if it should initially be sleeping.
    /// </summary>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetStartAwake(bool value);

    /// <summary>
    /// If true, it will update mass when actor scale changes.
    /// </summary>
    /// <returns>The value.</returns>
    API_PROPERTY(Attributes="EditorOrder(130), DefaultValue(false), EditorDisplay(\"Rigid Body\")")
    FORCE_INLINE bool GetUpdateMassWhenScaleChanges() const
    {
        return _updateMassWhenScaleChanges != 0;
    }

    /// <summary>
    /// If true, it will update mass when actor scale changes.
    /// </summary>
    /// <param name="value">The value.</
    API_PROPERTY() void SetUpdateMassWhenScaleChanges(bool value);

    /// <summary>
    /// Gets the maximum angular velocity that a simulated object can achieve.
    /// </summary>
    /// <remarks>
    /// The angular velocity of rigidbodies is clamped to MaxAngularVelocity to avoid numerical instability with fast rotating bodies.
    /// Because this may prevent intentional fast rotations on objects such as wheels, you can override this value per rigidbody.
    /// </remarks>
    /// <returns>The value.</returns>
    API_PROPERTY(Attributes="EditorOrder(90), DefaultValue(7.0f), Limit(0), EditorDisplay(\"Rigid Body\")")
    FORCE_INLINE float GetMaxAngularVelocity() const
    {
        return _maxAngularVelocity;
    }

    /// <summary>
    /// Sets the maximum angular velocity that a simulated object can achieve.
    /// </summary>
    /// <remarks>
    /// The angular velocity of rigidbodies is clamped to MaxAngularVelocity to avoid numerical instability with fast rotating bodies.
    /// Because this may prevent intentional fast rotations on objects such as wheels, you can override this value per rigidbody.
    /// </remarks>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetMaxAngularVelocity(float value);

    /// <summary>
    /// Override the auto computed mass.
    /// </summary>
    /// <returns>The value.</returns>
    API_PROPERTY(Attributes="HideInEditor")
    FORCE_INLINE bool GetOverrideMass() const
    {
        return _overrideMass != 0;
    }

    /// <summary>
    /// Override the auto computed mass.
    /// </summary>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetOverrideMass(bool value);

    /// <summary>
    /// Gets the mass value measured in kilograms (use override value only if EnableOverrideMass is enabled).
    /// </summary>
    /// <returns>The value.</returns>
    API_PROPERTY(Attributes="EditorOrder(110), Limit(0), EditorDisplay(\"Rigid Body\")")
    FORCE_INLINE float GetMass() const
    {
        return _mass;
    }

    /// <summary>
    /// Sets the mass value measured in kilograms (use override value only if EnableOverrideMass is enabled).
    /// </summary>
    /// <remarks>
    /// If set auto enables mass override.
    /// </remarks>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetMass(float value);

    /// <summary>
    /// Gets the per-instance scaling of the mass.
    /// </summary>
    /// <returns>The value.</returns>
    API_PROPERTY(Attributes="EditorOrder(120), DefaultValue(1.0f), Limit(0.001f, 100.0f), EditorDisplay(\"Rigid Body\")")
    FORCE_INLINE float GetMassScale() const
    {
        return _massScale;
    }

    /// <summary>
    /// Sets the per-instance scaling of the mass.
    /// </summary>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetMassScale(float value);

    /// <summary>
    /// Gets the user specified offset for the center of mass of this object, from the calculated location.
    /// </summary>
    /// <returns>The value.</returns>
    API_PROPERTY(Attributes="EditorOrder(140), DefaultValue(typeof(Vector3), \"0,0,0\"), EditorDisplay(\"Rigid Body\", \"Center Of Mass Offset\")")
    FORCE_INLINE Vector3 GetCenterOfMassOffset() const
    {
        return _centerOfMassOffset;
    }

    /// <summary>
    /// Sets the user specified offset for the center of mass of this object, from the calculated location.
    /// </summary>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetCenterOfMassOffset(const Vector3& value);

    /// <summary>
    /// Gets the object movement constraint flags that define degrees of freedom are allowed for the simulation of object.
    /// </summary>
    /// <returns>The value.</returns>
    API_PROPERTY(Attributes="EditorOrder(150), DefaultValue(RigidbodyConstraints.None), EditorDisplay(\"Rigid Body\")")
    FORCE_INLINE RigidbodyConstraints GetConstraints() const
    {
        return _constraints;
    }

    /// <summary>
    /// Sets the object movement constraint flags that define degrees of freedom are allowed for the simulation of object.
    /// </summary>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetConstraints(const RigidbodyConstraints value);

public:

    /// <summary>
    /// Gets the linear velocity of the rigidbody.
    /// </summary>
    /// <remarks>
    /// It's used mostly to get the current velocity. Manual modifications may result in unrealistic behaviour.
    /// </remarks>
    /// <returns>The value.</returns>
    API_PROPERTY(Attributes="HideInEditor")
    Vector3 GetLinearVelocity() const;

    /// <summary>
    /// Sets the linear velocity of the rigidbody.
    /// </summary>
    /// <remarks>
    /// It's used mostly to get the current velocity. Manual modifications may result in unrealistic behaviour.
    /// </remarks>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetLinearVelocity(const Vector3& value) const;

    /// <summary>
    /// Gets the angular velocity of the rigidbody measured in radians per second.
    /// </summary>
    /// <remarks>
    /// It's used mostly to get the current angular velocity. Manual modifications may result in unrealistic behaviour.
    /// </remarks>
    /// <returns>The value.</returns>
    API_PROPERTY(Attributes="HideInEditor")
    Vector3 GetAngularVelocity() const;

    /// <summary>
    /// Sets the angular velocity of the rigidbody measured in radians per second.
    /// </summary>
    /// <remarks>
    /// It's used mostly to get the current angular velocity. Manual modifications may result in unrealistic behaviour.
    /// </remarks>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetAngularVelocity(const Vector3& value) const;

    /// <summary>
    /// Gets the maximum depenetration velocity when rigidbody moving out of penetrating state.
    /// </summary>
    /// <remarks>
    /// This value controls how much velocity the solver can introduce to correct for penetrations in contacts. 
    /// Using this property can smooth objects moving out of colliding state and prevent unstable motion.
    /// </remarks>
    /// <returns>The value</returns>
    API_PROPERTY(Attributes="HideInEditor")
    float GetMaxDepenetrationVelocity() const;

    /// <summary>
    /// Sets the maximum depenetration velocity when rigidbody moving out of penetrating state.
    /// </summary>
    /// <remarks>
    /// This value controls how much velocity the solver can introduce to correct for penetrations in contacts. 
    /// Using this property can smooth objects moving out of colliding state and prevent unstable motion.
    /// </remarks>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetMaxDepenetrationVelocity(const float value) const;

    /// <summary>
    /// Gets the mass-normalized kinetic energy threshold below which an actor may go to sleep.
    /// </summary>
    /// <remarks>
    /// Actors whose kinetic energy divided by their mass is below this threshold will be candidates for sleeping.
    /// </remarks>
    /// <returns>The value</returns>
    API_PROPERTY(Attributes="HideInEditor")
    float GetSleepThreshold() const;

    /// <summary>
    /// Sets the mass-normalized kinetic energy threshold below which an actor may go to sleep.
    /// </summary>
    /// <remarks>
    /// Actors whose kinetic energy divided by their mass is below this threshold will be candidates for sleeping.
    /// </remarks>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetSleepThreshold(const float value) const;

    /// <summary>
    /// Gets the center of the mass in the local space.
    /// </summary>
    /// <returns>The value.</returns>
    API_PROPERTY() Vector3 GetCenterOfMass() const;

    /// <summary>
    /// Determines whether this rigidbody is sleeping.
    /// </summary>
    /// <returns><c>true</c> if this rigidbody is sleeping; otherwise, <c>false</c>.</returns>
    API_PROPERTY() bool IsSleeping() const;

public:

    /// <summary>
    /// Forces a rigidbody to sleep (for at least one frame).
    /// </summary>
    API_FUNCTION() void Sleep() const;

    /// <summary>
    /// Forces a rigidbody to wake up.
    /// </summary>
    API_FUNCTION() void WakeUp() const;

    /// <summary>
    /// Updates the actor's mass (auto calculated mass from density and inertia tensor or overriden value).
    /// </summary>
    API_FUNCTION() void UpdateMass();

    /// <summary>
    /// Gets the native PhysX rigid actor object.
    /// </summary>
    /// <returns>The PhysX dynamic rigid actor.</returns>
    FORCE_INLINE PxRigidDynamic* GetPhysXRigidActor() const
    {
        return _actor;
    }

    /// <summary>
    /// Applies a force (or impulse) defined in the world space to the rigidbody at its center of mass.
    /// </summary>
    /// <remarks>
    /// This will not induce any torque
    /// <para>
    /// ForceMode determines if the force is to be conventional or impulsive.
    /// </para>
    /// <para>
    /// Each actor has an acceleration and a velocity change accumulator which are directly modified using the modes ForceMode::Acceleration
    /// and ForceMode::VelocityChange respectively. The modes ForceMode::Force and ForceMode::Impulse also modify these same
    /// accumulators and are just short hand for multiplying the vector parameter by inverse mass and then using ForceMode::Acceleration and
    /// ForceMode::VelocityChange respectively.
    /// </para>
    /// </remarks>
    /// <param name="force">The force/impulse to apply defined in the world space.</param>
    /// <param name="mode">The mode to use when applying the force/impulse.</param>
    API_FUNCTION() void AddForce(const Vector3& force, ForceMode mode = ForceMode::Force) const;

    /// <summary>
    /// Applies a force (or impulse) defined in the world space to the rigidbody at given position in the world space.
    /// </summary>
    /// <remarks>
    /// Also applies appropriate amount of torque
    /// <para>
    /// ForceMode determines if the force is to be conventional or impulsive.
    /// </para>
    /// <para>
    /// Each actor has an acceleration and a velocity change accumulator which are directly modified using the modes ForceMode::Acceleration
    /// and ForceMode::VelocityChange respectively. The modes ForceMode::Force and ForceMode::Impulse also modify these same
    /// accumulators and are just short hand for multiplying the vector parameter by inverse mass and then using ForceMode::Acceleration and
    /// ForceMode::VelocityChange respectively.
    /// </para>
    /// </remarks>
    /// <param name="force">The force/impulse to apply defined in the world space.</param>
    /// <param name="position">The position of the force/impulse in the world space.</param>
    /// <param name="mode">The mode to use when applying the force/impulse.</param>
    API_FUNCTION() void AddForceAtPosition(const Vector3& force, const Vector3& position, ForceMode mode = ForceMode::Force) const;

    /// <summary>
    /// Applies a force (or impulse) defined in the local space of the rigidbody (relative to its coordinate system) at its center of mass.
    /// </summary>
    /// <remarks>
    /// This will not induce any torque
    /// <para>
    /// ForceMode determines if the force is to be conventional or impulsive.
    /// </para>
    /// <para>
    /// Each actor has an acceleration and a velocity change accumulator which are directly modified using the modes ForceMode::Acceleration
    /// and ForceMode::VelocityChange respectively. The modes ForceMode::Force and ForceMode::Impulse also modify these same
    /// accumulators and are just short hand for multiplying the vector parameter by inverse mass and then using ForceMode::Acceleration and
    /// ForceMode::VelocityChange respectively.
    /// </para>
    /// </remarks>
    /// <param name="force">The force/impulse to apply defined in the local space.</param>
    /// <param name="mode">The mode to use when applying the force/impulse.</param>
    API_FUNCTION() void AddRelativeForce(const Vector3& force, ForceMode mode = ForceMode::Force) const;

    /// <summary>
    /// Applies an impulsive torque defined in the world space to the rigidbody.
    /// </summary>
    /// <remarks>
    /// ForceMode determines if the force is to be conventional or impulsive.
    /// <para>
    /// Each actor has an angular acceleration and an angular velocity change accumulator which are directly modified using the modes 
    /// ForceMode::Acceleration and ForceMode::VelocityChange respectively.The modes ForceMode::Force and ForceMode::Impulse
    /// also modify these same accumulators and are just short hand for multiplying the vector parameter by inverse inertia and then
    /// using ForceMode::Acceleration and ForceMode::VelocityChange respectively.
    /// </para>
    /// </remarks>
    /// <param name="torque">The torque to apply defined in the world space.</param>
    /// <param name="mode">The mode to use when applying the force/impulse.</param>
    API_FUNCTION() void AddTorque(const Vector3& torque, ForceMode mode = ForceMode::Force) const;

    /// <summary>
    /// Applies an impulsive torque defined in the local space of the rigidbody (relative to its coordinate system).
    /// </summary>
    /// <remarks>
    /// ForceMode determines if the force is to be conventional or impulsive.
    /// <para>
    /// Each actor has an angular acceleration and an angular velocity change accumulator which are directly modified using the modes 
    /// ForceMode::Acceleration and ForceMode::VelocityChange respectively.The modes ForceMode::Force and ForceMode::Impulse
    /// also modify these same accumulators and are just short hand for multiplying the vector parameter by inverse inertia and then
    /// using ForceMode::Acceleration and ForceMode::VelocityChange respectively.
    /// </para>
    /// </remarks>
    /// <param name="torque">The torque to apply defined in the local space.</param>
    /// <param name="mode">The mode to use when applying the force/impulse.</param>
    API_FUNCTION() void AddRelativeTorque(const Vector3& torque, ForceMode mode = ForceMode::Force) const;

    /// <summary>
    /// Sets the solver iteration counts for the rigidbody.
    /// </summary>
    /// <remarks>
    /// The solver iteration count determines how accurately joints and contacts are resolved.
    /// If you are having trouble with jointed bodies oscillating and behaving erratically,
    /// then setting a higher position iteration count may improve their stability.
    /// <para>
    /// If intersecting bodies are being depenetrated too violently, increase the number of velocity
    /// iterations. More velocity iterations will drive the relative exit velocity of the intersecting
    /// objects closer to the correct value given the restitution.
    /// </para>
    /// <para>
    /// Default: 4 position iterations, 1 velocity iteration.
    /// </para>
    /// </remarks>
    /// <param name="minPositionIters">The minimum number of position iterations the solver should perform for this body.</param>
    /// <param name="minVelocityIters">The minimum number of velocity iterations the solver should perform for this body.</param>
    API_FUNCTION() void SetSolverIterationCounts(int32 minPositionIters, int32 minVelocityIters) const;

    /// <summary>
    /// Gets a point on one of the colliders attached to the attached that is closest to a given location. Can be used to find a hit location or position to apply explosion force or any other special effects.
    /// </summary>
    /// <param name="position">The position to find the closest point to it.</param>
    /// <param name="result">The result point on the rigidbody shape that is closest to the specified location.</param>
    API_FUNCTION() void ClosestPoint(const Vector3& position, API_PARAM(Out) Vector3& result) const;

public:

    /// <summary>
    /// Occurs when a collision start gets registered for this rigidbody (it collides with something).
    /// </summary>
    API_EVENT() Delegate<const Collision&> CollisionEnter;

    /// <summary>
    /// Occurs when a collision end gets registered for this rigidbody (it ends colliding with something).
    /// </summary>
    API_EVENT() Delegate<const Collision&> CollisionExit;

    /// <summary>
    /// Occurs when this rigidbody trigger touching start gets registered for this collider (the other collider enters it and triggers the event).
    /// </summary>
    API_EVENT() Delegate<PhysicsColliderActor*> TriggerEnter;

    /// <summary>
    /// Occurs when this rigidbody trigger touching end gets registered for this collider (the other collider enters it and triggers the event).
    /// </summary>
    API_EVENT() Delegate<PhysicsColliderActor*> TriggerExit;

public:

    void OnCollisionEnter(const Collision& c);
    void OnCollisionExit(const Collision& c);

    void OnTriggerEnter(PhysicsColliderActor* c);
    void OnTriggerExit(PhysicsColliderActor* c);

    // Called when collider gets detached from this rigidbody or activated/deactivated. Used to update rigidbody mass.
    virtual void OnColliderChanged(Collider* c);

protected:

    /// <summary>
    /// Creates the physics actor.
    /// </summary>
    void CreateActor();

    /// <summary>
    /// Updates the rigidbody scale dependent properties like mass (may be modified when actor transformation changes).
    /// </summary>
    void UpdateScale();

public:

    // [PhysicsActor]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    PxActor* GetPhysXActor() override;
    PxRigidActor* GetRigidActor() override;

protected:

    // [PhysicsActor]
    void BeginPlay(SceneBeginData* data) override;
    void EndPlay() override;
    void OnActiveInTreeChanged() override;
    void OnTransformChanged() override;
};
