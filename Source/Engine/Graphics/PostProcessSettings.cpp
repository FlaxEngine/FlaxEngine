// Copyright (c) Wojciech Figat. All rights reserved.

#include "PostProcessSettings.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Serialization/Serialization.h"

#define BLEND_BOOL(name) name = (((int32)other.OverrideFlags & (int32)Override::name) != 0) && isHalf ? other.name : name
#define BLEND_FLOAT(name) if ((((int32)other.OverrideFlags & (int32)Override::name) != 0)) name = Math::Lerp(name, other.name, weight)
#define BLEND_INT(name) BLEND_FLOAT(name)
#define BLEND_VEC3(name) if ((((int32)other.OverrideFlags & (int32)Override::name) != 0)) name = Float3::Lerp(name, other.name, weight)
#define BLEND_COL(name) if ((((int32)other.OverrideFlags & (int32)Override::name) != 0)) name = Color::Lerp(name, other.name, weight)
#define BLEND_ENUM(name) BLEND_BOOL(name)
#define BLEND_PROPERTY(name) BLEND_BOOL(name)

void AmbientOcclusionSettings::BlendWith(AmbientOcclusionSettings& other, float weight)
{
    const bool isHalf = weight >= 0.5f;

    BLEND_BOOL(Enabled);
    BLEND_FLOAT(Intensity);
    BLEND_FLOAT(Power);
    BLEND_FLOAT(Radius);
    BLEND_FLOAT(FadeOutDistance);
    BLEND_FLOAT(FadeDistance);
}

void GlobalIlluminationSettings::BlendWith(GlobalIlluminationSettings& other, float weight)
{
    const bool isHalf = weight >= 0.5f;
    BLEND_BOOL(Mode);
    BLEND_FLOAT(Intensity);
    BLEND_FLOAT(BounceIntensity);
    BLEND_FLOAT(TemporalResponse);
    BLEND_FLOAT(Distance);
    BLEND_COL(FallbackIrradiance);
}

void BloomSettings::BlendWith(BloomSettings& other, float weight)
{
    const bool isHalf = weight >= 0.5f;

    BLEND_BOOL(Enabled);
    BLEND_FLOAT(Intensity);
    BLEND_FLOAT(Threshold);
    BLEND_FLOAT(ThresholdKnee);
    BLEND_FLOAT(Clamp);
    BLEND_FLOAT(BaseMix);
    BLEND_FLOAT(HighMix);
}

void ToneMappingSettings::BlendWith(ToneMappingSettings& other, float weight)
{
    const bool isHalf = weight >= 0.5f;

    BLEND_FLOAT(WhiteTemperature);
    BLEND_FLOAT(WhiteTint);
    BLEND_ENUM(Mode);
}

void ColorGradingSettings::BlendWith(ColorGradingSettings& other, float weight)
{
    const bool isHalf = weight >= 0.5f;

    BLEND_FLOAT(ColorSaturation);
    BLEND_FLOAT(ColorContrast);
    BLEND_FLOAT(ColorGamma);
    BLEND_FLOAT(ColorGain);
    BLEND_FLOAT(ColorOffset);

    BLEND_FLOAT(ColorSaturationShadows);
    BLEND_FLOAT(ColorContrastShadows);
    BLEND_FLOAT(ColorGammaShadows);
    BLEND_FLOAT(ColorGainShadows);
    BLEND_FLOAT(ColorOffsetShadows);

    BLEND_FLOAT(ColorSaturationMidtones);
    BLEND_FLOAT(ColorContrastMidtones);
    BLEND_FLOAT(ColorGammaMidtones);
    BLEND_FLOAT(ColorGainMidtones);
    BLEND_FLOAT(ColorOffsetMidtones);

    BLEND_FLOAT(ColorSaturationHighlights);
    BLEND_FLOAT(ColorContrastHighlights);
    BLEND_FLOAT(ColorGammaHighlights);
    BLEND_FLOAT(ColorGainHighlights);
    BLEND_FLOAT(ColorOffsetHighlights);

    BLEND_FLOAT(ShadowsMax);
    BLEND_FLOAT(HighlightsMin);

    BLEND_PROPERTY(LutTexture);
    BLEND_FLOAT(LutWeight);
}

void EyeAdaptationSettings::BlendWith(EyeAdaptationSettings& other, float weight)
{
    const bool isHalf = weight >= 0.5f;

    BLEND_ENUM(Mode);
    BLEND_FLOAT(SpeedUp);
    BLEND_FLOAT(SpeedDown);
    BLEND_FLOAT(PreExposure);
    BLEND_FLOAT(PostExposure);
    BLEND_FLOAT(MinBrightness);
    BLEND_FLOAT(MaxBrightness);
    BLEND_FLOAT(HistogramLowPercent);
    BLEND_FLOAT(HistogramHighPercent);
}

