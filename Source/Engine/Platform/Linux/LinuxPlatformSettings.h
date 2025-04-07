// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_LINUX || USE_EDITOR

#include "Engine/Core/Config/PlatformSettingsBase.h"
#include "Engine/Scripting/SoftObjectReference.h"

class Texture;

/// <summary>
/// Linux platform settings.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API LinuxPlatformSettings : public SettingsBase
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(LinuxPlatformSettings);
    API_AUTO_SERIALIZATION();

    /// <summary>
    /// The default game window mode.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), DefaultValue(GameWindowMode.Windowed), EditorDisplay(\"Window\")")
    GameWindowMode WindowMode = GameWindowMode::Windowed;

    /// <summary>
    /// The default game window width (in pixels).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), DefaultValue(1280), EditorDisplay(\"Window\")")
    int32 ScreenWidth = 1280;

    /// <summary>
    /// The default game window height (in pixels).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), DefaultValue(720), EditorDisplay(\"Window\")")
    int32 ScreenHeight = 720;

    /// <summary>
    /// Enables resizing the game window by the user.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), DefaultValue(false), EditorDisplay(\"Window\")")
    bool ResizableWindow = false;

    /// <summary>
    /// Enables game running when application window loses focus.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1010), DefaultValue(false), EditorDisplay(\"Other\", \"Run In Background\")")
    bool RunInBackground = false;

    /// <summary>
    /// Limits maximum amount of concurrent game instances running to one, otherwise user may launch application more than once.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1020), DefaultValue(false), EditorDisplay(\"Other\")")
    bool ForceSingleInstance = false;

    /// <summary>
    /// Custom icon texture to use for the application (overrides the default one).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1030), EditorDisplay(\"Other\")")
    SoftObjectReference<Texture> OverrideIcon;

    /// <summary>
    /// Enables support for Vulkan. Disabling it reduces compiled shaders count.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2000), DefaultValue(true), EditorDisplay(\"Graphics\")")
    bool SupportVulkan = true;

    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static LinuxPlatformSettings* Get();
};

#if PLATFORM_LINUX
typedef LinuxPlatformSettings PlatformSettings;
#endif

#endif
