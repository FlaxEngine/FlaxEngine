// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if PLATFORM_SDL

#include "SDLWindow.h"
#include "SDLInput.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUSwapChain.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Keyboard.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Platform/IGuiData.h"
#include "Engine/Platform/WindowsManager.h"

#define NOGDI
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>
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
#endif

#define DefaultDPI 96
SDLWindow* lastEventWindow = nullptr;

void* GetNativeWindowPointer(SDL_Window* window);
SDL_HitTestResult OnWindowHitTest(SDL_Window* win, const SDL_Point* area, void* data);

class SDLDropFilesData : public IGuiData
{
public:
    Array<String> Files;

    Type GetType() const override
    {
        return Type::Files;
    }
    String GetAsText() const override
    {
        return String::Empty;
    }
    void GetAsFiles(Array<String>* files) const override
    {
        files->Add(Files);
    }
};

class SDLDropTextData : public IGuiData
{
public:
    StringView Text;

    Type GetType() const override
    {
        return Type::Text;
    }
    String GetAsText() const override
    {
        return String(Text);
    }
    void GetAsFiles(Array<String>* files) const override
    {
    }
};

SDLWindow::SDLWindow(const CreateWindowSettings& settings)
    : WindowBase(settings)
    , _handle(nullptr)
    , _cachedClientRectangle(Rectangle())
{
    int32 x = Math::TruncToInt(settings.Position.X);
    int32 y = Math::TruncToInt(settings.Position.Y);
    int32 clientWidth = Math::TruncToInt(settings.Size.X);
    int32 clientHeight = Math::TruncToInt(settings.Size.Y);
    int32 windowWidth = clientWidth;
    int32 windowHeight = clientHeight;
    _clientSize = Float2((float)clientWidth, (float)clientHeight);

    if (SDLPlatform::UsesWayland())
    {
        // The compositor seems to crash when something is rendered to the surface when window is hidden
        _settings.ShowAfterFirstPaint = _showAfterFirstPaint = false;
    }
    
    uint32 flags = 0;
    if (!_settings.HasBorder)
        flags |= SDL_WINDOW_BORDERLESS;
    if (_settings.Type == WindowType::Regular)
        flags |= SDL_WINDOW_INPUT_FOCUS;
    else if (_settings.Type == WindowType::Utility)
        flags |= SDL_WINDOW_UTILITY;
    else if (_settings.Type == WindowType::Tooltip)
        flags |= SDL_WINDOW_TOOLTIP;
    else if (_settings.Type == WindowType::Popup)
        flags |= SDL_WINDOW_POPUP_MENU;


    if (!_settings.AllowInput)
        flags |= SDL_WINDOW_NOT_FOCUSABLE;
    //if (!_settings.ShowInTaskbar && _settings.IsRegularWindow)
    //flags |= SDL_WINDOW_UTILITY;
    if (_settings.ShowAfterFirstPaint)
        flags |= SDL_WINDOW_HIDDEN;
    if (_settings.HasSizingFrame)
        flags |= SDL_WINDOW_RESIZABLE;
    //flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;

    if (_settings.Parent == nullptr && (_settings.Type == WindowType::Tooltip || _settings.Type == WindowType::Popup))
    {
        // Creating a popup window on some platforms brings the parent window on top.
        // Use the currently focused window as the parent instead to avoid losing focus of it
        WindowsManager::WindowsLocker.Lock();
        for (auto win : WindowsManager::Windows)
        {
            if (win->IsFocused())
            {
                _settings.Parent = win;
                break;
            }
        }
        WindowsManager::WindowsLocker.Unlock();

        if (_settings.Parent == nullptr)
            _settings.Parent = Engine::MainWindow;
    }

    // The SDL window position is always relative to the parent window
    if (_settings.Parent != nullptr)
    {
        x -= Math::TruncToInt(_settings.Parent->GetPosition().X);
        y -= Math::TruncToInt(_settings.Parent->GetPosition().Y);
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, "flags", flags);
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, settings.Title.ToStringAnsi().Get());
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, x);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, y);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, windowWidth);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, windowHeight);
    if ((flags & SDL_WINDOW_TOOLTIP) != 0)
    {
        SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_TOOLTIP_BOOLEAN, SDL_TRUE);
        SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_PARENT_POINTER, _settings.Parent->_window);
    }
    else if ((flags & SDL_WINDOW_POPUP_MENU) != 0)
    {
        SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_MENU_BOOLEAN, SDL_TRUE);
        SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_PARENT_POINTER, _settings.Parent->_window);
    }

    _window = SDL_CreateWindowWithProperties(props);
    if (_window == nullptr)
        Platform::Fatal(String::Format(TEXT("Cannot create SDL window: {0}"), String(SDL_GetError())));

    _windowId = SDL_GetWindowID(_window);
    _handle = GetNativeWindowPointer(_window);
    ASSERT(_handle != nullptr);
    
    SDL_DisplayID display = SDL_GetDisplayForWindow(_window);
    _dpiScale = SDL_GetDisplayContentScale(display);
    _dpi = (int)(_dpiScale * DefaultDPI);

    SDL_SetWindowMinimumSize(_window, (int)_settings.MinimumSize.X, (int)_settings.MinimumSize.Y);
    SDL_SetWindowMaximumSize(_window, (int)_settings.MaximumSize.X, (int)_settings.MaximumSize.Y);

    SDL_Rect rect;
    SDL_GetWindowPosition(_window, &rect.x, &rect.y);
    SDL_GetWindowSizeInPixels(_window, &rect.w, &rect.h);
    _cachedClientRectangle = Rectangle((float)rect.x, (float)rect.y, (float)rect.w, (float)rect.h);

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
        else
        {
            // TODO: Wayland
        }
