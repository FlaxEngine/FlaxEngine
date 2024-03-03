// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"
#include "PostProcessSettings.h"
#include "Enums.h"

/// <summary>
/// Graphics device manager that creates, manages and releases graphics device and related objects.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API Graphics
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
    /// The default Post Process settings. Can be overriden by PostFxVolume on a level locally, per camera or for a whole map.
    /// </summary>
    API_FIELD() static PostProcessSettings PostProcessSettings;

public:
    /// <summary>
    /// Disposes the device.
    /// </summary>
    static void DisposeDevice();
};
