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
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static iOSPlatformSettings* Get();

    // [SettingsBase]
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override
    {
        ApplePlatformSettings::Deserialize(stream, modifier);
    }
};

#if PLATFORM_IOS
typedef iOSPlatformSettings PlatformSettings;
#endif

#endif
