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
    /// Enables rendering synchronization with the refresh rate of the display device to avoid "tearing" artifacts.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), DefaultValue(false), EditorDisplay(\"General\", \"Use V-Sync\")")
    bool UseVSync = false;

    /// <summary>
    /// Anti Aliasing quality setting.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1000), DefaultValue(Quality.Medium), EditorDisplay(\"Quality\", \"AA Quality\")")
    Quality AAQuality = Quality::Medium;

    /// <summary>
    /// Screen Space Reflections quality setting.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1100), DefaultValue(Quality.Medium), EditorDisplay(\"Quality\", \"SSR Quality\")")
    Quality SSRQuality = Quality::Medium;

    /// <summary>
    /// Screen Space Ambient Occlusion quality setting.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1200), DefaultValue(Quality.Medium), EditorDisplay(\"Quality\", \"SSAO Quality\")")
    Quality SSAOQuality = Quality::Medium;

    /// <summary>
    /// Volumetric Fog quality setting.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1250), DefaultValue(Quality.High), EditorDisplay(\"Quality\")")
    Quality VolumetricFogQuality = Quality::High;

    /// <summary>
    /// The shadows quality.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1300), DefaultValue(Quality.Medium), EditorDisplay(\"Quality\")")
    Quality ShadowsQuality = Quality::Medium;

    /// <summary>
    /// The shadow maps quality (textures resolution).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1310), DefaultValue(Quality.Medium), EditorDisplay(\"Quality\")")
    Quality ShadowMapsQuality = Quality::Medium;

    /// <summary>
    /// Enables cascades splits blending for directional light shadows.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1320), DefaultValue(false), EditorDisplay(\"Quality\", \"Allow CSM Blending\")")
    bool AllowCSMBlending = false;

    /// <summary>
    /// Default probes cubemap resolution (use for Environment Probes, can be overriden per-actor).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1500), EditorDisplay(\"Quality\")")
    ProbeCubemapResolution DefaultProbeResolution = ProbeCubemapResolution::_128;

    /// <summary>
    /// If checked, Environment Probes will use HDR texture format. Improves quality in very bright scenes at cost of higher memory usage.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1502), EditorDisplay(\"Quality\")")
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
    API_FIELD(Attributes="EditorOrder(2005), DefaultValue(Quality.High), EditorDisplay(\"Global SDF\")")
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
    API_FIELD(Attributes="EditorOrder(2100), DefaultValue(Quality.High), EditorDisplay(\"Global Illumination\")")
    Quality GIQuality = Quality::High;

    /// <summary>
    /// The Global Illumination probes spacing distance (in world units). Defines the quality of the GI resolution. Adjust to 200-500 to improve performance and lower frequency GI data.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2120), Limit(50, 1000), EditorDisplay(\"Global Illumination\")")
    float GIProbesSpacing = 100;

    /// <summary>
    /// Enables cascades splits blending for Global Illumination.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2125), DefaultValue(false), EditorDisplay(\"Global Illumination\", \"GI Cascades Blending\")")
    bool GICascadesBlending = false;

    /// <summary>
    /// The Global Surface Atlas resolution. Adjust it if atlas `flickers` due to overflow (eg. to 4096).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2130), Limit(256, 8192), EditorDisplay(\"Global Illumination\")")
    int32 GlobalSurfaceAtlasResolution = 2048;

    /// <summary>
    /// The default Post Process settings. Can be overriden by PostFxVolume on a level locally, per camera or for a whole map.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10000), EditorDisplay(\"Post Process Settings\", EditorDisplayAttribute.InlineStyle)")
    PostProcessSettings PostProcessSettings;

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
    API_PROPERTY(Attributes="Serialize, Obsolete, NoUndo") DEPRECATED("Use UseHDRProbes instead.") bool GetUeeHDRProbes() const
    {
        return UseHDRProbes;
    }

    API_PROPERTY(Attributes="Serialize, Obsolete, NoUndo") DEPRECATED("Use UseHDRProbes instead.") void SetUeeHDRProbes(bool value);

public:
    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static GraphicsSettings* Get();

    // [SettingsBase]
    void Apply() override;
};
