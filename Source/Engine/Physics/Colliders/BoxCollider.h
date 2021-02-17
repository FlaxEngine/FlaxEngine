// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Collider.h"
#include "Engine/Core/Math/OrientedBoundingBox.h"

/// <summary>
/// A box-shaped primitive collider.
/// </summary>
/// <seealso cref="Collider" />
API_CLASS() class FLAXENGINE_API BoxCollider : public Collider
{
DECLARE_SCENE_OBJECT(BoxCollider);
private:

    Vector3 _size;
    OrientedBoundingBox _bounds;

public:

    /// <summary>
    /// Gets the size of the box, measured in the object's local space.
    /// </summary>
    /// <remarks>
    /// The box size will be scaled by the actor's world scale.
    /// </remarks>
    API_PROPERTY(Attributes="EditorOrder(100), DefaultValue(typeof(Vector3), \"100,100,100\"), EditorDisplay(\"Collider\")")
    FORCE_INLINE Vector3 GetSize() const
    {
        return _size;
    }

    /// <summary>
    /// Sets the size of the box, measured in the object's local space.
    /// </summary>
    /// <remarks>
    /// The box size will be scaled by the actor's world scale.
    /// </remarks>
    API_PROPERTY() void SetSize(const Vector3& value);

    /// <summary>
    /// Gets the volume bounding box (oriented).
    /// </summary>
    API_PROPERTY() FORCE_INLINE OrientedBoundingBox GetOrientedBox() const
    {
        return _bounds;
    }

public:

    // [Collider]
#if USE_EDITOR
    void OnDebugDraw() override;
    void OnDebugDrawSelected() override;
#endif
    bool IntersectsItself(const Ray& ray, float& distance, Vector3& normal) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;

protected:

    // [Collider]
    void UpdateBounds() override;
    void GetGeometry(PxGeometryHolder& geometry) override;
#if USE_EDITOR
    void DrawPhysicsDebug(RenderView& view) override;
#endif
};
