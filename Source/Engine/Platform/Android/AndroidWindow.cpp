// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if PLATFORM_ANDROID

#include "../Window.h"
#include "Engine/Graphics/RenderTask.h"
#include <Engine/Main/Android/android_native_app_glue.h>

#define DefaultDPI 96

AndroidWindow::AndroidWindow(const CreateWindowSettings& settings)
    : WindowBase(settings)
{
    _clientSize = settings.Size;
    _dpi = DefaultDPI;
    _dpiScale = (float)_dpi / (float)DefaultDPI;
}

AndroidWindow::~AndroidWindow()
{
}

void* AndroidWindow::GetNativePtr() const
{
    return Platform::GetApp()->window;
}

void AndroidWindow::Show()
{
    if (!_visible)
    {
        if (_showAfterFirstPaint)
        {
            if (RenderTask)
                RenderTask->Enabled = true;
            return;
        }

        // Base
        WindowBase::Show();

        _focused = Platform::GetHasFocus();
    }
}

void AndroidWindow::Hide()
{
    if (_visible)
    {
        // Base
        WindowBase::Hide();
    }
}

void AndroidWindow::Minimize()
{
}

void AndroidWindow::Maximize()
{
}

void AndroidWindow::Restore()
{
}

void AndroidWindow::BringToFront(bool force)
{
}

bool AndroidWindow::IsClosed() const
{
    return _isClosing;
}

void AndroidWindow::SetClientBounds(const Rectangle& clientArea)
{
    if (Vector2::NearEqual(_clientSize, clientArea.Size))
        return;

    const int32 width = static_cast<int32>(clientArea.GetWidth());
    const int32 height = static_cast<int32>(clientArea.GetHeight());
    _clientSize = clientArea.Size;
    OnResize(width, height);
}

void AndroidWindow::SetPosition(const Vector2& position)
{
}

void AndroidWindow::SetClientPosition(const Vector2& position)
{
}

Vector2 AndroidWindow::GetPosition() const
{
    return Vector2::Zero;
}

Vector2 AndroidWindow::GetSize() const
{
    return _clientSize;
}

Vector2 AndroidWindow::GetClientSize() const
{
    return _clientSize;
}

Vector2 AndroidWindow::ScreenToClient(const Vector2& screenPos) const
{
    return screenPos;
}

Vector2 AndroidWindow::ClientToScreen(const Vector2& clientPos) const
{
    return clientPos;
}

void AndroidWindow::SetTitle(const StringView& title)
{
    _title = title;
}

DragDropEffect AndroidWindow::DoDragDrop(const StringView& data)
{
    return DragDropEffect::None;
}

void AndroidWindow::StartTrackingMouse(bool useMouseScreenOffset)
{
}

void AndroidWindow::EndTrackingMouse()
{
}

#endif
