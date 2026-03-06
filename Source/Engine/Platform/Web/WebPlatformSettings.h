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
    /// The output texture compression mode.
    /// </summary>
    API_ENUM() enum class TextureCompression
    {
        // Raw image data without any compression algorithm. Mostly for testing or compatibility.
        Uncompressed,
        // Maintains block compression formats BC1-7 that are supported on Desktop but might not work on Mobile.
        BC,
        // Converts compressed textures into ASTC format that is supported on Mobile but not on Desktop.
        ASTC,
        // Converts compressed textures into Basis Universal format that is supercompressed - can be quickly decoded into GPU native format on demand (BC1-7 or ASTC). Supported on all platforms.
        Basis,
    };

    /// <summary>
    /// The output textures compression mode.
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(500), EditorDisplay(\"General\")")
    TextureCompression TexturesCompression = TextureCompression::Basis;

    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static WebPlatformSettings* Get();
};

#if PLATFORM_WEB
typedef WebPlatformSettings PlatformSettings;
#endif

#endif
