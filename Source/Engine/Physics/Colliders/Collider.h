// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Physics/Types.h"
#include "Engine/Content/JsonAsset.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Physics/Actors/PhysicsColliderActor.h"

struct RayCastHit;
class RigidBody;

/// <summary>
/// A base class for all colliders.
/// </summary>
/// <seealso cref="Actor" />
/// <seealso cref="PhysicsColliderActor" />
API_CLASS(Abstract) class FLAXENGINE_API Collider : public PhysicsColliderActor
{
DECLARE_SCENE_OBJECT_ABSTRACT(Collider);
protected:

    Vector3 _center;
    bool _isTrigger;
    PxShape* _shape;
    PxRigidStatic* _staticActor;
    Vector3 _cachedScale;
    float _contactOffset;

public:

    /// <summary>
    /// Gets the collider shape PhysX object.
    /// </summary>
    /// <returns>The PhysX Shape object or null.</returns>
    FORCE_INLINE PxShape* GetPxShape() const
    {
        return _shape;
    }

    /// <summary>
    /// Gets the 'IsTrigger' flag.
    /// </summary>
    /// <remarks>
    /// A trigger doesn't register a collision with an incoming Rigidbody. Instead, it sends OnTriggerEnter, OnTriggerExit and OnTriggerStay message when a rigidbody enters or exits the trigger volume.
    /// </remarks>
    API_PROPERTY(Attributes="EditorOrder(0), DefaultValue(false), EditorDisplay(\"Collider\")")
    FORCE_INLINE bool GetIsTrigger() const
    {
        return _isTrigger;
    }

    /// <summary>
    /// Sets the `IsTrigger` flag. A trigger doesn't register a collision with an incoming Rigidbody. Instead, it sends OnTriggerEnter, OnTriggerExit and OnTriggerStay message when a rigidbody enters or exits the trigger volume.
    /// </summary>
    /// <remarks>
    /// A trigger doesn't register a collision with an incoming Rigidbody. Instead, it sends OnTriggerEnter, OnTriggerExit and OnTriggerStay message when a rigidbody enters or exits the trigger volume.
    /// </remarks>
    API_PROPERTY() void SetIsTrigger(bool value);

    /// <summary>
    /// Gets the center of the collider, measured in the object's local space.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(10), DefaultValue(typeof(Vector3), \"0,0,0\"), EditorDisplay(\"Collider\")")
    FORCE_INLINE Vector3 GetCenter() const
    {
        return _center;
    }

    /// <summary>
    /// Sets the center of the collider, measured in the object's local space.
    /// </summary>
    API_PROPERTY() void SetCenter(const Vector3& value);

    /// <summary>
    /// Gets the contact offset.
    /// </summary>
    /// <remarks>
    /// Colliders whose distance is less than the sum of their ContactOffset values will generate contacts. The contact offset must be positive. Contact offset allows the collision detection system to predictively enforce the contact constraint even when the objects are slightly separated.
    /// </remarks>
    API_PROPERTY(Attributes="EditorOrder(1), DefaultValue(10.0f), Limit(0, 100), EditorDisplay(\"Collider\")")
    FORCE_INLINE float GetContactOffset() const
    {
        return _contactOffset;
    }

    /// <summary>
    /// Sets the contact offset.
    /// </summary>
    /// <remarks>
    /// Colliders whose distance is less than the sum of their ContactOffset values will generate contacts. The contact offset must be positive. Contact offset allows the collision detection system to predictively enforce the contact constraint even when the objects are slightly separated.
    /// </remarks>
    API_PROPERTY() void SetContactOffset(float value);

    /// <summary>
    /// The physical material used to define the collider physical properties.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2), DefaultValue(null), AssetReference(typeof(PhysicalMaterial), true), EditorDisplay(\"Collider\")")
    AssetReference<JsonAsset> Material;

public:

    /// <summary>
    /// Performs a raycast against this collider shape.
    /// </summary>
    /// <param name="origin">The origin of the ray.</param>
    /// <param name="direction">The normalized direction of the ray.</param>
    /// <param name="resultHitDistance">The raycast result hit position distance from the ray origin. Valid only if raycast hits anything.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <returns>True if ray hits an object, otherwise false.</returns>
    API_FUNCTION() bool RayCast(const Vector3& origin, const Vector3& direction, API_PARAM(Out) float& resultHitDistance, float maxDistance = MAX_float) const;

    /// <summary>
    /// Performs a raycast against this collider, returns results in a RaycastHit structure.
    /// </summary>
    /// <param name="origin">The origin of the ray.</param>
    /// <param name="direction">The normalized direction of the ray.</param>
    /// <param name="hitInfo">The result hit information. Valid only when method returns true.</param>
    /// <param name="maxDistance">The maximum distance the ray should check for collisions.</param>
    /// <returns>True if ray hits an object, otherwise false.</returns>
    API_FUNCTION() bool RayCast(const Vector3& origin, const Vector3& direction, API_PARAM(Out) RayCastHit& hitInfo, float maxDistance = MAX_float) const;

    /// <summary>
    /// Gets a point on the collider that is closest to a given location. Can be used to find a hit location or position to apply explosion force or any other special effects.
    /// </summary>
    /// <param name="position">The position to find the closest point to it.</param>
    /// <param name="result">The result point on the collider that is closest to the specified location.</param>
    API_FUNCTION() void ClosestPoint(const Vector3& position, API_PARAM(Out) Vector3& result) const;

    /// <summary>
    /// Checks if a point is inside the collider.
    /// </summary>
    /// <param name="point">The point to check if is contained by the collider shape (in world-space).</param>
    /// <returns>True if collider shape contains a given point, otherwise false.</returns>
    API_FUNCTION() bool ContainsPoint(const Vector3& point) const;

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
    /// Determines whether this collider is attached to the body.
    /// </summary>
    /// <returns><c>true</c> if this instance is attached; otherwise, <c>false</c>.</returns>
    API_PROPERTY() bool IsAttached() const;

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
    /// Updates the shape scale (may be modified when actor transformation changes).
    /// </summary>
    void UpdateScale();

    /// <summary>
    /// Updates the shape actor collisions/queries layer mask bits.
    /// </summary>
    virtual void UpdateLayerBits();

    /// <summary>
    /// Updates the bounding box of the shape.
    /// </summary>
    virtual void UpdateBounds() = 0;

    /// <summary>
    /// Gets the collider shape geometry.
    /// </summary>
    /// <param name="geometry">The output geometry.</param>
    virtual void GetGeometry(PxGeometryHolder& geometry) = 0;

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

#if USE_EDITOR
    virtual void DrawPhysicsDebug(RenderView& view);
#endif

private:

    void OnMaterialChanged();

public:

    // [PhysicsColliderActor]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    RigidBody* GetAttachedRigidBody() const override;

protected:

    // [PhysicsColliderActor]
#if USE_EDITOR
    void OnEnable() override;
    void OnDisable() override;
#endif
    void BeginPlay(SceneBeginData* data) override;
    void EndPlay() override;
    void OnActiveInTreeChanged() override;
    void OnParentChanged() override;
    void OnTransformChanged() override;
    void OnLayerChanged() override;
};
