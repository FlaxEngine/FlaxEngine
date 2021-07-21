// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Serialization/ISerializable.h"
#include "Engine/Content/Assets/Texture.h"
#include "Engine/Content/Assets/MaterialBase.h"

/// <summary>
/// Tone mapping effect rendering modes.
/// </summary>
API_ENUM() enum class ToneMappingMode
{
    /// <summary>
    /// Disabled tone mapping effect.
    /// </summary>
    None = 0,

    /// <summary>
    /// The neutral tonemapper.
    /// </summary>
    Neutral = 1,

    /// <summary>
    /// The ACES Filmic reference tonemapper (approximation).
    /// </summary>
    ACES = 2,
};

/// <summary>
/// Eye adaptation effect rendering modes.
/// </summary>
API_ENUM() enum class EyeAdaptationMode
{
    /// <summary>
    /// Disabled eye adaptation effect.
    /// </summary>
    None = 0,

    /// <summary>
    /// The manual mode that uses a fixed exposure values.
    /// </summary>
    Manual = 1,

    /// <summary>
    /// The automatic mode applies the eye adaptation exposure based on the scene color luminance blending using the histogram. Requires compute shader support.
    /// </summary>
    AutomaticHistogram = 2,

    /// <summary>
    /// The automatic mode applies the eye adaptation exposure based on the scene color luminance blending using the average luminance.
    /// </summary>
    AutomaticAverageLuminance = 3,
};

/// <summary>
/// Depth of field bokeh shape types.
/// </summary>
API_ENUM() enum class BokehShapeType
{
    /// <summary>
    /// The hexagon shape.
    /// </summary>
    Hexagon = 0,

    /// <summary>
    /// The octagon shape.
    /// </summary>
    Octagon = 1,

    /// <summary>
    /// The circle shape.
    /// </summary>
    Circle = 2,

    /// <summary>
    /// The cross shape.
    /// </summary>
    Cross = 3,

    /// <summary>
    /// The custom texture shape.
    /// </summary>
    Custom = 4,
};

/// <summary>
/// Anti-aliasing modes.
/// </summary>
API_ENUM() enum class AntialiasingMode
{
    /// <summary>
    /// The none.
    /// </summary>
    None = 0,

    /// <summary>
    /// Fast-Approximate Anti-Aliasing effect.
    /// </summary>
    FastApproximateAntialiasing = 1,

    /// <summary>
    /// Temporal Anti-Aliasing effect.
    /// </summary>
    TemporalAntialiasing = 2,

    /// <summary>
    /// Subpixel Morphological Anti-Aliasing effect.
    /// </summary>
    SubpixelMorphologicalAntialiasing = 3,
};

// The maximum amount of postFx materials supported by a single postFx settings container.
#define POST_PROCESS_SETTINGS_MAX_MATERIALS 8

/// <summary>
/// The effect pass resolution.
/// </summary>
API_ENUM() enum class ResolutionMode : int32
{
    /// <summary>
    /// Full resolution
    /// </summary>
    Full = 1,

    /// <summary>
    /// Half resolution
    /// </summary>
    Half = 2,
};

/// <summary>
/// The <see cref="AmbientOcclusionSettings"/> structure members override flags.
/// </summary>
API_ENUM(Attributes="Flags") enum class AmbientOcclusionSettingsOverride : int32
{
    /// <summary>
    /// None properties.
    /// </summary>
    None = 0,

    /// <summary>
    /// Overrides <see cref="AmbientOcclusionSettings.Enabled"/> property.
    /// </summary>
    Enabled = 1 << 0,

    /// <summary>
    /// Overrides <see cref="AmbientOcclusionSettings.Intensity"/> property.
    /// </summary>
    Intensity = 1 << 1,

    /// <summary>
    /// Overrides <see cref="AmbientOcclusionSettings.Power"/> property.
    /// </summary>
    Power = 1 << 2,

    /// <summary>
    /// Overrides <see cref="AmbientOcclusionSettings.Radius"/> property.
    /// </summary>
    Radius = 1 << 3,

    /// <summary>
    /// Overrides <see cref="AmbientOcclusionSettings.FadeOutDistance"/> property.
    /// </summary>
    FadeOutDistance = 1 << 4,

    /// <summary>
    /// Overrides <see cref="AmbientOcclusionSettings.FadeDistance"/> property.
    /// </summary>
    FadeDistance = 1 << 5,

    /// <summary>
    /// All properties.
    /// </summary>
    All = Enabled | Intensity | Power | Radius | FadeOutDistance | FadeDistance,
};

/// <summary>
/// Contains settings for Ambient Occlusion effect rendering.
/// </summary>
API_STRUCT() struct FLAXENGINE_API AmbientOcclusionSettings : ISerializable
{
API_AUTO_SERIALIZATION();
DECLARE_SCRIPTING_TYPE_NO_SPAWN(AmbientOcclusionSettings);
    typedef AmbientOcclusionSettingsOverride Override;

    /// <summary>
    /// The flags for overriden properties.
    /// </summary>
    API_FIELD(Attributes="HideInEditor")
    AmbientOcclusionSettingsOverride OverrideFlags = Override::None;

    /// <summary>
    /// Enable/disable ambient occlusion effect.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(true), EditorOrder(0), PostProcessSetting((int)AmbientOcclusionSettingsOverride.Enabled)")
    bool Enabled = true;

    /// <summary>
    /// Ambient occlusion intensity.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.8f), Limit(0, 5.0f, 0.01f), EditorOrder(1), PostProcessSetting((int)AmbientOcclusionSettingsOverride.Intensity)")
    float Intensity = 0.8f;

    /// <summary>
    /// Ambient occlusion power.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.75f), Limit(0, 4.0f, 0.01f), EditorOrder(2), PostProcessSetting((int)AmbientOcclusionSettingsOverride.Power)")
    float Power = 0.75f;

    /// <summary>
    /// Ambient occlusion check range radius.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.7f), Limit(0, 100.0f, 0.01f), EditorOrder(3), PostProcessSetting((int)AmbientOcclusionSettingsOverride.Radius)")
    float Radius = 0.7f;

    /// <summary>
    /// Ambient occlusion fade out end distance from camera (in world units).
    /// </summary>
    API_FIELD(Attributes="DefaultValue(5000.0f), Limit(0.0f), EditorOrder(4), PostProcessSetting((int)AmbientOcclusionSettingsOverride.FadeOutDistance)")
    float FadeOutDistance = 5000.0f;

    /// <summary>
    /// Ambient occlusion fade distance (in world units). Defines the size of the effect fade from fully visible to fully invisible at FadeOutDistance.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(500.0f), Limit(0.0f), EditorOrder(5), PostProcessSetting((int)AmbientOcclusionSettingsOverride.FadeDistance)")
    float FadeDistance = 500.0f;

public:

    /// <summary>
    /// Blends the settings using given weight.
    /// </summary>
    /// <param name="other">The other settings.</param>
    /// <param name="weight">The blend weight.</param>
    void BlendWith(AmbientOcclusionSettings& other, float weight);
};

/// <summary>
/// The structure members override flags.
/// </summary>
API_ENUM(Attributes="Flags") enum class BloomSettingsOverride : int32
{
    /// <summary>
    /// None properties.
    /// </summary>
    None = 0,

    /// <summary>
    /// Overrides <see cref="BloomSettings.Enabled"/> property.
    /// </summary>
    Enabled = 1 << 0,

    /// <summary>
    /// Overrides <see cref="BloomSettings.Intensity"/> property.
    /// </summary>
    Intensity = 1 << 1,

    /// <summary>
    /// Overrides <see cref="BloomSettings.Threshold"/> property.
    /// </summary>
    Threshold = 1 << 2,

    /// <summary>
    /// Overrides <see cref="BloomSettings.BlurSigma"/> property.
    /// </summary>
    BlurSigma = 1 << 3,

    /// <summary>
    /// Overrides <see cref="BloomSettings.Limit"/> property.
    /// </summary>
    Limit = 1 << 4,

    /// <summary>
    /// All properties.
    /// </summary>
    All = Enabled | Intensity | Threshold | BlurSigma | Limit,
};

/// <summary>
/// Contains settings for Bloom effect rendering.
/// </summary>
API_STRUCT() struct FLAXENGINE_API BloomSettings : ISerializable
{
API_AUTO_SERIALIZATION();
DECLARE_SCRIPTING_TYPE_NO_SPAWN(BloomSettings);
    typedef BloomSettingsOverride Override;

    /// <summary>
    /// The flags for overriden properties.
    /// </summary>
    API_FIELD(Attributes="HideInEditor")
    BloomSettingsOverride OverrideFlags = Override::None;

    /// <summary>
    /// If checked, bloom effect will be rendered.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(true), EditorOrder(0), PostProcessSetting((int)BloomSettingsOverride.Enabled)")
    bool Enabled = true;

    /// <summary>
    /// Bloom effect strength. Value 0 disabled is, while higher values increase the effect.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(1.0f), Limit(0, 20.0f, 0.01f), EditorOrder(1), PostProcessSetting((int)BloomSettingsOverride.Intensity)")
    float Intensity = 1.0f;

    /// <summary>
    /// Minimum pixel brightness value to start blowing. Values below the threshold are skipped.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(3.0f), Limit(0, 15.0f, 0.01f), EditorOrder(2), PostProcessSetting((int)BloomSettingsOverride.Threshold)")
    float Threshold = 3.0f;

    /// <summary>
    /// This affects the fall-off of the bloom. It's the standard deviation (sigma) used in the Gaussian blur formula when calculating the kernel of the bloom.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(4.0f), Limit(0, 20.0f, 0.01f), EditorOrder(3), PostProcessSetting((int)BloomSettingsOverride.BlurSigma)")
    float BlurSigma = 4.0f;

    /// <summary>
    /// Bloom effect brightness limit. Pixels with higher luminance will be capped to this brightness level.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(10.0f), Limit(0, 100.0f, 0.01f), EditorOrder(4), PostProcessSetting((int)BloomSettingsOverride.Limit)")
    float Limit = 10.0f;

public:

    /// <summary>
    /// Blends the settings using given weight.
    /// </summary>
    /// <param name="other">The other settings.</param>
    /// <param name="weight">The blend weight.</param>
    void BlendWith(BloomSettings& other, float weight);
};

