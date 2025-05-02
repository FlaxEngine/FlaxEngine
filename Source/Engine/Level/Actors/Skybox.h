// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../Actor.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Content/Assets/MaterialInstance.h"
#include "Engine/Content/Assets/CubeTexture.h"
#include "Engine/Renderer/RenderList.h"

/// <summary>
/// Skybox actor renders sky using custom cube texture or material.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Visuals/Sky/Sky Box\"), ActorToolbox(\"Visuals\")")
class FLAXENGINE_API Skybox : public Actor, public ISkyRenderer
{
    DECLARE_SCENE_OBJECT(Skybox);
private:
    AssetReference<MaterialInstance> _proxyMaterial;
    int32 _sceneRenderingKey = -1;

public:
    /// <summary>
    /// The cube texture to draw.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), DefaultValue(null), EditorDisplay(\"Skybox\")")
    AssetReference<CubeTexture> CubeTexture;

    /// <summary>
    /// The panoramic texture to draw. It should have a resolution ratio close to 2:1.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), DefaultValue(null), EditorDisplay(\"Skybox\")")
    AssetReference<Texture> PanoramicTexture;

    /// <summary>
    /// The skybox custom material used to override default (domain set to surface).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), DefaultValue(null), EditorDisplay(\"Skybox\")")
    AssetReference<MaterialBase> CustomMaterial;

    /// <summary>
    /// The skybox texture tint color.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), DefaultValue(typeof(Color), \"1,1,1,1\"), EditorDisplay(\"Skybox\")")
    Color Color = Color::White;

    /// <summary>
    /// The skybox texture exposure value. Can be used to make skybox brighter or dimmer.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(50), DefaultValue(0.0f), Limit(-100, 100, 0.01f), EditorDisplay(\"Skybox\")")
    float Exposure = 0.0f;

private:
    void setupProxy();

public:
    // [Actor]
#if USE_EDITOR
    BoundingBox GetEditorBox() const override
    {
        const Vector3 size(50);
        return BoundingBox(_transform.Translation - size, _transform.Translation + size);
    }
#endif
    void Draw(RenderContext& renderContext) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    bool HasContentLoaded() const override;
    bool IntersectsItself(const Ray& ray, Real& distance, Vector3& normal) override;

    // [ISkyRenderer]
    bool IsDynamicSky() const override;
    float GetIndirectLightingIntensity() const override;
    void ApplySky(GPUContext* context, RenderContext& renderContext, const Matrix& world) override;

protected:
    // [Actor]
    void OnEnable() override;
    void OnDisable() override;
    void OnTransformChanged() override;
};
