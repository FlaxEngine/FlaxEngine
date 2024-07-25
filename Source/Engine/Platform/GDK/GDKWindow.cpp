// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if PLATFORM_GDK

#undef _GAMING_XBOX
#include "GDKWindow.h"
#include "GDKPlatform.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Graphics/GPUSwapChain.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"

extern void OnMainWindowCreated(HWND hWnd);

GDKWindow::GDKWindow(const CreateWindowSettings& settings)
    : WindowBase(settings)
{
    int32 x = Math::TruncToInt(settings.Position.X);
    int32 y = Math::TruncToInt(settings.Position.Y);
    int32 clientWidth = Math::TruncToInt(settings.Size.X);
    int32 clientHeight = Math::TruncToInt(settings.Size.Y);
    int32 windowWidth = clientWidth;
    int32 windowHeight = clientHeight;
    _clientSize = Float2((float)clientWidth, (float)clientHeight);

    // Setup window style
    uint32 style = WS_POPUP, exStyle = 0;
    if (settings.SupportsTransparency)
        exStyle |= WS_EX_LAYERED;
    if (!settings.ActivateWhenFirstShown)
        exStyle |= WS_EX_NOACTIVATE;
    if (settings.ShowInTaskbar)
        exStyle |= WS_EX_APPWINDOW;
    else
        exStyle |= WS_EX_TOOLWINDOW;
    if (settings.IsTopmost)
        exStyle |= WS_EX_TOPMOST;
    if (!settings.AllowInput)
        exStyle |= WS_EX_TRANSPARENT;
    if (settings.AllowMaximize)
        style |= WS_MAXIMIZEBOX;
    if (settings.AllowMinimize)
        style |= WS_MINIMIZEBOX;
    if (settings.HasSizingFrame)
        style |= WS_THICKFRAME;

    // Check if window should have a border
    if (settings.HasBorder)
    {
        // Create window style flags
        style |= WS_OVERLAPPED | WS_SYSMENU | WS_BORDER | WS_CAPTION;
        exStyle |= 0;

        // Adjust window size and positions to take into account window border
        RECT winRect = { 0, 0, clientWidth, clientHeight };
        AdjustWindowRectEx(&winRect, style, FALSE, exStyle);
        x += winRect.left;
        y += winRect.top;
        windowWidth = winRect.right - winRect.left;
        windowHeight = winRect.bottom - winRect.top;
    }
    else
    {
        // Create window style flags
        style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
        exStyle |= WS_EX_WINDOWEDGE;
    }

    // Creating the window
    _handle = CreateWindowExW(
        exStyle,
        Platform::ApplicationClassName,
        settings.Title.GetText(),
        style,
        x,
        y,
        windowWidth,
        windowHeight,
        settings.Parent ? static_cast<HWND>(settings.Parent->GetNativePtr()) : nullptr,
        nullptr,
        (HINSTANCE)Platform::Instance,
        nullptr);

    // Validate result
    if (!HasHWND())
    {
        LOG_WIN32_LAST_ERROR;
        Platform::Fatal(TEXT("Cannot create window."));
    }

    OnMainWindowCreated(_handle);
}

GDKWindow::~GDKWindow()
{
    if (HasHWND())
    {
        // Destroy window
        if (DestroyWindow(_handle) == 0)
        {
            LOG(Warning, "DestroyWindow failed! Error: {0:#x}", GetLastError());
        }

        // Clear
        _handle = nullptr;
        _visible = false;
    }
}

void* GDKWindow::GetNativePtr() const
{
    return _handle;
}

void GDKWindow::Show()
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

        ASSERT(HasHWND());

        // Show
        ShowWindow(_handle, (_settings.AllowInput && _settings.ActivateWhenFirstShown) ? SW_SHOW : SW_SHOWNA);

        // Base
        WindowBase::Show();

        _focused = true; // TODO: remove it and check if the initial focus works when game starts with rendering, do we get WM_ACTIVATEAPP?
    }
}

void GDKWindow::Hide()
{
    if (_visible)
    {
        ASSERT(HasHWND());

        // Hide
        ShowWindow(_handle, SW_HIDE);

        // Base
        WindowBase::Hide();
    }
}

void GDKWindow::Minimize()
{
    ASSERT(HasHWND());
    ShowWindow(_handle, SW_MINIMIZE);
}

void GDKWindow::Maximize()
{
    ASSERT(HasHWND());
    ShowWindow(_handle, SW_MAXIMIZE);
}

void GDKWindow::Restore()
{
    ASSERT(HasHWND());
    ShowWindow(_handle, SW_RESTORE);
}

bool GDKWindow::IsClosed() const
{
    return !HasHWND();
}

bool GDKWindow::IsForegroundWindow() const
{
    return Platform::GetHasFocus();
}

void GDKWindow::SetIsFullscreen(bool isFullscreen)
{
}