/// <summary>
/// The structure members override flags.
/// </summary>
API_ENUM(Attributes="Flags") enum class ToneMappingSettingsOverride : int32
{
    /// <summary>
    /// None properties.
    /// </summary>
    None = 0,

    /// <summary>
    /// Overrides <see cref="ToneMappingSettings.WhiteTemperature"/> property.
    /// </summary>
    WhiteTemperature = 1 << 0,

    /// <summary>
    /// Overrides <see cref="ToneMappingSettings.WhiteTint"/> property.
    /// </summary>
    WhiteTint = 1 << 1,

    /// <summary>
    /// Overrides <see cref="ToneMappingSettings.Mode"/> property.
    /// </summary>
    Mode = 1 << 2,

    /// <summary>
    /// All properties.
    /// </summary>
    All = WhiteTemperature | WhiteTint | Mode,
};

/// <summary>
/// Contains settings for Tone Mapping effect rendering.
/// </summary>
API_STRUCT() struct FLAXENGINE_API ToneMappingSettings : ISerializable
{
API_AUTO_SERIALIZATION();
DECLARE_SCRIPTING_TYPE_NO_SPAWN(ToneMappingSettings);
    typedef ToneMappingSettingsOverride Override;

    /// <summary>
    /// The flags for overriden properties.
    /// </summary>
    API_FIELD(Attributes="HideInEditor")
    ToneMappingSettingsOverride OverrideFlags = Override::None;

    /// <summary>
    /// Adjusts the white balance in relation to the temperature of the light in the scene. When the light temperature and this one match the light will appear white. When a value is used that is higher than the light in the scene it will yield a "warm" or yellow color, and, conversely, if the value is lower, it would yield a "cool" or blue color. The default value is `6500`.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(6500.0f), Limit(1500, 15000), EditorOrder(0), PostProcessSetting((int)ToneMappingSettingsOverride.WhiteTemperature)")
    float WhiteTemperature = 6500.0f;

    /// <summary>
    /// Adjusts the white balance temperature tint for the scene by adjusting the cyan and magenta color ranges. Ideally, this setting should be used once you've adjusted the white balance temperature to get accurate colors. Under some light temperatures, the colors may appear to be more yellow or blue. This can be used to balance the resulting color to look more natural. The default value is `0`.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.0f), Limit(-1, 1, 0.001f), EditorOrder(1), PostProcessSetting((int)ToneMappingSettingsOverride.WhiteTint)")
    float WhiteTint = 0.0f;

    /// <summary>
    /// The tone mapping mode to use for the color grading process.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(ToneMappingMode.ACES), EditorOrder(2), PostProcessSetting((int)ToneMappingSettingsOverride.Mode)")
    ToneMappingMode Mode = ToneMappingMode::ACES;

public:

    /// <summary>
    /// Blends the settings using given weight.
    /// </summary>
    /// <param name="other">The other settings.</param>
    /// <param name="weight">The blend weight.</param>
    void BlendWith(ToneMappingSettings& other, float weight);
};

/// <summary>
/// The structure members override flags.
/// </summary>
API_ENUM(Attributes="Flags") enum class ColorGradingSettingsOverride : int32
{
    /// <summary>
    /// None properties.
    /// </summary>
    None = 0,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ColorSaturation"/> property.
    /// </summary>
    ColorSaturation = 1 << 0,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ColorContrast"/> property.
    /// </summary>
    ColorContrast = 1 << 1,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ColorGamma"/> property.
    /// </summary>
    ColorGamma = 1 << 2,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ColorGain"/> property.
    /// </summary>
    ColorGain = 1 << 3,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ColorOffset"/> property.
    /// </summary>
    ColorOffset = 1 << 4,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ColorSaturationShadows"/> property.
    /// </summary>
    ColorSaturationShadows = 1 << 5,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ColorContrastShadows"/> property.
    /// </summary>
    ColorContrastShadows = 1 << 6,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ColorGammaShadows"/> property.
    /// </summary>
    ColorGammaShadows = 1 << 7,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ColorGainShadows"/> property.
    /// </summary>
    ColorGainShadows = 1 << 8,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ColorOffsetShadows"/> property.
    /// </summary>
    ColorOffsetShadows = 1 << 9,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ColorSaturationMidtones"/> property.
    /// </summary>
    ColorSaturationMidtones = 1 << 10,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ColorContrastMidtones"/> property.
    /// </summary>
    ColorContrastMidtones = 1 << 11,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ColorGammaMidtones"/> property.
    /// </summary>
    ColorGammaMidtones = 1 << 12,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ColorGainMidtones"/> property.
    /// </summary>
    ColorGainMidtones = 1 << 13,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ColorOffsetMidtones"/> property.
    /// </summary>
    ColorOffsetMidtones = 1 << 14,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ColorSaturationHighlights"/> property.
    /// </summary>
    ColorSaturationHighlights = 1 << 15,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ColorContrastHighlights"/> property.
    /// </summary>
    ColorContrastHighlights = 1 << 16,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ColorGammaHighlights"/> property.
    /// </summary>
    ColorGammaHighlights = 1 << 17,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ColorGainHighlights"/> property.
    /// </summary>
    ColorGainHighlights = 1 << 18,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ColorOffsetHighlights"/> property.
    /// </summary>
    ColorOffsetHighlights = 1 << 19,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.ShadowsMax"/> property.
    /// </summary>
    ShadowsMax = 1 << 20,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.HighlightsMin"/> property.
    /// </summary>
    HighlightsMin = 1 << 21,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.LutTexture"/> property.
    /// </summary>
    LutTexture = 1 << 22,

    /// <summary>
    /// Overrides <see cref="ColorGradingSettings.LutWeight"/> property.
    /// </summary>
    LutWeight = 1 << 23,

    /// <summary>
    /// All properties.
    /// </summary>
    All = ColorSaturation | ColorContrast | ColorGamma | ColorGain | ColorOffset | ColorSaturationShadows | ColorContrastShadows | ColorGammaShadows | ColorGainShadows | ColorOffsetShadows | ColorSaturationMidtones | ColorContrastMidtones | ColorGammaMidtones | ColorGainMidtones | ColorOffsetMidtones | ColorSaturationHighlights | ColorContrastHighlights | ColorGammaHighlights | ColorGainHighlights | ColorOffsetHighlights | ShadowsMax | HighlightsMin | LutTexture | LutWeight,
};

