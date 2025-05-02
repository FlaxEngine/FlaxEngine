// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../Actor.h"
#include "Engine/Core/Collections/Dictionary.h"

/// <summary>
/// Actor that synchronizes Animated Model skeleton pose with physical bones bodies simulated with physics. Child rigidbodies are used for per-bone simulation - rigidbodies names must match skeleton bone name and should be ordered based on importance in the skeleton tree (parents first).
/// </summary>
API_CLASS(Attributes="ActorToolbox(\"Physics\")") class FLAXENGINE_API Ragdoll : public Actor
{
    DECLARE_SCENE_OBJECT(Ragdoll);
    API_AUTO_SERIALIZATION();
private:
    AnimatedModel* _animatedModel = nullptr;
    Dictionary<RigidBody*, Transform> _bonesOffsets;

public:
    /// <summary>
    /// The default bones weight where 0 means fully animated bone and 1 means fully simulate bones. Can be used to control all bones simulation mode but is overriden by per-bone BonesWeights.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), EditorDisplay(\"Ragdoll\"), Limit(0, 1)")
    float BonesWeight = 1.0f;

    /// <summary>
    /// The per-bone weights for ragdoll simulation. Key is bone name, value is the blend weight where 0 means fully animated bone and 1 means fully simulated bone. Can be used to control per-bone simulation.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), EditorDisplay(\"Ragdoll\")")
    Dictionary<String, float> BonesWeights;

    /// <summary>
    /// The minimum number of position iterations the physics solver should perform for bodies in this ragdoll. Higher values improve stability but affect performance.
    /// </summary>
    /// <seealso cref="RigidBody.SetSolverIterationCounts"/>
    API_FIELD(Attributes="EditorOrder(100), EditorDisplay(\"Ragdoll\"), Limit(1, 255)")
    uint8 PositionSolverIterations = 8;

    /// <summary>
    /// The minimum number of velocity iterations the physics solver should perform for bodies in this ragdoll. Higher values improve stability but affect performance.
    /// </summary>
    /// <seealso cref="RigidBody.SetSolverIterationCounts"/>
    API_FIELD(Attributes="EditorOrder(110), EditorDisplay(\"Ragdoll\"), Limit(1, 255)")
    uint8 VelocitySolverIterations = 2;

    /// <summary>
    /// The maximum depenetration velocity when ragdoll's rigidbody moving out of penetrating state. Using this property can smooth objects moving out of colliding state and prevent unstable motion.
    /// </summary>
    /// <seealso cref="RigidBody.MaxDepenetrationVelocity"/>
    API_FIELD(Attributes="EditorOrder(120), EditorDisplay(\"Ragdoll\"), Limit(0)")
    float MaxDepenetrationVelocity = MAX_float;

public:
    /// <summary>
    /// Calculates the total mass of all ragdoll bodies.
    /// </summary>
    API_PROPERTY() float GetTotalMass() const;

    /// <summary>
    /// Sets the linear velocity of all rigidbodies in the ragdoll.
    /// </summary>
    API_FUNCTION() void SetLinearVelocity(const Vector3& value) const;

    /// <summary>
    /// Sets the angular velocity of all rigidbodies in the ragdoll (in radians per second).
    /// </summary>
    API_FUNCTION() void SetAngularVelocity(const Vector3& value) const;

private:
    float InitBone(RigidBody* rigidBody, int32& nodeIndex, Transform& localPose);
    void OnFixedUpdate();
    void OnAnimationUpdating(struct AnimGraphImpulse* localPose);

public:
    // [Actor]
    void OnEnable() override;
    void OnDisable() override;
    void OnParentChanged() override;
    void OnTransformChanged() override;
#if USE_EDITOR
    void OnDebugDrawSelected() override;
#endif
};
