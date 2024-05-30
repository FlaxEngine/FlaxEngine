// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Collider.h"
#include "Engine/Core/Math/OrientedBoundingBox.h"

/// <summary>
/// A capsule-shaped primitive collider.
/// </summary>
/// <remarks>Capsules are cylinders with a half-sphere at each end centered at the origin and extending along the X axis, and two hemispherical ends.</remarks>
/// <seealso cref="Collider" />
API_CLASS(Attributes="ActorContextMenu(\"New/Physics/Colliders/Cylinder Collider\"), ActorToolbox(\"Physics\")")
class FLAXENGINE_API CylinderCollider : public Collider
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCENE_OBJECT(CylinderCollider);
private:
    float _radius;
    float _height;
    ColliderAxis _axis;
    OrientedBoundingBox _orientedBox;

public:
    /// <summary>
    /// Gets the Margin.
    /// </summary>
    API_PROPERTY(Attributes = "EditorOrder(100), DefaultValue(20.0f), EditorDisplay(\"Collider\")")
    FORCE_INLINE ColliderAxis GetAxis() const
    {
        return _axis;
    }

    /// <summary>
    /// Sets the Margin.
    /// </summary>
    API_PROPERTY() void SetAxis(ColliderAxis value);

    /// <summary>
    /// Gets the radius of the cylinder, measured in the object's local space.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(100), DefaultValue(20.0f), EditorDisplay(\"Collider\"), ValueCategory(Utils.ValueCategory.Distance)")
    FORCE_INLINE float GetRadius() const
    {
        return _radius;
    }

    /// <summary>
    /// Sets the radius of the cylinder, measured in the object's local space.
    /// </summary>
    API_PROPERTY() void SetRadius(float value);

    /// <summary>
    /// Gets the height of the cylinder, measured in the object's local space.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(110), DefaultValue(100.0f), EditorDisplay(\"Collider\"), ValueCategory(Utils.ValueCategory.Distance)")
    FORCE_INLINE float GetHeight() const
    {
        return _height;
    }

    /// <summary>
    /// Sets the height of the cylinder, measured in the object's local space.
    /// </summary>
    API_PROPERTY() void SetHeight(float value);

public:
    bool IntersectsItself(const Ray& ray, Real& distance, Vector3& normal) override;  

    /// <summary>
    /// Gets the quaternion offset. used to effset the visual based on selected axis
    /// </summary>
    /// <returns>Quaternion offset</returns>
    API_FUNCTION() Quaternion GetQuaternionOffset();
protected:
    // [Collider]
    void Internal_SetContactOffset(float value) override;
    void UpdateBounds() override;
    void GetGeometry(CollisionShape& collision) override;
#if USE_EDITOR
    void OnDebugDrawSelected() override;
    void DrawPhysicsDebug(RenderView& view) override;
    void OnDebugDraw() override;
#endif
};
