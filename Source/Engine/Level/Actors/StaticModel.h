// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "ModelInstanceActor.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Renderer/Lightmaps.h"

/// <summary>
/// Renders model on the screen.
/// </summary>
API_CLASS() class FLAXENGINE_API StaticModel : public ModelInstanceActor
{
DECLARE_SCENE_OBJECT(StaticModel);
private:

    Matrix _world;
    GeometryDrawStateData _drawState;
    float _scaleInLightmap;
    float _boundsScale;
    char _lodBias;
    char _forcedLod;
    bool _vertexColorsDirty;
    byte _vertexColorsCount;
    Array<Color32> _vertexColorsData[MODEL_MAX_LODS];
    GPUBuffer* _vertexColorsBuffer[MODEL_MAX_LODS];

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
    /// Gets the model world matrix transform.
    /// </summary>
    /// <param name="world">The result world matrix.</param>
    FORCE_INLINE void GetWorld(Matrix* world) const
    {
        *world = _world;
    }

    /// <summary>
    /// Gets the model scale in lightmap (applied to all the meshes).
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(10), DefaultValue(1.0f), EditorDisplay(\"Model\", \"Scale In Lightmap\"), Limit(0, 1000.0f, 0.1f)")
    FORCE_INLINE float GetScaleInLightmap() const
    {
        return _scaleInLightmap;
    }

    /// <summary>
    /// Sets the model scale in lightmap (applied to all the meshes).
    /// </summary>
    API_PROPERTY() void SetScaleInLightmap(float value);

    /// <summary>
    /// Gets the model bounds scale. It is useful when using Position Offset to animate the vertices of the object outside of its bounds. Increasing the bounds of an object will reduce performance.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(12), DefaultValue(1.0f), EditorDisplay(\"Model\"), Limit(0, 10.0f, 0.1f)")
    FORCE_INLINE float GetBoundsScale() const
    {
        return _boundsScale;
    }

    /// <summary>
    /// Sets the model bounds scale. It is useful when using Position Offset to animate the vertices of the object outside of its bounds.
    /// </summary>
    API_PROPERTY() void SetBoundsScale(float value);

    /// <summary>
    /// Gets the model Level Of Detail bias value. Allows to increase or decrease rendered model quality.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(40), DefaultValue(0), Limit(-100, 100, 0.1f), EditorDisplay(\"Model\", \"LOD Bias\")")
    FORCE_INLINE int32 GetLODBias() const
    {
        return static_cast<int32>(_lodBias);
    }

    /// <summary>
    /// Sets the model Level Of Detail bias value. Allows to increase or decrease rendered model quality.
    /// </summary>
    API_PROPERTY() void SetLODBias(int32 value)
    {
        _lodBias = static_cast<char>(Math::Clamp(value, -100, 100));
    }

    /// <summary>
    /// Gets the model forced Level Of Detail index. Allows to bind the given model LOD to show. Value -1 disables this feature.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(50), DefaultValue(-1), Limit(-1, 100, 0.1f), EditorDisplay(\"Model\", \"Forced LOD\")")
    FORCE_INLINE int32 GetForcedLOD() const
    {
        return static_cast<int32>(_forcedLod);
    }

    /// <summary>
    /// Sets the model forced Level Of Detail index. Allows to bind the given model LOD to show. Value -1 disables this feature.
    /// </summary>
    API_PROPERTY() void SetForcedLOD(int32 value)
    {
        _forcedLod = static_cast<char>(Math::Clamp(value, -1, 100));
    }

    /// <summary>
    /// Determines whether this model has valid lightmap data.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool HasLightmap() const
    {
        return Lightmap.TextureIndex != INVALID_INDEX;
    }

    /// <summary>
    /// Removes the lightmap data from the model.
    /// </summary>
    API_FUNCTION() FORCE_INLINE void RemoveLightmap()
    {
        Lightmap.TextureIndex = INVALID_INDEX;
    }

    /// <summary>
    /// Gets the material used to render mesh at given index (overriden by model instance buffer or model default).
    /// </summary>
    /// <param name="meshIndex">The zero-based mesh index.</param>
    /// <param name="lodIndex">The LOD index.</param>
    /// <returns>Material or null if not assigned.</returns>
    API_FUNCTION() MaterialBase* GetMaterial(int32 meshIndex, int32 lodIndex = 0) const;

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
    API_PROPERTY() bool HasVertexColors() const
    {
        return _vertexColorsCount != 0;
    }

    /// <summary>
    /// Removes the vertex colors buffer from this instance.
    /// </summary>
    API_FUNCTION() void RemoveVertexColors();

private:

    void OnModelChanged();
    void OnModelLoaded();
    void UpdateBounds();

public:

    // [ModelInstanceActor]
    bool HasContentLoaded() const override;
    void Draw(RenderContext& renderContext) override;
    void DrawGeneric(RenderContext& renderContext) override;
    bool IntersectsItself(const Ray& ray, float& distance, Vector3& normal) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    bool IntersectsEntry(int32 entryIndex, const Ray& ray, float& distance, Vector3& normal) override;
    bool IntersectsEntry(const Ray& ray, float& distance, Vector3& normal, int32& entryIndex) override;

protected:

    // [ModelInstanceActor]
    void OnTransformChanged() override;
};
