// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "ModelInstanceActor.h"
#include "Engine/Content/Assets/Model.h"

class Spline;

/// <summary>
/// Renders model over the spline segments.
/// </summary>
API_CLASS() class FLAXENGINE_API SplineModel : public ModelInstanceActor
{
DECLARE_SCENE_OBJECT(SplineModel);
private:

    struct Instance
    {
        BoundingSphere Sphere;
        float RotDeterminant;
    };

    float _boundsScale = 1.0f, _quality = 1.0f;
    char _lodBias = 0;
    char _forcedLod = -1;
    bool _deformationDirty = false;
    Array<Instance> _instances;
    Transform _preTransform = Transform::Identity;
    Spline* _spline = nullptr;
    GPUBuffer* _deformationBuffer = nullptr;
    void* _deformationBufferData = nullptr;
    float _chunksPerSegment, _meshMinZ, _meshMaxZ;

public:

    ~SplineModel();

    /// <summary>
    /// The model asset to draw.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), DefaultValue(null), EditorDisplay(\"Model\")")
    AssetReference<Model> Model;

    /// <summary>
    /// Gets the transformation applied to the model geometry before placing it over the spline. Can be used to change the way model goes over the spline.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(21), EditorDisplay(\"Model\")")
    Transform GetPreTransform() const;

    /// <summary>
    /// Sets the transformation applied to the model geometry before placing it over the spline. Can be used to change the way model goes over the spline.
    /// </summary>
    API_PROPERTY() void SetPreTransform(const Transform& value);

    /// <summary>
    /// The draw passes to use for rendering this object.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(15), DefaultValue(DrawPass.Default), EditorDisplay(\"Model\")")
    DrawPass DrawModes = DrawPass::Default;

    /// <summary>
    /// Gets the spline model quality scale. Higher values improve the spline representation (better tessellation) but reduce performance.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(11), DefaultValue(1.0f), EditorDisplay(\"Model\"), Limit(0.1f, 100.0f, 0.1f)")
    float GetQuality() const;

    /// <summary>
    /// Sets the spline model quality scale. Higher values improve the spline representation (better tessellation) but reduce performance.
    /// </summary>
    API_PROPERTY() void SetQuality(float value);

    /// <summary>
    /// Gets the model bounds scale. It is useful when using Position Offset to animate the vertices of the object outside of its bounds.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(12), DefaultValue(1.0f), EditorDisplay(\"Model\"), Limit(0, 10.0f, 0.1f)")
    float GetBoundsScale() const;

    /// <summary>
    /// Sets the model bounds scale. It is useful when using Position Offset to animate the vertices of the object outside of its bounds.
    /// </summary>
    API_PROPERTY() void SetBoundsScale(float value);

    /// <summary>
    /// Gets the model Level Of Detail bias value. Allows to increase or decrease rendered model quality.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(40), DefaultValue(0), Limit(-100, 100, 0.1f), EditorDisplay(\"Model\", \"LOD Bias\")")
    int32 GetLODBias() const;

    /// <summary>
    /// Sets the model Level Of Detail bias value. Allows to increase or decrease rendered model quality.
    /// </summary>
    API_PROPERTY() void SetLODBias(int32 value);

    /// <summary>
    /// Gets the model forced Level Of Detail index. Allows to bind the given model LOD to show. Value -1 disables this feature.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(50), DefaultValue(-1), Limit(-1, 100, 0.1f), EditorDisplay(\"Model\", \"Forced LOD\")")
    int32 GetForcedLOD() const;

    /// <summary>
    /// Sets the model forced Level Of Detail index. Allows to bind the given model LOD to show. Value -1 disables this feature.
    /// </summary>
    API_PROPERTY() void SetForcedLOD(int32 value);

private:

    void OnModelChanged();
    void OnModelLoaded();
    void OnSplineUpdated();
    void UpdateDeformationBuffer();

public:

    // [ModelInstanceActor]
    bool HasContentLoaded() const override;
    void Draw(RenderContext& renderContext) override;
    void DrawGeneric(RenderContext& renderContext) override;
    bool IntersectsItself(const Ray& ray, float& distance, Vector3& normal) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    void OnParentChanged() override;

protected:

    // [ModelInstanceActor]
    void OnTransformChanged() override;
};
