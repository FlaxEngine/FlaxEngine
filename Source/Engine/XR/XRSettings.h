// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Config/Settings.h"

/// <summary>
/// XR settings container.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API XRSettings : public SettingsBase
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(XRSettings);
    API_AUTO_SERIALIZATION();

public:
    /// <summary>
    /// If checked, the game connects to the XR runtime (eg. SteamVR) on startup to use the head-mounted display. Can be overridden with the -xr and -noxr command line options. In the editor the head-mounted display is used only in play mode and changing this option requires restarting the editor.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0), EditorDisplay(\"General\")")
    bool Enabled = false;

    /// <summary>
    /// The scale of the rendering resolution relative to the resolution recommended by the XR runtime for the head-mounted display. Use lower values to improve performance or higher values to improve quality.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(100), Limit(0.1f, 2.0f, 0.01f), EditorDisplay(\"Rendering\")")
    float RenderScale = 1.0f;

    /// <summary>
    /// If checked, the depth buffer is submitted to the XR runtime together with the headset image. It improves the reprojection of the image when the head moves or the frame rate drops. Requires the runtime support for XR_KHR_composition_layer_depth.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(110), EditorDisplay(\"Rendering\")")
    bool SubmitDepth = true;

    /// <summary>
    /// The display refresh rate (in Hz) requested from the head-mounted display. The closest rate supported by the headset is used. Use 0 to keep the rate set in the XR runtime. Requires the runtime support for XR_FB_display_refresh_rate. Some runtimes (eg. SteamVR) support only the rate selected in the runtime settings.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(120), Limit(0, 1000), EditorDisplay(\"Rendering\")")
    float DisplayRefreshRate = 0.0f;

public:
    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static XRSettings* Get();
};