/// <summary>
/// Contains settings for Color Grading effect rendering.
/// </summary>
API_STRUCT() struct FLAXENGINE_API ColorGradingSettings : ISerializable
{
API_AUTO_SERIALIZATION();
DECLARE_SCRIPTING_TYPE_NO_SPAWN(ColorGradingSettings);
    typedef ColorGradingSettingsOverride Override;

    /// <summary>
    /// The flags for overriden properties.
    /// </summary>
    API_FIELD(Attributes="HideInEditor")
    ColorGradingSettingsOverride OverrideFlags = Override::None;

    // Global

    /// <summary>
    /// Gets or sets the color saturation (applies globally to the whole image). Default is 1.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Vector4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(0), PostProcessSetting((int)ColorGradingSettingsOverride.ColorSaturation), Limit(0, 2, 0.01f), EditorDisplay(\"Global\", \"Saturation\")")
    Vector4 ColorSaturation = Vector4::One;

    /// <summary>
    /// Gets or sets the color contrast (applies globally to the whole image). Default is 1.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Vector4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(1), PostProcessSetting((int)ColorGradingSettingsOverride.ColorContrast), Limit(0, 2, 0.01f), EditorDisplay(\"Global\", \"Contrast\")")
    Vector4 ColorContrast = Vector4::One;

    /// <summary>
    /// Gets or sets the color gamma (applies globally to the whole image). Default is 1.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Vector4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(2), PostProcessSetting((int)ColorGradingSettingsOverride.ColorGamma), Limit(0, 2, 0.01f), EditorDisplay(\"Global\", \"Gamma\")")
    Vector4 ColorGamma = Vector4::One;

    /// <summary>
    /// Gets or sets the color gain (applies globally to the whole image). Default is 1.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Vector4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(3), PostProcessSetting((int)ColorGradingSettingsOverride.ColorGain), Limit(0, 2, 0.01f), EditorDisplay(\"Global\", \"Gain\")")
    Vector4 ColorGain = Vector4::One;

    /// <summary>
    /// Gets or sets the color offset (applies globally to the whole image). Default is 0.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Vector4), \"0,0,0,0\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(4), PostProcessSetting((int)ColorGradingSettingsOverride.ColorOffset), Limit(-1, 1, 0.001f), EditorDisplay(\"Global\", \"Offset\")")
    Vector4 ColorOffset = Vector4::Zero;

    // Shadows

    /// <summary>
    /// Gets or sets the color saturation (applies to shadows only). Default is 1.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Vector4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(5), PostProcessSetting((int)ColorGradingSettingsOverride.ColorSaturationShadows), Limit(0, 2, 0.01f), EditorDisplay(\"Shadows\", \"Shadows Saturation\")")
    Vector4 ColorSaturationShadows = Vector4::One;

    /// <summary>
    /// Gets or sets the color contrast (applies to shadows only). Default is 1.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Vector4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(6), PostProcessSetting((int)ColorGradingSettingsOverride.ColorContrastShadows), Limit(0, 2, 0.01f), EditorDisplay(\"Shadows\", \"Shadows Contrast\")")
    Vector4 ColorContrastShadows = Vector4::One;

    /// <summary>
    /// Gets or sets the color gamma (applies to shadows only). Default is 1.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Vector4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(7), PostProcessSetting((int)ColorGradingSettingsOverride.ColorGammaShadows), Limit(0, 2, 0.01f), EditorDisplay(\"Shadows\", \"Shadows Gamma\")")
    Vector4 ColorGammaShadows = Vector4::One;

    /// <summary>
    /// Gets or sets the color gain (applies to shadows only). Default is 1.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Vector4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(8), PostProcessSetting((int)ColorGradingSettingsOverride.ColorGainShadows), Limit(0, 2, 0.01f), EditorDisplay(\"Shadows\", \"Shadows Gain\")")
    Vector4 ColorGainShadows = Vector4::One;

    /// <summary>
    /// Gets or sets the color offset (applies to shadows only). Default is 0.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Vector4), \"0,0,0,0\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(9), PostProcessSetting((int)ColorGradingSettingsOverride.ColorOffsetShadows), Limit(-1, 1, 0.001f), EditorDisplay(\"Shadows\", \"Shadows Offset\")")
    Vector4 ColorOffsetShadows = Vector4::Zero;

    // Midtones

    /// <summary>
    /// Gets or sets the color saturation (applies to midtones only). Default is 1.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Vector4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(10), PostProcessSetting((int)ColorGradingSettingsOverride.ColorSaturationMidtones), Limit(0, 2, 0.01f), EditorDisplay(\"Midtones\", \"Midtones Saturation\")")
    Vector4 ColorSaturationMidtones = Vector4::One;

    /// <summary>
    /// Gets or sets the color contrast (applies to midtones only). Default is 1.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Vector4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(11), PostProcessSetting((int)ColorGradingSettingsOverride.ColorContrastMidtones), Limit(0, 2, 0.01f), EditorDisplay(\"Midtones\", \"Midtones Contrast\")")
    Vector4 ColorContrastMidtones = Vector4::One;

    /// <summary>
    /// Gets or sets the color gamma (applies to midtones only). Default is 1.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Vector4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(12), PostProcessSetting((int)ColorGradingSettingsOverride.ColorGammaMidtones), Limit(0, 2, 0.01f), EditorDisplay(\"Midtones\", \"Midtones Gamma\")")
    Vector4 ColorGammaMidtones = Vector4::One;

    /// <summary>
    /// Gets or sets the color gain (applies to midtones only). Default is 1.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Vector4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(13), PostProcessSetting((int)ColorGradingSettingsOverride.ColorGainMidtones), Limit(0, 2, 0.01f), EditorDisplay(\"Midtones\", \"Midtones Gain\")")
    Vector4 ColorGainMidtones = Vector4::One;

    /// <summary>
    /// Gets or sets the color offset (applies to midtones only). Default is 0.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Vector4), \"0,0,0,0\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(14), PostProcessSetting((int)ColorGradingSettingsOverride.ColorOffsetMidtones), Limit(-1, 1, 0.001f), EditorDisplay(\"Midtones\", \"Midtones Offset\")")
    Vector4 ColorOffsetMidtones = Vector4::Zero;

    // Highlights

    /// <summary>
    /// Gets or sets the color saturation (applies to highlights only). Default is 1.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Vector4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(15), PostProcessSetting((int)ColorGradingSettingsOverride.ColorSaturationHighlights), Limit(0, 2, 0.01f), EditorDisplay(\"Highlights\", \"Highlights Saturation\")")
    Vector4 ColorSaturationHighlights = Vector4::One;

    /// <summary>
    /// Gets or sets the color contrast (applies to highlights only). Default is 1.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Vector4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(16), PostProcessSetting((int)ColorGradingSettingsOverride.ColorContrastHighlights), Limit(0, 2, 0.01f), EditorDisplay(\"Highlights\", \"Highlights Contrast\")")
    Vector4 ColorContrastHighlights = Vector4::One;

    /// <summary>
    /// Gets or sets the color gamma (applies to highlights only). Default is 1.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Vector4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(17), PostProcessSetting((int)ColorGradingSettingsOverride.ColorGammaHighlights), Limit(0, 2, 0.01f), EditorDisplay(\"Highlights\", \"Highlights Gamma\")")
    Vector4 ColorGammaHighlights = Vector4::One;

    /// <summary>
    /// Gets or sets the color gain (applies to highlights only). Default is 1.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Vector4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(18), PostProcessSetting((int)ColorGradingSettingsOverride.ColorGainHighlights), Limit(0, 2, 0.01f), EditorDisplay(\"Highlights\", \"Highlights Gain\")")
    Vector4 ColorGainHighlights = Vector4::One;

    /// <summary>
    /// Gets or sets the color offset (applies to highlights only). Default is 0.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Vector4), \"0,0,0,0\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(19), PostProcessSetting((int)ColorGradingSettingsOverride.ColorOffsetHighlights), Limit(-1, 1, 0.001f), EditorDisplay(\"Highlights\", \"Highlights Offset\")")
    Vector4 ColorOffsetHighlights = Vector4::Zero;

    //

    /// <summary>
    /// The shadows maximum value. Default is 0.09.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.09f), Limit(-1, 1, 0.01f), EditorOrder(20), PostProcessSetting((int)ColorGradingSettingsOverride.ShadowsMax)")
    float ShadowsMax = 0.09f;

    /// <summary>
    /// The highlights minimum value. Default is 0.5.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.5f), Limit(-1, 1, 0.01f), EditorOrder(21), PostProcessSetting((int)ColorGradingSettingsOverride.HighlightsMin)")
    float HighlightsMin = 0.5f;

    //

    /// <summary>
    /// The Lookup Table (LUT) used to perform color correction.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(null), EditorOrder(22), PostProcessSetting((int)ColorGradingSettingsOverride.LutTexture)")
    AssetReference<Texture> LutTexture;

    /// <summary>
    /// The LUT blending weight (normalized to range 0-1). Default is 1.0.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(1.0f), Limit(0, 1, 0.01f), EditorOrder(23), PostProcessSetting((int)ColorGradingSettingsOverride.LutWeight)")
    float LutWeight = 1.0f;

public:

    /// <summary>
    /// Blends the settings using given weight.
    /// </summary>
    /// <param name="other">The other settings.</param>
    /// <param name="weight">The blend weight.</param>
    void BlendWith(ColorGradingSettings& other, float weight);
};

/// <summary>
/// The structure members override flags.
/// </summary>
API_ENUM(Attributes="Flags") enum class EyeAdaptationSettingsOverride : int32
{
    /// <summary>
    /// None properties.
    /// </summary>
    None = 0,

    /// <summary>
    /// Overrides <see cref="EyeAdaptationSettings.Mode"/> property.
    /// </summary>
    Mode = 1 << 0,

    /// <summary>
    /// Overrides <see cref="EyeAdaptationSettings.SpeedUp"/> property.
    /// </summary>
    SpeedUp = 1 << 1,

    /// <summary>
    /// Overrides <see cref="EyeAdaptationSettings.SpeedDown"/> property.
    /// </summary>
    SpeedDown = 1 << 2,

    /// <summary>
    /// Overrides <see cref="EyeAdaptationSettings.PreExposure"/> property.
    /// </summary>
    PreExposure = 1 << 3,

    /// <summary>
    /// Overrides <see cref="EyeAdaptationSettings.PostExposure"/> property.
    /// </summary>
    PostExposure = 1 << 4,

    /// <summary>
    /// Overrides <see cref="EyeAdaptationSettings.MinBrightness"/> property.
    /// </summary>
    MinBrightness = 1 << 5,

    /// <summary>
    /// Overrides <see cref="EyeAdaptationSettings.MaxBrightness"/> property.
    /// </summary>
    MaxBrightness = 1 << 6,

    /// <summary>
    /// Overrides <see cref="EyeAdaptationSettings.HistogramLowPercent"/> property.
    /// </summary>
    HistogramLowPercent = 1 << 7,

    /// <summary>
    /// Overrides <see cref="EyeAdaptationSettings.HistogramHighPercent"/> property.
    /// </summary>
    HistogramHighPercent = 1 << 8,

    /// <summary>
    /// All properties.
    /// </summary>
    All = Mode | SpeedUp | SpeedDown | PreExposure | PostExposure | MinBrightness | MaxBrightness | HistogramLowPercent | HistogramHighPercent,
};

