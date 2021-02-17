// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Collider.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Physics/CollisionData.h"

/// <summary>
/// A collider represented by an arbitrary mesh.
/// </summary>
/// <seealso cref="Collider" />
API_CLASS() class FLAXENGINE_API MeshCollider : public Collider
{
DECLARE_SCENE_OBJECT(MeshCollider);
public:

    /// <summary>
    /// Linked collision data asset that contains convex mesh or triangle mesh used to represent a mesh collider shape.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(100), DefaultValue(null), EditorDisplay(\"Collider\")")
    AssetReference<CollisionData> CollisionData;

private:

    void OnCollisionDataChanged();
    void OnCollisionDataLoaded();

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

protected:

    // [Collider]
#if USE_EDITOR
    void DrawPhysicsDebug(RenderView& view) override;
#endif
    void UpdateBounds() override;
    void GetGeometry(PxGeometryHolder& geometry) override;
};
