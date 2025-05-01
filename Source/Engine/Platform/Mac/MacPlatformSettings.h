// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_MAC || USE_EDITOR

#include "../Apple/ApplePlatformSettings.h"

/// <summary>
/// Mac platform settings.
/// </summary>
API_CLASS(Sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API MacPlatformSettings : public ApplePlatformSettings
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(MacPlatformSettings);
    API_AUTO_SERIALIZATION();

    /// <summary>
    /// The default game window mode.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(110), EditorDisplay(\"Window\")")
    GameWindowMode WindowMode = GameWindowMode::Windowed;

    /// <summary>
    /// The default game window width (in pixels).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(120), EditorDisplay(\"Window\")")
    int32 ScreenWidth = 1280;

    /// <summary>
    /// The default game window height (in pixels).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(130), EditorDisplay(\"Window\")")
    int32 ScreenHeight = 720;

    /// <summary>
    /// Enables resizing the game window by the user.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(140), EditorDisplay(\"Window\")")
    bool ResizableWindow = false;

    /// <summary>
    /// Enables game running when application window loses focus.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1010), EditorDisplay(\"Other\", \"Run In Background\")")
    bool RunInBackground = false;

    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static MacPlatformSettings* Get();
};

#if PLATFORM_MAC
typedef MacPlatformSettings PlatformSettings;
#endif

#endif
