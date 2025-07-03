// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_SDL

#include "SDLWindow.h"
#include "SDLInput.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUSwapChain.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Keyboard.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/WindowsManager.h"
#if PLATFORM_LINUX
#define COMPILE_WITH_TEXTURE_TOOL 1 // FIXME
#include "Engine/Tools/TextureTool/TextureTool.h"
#endif

#define NOGDI
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>
#undef CreateWindow

#if PLATFORM_WINDOWS
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#if USE_EDITOR
#include <oleidl.h>
#endif
#elif PLATFORM_LINUX
#include "Engine/Platform/Linux/IncludeX11.h"
#elif PLATFORM_MAC
#include <Cocoa/Cocoa.h>
#else
static_assert(false, "Unsupported Platform");
#endif

#define DefaultDPI 96

namespace SDLImpl
{
    SDLWindow* LastEventWindow = nullptr;
    SDL_Cursor* Cursors[SDL_SYSTEM_CURSOR_COUNT] = { nullptr };
    extern String XDGCurrentDesktop;
}

SDL_HitTestResult OnWindowHitTest(SDL_Window* win, const SDL_Point* area, void* data);
void GetRelativeWindowOffset(WindowType type, SDLWindow* parentWindow, Int2& positionOffset);
Int2 GetSDLWindowScreenPosition(const SDLWindow* window);
void SetSDLWindowScreenPosition(const SDLWindow* window, const Int2 position);

bool IsPopupWindow(WindowType type)
{
    return type == WindowType::Popup || type == WindowType::Tooltip;
}

void* GetNativeWindowPointer(SDL_Window* window)
{
    void* windowPtr;
    auto props = SDL_GetWindowProperties(window);
#if PLATFORM_WINDOWS
    windowPtr = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#elif PLATFORM_LINUX
    windowPtr = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
    if (windowPtr == nullptr)
        windowPtr = (void*)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
#elif PLATFORM_MAC
    windowPtr = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
#elif PLATFORM_ANDROID
    windowPtr = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, nullptr);
#elif PLATFORM_IOS
    windowPtr = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, nullptr);
#else
    static_assert(false, "unsupported platform");
#endif
    return windowPtr;
}

void SDLWindow::Init()
{
}

SDLWindow::SDLWindow(const CreateWindowSettings& settings)
    : WindowBase(settings)
    , _handle(nullptr)
    , _cachedClientRectangle(Rectangle())
#if PLATFORM_LINUX
    , _dragOver(false)
