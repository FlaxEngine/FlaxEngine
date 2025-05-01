// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_ANDROID

#include "../Window.h"
#include "Engine/Graphics/RenderTask.h"
#include <Engine/Main/Android/android_native_app_glue.h>

AndroidWindow::AndroidWindow(const CreateWindowSettings& settings)
    : WindowBase(settings)
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

void AndroidWindow::SetClientBounds(const Rectangle& clientArea)
{
    if (Float2::NearEqual(_clientSize, clientArea.Size))
        return;

    const int32 width = static_cast<int32>(clientArea.GetWidth());
    const int32 height = static_cast<int32>(clientArea.GetHeight());
    _clientSize = clientArea.Size;
    OnResize(width, height);
}

#endif