void CameraArtifactsSettings::BlendWith(CameraArtifactsSettings& other, float weight)
{
    BLEND_FLOAT(VignetteIntensity);
    BLEND_VEC3(VignetteColor);
    BLEND_FLOAT(VignetteShapeFactor);
    BLEND_FLOAT(GrainAmount);
    BLEND_FLOAT(GrainParticleSize);
    BLEND_FLOAT(GrainSpeed);
    BLEND_FLOAT(ChromaticDistortion);
    BLEND_FLOAT(ScreenFadeColor);
}

void LensFlaresSettings::BlendWith(LensFlaresSettings& other, float weight)
{
    const bool isHalf = weight >= 0.5f;

    BLEND_PROPERTY(LensColor);
    BLEND_PROPERTY(LensStar);
    BLEND_PROPERTY(LensDirt);
    BLEND_FLOAT(Intensity);
    BLEND_INT(Ghosts);
    BLEND_FLOAT(HaloWidth);
    BLEND_FLOAT(HaloIntensity);
    BLEND_FLOAT(GhostDispersal);
    BLEND_FLOAT(Distortion);
    BLEND_FLOAT(ThresholdBias);
    BLEND_FLOAT(ThresholdScale);
    BLEND_FLOAT(LensDirtIntensity);
}

void DepthOfFieldSettings::BlendWith(DepthOfFieldSettings& other, float weight)
{
    const bool isHalf = weight >= 0.5f;

    BLEND_PROPERTY(BokehShapeCustom);
    BLEND_BOOL(Enabled);
    BLEND_FLOAT(BlurStrength);
    BLEND_FLOAT(FocalDistance);
    BLEND_FLOAT(FocalRegion);
    BLEND_FLOAT(NearTransitionRange);
    BLEND_FLOAT(FarTransitionRange);
    BLEND_FLOAT(DepthLimit);
    BLEND_BOOL(BokehEnabled);
    BLEND_FLOAT(BokehBrightness);
    BLEND_FLOAT(BokehSize);
    BLEND_ENUM(BokehShape);
    BLEND_FLOAT(BokehBrightnessThreshold);
    BLEND_FLOAT(BokehBlurThreshold);
    BLEND_FLOAT(BokehFalloff);
    BLEND_FLOAT(BokehDepthCutoff);
}

void MotionBlurSettings::BlendWith(MotionBlurSettings& other, float weight)
{
    const bool isHalf = weight >= 0.5f;

    BLEND_BOOL(Enabled);
    BLEND_FLOAT(Scale);
    BLEND_INT(SampleCount);
    BLEND_ENUM(MotionVectorsResolution);
}

void ScreenSpaceReflectionsSettings::BlendWith(ScreenSpaceReflectionsSettings& other, float weight)
{
    const bool isHalf = weight >= 0.5f;

    BLEND_FLOAT(Intensity);
    BLEND_ENUM(TraceMode);
    BLEND_ENUM(DepthResolution);
    BLEND_ENUM(RayTracePassResolution);
    BLEND_FLOAT(BRDFBias);
    BLEND_FLOAT(RoughnessThreshold);
    BLEND_FLOAT(WorldAntiSelfOcclusionBias);
    BLEND_ENUM(ResolvePassResolution);
    BLEND_INT(ResolveSamples);
    BLEND_FLOAT(EdgeFadeFactor);
    BLEND_FLOAT(FadeOutDistance);
    BLEND_FLOAT(FadeDistance);
    BLEND_BOOL(UseColorBufferMips);
    BLEND_BOOL(TemporalEffect);
    BLEND_FLOAT(TemporalScale);
    BLEND_FLOAT(TemporalResponse);
}

void AntiAliasingSettings::BlendWith(AntiAliasingSettings& other, float weight)
{
    const bool isHalf = weight >= 0.5f;

    BLEND_ENUM(Mode);
    BLEND_FLOAT(TAA_JitterSpread);
    BLEND_FLOAT(TAA_Sharpness);
    BLEND_FLOAT(TAA_StationaryBlending);
    BLEND_FLOAT(TAA_MotionBlending);
    BLEND_FLOAT(CAS_SharpeningAmount);
    BLEND_FLOAT(CAS_EdgeSharpening);
    BLEND_FLOAT(CAS_MinEdgeThreshold);
    BLEND_FLOAT(CAS_OverBlurLimit);
}

void PostFxMaterialsSettings::BlendWith(PostFxMaterialsSettings& other, float weight)
{
    const bool isHalf = weight >= 0.5f;

    if (isHalf)
    {
        int32 indexSrc = 0;
        const SoftAssetReference<MaterialBase>* materialsSrc = other.Materials.Get();
        while (Materials.Count() != POST_PROCESS_SETTINGS_MAX_MATERIALS && indexSrc < other.Materials.Count())
        {
            if (!Materials.Contains(materialsSrc[indexSrc].GetID()))
                Materials.Add(materialsSrc[indexSrc]);
            indexSrc++;
        }
    }
}

