// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_UWP || USE_EDITOR

#include "Engine/Core/Config/PlatformSettingsBase.h"

/// <summary>
/// Universal Windows Platform settings.
/// </summary>
/// <seealso cref="Settings{UWPPlatformSettings}" />
class UWPPlatformSettings : public Settings<UWPPlatformSettings>
{
public:

    /// <summary>
    /// The preferred launch windowing mode.
    /// </summary>
    enum class WindowMode
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
    enum class DisplayOrientations
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
    WindowMode PreferredLaunchWindowingMode = WindowMode::FullScreen;

    /// <summary>
    /// The display orientation modes. Can be combined as flags.
    /// </summary>
    DisplayOrientations AutoRotationPreferences = DisplayOrientations::All;

    /// <summary>
    /// The location of the package certificate (relative to the project).
    /// </summary>
    String CertificateLocation;

    /// <summary>
    /// Enables support for DirectX 11. Disabling it reduces compiled shaders count.
    /// </summary>
    bool SupportDX11 = true;

    /// <summary>
    /// Enables support for DirectX 10 and DirectX 10.1. Disabling it reduces compiled shaders count.
    /// </summary>
    bool SupportDX10 = false;

public:

    // [Settings]
    void RestoreDefault() final override
    {
        PreferredLaunchWindowingMode = WindowMode::FullScreen;
        AutoRotationPreferences = DisplayOrientations::All;
        CertificateLocation.Clear();
        SupportDX11 = true;
        SupportDX10 = false;
    }

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
