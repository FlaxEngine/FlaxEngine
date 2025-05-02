// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_WINDOWS && !USE_EDITOR

#include "WindowsGame.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Core/Config/PlatformSettings.h"

void WindowsGame::InitMainWindowSettings(CreateWindowSettings& settings)
{
    // TODO: restore window size and fullscreen mode from the cached local settings saved after previous session

    const auto platformSettings = WindowsPlatformSettings::Get();
    auto windowMode = platformSettings->WindowMode;

    // Use command line switches
    if (CommandLine::Options.Fullscreen.IsTrue())
        windowMode = GameWindowMode::Fullscreen;
    else if (CommandLine::Options.Windowed.IsTrue())
        windowMode = GameWindowMode::Windowed;

    settings.Fullscreen = windowMode == GameWindowMode::Fullscreen;
    settings.HasSizingFrame = platformSettings->ResizableWindow;

    // Fullscreen - put window to cover the whole desktop area
    if (windowMode == GameWindowMode::FullscreenBorderless ||
        windowMode == GameWindowMode::Fullscreen)
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
    settings.AllowMinimize = platformSettings->ResizableWindow;
}

bool WindowsGame::Init()
{
    const auto platformSettings = WindowsPlatformSettings::Get();

    // Create mutex if need to
    if (platformSettings->ForceSingleInstance)
    {
        if (Platform::CreateMutex(*Globals::ProductName))
        {
            Platform::ReleaseMutex();
            Platform::Error(String::Format(TEXT("Only one instance of {0} can be run at the same time."), Globals::ProductName));
            return true;
        }
    }

    return GameBase::Init();
}

void WindowsGame::BeforeExit()
{
    Platform::ReleaseMutex();

    GameBase::BeforeExit();
}

#endif