/// <summary>
/// Contains settings for Eye Adaptation effect rendering.
/// </summary>
API_STRUCT() struct FLAXENGINE_API EyeAdaptationSettings : ISerializable
{
API_AUTO_SERIALIZATION();
DECLARE_SCRIPTING_TYPE_NO_SPAWN(EyeAdaptationSettings);
    typedef EyeAdaptationSettingsOverride Override;

    /// <summary>
    /// The flags for overriden properties.
    /// </summary>
    API_FIELD(Attributes="HideInEditor")
    EyeAdaptationSettingsOverride OverrideFlags = Override::None;

    /// <summary>
    /// The effect rendering mode used for the exposure processing.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(EyeAdaptationMode.AutomaticHistogram), EditorOrder(0), PostProcessSetting((int)EyeAdaptationSettingsOverride.Mode)")
    EyeAdaptationMode Mode = EyeAdaptationMode::AutomaticHistogram;

    /// <summary>
    /// The speed at which the exposure changes when the scene brightness moves from a dark area to a bright area (brightness goes up).
    /// </summary>
    API_FIELD(Attributes="DefaultValue(3.0f), Limit(0, 100.0f, 0.01f), EditorOrder(1), PostProcessSetting((int)EyeAdaptationSettingsOverride.SpeedUp)")
    float SpeedUp = 3.0f;

    /// <summary>
    /// The speed at which the exposure changes when the scene brightness moves from a bright area to a dark area (brightness goes down).
    /// </summary>
    API_FIELD(Attributes="DefaultValue(1.0f), Limit(0, 100.0f, 0.01f), EditorOrder(2), PostProcessSetting((int)EyeAdaptationSettingsOverride.SpeedDown)")
    float SpeedDown = 1.0f;

    /// <summary>
    /// The pre-exposure value applied to the scene color before performing post-processing (such as bloom, lens flares, etc.).
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.0f), Limit(-100, 100, 0.01f), EditorOrder(3), PostProcessSetting((int)EyeAdaptationSettingsOverride.PreExposure)")
    float PreExposure = 0.0f;

    /// <summary>
    /// The post-exposure value applied to the scene color after performing post-processing (such as bloom, lens flares, etc.) but before color grading and tone mapping.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.0f), Limit(-100, 100, 0.01f), EditorOrder(3), PostProcessSetting((int)EyeAdaptationSettingsOverride.PostExposure)")
    float PostExposure = 0.0f;

    /// <summary>
    /// The minimum brightness for the auto exposure which limits the lower brightness the eye can adapt within.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.03f), Limit(0, 20.0f, 0.01f), EditorOrder(5), PostProcessSetting((int)EyeAdaptationSettingsOverride.MinBrightness), EditorDisplay(null, \"Minimum Brightness\")")
    float MinBrightness = 0.03f;

    /// <summary>
    /// The maximum brightness for the auto exposure which limits the upper brightness the eye can adapt within.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(2.0f), Limit(0, 100.0f, 0.01f), EditorOrder(6), PostProcessSetting((int)EyeAdaptationSettingsOverride.MaxBrightness), EditorDisplay(null, \"Maximum Brightness\")")
    float MaxBrightness = 2.0f;

    /// <summary>
    /// The lower bound for the luminance histogram of the scene color. Value is in percent and limits the pixels below this brightness. Use values from range 60-80. Used only in AutomaticHistogram mode.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(70.0f), Limit(1, 99, 0.001f), EditorOrder(3), PostProcessSetting((int)EyeAdaptationSettingsOverride.HistogramLowPercent)")
    float HistogramLowPercent = 70.0f;

    /// <summary>
    /// The upper bound for the luminance histogram of the scene color. Value is in percent and limits the pixels above this brightness. Use values from range 80-95. Used only in AutomaticHistogram mode.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(98.0f), Limit(1, 99, 0.001f), EditorOrder(3), PostProcessSetting((int)EyeAdaptationSettingsOverride.HistogramHighPercent)")
    float HistogramHighPercent = 98.0f;

public:

    /// <summary>
    /// Blends the settings using given weight.
    /// </summary>
    /// <param name="other">The other settings.</param>
    /// <param name="weight">The blend weight.</param>
    void BlendWith(EyeAdaptationSettings& other, float weight);
};

/// <summary>
/// The structure members override flags.
/// </summary>
API_ENUM(Attributes="Flags") enum class CameraArtifactsSettingsOverride : int32
{
    /// <summary>
    /// None properties.
    /// </summary>
    None = 0,

    /// <summary>
    /// Overrides <see cref="CameraArtifactsSettings.VignetteIntensity"/> property.
    /// </summary>
    VignetteIntensity = 1 << 0,

    /// <summary>
    /// Overrides <see cref="CameraArtifactsSettings.VignetteColor"/> property.
    /// </summary>
    VignetteColor = 1 << 1,

    /// <summary>
    /// Overrides <see cref="CameraArtifactsSettings.VignetteShapeFactor"/> property.
    /// </summary>
    VignetteShapeFactor = 1 << 2,

    /// <summary>
    /// Overrides <see cref="CameraArtifactsSettings.GrainAmount"/> property.
    /// </summary>
    GrainAmount = 1 << 3,

    /// <summary>
    /// Overrides <see cref="CameraArtifactsSettings.GrainParticleSize"/> property.
    /// </summary>
    GrainParticleSize = 1 << 4,

    /// <summary>
    /// Overrides <see cref="CameraArtifactsSettings.GrainSpeed"/> property.
    /// </summary>
    GrainSpeed = 1 << 5,

    /// <summary>
    /// Overrides <see cref="CameraArtifactsSettings.ChromaticDistortion"/> property.
    /// </summary>
    ChromaticDistortion = 1 << 6,

    /// <summary>
    /// Overrides <see cref="CameraArtifactsSettings.ScreenFadeColor"/> property.
    /// </summary>
    ScreenFadeColor = 1 << 7,

    /// <summary>
    /// All properties.
    /// </summary>
    All = VignetteIntensity | VignetteColor | VignetteShapeFactor | GrainAmount | GrainParticleSize | GrainSpeed | ChromaticDistortion | ScreenFadeColor,
};

/// <summary>
/// Contains settings for Camera Artifacts effect rendering.
/// </summary>
API_STRUCT() struct FLAXENGINE_API CameraArtifactsSettings : ISerializable
{
API_AUTO_SERIALIZATION();
DECLARE_SCRIPTING_TYPE_NO_SPAWN(CameraArtifactsSettings);
    typedef CameraArtifactsSettingsOverride Override;

    /// <summary>
    /// The flags for overriden properties.
    /// </summary>
    API_FIELD(Attributes="HideInEditor")
    CameraArtifactsSettingsOverride OverrideFlags = Override::None;

    /// <summary>
    /// Strength of the vignette effect. Value 0 hides it. The default value is 0.8.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.8f), Limit(0, 2, 0.001f), EditorOrder(0), PostProcessSetting((int)CameraArtifactsSettingsOverride.VignetteIntensity)")
    float VignetteIntensity = 0.8f;

    /// <summary>
    /// Color of the vignette.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1), PostProcessSetting((int)CameraArtifactsSettingsOverride.VignetteColor)")
    Vector3 VignetteColor = Vector3(0, 0, 0.001f);

    /// <summary>
    /// Controls shape of the vignette. Values near 0 produce rectangle shape. Higher values result in round shape. The default value is 0.125.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.125f), Limit(0.0001f, 2.0f, 0.001f), EditorOrder(2), PostProcessSetting((int)CameraArtifactsSettingsOverride.VignetteShapeFactor)")
    float VignetteShapeFactor = 0.125f;

    /// <summary>
    /// Intensity of the grain filter. Value 0 hides it. The default value is 0.005.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.006f), Limit(0.0f, 2.0f, 0.005f), EditorOrder(3), PostProcessSetting((int)CameraArtifactsSettingsOverride.GrainAmount)")
    float GrainAmount = 0.006f;

    /// <summary>
    /// Size of the grain particles. The default value is 1.6.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(1.6f), Limit(1.0f, 3.0f, 0.01f), EditorOrder(4), PostProcessSetting((int)CameraArtifactsSettingsOverride.GrainParticleSize)")
    float GrainParticleSize = 1.6f;

    /// <summary>
    /// Speed of the grain particles animation.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(1.0f), Limit(0.0f, 10.0f, 0.01f), EditorOrder(5), PostProcessSetting((int)CameraArtifactsSettingsOverride.GrainSpeed)")
    float GrainSpeed = 1.0f;

    /// <summary>
    /// Controls chromatic aberration effect strength. Value 0 hides it.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.0f), Limit(0.0f, 1.0f, 0.01f), EditorOrder(6), PostProcessSetting((int)CameraArtifactsSettingsOverride.ChromaticDistortion)")
    float ChromaticDistortion = 0.0f;

    /// <summary>
    /// Screen tint color (alpha channel defines the blending factor).
    /// </summary>
    API_FIELD(Attributes="DefaultValue(typeof(Color), \"0,0,0,0\"), EditorOrder(7), PostProcessSetting((int)CameraArtifactsSettingsOverride.ScreenFadeColor)")
    Color ScreenFadeColor = Color::Transparent;

public:

    /// <summary>
    /// Blends the settings using given weight.
    /// </summary>
    /// <param name="other">The other settings.</param>
    /// <param name="weight">The blend weight.</param>
    void BlendWith(CameraArtifactsSettings& other, float weight);
};

