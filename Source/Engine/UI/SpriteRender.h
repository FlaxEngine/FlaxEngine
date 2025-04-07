// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"
#include "Engine/Content/Assets/MaterialBase.h"
#include "Engine/Content/Assets/Texture.h"
#include "Engine/Render2D/SpriteAtlas.h"

/// <summary>
/// Sprite rendering object.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/UI/Sprite Render\"), ActorToolbox(\"Visuals\")")
class FLAXENGINE_API SpriteRender : public Actor
{
    DECLARE_SCENE_OBJECT(SpriteRender);
private:
    Color _color;
    Float2 _size;
    SpriteHandle _sprite;
    MaterialInstance* _materialInstance = nullptr;
    MaterialParameter* _paramImage = nullptr;
    MaterialParameter* _paramImageMAD = nullptr;
    MaterialParameter* _paramColor = nullptr;
    AssetReference<Asset> _quadModel;
    int32 _sceneRenderingKey = -1;

public:
    /// <summary>
    /// Gets the size of the sprite.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(0), EditorDisplay(\"Sprite\")")
    Float2 GetSize() const;

    /// <summary>
    /// Sets the size of the sprite.
    /// </summary>
    API_PROPERTY() void SetSize(const Float2& value);

    /// <summary>
    /// Gets the color of the sprite. Passed to the sprite material in parameter named `Color`.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(10), DefaultValue(typeof(Color), \"1,1,1,1\"), EditorDisplay(\"Sprite\")")
    Color GetColor() const;

    /// <summary>
    /// Sets the color of the sprite. Passed to the sprite material in parameter named `Color`.
    /// </summary>
    API_PROPERTY() void SetColor(const Color& value);

    /// <summary>
    /// The sprite texture to draw.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), DefaultValue(null), EditorDisplay(\"Sprite\")")
    AssetReference<Texture> Image;

    /// <summary>
    /// Gets the sprite to draw. Used only if Image is unset.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(25), EditorDisplay(\"Sprite\")")
    SpriteHandle GetSprite() const;

    /// <summary>
    /// Sets the sprite to draw. Used only if Image is unset.
    /// </summary>
    API_PROPERTY()
    void SetSprite(const SpriteHandle& value);

    /// <summary>
    /// The material used for the sprite rendering. It should contain texture parameter named Image and color parameter named Color. For showing sprites from sprite atlas ensure to add Vector4 param ImageMAD for UVs transformation.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), DefaultValue(null), EditorDisplay(\"Sprite\")")
    AssetReference<MaterialBase> Material;

    /// <summary>
    /// If checked, the sprite will automatically face the view camera, otherwise it will be oriented as an actor.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), EditorDisplay(\"Sprite\")")
    bool FaceCamera = true;

    /// <summary>
    /// The draw passes to use for rendering this object. Uncheck `Depth` to disable sprite casting shadows.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(50), DefaultValue(DrawPass.Default), EditorDisplay(\"Sprite\")")
    DrawPass DrawModes = DrawPass::Default;

    /// <summary>
    /// Gets the object sort order key used when sorting drawable objects during rendering. Use lower values to draw object before others, higher values are rendered later (on top). Can be used to control transparency drawing.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(60), DefaultValue(0), EditorDisplay(\"Sprite\")")
    int8 SortOrder = 0;

private:
    void OnMaterialLoaded();
    void SetImageParam();
    void SetColorParam();

public:
    // [Actor]
    bool HasContentLoaded() const override;
    void Draw(RenderContext& renderContext) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    void OnLayerChanged() override;
    void OnEndPlay() override;

protected:
    // [Actor]
    void OnEnable() override;
    void OnDisable() override;
    void OnTransformChanged() override;
};
