// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Collider.h"

/// <summary>
/// A sphere-shaped primitive collider.
/// </summary>
/// <seealso cref="Collider" />
API_CLASS() class FLAXENGINE_API SphereCollider : public Collider
{
DECLARE_SCENE_OBJECT(SphereCollider);
private:

    float _radius;

public:

    /// <summary>
    /// Gets the radius of the sphere, measured in the object's local space.
    /// </summary>
    /// <remarks>
    /// The sphere radius will be scaled by the actor's world scale.
    /// </remarks>
    API_PROPERTY(Attributes="EditorOrder(100), DefaultValue(50.0f), EditorDisplay(\"Collider\")")
    FORCE_INLINE float GetRadius() const
    {
        return _radius;
    }

    /// <summary>
    /// Sets the radius of the sphere, measured in the object's local space.
    /// </summary>
    /// <remarks>
    /// The sphere radius will be scaled by the actor's world scale.
    /// </remarks>
    API_PROPERTY() void SetRadius(float value);

public:

    // [Collider]
#if USE_EDITOR
    void OnDebugDrawSelected() override;
#endif
    bool IntersectsItself(const Ray& ray, float& distance, Vector3& normal) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;

protected:

    // [Collider]
#if USE_EDITOR
    void DrawPhysicsDebug(RenderView& view) override;
#endif
    void UpdateBounds() override;
    void GetGeometry(PxGeometryHolder& geometry) override;
};
