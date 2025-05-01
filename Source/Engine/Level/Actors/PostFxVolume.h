// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "BoxVolume.h"
#include "Engine/Graphics/PostProcessSettings.h"
#include "Engine/Level/Scene/SceneRendering.h"

/// <summary>
/// A special type of volume that blends custom set of post process settings into the rendering.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Visuals/Lighting & PostFX/Post Fx Volume\"), ActorToolbox(\"Visuals\")")
class FLAXENGINE_API PostFxVolume : public BoxVolume, public IPostFxSettingsProvider
{
    DECLARE_SCENE_OBJECT(PostFxVolume);
private:
    int32 _priority;
    float _blendRadius;
    float _blendWeight;
    bool _isBounded;

public:
    /// <summary>
    /// The ambient occlusion effect settings.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Ambient Occlusion\"), EditorOrder(100)")
    AmbientOcclusionSettings AmbientOcclusion;

    /// <summary>
    /// The Global Illumination effect settings.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Global Illumination\"), EditorOrder(150)")
    GlobalIlluminationSettings GlobalIllumination;

    /// <summary>
    /// The bloom effect settings.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Bloom\"), EditorOrder(200)")
    BloomSettings Bloom;

    /// <summary>
    /// The tone mapping effect settings.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Tone Mapping\"), EditorOrder(300)")
    ToneMappingSettings ToneMapping;

    /// <summary>
    /// The color grading effect settings.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Color Grading\"), EditorOrder(400)")
    ColorGradingSettings ColorGrading;

    /// <summary>
    /// The eye adaptation effect settings.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Eye Adaptation\"), EditorOrder(500)")
    EyeAdaptationSettings EyeAdaptation;

    /// <summary>
    /// The camera artifacts effect settings.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Camera Artifacts\"), EditorOrder(600)")
    CameraArtifactsSettings CameraArtifacts;

    /// <summary>
    /// The lens flares effect settings.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Lens Flares\"), EditorOrder(700)")
    LensFlaresSettings LensFlares;

    /// <summary>
    /// The depth of field effect settings.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Depth Of Field\"), EditorOrder(800)")
    DepthOfFieldSettings DepthOfField;

    /// <summary>
    /// The motion blur effect settings.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Motion Blur\"), EditorOrder(900)")
    MotionBlurSettings MotionBlur;

    /// <summary>
    /// The screen space reflections effect settings.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Screen Space Reflections\"), EditorOrder(1000)")
    ScreenSpaceReflectionsSettings ScreenSpaceReflections;

    /// <summary>
    /// The anti-aliasing effect settings.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Anti Aliasing\"), EditorOrder(1100)")
    AntiAliasingSettings AntiAliasing;

    /// <summary>
    /// The PostFx materials rendering settings.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"PostFx Materials\"), NoAnimate, EditorOrder(1200)")
    PostFxMaterialsSettings PostFxMaterials;

public:
    /// <summary>
    /// Gets the order in which multiple volumes are blended together. The volume with the highest priority takes precedence over all other overlapping volumes.
    /// </summary>
    API_PROPERTY(Attributes="EditorDisplay(\"PostFx Volume\"), EditorOrder(60)")
    FORCE_INLINE int32 GetPriority() const
    {
        return _priority;
    }

    /// <summary>
    /// Sets the order in which multiple volumes are blended together. The volume with the highest priority takes precedence over all other overlapping volumes.
    /// </summary>
    API_PROPERTY() FORCE_INLINE void SetPriority(int32 value)
    {
        _priority = value;
    }

    /// <summary>
    /// Gets the distance inside the volume at which blending with the volume's settings occurs.
    /// </summary>
    API_PROPERTY(Attributes="EditorDisplay(\"PostFx Volume\"), EditorOrder(70)")
    FORCE_INLINE float GetBlendRadius() const
    {
        return _blendRadius;
    }

    /// <summary>
    /// Sets the distance inside the volume at which blending with the volume's settings occurs.
    /// </summary>
    API_PROPERTY() void SetBlendRadius(float value)
    {
        _blendRadius = Math::Clamp(value, 0.0f, 1000.0f);
    }

    /// <summary>
    /// Gets the amount of influence the volume's properties have. 0 is no effect; 1 is full effect.
    /// </summary>
    API_PROPERTY(Attributes="EditorDisplay(\"PostFx Volume\"), EditorOrder(80)")
    FORCE_INLINE float GetBlendWeight() const
    {
        return _blendWeight;
    }

    /// <summary>
    /// Sets the amount of influence the volume's properties have. 0 is no effect; 1 is full effect.
    /// </summary>
    API_PROPERTY() void SetBlendWeight(float value)
    {
        _blendWeight = Math::Saturate(value);
    }

    /// <summary>
    /// Gets the value indicating whether the bounds of the volume are taken into account. If false, the volume affects the entire world, regardless of its bounds. If true, the volume only has an effect within its bounds.
    /// </summary>
    API_PROPERTY(Attributes="EditorDisplay(\"PostFx Volume\"), EditorOrder(90)")
    FORCE_INLINE bool GetIsBounded() const
    {
        return _isBounded;
    }

    /// <summary>
    /// Sets the value indicating whether the bounds of the volume are taken into account. If false, the volume affects the entire world, regardless of its bounds. If true, the volume only has an effect within its bounds.
    /// </summary>
    API_PROPERTY() FORCE_INLINE void SetIsBounded(bool value)
    {
        _isBounded = value;
    }

public:
    /// <summary>
    /// Adds the post fx material to the settings.
    /// </summary>
    /// <param name="material">The material.</param>
    API_FUNCTION() void AddPostFxMaterial(MaterialBase* material);

    /// <summary>
    /// Removes the post fx material from the settings.
    /// </summary>
    /// <param name="material">The material.</param>
    API_FUNCTION() void RemovePostFxMaterial(MaterialBase* material);

public:
    // [BoxVolume]
    bool HasContentLoaded() const override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;

    // [IPostFxSettingsProvider]
    void Collect(RenderContext& renderContext) override;
    void Blend(PostProcessSettings& other, float weight) override;

protected:
    // [BoxVolume]
    void OnEnable() override;
    void OnDisable() override;
#if USE_EDITOR
    Color GetWiresColor() override;
#endif
};
