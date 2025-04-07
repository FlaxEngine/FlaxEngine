// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "ModelInstanceActor.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Renderer/Lightmaps.h"

/// <summary>
/// Renders model on the screen.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Model\"), ActorToolbox(\"Visuals\")")
class FLAXENGINE_API StaticModel : public ModelInstanceActor
{
    DECLARE_SCENE_OBJECT(StaticModel);
private:
    GeometryDrawStateData _drawState;
    float _scaleInLightmap;
    float _boundsScale;
    char _lodBias;
    char _forcedLod;
    bool _vertexColorsDirty;
    byte _vertexColorsCount;
    int8 _sortOrder;
    Array<Color32> _vertexColorsData[MODEL_MAX_LODS];
    GPUBuffer* _vertexColorsBuffer[MODEL_MAX_LODS];
    Model* _residencyChangedModel = nullptr;
    mutable MeshDeformation* _deformation = nullptr;

public:
    /// <summary>
    /// Finalizes an instance of the <see cref="StaticModel"/> class.
    /// </summary>
    ~StaticModel();

    /// <summary>
    /// The model asset to draw.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), DefaultValue(null), EditorDisplay(\"Model\")")
    AssetReference<Model> Model;

    /// <summary>
    /// The draw passes to use for rendering this object.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(15), DefaultValue(DrawPass.Default), EditorDisplay(\"Model\")")
    DrawPass DrawModes = DrawPass::Default;

    /// <summary>
    /// The baked lightmap entry.
    /// </summary>
    LightmapEntry Lightmap;

public:
    /// <summary>
    /// Gets the model scale in lightmap (applied to all the meshes).
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(10), DefaultValue(1.0f), EditorDisplay(\"Model\", \"Scale In Lightmap\"), Limit(0, 1000.0f, 0.1f)")
    float GetScaleInLightmap() const;

    /// <summary>
    /// Sets the model scale in lightmap (applied to all the meshes).
    /// </summary>
    API_PROPERTY() void SetScaleInLightmap(float value);

    /// <summary>
    /// Gets the model bounds scale. It is useful when using Position Offset to animate the vertices of the object outside of its bounds. Increasing the bounds of an object will reduce performance.
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

    /// <summary>
    /// Gets the model sort order key used when sorting drawable objects during rendering. Use lower values to draw object before others, higher values are rendered later (on top). Can be used to control transparency drawing.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(60), DefaultValue(0), EditorDisplay(\"Model\")")
    int32 GetSortOrder() const;

    /// <summary>
    /// Sets the model sort order key used when sorting drawable objects during rendering. Use lower values to draw object before others, higher values are rendered later (on top). Can be used to control transparency drawing.
    /// </summary>
    API_PROPERTY() void SetSortOrder(int32 value);

    /// <summary>
    /// Determines whether this model has valid lightmap data.
    /// </summary>
    API_PROPERTY() bool HasLightmap() const;

    /// <summary>
    /// Removes the lightmap data from the model.
    /// </summary>
    API_FUNCTION() void RemoveLightmap();

    /// <summary>
    /// Gets the material used to render mesh at given index (overriden by model instance buffer or model default).
    /// </summary>
    /// <param name="meshIndex">The zero-based mesh index.</param>
    /// <param name="lodIndex">The LOD index.</param>
    /// <returns>Material or null if not assigned.</returns>
    API_FUNCTION() MaterialBase* GetMaterial(int32 meshIndex, int32 lodIndex) const;

    /// <summary>
    /// Gets the color of the painter vertex (this model instance).
    /// </summary>
    /// <param name="lodIndex">The model LOD index.</param>
    /// <param name="meshIndex">The mesh index.</param>
    /// <param name="vertexIndex">The vertex index.</param>
    /// <returns>The color of the vertex.</returns>
    API_FUNCTION() Color32 GetVertexColor(int32 lodIndex, int32 meshIndex, int32 vertexIndex) const;

    /// <summary>
    /// Sets the color of the painter vertex (this model instance).
    /// </summary>
    /// <param name="lodIndex">The model LOD index.</param>
    /// <param name="meshIndex">The mesh index.</param>
    /// <param name="vertexIndex">The vertex index.</param>
    /// <param name="color">The color to set.</param>
    API_FUNCTION() void SetVertexColor(int32 lodIndex, int32 meshIndex, int32 vertexIndex, const Color32& color);

    /// <summary>
    /// Returns true if model instance is using custom painted vertex colors buffer, otherwise it will use vertex colors from the original asset.
    /// </summary>
    API_PROPERTY() bool HasVertexColors() const;

    /// <summary>
    /// Removes the vertex colors buffer from this instance.
    /// </summary>
    API_FUNCTION() void RemoveVertexColors();

private:
    void OnModelChanged();
    void OnModelLoaded();
    void OnModelResidencyChanged();
    void FlushVertexColors();

public:
    // [ModelInstanceActor]
    bool HasContentLoaded() const override;
    void Draw(RenderContext& renderContext) override;
    void Draw(RenderContextBatch& renderContextBatch) override;
    bool IntersectsItself(const Ray& ray, Real& distance, Vector3& normal) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    const Span<MaterialSlot> GetMaterialSlots() const override;
    MaterialBase* GetMaterial(int32 entryIndex) override;
    bool IntersectsEntry(int32 entryIndex, const Ray& ray, Real& distance, Vector3& normal) override;
    bool IntersectsEntry(const Ray& ray, Real& distance, Vector3& normal, int32& entryIndex) override;
    bool GetMeshData(const MeshReference& ref, MeshBufferType type, BytesContainer& result, int32& count, GPUVertexLayout** layout) const override;
    MeshDeformation* GetMeshDeformation() const override;
    void UpdateBounds() override;

protected:
    // [ModelInstanceActor]
    void OnEnable() override;
    void OnDisable() override;
    void WaitForModelLoad() override;
};
