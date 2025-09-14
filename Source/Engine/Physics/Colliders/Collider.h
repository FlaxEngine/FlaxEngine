// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Physics/Types.h"
#include "Engine/Content/JsonAsset.h"
#include "Engine/Content/JsonAssetReference.h"
#include "Engine/Physics/Actors/PhysicsColliderActor.h"
#include "Engine/Physics/Actors/IPhysicsDebug.h"

struct RayCastHit;
class RigidBody;

/// <summary>
/// A base class for all colliders.
/// </summary>
/// <seealso cref="Actor" />
/// <seealso cref="PhysicsColliderActor" />
API_CLASS(Abstract) class FLAXENGINE_API Collider : public PhysicsColliderActor
#if USE_EDITOR
    , public IPhysicsDebug
#endif
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCENE_OBJECT_ABSTRACT(Collider);
protected:
    Vector3 _center;
    bool _isTrigger;
    void* _shape;
    void* _staticActor;
    float _cachedScale;
    float _contactOffset;
    Vector3 _cachedLocalPosePos;
    Quaternion _cachedLocalPoseRot;

public:
    /// <summary>
    /// Gets the native physics backend object.
    /// </summary>
    void* GetPhysicsShape() const;

    /// <summary>
    /// Gets the 'IsTrigger' flag. A trigger doesn't register a collision with an incoming Rigidbody. Instead, it sends OnTriggerEnter and OnTriggerExit message when a rigidbody enters or exits the trigger volume.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(0), DefaultValue(false), EditorDisplay(\"Collider\")")
    FORCE_INLINE bool GetIsTrigger() const
    {
        return _isTrigger;
    }

    /// <summary>
    /// Sets the `IsTrigger` flag. A trigger doesn't register a collision with an incoming Rigidbody. Instead, it sends OnTriggerEnter and OnTriggerExit message when a rigidbody enters or exits the trigger volume.
    /// </summary>
    API_PROPERTY() void SetIsTrigger(bool value);

    /// <summary>
    /// Gets the center of the collider, measured in the object's local space.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(10), DefaultValue(typeof(Vector3), \"0,0,0\"), EditorDisplay(\"Collider\"), ValueCategory(Utils.ValueCategory.Distance)")
    FORCE_INLINE Vector3 GetCenter() const
    {
        return _center;
    }

    /// <summary>
    /// Sets the center of the collider, measured in the object's local space.
    /// </summary>
    API_PROPERTY() virtual void SetCenter(const Vector3& value);

    /// <summary>
    /// Gets the contact offset. Colliders whose distance is less than the sum of their ContactOffset values will generate contacts. The contact offset must be positive. Contact offset allows the collision detection system to predictively enforce the contact constraint even when the objects are slightly separated.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(1), DefaultValue(2.0f), Limit(0, 100), EditorDisplay(\"Collider\"), ValueCategory(Utils.ValueCategory.Distance)")
    FORCE_INLINE float GetContactOffset() const
    {
        return _contactOffset;
    }

    /// <summary>
    /// Sets the contact offset. Colliders whose distance is less than the sum of their ContactOffset values will generate contacts. The contact offset must be positive. Contact offset allows the collision detection system to predictively enforce the contact constraint even when the objects are slightly separated.
    /// </summary>
    API_PROPERTY() void SetContactOffset(float value);

    /// <summary>
    /// The physical material used to define the collider physical properties.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2), DefaultValue(null), EditorDisplay(\"Collider\")")
    JsonAssetReference<PhysicalMaterial> Material;

public:
    /// <summary>
    /// Computes minimum translational distance between two geometry objects.
    /// Translating the first collider by direction * distance will separate the colliders apart if the function returned true. Otherwise, direction and distance are not defined.
    /// The one of the colliders has to be BoxCollider, SphereCollider CapsuleCollider or a convex MeshCollider. The other one can be any type. 
    /// If objects do not overlap, the function can not compute the distance and returns false.
    /// </summary>
    /// <param name="colliderA">The first collider.</param>
    /// <param name="colliderB">The second collider.</param>
    /// <param name="direction">The computed direction along which the translation required to separate the colliders apart is minimal. Valid only if function returns true.</param>
    /// <param name="distance">The penetration distance along direction that is required to separate the colliders apart. Valid only if function returns true.</param>
    /// <returns>True if the distance has successfully been computed, i.e. if objects do overlap, otherwise false.</returns>
    API_FUNCTION() static bool ComputePenetration(const Collider* colliderA, const Collider* colliderB, API_PARAM(Out) Vector3& direction, API_PARAM(Out) float& distance);

public:
    /// <summary>
    /// Determines whether this collider can be attached the specified rigid body.
    /// </summary>
    /// <param name="rigidBody">The rigid body.</param>
    /// <returns><c>true</c> if this collider can be attached the specified rigid body; otherwise, <c>false</c>.</returns>
    virtual bool CanAttach(RigidBody* rigidBody) const;

    /// <summary>
    /// Determines whether this collider can be a trigger shape.
    /// </summary>
    /// <returns><c>true</c> if this collider can be trigger; otherwise, <c>false</c>.</returns>
    virtual bool CanBeTrigger() const;

    /// <summary>
    /// Attaches collider to the specified rigid body.
    /// </summary>
    /// <param name="rigidBody">The rigid body.</param>
    void Attach(RigidBody* rigidBody);

protected:
    /// <summary>
    /// Updates the shape actor collisions/queries layer mask bits.
    /// </summary>
    void UpdateLayerBits();

    /// <summary>
    /// Updates the bounding box of the shape.
    /// </summary>
    virtual void UpdateBounds() = 0;

    /// <summary>
    /// Gets the collider shape geometry.
    /// </summary>
    /// <param name="collision">The output collision shape.</param>
    virtual void GetGeometry(CollisionShape& collision) = 0;

    /// <summary>
    /// Creates the collider shape.
    /// </summary>
    virtual void CreateShape();

    /// <summary>
    /// Updates the shape geometry.
    /// </summary>
    virtual void UpdateGeometry();

    /// <summary>
    /// Creates the static actor.
    /// </summary>
    void CreateStaticActor();

    /// <summary>
    /// Removes the static actor.
    /// </summary>
    void RemoveStaticActor();

private:
    void OnMaterialChanged();

public:
    // [PhysicsColliderActor]
    RigidBody* GetAttachedRigidBody() const override;
    bool RayCast(const Vector3& origin, const Vector3& direction, float& resultHitDistance, float maxDistance = MAX_float) const final;
    bool RayCast(const Vector3& origin, const Vector3& direction, RayCastHit& hitInfo, float maxDistance = MAX_float) const final;
    void ClosestPoint(const Vector3& point, Vector3& result) const final;
    bool ContainsPoint(const Vector3& point) const final;


protected:
    // [PhysicsColliderActor]
    void OnEnable() override;
    void OnDisable() override;
    void BeginPlay(SceneBeginData* data) override;
    void EndPlay() override;
    void OnActiveInTreeChanged() override;
    void OnParentChanged() override;
    void OnTransformChanged() override;
    void OnLayerChanged() override;
    void OnStaticFlagsChanged() override;
    void OnPhysicsSceneChanged(PhysicsScene* previous) override;
};
