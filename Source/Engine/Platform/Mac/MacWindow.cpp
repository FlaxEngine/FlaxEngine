// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if PLATFORM_MAC

#include "../Window.h"
#include "Engine/Graphics/RenderTask.h"

MacWindow::MacWindow(const CreateWindowSettings& settings)
    : WindowBase(settings)
{
    int32 x = Math::TruncToInt(settings.Position.X);
    int32 y = Math::TruncToInt(settings.Position.Y);
    int32 clientWidth = Math::TruncToInt(settings.Size.X);
    int32 clientHeight = Math::TruncToInt(settings.Size.Y);
    int32 windowWidth = clientWidth;
    int32 windowHeight = clientHeight;
    _clientSize = Vector2((float)clientWidth, (float)clientHeight);

    // TODO: setup window
}

MacWindow::~MacWindow()
{
    // TODO: close window
}

void* MacWindow::GetNativePtr() const
{
    // TODO: return window handle
    return nullptr;
}

void MacWindow::Show()
{
    if (!_visible)
    {
        InitSwapChain();
        if (_showAfterFirstPaint)
        {
            if (RenderTask)
                RenderTask->Enabled = true;
            return;
        }

        // Show
        // TODO: show window
        _focused = true;

        // Base
        WindowBase::Show();
    }
}

void MacWindow::Hide()
{
    if (_visible)
    {
        // Hide
        // TODO: hide window

        // Base
        WindowBase::Hide();
    }
}

void MacWindow::Minimize()
{
    // TODO: Minimize
}

void MacWindow::Maximize()
{
    // TODO: Maximize
}

void MacWindow::Restore()
{
    // TODO: Restore
}

bool MacWindow::IsClosed() const
{
    return false;
}

bool MacWindow::IsForegroundWindow() const
{
    return Platform::GetHasFocus();
}

void MacWindow::SetIsFullscreen(bool isFullscreen)
{
}

#endif
