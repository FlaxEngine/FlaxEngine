// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.


#if PLATFORM_SDL && PLATFORM_WINDOWS

#include "SDLPlatform.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Platform/WindowsManager.h"
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"

#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_system.h>

// The events for releasing the mouse during window dragging are missing, handle the mouse release event here
SDL_bool SDLCALL SDLPlatform::EventMessageHook(void* userdata, MSG* msg)
{
#define GET_WINDOW_WITH_HWND(window, hwnd) \
    do { \
        (window) = nullptr; \
        WindowsManager::WindowsLocker.Lock(); \
        for (int32 i = 0; i < WindowsManager::Windows.Count(); i++) \
        { \
            if (WindowsManager::Windows[i]->GetNativePtr() == (hwnd)) \
            { \
                (window) = WindowsManager::Windows[i]; \
                break; \
            } \
        } \
        WindowsManager::WindowsLocker.Unlock(); \
        ASSERT((window) != nullptr); \
    } while (false)

    if (msg->message == WM_NCLBUTTONDOWN)
    {
        Window* window;
        GET_WINDOW_WITH_HWND(window, msg->hwnd);
        
        auto hit = static_cast<WindowHitCodes>(msg->wParam);
        if (SDLPlatform::CheckWindowDragging(window, hit))
            return SDL_FALSE;
    }
    return SDL_TRUE;
#undef GET_WINDOW_WITH_HWND
}

bool SDLPlatform::InitPlatform()
{
    // Workaround required for handling window dragging events properly for DockHintWindow
    SDL_SetWindowsMessageHook(&EventMessageHook, nullptr);

    if (WindowsPlatform::Init())
        return true;

    return false;
}

void SDLPlatform::SetHighDpiAwarenessEnabled(bool enable)
{
    // Other supported values: "permonitor", "permonitorv2"
    SDL_SetHint("SDL_WINDOWS_DPI_AWARENESS", enable ? "system" : "unaware");
}

#endif
