// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"
#include "Engine/Content/Assets/MaterialBase.h"
#include "Engine/Content/Assets/Texture.h"
#include "Engine/Render2D/SpriteAtlas.h"

/// <summary>
/// Sprite rendering object.
/// </summary>
API_CLASS() class FLAXENGINE_API SpriteRender : public Actor
{
DECLARE_SCENE_OBJECT(SpriteRender);
private:

    Color _color;
    Vector2 _size;
    SpriteHandle _sprite;
    MaterialInstance* _materialInstance = nullptr;
    MaterialParameter* _paramImage = nullptr;
    MaterialParameter* _paramImageMAD = nullptr;
    MaterialParameter* _paramColor = nullptr;
    AssetReference<Asset> _quadModel;

public:

    /// <summary>
    /// Gets the size of the sprite.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(0), EditorDisplay(\"Sprite\")")
    Vector2 GetSize() const;

    /// <summary>
    /// Sets the size of the sprite.
    /// </summary>
    API_PROPERTY() void SetSize(const Vector2& value);

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

private:

    void OnMaterialLoaded();
    void SetImage();

public:

    // [Actor]
    bool HasContentLoaded() const override;
    void Draw(RenderContext& renderContext) override;
    void DrawGeneric(RenderContext& renderContext) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    void OnEndPlay() override;

protected:

    // [Actor]
    void OnEnable() override;
    void OnDisable() override;
    void OnTransformChanged() override;
};
