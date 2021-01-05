// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_UWP || USE_EDITOR

#include "Engine/Core/Config/PlatformSettingsBase.h"

/// <summary>
/// Universal Windows Platform settings.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API UWPPlatformSettings : public SettingsBase
{
DECLARE_SCRIPTING_TYPE_MINIMAL(UWPPlatformSettings);
public:

    /// <summary>
    /// The preferred launch windowing mode.
    /// </summary>
    API_ENUM() enum class WindowMode
    {
        /// <summary>
        /// The full screen mode
        /// </summary>
        FullScreen = 0,

        /// <summary>
        /// The view size.
        /// </summary>
        ViewSize = 1,
    };

    /// <summary>
    /// The display orientation modes. Can be combined as flags.
    /// </summary>
    API_ENUM(Attributes="Flags") enum class DisplayOrientations
    {
        /// <summary>
        /// The none.
        /// </summary>
        None = 0,

        /// <summary>
        /// The landscape.
        /// </summary>
        Landscape = 1,

        /// <summary>
        /// The landscape flipped.
        /// </summary>
        LandscapeFlipped = 2,

        /// <summary>
        /// The portrait.
        /// </summary>
        Portrait = 4,

        /// <summary>
        /// The portrait flipped.
        /// </summary>
        PortraitFlipped = 8,

        /// <summary>
        /// The all modes.
        /// </summary>
        All = Landscape | LandscapeFlipped | Portrait | PortraitFlipped
    };

public:

    /// <summary>
    /// The preferred launch windowing mode. Always fullscreen on Xbox.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), DefaultValue(WindowMode.FullScreen), EditorDisplay(\"Window\")")
    WindowMode PreferredLaunchWindowingMode = WindowMode::FullScreen;

    /// <summary>
    /// The display orientation modes. Can be combined as flags.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), DefaultValue(DisplayOrientations.All), EditorDisplay(\"Window\")")
    DisplayOrientations AutoRotationPreferences = DisplayOrientations::All;

    /// <summary>
    /// The location of the package certificate (relative to the project).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1010), DefaultValue(\"\"), EditorDisplay(\"Other\")")
    String CertificateLocation;

    /// <summary>
    /// Enables support for DirectX 11. Disabling it reduces compiled shaders count.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2000), DefaultValue(true), EditorDisplay(\"Graphics\", \"Support DirectX 11\")")
    bool SupportDX11 = true;

    /// <summary>
    /// Enables support for DirectX 10 and DirectX 10.1. Disabling it reduces compiled shaders count.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2010), DefaultValue(false), EditorDisplay(\"Graphics\", \"Support DirectX 10\")")
    bool SupportDX10 = false;

public:

    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static UWPPlatformSettings* Get();

    // [SettingsBase]
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override
    {
        DESERIALIZE(PreferredLaunchWindowingMode);
        DESERIALIZE(AutoRotationPreferences);
        DESERIALIZE(CertificateLocation);
        DESERIALIZE(SupportDX11);
        DESERIALIZE(SupportDX10);
    }
};

#if PLATFORM_UWP
typedef UWPPlatformSettings PlatformSettings;
#endif

#endif
