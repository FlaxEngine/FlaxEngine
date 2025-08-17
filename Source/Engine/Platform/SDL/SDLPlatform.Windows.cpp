// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_SDL && PLATFORM_WINDOWS

#include "SDLPlatform.h"
#include "SDLInput.h"

#include "Engine/Core/Collections/Array.h"
#include "Engine/Platform/WindowsManager.h"
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/Engine.h"

#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_system.h>
#include <SDL3/SDL_timer.h>

#if USE_EDITOR
#include <oleidl.h>
#endif

namespace WinImpl
{
    Window* DraggedWindow;
    Float2 DraggedWindowStartPosition = Float2::Zero;
    Float2 DraggedWindowMousePosition = Float2::Zero;
    Float2 DraggedWindowSize = Float2::Zero;
}

// The events for releasing the mouse during window dragging are missing, handle the mouse release event here
bool SDLCALL SDLPlatform::EventMessageHook(void* userdata, MSG* msg)
{
    if (msg->message == WM_NCLBUTTONDOWN)
    {
        Window* window = WindowsManager::GetByNativePtr(msg->hwnd);
        Float2 mousePosition(static_cast<float>(static_cast<LONG>(WINDOWS_GET_X_LPARAM(msg->lParam))), static_cast<float>(static_cast<LONG>(WINDOWS_GET_Y_LPARAM(msg->lParam))));

        WinImpl::DraggedWindow = window;
        WinImpl::DraggedWindowStartPosition = window->GetClientPosition();
        WinImpl::DraggedWindowMousePosition = mousePosition - WinImpl::DraggedWindowStartPosition;
        WinImpl::DraggedWindowSize = window->GetClientSize();

        bool result = false;
        WindowHitCodes hit = static_cast<WindowHitCodes>(msg->wParam);
        window->OnHitTest(mousePosition, hit, result);
        //if (result && hit != WindowHitCodes::Caption)
        //    return false;

        if (hit == WindowHitCodes::Caption)
        {
            SDL_Event event{ 0 };
            event.button.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
            event.button.down = true;
            event.button.timestamp = SDL_GetTicksNS();
            event.button.windowID = SDL_GetWindowID(window->GetSDLWindow());
            event.button.button = SDL_BUTTON_LEFT;
            event.button.clicks = 1;
            event.button.x = WinImpl::DraggedWindowMousePosition.X;
            event.button.y = WinImpl::DraggedWindowMousePosition.Y;

            SDL_PushEvent(&event);
        }
    }
    return true;
}

bool SDLPlatform::InitInternal()
{
    // Workaround required for handling window dragging events properly
    SDL_SetWindowsMessageHook(&EventMessageHook, nullptr);

    if (WindowsPlatform::Init())
        return true;

    return false;
}

bool SDLPlatform::EventFilterCallback(void* userdata, SDL_Event* event)
{
    Window* draggedWindow = *(Window**)userdata;
    if (draggedWindow == nullptr)
        return true;

    // When the window is being dragged on Windows, the internal message loop is blocking
    // the SDL event queue. We need to handle all relevant events in this event watch callback
    // to ensure dragging related functionality doesn't break due to engine not getting updated.
    // This also happens to fix the engine freezing during the dragging operation.

    SDLWindow* window = SDLWindow::GetWindowFromEvent(*event);
    if (event->type == SDL_EVENT_WINDOW_EXPOSED)
    {
        // The internal timer is sending exposed events every ~16ms
        Engine::OnUpdate(); // For docking updates
        Engine::OnDraw();
        return false;
    }
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        if (window)
        {
            bool result = false;
            window->OnLeftButtonHit(WindowHitCodes::Caption, result);
            //if (result)
            //    return false;
            window->HandleEvent(*event);
        }
        return false;
    }
    else if (event->type == SDL_EVENT_WINDOW_MOVED)
    {
        if (window)
        {
            window->HandleEvent(*event);

            Float2 windowSize = window->GetClientSize();
            if (WinImpl::DraggedWindowSize != windowSize)
            {
                // The window size changed while dragging, most likely due to maximized window restoring back to previous size.
                WinImpl::DraggedWindowMousePosition = WinImpl::DraggedWindowStartPosition + WinImpl::DraggedWindowMousePosition - window->GetClientPosition();
                WinImpl::DraggedWindowStartPosition = window->GetClientPosition();
                WinImpl::DraggedWindowSize = windowSize;
            }
            Float2 windowPosition = Float2(static_cast<float>(event->window.data1), static_cast<float>(event->window.data2));
            Float2 mousePosition = WinImpl::DraggedWindowMousePosition;

            // Generate mouse movement events while dragging the window around
            SDL_Event mouseMovedEvent{ 0 };
            mouseMovedEvent.motion.type = SDL_EVENT_MOUSE_MOTION;
            mouseMovedEvent.motion.windowID = SDL_GetWindowID(WinImpl::DraggedWindow->GetSDLWindow());
            mouseMovedEvent.motion.timestamp = SDL_GetTicksNS();
            mouseMovedEvent.motion.state = SDL_BUTTON_LEFT;
            mouseMovedEvent.motion.x = mousePosition.X;
            mouseMovedEvent.motion.y = mousePosition.Y;
            window->HandleEvent(mouseMovedEvent);
        }
        return false;
    }
    if (window)
        window->HandleEvent(*event);
    
    return false;
}