/// <summary>
/// The structure members override flags.
/// </summary>
API_ENUM(Attributes="Flags") enum class LensFlaresSettingsOverride : int32
{
    /// <summary>
    /// None properties.
    /// </summary>
    None = 0,

    /// <summary>
    /// Overrides <see cref="LensFlaresSettings.Intensity"/> property.
    /// </summary>
    Intensity = 1 << 0,

    /// <summary>
    /// Overrides <see cref="LensFlaresSettings.Ghosts"/> property.
    /// </summary>
    Ghosts = 1 << 1,

    /// <summary>
    /// Overrides <see cref="LensFlaresSettings.HaloWidth"/> property.
    /// </summary>
    HaloWidth = 1 << 2,

    /// <summary>
    /// Overrides <see cref="LensFlaresSettings.HaloIntensity"/> property.
    /// </summary>
    HaloIntensity = 1 << 3,

    /// <summary>
    /// Overrides <see cref="LensFlaresSettings.GhostDispersal"/> property.
    /// </summary>
    GhostDispersal = 1 << 4,

    /// <summary>
    /// Overrides <see cref="LensFlaresSettings.Distortion"/> property.
    /// </summary>
    Distortion = 1 << 5,

    /// <summary>
    /// Overrides <see cref="LensFlaresSettings.ThresholdBias"/> property.
    /// </summary>
    ThresholdBias = 1 << 6,

    /// <summary>
    /// Overrides <see cref="LensFlaresSettings.ThresholdScale"/> property.
    /// </summary>
    ThresholdScale = 1 << 7,

    /// <summary>
    /// Overrides <see cref="LensFlaresSettings.LensDirt"/> property.
    /// </summary>
    LensDirt = 1 << 8,

    /// <summary>
    /// Overrides <see cref="LensFlaresSettings.LensDirtIntensity"/> property.
    /// </summary>
    LensDirtIntensity = 1 << 9,

    /// <summary>
    /// Overrides <see cref="LensFlaresSettings.LensColor"/> property.
    /// </summary>
    LensColor = 1 << 10,

    /// <summary>
    /// Overrides <see cref="LensFlaresSettings.LensStar"/> property.
    /// </summary>
    LensStar = 1 << 11,

    /// <summary>
    /// All properties.
    /// </summary>
    All = Intensity | Ghosts | HaloWidth | HaloIntensity | GhostDispersal | Distortion | ThresholdBias | ThresholdScale | LensDirt | LensDirtIntensity | LensColor | LensStar,
};

/// <summary>
/// Contains settings for Lens Flares effect rendering.
/// </summary>
API_STRUCT() struct FLAXENGINE_API LensFlaresSettings : ISerializable
{
API_AUTO_SERIALIZATION();
DECLARE_SCRIPTING_TYPE_NO_SPAWN(LensFlaresSettings);
    typedef LensFlaresSettingsOverride Override;

    /// <summary>
    /// The flags for overriden properties.
    /// </summary>
    API_FIELD(Attributes="HideInEditor")
    LensFlaresSettingsOverride OverrideFlags = Override::None;

    /// <summary>
    /// Strength of the effect. Value 0 disabled it.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(1.0f), Limit(0, 10.0f, 0.01f), EditorOrder(0), PostProcessSetting((int)LensFlaresSettingsOverride.Intensity)")
    float Intensity = 1.0f;

    /// <summary>
    /// Amount of lens flares ghosts.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(8), Limit(0, 16), EditorOrder(1), PostProcessSetting((int)LensFlaresSettingsOverride.Ghosts)")
    int32 Ghosts = 8;

    /// <summary>
    /// Lens flares halo width.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.16f), EditorOrder(2), PostProcessSetting((int)LensFlaresSettingsOverride.HaloWidth)")
    float HaloWidth = 0.16f;

    /// <summary>
    /// Lens flares halo intensity.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.666f), Limit(0, 10.0f, 0.01f), EditorOrder(3), PostProcessSetting((int)LensFlaresSettingsOverride.HaloIntensity)")
    float HaloIntensity = 0.666f;

    /// <summary>
    /// Ghost samples dispersal parameter.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.3f), EditorOrder(4), PostProcessSetting((int)LensFlaresSettingsOverride.GhostDispersal)")
    float GhostDispersal = 0.3f;

    /// <summary>
    /// Lens flares color distortion parameter.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(1.5f), EditorOrder(5), PostProcessSetting((int)LensFlaresSettingsOverride.Distortion)")
    float Distortion = 1.5f;

    /// <summary>
    /// Input image brightness threshold. Added to input pixels.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(-0.5f), EditorOrder(6), PostProcessSetting((int)LensFlaresSettingsOverride.ThresholdBias)")
    float ThresholdBias = -0.5f;

    /// <summary>
    /// Input image brightness threshold scale. Used to multiply input pixels.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.22f), EditorOrder(7), PostProcessSetting((int)LensFlaresSettingsOverride.ThresholdScale)")
    float ThresholdScale = 0.22f;

    /// <summary>
    /// Fullscreen lens dirt texture.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(null), EditorOrder(8), PostProcessSetting((int)LensFlaresSettingsOverride.LensDirt)")
    AssetReference<Texture> LensDirt;

    /// <summary>
    /// Fullscreen lens dirt intensity parameter. Allows to tune dirt visibility.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(1.0f), Limit(0, 100, 0.01f), EditorOrder(9), PostProcessSetting((int)LensFlaresSettingsOverride.LensDirtIntensity)")
    float LensDirtIntensity = 1.0f;

    /// <summary>
    /// Custom lens color texture (1D) used for lens color spectrum.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(null), EditorOrder(10), PostProcessSetting((int)LensFlaresSettingsOverride.LensColor)")
    AssetReference<Texture> LensColor;

    /// <summary>
    /// Custom lens star texture sampled by lens flares.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(null), EditorOrder(11), PostProcessSetting((int)LensFlaresSettingsOverride.LensStar)")
    AssetReference<Texture> LensStar;

public:

    /// <summary>
    /// Blends the settings using given weight.
    /// </summary>
    /// <param name="other">The other settings.</param>
    /// <param name="weight">The blend weight.</param>
    void BlendWith(LensFlaresSettings& other, float weight);
};

/// <summary>
/// The structure members override flags.
/// </summary>
API_ENUM(Attributes="Flags") enum class DepthOfFieldSettingsOverride : int32
{
    /// <summary>
    /// None properties.
    /// </summary>
    None = 0,

    /// <summary>
    /// Overrides <see cref="DepthOfFieldSettings.Enabled"/> property.
    /// </summary>
    Enabled = 1 << 0,

    /// <summary>
    /// Overrides <see cref="DepthOfFieldSettings.BlurStrength"/> property.
    /// </summary>
    BlurStrength = 1 << 1,

    /// <summary>
    /// Overrides <see cref="DepthOfFieldSettings.FocalDistance"/> property.
    /// </summary>
    FocalDistance = 1 << 2,

    /// <summary>
    /// Overrides <see cref="DepthOfFieldSettings.FocalRegion"/> property.
    /// </summary>
    FocalRegion = 1 << 3,

    /// <summary>
    /// Overrides <see cref="DepthOfFieldSettings.NearTransitionRange"/> property.
    /// </summary>
    NearTransitionRange = 1 << 4,

    /// <summary>
    /// Overrides <see cref="DepthOfFieldSettings.FarTransitionRange"/> property.
    /// </summary>
    FarTransitionRange = 1 << 5,

    /// <summary>
    /// Overrides <see cref="DepthOfFieldSettings.DepthLimit"/> property.
    /// </summary>
    DepthLimit = 1 << 6,

    /// <summary>
    /// Overrides <see cref="DepthOfFieldSettings.BokehEnabled"/> property.
    /// </summary>
    BokehEnabled = 1 << 7,

    /// <summary>
    /// Overrides <see cref="DepthOfFieldSettings.BokehSize"/> property.
    /// </summary>
    BokehSize = 1 << 8,

    /// <summary>
    /// Overrides <see cref="DepthOfFieldSettings.BokehShape"/> property.
    /// </summary>
    BokehShape = 1 << 9,

    /// <summary>
    /// Overrides <see cref="DepthOfFieldSettings.BokehShapeCustom"/> property.
    /// </summary>
    BokehShapeCustom = 1 << 10,

    /// <summary>
    /// Overrides <see cref="DepthOfFieldSettings.BokehBrightnessThreshold"/> property.
    /// </summary>
    BokehBrightnessThreshold = 1 << 11,

    /// <summary>
    /// Overrides <see cref="DepthOfFieldSettings.BokehBlurThreshold"/> property.
    /// </summary>
    BokehBlurThreshold = 1 << 12,

    /// <summary>
    /// Overrides <see cref="DepthOfFieldSettings.BokehFalloff"/> property.
    /// </summary>
    BokehFalloff = 1 << 13,

    /// <summary>
    /// Overrides <see cref="DepthOfFieldSettings.BokehDepthCutoff"/> property.
    /// </summary>
    BokehDepthCutoff = 1 << 14,

    /// <summary>
    /// Overrides <see cref="DepthOfFieldSettings.BokehBrightness"/> property.
    /// </summary>
    BokehBrightness = 1 << 15,

    /// <summary>
    /// All properties.
    /// </summary>
    All = Enabled | BlurStrength | FocalDistance | FocalRegion | NearTransitionRange | FarTransitionRange | DepthLimit | BokehEnabled | BokehSize | BokehShape | BokehShapeCustom | BokehBrightnessThreshold | BokehBlurThreshold | BokehFalloff | BokehDepthCutoff | BokehBrightness,
};

