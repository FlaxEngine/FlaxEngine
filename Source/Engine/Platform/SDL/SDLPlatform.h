// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_SDL

#include "Engine/Platform/Base/Enums.h"
#if PLATFORM_WINDOWS
#include "Engine/Platform/Windows/WindowsPlatform.h"
typedef struct tagMSG MSG;
#elif PLATFORM_LINUX
#include "Engine/Platform/Linux/LinuxPlatform.h"
union _XEvent;
#elif PLATFORM_MAC
#include "Engine/Platform/Mac/MacPlatform.h"
#else
static_assert(false, "Unsupported Platform");
#endif

class SDLWindow;
union SDL_Event;

/// <summary>
/// The SDL platform implementation and application management utilities.
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
#elif PLATFORM_MAC
: public MacPlatform
{
    using base = MacPlatform;
#else
{
    static_assert(false, "Unsupported Platform");
#endif
    friend SDLWindow;

private:
    static bool InitInternal();
#if PLATFORM_LINUX
    static bool InitX11(void* display);
#endif
    static bool HandleEvent(SDL_Event& event);
#if PLATFORM_WINDOWS
    static bool EventMessageHook(void* userdata, MSG* msg);
    static bool EventFilterCallback(void* userdata, SDL_Event* event);
#elif PLATFORM_LINUX
    static bool X11EventHook(void* userdata, _XEvent* xevent);
#elif PLATFORM_MAC
    static bool EventFilterCallback(void* userdata, SDL_Event* event);
#endif
    static void PreHandleEvents();
    static void PostHandleEvents();

public:
#if PLATFORM_LINUX
    static void* GetXDisplay();
#endif
    static bool UsesWindows();
    static bool UsesWayland();
    static bool UsesX11();

public:

    // [PlatformBase]
    static bool Init();
    static void LogInfo();
    static void Tick();
    static String GetDisplayServer();
    static void SetHighDpiAwarenessEnabled(bool enable);
    static BatteryInfo GetBatteryInfo();
    static int32 GetDpi();
    static String GetUserLocaleName();
    static void OpenUrl(const StringView& url);
    static Float2 GetMousePosition();
    static void SetMousePosition(const Float2& pos);
    static Float2 GetDesktopSize();
    static Rectangle GetMonitorBounds(const Float2& screenPos);
    static Rectangle GetVirtualDesktopBounds();
    static Window* CreateWindow(const CreateWindowSettings& settings);
};

#endif
