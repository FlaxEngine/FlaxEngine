// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_WINDOWS

#include "WindowsWindow.h"
#include "WindowsPlatform.h"
#include "WindowsInput.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Graphics/GPUSwapChain.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/GPUDevice.h"
#if USE_EDITOR
#include "Engine/Core/Collections/Array.h"
#include "Engine/Platform/IGuiData.h"
#include "Engine/Platform/Base/DragDropHelper.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Mouse.h"
#endif
#include "../Win32/IncludeWindowsHeaders.h"
#include <propidl.h>
#if USE_EDITOR
#include <oleidl.h>
#include <shellapi.h>
#endif

#define DefaultDPI 96

// Use improved borderless window support for Editor
#define WINDOWS_USE_NEW_BORDER_LESS USE_EDITOR && 0
#if WINDOWS_USE_NEW_BORDER_LESS
#pragma comment(lib, "Gdi32.lib")
WIN_API HRGN WIN_API_CALLCONV CreateRectRgn(int x1, int y1, int x2, int y2);
#endif
#define WINDOWS_USE_NEWER_BORDER_LESS USE_EDITOR && 1
#if WINDOWS_USE_NEWER_BORDER_LESS
#pragma comment(lib, "dwmapi.lib")
WIN_API HRESULT WIN_API_CALLCONV DwmExtendFrameIntoClientArea(HWND hWnd, const void* pMarInset);
WIN_API HRESULT WIN_API_CALLCONV DwmIsCompositionEnabled(BOOL* pfEnabled);

namespace
{
    bool IsCompositionEnabled()
    {
        BOOL result = FALSE;
        const bool success = ::DwmIsCompositionEnabled(&result) == S_OK;
        return result && success;
    }
}
#endif

namespace
{
    bool IsWindowMaximized(HWND window)
    {
        WINDOWPLACEMENT placement;
        if (!::GetWindowPlacement(window, &placement))
            return false;
        return placement.showCmd == SW_MAXIMIZE;
    }
}

WindowsWindow::WindowsWindow(const CreateWindowSettings& settings)
    : WindowBase(settings)
#if USE_EDITOR
    , _refCount(1)
#endif
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
#if WINDOWS_USE_NEW_BORDER_LESS
        if (settings.IsRegularWindow)
            style |= WS_BORDER | WS_CAPTION | WS_DLGFRAME | WS_SYSMENU | WS_THICKFRAME | WS_GROUP;
#elif WINDOWS_USE_NEWER_BORDER_LESS
        if (settings.IsRegularWindow)
            style |= WS_THICKFRAME | WS_SYSMENU | WS_CAPTION;
#endif
        exStyle |= WS_EX_WINDOWEDGE;
    }

    // Creating the window
    _handle = CreateWindowExW(
        exStyle,
        Platform::ApplicationWindowClass,
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
    if (_handle == nullptr)
    {
        LOG_WIN32_LAST_ERROR;
        Platform::Fatal(TEXT("Cannot create window."));
        return;
    }

    // Query DPI
    _dpi = Platform::GetDpi();
    const HMODULE user32Dll = LoadLibraryW(L"user32.dll");
    if (user32Dll)
    {
        typedef UINT (STDAPICALLTYPE*GetDpiForWindowProc)(HWND hwnd);
        const GetDpiForWindowProc getDpiForWindowProc = (GetDpiForWindowProc)GetProcAddress(user32Dll, "GetDpiForWindow");
        if (getDpiForWindowProc)
        {
            _dpi = getDpiForWindowProc(_handle);
        }
        FreeLibrary(user32Dll);
    }
    _dpiScale = (float)_dpi / (float)DefaultDPI;

#if WINDOWS_USE_NEWER_BORDER_LESS
    // Enable shadow
    if (_settings.IsRegularWindow && !_settings.HasBorder && IsCompositionEnabled())
    {
        const int margin[4] = { 1, 1, 1, 1 };
        ::DwmExtendFrameIntoClientArea(_handle, margin);
    }
#endif

#if USE_EDITOR
    // Enable file dropping
    if (_settings.AllowDragAndDrop)
    {
        const auto result = RegisterDragDrop(_handle, (LPDROPTARGET)(Windows::IDropTarget*)this);
        if (result != S_OK)
        {
            LOG(Warning, "Window drag and drop service error: 0x{0:x}:{1}", result, 1);
        }
    }
#endif

    UpdateRegion();
}

WindowsWindow::~WindowsWindow()
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

void* WindowsWindow::GetNativePtr() const
{
    return _handle;
}

void WindowsWindow::Show()
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
#if WINDOWS_USE_NEW_BORDER_LESS
        if (!_settings.HasBorder && _settings.IsRegularWindow)
        {
            SetWindowPos(_handle, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        }
#elif WINDOWS_USE_NEWER_BORDER_LESS
        if (!_settings.HasBorder)
        {
            SetWindowPos(_handle, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
        }
#endif

        // Base
        WindowBase::Show();
    }
}

void WindowsWindow::Hide()
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

void WindowsWindow::Minimize()
{
    if (!_settings.AllowMinimize)
        return;
    ASSERT(HasHWND());
    ShowWindow(_handle, SW_MINIMIZE);
}

void WindowsWindow::Maximize()
{
    if (!_settings.AllowMaximize)
        return;
    ASSERT(HasHWND());
    _isDuringMaximize = true;
    ShowWindow(_handle, SW_MAXIMIZE);
    _isDuringMaximize = false;
}