/// <summary>
/// Contains settings for Depth Of Field effect rendering.
/// </summary>
API_STRUCT() struct FLAXENGINE_API DepthOfFieldSettings : ISerializable
{
API_AUTO_SERIALIZATION();
DECLARE_SCRIPTING_TYPE_NO_SPAWN(DepthOfFieldSettings);
    typedef DepthOfFieldSettingsOverride Override;

    /// <summary>
    /// The flags for overriden properties.
    /// </summary>
    API_FIELD(Attributes="HideInEditor")
    DepthOfFieldSettingsOverride OverrideFlags = Override::None;

    /// <summary>
    /// If checked, depth of field effect will be visible.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(false), EditorOrder(0), PostProcessSetting((int)DepthOfFieldSettingsOverride.Enabled)")
    bool Enabled = false;

    /// <summary>
    /// The blur intensity in the out-of-focus areas. Allows reducing blur amount by scaling down the Gaussian Blur radius. Normalized to range 0-1.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(1.0f), Limit(0, 1, 0.01f), EditorOrder(1), PostProcessSetting((int)DepthOfFieldSettingsOverride.BlurStrength)")
    float BlurStrength = 1.0f;

    /// <summary>
    /// The distance in World Units from the camera that acts as the center of the region where the scene is perfectly in focus and no blurring occurs.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(1700.0f), Limit(0), EditorOrder(2), PostProcessSetting((int)DepthOfFieldSettingsOverride.FocalDistance)")
    float FocalDistance = 1700.0f;

    /// <summary>
    /// The distance in World Units beyond the focal distance where the scene is perfectly in focus and no blurring occurs.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(3000.0f), Limit(0), EditorOrder(3), PostProcessSetting((int)DepthOfFieldSettingsOverride.FocalRegion)")
    float FocalRegion = 3000.0f;

    /// <summary>
    /// The distance in World Units from the focal region on the side nearer to the camera over which the scene transitions from focused to blurred.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(300.0f), Limit(0), EditorOrder(4), PostProcessSetting((int)DepthOfFieldSettingsOverride.NearTransitionRange)")
    float NearTransitionRange = 300.0f;

    /// <summary>
    /// The distance in World Units from the focal region on the side farther from the camera over which the scene transitions from focused to blurred.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(500.0f), Limit(0), EditorOrder(5), PostProcessSetting((int)DepthOfFieldSettingsOverride.FarTransitionRange)")
    float FarTransitionRange = 500.0f;

    /// <summary>
    /// The distance in World Units which describes border after that there is no blur (useful to disable DoF on sky). Use 0 to disable that feature.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.0f), Limit(0, float.MaxValue, 2), EditorOrder(6), PostProcessSetting((int)DepthOfFieldSettingsOverride.DepthLimit)")
    float DepthLimit = 0.0f;

    /// <summary>
    /// If checked, bokeh shapes will be rendered.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(true), EditorOrder(7), PostProcessSetting((int)DepthOfFieldSettingsOverride.BokehEnabled)")
    bool BokehEnabled = true;

    /// <summary>
    /// Controls size of the bokeh shapes.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(25.0f), Limit(0, 200.0f, 0.1f), EditorOrder(8), PostProcessSetting((int)DepthOfFieldSettingsOverride.BokehSize)")
    float BokehSize = 25.0f;

    /// <summary>
    /// Controls brightness of the bokeh shapes. Can be used to fade them or make more intense.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(1.0f), Limit(0, 10.0f, 0.01f), EditorOrder(9), PostProcessSetting((int)DepthOfFieldSettingsOverride.BokehBrightness)")
    float BokehBrightness = 1.0f;

    /// <summary>
    /// Defines bokeh shapes type.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(BokehShapeType.Octagon), EditorOrder(10), PostProcessSetting((int)DepthOfFieldSettingsOverride.BokehShape)")
    BokehShapeType BokehShape = BokehShapeType::Octagon;

    /// <summary>
    /// If BokehShape is set to Custom, then this texture will be used for the bokeh shapes. For best performance, use small, compressed, grayscale textures (for instance 32px).
    /// </summary>
    API_FIELD(Attributes="DefaultValue(null), EditorOrder(11), PostProcessSetting((int)DepthOfFieldSettingsOverride.BokehShapeCustom)")
    AssetReference<Texture> BokehShapeCustom;

    /// <summary>
    /// The minimum pixel brightness to create bokeh. Pixels with lower brightness will be skipped.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(3.0f), Limit(0, 10000.0f, 0.01f), EditorOrder(12), PostProcessSetting((int)DepthOfFieldSettingsOverride.BokehBrightnessThreshold)")
    float BokehBrightnessThreshold = 3.0f;

    /// <summary>
    /// Depth of Field bokeh shapes blur threshold.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.05f), Limit(0, 1.0f, 0.001f), EditorOrder(13), PostProcessSetting((int)DepthOfFieldSettingsOverride.BokehBlurThreshold)")
    float BokehBlurThreshold = 0.05f;

    /// <summary>
    /// Controls bokeh shapes brightness falloff. Higher values reduce bokeh visibility.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.5f), Limit(0, 2.0f, 0.001f), EditorOrder(14), PostProcessSetting((int)DepthOfFieldSettingsOverride.BokehFalloff)")
    float BokehFalloff = 0.5f;

    /// <summary>
    /// Controls bokeh shape generation for depth discontinuities.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(1.5f), Limit(0, 5.0f, 0.001f), EditorOrder(15), PostProcessSetting((int)DepthOfFieldSettingsOverride.BokehDepthCutoff)")
    float BokehDepthCutoff = 1.5f;

public:

    /// <summary>
    /// Blends the settings using given weight.
    /// </summary>
    /// <param name="other">The other settings.</param>
    /// <param name="weight">The blend weight.</param>
    void BlendWith(DepthOfFieldSettings& other, float weight);
};

/// <summary>
/// The structure members override flags.
/// </summary>
API_ENUM(Attributes="Flags") enum class MotionBlurSettingsOverride : int32
{
    /// <summary>
    /// None properties.
    /// </summary>
    None = 0,

    /// <summary>
    /// Overrides <see cref="MotionBlurSettings.Enabled"/> property.
    /// </summary>
    Enabled = 1 << 0,

    /// <summary>
    /// Overrides <see cref="MotionBlurSettings.Scale"/> property.
    /// </summary>
    Scale = 1 << 1,

    /// <summary>
    /// Overrides <see cref="MotionBlurSettings.SampleCount"/> property.
    /// </summary>
    SampleCount = 1 << 2,

    /// <summary>
    /// Overrides <see cref="MotionBlurSettings.MotionVectorsResolution"/> property.
    /// </summary>
    MotionVectorsResolution = 1 << 3,

    /// <summary>
    /// All properties.
    /// </summary>
    All = Enabled | Scale | SampleCount | MotionVectorsResolution,
};

/// <summary>
/// Contains settings for Motion Blur effect rendering.
/// </summary>
API_STRUCT() struct FLAXENGINE_API MotionBlurSettings : ISerializable
{
API_AUTO_SERIALIZATION();
DECLARE_SCRIPTING_TYPE_NO_SPAWN(MotionBlurSettings);
    typedef MotionBlurSettingsOverride Override;

    /// <summary>
    /// The flags for overriden properties.
    /// </summary>
    API_FIELD(Attributes="HideInEditor")
    MotionBlurSettingsOverride OverrideFlags = Override::None;

    /// <summary>
    /// If checked, motion blur effect will be rendered.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(true), EditorOrder(0), PostProcessSetting((int)MotionBlurSettingsOverride.Enabled)")
    bool Enabled = true;

    /// <summary>
    /// The blur effect strength. Value 0 disabled is, while higher values increase the effect.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(1.0f), Limit(0, 5, 0.01f), EditorOrder(1), PostProcessSetting((int)MotionBlurSettingsOverride.Scale)")
    float Scale = 1.0f;

    /// <summary>
    /// The amount of sample points used during motion blur rendering. It affects quality and performance.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(10), Limit(4, 32, 0.1f), EditorOrder(2), PostProcessSetting((int)MotionBlurSettingsOverride.SampleCount)")
    int32 SampleCount = 10;

    /// <summary>
    /// The motion vectors texture resolution. Motion blur uses per-pixel motion vectors buffer that contains objects movement information. Use lower resolution to improve performance.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(ResolutionMode.Half), EditorOrder(3), PostProcessSetting((int)MotionBlurSettingsOverride.MotionVectorsResolution)")
    ResolutionMode MotionVectorsResolution = ResolutionMode::Half;

public:

    /// <summary>
    /// Blends the settings using given weight.
    /// </summary>
    /// <param name="other">The other settings.</param>
    /// <param name="weight">The blend weight.</param>
    void BlendWith(MotionBlurSettings& other, float weight);
};

