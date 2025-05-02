// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"
#include "PostProcessSettings.h"
#include "Enums.h"

/// <summary>
/// Graphics device manager that creates, manages and releases graphics device and related objects.
/// </summary>
API_CLASS(Static, Attributes="DebugCommand") class FLAXENGINE_API Graphics
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(Graphics);
public:
    /// <summary>
    /// Enables rendering synchronization with the refresh rate of the display device to avoid "tearing" artifacts.
    /// </summary>
    API_FIELD() static bool UseVSync;

    /// <summary>
    /// Anti Aliasing quality setting.
    /// </summary>
    API_FIELD() static Quality AAQuality;

    /// <summary>
    /// Screen Space Reflections quality setting.
    /// </summary>
    API_FIELD() static Quality SSRQuality;

    /// <summary>
    /// Screen Space Ambient Occlusion quality setting.
    /// </summary>
    API_FIELD() static Quality SSAOQuality;

    /// <summary>
    /// Volumetric Fog quality setting.
    /// </summary>
    API_FIELD() static Quality VolumetricFogQuality;

    /// <summary>
    /// The shadows quality.
    /// </summary>
    API_FIELD() static Quality ShadowsQuality;

    /// <summary>
    /// The shadow maps quality (textures resolution).
    /// </summary>
    API_FIELD() static Quality ShadowMapsQuality;

    /// <summary>
    /// The global scale for all shadow maps update rate. Can be used to slow down shadows rendering frequency on lower quality settings or low-end platforms. Default 1.
    /// </summary>
    API_FIELD() static float ShadowUpdateRate;

    /// <summary>
    /// Enables cascades splits blending for directional light shadows.
    /// </summary>
    API_FIELD() static bool AllowCSMBlending;

    /// <summary>
    /// The Global SDF quality. Controls the volume texture resolution and amount of cascades to use.
    /// </summary>
    API_FIELD() static Quality GlobalSDFQuality;

    /// <summary>
    /// The Global Illumination quality. Controls the quality of the GI effect.
    /// </summary>
    API_FIELD() static Quality GIQuality;

    /// <summary>
    /// Enables cascades splits blending for Global Illumination.
    /// </summary>
    API_FIELD() static bool GICascadesBlending;

    /// <summary>
    /// The default Post Process settings. Can be overriden by PostFxVolume on a level locally, per camera or for a whole map.
    /// </summary>
    API_FIELD() static PostProcessSettings PostProcessSettings;

public:
    /// <summary>
    /// Debug utility to toggle graphics workloads amortization over several frames by systems such as shadows mapping, global illumination or surface atlas. Can be used to test performance in the worst-case scenario (eg. camera-cut).
    /// </summary>
    API_FIELD() static bool SpreadWorkload;

public:
    /// <summary>
    /// Disposes the device.
    /// </summary>
    static void DisposeDevice();
};

// Skip disabling workload spreading in Release builds
#if BUILD_RELEASE
#define GPU_SPREAD_WORKLOAD true
#else
#define GPU_SPREAD_WORKLOAD Graphics::SpreadWorkload
#endif