#endif
{
    Int2 clientSize(Math::TruncToInt(settings.Size.X), Math::TruncToInt(settings.Size.Y));
    _clientSize = Float2(clientSize);

    if (SDLPlatform::UsesWayland())
    {
        // The compositor seems to crash when something is rendered to the hidden popup window surface
        _settings.ShowAfterFirstPaint = _showAfterFirstPaint = false;
    }
    
    uint32 flags = SDL_WINDOW_HIDDEN;
    if (_settings.Type == WindowType::Utility)
        flags |= SDL_WINDOW_UTILITY;
    else if (_settings.Type == WindowType::Regular && !_settings.ShowInTaskbar)
        flags |= SDL_WINDOW_UTILITY;
    else if (_settings.Type == WindowType::Tooltip)
        flags |= SDL_WINDOW_TOOLTIP;
    else if (_settings.Type == WindowType::Popup)
        flags |= SDL_WINDOW_POPUP_MENU;

    if (!_settings.HasBorder)
        flags |= SDL_WINDOW_BORDERLESS;
    if (_settings.AllowInput)
        flags |= SDL_WINDOW_INPUT_FOCUS;
    else
        flags |= SDL_WINDOW_NOT_FOCUSABLE;
    if (_settings.HasSizingFrame)
        flags |= SDL_WINDOW_RESIZABLE;
    if (_settings.IsTopmost)
        flags |= SDL_WINDOW_ALWAYS_ON_TOP;
    if (_settings.SupportsTransparency)
        flags |= SDL_WINDOW_TRANSPARENT;

    // Disable parenting of child windows as those are always on top of the parent window and never show up in taskbar
    if (_settings.Parent != nullptr && (_settings.Type != WindowType::Tooltip && _settings.Type != WindowType::Popup))
        _settings.Parent = nullptr;

    // The window position needs to be relative to the parent window
    Int2 relativePosition(Math::TruncToInt(settings.Position.X), Math::TruncToInt(settings.Position.Y));
    GetRelativeWindowOffset(_settings.Type, _settings.Parent, relativePosition);

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, flags);
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, settings.Title.ToStringAnsi().Get());
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, relativePosition.X);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, relativePosition.Y);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, clientSize.X);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, clientSize.Y);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_EXTERNAL_GRAPHICS_CONTEXT_BOOLEAN, true);
    if ((flags & SDL_WINDOW_TOOLTIP) != 0)
        SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_TOOLTIP_BOOLEAN, true);
    else if ((flags & SDL_WINDOW_POPUP_MENU) != 0)
        SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_MENU_BOOLEAN, true);
    if (_settings.Parent != nullptr)
        SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_PARENT_POINTER, _settings.Parent->_window);

    _window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);
    if (_window == nullptr)
        Platform::Fatal(String::Format(TEXT("Cannot create SDL window: {0}"), String(SDL_GetError())));

    _windowId = SDL_GetWindowID(_window);
    _handle = GetNativeWindowPointer(_window);
    ASSERT(_handle != nullptr);

    SDL_DisplayID display = SDL_GetDisplayForWindow(_window);
    _dpiScale = SDL_GetWindowDisplayScale(_window);
    _dpi = Math::TruncToInt(_dpiScale * DefaultDPI);

    Int2 minimumSize(Math::TruncToInt(_settings.MinimumSize.X) , Math::TruncToInt(_settings.MinimumSize.Y));
    Int2 maximumSize(Math::TruncToInt(_settings.MaximumSize.X) , Math::TruncToInt(_settings.MaximumSize.Y));

    SDL_SetWindowMinimumSize(_window, minimumSize.X, minimumSize.Y);
#if PLATFORM_MAC
    // BUG: The maximum size is not enforced correctly, set it to real high value instead
    if (maximumSize.X == 0)
        maximumSize.X = 999999;
    if (maximumSize.Y == 0)
        maximumSize.Y = 999999;
#endif
    SDL_SetWindowMaximumSize(_window, maximumSize.X, maximumSize.Y);
    
    SDL_SetWindowHitTest(_window, &OnWindowHitTest, this);
    InitSwapChain();

#if USE_EDITOR
    // Enable file drag & drop support
    if (_settings.AllowDragAndDrop)
    {
#if PLATFORM_WINDOWS
        const auto result = RegisterDragDrop((HWND)_handle, (LPDROPTARGET)(Windows::IDropTarget*)this);
        if (result != S_OK)
        {
            LOG(Warning, "Window drag and drop service error: 0x{0:x}:{1}", result, 1);
        }
#elif PLATFORM_LINUX
        auto xDisplay = (X11::Display*)GetX11Display();
        if (xDisplay)
        {
            auto xdndVersion = 5;
            auto xdndAware = X11::XInternAtom(xDisplay, "XdndAware", 0);
            if (xdndAware != 0)
                X11::XChangeProperty(xDisplay, (X11::Window)_handle, xdndAware, (X11::Atom)4, 32, PropModeReplace, (unsigned char*)&xdndVersion, 1);
        }
#elif PLATFORM_MAC
        NSWindow* win = ((NSWindow*)_handle);
        NSView* view = win.contentView;
        [win unregisterDraggedTypes];
        [win registerForDraggedTypes:@[NSPasteboardTypeFileURL, NSPasteboardTypeString, (NSString*)kUTTypeFileURL, (NSString*)kUTTypeUTF8PlainText]];
#endif
    }
#endif

    SDLImpl::LastEventWindow = this;

#if PLATFORM_LINUX
    // Initialize using the shared Display instance from SDL
    if (SDLPlatform::UsesX11() && SDLPlatform::GetXDisplay() == nullptr)
        SDLPlatform::InitX11(GetX11Display());

    // Window focus changes breaks the text input for some reason, just keep it enabled for good
    if (SDLPlatform::UsesX11() && _settings.AllowInput)
        SDL_StartTextInput(_window);
