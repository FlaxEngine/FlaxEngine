// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_SDL

#include "Engine/Platform/Base/Enums.h"
#if PLATFORM_WINDOWS
#include "Engine/Platform/Windows/WindowsPlatform.h"
#elif PLATFORM_LINUX
#include "Engine/Platform/Linux/LinuxPlatform.h"
#else
#endif

class SDLWindow;
union SDL_Event;
#if PLATFORM_WINDOWS
typedef struct tagMSG MSG;
#elif PLATFORM_LINUX
union _XEvent;
#endif

/// <summary>
/// The Windows platform implementation and application management utilities.
/// </summary>
class FLAXENGINE_API SDLPlatform
#if PLATFORM_WINDOWS
    : public WindowsPlatform
{
    using base = WindowsPlatform;
#elif PLATFORM_LINUX
    : public LinuxPlatform
{
    using base = LinuxPlatform;
#else
{
#endif
    friend SDLWindow;

private:
    static uint32 DraggedWindowId;

private:
    static bool InitPlatform();
#if PLATFORM_LINUX
    static bool InitPlatformX11(void* display);
#endif
    static bool HandleEvent(SDL_Event& event);
#if PLATFORM_WINDOWS
    static int __cdecl EventMessageHook(void* userdata, MSG* msg);
#elif PLATFORM_LINUX
    static int __cdecl X11EventHook(void *userdata, _XEvent *xevent);
#endif

public:
    static bool CheckWindowDragging(Window* window, WindowHitCodes hit);
#if PLATFORM_LINUX
    static void* GetXDisplay();
#endif
    static bool UsesWayland();
    static bool UsesXWayland();
    static bool UsesX11();

public:

    // [PlatformBase]
    static bool Init();
    static void LogInfo();
    static void Tick();
    static void SetHighDpiAwarenessEnabled(bool enable);
    static BatteryInfo GetBatteryInfo();
    static int32 GetDpi();
    static void OpenUrl(const StringView& url);
    static Float2 GetMousePosition();
    static void SetMousePosition(const Float2& pos);
    static Float2 GetDesktopSize();
    static Rectangle GetMonitorBounds(const Float2& screenPos);
    static Rectangle GetVirtualDesktopBounds();
    static Window* CreateWindow(const CreateWindowSettings& settings);
};

#endif
