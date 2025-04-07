// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"
#include "Engine/Render2D/TextLayoutOptions.h"
#include "Engine/Render2D/FontAsset.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Content/Assets/MaterialBase.h"
#include "Engine/Content/Assets/MaterialInstance.h"
#include "Engine/Graphics/DynamicBuffer.h"
#include "Engine/Graphics/Models/Config.h"
#include "Engine/Localization/LocalizedString.h"
#if MODEL_USE_PRECISE_MESH_INTERSECTS
#include "Engine/Graphics/Models/CollisionProxy.h"
#endif

/// <summary>
/// Text rendering object.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/UI/Text Render\"), ActorToolbox(\"Visuals\")")
class FLAXENGINE_API TextRender : public Actor
{
    DECLARE_SCENE_OBJECT(TextRender);
private:
    struct DrawChunk
    {
        TextRender* Actor;
        int32 FontAtlasIndex;
        int32 StartIndex;
        int32 IndicesCount;
        AssetReference<MaterialInstance> Material;
    };

    bool _isDirty = false;
    bool _buffersDirty = false;
    bool _isLocalized = false;
    LocalizedString _text;
    Color _color;
    TextLayoutOptions _layoutOptions;
    float _size;
    int32 _sceneRenderingKey = -1;

    BoundingBox _localBox;
    GeometryDrawStateData _drawState;
    DynamicIndexBuffer _ib;
    DynamicVertexBuffer _vb;
#if MODEL_USE_PRECISE_MESH_INTERSECTS
    CollisionProxy _collisionProxy;
#endif
    Array<DrawChunk, InlinedAllocation<8>> _drawChunks;

public:
    /// <summary>
    /// Gets the text.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(0), MultilineText, EditorDisplay(\"Text\")")
    const LocalizedString& GetText() const;

    /// <summary>
    /// Sets the text.
    /// </summary>
    API_PROPERTY() void SetText(const LocalizedString& value);

    /// <summary>
    /// Gets the color of the text.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(10), DefaultValue(typeof(Color), \"1,1,1,1\"), EditorDisplay(\"Text\")")
    Color GetColor() const;

    /// <summary>
    /// Sets the color of the text.
    /// </summary>
    API_PROPERTY() void SetColor(const Color& value);

    /// <summary>
    /// The material used for the text rendering. It must contain texture parameter named Font used to sample font texture.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), DefaultValue(null), AssetReference(true), EditorDisplay(\"Text\")")
    AssetReference<MaterialBase> Material;

    /// <summary>
    /// The font asset used as a text characters source.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), DefaultValue(null), AssetReference(true), EditorDisplay(\"Text\")")
    AssetReference<FontAsset> Font;

    /// <summary>
    /// Gets the font characters size.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(40), DefaultValue(32), Limit(1, 1000), EditorDisplay(\"Text\")")
    float GetFontSize() const;

    /// <summary>
    /// Sets the font characters size.
    /// </summary>
    API_PROPERTY() void SetFontSize(float value);

    /// <summary>
    /// The draw passes to use for rendering this object.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(75), DefaultValue(DrawPass.Default), EditorDisplay(\"Text\")")
    DrawPass DrawModes = DrawPass::Default;

    /// <summary>
    /// The shadows casting mode by this visual element.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(80), DefaultValue(ShadowsCastingMode.All), EditorDisplay(\"Text\")")
    ShadowsCastingMode ShadowsMode = ShadowsCastingMode::All;

    /// <summary>
    /// The object sort order key used when sorting drawable objects during rendering. Use lower values to draw object before others, higher values are rendered later (on top). Can be used to control transparency drawing.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(85), DefaultValue(0), EditorDisplay(\"Text\")")
    int8 SortOrder = 0;

    /// <summary>
    /// Gets the layout options. Layout is defined in local space of the object (on XY plane).
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(100), EditorDisplay(\"Text\")")
    FORCE_INLINE const TextLayoutOptions& GetLayoutOptions() const
    {
        return _layoutOptions;
    }

    /// <summary>
    /// Sets the layout options. Layout is defined in local space of the object (on XY plane).
    /// </summary>
    API_PROPERTY() void SetLayoutOptions(TextLayoutOptions& value);

    /// <summary>
    /// Gets the axis=aligned bounding box of the text vertices in the local-space of the actor.
    /// </summary>
    API_PROPERTY() FORCE_INLINE BoundingBox GetLocalBox() const
    {
        return _localBox;
    }

    /// <summary>
    /// Updates the text vertex buffer layout and cached data if its dirty.
    /// </summary>
    API_FUNCTION() void UpdateLayout();

#if MODEL_USE_PRECISE_MESH_INTERSECTS
    /// <summary>
    /// Gets the collision proxy used by the text geometry.
    /// </summary>
    FORCE_INLINE const CollisionProxy& GetCollisionProxy() const
    {
        return _collisionProxy;
    }
#endif

private:
    void Invalidate()
    {
        // Invalidate data
        _isDirty = true;
    }

public:
    // [Actor]
    bool HasContentLoaded() const override;
    void Draw(RenderContext& renderContext) override;
#if USE_EDITOR
    void OnDebugDrawSelected() override;
#endif
    void OnLayerChanged() override;
    bool IntersectsItself(const Ray& ray, Real& distance, Vector3& normal) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;

protected:
    // [Actor]
    void OnEnable() override;
    void OnDisable() override;
    void OnTransformChanged() override;
};
