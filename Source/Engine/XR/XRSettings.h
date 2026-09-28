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
    /// If checked, the game connects to the XR runtime (eg. SteamVR) on startup to use the head-mounted display. Can be overridden with the -xr and -noxr command line options.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0), EditorDisplay(\"General\")")
    bool Enabled = false;

public:
    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static XRSettings* Get();
};
