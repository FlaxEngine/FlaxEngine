// Copyright (c) Wojciech Figat. All rights reserved.

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

#if PLATFORM_WINDOWS
typedef WindowsPlatform SDLPlatformBase;
#elif PLATFORM_LINUX
typedef LinuxPlatform SDLPlatformBase;
#elif PLATFORM_MAC
typedef MacPlatform SDLPlatformBase;
#else
static_assert(false, "Unsupported SDL platform.");
#endif

/// <summary>
/// The SDL platform implementation and application management utilities.
/// </summary>
API_CLASS(Static, Tag="NoTypeInitializer")
class FLAXENGINE_API SDLPlatform : public SDLPlatformBase
{
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
    static String GetDisplayServer();
#endif
    static bool UsesWindows();
    static bool UsesWayland();
    static bool UsesX11();

    /// <summary>
    /// Returns true if system provides decorations for windows.
    /// </summary>
    API_PROPERTY() static bool SupportsNativeDecorations();

    /// <summary>
    /// Returns true if system provides support for native window dragging events.
    /// </summary>
    API_PROPERTY() static bool SupportsNativeDecorationDragging();

public:
    // [PlatformBase]
    static bool Init();
    static void LogInfo();
    static void Tick();
    static void SetHighDpiAwarenessEnabled(bool enable);
#if !PLATFORM_WINDOWS
    static BatteryInfo GetBatteryInfo();
#endif
    static int32 GetDpi();
#if PLATFORM_LINUX
    static String GetUserLocaleName();
#endif
    static bool CanOpenUrl(const StringView& url);
    static void OpenUrl(const StringView& url);
    static Float2 GetMousePosition();
    static void SetMousePosition(const Float2& pos);
    static Float2 GetDesktopSize();
    static Rectangle GetMonitorBounds(const Float2& screenPos);
    static Rectangle GetVirtualDesktopBounds();
    static Window* CreateWindow(const CreateWindowSettings& settings);
#if !PLATFORM_WINDOWS
    static int32 CreateProcess(CreateProcessSettings& settings);
#endif
};

#endif
