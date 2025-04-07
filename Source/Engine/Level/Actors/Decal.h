// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../Actor.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Math/OrientedBoundingBox.h"
#include "Engine/Content/Assets/MaterialBase.h"
#include "Engine/Content/AssetReference.h"

/// <summary>
/// Actor that draws the can be used to draw a custom decals on top of the other objects.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Visuals/Decal\"), ActorToolbox(\"Visuals\")")
class FLAXENGINE_API Decal : public Actor
{
    DECLARE_SCENE_OBJECT(Decal);
private:
    Vector3 _size;
    OrientedBoundingBox _bounds;
    int32 _sceneRenderingKey = -1;

public:
    /// <summary>
    /// The decal material. Must have domain mode to Decal type.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), EditorDisplay(\"Decal\")")
    AssetReference<MaterialBase> Material;

    /// <summary>
    /// The decal rendering order. The higher values are render later (on top).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), EditorDisplay(\"Decal\")")
    int32 SortOrder = 0;

    /// <summary>
    /// The minimum screen size for the decal drawing. If the decal size on the screen is smaller than this value then decal will be culled. Set it to higher value to make culling more aggressive.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), EditorDisplay(\"Decal\")")
    float DrawMinScreenSize = 0.02f;

    /// <summary>
    /// Gets the decal bounds size (in local space).
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(30), DefaultValue(typeof(Vector3), \"100,100,100\"), Limit(0), EditorDisplay(\"Decal\")")
    FORCE_INLINE Vector3 GetSize() const
    {
        return _size;
    }

    /// <summary>
    /// Sets the decal bounds size (in local space).
    /// </summary>
    API_PROPERTY() void SetSize(const Vector3& value);

public:
    /// <summary>
    /// Utility to crate a new virtual Material Instance asset, set its parent to the currently applied material, and assign it to the decal. Can be used to modify the decal material parameters from code.
    /// </summary>
    /// <returns>The created virtual material instance.</returns>
    API_FUNCTION() MaterialInstance* CreateAndSetVirtualMaterialInstance();

public:
    // [Actor]
#if USE_EDITOR
    void OnDebugDrawSelected() override;
    BoundingBox GetEditorBox() const override;
#endif
    void OnLayerChanged() override;
    void Draw(RenderContext& renderContext) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    bool IntersectsItself(const Ray& ray, Real& distance, Vector3& normal) override;

protected:
    // [Actor]
    void OnEnable() override;
    void OnDisable() override;
    void OnTransformChanged() override;
};