/// <summary>
/// The structure members override flags.
/// </summary>
API_ENUM(Attributes="Flags") enum class ScreenSpaceReflectionsSettingsOverride : int32
{
    /// <summary>
    /// None properties.
    /// </summary>
    None = 0,

    /// <summary>
    /// Overrides <see cref="ScreenSpaceReflectionsSettings.Intensity"/> property.
    /// </summary>
    Intensity = 1 << 0,

    /// <summary>
    /// Overrides <see cref="ScreenSpaceReflectionsSettings.DepthResolution"/> property.
    /// </summary>
    DepthResolution = 1 << 1,

    /// <summary>
    /// Overrides <see cref="ScreenSpaceReflectionsSettings.RayTracePassResolution"/> property.
    /// </summary>
    RayTracePassResolution = 1 << 2,

    /// <summary>
    /// Overrides <see cref="ScreenSpaceReflectionsSettings.BRDFBias"/> property.
    /// </summary>
    BRDFBias = 1 << 3,

    /// <summary>
    /// Overrides <see cref="ScreenSpaceReflectionsSettings.RoughnessThreshold"/> property.
    /// </summary>
    RoughnessThreshold = 1 << 4,

    /// <summary>
    /// Overrides <see cref="ScreenSpaceReflectionsSettings.WorldAntiSelfOcclusionBias"/> property.
    /// </summary>
    WorldAntiSelfOcclusionBias = 1 << 5,

    /// <summary>
    /// Overrides <see cref="ScreenSpaceReflectionsSettings.ResolvePassResolution"/> property.
    /// </summary>
    ResolvePassResolution = 1 << 6,

    /// <summary>
    /// Overrides <see cref="ScreenSpaceReflectionsSettings.ResolveSamples"/> property.
    /// </summary>
    ResolveSamples = 1 << 7,

    /// <summary>
    /// Overrides <see cref="ScreenSpaceReflectionsSettings.EdgeFadeFactor"/> property.
    /// </summary>
    EdgeFadeFactor = 1 << 8,

    /// <summary>
    /// Overrides <see cref="ScreenSpaceReflectionsSettings.UseColorBufferMips"/> property.
    /// </summary>
    UseColorBufferMips = 1 << 9,

    /// <summary>
    /// Overrides <see cref="ScreenSpaceReflectionsSettings.TemporalEffect"/> property.
    /// </summary>
    TemporalEffect = 1 << 10,

    /// <summary>
    /// Overrides <see cref="ScreenSpaceReflectionsSettings.TemporalScale"/> property.
    /// </summary>
    TemporalScale = 1 << 11,

    /// <summary>
    /// Overrides <see cref="ScreenSpaceReflectionsSettings.TemporalResponse"/> property.
    /// </summary>
    TemporalResponse = 1 << 12,

    /// <summary>
    /// Overrides <see cref="ScreenSpaceReflectionsSettings.FadeOutDistance"/> property.
    /// </summary>
    FadeOutDistance = 1 << 13,

    /// <summary>
    /// Overrides <see cref="ScreenSpaceReflectionsSettings.FadeDistance"/> property.
    /// </summary>
    FadeDistance = 1 << 14,

    /// <summary>
    /// All properties.
    /// </summary>
    All = Intensity | DepthResolution | RayTracePassResolution | BRDFBias | RoughnessThreshold | WorldAntiSelfOcclusionBias | ResolvePassResolution | ResolveSamples | EdgeFadeFactor | UseColorBufferMips | TemporalEffect | TemporalScale | TemporalResponse | FadeOutDistance | FadeDistance,
};

/// <summary>
/// Contains settings for Screen Space Reflections effect rendering.
/// </summary>
API_STRUCT() struct FLAXENGINE_API ScreenSpaceReflectionsSettings : ISerializable
{
API_AUTO_SERIALIZATION();
DECLARE_SCRIPTING_TYPE_NO_SPAWN(ScreenSpaceReflectionsSettings);
    typedef ScreenSpaceReflectionsSettingsOverride Override;

    /// <summary>
    /// The flags for overriden properties.
    /// </summary>
    API_FIELD(Attributes="HideInEditor")
    ScreenSpaceReflectionsSettingsOverride OverrideFlags = Override::None;

    /// <summary>
    /// The effect intensity (normalized to range [0;1]). Use 0 to disable it.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(1.0f), Limit(0, 5.0f, 0.01f), EditorOrder(0), PostProcessSetting((int)ScreenSpaceReflectionsSettingsOverride.Intensity)")
    float Intensity = 1.0f;

    /// <summary>
    /// The depth buffer downscale option to optimize raycast performance. Full gives better quality, but half improves performance. The default value is half.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(ResolutionMode.Half), EditorOrder(1), PostProcessSetting((int)ScreenSpaceReflectionsSettingsOverride.DepthResolution)")
    ResolutionMode DepthResolution = ResolutionMode::Half;

    /// <summary>
    /// The raycast resolution. Full gives better quality, but half improves performance. The default value is half.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(ResolutionMode.Half), EditorOrder(2), PostProcessSetting((int)ScreenSpaceReflectionsSettingsOverride.RayTracePassResolution)")
    ResolutionMode RayTracePassResolution = ResolutionMode::Half;

    /// <summary>
    /// The reflection spread parameter. This value controls source roughness effect on reflections blur. Smaller values produce wider reflections spread but also introduce more noise. Higher values provide more mirror-like reflections. Default value is 0.82.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.82f), Limit(0, 1.0f, 0.01f), EditorOrder(3), PostProcessSetting((int)ScreenSpaceReflectionsSettingsOverride.BRDFBias), EditorDisplay(null, \"BRDF Bias\")")
    float BRDFBias = 0.82f;

    /// <summary>
    /// The maximum amount of roughness a material must have to reflect the scene. For example, if this value is set to 0.4, only materials with a roughness value of 0.4 or below reflect the scene. The default value is 0.45.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.45f), Limit(0, 1.0f, 0.01f), EditorOrder(4), PostProcessSetting((int)ScreenSpaceReflectionsSettingsOverride.RoughnessThreshold)")
    float RoughnessThreshold = 0.45f;

    /// <summary>
    /// The offset of the raycast origin. Lower values produce more correct reflection placement, but produce more artifacts. We recommend values of 0.3 or lower. The default value is 0.1.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.1f), Limit(0, 10.0f, 0.01f), EditorOrder(5), PostProcessSetting((int)ScreenSpaceReflectionsSettingsOverride.WorldAntiSelfOcclusionBias)")
    float WorldAntiSelfOcclusionBias = 0.1f;

    /// <summary>
    /// The raycast resolution. Full gives better quality, but half improves performance. The default value is half.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(ResolutionMode.Full), EditorOrder(6), PostProcessSetting((int)ScreenSpaceReflectionsSettingsOverride.ResolvePassResolution)")
    ResolutionMode ResolvePassResolution = ResolutionMode::Full;

    /// <summary>
    /// The number of rays used to resolve the reflection color. Higher values provide better quality but reduce effect performance. Default value is 4. Use 1 for the highest speed.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(4), Limit(1, 8), EditorOrder(7), PostProcessSetting((int)ScreenSpaceReflectionsSettingsOverride.ResolveSamples)")
    int32 ResolveSamples = 4;

    /// <summary>
    /// The point at which the far edges of the reflection begin to fade. Has no effect on performance. The default value is 0.1.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.1f), Limit(0, 1.0f, 0.02f), EditorOrder(8), PostProcessSetting((int)ScreenSpaceReflectionsSettingsOverride.EdgeFadeFactor)")
    float EdgeFadeFactor = 0.1f;

    /// <summary>
    /// The effect fade out end distance from camera (in world units).
    /// </summary>
    API_FIELD(Attributes="DefaultValue(5000.0f), Limit(0), EditorOrder(9), PostProcessSetting((int)ScreenSpaceReflectionsSettingsOverride.FadeOutDistance)")
    float FadeOutDistance = 5000.0f;

    /// <summary>
    /// The effect fade distance (in world units). Defines the size of the effect fade from fully visible to fully invisible at FadeOutDistance.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(500.0f), Limit(0), EditorOrder(10), PostProcessSetting((int)ScreenSpaceReflectionsSettingsOverride.FadeDistance)")
    float FadeDistance = 500.0f;

    /// <summary>
    /// "The input color buffer downscale mode that uses blurred mipmaps when resolving the reflection color. Produces more realistic results by blurring distant parts of reflections in rough (low-gloss) materials. It also improves performance on most platforms but uses more memory.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(true), EditorOrder(11), PostProcessSetting((int)ScreenSpaceReflectionsSettingsOverride.UseColorBufferMips), EditorDisplay(null, \"Use Color Buffer Mips\")")
    bool UseColorBufferMips = true;

    /// <summary>
    /// If checked, enables the temporal pass. Reduces noise, but produces an animated "jittering" effect that's sometimes noticeable. If disabled, the properties below have no effect.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(true), EditorOrder(12), PostProcessSetting((int)ScreenSpaceReflectionsSettingsOverride.TemporalEffect), EditorDisplay(null, \"Enable Temporal Effect\")")
    bool TemporalEffect = true;

    /// <summary>
    /// The intensity of the temporal effect. Lower values produce reflections faster, but more noise. The default value is 8.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(8.0f), Limit(0, 20.0f, 0.5f), EditorOrder(13), PostProcessSetting((int)ScreenSpaceReflectionsSettingsOverride.TemporalScale)")
    float TemporalScale = 8.0f;

    /// <summary>
    /// Defines how quickly reflections blend between the reflection in the current frame and the history buffer. Lower values produce reflections faster, but with more jittering. If the camera in your game doesn't move much, we recommend values closer to 1. The default value is 0.8.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.8f), Limit(0.05f, 1.0f, 0.01f), EditorOrder(14), PostProcessSetting((int)ScreenSpaceReflectionsSettingsOverride.TemporalResponse)")
    float TemporalResponse = 0.8f;

public:

    /// <summary>
    /// Blends the settings using given weight.
    /// </summary>
    /// <param name="other">The other settings.</param>
    /// <param name="weight">The blend weight.</param>
    void BlendWith(ScreenSpaceReflectionsSettings& other, float weight);
};