#endif
    }
#endif

    lastEventWindow = this;

#if PLATFORM_LINUX
    SDLPlatform::InitPlatformX11(GetX11Display());
#endif
}

void* GetNativeWindowPointer(SDL_Window* window)
{
    void* windowPtr;
    auto props = SDL_GetWindowProperties(window);
#if PLATFORM_WINDOWS
    windowPtr = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#elif PLATFORM_LINUX
    windowPtr = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
    if (windowPtr == nullptr)
        windowPtr = (void*)SDL_GetNumberProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
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

#if PLATFORM_LINUX

void* SDLWindow::GetWaylandSurfacePtr() const
{
    return SDL_GetPointerProperty(SDL_GetWindowProperties(_window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
}

void* SDLWindow::GetWaylandDisplay() const
{
    return SDL_GetPointerProperty(SDL_GetWindowProperties(_window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
}

uintptr SDLWindow::GetX11WindowHandle() const
{
    return (uintptr)SDL_GetNumberProperty(SDL_GetWindowProperties(_window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
}

void* SDLWindow::GetX11Display() const
{
    return SDL_GetPointerProperty(SDL_GetWindowProperties(_window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
}

#endif

SDLWindow::~SDLWindow()
{
    if (lastEventWindow == this)
        lastEventWindow = nullptr;

    if (_window == nullptr)
        return;

    SDL_StopTextInput(_window);
    SDL_DestroyWindow(_window);

    _window = nullptr;
    _handle = nullptr;
    _windowId = 0;
    _visible = false;
}

SDL_HitTestResult OnWindowHitTest(SDL_Window* win, const SDL_Point* area, void* data)
{
    SDLWindow* window = (SDLWindow*)data;
    if (window->IsFullscreen())
        return SDL_HITTEST_NORMAL;

    Float2 clientPosition = Float2(static_cast<float>(area->x), static_cast<float>(area->y));
    Float2 screenPosition = window->ClientToScreen(clientPosition);

    //auto clientBounds = window->GetClientBounds();

    WindowHitCodes hit = WindowHitCodes::Client;
    bool handled = false;
    window->OnHitTest(screenPosition, hit, handled);

    if (!handled)
    {
        int margin = window->GetSettings().HasBorder ? 0 : 0;
        auto size = window->GetClientSize();
        //if (clientPosition.Y < 0)
        //    return SDL_HITTEST_DRAGGABLE;
        if (clientPosition.Y < margin && clientPosition.X < margin)
            return SDL_HITTEST_RESIZE_TOPLEFT;
        else if (clientPosition.Y < margin && clientPosition.X > size.X - margin)
            return SDL_HITTEST_RESIZE_TOPRIGHT;
        else if (clientPosition.Y < margin)
            return SDL_HITTEST_RESIZE_TOP;
        else if (clientPosition.X < margin && clientPosition.Y > size.Y - margin)
            return SDL_HITTEST_RESIZE_BOTTOMLEFT;
        else if (clientPosition.X < margin)
            return SDL_HITTEST_RESIZE_LEFT;
        else if (clientPosition.X > size.X - margin && clientPosition.Y > size.Y - margin)
            return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
        else if (clientPosition.X > size.X - margin)
            return SDL_HITTEST_RESIZE_RIGHT;
        else if (clientPosition.Y > size.Y - margin)
            return SDL_HITTEST_RESIZE_BOTTOM;
        else
            return SDL_HITTEST_NORMAL;
    }

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
    SDL_WindowID windowId;
    if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST)
        windowId = event.window.windowID;
    else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
        windowId = event.key.windowID;
    else if (event.type == SDL_EVENT_TEXT_EDITING)
        windowId = event.edit.windowID;
    else if (event.type == SDL_EVENT_TEXT_INPUT)
        windowId = event.text.windowID;
    else if (event.type == SDL_EVENT_MOUSE_MOTION)
        windowId = event.motion.windowID;
    else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        windowId = event.button.windowID;
    else if (event.type == SDL_EVENT_MOUSE_WHEEL)
        windowId = event.wheel.windowID;
    else if (event.type == SDL_EVENT_FINGER_MOTION || event.type == SDL_EVENT_FINGER_DOWN || event.type == SDL_EVENT_FINGER_UP)
        windowId = event.tfinger.windowID;
    else if (event.type == SDL_EVENT_PEN_DOWN || event.type == SDL_EVENT_PEN_UP)
        windowId = event.ptip.windowID;
    else if (event.type == SDL_EVENT_PEN_MOTION)
        windowId = event.pmotion.windowID;
    else if (event.type == SDL_EVENT_PEN_BUTTON_DOWN || event.type == SDL_EVENT_PEN_BUTTON_UP)
        windowId = event.pbutton.windowID;
    else if (event.type == SDL_EVENT_DROP_BEGIN || event.type == SDL_EVENT_DROP_FILE || event.type == SDL_EVENT_DROP_TEXT || event.type == SDL_EVENT_DROP_COMPLETE || event.type == SDL_EVENT_DROP_POSITION)
        windowId = event.drop.windowID;
    else if (event.type >= SDL_EVENT_USER && event.type <= SDL_EVENT_LAST)
        windowId = event.user.windowID;
    else
        return nullptr;

    if (lastEventWindow == nullptr || windowId != lastEventWindow->_windowId)
        lastEventWindow = GetWindowWithId(windowId);
    return lastEventWindow;
}

SDLWindow* SDLWindow::GetWindowWithId(uint32 windowId)
{
    WindowsManager::WindowsLocker.Lock();
    for (auto win : WindowsManager::Windows)
    {
        if (win->_windowId == windowId)
            return win;
    }
    WindowsManager::WindowsLocker.Unlock();
    return nullptr;
}

void SDLWindow::HandleEvent(SDL_Event& event)
{
    if (_isClosing)
        return;

    // NOTE: Update window handling in SDLWindow::GetWindowFromEvent when any new events are added here
    switch (event.type)
    {
    case SDL_EVENT_WINDOW_EXPOSED:
    {
        // Check if window is during resizing
        if (_swapChain && !_isClosing)
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
        return;
    }
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    {
        Close(ClosingReason::User);
        return;
    }
    case SDL_EVENT_WINDOW_DESTROYED:
    {
#if USE_EDITOR && PLATFORM_WINDOWS
        // Disable file dropping
        if (_settings.AllowDragAndDrop)
        {
            const auto result = RevokeDragDrop((HWND)_handle);
            if (result != S_OK)
                LOG(Warning, "Window drag and drop service error: 0x{0:x}:{1}", result, 2);
        }
#endif

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
#if PLATFORM_LINUX
        if (SDLPlatform::UsesX11())
        {
            // X11 doesn't report any mouse events when mouse is over the caption area, send a simulated event instead...
            Float2 mousePosition;
            auto buttons = SDL_GetGlobalMouseState(&mousePosition.X, &mousePosition.Y);
            if ((buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0)
                SDLPlatform::CheckWindowDragging(this, WindowHitCodes::Caption);
        }
#endif
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
        
#if PLATFORM_WINDOWS
        if (!_settings.HasBorder && _settings.HasSizingFrame)
        {
            // Borderless window doesn't maximize properly, manually force the window into correct location and size
            SDL_Rect rect;
            SDL_DisplayID display = SDL_GetDisplayForWindow(_window);
            SDL_GetDisplayUsableBounds(display, &rect); // Excludes taskbar etc.

            // Check if the window actually clips past the display work region
            auto pos = GetPosition();
            auto size = GetClientSize();
            if (pos.X < rect.x || pos.Y < rect.y || size.X > rect.w || size.Y > rect.h)
            {
                // Disable resizable flag so SDL updates the client rectangle to expected values during WM_NCCALCSIZE
                SDL_SetWindowResizable(_window, SDL_FALSE);
                SDL_MaximizeWindow(_window);

                // Set the internal floating window rectangle to expected position
                SetClientBounds(Rectangle((float)rect.x, (float)rect.y, (float)rect.w, (float)rect.h));

                // Flush and handle the events again
                ::SetWindowPos((HWND)_handle, HWND_TOP, rect.x, rect.y, rect.w, rect.h, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
                SDL_PumpEvents();

                // Restore previous values
                SDL_SetWindowResizable(_window, SDL_TRUE);
                SetClientBounds(_cachedClientRectangle);
            }
        }
#endif 
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
        SDL_StartTextInput(_window);
        const SDL_Rect* currentClippingRect = SDL_GetWindowMouseRect(_window);
        if (_isClippingCursor && currentClippingRect == nullptr)
        {
            SDL_Rect rect{ (int)_clipCursorRect.GetX(), (int)_clipCursorRect.GetY(), (int)_clipCursorRect.GetWidth(), (int)_clipCursorRect.GetHeight() };
            SDL_SetWindowMouseRect(_window, &rect);
        }
        return;
    }
    case SDL_EVENT_WINDOW_FOCUS_LOST:
    {
        SDL_StopTextInput(_window);
        const SDL_Rect* currentClippingRect = SDL_GetWindowMouseRect(_window);
        if (currentClippingRect != nullptr)
            SDL_SetWindowMouseRect(_window, nullptr);
        OnLostFocus();
        return;
    }
    case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
    {
        SDL_DisplayID display = SDL_GetDisplayForWindow(_window);
        float scale = SDL_GetDisplayContentScale(display);
        if (scale > 0.0f)
        {
            float oldScale = _dpiScale;
            _dpiScale = scale;
            _dpi = (int)(_dpiScale * DefaultDPI);

            int w = (int)(_cachedClientRectangle.GetWidth() * (scale / oldScale));
            int h = (int)(_cachedClientRectangle.GetHeight() * (scale / oldScale));
            SDL_SetWindowSize(_window, w, h);
            // TODO: Recalculate fonts
        }
        return;
    }
#if false
    case SDL_EVENT_DROP_BEGIN:
    {
        Focus();
        Float2 mousePosition;
        SDL_GetGlobalMouseState(&mousePosition.X, &mousePosition.Y);
        mousePosition = ScreenToClient(mousePosition);

        DragDropEffect effect;
        SDLDropTextData dropData;
        OnDragEnter(&dropData, mousePosition, effect);
        OnDragOver(&dropData, mousePosition, effect);
        return;
    }
    case SDL_EVENT_DROP_POSITION:
    {
        DragDropEffect effect = DragDropEffect::None;

        SDLDropTextData dropData;
        OnDragOver(&dropData, Float2(static_cast<float>(event.drop.x), static_cast<float>(event.drop.y)), effect);
        return;
    }
    case SDL_EVENT_DROP_FILE:
    {
        SDLDropFilesData dropData;
        dropData.Files.Add(StringAnsi(event.drop.data).ToString()); // TODO: collect multiple files at once?

        Focus();
        
        Float2 mousePosition;
        SDL_GetGlobalMouseState(&mousePosition.X, &mousePosition.Y);
        mousePosition = ScreenToClient(mousePosition);
        DragDropEffect effect = DragDropEffect::None;
        OnDragDrop(&dropData, mousePosition, effect);
        return;
    }
    case SDL_EVENT_DROP_TEXT:
    {
        SDLDropTextData dropData;
        String str = StringAnsi(event.drop.data).ToString();
        dropData.Text = StringView(str);

        Focus();
        Float2 mousePosition;
        SDL_GetGlobalMouseState(&mousePosition.X, &mousePosition.Y);
        mousePosition = ScreenToClient(mousePosition);
        DragDropEffect effect = DragDropEffect::None;
        OnDragDrop(&dropData, mousePosition, effect);
        return;
    }
    case SDL_EVENT_DROP_COMPLETE:
    {
        return;
    }
#endif
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
    {
#if !PLATFORM_WINDOWS
        OnDragLeave(); // Check for release of mouse button too?
#endif
        break;
    }
    default:
        break;
    }

    if (_settings.AllowInput)
    {
        if (SDLInput::HandleEvent(this, event))
            return;
    }

    // ignored
    if (event.type == SDL_EVENT_WINDOW_ICCPROF_CHANGED)
        return;

    //LOG(Info, "Unhandled SDL event: {0}", event.type);
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
    
    // Reused top-most windows (DockHintWindow) doesn't stay on top for some reason
    if (_settings.IsTopmost && _settings.Type != WindowType::Tooltip)
        SDL_SetWindowAlwaysOnTop(_window, SDL_TRUE);

    if (_isTrackingMouse)
    {
        if (SDL_CaptureMouse(SDL_TRUE) != 0)
            LOG(Warning, "Show SDL_CaptureMouse: {0}", String(SDL_GetError()));
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

    if (isBorderless)
    {
        SDL_SetWindowBordered(_window, !isBorderless ? SDL_TRUE : SDL_FALSE);
        if (maximized)
            SDL_MaximizeWindow(_window);
        else
            Focus();
    }
    else
    {
        SDL_SetWindowBordered(_window, !isBorderless ? SDL_TRUE : SDL_FALSE);
        if (maximized)
            SDL_MaximizeWindow(_window);
        else
            Focus();
    }

    CheckForWindowResize();
}

void SDLWindow::Restore()
{
    SDL_RestoreWindow(_window);
}

bool SDLWindow::IsClosed() const
{
    return _handle == nullptr;
}

bool SDLWindow::IsForegroundWindow() const
{
    SDL_WindowFlags flags = SDL_GetWindowFlags(_window);
    return (flags & SDL_WINDOW_MOUSE_FOCUS) != 0 || (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
}

void SDLWindow::BringToFront(bool force)
{
    auto activateWhenRaised = SDL_GetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_RAISED);
    SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_RAISED, "0");
    SDL_RaiseWindow(_window);
    SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_RAISED, activateWhenRaised);
}

void SDLWindow::SetClientBounds(const Rectangle& clientArea)
{
    SDL_SetWindowPosition(_window, (int)clientArea.GetLeft(), (int)clientArea.GetTop());
    SDL_SetWindowSize(_window, (int)clientArea.GetWidth(), (int)clientArea.GetHeight());
}

void SDLWindow::SetPosition(const Float2& position)
{
    int top, left, bottom, right;
    SDL_GetWindowBordersSize(_window, &top, &left, &bottom, &right);

    // The position is relative to the parent window
    Float2 newPosition = position;
    SDLWindow* parent = (_settings.Type == WindowType::Tooltip || _settings.Type == WindowType::Popup) ? _settings.Parent : nullptr;
    if (parent != nullptr)
        newPosition -= parent->GetClientPosition();

    SDL_SetWindowPosition(_window, static_cast<int>(newPosition.X) + left, static_cast<int>(newPosition.Y) + top);
}

void SDLWindow::SetClientPosition(const Float2& position)
{
    // The position is relative to the parent window
    Float2 newPosition = position;
    SDLWindow* parent = (_settings.Type == WindowType::Tooltip || _settings.Type == WindowType::Popup) ? _settings.Parent : nullptr;
    if (parent != nullptr)
        newPosition -= parent->GetClientPosition();

    SDL_SetWindowPosition(_window, static_cast<int>(newPosition.X), static_cast<int>(newPosition.Y));
}

void SDLWindow::SetIsFullscreen(bool isFullscreen)
{
    SDL_SetWindowFullscreen(_window, isFullscreen ? SDL_TRUE : SDL_FALSE);
    if (!isFullscreen)
    {
        // The window is set to always-on-top for some reason when leaving fullscreen
        SDL_SetWindowAlwaysOnTop(_window, SDL_FALSE);
    }

    WindowBase::SetIsFullscreen(isFullscreen);
}

Float2 SDLWindow::GetPosition() const
{
    int top, left, bottom, right;
    SDL_GetWindowBordersSize(_window, &top, &left, &bottom, &right);

    // The position is relative to the parent window
    Float2 newPosition = GetClientPosition();
    SDLWindow* parent = (_settings.Type == WindowType::Tooltip || _settings.Type == WindowType::Popup) ? _settings.Parent : nullptr;
    if (parent != nullptr)
        newPosition += parent->GetClientPosition();

    return newPosition - Float2(static_cast<float>(left), static_cast<float>(top));
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
#if PLATFORM_LINUX
    auto res1 = screenPos - GetPosition();
    auto res1b = screenPos - GetClientPosition();

    X11::Display* display = (X11::Display*)GetX11Display();
    if (display)
    {
        X11::Window window =  (X11::Window)GetX11WindowHandle();
        if (!display)
            return screenPos;
        int32 x, y;
        X11::Window child;
        X11::XTranslateCoordinates(display, X11_DefaultRootWindow(display), window, (int32)screenPos.X, (int32)screenPos.Y, &x, &y, &child);
    
        auto res2 = Float2((float)x, (float)y);
        if (Float2::DistanceSquared(res1b, res2) > 1)
            res1 = res1;
    }
#endif
    return screenPos - GetClientPosition();
}

Float2 SDLWindow::ClientToScreen(const Float2& clientPos) const
{
    int x, y;
    SDL_GetWindowPosition(_window, &x, &y);

    return clientPos + Float2(static_cast<float>(x), static_cast<float>(y));
}

void SDLWindow::FlashWindow()
{
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
    SDL_SetWindowOpacity(_window, opacity);
}

void SDLWindow::Focus()
{
    auto activateWhenRaised = SDL_GetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_RAISED);
    SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_RAISED, "1");

    // Forcing the window to focus causes issues with opening context menus while window is maximized
    //auto forceRaiseWindow = SDL_GetHint(SDL_HINT_FORCE_RAISEWINDOW);
    //SDL_SetHint(SDL_HINT_FORCE_RAISEWINDOW, "1");

    SDL_RaiseWindow(_window);

    SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_RAISED, activateWhenRaised);
    //SDL_SetHint(SDL_HINT_FORCE_RAISEWINDOW, forceRaiseWindow);
}

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
        if (SDL_CaptureMouse(SDL_TRUE) != 0)
            LOG(Warning, "SDL_CaptureMouse: {0}", String(SDL_GetError()));

        // For viewport camera mouse tracking we want to use relative mode for best precision
        if (_cursor == CursorType::Hidden)
            Input::Mouse->SetRelativeMode(true);
    }
}

void SDLWindow::EndTrackingMouse()
{
    if (!_isTrackingMouse)
        return;
    
    _isTrackingMouse = false;
    _isHorizontalFlippingMouse = false;
    _isVerticalFlippingMouse = false;

    if (SDL_CaptureMouse(SDL_FALSE) != 0)
        LOG(Warning, "SDL_CaptureMouse: {0}", String(SDL_GetError()));

    //SDL_SetWindowGrab(_window, SDL_FALSE);
    Input::Mouse->SetRelativeMode(false);
}

void SDLWindow::StartClippingCursor(const Rectangle& bounds)
{
    if (!IsFocused())
        return;

    _isClippingCursor = true;
    SDL_Rect rect{ (int)bounds.GetX(), (int)bounds.GetY(), (int)bounds.GetWidth(), (int)bounds.GetHeight() };
    SDL_SetWindowMouseRect(_window, &rect);
}

void SDLWindow::EndClippingCursor()
{
    if (!_isClippingCursor)
        return;

    _isClippingCursor = false;
    SDL_SetWindowMouseRect(_window, nullptr);
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

void SDLWindow::UpdateCursor() const
{
    static SDL_Cursor* cursors[SDL_NUM_SYSTEM_CURSORS] = { nullptr };

    if (_cursor == CursorType::Hidden)
    {
        SDL_HideCursor();

        if (_isTrackingMouse)
            Input::Mouse->SetRelativeMode(true);
        return;
    }
    SDL_ShowCursor();
    //if (_isTrackingMouse)
    //    Input::Mouse->SetRelativeMode(false);
    
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

    if (cursors[index] == nullptr)
        cursors[index] = SDL_CreateSystemCursor(static_cast<SDL_SystemCursor>(index));
    SDL_SetCursor(cursors[index]);
}

#endif

