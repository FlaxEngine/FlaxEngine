// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if PLATFORM_SDL && PLATFORM_MAC

#include "SDLWindow.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/Time.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Platform/IGuiData.h"
#include "Engine/Platform/MessageBox.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Platform/WindowsManager.h"
#include "Engine/Platform/Base/DragDropHelper.h"
#include "Engine/Platform/SDL/SDLClipboard.h"
#include "Engine/Platform/Unix/UnixFile.h"
#include "Engine/Profiler/ProfilerCPU.h"

#include "Engine/Platform/Linux/IncludeX11.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_system.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

bool SDLPlatform::InitInternal()
{
    return false;
}

bool SDLPlatform::UsesWindows()
{
    return false;
}

bool SDLPlatform::UsesWayland()
{
    return false;
}

bool SDLPlatform::UsesX11()
{
    return false;
}

void SDLPlatform::PreHandleEvents()
{
}

void SDLPlatform::PostHandleEvents()
{
}

bool SDLWindow::HandleEventInternal(SDL_Event& event)
{
    return false;
}

void SDLPlatform::SetHighDpiAwarenessEnabled(bool enable)
{
    // TODO: This is now called before Platform::Init, ensure the scaling is changed accordingly during Platform::Init (see ApplePlatform::SetHighDpiAwarenessEnabled)
}

DragDropEffect SDLWindow::DoDragDrop(const StringView& data)
{
    return DragDropEffect::None;
}

DragDropEffect SDLWindow::DoDragDrop(const StringView& data, const Float2& offset, Window* dragSourceWindow)
{
    Show();
    return DragDropEffect::None;
}

#endif
