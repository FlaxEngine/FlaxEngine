// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Collider.h"
#include "Engine/Core/Math/OrientedBoundingBox.h"

/// <summary>
/// A capsule-shaped primitive collider.
/// </summary>
/// <remarks>Capsules are cylinders with a half-sphere at each end centered at the origin and extending along the X axis, and two hemispherical ends.</remarks>
/// <seealso cref="Collider" />
API_CLASS(Attributes="ActorContextMenu(\"New/Physics/Colliders/Capsule Collider\"), ActorToolbox(\"Physics\")")
class FLAXENGINE_API CapsuleCollider : public Collider
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCENE_OBJECT(CapsuleCollider);
private:
    float _radius;
    float _height;
    OrientedBoundingBox _orientedBox;

public:
    /// <summary>
    /// Gets the radius of the sphere, measured in the object's local space.
    /// </summary>
    /// <remarks>The sphere radius will be scaled by the actor's world scale.</remarks>
    API_PROPERTY(Attributes="EditorOrder(100), DefaultValue(20.0f), EditorDisplay(\"Collider\"), ValueCategory(Utils.ValueCategory.Distance)")
    FORCE_INLINE float GetRadius() const
    {
        return _radius;
    }

    /// <summary>
    /// Sets the radius of the sphere, measured in the object's local space.
    /// </summary>
    /// <remarks>The sphere radius will be scaled by the actor's world scale. </remarks>
    API_PROPERTY() void SetRadius(float value);

    /// <summary>
    /// Gets the height of the capsule, measured in the object's local space between the centers of the hemispherical ends.
    /// </summary>
    /// <remarks>The capsule height will be scaled by the actor's world scale.</remarks>
    API_PROPERTY(Attributes="EditorOrder(110), DefaultValue(100.0f), EditorDisplay(\"Collider\"), ValueCategory(Utils.ValueCategory.Distance)")
    FORCE_INLINE float GetHeight() const
    {
        return _height;
    }

    /// <summary>
    /// Sets the height of the capsule, measured in the object's local space between the centers of the hemispherical ends.
    /// </summary>
    /// <remarks>The capsule height will be scaled by the actor's world scale.</remarks>
    API_PROPERTY() void SetHeight(float value);

public:
    // [Collider]
#if USE_EDITOR
    void OnDebugDrawSelected() override;
#endif
    bool IntersectsItself(const Ray& ray, Real& distance, Vector3& normal) override;

protected:
    // [Collider]
    ImplementPhysicsDebug;
    void UpdateBounds() override;
    void GetGeometry(CollisionShape& collision) override;
};
