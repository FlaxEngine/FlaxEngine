// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_IOS || USE_EDITOR

#include "../Apple/ApplePlatformSettings.h"

/// <summary>
/// iOS platform settings.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API iOSPlatformSettings : public ApplePlatformSettings
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(ApplePlatformSettings);

    /// <summary>
    /// The display orientation modes. Can be combined as flags.
    /// </summary>
    API_ENUM(Attributes="Flags") enum class UIInterfaceOrientations
    {
        // The device is in portrait mode, with the device upright and the Home button on the bottom.
        Portrait = 1,
        // The device is in portrait mode but is upside down, with the device upright and the Home button at the top.
        PortraitUpsideDown = 2,
        // The device is in landscape mode, with the device upright and the Home button on the left.
        LandscapeLeft = 4,
        // The device is in landscape mode, with the device upright and the Home button on the right.
        LandscapeRight = 8,
        // The all modes.
        All = Portrait | PortraitUpsideDown | LandscapeLeft | LandscapeRight
    };

    /// <summary>
    /// The app developer name - App Store Team ID.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), EditorDisplay(\"General\")")
    String AppTeamId;

    /// <summary>
    /// The app version number (matches CURRENT_PROJECT_VERSION in XCode).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), EditorDisplay(\"General\")")
    String AppVersion = TEXT("1");

    /// <summary>
    /// The UI interface orientation modes supported on iPhone devices.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(200), EditorDisplay(\"UI\", \"Supported Interface Orientations (iPhone)\")")
    UIInterfaceOrientations SupportedInterfaceOrientationsiPhone = UIInterfaceOrientations::All;

    /// <summary>
    /// The UI interface orientation modes supported on iPad devices.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(210), EditorDisplay(\"UI\", \"Supported Interface Orientations (iPad)\")")
    UIInterfaceOrientations SupportedInterfaceOrientationsiPad = UIInterfaceOrientations::All;

    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static iOSPlatformSettings* Get();

    // [SettingsBase]
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override
    {
        ApplePlatformSettings::Deserialize(stream, modifier);
        DESERIALIZE(AppTeamId);
        DESERIALIZE(AppVersion);
        DESERIALIZE(SupportedInterfaceOrientationsiPhone);
        DESERIALIZE(SupportedInterfaceOrientationsiPad);
    }
};

#if PLATFORM_IOS
typedef iOSPlatformSettings PlatformSettings;
#endif

#endif
