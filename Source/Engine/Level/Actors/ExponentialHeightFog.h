// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../Actor.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Graphics/GPUPipelineStatePermutations.h"

/// <summary>
/// Used to create fogging effects such as clouds but with a density that is related to the height of the fog.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Visuals/Lighting & PostFX/Exponential Height Fog\"), ActorToolbox(\"Visuals\")")
class FLAXENGINE_API ExponentialHeightFog : public Actor, public IFogRenderer
{
    DECLARE_SCENE_OBJECT(ExponentialHeightFog);
private:
    AssetReference<Shader> _shader;
    GPUPipelineStatePermutationsPs<2> _psFog;
    int32 _sceneRenderingKey = -1;

public:
    /// <summary>
    /// The fog density factor.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), DefaultValue(0.02f), Limit(0.0000001f, 100.0f, 0.001f), EditorDisplay(\"Exponential Height Fog\")")
    float FogDensity = 0.02f;

    /// <summary>
    /// The fog height density factor that controls how the density increases as height decreases. Smaller values produce a more visible transition layer.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), DefaultValue(0.2f), Limit(0.0001f, 10.0f, 0.001f), EditorDisplay(\"Exponential Height Fog\")")
    float FogHeightFalloff = 0.2f;

    /// <summary>
    /// Color of the fog.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), DefaultValue(typeof(Color), \"0.448,0.634,1.0\"), EditorDisplay(\"Exponential Height Fog\")")
    Color FogInscatteringColor = Color(0.448f, 0.634f, 1.0f);

    /// <summary>
    /// Maximum opacity of the fog.
    /// A value of 1 means the fog can become fully opaque at a distance and replace scene color completely.
    /// A value of 0 means the fog color will not be factored in at all.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), DefaultValue(1.0f), Limit(0, 1, 0.001f), EditorDisplay(\"Exponential Height Fog\")")
    float FogMaxOpacity = 1.0f;

    /// <summary>
    /// Distance from the camera that the fog will start, in world units.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(50), DefaultValue(0.0f), Limit(0), EditorDisplay(\"Exponential Height Fog\")")
    float StartDistance = 0.0f;

    /// <summary>
    /// Scene elements past this distance will not have fog applied. This is useful for excluding skyboxes which already have fog baked in. Setting this value to 0 disables it. Negative value sets the cutoff distance relative to the far plane of the camera.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(60), DefaultValue(0.0f), EditorDisplay(\"Exponential Height Fog\")")
    float FogCutoffDistance = 0.0f;

public:
    /// <summary>
    /// Directional light used for Directional Inscattering.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(200), DefaultValue(null), EditorDisplay(\"Directional Inscattering\", \"Light\")")
    ScriptingObjectReference<DirectionalLight> DirectionalInscatteringLight;

    /// <summary>
    /// Controls the size of the directional inscattering cone, which is used to approximate inscattering from a directional light.
    /// Note: there must be a directional light enabled for DirectionalInscattering to be used. Range: 2-64.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(210), DefaultValue(4.0f), Limit(2, 64, 0.1f), EditorDisplay(\"Directional Inscattering\", \"Exponent\")")
    float DirectionalInscatteringExponent = 4.0f;

    /// <summary>
    /// Controls the start distance from the viewer of the directional inscattering, which is used to approximate inscattering from a directional light.
    /// Note: there must be a directional light enabled for DirectionalInscattering to be used.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(220), DefaultValue(10000.0f), Limit(0), EditorDisplay(\"Directional Inscattering\", \"Start Distance\")")
    float DirectionalInscatteringStartDistance = 10000.0f;

    /// <summary>
    /// Controls the color of the directional inscattering, which is used to approximate inscattering from a directional light.
    /// Note: there must be a directional light enabled for DirectionalInscattering to be used.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(230), DefaultValue(typeof(Color), \"0.25,0.25,0.125\"), EditorDisplay(\"Directional Inscattering\", \"Color\")")
    Color DirectionalInscatteringColor = Color(0.25, 0.25f, 0.125f);

public:
    /// <summary>
    /// Whether to enable Volumetric fog. Graphics quality settings control the resolution of the fog simulation.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(300), DefaultValue(false), EditorDisplay(\"Volumetric Fog\", \"Enable\")")
    bool VolumetricFogEnable = false;

    /// <summary>
    /// Controls the scattering phase function - how much incoming light scatters in various directions.
    /// A distribution value of 0 scatters equally in all directions, while 0.9 scatters predominantly in the light direction.
    /// In order to have visible volumetric fog light shafts from the side, the distribution will need to be closer to 0. Range: -0.9-0.9.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(310), DefaultValue(0.2f), Limit(-0.9f, 0.9f, 0.001f), EditorDisplay(\"Volumetric Fog\", \"Scattering Distribution\")")
    float VolumetricFogScatteringDistribution = 0.2f;

    /// <summary>
    /// The height fog particle reflectiveness used by volumetric fog.
    /// Water particles in air have an albedo near white, while dust has slightly darker value.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(320), DefaultValue(typeof(Color), \"1,1,1,1\"), EditorDisplay(\"Volumetric Fog\", \"Albedo\")")
    Color VolumetricFogAlbedo = Color::White;

    /// <summary>
    /// Light emitted by height fog. This is a density value so more light is emitted the further you are looking through the fog.
    /// In most cases using a Skylight is a better choice, however, it may be useful in certain scenarios.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(330), DefaultValue(typeof(Color), \"0,0,0,1\"), EditorDisplay(\"Volumetric Fog\", \"Emissive\")")
    Color VolumetricFogEmissive = Color::Black;

    /// <summary>
    /// Scales the height fog particle extinction amount used by volumetric fog.
    /// Values larger than 1 cause fog particles everywhere absorb more light. Range: 0.1-10.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(340), DefaultValue(1.0f), Limit(0.1f, 10, 0.1f), EditorDisplay(\"Volumetric Fog\", \"Extinction Scale\")")
    float VolumetricFogExtinctionScale = 1.0f;

    /// <summary>
    /// Distance over which volumetric fog should be computed. Larger values extend the effect into the distance but expose under-sampling artifacts in details.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(350), DefaultValue(6000.0f), Limit(0), EditorDisplay(\"Volumetric Fog\", \"Distance\")")
    float VolumetricFogDistance = 6000.0f;

private:
#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psFog.Release();
    }
#endif

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

    // [IFogRenderer]
    void GetVolumetricFogOptions(VolumetricFogOptions& result) const override;
    void GetExponentialHeightFogData(const RenderView& view, ShaderExponentialHeightFogData& result) const override;
    void DrawFog(GPUContext* context, RenderContext& renderContext, GPUTextureView* output) override;

protected:
    // [Actor]
    void OnEnable() override;
    void OnDisable() override;
    void OnTransformChanged() override;
};