void SDLPlatform::PreHandleEvents()
{
    SDL_AddEventWatch(EventFilterCallback, &WinImpl::DraggedWindow);
}

void SDLPlatform::PostHandleEvents()
{
    SDL_RemoveEventWatch(EventFilterCallback, &WinImpl::DraggedWindow);

    // Handle window dragging release here
    if (WinImpl::DraggedWindow != nullptr)
    {
        Float2 mousePosition;
        auto buttons = SDL_GetGlobalMouseState(&mousePosition.X, &mousePosition.Y);

        // Send simulated mouse up event
        SDL_Event buttonUpEvent { 0 };
        buttonUpEvent.motion.type = SDL_EVENT_MOUSE_BUTTON_UP;
        buttonUpEvent.button.down = false;
        buttonUpEvent.motion.windowID = SDL_GetWindowID(WinImpl::DraggedWindow->GetSDLWindow());
        buttonUpEvent.motion.timestamp = SDL_GetTicksNS();
        buttonUpEvent.motion.state = SDL_BUTTON_LEFT;
        buttonUpEvent.button.clicks = 1;
        buttonUpEvent.motion.x = mousePosition.X;
        buttonUpEvent.motion.y = mousePosition.Y;
        WinImpl::DraggedWindow->HandleEvent(buttonUpEvent);
        WinImpl::DraggedWindow = nullptr;
    }
}

bool SDLWindow::HandleEventInternal(SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_EVENT_WINDOW_DESTROYED:
    {
#if USE_EDITOR
        // Disable file dropping
        if (_settings.AllowDragAndDrop)
        {
            const auto result = RevokeDragDrop((HWND)_handle);
            if (result != S_OK)
                LOG(Warning, "Window drag and drop service error: 0x{0:x}:{1}", result, 2);
        }
#endif
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        if (WinImpl::DraggedWindow != nullptr && WinImpl::DraggedWindow->_windowId != event.button.windowID)
        {
            // Send the button event to dragged window as well
            Float2 mousePos = ClientToScreen({ event.button.x, event.button.y });
            Float2 clientPos = WinImpl::DraggedWindow->ScreenToClient(mousePos);

            SDL_Event event2 = event;
            event2.button.windowID = WinImpl::DraggedWindow->_windowId;
            event2.button.x = clientPos.X;
            event2.button.y = clientPos.Y;

            SDLInput::HandleEvent(WinImpl::DraggedWindow, event2);
        }
        break;
    }
    default:
        break;
    }
    
    return false;
}

bool SDLPlatform::UsesWindows()
{
    return true;
}

bool SDLPlatform::UsesWayland()
{
    return false;
}

bool SDLPlatform::UsesX11()
{
    return false;
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

void SDLPlatform::SetHighDpiAwarenessEnabled(bool enable)
{
    // Other supported values: "permonitor", "permonitorv2"
    SDL_SetHint("SDL_WINDOWS_DPI_AWARENESS", enable ? "system" : "unaware");
}

DragDropEffect SDLWindow::DoDragDrop(const StringView& data, const Float2& offset, Window* dragSourceWindow)
{
    Show();
    return DragDropEffect::None;
}

#endif
