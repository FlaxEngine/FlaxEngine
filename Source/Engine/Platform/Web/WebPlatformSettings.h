// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WEB || USE_EDITOR

#include "Engine/Core/Config/PlatformSettingsBase.h"

/// <summary>
/// Web platform settings.
/// </summary>
API_CLASS(Sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API WebPlatformSettings : public SettingsBase
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(WebPlatformSettings);
    API_AUTO_SERIALIZATION();

    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static WebPlatformSettings* Get();
};

#if PLATFORM_WEB
typedef WebPlatformSettings PlatformSettings;
#endif

#endif
