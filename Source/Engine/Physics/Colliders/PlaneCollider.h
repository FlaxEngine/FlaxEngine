// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Collider.h"
#include "Engine/Core/Math/OrientedBoundingBox.h"

/// <summary>
/// A capsule-shaped primitive collider.
/// </summary>
/// <remarks>Capsules are cylinders with a half-sphere at each end centered at the origin and extending along the X axis, and two hemispherical ends.</remarks>
/// <seealso cref="Collider" />
API_CLASS(Attributes="ActorContextMenu(\"New/Physics/Colliders/Plane Collider\"), ActorToolbox(\"Physics\")")
class FLAXENGINE_API PlaneCollider : public Collider
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCENE_OBJECT(PlaneCollider);
private:
    OrientedBoundingBox _orientedBox;
public:
    bool IntersectsItself(const Ray& ray, Real& distance, Vector3& normal) override;
protected:
    const float MaxBoundingBox = 100000.0f;//100 000 units

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
