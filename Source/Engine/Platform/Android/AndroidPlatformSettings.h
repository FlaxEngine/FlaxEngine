// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_ANDROID || USE_EDITOR

#include "Engine/Core/Config/PlatformSettingsBase.h"

/// <summary>
/// Android platform settings.
/// </summary>
/// <seealso cref="Settings{AndroidPlatformSettings}" />
class AndroidPlatformSettings : public Settings<AndroidPlatformSettings>
{
public:

    /// <summary>
    /// The application package name (eg. com.company.product). Custom tokens: ${PROJECT_NAME}, ${COMPANY_NAME}.
    /// </summary>
    String PackageName;

    /// <summary>
    /// The application permissions list (eg. android.media.action.IMAGE_CAPTURE). Added to the generated manifest file.
    /// </summary>
    Array<String> Permissions;

    /// <summary>
    /// Custom icon texture (asset id) to use for the application (overrides the default one).
    /// </summary>
    Guid OverrideIcon;

public:

    AndroidPlatformSettings()
    {
        RestoreDefault();
    }

    // [Settings]
    void RestoreDefault() final override
    {
        PackageName = TEXT("com.${COMPANY_NAME}.${PROJECT_NAME}");
        Permissions.Clear();
        OverrideIcon = Guid::Empty;
    }

    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override
    {
        DESERIALIZE(PackageName);
        DESERIALIZE(Permissions);
        DESERIALIZE(OverrideIcon);
    }
};

#if PLATFORM_LINUX
typedef AndroidPlatformSettings PlatformSettings;
#endif

#endif
