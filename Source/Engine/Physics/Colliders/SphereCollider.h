// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Collider.h"

/// <summary>
/// A sphere-shaped primitive collider.
/// </summary>
/// <seealso cref="Collider" />
API_CLASS(Attributes="ActorContextMenu(\"New/Physics/Colliders/Sphere Collider\"), ActorToolbox(\"Physics\")")
class FLAXENGINE_API SphereCollider : public Collider
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCENE_OBJECT(SphereCollider);
private:
    float _radius;

public:
    /// <summary>
    /// Gets the radius of the sphere, measured in the object's local space.
    /// </summary>
    /// <remarks>The sphere radius will be scaled by the actor's world scale.</remarks>
    API_PROPERTY(Attributes="EditorOrder(100), DefaultValue(50.0f), EditorDisplay(\"Collider\"), ValueCategory(Utils.ValueCategory.Distance)")
    FORCE_INLINE float GetRadius() const
    {
        return _radius;
    }

    /// <summary>
    /// Sets the radius of the sphere, measured in the object's local space.
    /// </summary>
    /// <remarks>The sphere radius will be scaled by the actor's world scale.</remarks>
    API_PROPERTY() void SetRadius(float value);

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
