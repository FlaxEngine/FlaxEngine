// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Collider.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Physics/CollisionData.h"

class Spline;

/// <summary>
/// A collider represented by an arbitrary mesh that goes over the spline.
/// </summary>
/// <seealso cref="Collider" />
/// <seealso cref="Spline" />
API_CLASS() class FLAXENGINE_API SplineCollider : public Collider
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCENE_OBJECT(SplineCollider);
private:
    Spline* _spline = nullptr;
    void* _triangleMesh = nullptr;
    Array<Float3> _vertexBuffer;
    Array<int32> _indexBuffer;
    Transform _preTransform = Transform::Identity;

public:
    /// <summary>
    /// Linked collision data asset that contains convex mesh or triangle mesh used to represent a spline collider shape.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(100), DefaultValue(null), EditorDisplay(\"Collider\")")
    AssetReference<CollisionData> CollisionData;

    /// <summary>
    /// Gets the transformation applied to the collision data model geometry before placing it over the spline. Can be used to change the way model goes over the spline.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(101), EditorDisplay(\"Collider\")")
    Transform GetPreTransform() const;

    /// <summary>
    /// Sets the transformation applied to the collision data model geometry before placing it over the spline. Can be used to change the way model goes over the spline.
    /// </summary>
    API_PROPERTY() void SetPreTransform(const Transform& value);

    /// <summary>
    /// Extracts the collision data geometry into list of triangles.
    /// </summary>
    /// <param name="vertexBuffer">The output vertex buffer.</param>
    /// <param name="indexBuffer">The output index buffer.</param>
    void ExtractGeometry(Array<Float3>& vertexBuffer, Array<int32>& indexBuffer) const;

private:
    void OnCollisionDataChanged();
    void OnCollisionDataLoaded();
    void OnSplineUpdated();

public:
    // [Collider]
    bool CanAttach(RigidBody* rigidBody) const override;
    bool CanBeTrigger() const override;
#if USE_EDITOR
    void OnDebugDrawSelected() override;
#endif
    bool IntersectsItself(const Ray& ray, Real& distance, Vector3& normal) override;
    void OnParentChanged() override;
    void EndPlay() override;

protected:
    // [Collider]
    ImplementPhysicsDebug;
    void UpdateBounds() override;
    void GetGeometry(CollisionShape& collision) override;
};
