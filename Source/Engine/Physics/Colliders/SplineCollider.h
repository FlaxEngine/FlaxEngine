// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
DECLARE_SCENE_OBJECT(SplineCollider);
private:
    Spline* _spline = nullptr;
    PxTriangleMesh* _triangleMesh = nullptr;
    Array<Vector3> _vertexBuffer;
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
    void ExtractGeometry(Array<Vector3>& vertexBuffer, Array<int32>& indexBuffer) const;

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
    bool IntersectsItself(const Ray& ray, float& distance, Vector3& normal) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    void OnParentChanged() override;
    void EndPlay() override;

protected:

    // [Collider]
#if USE_EDITOR
    void DrawPhysicsDebug(RenderView& view) override;
#endif
    void UpdateBounds() override;
    void GetGeometry(PxGeometryHolder& geometry) override;
};
