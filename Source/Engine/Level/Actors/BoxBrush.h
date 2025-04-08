// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../Actor.h"
#include "Engine/Core/Math/OrientedBoundingBox.h"
#include "Engine/Content/Assets/MaterialBase.h"
#include "Engine/CSG/Brush.h"

/// <summary>
/// Represents a part of the CSG brush actor. Contains information about single surface.
/// </summary>
API_STRUCT() struct BrushSurface : ISerializable
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(BrushSurface);

    /// <summary>
    /// The parent brush.
    /// </summary>
    API_FIELD(Attributes="HideInEditor")
    BoxBrush* Brush = nullptr;

    /// <summary>
    /// The surface index in the parent brush surfaces list.
    /// </summary>
    API_FIELD(Attributes="HideInEditor")
    int32 Index = -1;

    /// <summary>
    /// The material used to render the brush surface.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), EditorDisplay(\"Brush\")")
    AssetReference<MaterialBase> Material;

    /// <summary>
    /// The surface texture coordinates scale.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), EditorDisplay(\"Brush\", \"UV Scale\"), Limit(-1000, 1000, 0.01f)")
    Float2 TexCoordScale = Float2::One;

    /// <summary>
    /// The surface texture coordinates offset.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), EditorDisplay(\"Brush\", \"UV Offset\"), Limit(-1000, 1000, 0.01f)")
    Float2 TexCoordOffset = Float2::Zero;

    /// <summary>
    /// The surface texture coordinates rotation angle (in degrees).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(50), EditorDisplay(\"Brush\", \"UV Rotation\")")
    float TexCoordRotation = 0.0f;

    /// <summary>
    /// The scale in lightmap (per surface).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), EditorDisplay(\"Brush\", \"Scale In Lightmap\"), Limit(0, 10000, 0.1f)")
    float ScaleInLightmap = 1.0f;

public:
    // [ISerializable]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};

/// <summary>
/// Performs CSG box brush operation that adds or removes geometry.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Other/CSG Box Brush\"), ActorToolbox(\"Other\", \"CSG Box Brush\")")
class FLAXENGINE_API BoxBrush : public Actor, public CSG::Brush
{
    DECLARE_SCENE_OBJECT(BoxBrush);
private:
    Vector3 _center;
    Vector3 _size;
    OrientedBoundingBox _bounds;
    BrushMode _mode;

public:
    /// <summary>
    /// Brush surfaces scale in lightmap
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), DefaultValue(1.0f), EditorDisplay(\"CSG\", \"Scale In Lightmap\"), Limit(0, 1000.0f, 0.1f)")
    float ScaleInLightmap = 1.0f;

    /// <summary>
    /// Brush proxy per surface
    /// </summary>
    BrushSurface Surfaces[6];

    /// <summary>
    /// Gets the brush proxies per surface.
    /// </summary>
    API_PROPERTY(Attributes="Serialize, EditorOrder(100), EditorDisplay(\"Surfaces\", EditorDisplayAttribute.InlineStyle), Collection(CanReorderItems = false, NotNullItems = true, CanResize = true)")
    Array<BrushSurface> GetSurfaces() const;

    /// <summary>
    /// Sets the brush proxies per surface.
    /// </summary>
    API_PROPERTY() void SetSurfaces(const Array<BrushSurface>& value);

    /// <summary>
    /// Gets the CSG brush mode.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(10), DefaultValue(BrushMode.Additive), EditorDisplay(\"CSG\")")
    FORCE_INLINE BrushMode GetMode() const
    {
        return _mode;
    }

    /// <summary>
    /// Sets the CSG brush mode.
    /// </summary>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetMode(BrushMode value);

    /// <summary>
    /// Gets the brush center (in local space).
    /// </summary>
    /// <returns>The value.</returns>
    API_PROPERTY(Attributes="EditorOrder(21), DefaultValue(typeof(Vector3), \"0,0,0\"), EditorDisplay(\"CSG\")")
    FORCE_INLINE Vector3 GetCenter() const
    {
        return _center;
    }

    /// <summary>
    /// Sets the brush center (in local space).
    /// </summary>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetCenter(const Vector3& value);

    /// <summary>
    /// Gets the brush size.
    /// </summary>
    /// <returns>The value.</returns>
    API_PROPERTY(Attributes="EditorOrder(20), EditorDisplay(\"CSG\")")
    FORCE_INLINE Vector3 GetSize() const
    {
        return _size;
    }

    /// <summary>
    /// Sets the brush size.
    /// </summary>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetSize(const Vector3& value);

    /// <summary>
    /// Gets CSG surfaces
    /// </summary>
    /// <param name="surfaces">Surfaces</param>
    void GetSurfaces(CSG::Surface surfaces[6]);

    /// <summary>
    /// Sets the brush surface material.
    /// </summary>
    /// <param name="surfaceIndex">The brush surface index.</param>
    /// <param name="material">The material.</param>
    API_FUNCTION() void SetMaterial(int32 surfaceIndex, MaterialBase* material);

public:
    /// <summary>
    /// Gets the volume bounding box (oriented).
    /// </summary>
    API_PROPERTY() FORCE_INLINE OrientedBoundingBox GetOrientedBox() const
    {
        return _bounds;
    }

    /// <summary>
    /// Determines if there is an intersection between the brush surface and a ray.
    /// If collision data is available on the CPU performs exact intersection check with the geometry.
    /// Otherwise performs simple <see cref="BoundingBox"/> vs <see cref="Ray"/> test.
    /// For more efficient collisions detection and ray casting use physics.
    /// </summary>
    /// <param name="surfaceIndex">The brush surface index.</param>
    /// <param name="ray">The ray to test.</param>
    /// <param name="distance">When the method completes and returns true, contains the distance of the intersection (if any valid).</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
    /// <returns>True if the actor is intersected by the ray, otherwise false.</returns>
    API_FUNCTION() bool Intersects(int32 surfaceIndex, API_PARAM(Ref) const Ray& ray, API_PARAM(Out) Real& distance, API_PARAM(Out) Vector3& normal) const;

    /// <summary>
    /// Gets the brush surface triangles array (group by 3 vertices).
    /// </summary>
    /// <param name="surfaceIndex">The brush surface index.</param>
    /// <param name="outputData">The output vertices buffer with triangles or empty if no data loaded.</param>
    API_FUNCTION() void GetVertices(int32 surfaceIndex, API_PARAM(Out) Array<Vector3>& outputData) const;

private:
    FORCE_INLINE void UpdateBounds()
    {
        OrientedBoundingBox::CreateCentered(_center, _size, _bounds);
        _bounds.Transform(_transform);
        _bounds.GetBoundingBox(_box);
        BoundingSphere::FromBox(_box, _sphere);
    }

public:
    // [Actor]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    bool IntersectsItself(const Ray& ray, Real& distance, Vector3& normal) override;
#if USE_EDITOR
    void OnDebugDrawSelected() override;
#endif

    // [CSG::Brush]
    Scene* GetBrushScene() const override;
    Guid GetBrushID() const override;
    bool CanUseCSG() const override;
    CSG::Mode GetBrushMode() const override;
    void GetSurfaces(Array<CSG::Surface>& surfaces) override;
    int32 GetSurfacesCount() override;

protected:
    // [Actor]
    void OnTransformChanged() override;
    void OnActiveInTreeChanged() override;
    void OnOrderInParentChanged() override;
    void OnParentChanged() override;
};
