// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_MAC && !USE_EDITOR

#include "MacGame.h"
#include "Engine/Platform/Window.h"
#include "Engine/Core/Config/PlatformSettings.h"
#include "Engine/Engine/CommandLine.h"

void MacGame::InitMainWindowSettings(CreateWindowSettings& settings)
{
    // TODO: restore window size and fullscreen mode from the cached local settings saved after previous session

    const auto platformSettings = MacPlatformSettings::Get();
    auto windowMode = platformSettings->WindowMode;

    // Use command line switches
    if (CommandLine::Options.Fullscreen.IsTrue())
        windowMode = GameWindowMode::Fullscreen;
    else if (CommandLine::Options.Windowed.IsTrue())
        windowMode = GameWindowMode::Windowed;

    settings.AllowDragAndDrop = false;
    settings.Fullscreen = windowMode == GameWindowMode::Fullscreen;
    settings.HasSizingFrame = platformSettings->ResizableWindow;

    // Fullscreen - put window to cover the whole desktop area
    if (windowMode == GameWindowMode::FullscreenBorderless ||  windowMode == GameWindowMode::Fullscreen)
    {
        settings.Size = Platform::GetDesktopSize();
        settings.Position = Float2::Zero;
    }
    // Not fullscreen - put window in the middle of the screen
    else if (windowMode == GameWindowMode::Windowed || windowMode == GameWindowMode::Borderless)
    {
        settings.Size = Float2((float)platformSettings->ScreenWidth, (float)platformSettings->ScreenHeight);
        settings.Position = (Platform::GetDesktopSize() - settings.Size) * 0.5f;
    }

    // Windowed mode
    settings.HasBorder = windowMode == GameWindowMode::Windowed || windowMode == GameWindowMode::Fullscreen;
    settings.AllowMaximize = true;
    settings.AllowMinimize = platformSettings->ResizableWindow;

}

#endif
