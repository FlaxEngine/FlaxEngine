// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"
#include "Engine/Graphics/Enums.h"
#include "Engine/Graphics/PostProcessSettings.h"

class FontAsset;

/// <summary>
/// Graphics rendering settings.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings", NoConstructor) class FLAXENGINE_API GraphicsSettings : public SettingsBase
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE_MINIMAL(GraphicsSettings);

public:
    /// <summary>
    /// List of pixel formats that can be used by the rendering pipeline (for light buffer and post-processing).
    /// </summary>
    API_ENUM(Attributes="EnumDisplay(EnumDisplayAttribute.FormatMode.None)")
    enum class RenderColorFormats
    {
        // HDR 32-bit buffer without alpha channel support. Offers good performance but might result in colors banding or shift towards yellowish colors due to low data precision.
        R11G11B10,
        // LDR 32-bit buffer with alpha channel support. Offers good performance but doesn't support High Dynamic Range rendering.
        R8G8B8A8,
        // HDR 64-bit buffer with alpha channel support. Offers very good quality for wide range of colors but requires more memory.
        R16G16B16A16,
    };

    /// <summary>
    /// The environment probes cubemap texture storage formats.
    /// </summary>
    API_ENUM(Attributes = "EnumDisplay(EnumDisplayAttribute.FormatMode.None)")
    enum class ProbeCubemapFormats
    {
        // LDR uncompressed format (32-bit per pixel).
        R8G8B8A8,
        // HDR uncompressed format (32-bit per pixel, no alpha).
        R11G11B10,
        // HDR compressed format (8-bit per pixel, no alpha). Converted into ASTC/Basis for mobile/web. Realtime probes will fallback to R11G11B10.
        BC6,
        // HDR compressed format (8-bit per pixel). Converted into ASTC/Basis for mobile/web. Realtime probes will fallback to R11G11B10.
        BC7,
    };

public:
    /// <summary>
    /// Enables rendering synchronization with the refresh rate of the display device to avoid "tearing" artifacts.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), EditorDisplay(\"General\", \"Use V-Sync\")")
    bool UseVSync = false;

    /// <summary>
    /// Anti Aliasing quality setting.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1000), EditorDisplay(\"Quality\", \"AA Quality\")")
    Quality AAQuality = Quality::High;

    /// <summary>
    /// Screen Space Reflections quality setting.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1100), EditorDisplay(\"Quality\", \"SSR Quality\")")
    Quality SSRQuality = Quality::High;

    /// <summary>
    /// Screen Space Ambient Occlusion quality setting.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1200), EditorDisplay(\"Quality\", \"SSAO Quality\")")
    Quality SSAOQuality = Quality::High;

    /// <summary>
    /// Volumetric Fog quality setting.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1250), EditorDisplay(\"Quality\")")
    Quality VolumetricFogQuality = Quality::High;

    /// <summary>
    /// The shadows quality.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1300), EditorDisplay(\"Quality\")")
    Quality ShadowsQuality = Quality::High;

    /// <summary>
    /// The shadow maps quality (textures resolution).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1310), EditorDisplay(\"Quality\")")
    Quality ShadowMapsQuality = Quality::High;

    /// <summary>
    /// Enables cascades splits blending for directional light shadows.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1320), EditorDisplay(\"Quality\", \"Allow CSM Blending\")")
    bool AllowCSMBlending = false;

    /// <summary>
    /// Default probes cubemap resolution (use for Environment Probes, can be overriden per-actor).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1500), EditorDisplay(\"Quality\")")
    ProbeCubemapResolution DefaultProbeResolution = ProbeCubemapResolution::_128;

    /// <summary>
    /// Environment Probes texture storage format. Controls the quality fo reflections and memory usage of probes data.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1502), EditorDisplay(\"Quality\")")
    ProbeCubemapFormats DefaultProbeCubemapFormat = ProbeCubemapFormats::BC6;

    /// <summary>
    /// If checked, Environment Probes will use HDR texture format. Improves quality in very bright scenes at cost of higher memory usage.
    /// [Deprecated in v1.13]
    /// </summary>
    DEPRECATED("Use DefaultProbeCubemapFormat instead.")
    bool UseHDRProbes = false;

    /// <summary>
    /// If checked, enables Global SDF rendering. This can be used in materials, shaders, and particles.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2000), EditorDisplay(\"Global SDF\")")
    bool EnableGlobalSDF = false;

    /// <summary>
    /// Draw distance of the Global SDF. Actual value can be larger when using DDGI.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2001), EditorDisplay(\"Global SDF\"), Limit(1000), ValueCategory(Utils.ValueCategory.Distance)")
    float GlobalSDFDistance = 15000.0f;

    /// <summary>
    /// The Global SDF quality. Controls the volume texture resolution and amount of cascades to use.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2005), EditorDisplay(\"Global SDF\")")
    Quality GlobalSDFQuality = Quality::High;

