// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Collider.h"
#include "Engine/Physics/Actors/IPhysicsActor.h"

/// <summary>
/// Physical objects that allows to easily do player movement constrained by collisions without having to deal with a rigidbody.
/// </summary>
/// <seealso cref="Collider" />
API_CLASS() class FLAXENGINE_API CharacterController : public Collider, public IPhysicsActor
{
DECLARE_SCENE_OBJECT(CharacterController);
public:

    /// <summary>
    /// Specifies which sides a character is colliding with.
    /// </summary>
    API_ENUM(Attributes="Flags") enum class CollisionFlags
    {
        /// <summary>
        /// The character is not colliding.
        /// </summary>
        None = 0,

        /// <summary>
        /// The character is colliding to the sides.
        /// </summary>
        Sides = 1 << 0,

        /// <summary>
        /// The character has collision above.
        /// </summary>
        Above = 1 << 1,

        /// <summary>
        /// The character has collision below.
        /// </summary>
        Below = 1 << 2,
    };

    /// <summary>
    /// Specifies how a character controller interacts with non-walkable parts.
    /// </summary>
    API_ENUM() enum class NonWalkableModes
    {
        /// <summary>
        /// Stops character from climbing up non-walkable slopes, but doesn't move it otherwise.
        /// </summary>
        PreventClimbing,

        /// <summary>
        /// Stops character from climbing up non-walkable slopes, and forces it to slide down those slopes.
        /// </summary>
        PreventClimbingAndForceSliding,
    };

private:

    PxCapsuleController* _controller;
    float _stepOffset;
    float _slopeLimit;
    float _radius;
    float _height;
    float _minMoveDistance;
    bool _isUpdatingTransform;
    NonWalkableModes _nonWalkableMode;
    CollisionFlags _lastFlags;
    uint32 _filterData[4];

public:

    /// <summary>
    /// Gets the radius of the sphere, measured in the object's local space. The sphere radius will be scaled by the actor's world scale.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(100), DefaultValue(50.0f), EditorDisplay(\"Collider\")")
    FORCE_INLINE float GetRadius() const
    {
        return _radius;
    }

    /// <summary>
    /// Sets the radius of the sphere, measured in the object's local space. The sphere radius will be scaled by the actor's world scale.
    /// </summary>
    API_PROPERTY() void SetRadius(float value);

    /// <summary>
    /// Gets the height of the capsule, measured in the object's local space. The capsule height will be scaled by the actor's world scale.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(110), DefaultValue(150.0f), EditorDisplay(\"Collider\")")
    FORCE_INLINE float GetHeight() const
    {
        return _height;
    }

    /// <summary>
    /// Sets the height of the capsule, measured in the object's local space. The capsule height will be scaled by the actor's world scale.
    /// </summary>
    API_PROPERTY() void SetHeight(float value);

    /// <summary>
    /// Gets the slope limit (in degrees). Limits the collider to only climb slopes that are less steep (in degrees) than the indicated value.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(210), DefaultValue(45.0f), Limit(0, 100), EditorDisplay(\"Character Controller\")")
    FORCE_INLINE float GetSlopeLimit() const
    {
        return _slopeLimit;
    }

    /// <summary>
    /// Sets the slope limit (in degrees). Limits the collider to only climb slopes that are less steep (in degrees) than the indicated value.
    /// </summary>
    API_PROPERTY() void SetSlopeLimit(float value);

    /// <summary>
    /// Gets the non-walkable mode for the character controller.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(215), DefaultValue(NonWalkableModes.PreventClimbing), EditorDisplay(\"Character Controller\")")
    FORCE_INLINE NonWalkableModes GetNonWalkableMode() const
    {
        return _nonWalkableMode;
    }

    /// <summary>
    /// Sets the non-walkable mode for the character controller.
    /// </summary>
    API_PROPERTY() void SetNonWalkableMode(NonWalkableModes value);

    /// <summary>
    /// Gets the step height. The character will step up a stair only if it is closer to the ground than the indicated value. This should not be greater than the Character Controller’s height or it will generate an error.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(220), DefaultValue(30.0f), Limit(0), EditorDisplay(\"Character Controller\")")
    FORCE_INLINE float GetStepOffset() const
    {
        return _stepOffset;
    }

    /// <summary>
    /// Sets the step height. The character will step up a stair only if it is closer to the ground than the indicated value. This should not be greater than the Character Controller’s height or it will generate an error.
    /// </summary>
    API_PROPERTY() void SetStepOffset(float value);

    /// <summary>
    /// Gets the minimum move distance of the character controller. The minimum traveled distance to consider. If traveled distance is smaller, the character doesn't move. This is used to stop the recursive motion algorithm when remaining distance to travel is small.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(230), DefaultValue(0.0f), Limit(0, 1000), EditorDisplay(\"Character Controller\")")
    FORCE_INLINE float GetMinMoveDistance() const
    {
        return _minMoveDistance;
    }

    /// <summary>
    /// Sets the minimum move distance of the character controller.The minimum traveled distance to consider. If traveled distance is smaller, the character doesn't move. This is used to stop the recursive motion algorithm when remaining distance to travel is small.
    /// </summary>
    API_PROPERTY() void SetMinMoveDistance(float value);

public:

    /// <summary>
    /// Gets the linear velocity of the Character Controller. This allows tracking how fast the character is actually moving, for instance when it is stuck at a wall this value will be the near zero vector.
    /// </summary>
    API_PROPERTY() Vector3 GetVelocity() const;

    /// <summary>
    /// Gets a value indicating whether this character was grounded during last move call grounded.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool IsGrounded() const
    {
        return (static_cast<int>(_lastFlags) & static_cast<int>(CollisionFlags::Below)) != 0;
    }

    /// <summary>
    /// Gets the current collision flags. Tells which parts of the character capsule collided with the environment during the last move call. It can be used to trigger various character animations.
    /// </summary>
    API_PROPERTY() FORCE_INLINE CollisionFlags GetFlags() const
    {
        return _lastFlags;
    }

public:

    /// <summary>
    /// Moves the character with the given speed. Gravity is automatically applied. It will slide along colliders. Result collision flags is the summary of collisions that occurred during the Move.
    /// </summary>
    /// <param name="speed">The movement speed (in units/s).</param>
    /// <returns>The collision flags. It can be used to trigger various character animations.</returns>
    API_FUNCTION() CollisionFlags SimpleMove(const Vector3& speed);

    /// <summary>
    /// Moves the character using a 'collide-and-slide' algorithm. Attempts to move the controller by the given displacement vector, the motion will only be constrained by collisions. It will slide along colliders. Result collision flags is the summary of collisions that occurred during the Move. This function does not apply any gravity.
    /// </summary>
    /// <param name="displacement">The displacement vector (in world units).</param>
    /// <returns>The collision flags. It can be used to trigger various character animations.</returns>
    API_FUNCTION() CollisionFlags Move(const Vector3& displacement);

    /// <summary>
    /// Gets the native PhysX rigid actor object.
    /// </summary>
    /// <returns>The PhysX dynamic rigid actor.</returns>
    PxRigidDynamic* GetPhysXRigidActor() const;

protected:

    /// <summary>
    /// Creates the physics actor.
    /// </summary>
    void CreateActor();

    /// <summary>
    /// Updates the character height and radius.
    /// </summary>
    void UpdateSize() const;

public:

    // [Collider]
#if USE_EDITOR
    void OnDebugDrawSelected() override;
#endif
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    void CreateShape() override;
    void UpdateBounds() override;
    void AddMovement(const Vector3& translation, const Quaternion& rotation) override;
    bool CanAttach(RigidBody* rigidBody) const override;
    RigidBody* GetAttachedRigidBody() const override;

    // [IPhysicsActor]
    void OnActiveTransformChanged(const PxTransform& transform) override;
    PxRigidActor* GetRigidActor() override;

protected:

    // [PhysicsActor]
    void UpdateGeometry() override;
    void GetGeometry(PxGeometryHolder& geometry) override;
    void UpdateLayerBits() override;
    void BeginPlay(SceneBeginData* data) override;
    void EndPlay() override;
#if USE_EDITOR
    void DrawPhysicsDebug(RenderView& view) override;
#endif
    void OnActiveInTreeChanged() override;
    void OnParentChanged() override;
    void OnTransformChanged() override;
};
