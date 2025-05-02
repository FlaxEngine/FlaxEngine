// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../Actor.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Renderer/Config.h"
#include "Engine/Renderer/DrawCall.h"

class GPUPipelineState;

/// <summary>
/// Sky actor renders atmosphere around the scene with fog and sky.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Visuals/Sky/Sky\"), ActorToolbox(\"Visuals\")")
class FLAXENGINE_API Sky : public Actor, public IAtmosphericFogRenderer, public ISkyRenderer
{
    DECLARE_SCENE_OBJECT(Sky);
private:
    AssetReference<Shader> _shader;
    GPUPipelineState* _psSky;
    GPUPipelineState* _psFog;
    int32 _sceneRenderingKey = -1;

public:
    ~Sky();

public:
    /// <summary>
    /// Directional light that is used to simulate the sun.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), EditorDisplay(\"Sky\")")
    ScriptingObjectReference<DirectionalLight> SunLight;

    /// <summary>
    /// The sun disc scale.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), EditorDisplay(\"Sky\"), Limit(0, 100, 0.01f)")
    float SunDiscScale = 2.0f;

    /// <summary>
    /// The sun power.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), EditorDisplay(\"Sky\"), Limit(0, 1000, 0.01f)")
    float SunPower = 8.0f;

    /// <summary>
    /// Controls how much sky will contribute indirect lighting. When set to 0, there is no GI from the sky. The default value is 1.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), Limit(0, 100, 0.1f), EditorDisplay(\"Sky\")")
    float IndirectLightingIntensity = 1.0f;

private:
#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psSky = nullptr;
        _psFog = nullptr;
    }
#endif
    void InitConfig(ShaderAtmosphericFogData& config) const;

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

    // [IAtmosphericFogRenderer]
    void DrawFog(GPUContext* context, RenderContext& renderContext, GPUTextureView* output) override;

    // [ISkyRenderer]
    bool IsDynamicSky() const override;
    float GetIndirectLightingIntensity() const override;
    void ApplySky(GPUContext* context, RenderContext& renderContext, const Matrix& world) override;

protected:
    // [Actor]
    void EndPlay() override;
    void OnEnable() override;
    void OnDisable() override;
    void OnTransformChanged() override;
};
