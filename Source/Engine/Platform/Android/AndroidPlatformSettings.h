// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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
    /// The application package name (eg. com.company.product). Custom tokens: ${PROJECT_NAME}, ${COMPANY_NAME}.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0), EditorDisplay(\"General\")")
    String PackageName = TEXT("com.${COMPANY_NAME}.${PROJECT_NAME}");

    /// <summary>
    /// The application permissions list (eg. android.media.action.IMAGE_CAPTURE). Added to the generated manifest file.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(100), EditorDisplay(\"General\")")
    Array<String> Permissions;

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