/// <summary>
/// The structure members override flags.
/// </summary>
API_ENUM(Attributes="Flags") enum class AntiAliasingSettingsOverride : int32
{
    /// <summary>
    /// None properties.
    /// </summary>
    None = 0,

    /// <summary>
    /// Overrides <see cref="AntiAliasingSettings.Mode"/> property.
    /// </summary>
    Mode = 1 << 0,

    /// <summary>
    /// Overrides <see cref="AntiAliasingSettings.TAA_JitterSpread"/> property.
    /// </summary>
    TAA_JitterSpread = 1 << 1,

    /// <summary>
    /// Overrides <see cref="AntiAliasingSettings.TAA_Sharpness"/> property.
    /// </summary>
    TAA_Sharpness = 1 << 2,

    /// <summary>
    /// Overrides <see cref="AntiAliasingSettings.TAA_StationaryBlending"/> property.
    /// </summary>
    TAA_StationaryBlending = 1 << 3,

    /// <summary>
    /// Overrides <see cref="AntiAliasingSettings.TAA_MotionBlending"/> property.
    /// </summary>
    TAA_MotionBlending = 1 << 4,

    /// <summary>
    /// All properties.
    /// </summary>
    All = Mode | TAA_JitterSpread | TAA_Sharpness | TAA_StationaryBlending | TAA_MotionBlending,
};

/// <summary>
/// Contains settings for Anti Aliasing effect rendering.
/// </summary>
API_STRUCT() struct FLAXENGINE_API AntiAliasingSettings : ISerializable
{
API_AUTO_SERIALIZATION();
DECLARE_SCRIPTING_TYPE_NO_SPAWN(AntiAliasingSettings);
    typedef AntiAliasingSettingsOverride Override;

    /// <summary>
    /// The flags for overriden properties.
    /// </summary>
    API_FIELD(Attributes="HideInEditor")
    AntiAliasingSettingsOverride OverrideFlags = Override::None;

    /// <summary>
    /// The anti-aliasing effect mode.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(AntialiasingMode.FastApproximateAntialiasing), EditorOrder(0), PostProcessSetting((int)AntiAliasingSettingsOverride.Mode)")
    AntialiasingMode Mode = AntialiasingMode::FastApproximateAntialiasing;

    /// <summary>
    /// The diameter (in texels) inside which jitter samples are spread. Smaller values result in crisper but more aliased output, while larger values result in more stable but blurrier output.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.75f), Limit(0.1f, 1f, 0.001f), EditorOrder(1), PostProcessSetting((int)AntiAliasingSettingsOverride.TAA_JitterSpread), EditorDisplay(null, \"TAA Jitter Spread\")")
    float TAA_JitterSpread = 0.75f;

    /// <summary>
    /// Controls the amount of sharpening applied to the color buffer. TAA can induce a slight loss of details in high frequency regions. Sharpening alleviates this issue. High values may introduce dark-border artifacts.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0f), Limit(0, 3f, 0.001f), EditorOrder(2), PostProcessSetting((int)AntiAliasingSettingsOverride.TAA_Sharpness), EditorDisplay(null, \"TAA Sharpness\")")
    float TAA_Sharpness = 0.0f;

    /// <summary>
    /// The blend coefficient for stationary fragments. Controls the percentage of history sample blended into final color for fragments with minimal active motion.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.95f), Limit(0, 0.99f, 0.001f), EditorOrder(3), PostProcessSetting((int)AntiAliasingSettingsOverride.TAA_StationaryBlending), EditorDisplay(null, \"TAA Stationary Blending\")")
    float TAA_StationaryBlending = 0.95f;

    /// <summary>
    /// The blending coefficient for moving fragments. Controls the percentage of history sample blended into the final color for fragments with significant active motion.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0.4f), Limit(0, 0.99f, 0.001f), EditorOrder(4), PostProcessSetting((int)AntiAliasingSettingsOverride.TAA_MotionBlending), EditorDisplay(null, \"TAA Motion Blending\")")
    float TAA_MotionBlending = 0.4f;

public:

    /// <summary>
    /// Blends the settings using given weight.
    /// </summary>
    /// <param name="other">The other settings.</param>
    /// <param name="weight">The blend weight.</param>
    void BlendWith(AntiAliasingSettings& other, float weight);
};

/// <summary>
/// Contains settings for custom PostFx materials rendering.
/// </summary>
API_STRUCT() struct FLAXENGINE_API PostFxMaterialsSettings : ISerializable
{
API_AUTO_SERIALIZATION();
DECLARE_SCRIPTING_TYPE_NO_SPAWN(PostFxMaterialsSettings);

    /// <summary>
    /// The post-process materials collection for rendering (fixed capacity).
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(null, EditorDisplayAttribute.InlineStyle)")
    Array<AssetReference<MaterialBase>, FixedAllocation<POST_PROCESS_SETTINGS_MAX_MATERIALS>> Materials;

public:

    /// <summary>
    /// Blends the settings using given weight.
    /// </summary>
    /// <param name="other">The other settings.</param>
    /// <param name="weight">The blend weight.</param>
    void BlendWith(PostFxMaterialsSettings& other, float weight);
};

/// <summary>
/// Contains settings for rendering advanced visual effects and post effects.
/// </summary>
API_STRUCT() struct FLAXENGINE_API PostProcessSettings : ISerializable
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(PostProcessSettings);

    /// <summary>
    /// The ambient occlusion effect settings.
    /// </summary>
    API_FIELD()
    AmbientOcclusionSettings AmbientOcclusion;

    /// <summary>
    /// The bloom effect settings.
    /// </summary>
    API_FIELD()
    BloomSettings Bloom;

    /// <summary>
    /// The tone mapping effect settings.
    /// </summary>
    API_FIELD()
    ToneMappingSettings ToneMapping;

    /// <summary>
    /// The color grading effect settings.
    /// </summary>
    API_FIELD()
    ColorGradingSettings ColorGrading;

    /// <summary>
    /// The eye adaptation effect settings.
    /// </summary>
    API_FIELD()
    EyeAdaptationSettings EyeAdaptation;

    /// <summary>
    /// The camera artifacts effect settings.
    /// </summary>
    API_FIELD()
    CameraArtifactsSettings CameraArtifacts;

    /// <summary>
    /// The lens flares effect settings.
    /// </summary>
    API_FIELD()
    LensFlaresSettings LensFlares;

    /// <summary>
    /// The depth of field effect settings.
    /// </summary>
    API_FIELD()
    DepthOfFieldSettings DepthOfField;

    /// <summary>
    /// The motion blur effect settings.
    /// </summary>
    API_FIELD()
    MotionBlurSettings MotionBlur;

    /// <summary>
    /// The screen space reflections effect settings.
    /// </summary>
    API_FIELD()
    ScreenSpaceReflectionsSettings ScreenSpaceReflections;

    /// <summary>
    /// The anti-aliasing effect settings.
    /// </summary>
    API_FIELD()
    AntiAliasingSettings AntiAliasing;

    /// <summary>
    /// The PostFx materials rendering settings.
    /// </summary>
    API_FIELD()
    PostFxMaterialsSettings PostFxMaterials;

public:

    /// <summary>
    /// Blends the settings using given weight.
    /// </summary>
    /// <param name="other">The other settings.</param>
    /// <param name="weight">The blend weight.</param>
    void BlendWith(PostProcessSettings& other, float weight);

    /// <summary>
    /// Returns true if object has loaded content (all postFx materials and textures are loaded).
    /// </summary>
    /// <returns>True if has content loaded.</returns>
    bool HasContentLoaded() const;

public:

    // [ISerializable]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};

DECLARE_ENUM_OPERATORS(AmbientOcclusionSettingsOverride);
DECLARE_ENUM_OPERATORS(BloomSettingsOverride);
DECLARE_ENUM_OPERATORS(ToneMappingSettingsOverride);
DECLARE_ENUM_OPERATORS(ColorGradingSettingsOverride);
DECLARE_ENUM_OPERATORS(EyeAdaptationSettingsOverride);
DECLARE_ENUM_OPERATORS(CameraArtifactsSettingsOverride);
DECLARE_ENUM_OPERATORS(LensFlaresSettingsOverride);
DECLARE_ENUM_OPERATORS(DepthOfFieldSettingsOverride);
DECLARE_ENUM_OPERATORS(MotionBlurSettingsOverride);
DECLARE_ENUM_OPERATORS(ScreenSpaceReflectionsSettingsOverride);
DECLARE_ENUM_OPERATORS(AntiAliasingSettingsOverride);
