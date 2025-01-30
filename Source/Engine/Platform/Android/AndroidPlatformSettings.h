// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_ANDROID || USE_EDITOR

#include "Engine/Core/Config/PlatformSettingsBase.h"
#include "Engine/Scripting/SoftObjectReference.h"

class Texture;

/// <summary>
/// Android platform settings.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API AndroidPlatformSettings : public SettingsBase
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(AndroidPlatformSettings);
    API_AUTO_SERIALIZATION();

    /// <summary>
    /// Android screen orientation options.
    /// </summary>
    API_ENUM() enum class FLAXENGINE_API ScreenOrientation
    {
        /// <summary>
        /// "userPortrait" mode
        /// </summary>
        Portrait,

        /// <summary>
        /// "userLandscape" mode
        /// </summary>
        Landscape,

        /// <summary>
        /// "sensorPortrait" mode
        /// </summary>
        SensorPortrait,

        /// <summary>
        /// "sensorLandscape" mode
        /// </summary>
        SensorLandscape,

        /// <summary>
        /// "fullSensor" mode
        /// </summary>
        AutoRotation,
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
    /// The application package name (eg. com.company.product). Custom tokens: ${PROJECT_NAME}, ${COMPANY_NAME}.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0), EditorDisplay(\"General\")")
    String PackageName = TEXT("com.${COMPANY_NAME}.${PROJECT_NAME}");

    /// <summary>
    /// The application version code (eg. 1, 12, 123).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), EditorDisplay(\"General\")")
    String VersionCode = TEXT("1");

    /// <summary>
    /// The minimum Android API level (eg. 20, 28, 34).
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(20), EditorDisplay(\"General\")")
    String MinimumAPILevel = TEXT("23");

    /// <summary>
    /// The target Android API level (eg. 20, 28, 34).
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(30), EditorDisplay(\"General\")")
    String TargetAPILevel = TEXT("33");

    /// <summary>
    /// The application permissions list (eg. android.media.action.IMAGE_CAPTURE). Added to the generated manifest file.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(100), EditorDisplay(\"General\")")
    Array<String> Permissions;

    /// <summary>
    /// The default screen orientation.
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(110), EditorDisplay(\"General\")")
    ScreenOrientation DefaultOrientation = ScreenOrientation::AutoRotation;

    /// <summary>
    /// The output textures quality (compression).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(500), EditorDisplay(\"General\")")
    TextureQuality TexturesQuality = TextureQuality::ASTC_Medium;

    /// <summary>
    /// Whether to build Android App Bundle (aab) side by side with apk.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(500), EditorDisplay(\"General\", \"Build .aab\")")
    bool BuildAAB = true;

    /// <summary>
    /// Custom icon texture to use for the application (overrides the default one).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1030), EditorDisplay(\"Other\")")
    SoftObjectReference<Texture> OverrideIcon;

    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static AndroidPlatformSettings* Get();
};

#if PLATFORM_ANDROID
typedef AndroidPlatformSettings PlatformSettings;
#endif

#endif
