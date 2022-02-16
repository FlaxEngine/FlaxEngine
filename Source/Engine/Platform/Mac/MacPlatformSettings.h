// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_MAC || USE_EDITOR

#include "Engine/Core/Config/PlatformSettingsBase.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Scripting/SoftObjectReference.h"

class Texture;

/// <summary>
/// Mac platform settings.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API MacPlatformSettings : public SettingsBase
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(MacPlatformSettings);

    /// <summary>
    /// The app identifier (reversed DNS, eg. com.company.product). Custom tokens: ${PROJECT_NAME}, ${COMPANY_NAME}.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0), EditorDisplay(\"General\")")
    String AppIdentifier = TEXT("com.${COMPANY_NAME}.${PROJECT_NAME}");

    /// <summary>
    /// The default game window mode.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(110), EditorDisplay(\"Window\")")
    GameWindowMode WindowMode = GameWindowMode::Windowed;

    /// <summary>
    /// The default game window width (in pixels).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(120), EditorDisplay(\"Window\")")
    int32 ScreenWidth = 1280;

    /// <summary>
    /// The default game window height (in pixels).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(130), EditorDisplay(\"Window\")")
    int32 ScreenHeight = 720;

    /// <summary>
    /// Enables resizing the game window by the user.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(140), EditorDisplay(\"Window\")")
    bool ResizableWindow = false;

    /// <summary>
    /// Custom icon texture to use for the application (overrides the default one).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1000), EditorDisplay(\"Other\")")
    SoftObjectReference<Texture> OverrideIcon;

    /// <summary>
    /// Enables game running when application window loses focus.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1010), EditorDisplay(\"Other\", \"Run In Background\")")
    bool RunInBackground = false;

public:
    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static MacPlatformSettings* Get();

    // [SettingsBase]
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override
    {
        DESERIALIZE(AppIdentifier);
        DESERIALIZE(WindowMode);
        DESERIALIZE(ScreenWidth);
        DESERIALIZE(ScreenHeight);
        DESERIALIZE(ResizableWindow);
        DESERIALIZE(OverrideIcon);
        DESERIALIZE(RunInBackground);
    }
};

#if PLATFORM_MAC
typedef MacPlatformSettings PlatformSettings;
#endif

#endif
