// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if PLATFORM_LINUX && !USE_EDITOR

#include "LinuxGame.h"
#include "Engine/Platform/Window.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Core/Config/PlatformSettings.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Graphics/Textures/TextureData.h"
// hack using TextureTool in Platform module -> TODO: move texture data sampling to texture data itself
#define COMPILE_WITH_TEXTURE_TOOL 1
#include "Engine/Tools/TextureTool/TextureTool.h"

void LinuxGame::InitMainWindowSettings(CreateWindowSettings& settings)
{
    // TODO: restore window size and fullscreen mode from the cached local settings saved after previous session

    const auto platformSettings = LinuxPlatformSettings::Get();
    auto windowMode = platformSettings->WindowMode;

    // Use command line switches
    if (CommandLine::Options.Fullscreen.IsTrue())
        windowMode = GameWindowMode::Fullscreen;
    else if (CommandLine::Options.Windowed.IsTrue())
        windowMode = GameWindowMode::Windowed;

    settings.Title = *Globals::ProductName;
    settings.AllowDragAndDrop = false;
    settings.Fullscreen = windowMode == GameWindowMode::Fullscreen;
    settings.HasSizingFrame = platformSettings->ResizableWindow;

    // Fullscreen - put window to cover the whole desktop area
    if (windowMode == GameWindowMode::FullscreenBorderless ||
        windowMode == GameWindowMode::Fullscreen)
    {
        settings.Size = Platform::GetDesktopSize();
        settings.Position = Vector2::Zero;
    }
        // Not fullscreen - put window in the middle of the screen
    else if (windowMode == GameWindowMode::Windowed ||
        windowMode == GameWindowMode::Borderless)
    {
        settings.Size = Vector2((float)platformSettings->ScreenWidth, (float)platformSettings->ScreenHeight);
        settings.Position = (Platform::GetDesktopSize() - settings.Size) / 2;
    }

    // Windowed mode
    settings.HasBorder = windowMode == GameWindowMode::Windowed || windowMode == GameWindowMode::Fullscreen;
    settings.AllowMaximize = true;
    settings.AllowMinimize = platformSettings->ResizableWindow;

}

bool LinuxGame::Init()
{
    const auto platformSettings = LinuxPlatformSettings::Get();

    // Create mutex if need to
    if (platformSettings->ForceSingleInstance)
    {
        String& appName = Globals::ProductName;
        if (Platform::CreateMutex(*appName))
        {
            Platform::Error(String::Format(TEXT("Only one instance of {0} can be run at the same time."), appName));
            exit(-1);
        }
    }

    return GameBase::Init();
}

Window* LinuxGame::CreateMainWindow()
{
    auto window = GameBase::CreateMainWindow();

    // Setup window icon
    const String iconPath = Globals::ProjectContentFolder / TEXT("icon.png");
    if (FileSystem::FileExists(iconPath))
    {
        TextureData icon;
        if (TextureTool::ImportTexture(iconPath, icon))
        {
            LOG(Warning, "Failed to load icon file.");
        }
        else
        {
            window->SetIcon(icon);
        }
    }
    else
    {
        LOG(Warning, "Missing icon file.");
    }

    return window;
}

#endif
