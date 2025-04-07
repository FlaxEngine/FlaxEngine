// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_IOS || USE_EDITOR

#include "../Apple/ApplePlatformSettings.h"

/// <summary>
/// iOS platform settings.
/// </summary>
API_CLASS(Sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API iOSPlatformSettings : public ApplePlatformSettings
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(ApplePlatformSettings);
    API_AUTO_SERIALIZATION();

    /// <summary>
    /// The app export destination methods.
    /// </summary>
    API_ENUM() enum class ExportMethods
    {
        // Distribute using TestFlight or through the App Store.
        AppStore,
        // Distribute to a limited number of devices you register in App Store Connect.
        Development,
        // Distribute to a limited number of devices you register in App Store Connect.
        AdHoc,
        // Distribute to members of your organization if you’re a part of the Apple Developer Enterprise Program and are ready to release your app to users in your organization.
        Enterprise,
    };

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
    /// The output textures quality (compression).
    /// </summary>
    API_ENUM() enum class TextureQuality
    {
        // Raw image data without any compression algorithm. Mostly for testing or compatibility.
        Uncompressed,
        // ASTC 4x4 block compression.
        API_ENUM(Attributes="EditorDisplay(null, \"ASTC High\")")
        ASTC_High,
        // ASTC 6x6 block compression.
        API_ENUM(Attributes="EditorDisplay(null, \"ASTC Medium\")")
        ASTC_Medium,
        // ASTC 8x8 block compression.
        API_ENUM(Attributes="EditorDisplay(null, \"ASTC Low\")")
        ASTC_Low,
    };

    /// <summary>
    /// The app developer name - App Store Team ID. For example: 'VG6K6HT8B'.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), EditorDisplay(\"General\")")
    String AppTeamId;

    /// <summary>
    /// The app version number (matches CURRENT_PROJECT_VERSION in XCode).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), EditorDisplay(\"General\")")
    String AppVersion = TEXT("1");

    /// <summary>
    /// The app export mode (if automatic packaging is not disabled via Build Settings, otherwise export app manually via XCode project).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(50), EditorDisplay(\"General\")")
    ExportMethods ExportMethod = ExportMethods::Development;

    /// <summary>
    /// The output textures quality (compression).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(100), EditorDisplay(\"General\")")
    TextureQuality TexturesQuality = TextureQuality::ASTC_Medium;

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
};

#if PLATFORM_IOS
typedef iOSPlatformSettings PlatformSettings;
#endif

#endif
