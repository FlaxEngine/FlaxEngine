// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_SDL && PLATFORM_WEB

#include "SDLWindow.h"
#include "Engine/Platform/MessageBox.h"
#include "Engine/Core/Log.h"

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