void PostProcessSettings::BlendWith(PostProcessSettings& other, float weight)
{
    AmbientOcclusion.BlendWith(other.AmbientOcclusion, weight);
    GlobalIllumination.BlendWith(other.GlobalIllumination, weight);
    Bloom.BlendWith(other.Bloom, weight);
    ToneMapping.BlendWith(other.ToneMapping, weight);
    ColorGrading.BlendWith(other.ColorGrading, weight);
    EyeAdaptation.BlendWith(other.EyeAdaptation, weight);
    CameraArtifacts.BlendWith(other.CameraArtifacts, weight);
    LensFlares.BlendWith(other.LensFlares, weight);
    DepthOfField.BlendWith(other.DepthOfField, weight);
    MotionBlur.BlendWith(other.MotionBlur, weight);
    ScreenSpaceReflections.BlendWith(other.ScreenSpaceReflections, weight);
    AntiAliasing.BlendWith(other.AntiAliasing, weight);
    PostFxMaterials.BlendWith(other.PostFxMaterials, weight);
}

bool PostProcessSettings::HasContentLoaded() const
{
    // Helper textures
    if (LensFlares.LensColor && !LensFlares.LensColor->IsLoaded())
        return false;
    if (LensFlares.LensDirt && !LensFlares.LensDirt->IsLoaded())
        return false;
    if (LensFlares.LensStar && !LensFlares.LensStar->IsLoaded())
        return false;
    if (DepthOfField.BokehShapeCustom && !DepthOfField.BokehShapeCustom->IsLoaded())
        return false;

    // PostFx materials
    for (int32 i = 0; i < PostFxMaterials.Materials.Count(); i++)
    {
        const auto material = PostFxMaterials.Materials[i].Get();
        if (material && !material->IsLoaded())
            return false;
    }

    return true;
}

void PostProcessSettings::Serialize(SerializeStream& stream, const void* otherObj)
{
    SERIALIZE_GET_OTHER_OBJ(PostProcessSettings);

    stream.JKEY("AO");
    stream.Object(&AmbientOcclusion, other ? &other->AmbientOcclusion : nullptr);

    stream.JKEY("GI");
    stream.Object(&GlobalIllumination, other ? &other->GlobalIllumination : nullptr);

    stream.JKEY("Bloom");
    stream.Object(&Bloom, other ? &other->Bloom : nullptr);

    stream.JKEY("ToneMapping");
    stream.Object(&ToneMapping, other ? &other->ToneMapping : nullptr);

    stream.JKEY("ColorGrading");
    stream.Object(&ColorGrading, other ? &other->ColorGrading : nullptr);

    stream.JKEY("EyeAdaptation");
    stream.Object(&EyeAdaptation, other ? &other->EyeAdaptation : nullptr);

    stream.JKEY("CameraArtifacts");
    stream.Object(&CameraArtifacts, other ? &other->CameraArtifacts : nullptr);

    stream.JKEY("LensFlares");
    stream.Object(&LensFlares, other ? &other->LensFlares : nullptr);

    stream.JKEY("DepthOfField");
    stream.Object(&DepthOfField, other ? &other->DepthOfField : nullptr);

    stream.JKEY("MotionBlur");
    stream.Object(&MotionBlur, other ? &other->MotionBlur : nullptr);

    stream.JKEY("SSR");
    stream.Object(&ScreenSpaceReflections, other ? &other->ScreenSpaceReflections : nullptr);

    stream.JKEY("AA");
    stream.Object(&AntiAliasing, other ? &other->AntiAliasing : nullptr);

    stream.JKEY("PostFxMaterials");
    stream.Object(&PostFxMaterials, other ? &other->PostFxMaterials : nullptr);
}

void PostProcessSettings::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    AmbientOcclusion.DeserializeIfExists(stream, "AO", modifier);
    GlobalIllumination.DeserializeIfExists(stream, "GI", modifier);
    Bloom.DeserializeIfExists(stream, "Bloom", modifier);
    ToneMapping.DeserializeIfExists(stream, "ToneMapping", modifier);
    ColorGrading.DeserializeIfExists(stream, "ColorGrading", modifier);
    EyeAdaptation.DeserializeIfExists(stream, "EyeAdaptation", modifier);
    CameraArtifacts.DeserializeIfExists(stream, "CameraArtifacts", modifier);
    LensFlares.DeserializeIfExists(stream, "LensFlares", modifier);
    DepthOfField.DeserializeIfExists(stream, "DepthOfField", modifier);
    MotionBlur.DeserializeIfExists(stream, "MotionBlur", modifier);
    ScreenSpaceReflections.DeserializeIfExists(stream, "SSR", modifier);
    AntiAliasing.DeserializeIfExists(stream, "AA", modifier);
    PostFxMaterials.DeserializeIfExists(stream, "PostFxMaterials", modifier);
}
