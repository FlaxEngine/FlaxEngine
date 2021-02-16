// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS || USE_EDITOR

#include "Engine/Core/Config/PlatformSettingsBase.h"

/// <summary>
/// Windows platform settings.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API WindowsPlatformSettings : public SettingsBase
{
DECLARE_SCRIPTING_TYPE_MINIMAL(WindowsPlatformSettings);
public:

    /// <summary>
    /// The default game window mode.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), DefaultValue(GameWindowMode.Windowed), EditorDisplay(\"Window\")")
    GameWindowMode WindowMode = GameWindowMode::Windowed;

    /// <summary>
    /// The default game window width (in pixels).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), DefaultValue(1280), EditorDisplay(\"Window\")")
    int32 ScreenWidth = 1280;

    /// <summary>
    /// The default game window height (in pixels).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), DefaultValue(720), EditorDisplay(\"Window\")")
    int32 ScreenHeight = 720;

    /// <summary>
    /// Enables resizing the game window by the user.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), DefaultValue(false), EditorDisplay(\"Window\")")
    bool ResizableWindow = false;

    /// <summary>
    /// Enables game running when application window loses focus.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1010), DefaultValue(false), EditorDisplay(\"Other\", \"Run In Background\")")
    bool RunInBackground = false;

    /// <summary>
    /// Limits maximum amount of concurrent game instances running to one, otherwise user may launch application more than once.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1020), DefaultValue(false), EditorDisplay(\"Other\")")
    bool ForceSingleInstance = false;

    /// <summary>
    /// Custom icon texture (asset id) to use for the application (overrides the default one).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1030), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.AssetRefEditor\"), AssetReference(typeof(Texture)), EditorDisplay(\"Other\")")
    Guid OverrideIcon;

    /// <summary>
    /// Enables support for DirectX 12. Disabling it reduces compiled shaders count.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2000), DefaultValue(false), EditorDisplay(\"Graphics\", \"Support DirectX 12\")")
    bool SupportDX12 = false;

    /// <summary>
    /// Enables support for DirectX 11. Disabling it reduces compiled shaders count.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2010), DefaultValue(true), EditorDisplay(\"Graphics\", \"Support DirectX 11\")")
    bool SupportDX11 = true;

    /// <summary>
    /// Enables support for DirectX 10 and DirectX 10.1. Disabling it reduces compiled shaders count.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2020), DefaultValue(false), EditorDisplay(\"Graphics\", \"Support DirectX 10\")")
    bool SupportDX10 = false;

    /// <summary>
    /// Enables support for Vulkan. Disabling it reduces compiled shaders count.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2030), DefaultValue(false), EditorDisplay(\"Graphics\")")
    bool SupportVulkan = false;

public:

    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static WindowsPlatformSettings* Get();

    // [SettingsBase]
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override
    {
        DESERIALIZE(WindowMode);
        DESERIALIZE(ScreenWidth);
        DESERIALIZE(ScreenHeight);
        DESERIALIZE(RunInBackground);
        DESERIALIZE(ResizableWindow);
        DESERIALIZE(ForceSingleInstance);
        DESERIALIZE(OverrideIcon);
        DESERIALIZE(SupportDX12);
        DESERIALIZE(SupportDX11);
        DESERIALIZE(SupportDX10);
        DESERIALIZE(SupportVulkan);
    }
};

#if PLATFORM_WINDOWS
typedef WindowsPlatformSettings PlatformSettings;
#endif

#endif