#if USE_EDITOR
    /// <summary>
    /// If checked, the 'Generate SDF' option will be checked on model import options by default. Use it if your project uses Global SDF (eg. for Global Illumination or particles).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2010), EditorDisplay(\"Global SDF\")")
    bool GenerateSDFOnModelImport = false;
#endif

    /// <summary>
    /// The Global Illumination quality. Controls the quality of the GI effect.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2100), EditorDisplay(\"Global Illumination\")")
    Quality GIQuality = Quality::High;

    /// <summary>
    /// The Global Illumination probes spacing distance (in world units). Defines the quality of the GI resolution. Adjust to 200-500 to improve performance and lower frequency GI data.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2120), Limit(50, 1000), EditorDisplay(\"Global Illumination\"), ValueCategory(Utils.ValueCategory.Distance)")
    float GIProbesSpacing = 100;

    /// <summary>
    /// Enables cascades splits blending for Global Illumination.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2125), EditorDisplay(\"Global Illumination\", \"GI Cascades Blending\")")
    bool GICascadesBlending = false;

    /// <summary>
    /// The Global Surface Atlas resolution. Adjust it if atlas `flickers` due to overflow (eg. to 4096).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2130), Limit(256, 8192), EditorDisplay(\"Global Illumination\")")
    int32 GlobalSurfaceAtlasResolution = 2048;

public:
    /// <summary>
    /// If checked, color space workflow will use Gamma instead of Linear. Gamma color space defines colors with an applied a gamma curve (sRGB) so they are perceptually linear.
    /// This makes sense when the output of the rendering represent final color values that will be presented to a non-HDR screen.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(3000), EditorDisplay(\"Colors\")")
    bool GammaColorSpace = true;

    /// <summary>
    /// Pixel format used by the rendering pipeline (for light buffer and post-processing).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(3010), EditorDisplay(\"Colors\")")
    RenderColorFormats RenderColorFormat = RenderColorFormats::R11G11B10;

public:
    /// <summary>
    /// The default Post Process settings. Can be overriden by PostFxVolume on a level locally, per camera or for a whole map.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10000), EditorDisplay(\"Post Process Settings\", EditorDisplayAttribute.InlineStyle)")
    PostProcessSettings PostProcessSettings;

public:
    /// <summary>
    /// The list of fallback fonts used for text rendering. Ignored if empty.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(5000), EditorDisplay(\"Text\")")
    Array<AssetReference<FontAsset>> FallbackFonts;

private:
    /// <summary>
    /// Renamed UeeHDRProbes into UseHDRProbes
    /// [Deprecated on 12.10.2022, expires on 12.10.2024]
    /// </summary>
    API_PROPERTY(Attributes="Serialize, Obsolete, NoUndo") DEPRECATED("Use DefaultProbeCubemapFormat instead.") bool GetUeeHDRProbes() const;
    API_PROPERTY(Attributes="Serialize, Obsolete, NoUndo") DEPRECATED("Use DefaultProbeCubemapFormat instead.") void SetUeeHDRProbes(bool value);
    API_PROPERTY(Attributes="Serialize, Obsolete, NoUndo") DEPRECATED("Use DefaultProbeCubemapFormat instead.") bool GetUseHDRProbes() const;
    API_PROPERTY(Attributes="Serialize, Obsolete, NoUndo") DEPRECATED("Use DefaultProbeCubemapFormat instead.") void SetUseHDRProbes(bool value);
    API_FUNCTION(Attributes="OnDeserializing", Hidden) void OnDeserializing(const CallbackContext& context);

public:
    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static GraphicsSettings* Get();

    // [SettingsBase]
    void Apply() override;
};
