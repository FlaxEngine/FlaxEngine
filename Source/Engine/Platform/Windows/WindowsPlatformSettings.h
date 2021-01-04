// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS || USE_EDITOR

#include "Engine/Core/Config/PlatformSettingsBase.h"

/// <summary>
/// Windows platform settings.
/// </summary>
/// <seealso cref="SettingsBase{WindowsPlatformSettings}" />
class WindowsPlatformSettings : public SettingsBase<WindowsPlatformSettings>
{
public:

    /// <summary>
    /// The default game window mode.
    /// </summary>
    GameWindowMode WindowMode = GameWindowMode::Windowed;

    /// <summary>
    /// The default game window width (in pixels).
    /// </summary>
    int32 ScreenWidth = 1280;

    /// <summary>
    /// The default game window height (in pixels).
    /// </summary>
    int32 ScreenHeight = 720;

    /// <summary>
    /// Enables game running when application window loses focus.
    /// </summary>
    bool RunInBackground = false;

    /// <summary>
    /// Enables resizing the game window by the user.
    /// </summary>
    bool ResizableWindow = false;

    /// <summary>
    /// Limits maximum amount of concurrent game instances running to one, otherwise user may launch application more than once.
    /// </summary>
    bool ForceSingleInstance = false;

    /// <summary>
    /// Custom icon texture (asset id) to use for the application (overrides the default one).
    /// </summary>
    Guid OverrideIcon;

    /// <summary>
    /// Enables support for DirectX 12. Disabling it reduces compiled shaders count.
    /// </summary>
    bool SupportDX12 = false;

    /// <summary>
    /// Enables support for DirectX 11. Disabling it reduces compiled shaders count.
    /// </summary>
    bool SupportDX11 = true;

    /// <summary>
    /// Enables support for DirectX 10 and DirectX 10.1. Disabling it reduces compiled shaders count.
    /// </summary>
    bool SupportDX10 = false;

    /// <summary>
    /// Enables support for Vulkan. Disabling it reduces compiled shaders count.
    /// </summary>
    bool SupportVulkan = false;

public:

    // [SettingsBase]
    void RestoreDefault() final override
    {
        WindowMode = GameWindowMode::Windowed;
        ScreenWidth = 1280;
        ScreenHeight = 720;
        RunInBackground = false;
        ResizableWindow = false;
        ForceSingleInstance = false;
        OverrideIcon = Guid::Empty;
        SupportDX12 = false;
        SupportDX11 = true;
        SupportDX10 = false;
        SupportVulkan = false;
    }

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