#endif

#if PLATFORM_LINUX && COMPILE_WITH_TEXTURE_TOOL
    // Ensure windows other than the main window have some kind of icon
    static SDL_Surface* surface = nullptr;
    static Array<Color32> colorData;
    if (surface == nullptr)
    {
        const String iconPath = Globals::BinariesFolder / TEXT("Logo.png");
        if (FileSystem::FileExists(iconPath))
        {
            TextureData icon;
            if (!TextureTool::ImportTexture(iconPath, icon))
            {
                icon.GetPixels(colorData);
                surface = SDL_CreateSurfaceFrom(icon.Width, icon.Height, SDL_PIXELFORMAT_ABGR8888, colorData.Get(), sizeof(Color32) * icon.Width);
            }
        }
    }
    if (surface != nullptr)
        SDL_SetWindowIcon(_window, surface);
#endif
}

SDL_Window* SDLWindow::GetSDLWindow() const
{
    return _window;
}

#if PLATFORM_LINUX

void* SDLWindow::GetWaylandDisplay() const
{
    return SDL_GetPointerProperty(SDL_GetWindowProperties(_window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
}

void* SDLWindow::GetX11Display() const
{
    return SDL_GetPointerProperty(SDL_GetWindowProperties(_window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
}

#endif

SDLWindow::~SDLWindow()
{
    if (SDLImpl::LastEventWindow == this)
        SDLImpl::LastEventWindow = nullptr;

    if (_window == nullptr)
        return;

    if (Input::Mouse != nullptr && Input::Mouse->IsRelative(this))
        Input::Mouse->SetRelativeMode(false, this);
    
    SDL_StopTextInput(_window);
    SDL_DestroyWindow(_window);

    _window = nullptr;
    _handle = nullptr;
    _windowId = 0;
    _visible = false;
}

WindowHitCodes SDLWindow::OnWindowHit(const Float2 point)
{
    WindowHitCodes hit = WindowHitCodes::Client;
    if (!IsFullscreen())
    {
        Float2 screenPosition = ClientToScreen(point);
        bool handled = false;
        OnHitTest(screenPosition, hit, handled);
        if (!handled)
        {
            int margin = _settings.HasBorder ? 0 : 0;
            auto size = GetClientSize();
            //if (point.Y < 0)
            //    hit = WindowHitCodes::Caption;
            if (point.Y < margin && point.X < margin)
                hit = WindowHitCodes::TopLeft;
            else if (point.Y < margin && point.X > size.X - margin)
                hit = WindowHitCodes::TopRight;
            else if (point.Y < margin)
                hit = WindowHitCodes::Top;
            else if (point.X < margin && point.Y > size.Y - margin)
                hit = WindowHitCodes::BottomLeft;
            else if (point.X < margin)
                hit = WindowHitCodes::Left;
            else if (point.X > size.X - margin && point.Y > size.Y - margin)
                hit = WindowHitCodes::BottomRight;
            else if (point.X > size.X - margin)
                hit = WindowHitCodes::Right;
            else if (point.Y > size.Y - margin)
                hit = WindowHitCodes::Bottom;
            else
                hit = WindowHitCodes::Client;
        }
    }
    return hit;
}

SDL_HitTestResult OnWindowHitTest(SDL_Window* win, const SDL_Point* area, void* data)
{
    SDLWindow* window = static_cast<SDLWindow*>(data);
    const Float2 point(static_cast<float>(area->x), static_cast<float>(area->y));
    WindowHitCodes hit = window->OnWindowHit(point);
    switch (hit)
    {
    case WindowHitCodes::Caption:
        return SDL_HITTEST_DRAGGABLE;
    case WindowHitCodes::TopLeft:
        return SDL_HITTEST_RESIZE_TOPLEFT;
    case WindowHitCodes::Top:
        return SDL_HITTEST_RESIZE_TOP;
    case WindowHitCodes::TopRight:
        return SDL_HITTEST_RESIZE_TOPRIGHT;
    case WindowHitCodes::Right:
        return SDL_HITTEST_RESIZE_RIGHT;
    case WindowHitCodes::BottomRight:
        return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
    case WindowHitCodes::Bottom:
        return SDL_HITTEST_RESIZE_BOTTOM;
    case WindowHitCodes::BottomLeft:
        return SDL_HITTEST_RESIZE_BOTTOMLEFT;
    case WindowHitCodes::Left:
        return SDL_HITTEST_RESIZE_LEFT;
    default:
        return SDL_HITTEST_NORMAL;
    }
}

SDLWindow* SDLWindow::GetWindowFromEvent(const SDL_Event& event)
{
    SDL_Window* window = SDL_GetWindowFromEvent(&event);
    if (window == nullptr)
        return nullptr;
    if (SDLImpl::LastEventWindow == nullptr || window != SDLImpl::LastEventWindow->_window)
        SDLImpl::LastEventWindow = GetWindowWithSDLWindow(window);
    return SDLImpl::LastEventWindow;
}

SDLWindow* SDLWindow::GetWindowWithSDLWindow(SDL_Window* window)
{
    SDLWindow* found = nullptr;
    WindowsManager::WindowsLocker.Lock();
    for (auto win : WindowsManager::Windows)
    {
        if (win->_window == window)
        {
            found = win;
            break;
        }
    }
    WindowsManager::WindowsLocker.Unlock();
    return found;
}

void SDLWindow::HandleEvent(SDL_Event& event)
{
    if (_isClosing)
        return;

    // Platform specific event handling
    if (HandleEventInternal(event))
        return;

    switch (event.type)
    {
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    {
        Close(ClosingReason::User);
        return;
    }
    case SDL_EVENT_WINDOW_DESTROYED:
    {
        // Quit
#if PLATFORM_WINDOWS
        PostQuitMessage(0);
#endif
        return;
    }
    case SDL_EVENT_MOUSE_MOTION:
    {
        if (_isTrackingMouse && _isUsingMouseOffset)
        {
            Float2 delta(event.motion.xrel, event.motion.yrel);
            _trackingMouseOffset += delta;
        }
        break;
    }
    case SDL_EVENT_KEY_DOWN:
    {
        if (event.key.scancode == SDL_SCANCODE_RETURN && Input::Keyboard->GetKey(KeyboardKeys::Alt))
        {
            LOG(Info, "Alt+Enter pressed");
            SetIsFullscreen(!IsFullscreen());
            return;
        }
        break;
    }
    case SDL_EVENT_WINDOW_MOVED:
    {
        _cachedClientRectangle.Location = Float2(static_cast<float>(event.window.data1), static_cast<float>(event.window.data2));
        return;
    }
    case SDL_EVENT_WINDOW_HIT_TEST:
    {
        return;
    }
    case SDL_EVENT_WINDOW_MINIMIZED:
    {
        _minimized = true;
        _maximized = false;
        return;
    }
    case SDL_EVENT_WINDOW_MAXIMIZED:
    {
        _minimized = false;
        _maximized = true;
        
        CheckForWindowResize();
        return;
    }
    case SDL_EVENT_WINDOW_RESTORED:
    {
        if (_maximized)
        {
            _maximized = false;
            // We assume SDL_EVENT_WINDOW_RESIZED is called right afterwards, no need to check for resize here
            //CheckForWindowResize();
        }
        else if (_minimized)
        {
            _minimized = false;
            CheckForWindowResize();
        }
        return;
    }
    case SDL_EVENT_WINDOW_RESIZED:
    {
        int32 width = event.window.data1;
        int32 height = event.window.data2;

        _clientSize = Float2(static_cast<float>(width), static_cast<float>(height));
        _cachedClientRectangle.Size = _clientSize;

        // Check if window size has been changed
        if (width > 0 && height > 0 && (_swapChain == nullptr || width != _swapChain->GetWidth() || height != _swapChain->GetHeight()))
            OnResize(width, height);
        return;
    }
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
    {
        OnGotFocus();
        if (_settings.AllowInput && !SDLPlatform::UsesX11())
            SDL_StartTextInput(_window);
        if (_isClippingCursor)
        {
            // The relative mode needs to be disabled for clipping to take effect
            bool inRelativeMode = Input::Mouse->IsRelative(this) || _restoreRelativeMode;
            if (inRelativeMode)
                Input::Mouse->SetRelativeMode(false, this);

            // Restore previous clipping region
            SDL_Rect rect{ (int)_clipCursorRect.GetX(), (int)_clipCursorRect.GetY(), (int)_clipCursorRect.GetWidth(), (int)_clipCursorRect.GetHeight() };
            SDL_SetWindowMouseRect(_window, &rect);

            if (inRelativeMode)
                Input::Mouse->SetRelativeMode(true, this);
        }
        else if (_restoreRelativeMode)
            Input::Mouse->SetRelativeMode(true, this);
        _restoreRelativeMode = false;
        return;
    }
    case SDL_EVENT_WINDOW_FOCUS_LOST:
    {
        if (_settings.AllowInput && !SDLPlatform::UsesX11())
            SDL_StopTextInput(_window);
        if (_isClippingCursor)
            SDL_SetWindowMouseRect(_window, nullptr);

        if (Input::Mouse->IsRelative(this))
        {
            Input::Mouse->SetRelativeMode(false, this);
            _restoreRelativeMode = true;
        }

        OnLostFocus();
        return;
    }
    case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
    {
        float scale = SDL_GetWindowDisplayScale(_window);
        if (scale > 0.0f && _dpiScale != scale)
        {
            float oldScale = _dpiScale;
            _dpiScale = scale;
            _dpi = static_cast<int>(_dpiScale * DefaultDPI);
            int w = static_cast<int>(_cachedClientRectangle.GetWidth() * (scale / oldScale));
            int h = static_cast<int>(_cachedClientRectangle.GetHeight() * (scale / oldScale));
            _cachedClientRectangle.Size = Float2(static_cast<float>(w), static_cast<float>(h));
            SDL_SetWindowSize(_window, w, h);
            // TODO: Recalculate fonts
        }
        return;
    }
    default:
        break;
    }

    if (_settings.AllowInput)
    {
        if (SDLInput::HandleEvent(this, event))
            return;
    }
}

void* SDLWindow::GetNativePtr() const
{
    return _handle;
}

void SDLWindow::Show()
{
    if (_visible)
        return;

    if (_showAfterFirstPaint)
    {
        if (RenderTask)
            RenderTask->Enabled = true;
        return;
    }

    SDL_ShowWindow(_window);
    if (_settings.AllowInput && _settings.ActivateWhenFirstShown)
        Focus();
    else if (_settings.Parent == nullptr)
        BringToFront();
    
    // Reused top-most windows doesn't stay on top for some reason
    if (_settings.IsTopmost && !IsPopupWindow(_settings.Type))
        SetIsAlwaysOnTop(true);

    if (_isTrackingMouse)
    {
        if (!SDL_CaptureMouse(true))
        {
            if (!SDLPlatform::UsesWayland()) // Suppress "That operation is not supported" errors
                LOG(Warning, "SDL_CaptureMouse: {0}", String(SDL_GetError()));
        }
    }

    WindowBase::Show();
}

void SDLWindow::Hide()
{
    if (!_visible)
        return;

    SDL_HideWindow(_window);

    WindowBase::Hide();
}

void SDLWindow::Minimize()
{
    if (!_settings.AllowMinimize)
        return;

    SDL_MinimizeWindow(_window);
}

void SDLWindow::Maximize()
{
    if (!_settings.AllowMaximize)
        return;

    SDL_MaximizeWindow(_window);
}

void SDLWindow::SetBorderless(bool isBorderless, bool maximized)
{
    if (IsFullscreen())
        SetIsFullscreen(false);

    // Fixes issue of borderless window not going full screen
    if (IsMaximized())
        Restore();

    _settings.HasBorder = !isBorderless;

    BringToFront();

    SDL_SetWindowBordered(_window, !isBorderless ? true : false);
    if (maximized)
        Maximize();
    else
        Focus();
    
    CheckForWindowResize();
}

void SDLWindow::Restore()
{
    SDL_RestoreWindow(_window);
}

bool SDLWindow::IsClosed() const
{
    return WindowBase::IsClosed() || _handle == nullptr;
}

bool SDLWindow::IsForegroundWindow() const
{
    SDL_WindowFlags flags = SDL_GetWindowFlags(_window);
    return (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
}

void SDLWindow::BringToFront(bool force)
{
    SDL_RaiseWindow(_window);
}

void SDLWindow::SetClientBounds(const Rectangle& clientArea)
{
    Int2 newPos = Int2(clientArea.GetTopLeft());
    int newW = static_cast<int>(clientArea.GetWidth());
    int newH = static_cast<int>(clientArea.GetHeight());

    SDL_SetWindowSize(_window, newW, newH);
    SetSDLWindowScreenPosition(this, newPos);
}

void GetRelativeWindowOffset(WindowType type, SDLWindow* parentWindow, Int2& positionOffset)
{
    if (!IsPopupWindow(type))
        return;

    SDLWindow* window = parentWindow;
    while (window != nullptr)
    {
        Int2 parentPosition;
        SDL_GetWindowPosition(window->GetSDLWindow(), &parentPosition.X, &parentPosition.Y);
        positionOffset -= parentPosition;

        if (!IsPopupWindow(window->GetSettings().Type))
            break;

        window = window->GetSettings().Parent;
    }
}

Int2 GetSDLWindowScreenPosition(const SDLWindow* window)
{
    Int2 relativeOffset(0, 0);
    GetRelativeWindowOffset(window->GetSettings().Type, window->GetSettings().Parent, relativeOffset);
    
    Int2 position;
    SDL_GetWindowPosition(window->GetSDLWindow(), &position.X, &position.Y);

    return position - relativeOffset;
}

void SetSDLWindowScreenPosition(const SDLWindow* window, const Int2 position)
{
    Int2 relativePosition = position;
    GetRelativeWindowOffset(window->GetSettings().Type, window->GetSettings().Parent, relativePosition);
    SDL_SetWindowPosition(window->GetSDLWindow(), relativePosition.X, relativePosition.Y);
}

void SDLWindow::SetPosition(const Float2& position)
{
    Int2 topLeftBorder;
    SDL_GetWindowBordersSize(_window, &topLeftBorder.Y, &topLeftBorder.X, nullptr, nullptr);

    Int2 screenPosition(static_cast<int>(position.X), static_cast<int>(position.Y));
    screenPosition += topLeftBorder;

    if (false && SDLPlatform::UsesX11())
    {
        // TODO: is this needed?
        auto monitorBounds = Platform::GetMonitorBounds(Float2::Minimum);
        screenPosition += Int2(monitorBounds.GetTopLeft());
    }
    
    SetSDLWindowScreenPosition(this, screenPosition);
}

void SDLWindow::SetClientPosition(const Float2& position)
{
    SetSDLWindowScreenPosition(this, Int2(position));
}

void SDLWindow::SetIsFullscreen(bool isFullscreen)
{
    SDL_SetWindowFullscreen(_window, isFullscreen ? true : false);
    if (!isFullscreen)
    {
        // The window is set to always-on-top for some reason when leaving fullscreen
        SetIsAlwaysOnTop(false);
    }

    WindowBase::SetIsFullscreen(isFullscreen);
}

bool SDLWindow::IsAlwaysOnTop() const
{
    SDL_WindowFlags flags = SDL_GetWindowFlags(_window);
    return (flags & SDL_WINDOW_ALWAYS_ON_TOP) != 0;
}

void SDLWindow::SetIsAlwaysOnTop(bool isAlwaysOnTop)
{
    if (!SDL_SetWindowAlwaysOnTop(_window, isAlwaysOnTop))
        LOG(Warning, "SDL_SetWindowAlwaysOnTop failed: {0}", String(SDL_GetError()));
    // Not sure if this should change _settings.IsTopmost to reflect the new value?
}

Float2 SDLWindow::GetPosition() const
{
    Int2 topLeftBorder;
    SDL_GetWindowBordersSize(_window, &topLeftBorder.Y, &topLeftBorder.X, nullptr, nullptr);

    Int2 position = GetSDLWindowScreenPosition(this);
    position -= topLeftBorder;
    
    return Float2(static_cast<float>(position.X), static_cast<float>(position.Y));
}

Float2 SDLWindow::GetSize() const
{
    int top, left, bottom, right;
    SDL_GetWindowBordersSize(_window, &top, &left, &bottom, &right);
    
    int w, h;
    SDL_GetWindowSizeInPixels(_window, &w, &h);
    return Float2(static_cast<float>(w + left + right), static_cast<float>(h + top + bottom));
}

Float2 SDLWindow::GetClientSize() const
{
    int w, h;
    SDL_GetWindowSizeInPixels(_window, &w, &h);

    return Float2(static_cast<float>(w), static_cast<float>(h));;
}

Float2 SDLWindow::ScreenToClient(const Float2& screenPos) const
{
    Int2 position = GetSDLWindowScreenPosition(this);
    return screenPos - Float2(static_cast<float>(position.X), static_cast<float>(position.Y));
}

Float2 SDLWindow::ClientToScreen(const Float2& clientPos) const
{
    Int2 position = GetSDLWindowScreenPosition(this);
    return clientPos + Float2(static_cast<float>(position.X), static_cast<float>(position.Y));
}

void SDLWindow::FlashWindow()
{
#if PLATFORM_LINUX
    // KDE bug: flashing brings the window on top of other windows, disable it for now...
    if (SDLPlatform::UsesWayland() && SDLImpl::XDGCurrentDesktop.Compare(String("KDE"), StringSearchCase::IgnoreCase) == 0)
        return;
#endif
    SDL_FlashWindow(_window, SDL_FLASH_UNTIL_FOCUSED);
}

float SDLWindow::GetOpacity() const
{
    float opacity = SDL_GetWindowOpacity(_window);
    if (opacity < 0.0f)
    {
        LOG(Warning, "SDL_GetWindowOpacity failed: {0}", String(SDL_GetError()));
        opacity = 1.0f;
    }
    return opacity;
}

void SDLWindow::SetOpacity(const float opacity)
{
    if (!SDL_SetWindowOpacity(_window, opacity))
        LOG(Warning, "SDL_SetWindowOpacity failed: {0}", String(SDL_GetError()));
}

#if !PLATFORM_WINDOWS

void SDLWindow::Focus()
{
    SDL_RaiseWindow(_window);
}

#endif

String SDLWindow::GetTitle() const
{
    return String(SDL_GetWindowTitle(_window));
}

void SDLWindow::SetTitle(const StringView& title)
{
    SDL_SetWindowTitle(_window, title.ToStringAnsi().Get());
}

void SDLWindow::StartTrackingMouse(bool useMouseScreenOffset)
{
    if (_isTrackingMouse)
        return;

    _isTrackingMouse = true;
    _trackingMouseOffset = Float2::Zero;
    _isUsingMouseOffset = useMouseScreenOffset;

    if (_visible)
    {
        if (!SDL_CaptureMouse(true))
        {
            if (!SDLPlatform::UsesWayland()) // Suppress "That operation is not supported" errors
                LOG(Warning, "SDL_CaptureMouse: {0}", String(SDL_GetError()));
        }

        // For viewport camera mouse tracking we want to use relative mode for best precision
        if (_cursor == CursorType::Hidden)
            Input::Mouse->SetRelativeMode(true, this);
    }
}

void SDLWindow::EndTrackingMouse()
{
    if (!_isTrackingMouse)
        return;
    
    _isTrackingMouse = false;
    _isHorizontalFlippingMouse = false;
    _isVerticalFlippingMouse = false;

    if (!SDL_CaptureMouse(false))
    {
        if (!SDLPlatform::UsesWayland()) // Suppress "That operation is not supported" errors
            LOG(Warning, "SDL_CaptureMouse: {0}", String(SDL_GetError()));
    }

    Input::Mouse->SetRelativeMode(false, this);
}

void SDLWindow::StartClippingCursor(const Rectangle& bounds)
{
    if (!IsFocused())
        return;

    // The cursor is not fully constrained when positioned outside the clip region
    SetMousePosition(bounds.GetCenter());

    _isClippingCursor = true;
    SDL_Rect rect{ (int)bounds.GetX(), (int)bounds.GetY(), (int)bounds.GetWidth(), (int)bounds.GetHeight() };
    SDL_SetWindowMouseRect(_window, &rect);
    _clipCursorRect = bounds;
}

void SDLWindow::EndClippingCursor()
{
    if (!_isClippingCursor)
        return;

    _isClippingCursor = false;
    SDL_SetWindowMouseRect(_window, nullptr);
}

void SDLWindow::SetMousePosition(const Float2& position) const
{
    if (!_settings.AllowInput || !_focused)
        return;
        
    SDL_WarpMouseInWindow(_window, position.X, position.Y);

    Float2 screenPosition = ClientToScreen(position);
    Input::Mouse->OnMouseMoved(screenPosition);
}

void SDLWindow::SetCursor(CursorType type)
{
    CursorType oldCursor = _cursor;
    WindowBase::SetCursor(type);
    if (oldCursor != type)
        SDLWindow::UpdateCursor();
}

void SDLWindow::CheckForWindowResize()
{
    return;
    // Cache client size
    _clientSize = GetClientSize();
    int32 width = (int32)(_clientSize.X);
    int32 height = (int32)(_clientSize.Y);

    // Check for windows maximized size and see if it needs to adjust position if needed
    if (_maximized)
    {
        // Pick the current monitor data for sizing
        SDL_Rect rect;
        auto displayId = SDL_GetDisplayForWindow(_window);
        SDL_GetDisplayUsableBounds(displayId, &rect);

        if (width > rect.w && height > rect.h)
        {
            width = rect.w;
            height = rect.h;
            SDL_SetWindowSize(_window, width, height);
        }
    }
    
    // Check if window size has been changed
    if (width > 0 && height > 0 && (_swapChain == nullptr || width != _swapChain->GetWidth() || height != _swapChain->GetHeight()))
        OnResize(width, height);
}

void SDLWindow::UpdateCursor()
{
    if (_cursor == CursorType::Hidden)
    {
        SDL_HideCursor();

        if (_isTrackingMouse)
            Input::Mouse->SetRelativeMode(true, this);
        return;
    }
    SDL_ShowCursor();
    //if (_isTrackingMouse)
    //    Input::Mouse->SetRelativeMode(false, this);
    
    int32 index = SDL_SYSTEM_CURSOR_DEFAULT;
    switch (_cursor)
    {
    case CursorType::Cross:
        index = SDL_SYSTEM_CURSOR_CROSSHAIR;
        break;
    case CursorType::Hand:
        index = SDL_SYSTEM_CURSOR_POINTER;
        break;
    case CursorType::Help:
        //index = SDL_SYSTEM_CURSOR_DEFAULT;
        break;
    case CursorType::IBeam:
        index = SDL_SYSTEM_CURSOR_TEXT;
        break;
    case CursorType::No:
        index = SDL_SYSTEM_CURSOR_NOT_ALLOWED;
        break;
    case CursorType::Wait:
        index = SDL_SYSTEM_CURSOR_WAIT;
        break;
    case CursorType::SizeAll:
        index = SDL_SYSTEM_CURSOR_MOVE;
        break;
    case CursorType::SizeNESW:
        index = SDL_SYSTEM_CURSOR_NESW_RESIZE;
        break;
    case CursorType::SizeNS:
        index = SDL_SYSTEM_CURSOR_NS_RESIZE;
        break;
    case CursorType::SizeNWSE:
        index = SDL_SYSTEM_CURSOR_NWSE_RESIZE;
        break;
    case CursorType::SizeWE:
        index = SDL_SYSTEM_CURSOR_EW_RESIZE;
        break;
    case CursorType::Default:
    default:
        break;
    }

    if (SDLImpl::Cursors[index] == nullptr)
        SDLImpl::Cursors[index] = SDL_CreateSystemCursor(static_cast<SDL_SystemCursor>(index));
    SDL_SetCursor(SDLImpl::Cursors[index]);
}

void SDLWindow::SetIcon(TextureData& icon)
{
    Array<Color32> colorData;
    icon.GetPixels(colorData);
    SDL_Surface* surface = SDL_CreateSurfaceFrom(icon.Width, icon.Height, SDL_PIXELFORMAT_ABGR8888, colorData.Get(), sizeof(Color32) * icon.Width);

    SDL_SetWindowIcon(_window, surface);
    SDL_free(surface);
}

#endif

