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
    /// Anti Aliasing quality setting. Available values are: Low, Medium, High, Ultra (or 0, 1, 2, 3).
    /// </summary>
    API_FIELD() static Quality AAQuality;

    /// <summary>
    /// Screen Space Reflections quality setting. Available values are: Low, Medium, High, Ultra (or 0, 1, 2, 3).
    /// </summary>
    API_FIELD() static Quality SSRQuality;

    /// <summary>
    /// Screen Space Ambient Occlusion quality setting. Available values are: Low, Medium, High, Ultra (or 0, 1, 2, 3).
    /// </summary>
    API_FIELD() static Quality SSAOQuality;

    /// <summary>
    /// Volumetric Fog quality setting. Available values are: Low, Medium, High, Ultra (or 0, 1, 2, 3).
    /// </summary>
    API_FIELD() static Quality VolumetricFogQuality;

    /// <summary>
    /// The shadows filtering quality (sampling). Available values are: Low, Medium, High, Ultra (or 0, 1, 2, 3).
    /// </summary>
    API_FIELD() static Quality ShadowsQuality;

    /// <summary>
    /// The shadow maps quality (textures resolution). Available values are: Low, Medium, High, Ultra (or 0, 1, 2, 3).
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
    /// The Global SDF quality. Controls the volume texture resolution and amount of cascades to use. Available values are: Low, Medium, High, Ultra (or 0, 1, 2, 3).
    /// </summary>
    API_FIELD() static Quality GlobalSDFQuality;

    /// <summary>
    /// The Global Illumination quality. Controls the quality of the GI effect. Available values are: Low, Medium, High, Ultra (or 0, 1, 2, 3).
    /// </summary>
    API_FIELD() static Quality GIQuality;

    /// <summary>
    /// The Global Illumination probes spacing distance (in world units). Defines the quality of the GI resolution. Adjust to 200-500 to improve performance and lower frequency GI data.
    /// </summary>
    API_FIELD() static float GIProbesSpacing;

    /// <summary>
    /// Enables cascades splits blending for Global Illumination.
    /// </summary>
    API_FIELD() static bool GICascadesBlending;

    /// <summary>
    /// The Global Surface Atlas resolution. Adjust it if atlas `flickers` due to overflow (eg. to 4096).
    /// </summary>
    API_FIELD() static int32 GlobalSurfaceAtlasResolution;

    /// <summary>
    /// The default Post Process settings. Can be overriden by PostFxVolume on a level locally, per camera or for a whole map.
    /// </summary>
    API_FIELD() static PostProcessSettings PostProcessSettings;

    /// <summary>
    /// Enables Gamma color space workflow (instead of Linear). Gamma color space defines colors with an applied a gamma curve (sRGB) so they are perceptually linear.
    /// This makes sense when the output of the rendering represent final color values that will be presented to a non-HDR screen.
    /// </summary>
    API_FIELD(ReadOnly) static bool GammaColorSpace;

public:
    /// <summary>
    /// Debug utility to toggle graphics workloads amortization over several frames by systems such as shadows mapping, global illumination or surface atlas. Can be used to test performance in the worst-case scenario (eg. camera-cut).
    /// </summary>
    API_FIELD() static bool SpreadWorkload;

#if BUILD_RELEASE && !USE_EDITOR
    /// <summary>Unused.</summary>
    API_FIELD() static constexpr float TestValue = 0.0f;
#else
    /// <summary>
    /// Debug utility to control visual or rendering features during development. For example, can be used to branch different code paths in shaders for A/B testing (perf or quality).
    /// </summary>
    API_FIELD() static float TestValue;
#endif

public:
    // Shadows rendering configuration.
    API_CLASS(Static, Attributes="DebugCommand") class FLAXENGINE_API Shadows
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(Shadows);

        // The minimum size in pixels of objects to cast shadows. Improves performance by skipping too small objects (eg. sub-pixel) from rendering into shadow maps.
        API_FIELD() static float MinObjectPixelSize;

#if COMPILE_WITH_PROFILER
        /// <summary>
        /// Dumps active shadow projections info to the log (the next frame). Can be used to inspect what lights are casting shadows (for optimization).
        /// </summary>
        API_FUNCTION(Attributes="DebugCommand") static void Dump();
#endif
    };

    // Motion Vectors rendering configuration.
    API_CLASS(Static, Attributes="DebugCommand") class FLAXENGINE_API MotionVectors
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(MotionVectors);

        // The minimum screen size of objects to draw motion vectors. Improves performance by skipping too small objects (eg. sub-pixel) from rendering motion vectors.
        API_FIELD() static float MinObjectScreenSize;
    };

    // Post Processing effects rendering configuration.
    API_CLASS(Static, Attributes="DebugCommand") class FLAXENGINE_API PostProcessing
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(PostProcessing);

        // Toggles between 2D and 3D LUT texture for Color Grading.
        API_FIELD() static bool ColorGradingVolumeLUT;
    };

    // Global Illumination rendering configuration and API.
    API_CLASS(Static, Attributes="DebugCommand") class FLAXENGINE_API GI
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(GI);

        // Gets the normalized 0-1 progress of the Global Illumination lighting bounces convergence (0 = no GI, 1 = full GI convergence). Can be used to continue displaying loading screen after scene load to ensure indirect lighting has been evaluated. Non-zero value means GI has started to be calculated.
        API_FUNCTION(Attributes="DebugCommand(Hide=true)")
        static float GetConvergence(const RenderBuffers* buffers);

#if COMPILE_WITH_PROFILER
        /// <summary>
        /// Dumps Global Illumination rendering info to the log (the next frame). Can be used to inspect DDGI, Global Surface Atlas and Global SDF memory and usage (for optimization). Prints info about the number of probes, cascades and draw state.
        /// </summary>
        API_FUNCTION(Attributes="DebugCommand") static void Dump();
#endif
    };

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