void WindowsWindow::SetBorderless(bool isBorderless, bool maximized)
{
    ASSERT(HasHWND());

    if (IsFullscreen())
        SetIsFullscreen(false);

    // Fixes issue of borderless window not going full screen
    if (IsMaximized())
        Restore();

    _settings.HasBorder = !isBorderless;

    BringToFront();

    if (isBorderless)
    {
        LONG lStyle = GetWindowLong(_handle, GWL_STYLE);
        lStyle &= ~(WS_THICKFRAME | WS_SYSMENU | WS_OVERLAPPED | WS_BORDER | WS_CAPTION);
        lStyle |= WS_POPUP;
        lStyle |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
#if WINDOWS_USE_NEW_BORDER_LESS
        if (_settings.IsRegularWindow)
            style |= WS_BORDER | WS_CAPTION | WS_DLGFRAME | WS_SYSMENU | WS_THICKFRAME | WS_GROUP;
#elif WINDOWS_USE_NEWER_BORDER_LESS
        if (_settings.IsRegularWindow)
            lStyle |= WS_THICKFRAME | WS_SYSMENU;
#endif

        SetWindowLong(_handle, GWL_STYLE, lStyle);
        SetWindowPos(_handle, HWND_TOP, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

        if (maximized)
        {
            ShowWindow(_handle, SW_SHOWMAXIMIZED);
        }
        else
        {
            ShowWindow(_handle, SW_SHOW);
        }
    }
    else
    {
        LONG lStyle = GetWindowLong(_handle, GWL_STYLE);
        lStyle &= ~(WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
        if (_settings.AllowMaximize)
            lStyle |= WS_MAXIMIZEBOX;
        if (_settings.AllowMinimize)
            lStyle |= WS_MINIMIZEBOX;
        if (_settings.HasSizingFrame)
            lStyle |= WS_THICKFRAME;
        lStyle |= WS_OVERLAPPED | WS_SYSMENU | WS_BORDER | WS_CAPTION;

        SetWindowLong(_handle, GWL_STYLE, lStyle);
        const Float2 clientSize = GetClientSize();
        const Float2 desktopSize = Platform::GetDesktopSize();
        // Move window and half size if it is larger than desktop size
        if (clientSize.X >= desktopSize.X && clientSize.Y >= desktopSize.Y)
        {
            const Float2 halfSize = desktopSize * 0.5f;
            const Float2 middlePos = halfSize * 0.5f;
            SetWindowPos(_handle, nullptr, (int)middlePos.X, (int)middlePos.Y, (int)halfSize.X, (int)halfSize.Y, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        else
        {
            SetWindowPos(_handle, nullptr, 0, 0, (int)clientSize.X, (int)clientSize.Y, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        if (maximized)
        {
            Maximize();
        }
        else
        {
            ShowWindow(_handle, SW_SHOW);
        }
    }

    CheckForWindowResize();
}

void WindowsWindow::Restore()
{
    ASSERT(HasHWND());
    ShowWindow(_handle, SW_RESTORE);
}

bool WindowsWindow::IsClosed() const
{
    return !HasHWND();
}

bool WindowsWindow::IsForegroundWindow() const
{
    return ::GetForegroundWindow() == _handle;
}

void WindowsWindow::BringToFront(bool force)
{
    ASSERT(HasHWND());

    if (_settings.IsRegularWindow)
    {
        if (IsIconic(_handle))
        {
            ShowWindow(_handle, SW_RESTORE);
        }
        else
        {
            SetActiveWindow(_handle);
        }
    }
    else
    {
        HWND hWndInsertAfter = HWND_TOP;
        uint32 flags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER;

        if (!force)
        {
            flags |= SWP_NOACTIVATE;
        }

        if (_settings.IsTopmost)
        {
            hWndInsertAfter = HWND_TOPMOST;
        }

        SetWindowPos(_handle, hWndInsertAfter, 0, 0, 0, 0, flags);
    }
}

void WindowsWindow::SetClientBounds(const Rectangle& clientArea)
{
    ASSERT(HasHWND());

    // Check if position or/and size will change
    const auto rect = GetClientBounds();
    const bool changeLocation = !Float2::NearEqual(rect.Location, clientArea.Location);
    const bool changeSize = !Float2::NearEqual(rect.Size, clientArea.Size);
    if (!changeLocation && !changeSize)
        return;

    // Get values data
    int32 x = (int32)clientArea.GetX();
    int32 y = (int32)clientArea.GetY();
    int32 width = (int32)clientArea.GetWidth();
    int32 height = (int32)clientArea.GetHeight();

    if (changeSize)
    {
        _clientSize = clientArea.Size;

        // Update GUI
        OnResize(width, height);
    }

    // Check if need to adjust window rectangle
    if (_settings.HasBorder)
    {
        // Get window info
        WINDOWINFO winInfo;
        Platform::MemoryClear(&winInfo, sizeof(WINDOWINFO));
        winInfo.cbSize = sizeof(winInfo);
        GetWindowInfo(_handle, &winInfo);

        // Adjust rectangle from client size to window size
        RECT winRect = { 0, 0, width, height };
        AdjustWindowRectEx(&winRect, winInfo.dwStyle, FALSE, winInfo.dwExStyle);
        width = winRect.right - winRect.left;
        height = winRect.bottom - winRect.top;

        // Little hack but works great
        winRect = { x, y, width, height };
        AdjustWindowRectEx(&winRect, winInfo.dwStyle, FALSE, winInfo.dwExStyle);
        x = winRect.left;
        y = winRect.top;
    }

    // Change window size and location
    SetWindowPos(_handle, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);

    if (changeSize)
    {
        UpdateRegion();
    }
}

void WindowsWindow::SetPosition(const Float2& position)
{
    ASSERT(HasHWND());

    // Cache data
    int32 x = (int32)position.X;
    int32 y = (int32)position.Y;

    // Change window location
    SetWindowPos(_handle, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

void WindowsWindow::SetClientPosition(const Float2& position)
{
    ASSERT(HasHWND());

    // Cache data
    int32 x = (int32)position.X;
    int32 y = (int32)position.Y;

    // Check if need to adjust window rectangle
    if (_settings.HasBorder)
    {
        // Get window info
        WINDOWINFO winInfo;
        Platform::MemoryClear(&winInfo, sizeof(WINDOWINFO));
        winInfo.cbSize = sizeof(winInfo);
        GetWindowInfo(_handle, &winInfo);

        // Adjust rectangle from client size to window size
        RECT winRect = { x, y, 666, 69 };
        AdjustWindowRectEx(&winRect, winInfo.dwStyle, FALSE, winInfo.dwExStyle);

        // Calculate window size
        x = winRect.left;
        y = winRect.top;
    }

    // Change window location
    SetWindowPos(_handle, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

void WindowsWindow::SetIsFullscreen(bool isFullscreen)
{
    _isSwitchingFullScreen = true;

    ASSERT(HasHWND());

    // Base
    WindowBase::SetIsFullscreen(isFullscreen);

    if (!isFullscreen)
    {
        // Restore window
        ShowWindow(_handle, SW_NORMAL);
    }

    _isSwitchingFullScreen = false;
}

Float2 WindowsWindow::GetPosition() const
{
    ASSERT(HasHWND());

    RECT rect;
    GetWindowRect(_handle, &rect);
    return Float2(static_cast<float>(rect.left), static_cast<float>(rect.top));
}

Float2 WindowsWindow::GetSize() const
{
    ASSERT(HasHWND());

    RECT rect;
    GetWindowRect(_handle, &rect);
    return Float2(static_cast<float>(rect.right - rect.left), static_cast<float>(rect.bottom - rect.top));
}

Float2 WindowsWindow::GetClientSize() const
{
    return _clientSize;
}

Float2 WindowsWindow::ScreenToClient(const Float2& screenPos) const
{
    ASSERT(HasHWND());

    POINT p;
    p.x = static_cast<LONG>(screenPos.X);
    p.y = static_cast<LONG>(screenPos.Y);
    ::ScreenToClient(_handle, &p);
    return Float2(static_cast<float>(p.x), static_cast<float>(p.y));
}

Float2 WindowsWindow::ClientToScreen(const Float2& clientPos) const
{
    ASSERT(HasHWND());

    if (_minimized)
    {
        // Return cached position when window is not on screen
        return _minimizedScreenPosition + clientPos;
    }

    POINT p;
    p.x = static_cast<LONG>(clientPos.X);
    p.y = static_cast<LONG>(clientPos.Y);
    ::ClientToScreen(_handle, &p);
    return Float2(static_cast<float>(p.x), static_cast<float>(p.y));
}

void WindowsWindow::FlashWindow()
{
    ASSERT(HasHWND());

    if (IsFocused())
        return;

    ::FlashWindow(_handle, FALSE);
}

void WindowsWindow::GetScreenInfo(int32& x, int32& y, int32& width, int32& height) const
{
    ASSERT(HasHWND());

    // Pick the current monitor data for sizing
    const HMONITOR monitor = MonitorFromWindow(_handle, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFO);
    GetMonitorInfoW(monitor, &monitorInfo);

    // Calculate result
    x = monitorInfo.rcMonitor.left;
    y = monitorInfo.rcMonitor.top;
    width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
    height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
}

float WindowsWindow::GetOpacity() const
{
    ASSERT(HasHWND());

    COLORREF color;
    BYTE alpha;
    DWORD flags;
    GetLayeredWindowAttributes(_handle, &color, &alpha, &flags);
    return (float)alpha / 255.0f;
}

void WindowsWindow::SetOpacity(const float opacity)
{
    ASSERT(HasHWND());

    SetLayeredWindowAttributes(_handle, 0, static_cast<BYTE>(Math::Saturate(opacity) * 255), LWA_ALPHA);
}

void WindowsWindow::Focus()
{
    ASSERT(HasHWND());
    if (GetFocus() != _handle)
    {
        SetFocus(_handle);
    }
}

void WindowsWindow::SetTitle(const StringView& title)
{
    ASSERT(HasHWND());

    if (SetWindowTextW(_handle, *title))
    {
        _title = title;
    }
}

void WindowsWindow::StartTrackingMouse(bool useMouseScreenOffset)
{
    ASSERT(HasHWND());

    if (!_isTrackingMouse)
    {
        _isTrackingMouse = true;
        _trackingMouseOffset = Float2::Zero;
        _isUsingMouseOffset = useMouseScreenOffset;
        _isHorizontalFlippingMouse = false;
        _isVerticalFlippingMouse = false;

        int32 x = 0, y = 0, width = 0, height = 0;
        GetScreenInfo(x, y, width, height);
        _mouseOffsetScreenSize = Rectangle((float)x, (float)y, (float)width, (float)height);

        SetCapture(_handle);
    }
}

void WindowsWindow::EndTrackingMouse()
{
    if (_isTrackingMouse)
    {
        _isTrackingMouse = false;
        _isHorizontalFlippingMouse = false;
        _isVerticalFlippingMouse = false;

        ReleaseCapture();
    }
}

void WindowsWindow::StartClippingCursor(const Rectangle& bounds)
{
    _isClippingCursor = true;
    *(RECT*)_clipCursorRect = {
        (LONG)bounds.GetUpperLeft().X,
        (LONG)bounds.GetUpperLeft().Y,
        (LONG)bounds.GetBottomRight().X,
        (LONG)bounds.GetBottomRight().Y
    };
    if (IsFocused())
    {
        _clipCursorSet = true;
        ClipCursor((RECT*)_clipCursorRect);
    }
}

void WindowsWindow::EndClippingCursor()
{
    if (_isClippingCursor)
    {
        _isClippingCursor = false;
        _clipCursorSet = false;
        ClipCursor(nullptr);
    }
}

void WindowsWindow::SetCursor(CursorType type)
{
    // Base
    WindowBase::SetCursor(type);

    UpdateCursor();
}

#if USE_EDITOR

Windows::HRESULT WindowsWindow::QueryInterface(const Windows::IID& id, void** ppvObject)
{
    // Check to see what interface has been requested
    if ((const IID&)id == IID_IUnknown || (const IID&)id == IID_IDropTarget)
    {
        AddRef();
        *ppvObject = this;
        return S_OK;
    }

    // No interface
    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

Windows::ULONG WindowsWindow::AddRef()
{
    _InterlockedIncrement(&_refCount);
    return _refCount;
}

Windows::ULONG WindowsWindow::Release()
{
    return _InterlockedDecrement(&_refCount);
}

#endif

void WindowsWindow::CheckForWindowResize()
{
    // Skip for minimized window (GetClientRect for minimized window returns 0)
    if (_minimized)
        return;

    ASSERT(HasHWND());

    // Cache client size
    RECT rect;
    GetClientRect(_handle, &rect);
    int32 width = Math::Max(rect.right - rect.left, 0L);
    int32 height = Math::Max(rect.bottom - rect.top, 0L);

    // Check for windows maximized size and see if it needs to adjust position if needed
    if (_maximized)
    {
        // Pick the current monitor data for sizing
        const HMONITOR monitor = MonitorFromWindow(_handle, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo;
        monitorInfo.cbSize = sizeof(MONITORINFO);
        GetMonitorInfoW(monitor, &monitorInfo);

        auto cwidth = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
        auto cheight = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;
        if (width > cwidth && height > cheight)
        {
            width = cwidth;
            height = cheight;
            SetWindowPos(_handle, HWND_TOP, monitorInfo.rcWork.left, monitorInfo.rcWork.top, width, height, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        }
    }
    _clientSize = Float2(static_cast<float>(width), static_cast<float>(height));

    // Check if window size has been changed
    if (width > 0 && height > 0 && (_swapChain == nullptr || width != _swapChain->GetWidth() || height != _swapChain->GetHeight()))
    {
        UpdateRegion();
        OnResize(width, height);
    }
}

void WindowsWindow::UpdateCursor()
{
    // Don't hide cursor when window is not focused
    if (_cursor == CursorType::Hidden && _focused)
    {
        if (!_lastCursorHidden)
        {
            _lastCursorHidden = true;
            while(::ShowCursor(FALSE) >= 0)
            {
                if (_cursorHiddenSafetyCount >= 100)
                {
                    LOG(Warning, "Cursor has failed to hide.");
                    break;
                }
                _cursorHiddenSafetyCount += 1;
            }
            _cursorHiddenSafetyCount = 0;
        }
        ::SetCursor(nullptr);
        return;
    }
    else if (_lastCursorHidden)
    {
        _lastCursorHidden = false;
        while(::ShowCursor(TRUE) < 0)
        {
            if (_cursorHiddenSafetyCount >= 100)
            {
                LOG(Warning, "Cursor has failed to show.");
                break;
            }
            _cursorHiddenSafetyCount += 1;
        }
        _cursorHiddenSafetyCount = 0;
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

void WindowsWindow::UpdateRegion()
{
#if WINDOWS_USE_NEW_BORDER_LESS
    // Use region to remove rounded corners of the window
    if (!_settings.HasBorder && _settings.IsRegularWindow)
    {
        if (!_maximized && !_isResizing)
        {
            RECT rcWnd;
            GetWindowRect(_handle, &rcWnd);
            const int32 width = rcWnd.right - rcWnd.left;
            const int32 height = rcWnd.bottom - rcWnd.top;
            if (_regionWidth != width || _regionHeight != height)
            {
                _regionWidth = width;
                _regionHeight = height;
                const HRGN region = CreateRectRgn(0, 0, width, height);
                SetWindowRgn(_handle, region, FALSE);
            }
        }
        else if (_regionWidth != 0 || _regionHeight != 0)
        {
            _regionWidth = 0;
            _regionHeight = 0;
            SetWindowRgn(_handle, nullptr, FALSE);
        }
    }
#endif
}

void TrackMouse(HWND hwnd)
{
    TRACKMOUSEEVENT tme;
    tme.cbSize = sizeof(TRACKMOUSEEVENT);
    tme.dwFlags = TME_HOVER | TME_LEAVE;
    tme.dwHoverTime = 5000;
    tme.hwndTrack = hwnd;
    TrackMouseEvent(&tme);
}

LRESULT WindowsWindow::WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    const UINT_PTR MouseStopTimerID = 1;
    switch (msg)
    {
    case WM_PAINT:
    {
        // Check if window is during resizing
        if (_isResizing && _swapChain)
        {
            // Redraw window backbuffer on DX11
            switch (GPUDevice::Instance->GetRendererType())
            {
            case RendererType::DirectX10:
            case RendererType::DirectX10_1:
            case RendererType::DirectX11:
                _swapChain->Present(false);
                break;
            }
        }
        break;
    }
    case WM_TIMER:
    {
        if (wParam == MouseStopTimerID)
        {
            // Kill the timer after processing it
            KillTimer(_handle, MouseStopTimerID);
            return 0;
        }
        break;
    }
    case WM_SETCURSOR:
    {
        if (LOWORD(lParam) == HTCLIENT)
        {
            UpdateCursor();
            return true;
        }
        break;
    }
    case WM_MOUSEMOVE:
    {
        if (!_trackingMouse)
        {
            TrackMouse(_handle);
            _trackingMouse = true;
        }

        if (_isTrackingMouse)
        {
            KillTimer(_handle, MouseStopTimerID);
            SetTimer(_handle, MouseStopTimerID, 100, nullptr);
        }

        // Here we can transfer mouse pointer over virtual workspace
        if (_isTrackingMouse && _isUsingMouseOffset)
        {
            // Check if move mouse to another edge of the desktop
            Float2 desktopLocation = _mouseOffsetScreenSize.Location;
            Float2 desktopSize = _mouseOffsetScreenSize.GetBottomRight();

            const Float2 mousePos(static_cast<float>(WINDOWS_GET_X_LPARAM(lParam)), static_cast<float>(WINDOWS_GET_Y_LPARAM(lParam)));
            Float2 mousePosition = ClientToScreen(mousePos);
            Float2 newMousePosition = mousePosition;
            if (_isHorizontalFlippingMouse = mousePosition.X <= desktopLocation.X + 2)
                newMousePosition.X = desktopSize.X - 3;
            else if (_isHorizontalFlippingMouse = mousePosition.X >= desktopSize.X - 1)
                newMousePosition.X = desktopLocation.X + 3;
            if (_isVerticalFlippingMouse = mousePosition.Y <= desktopLocation.Y + 2)
                newMousePosition.Y = desktopSize.Y - 3;
            else if (_isVerticalFlippingMouse = mousePosition.Y >= desktopSize.Y - 1)
                newMousePosition.Y = desktopLocation.Y + 3;
            if (!Float2::NearEqual(mousePosition, newMousePosition))
            {
                _trackingMouseOffset -= newMousePosition - mousePosition;
                SetMousePosition(ScreenToClient(newMousePosition));
            }
        }

        break;
    }
    case WM_MOUSELEAVE:
    {
        _trackingMouse = false;
        break;
    }
    case WM_NCCALCSIZE:
    {
#if WINDOWS_USE_NEW_BORDER_LESS
        if (wParam && !_settings.HasBorder && _settings.IsRegularWindow)
        {
            if (_maximized)
            {
                WINDOWINFO winInfo = { 0 };
                winInfo.cbSize = sizeof(winInfo);
                ::GetWindowInfo(_handle, &winInfo);

                LPNCCALCSIZE_PARAMS rects = (LPNCCALCSIZE_PARAMS)lParam;

                rects->rgrc[0].left += winInfo.cxWindowBorders;
                rects->rgrc[0].top += winInfo.cxWindowBorders;
                rects->rgrc[0].right -= winInfo.cxWindowBorders;
                rects->rgrc[0].bottom -= winInfo.cxWindowBorders;

                rects->rgrc[1].left = rects->rgrc[0].left;
                rects->rgrc[1].top = rects->rgrc[0].top;
                rects->rgrc[1].right = rects->rgrc[0].right;
                rects->rgrc[1].bottom = rects->rgrc[0].bottom;

                rects->lppos->x += winInfo.cxWindowBorders;
                rects->lppos->y += winInfo.cxWindowBorders;
                rects->lppos->cx -= 2 * winInfo.cxWindowBorders;
                rects->lppos->cy -= 2 * winInfo.cxWindowBorders;

                return WVR_VALIDRECTS;
            }
            else
            {
                SetWindowPos(_handle, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
                return 0;
            }
        }
#elif WINDOWS_USE_NEWER_BORDER_LESS
        if (wParam == TRUE && !_settings.HasBorder) // && _settings.IsRegularWindow)
        {
            // In maximized mode fill the whole work area of the monitor (excludes task bar)
            if (IsWindowMaximized(_handle))
            {
                HMONITOR monitor = ::MonitorFromWindow(_handle, MONITOR_DEFAULTTONULL);
                if (monitor)
                {
                    MONITORINFO monitorInfo;
                    monitorInfo.cbSize = sizeof(monitorInfo);
                    if (::GetMonitorInfoW(monitor, &monitorInfo))
                    {
                        LPNCCALCSIZE_PARAMS rects = (LPNCCALCSIZE_PARAMS)lParam;
                        rects->rgrc[0] = monitorInfo.rcWork;
                    }
                }
            }
            //SetWindowPos(_handle, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
            return 0;
        }
#endif
        break;
    }
    case WM_NCHITTEST:
    {
        // Override it for fullscreen mode
        if (IsFullscreen())
            return static_cast<int32>(WindowHitCodes::Client);

        const Float2 mouse(static_cast<float>(WINDOWS_GET_X_LPARAM(lParam)), static_cast<float>(WINDOWS_GET_Y_LPARAM(lParam)));
        WindowHitCodes hit = WindowHitCodes::Client;
        bool handled = false;
        OnHitTest(mouse, hit, handled);
        if (handled)
            return static_cast<int32>(hit);
        break;
    }
    case WM_NCLBUTTONDOWN:
    {
        bool result = false;
        OnLeftButtonHit(static_cast<WindowHitCodes>(wParam), result);
        if (result)
            return 0;
        break;
    }
    case WM_NCLBUTTONDBLCLK:
    {
        // Handle non-client area double click manually
        if (IsMaximized())
            Restore();
        else
            Maximize();
        return 0;
    }
#if WINDOWS_USE_NEWER_BORDER_LESS
    case WM_NCACTIVATE:
    {
        // Skip for border-less windows
        if (!_settings.HasBorder && !IsCompositionEnabled())
            return 1;
        break;
    }
#endif
#if 0
    case WM_NCPAINT:
        // Skip for border-less windows
        if (!_settings.HasBorder)
            return 1;
        break;
#endif
    case WM_ERASEBKGND:
        // Skip the window background erasing
        return 1;
    case WM_GETMINMAXINFO:
    {
        const auto minMax = reinterpret_cast<MINMAXINFO*>(lParam);
        minMax->ptMinTrackSize.x = (int32)_settings.MinimumSize.X;
        minMax->ptMinTrackSize.y = (int32)_settings.MinimumSize.Y;
        if (_settings.MaximumSize.SumValues() > 0)
        {
            int32 borderWidth = 0, borderHeight = 0;
            if (_settings.HasBorder)
            {
                const DWORD windowStyle = GetWindowLongW(_handle, GWL_STYLE);
                const DWORD windowExStyle = GetWindowLongW(_handle, GWL_EXSTYLE);
                RECT borderRect = { 0, 0, 0, 0 };
                AdjustWindowRectEx(&borderRect, windowStyle, false, windowExStyle);
                borderWidth = borderRect.right - borderRect.left;
                borderHeight = borderRect.bottom - borderRect.top;
            }
            minMax->ptMaxTrackSize.x = (int32)_settings.MaximumSize.X + borderWidth;
            minMax->ptMaxTrackSize.y = (int32)_settings.MaximumSize.Y + borderHeight;
        }

        // Include Windows task bar size into maximized tool window
        WINDOWPLACEMENT e;
        if (!IsFullscreen() && ((GetWindowPlacement(_handle, &e) && (e.showCmd == SW_SHOWMAXIMIZED || e.showCmd == SW_SHOWMINIMIZED)) || _isDuringMaximize))
        {
            // Adjust the maximized size and position to fit the work area of the correct monitor
            const HMONITOR monitor = MonitorFromWindow(_handle, MONITOR_DEFAULTTONEAREST);
            if (monitor != nullptr)
            {
                MONITORINFO monitorInfo;
                monitorInfo.cbSize = sizeof(MONITORINFO);
                if (::GetMonitorInfoW(monitor, &monitorInfo))
                {
                    minMax->ptMaxPosition.x = Math::Abs(monitorInfo.rcWork.left - monitorInfo.rcMonitor.left);
                    minMax->ptMaxPosition.y = Math::Abs(monitorInfo.rcWork.top - monitorInfo.rcMonitor.top);
                    minMax->ptMaxSize.x = Math::Abs(monitorInfo.rcWork.right - monitorInfo.rcWork.left);
                    minMax->ptMaxSize.y = Math::Abs(monitorInfo.rcWork.bottom - monitorInfo.rcWork.top);
                }
            }
        }

        return 0;
    }
    case WM_SYSCOMMAND:
        // Prevent moving/sizing in full screen mode
        if (IsFullscreen())
        {
            switch ((wParam & 0xFFF0))
            {
            case SC_MOVE:
            case SC_SIZE:
            case SC_MAXIMIZE:
            case SC_KEYMENU:
                return 0;
            }
        }
        break;
    case WM_CREATE:
        return 0;
    case WM_SIZE:
    {
        if (SIZE_MINIMIZED == wParam)
        {
            // Get the minimized window position in workspace coordinates
            WINDOWPLACEMENT placement;
            placement.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(_handle, &placement);

            // Calculate client offsets from window borders and title bar
            RECT winRect = { 0, 0, static_cast<LONG>(_clientSize.X), static_cast<LONG>(_clientSize.Y) };
            LONG style = GetWindowLong(_handle, GWL_STYLE);
            LONG exStyle = GetWindowLong(_handle, GWL_EXSTYLE);
            AdjustWindowRectEx(&winRect, style, FALSE, exStyle);

            // Calculate monitor offsets from taskbar position
            const HMONITOR monitor = MonitorFromWindow(_handle, MONITOR_DEFAULTTONEAREST);
            MONITORINFO monitorInfo;
            monitorInfo.cbSize = sizeof(MONITORINFO);
            GetMonitorInfoW(monitor, &monitorInfo);

            // Convert the workspace coordinates to screen space and store it
            _minimizedScreenPosition = Float2(
                static_cast<float>(placement.rcNormalPosition.left + monitorInfo.rcWork.left - monitorInfo.rcMonitor.left - winRect.left),
                static_cast<float>(placement.rcNormalPosition.top + monitorInfo.rcWork.top - monitorInfo.rcMonitor.top - winRect.top));

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
                _minimized = false;
                _maximized = true;
                CheckForWindowResize();
                UpdateRegion();
            }
            else if (SIZE_RESTORED == wParam)
            {
                if (_maximized)
                {
                    _maximized = false;
                    CheckForWindowResize();
                    UpdateRegion();
                }
                else if (_minimized)
                {
                    _minimized = false;
                    CheckForWindowResize();
                }
                else if (_isResizing)
                {
                    // If we're neither maximized nor minimized, the window size is changing by the user dragging the window edges.
                    // In this case, we don't resize yet -- we wait until the user stops dragging, and a WM_EXITSIZEMOVE message comes.
                    UpdateRegion();
                }
                else if (_isSwitchingFullScreen)
                {
                    // Ignored
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
    case WM_DPICHANGED:
    {
        // Maybe https://stackoverflow.com/a/45110656
        _dpi = HIWORD(wParam);
        _dpiScale = (float)_dpi / (float)DefaultDPI;
        RECT* windowRect = (RECT*)lParam;
        SetWindowPos(_handle, nullptr, windowRect->left, windowRect->top, windowRect->right - windowRect->left, windowRect->bottom - windowRect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        // TODO: Recalculate fonts
        return 0;
    }
    case WM_ENTERSIZEMOVE:
    {
        _isResizing = true;
        break;
    }
    case WM_EXITSIZEMOVE:
        _isResizing = false;
        CheckForWindowResize();
        UpdateRegion();
        break;
    case WM_SETFOCUS:
        OnGotFocus();
        UpdateCursor();
        if (_isClippingCursor && !_clipCursorSet)
        {
            _clipCursorSet = true;
            ClipCursor((RECT*)_clipCursorRect);
        }
        break;
    case WM_KILLFOCUS:
        if (_clipCursorSet)
        {
            _clipCursorSet = false;
            ClipCursor(nullptr);
        }
        OnLostFocus();
        UpdateCursor();
        break;
    case WM_ACTIVATEAPP:
        if (wParam == TRUE && !_focused)
        {
            OnGotFocus();
        }
        else if (wParam == FALSE && _focused)
        {
            OnLostFocus();
            if (IsFullscreen() && !_isSwitchingFullScreen)
            {
                SetIsFullscreen(false);
            }
        }
        UpdateCursor();
        break;
    case WM_MENUCHAR:
        // A menu is active and the user presses a key that does not correspond to any mnemonic or accelerator key so just ignore and don't beep
        return MAKELRESULT(0, MNC_CLOSE);
    case WM_SYSKEYDOWN:
        if (wParam == VK_F4)
        {
            LOG(Info, "Alt+F4 pressed");
            Close(ClosingReason::User);
            return 0;
        }
        if (wParam == VK_RETURN)
        {
            LOG(Info, "Alt+Enter pressed");
            SetIsFullscreen(!IsFullscreen());
            return 0;
        }
        break;
    case WM_POWERBROADCAST:
        switch (wParam)
        {
        case PBT_APMQUERYSUSPEND:
            // TODO: add OnSystemSuspend event
            return true;
        case PBT_APMRESUMESUSPEND:
            // TODO: add OnSystemResume event
            return true;
        }
        break;
    case WM_CLOSE:
        Close(ClosingReason::User);
        return 0;
    case WM_DESTROY:
    {
#if USE_EDITOR
        // Disable file dropping
        if (_settings.AllowDragAndDrop)
        {
            const auto result = RevokeDragDrop(_handle);
            if (result != S_OK)
                LOG(Warning, "Window drag and drop service error: 0x{0:x}:{1}", result, 2);
        }
#endif

        // Quit
        PostQuitMessage(0);
        return 0;
    }
    }

    if (_settings.AllowInput)
    {
        if (WindowsInput::WndProc(this, msg, wParam, lParam))
            return true;
    }

    return DefWindowProc(_handle, msg, wParam, lParam);
}

#if USE_EDITOR

HGLOBAL duplicateGlobalMem(HGLOBAL hMem)
{
    auto len = GlobalSize(hMem);
    auto source = GlobalLock(hMem);
    auto dest = GlobalAlloc(GMEM_FIXED, len);
    Platform::MemoryCopy(dest, source, len);
    GlobalUnlock(hMem);
    return dest;
}

DWORD dropEffect2OleEnum(DragDropEffect effect)
{
    DWORD result;
    switch (effect)
    {
    case DragDropEffect::None:
        result = DROPEFFECT_NONE;
        break;
    case DragDropEffect::Copy:
        result = DROPEFFECT_COPY;
        break;
    case DragDropEffect::Move:
        result = DROPEFFECT_MOVE;
        break;
    case DragDropEffect::Link:
        result = DROPEFFECT_LINK;
        break;
    default:
        result = DROPEFFECT_NONE;
        break;
    }
    return result;
}

DragDropEffect dropEffectFromOleEnum(DWORD effect)
{
    DragDropEffect result;
    switch (effect)
    {
    case DROPEFFECT_NONE:
        result = DragDropEffect::None;
        break;
    case DROPEFFECT_COPY:
        result = DragDropEffect::Copy;
        break;
    case DROPEFFECT_MOVE:
        result = DragDropEffect::Move;
        break;
    case DROPEFFECT_LINK:
        result = DragDropEffect::Link;
        break;
    default:
        result = DragDropEffect::None;
        break;
    }
    return result;
}

HANDLE StringToHandle(const StringView& str)
{
    // Allocate and lock a global memory buffer.
    // Make it fixed data so we don't have to use GlobalLock
    const int32 length = str.Length();
    char* ptr = static_cast<char*>(GlobalAlloc(GMEM_FIXED, length + 1));

    // Copy the string into the buffer as ANSI text
    StringUtils::ConvertUTF162ANSI(str.Get(), ptr, length);
    ptr[length] = '\0';

    return ptr;
}

void DeepCopyFormatEtc(FORMATETC* dest, FORMATETC* source)
{
    // Copy the source FORMATETC into dest
    *dest = *source;

    if (source->ptd)
    {
        // Allocate memory for the DVTARGETDEVICE if necessary
        dest->ptd = static_cast<DVTARGETDEVICE*>(CoTaskMemAlloc(sizeof(DVTARGETDEVICE)));

        // Copy the contents of the source DVTARGETDEVICE into dest->ptd
        *(dest->ptd) = *(source->ptd);
    }
}

HRESULT CreateEnumFormatEtc(UINT nNumFormats, FORMATETC* pFormatEtc, IEnumFORMATETC** ppEnumFormatEtc);

/// <summary>
/// GUI data for Windows platform
/// </summary>
class WindowsGuiData : public IGuiData
{
private:
    Type _type;
    Array<String> _data;

public:
    /// <summary>
    /// Init
    /// </summary>
    WindowsGuiData()
        : _type(Type::Unknown)
        , _data(1)
    {
    }

public:
    /// <summary>
    /// Init from Ole IDataObject
    /// </summary>
    /// <param name="pDataObj">Object</param>
    void Init(IDataObject* pDataObj)
    {
        // Temporary data
        FORMATETC fmtetc;
        STGMEDIUM stgmed;

        // Clear
        _type = Type::Unknown;
        _data.Clear();

        // Check type
        fmtetc = { CF_TEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        if (pDataObj->GetData(&fmtetc, &stgmed) == S_OK)
        {
            // Text
            _type = Type::Text;

            // Get data
            char* text = static_cast<char*>(GlobalLock(stgmed.hGlobal));
            _data.Add(String(text));
            GlobalUnlock(stgmed.hGlobal);
            ReleaseStgMedium(&stgmed);
        }
        else
        {
            fmtetc = { CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
            if (pDataObj->GetData(&fmtetc, &stgmed) == S_OK)
            {
                // Unicode Text
                _type = Type::Text;

                // Get data
                Char* text = static_cast<Char*>(GlobalLock(stgmed.hGlobal));
                _data.Add(String(text));
                GlobalUnlock(stgmed.hGlobal);
                ReleaseStgMedium(&stgmed);
            }
            else
            {
                fmtetc = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
                if (pDataObj->GetData(&fmtetc, &stgmed) == S_OK)
                {
                    // Files
                    _type = Type::Files;

                    // Get data
                    Char item[MAX_PATH];
                    HDROP hdrop = static_cast<HDROP>(GlobalLock(stgmed.hGlobal));
                    UINT filesCount = DragQueryFileW(hdrop, 0xFFFFFFFF, nullptr, 0);
                    for (UINT i = 0; i < filesCount; i++)
                    {
                        if (DragQueryFileW(hdrop, i, item, MAX_PATH) != 0)
                        {
                            _data.Add(String(item));
                        }
                    }
                    GlobalUnlock(stgmed.hGlobal);
                    ReleaseStgMedium(&stgmed);
                }
            }
        }
    }

public:
    // [IGuiData]
    Type GetType() const override
    {
        return _type;
    }
    String GetAsText() const override
    {
        String result;
        if (_type == Type::Text)
        {
            result = _data[0];
        }
        return result;
    }
    void GetAsFiles(Array<String>* files) const override
    {
        if (_type == Type::Files)
        {
            files->Add(_data);
        }
    }
};

/// <summary>
/// Tool class for Windows Ole support
/// </summary>
class WindowsEnumFormatEtc : public IEnumFORMATETC
{
private:
    ULONG _refCount;
    ULONG _index;
    ULONG _formatsCount;
    FORMATETC* _formatEtc;

public:
    WindowsEnumFormatEtc(FORMATETC* pFormatEtc, int32 nNumFormats)
        : _refCount(1)
        , _index(0)
        , _formatsCount(nNumFormats)
        , _formatEtc(nullptr)
    {
        // Allocate memory
        _formatEtc = new FORMATETC[nNumFormats];

        // Copy the FORMATETC structures
        for (int32 i = 0; i < nNumFormats; i++)
        {
            DeepCopyFormatEtc(&_formatEtc[i], &pFormatEtc[i]);
        }
    }

    ~WindowsEnumFormatEtc()
    {
        if (_formatEtc)
        {
            for (uint32 i = 0; i < _formatsCount; i++)
            {
                if (_formatEtc[i].ptd)
                {
                    CoTaskMemFree(_formatEtc[i].ptd);
                }
            }

            delete[] _formatEtc;
        }
    }

public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR * ppvObject) override
    {
        // Check to see what interface has been requested
        if (riid == IID_IEnumFORMATETC || riid == IID_IUnknown)
        {
            AddRef();
            *ppvObject = this;
            return S_OK;
        }

        // No interface
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override
    {
        _InterlockedIncrement(&_refCount);
        return _refCount;
    }
    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG ulRefCount = _InterlockedDecrement(&_refCount);
        if (_refCount == 0)
        {
            delete this;
        }
        return ulRefCount;
    }

    // [IEnumFormatEtc]
    HRESULT STDMETHODCALLTYPE Next(ULONG celt, FORMATETC* pFormatEtc, ULONG* pceltFetched) override
    {
        ULONG copied = 0;

        // validate arguments
        if (celt == 0 || pFormatEtc == nullptr)
            return E_INVALIDARG;

        // copy FORMATETC structures into caller's buffer
        while (_index < _formatsCount && copied < celt)
        {
            DeepCopyFormatEtc(&pFormatEtc[copied], &_formatEtc[_index]);
            copied++;
            _index++;
        }

        // store result
        if (pceltFetched != nullptr)
            *pceltFetched = copied;

        // did we copy all that was requested?
        return (copied == celt) ? S_OK : S_FALSE;
    }
    HRESULT STDMETHODCALLTYPE Skip(ULONG celt) override
    {
        _index += celt;
        return (_index <= _formatsCount) ? S_OK : S_FALSE;
    }
    HRESULT STDMETHODCALLTYPE Reset() override
    {
        _index = 0;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Clone(IEnumFORMATETC** ppEnumFormatEtc) override
    {
        HRESULT result;

        // Make a duplicate enumerator
        result = CreateEnumFormatEtc(_formatsCount, _formatEtc, ppEnumFormatEtc);

        if (result == S_OK)
        {
            // Manually set the index state
            static_cast<WindowsEnumFormatEtc*>(*ppEnumFormatEtc)->_index = _index;
        }

        return result;
    }
};

HRESULT CreateEnumFormatEtc(UINT nNumFormats, FORMATETC* pFormatEtc, IEnumFORMATETC** ppEnumFormatEtc)
{
    if (nNumFormats == 0 || pFormatEtc == nullptr || ppEnumFormatEtc == nullptr)
        return E_INVALIDARG;
    *ppEnumFormatEtc = new WindowsEnumFormatEtc(pFormatEtc, nNumFormats);
    return *ppEnumFormatEtc ? S_OK : E_OUTOFMEMORY;
}

/// <summary>
/// Drag drop source and data container for Ole
/// </summary>
class WindowsDragSource : public IDataObject, public IDropSource
{
private:
    ULONG _refCount;
    int32 _formatsCount;
    FORMATETC* _formatEtc;
    STGMEDIUM* _stgMedium;

public:
    WindowsDragSource(FORMATETC* fmtetc, STGMEDIUM* stgmed, int32 count)
        : _refCount(1)
        , _formatsCount(count)
        , _formatEtc(nullptr)
        , _stgMedium(nullptr)
    {
        // Allocate memory
        _formatEtc = new FORMATETC[count];
        _stgMedium = new STGMEDIUM[count];

        // Copy descriptors
        for (int32 i = 0; i < count; i++)
        {
            _formatEtc[i] = fmtetc[i];
            _stgMedium[i] = stgmed[i];
        }
    }

    virtual ~WindowsDragSource()
    {
        delete[] _formatEtc;
        delete[] _stgMedium;
    }

public:
    // [IUnknown]
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR * ppvObject) override
    {
        // Check to see what interface has been requested
        if (riid == IID_IDataObject || riid == IID_IUnknown || riid == IID_IDropSource)
        {
            AddRef();
            *ppvObject = this;
            return S_OK;
        }

        // No interface
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        _InterlockedIncrement(&_refCount);
        return _refCount;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG ulRefCount = _InterlockedDecrement(&_refCount);
        if (_refCount == 0)
        {
            delete this;
        }
        return ulRefCount;
    }

    // [IDropSource]
    HRESULT STDMETHODCALLTYPE QueryContinueDrag(_In_ BOOL fEscapePressed, _In_ DWORD grfKeyState) override
    {
        // If the Escape key has been pressed since the last call, cancel the drop
        if (fEscapePressed == TRUE || grfKeyState & MK_RBUTTON)
            return DRAGDROP_S_CANCEL;

        // If the LeftMouse button has been released, then do the drop!
        if ((grfKeyState & MK_LBUTTON) == 0)
            return DRAGDROP_S_DROP;

        // Continue with the drag-drop
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GiveFeedback(_In_ DWORD dwEffect) override
    {
        // TODO: allow to use custom mouse cursor during drop and drag operation
        return DRAGDROP_S_USEDEFAULTCURSORS;
    }

    // [IDataObject]
    HRESULT STDMETHODCALLTYPE GetData(_In_ FORMATETC* pformatetcIn, _Out_ STGMEDIUM* pmedium) override
    {
        if (pformatetcIn == nullptr || pmedium == nullptr)
            return E_INVALIDARG;

        // Try to match the specified FORMATETC with one of our supported formats
        int32 index = lookupFormatEtc(pformatetcIn);
        if (index == INVALID_INDEX)
            return DV_E_FORMATETC;

        // Found a match - transfer data into supplied storage medium
        pmedium->tymed = _formatEtc[index].tymed;
        pmedium->pUnkForRelease = nullptr;

        // Copy the data into the caller's storage medium
        switch (_formatEtc[index].tymed)
        {
        case TYMED_HGLOBAL:
            pmedium->hGlobal = duplicateGlobalMem(_stgMedium[index].hGlobal);
            break;

        default:
            return DV_E_FORMATETC;
        }

        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetDataHere(_In_ FORMATETC* pformatetc, _Inout_ STGMEDIUM* pmedium) override
    {
        return DATA_E_FORMATETC;
    }
    HRESULT STDMETHODCALLTYPE QueryGetData(__RPC__in_opt FORMATETC* pformatetc) override
    {
        return lookupFormatEtc(pformatetc) == INVALID_INDEX ? DV_E_FORMATETC : S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(__RPC__in_opt FORMATETC* pformatectIn, __RPC__out FORMATETC* pformatetcOut) override
    {
        // Apparently we have to set this field to NULL even though we don't do anything else
        pformatetcOut->ptd = nullptr;
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE SetData(_In_ FORMATETC* pformatetc, _In_ STGMEDIUM* pmedium, BOOL fRelease) override
    {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, __RPC__deref_out_opt IEnumFORMATETC** ppenumFormatEtc) override
    {
        // Only the get direction is supported for OLE
        if (dwDirection == DATADIR_GET)
        {
            // TODO: use SHCreateStdEnumFmtEtc API call
            return CreateEnumFormatEtc(_formatsCount, _formatEtc, ppenumFormatEtc);
        }

        // The direction specified is not supported for drag+drop
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE DAdvise(__RPC__in FORMATETC* pformatetc, DWORD advf, __RPC__in_opt IAdviseSink* pAdvSink, __RPC__out DWORD* pdwConnection) override
    {
        return OLE_E_ADVISENOTSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection) override
    {
        return OLE_E_ADVISENOTSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE EnumDAdvise(__RPC__deref_out_opt IEnumSTATDATA** ppenumAdvise) override
    {
        return OLE_E_ADVISENOTSUPPORTED;
    }

private:
    int32 lookupFormatEtc(FORMATETC* pFormatEtc) const
    {
        // Check each of our formats in turn to see if one matches
        for (int32 i = 0; i < _formatsCount; i++)
        {
            if ((_formatEtc[i].tymed & pFormatEtc->tymed) &&
                _formatEtc[i].cfFormat == pFormatEtc->cfFormat &&
                _formatEtc[i].dwAspect == pFormatEtc->dwAspect)
            {
                // Return index of stored format
                return i;
            }
        }

        // Format not found
        return INVALID_INDEX;
    }
};

WindowsGuiData GuiDragDropData;

DragDropEffect WindowsWindow::DoDragDrop(const StringView& data)
{
    // Create background worker that will keep updating GUI (perform rendering)
    const auto task = New<DoDragDropJob>();
    Task::StartNew(task);
    while (task->GetState() == TaskState::Queued)
        Platform::Sleep(1);

    // Create descriptors
    FORMATETC fmtetc = { CF_TEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stgmed = { TYMED_HGLOBAL, { nullptr }, nullptr };

    // Create a HGLOBAL inside the storage medium
    stgmed.hGlobal = StringToHandle(data);

    // Create drop source
    auto dropSource = new WindowsDragSource(&fmtetc, &stgmed, 1);

    // Do the drag drop operation
    DWORD dwEffect;
    HRESULT result = ::DoDragDrop(dropSource, dropSource, DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK | DROPEFFECT_SCROLL, &dwEffect);

    // Wait for job end
    Platform::AtomicStore(&task->ExitFlag, 1);
    task->Wait();

    // Release allocated data
    dropSource->Release();
    ReleaseStgMedium(&stgmed);

    // Fix hanging mouse state (Windows doesn't send WM_LBUTTONUP when we end the drag and drop)
    if (Input::GetMouseButton(MouseButton::Left))
    {
        ::POINT point;
        ::GetCursorPos(&point);
        Input::Mouse->OnMouseUp(Float2((float)point.x, (float)point.y), MouseButton::Left, this);
    }

    return SUCCEEDED(result) ? dropEffectFromOleEnum(dwEffect) : DragDropEffect::None;
}

HRESULT WindowsWindow::DragEnter(Windows::IDataObject* pDataObj, Windows::DWORD grfKeyState, Windows::POINTL pt, Windows::DWORD* pdwEffect)
{
    POINT p = { pt.x, pt.y };
    ::ScreenToClient(_handle, &p);
    GuiDragDropData.Init((IDataObject*)pDataObj);
    DragDropEffect effect = DragDropEffect::None;
    OnDragEnter(&GuiDragDropData, Float2(static_cast<float>(p.x), static_cast<float>(p.y)), effect);
    Focus();
    *pdwEffect = dropEffect2OleEnum(effect);
    return S_OK;
}

HRESULT WindowsWindow::DragOver(Windows::DWORD grfKeyState, Windows::POINTL pt, Windows::DWORD* pdwEffect)
{
    POINT p = { pt.x, pt.y };
    ::ScreenToClient(_handle, &p);
    DragDropEffect effect = DragDropEffect::None;
    OnDragOver(&GuiDragDropData, Float2(static_cast<float>(p.x), static_cast<float>(p.y)), effect);
    *pdwEffect = dropEffect2OleEnum(effect);
    return S_OK;
}

HRESULT WindowsWindow::DragLeave()
{
    OnDragLeave();
    return S_OK;
}

HRESULT WindowsWindow::Drop(Windows::IDataObject* pDataObj, Windows::DWORD grfKeyState, Windows::POINTL pt, Windows::DWORD* pdwEffect)
{
    POINT p = { pt.x, pt.y };
    ::ScreenToClient(_handle, &p);
    GuiDragDropData.Init((IDataObject*)pDataObj);
    DragDropEffect effect = DragDropEffect::None;
    OnDragDrop(&GuiDragDropData, Float2(static_cast<float>(p.x), static_cast<float>(p.y)), effect);
    *pdwEffect = dropEffect2OleEnum(effect);
    return S_OK;
}

#else

DragDropEffect WindowsWindow::DoDragDrop(const StringView& data)
{
	return DragDropEffect::None;
}

#endif

#endif