void GDKWindow::GetScreenInfo(int32& x, int32& y, int32& width, int32& height) const
{
    x = 0;
    y = 0;
    width = (int32)_clientSize.X;
    height = (int32)_clientSize.Y;
}

void GDKWindow::SetCursor(CursorType type)
{
    // Base
    WindowBase::SetCursor(type);

    UpdateCursor();
}

void GDKWindow::CheckForWindowResize()
{
    // Skip for minimized window (GetClientRect for minimized window returns 0)
    if (_minimized)
        return;

    ASSERT(HasHWND());

    // Cache client size
    RECT rect;
    GetClientRect(_handle, &rect);
    const int32 width = Math::Max(rect.right - rect.left, 0L);
    const int32 height = Math::Max(rect.bottom - rect.top, 0L);
    _clientSize = Float2(static_cast<float>(width), static_cast<float>(height));

    // Check if window size has been changed
    if (width > 0 && height > 0 && (_swapChain == nullptr || width != _swapChain->GetWidth() || height != _swapChain->GetHeight()))
    {
        OnResize(width, height);
    }
}

void GDKWindow::UpdateCursor() const
{
    if (_cursor == CursorType::Hidden)
    {
        ::SetCursor(nullptr);
        return;
    }

    int32 index = 0;
    switch (_cursor)
    {
    case CursorType::Default:
        break;
    case CursorType::Cross:
        index = 1;
        break;
    case CursorType::Hand:
        index = 2;
        break;
    case CursorType::Help:
        index = 3;
        break;
    case CursorType::IBeam:
        index = 4;
        break;
    case CursorType::No:
        index = 5;
        break;
    case CursorType::Wait:
        index = 11;
        break;
    case CursorType::SizeAll:
        index = 6;
        break;
    case CursorType::SizeNESW:
        index = 7;
        break;
    case CursorType::SizeNS:
        index = 8;
        break;
    case CursorType::SizeNWSE:
        index = 9;
        break;
    case CursorType::SizeWE:
        index = 10;
        break;
    }

    static const LPCWSTR cursors[] =
    {
        IDC_ARROW,
        IDC_CROSS,
        IDC_HAND,
        IDC_HELP,
        IDC_IBEAM,
        IDC_NO,
        IDC_SIZEALL,
        IDC_SIZENESW,
        IDC_SIZENS,
        IDC_SIZENWSE,
        IDC_SIZEWE,
        IDC_WAIT,
    };

    ASSERT(index >= 0 && index < ARRAY_COUNT(cursors));
    const HCURSOR cursor = LoadCursorW(nullptr, cursors[index]);
    ::SetCursor(cursor);
}

Windows::LRESULT GDKWindow::WndProc(Windows::UINT msg, Windows::WPARAM wParam, Windows::LPARAM lParam)
{
    switch (msg)
    {
    case WM_SETCURSOR:
    {
        if (LOWORD(lParam) == HTCLIENT)
        {
            UpdateCursor();
            return true;
        }
        break;
    }
    case WM_CREATE:
    {
        return 0;
    }
    case WM_SIZE:
    {
        if (SIZE_MINIMIZED == wParam)
        {
            // Set flags
            _minimized = true;
            _maximized = false;
        }
        else
        {
            RECT rcCurrentClient;
            GetClientRect(_handle, &rcCurrentClient);
            if (rcCurrentClient.top == 0 && rcCurrentClient.bottom == 0)
            {
                // Rapidly clicking the task bar to minimize and restore a window can cause a WM_SIZE message with SIZE_RESTORED when 
                // the window has actually become minimized due to rapid change so just ignore this message.
            }
            else if (SIZE_MAXIMIZED == wParam)
            {
                // Set flags
                _minimized = false;
                _maximized = true;

                // Check size
                CheckForWindowResize();
            }
            else if (SIZE_RESTORED == wParam)
            {
                if (_maximized)
                {
                    // Clear flag
                    _maximized = false;

                    // Check size
                    CheckForWindowResize();
                }
                else if (_minimized)
                {
                    // Clear flag
                    _minimized = false;

                    // Check size
                    CheckForWindowResize();
                }
                else
                {
                    // This WM_SIZE come from resizing the window via an API like SetWindowPos() so resize
                    CheckForWindowResize();
                }
            }
        }
        break;
    }
    case WM_SETFOCUS:
        OnGotFocus();
        break;
    case WM_KILLFOCUS:
        OnLostFocus();
        break;
    case WM_ACTIVATEAPP:
        if (wParam == TRUE && !_focused)
        {
            OnGotFocus();
        }
        else if (wParam == FALSE && _focused)
        {
            OnLostFocus();
        }
        break;
    case WM_CLOSE:
        Close(ClosingReason::User);
        return 0;
    case WM_DESTROY:
    {
        // Quit
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProc(_handle, msg, wParam, lParam);
}

#endif
