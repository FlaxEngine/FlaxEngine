// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if PLATFORM_SDL

#include "SDLPlatform.h"
#include "SDLWindow.h"
#include "Engine/Core/Log.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Platform/BatteryInfo.h"
#include "Engine/Platform/WindowsManager.h"
#include "Engine/Platform/SDL/SDLInput.h"
#include "Engine/Engine/Engine.h"

#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_misc.h>
#include <SDL3/SDL_power.h>
#include <SDL3/SDL_revision.h>
#include <SDL3/SDL_system.h>
#include <SDL3/SDL_version.h>
#include <SDL3/SDL_locale.h>

#if PLATFORM_LINUX
#include "Engine/Engine/CommandLine.h"
#endif

#define DefaultDPI 96

namespace
{
    int32 SystemDpi = 96;
    String UserLocale("en");
}

bool SDLPlatform::Init()
{
#if PLATFORM_LINUX
    if (CommandLine::Options.X11)
        SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "x11", SDL_HINT_OVERRIDE);
    else if (CommandLine::Options.Wayland)
        SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "wayland", SDL_HINT_OVERRIDE);
    else
    {
        // Override the X11 preference when running in Wayland session
        String waylandDisplayEnv;
        if (!GetEnvironmentVariable(String("WAYLAND_DISPLAY"), waylandDisplayEnv))
            SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "wayland", SDL_HINT_OVERRIDE);
    }
#endif

#if PLATFORM_LINUX
    // TODO: This should be read from the platform configuration (needed for desktop icon handling)
#if USE_EDITOR
    SDL_SetHint(SDL_HINT_APP_ID, StringAnsi("com.FlaxEngine.FlaxEditor").Get());
#else
    SDL_SetHint(SDL_HINT_APP_ID, StringAnsi("com.FlaxEngine.FlaxGame").Get());
#endif
#else
    SDL_SetHint(SDL_HINT_APP_ID, StringAnsi(ApplicationClassName).Get());
#endif

    SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN, "0");
    SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_RAISED, "0");
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1"); // Fixes context menu focus issues when clicking unfocused menus
    SDL_SetHint(SDL_HINT_WINDOWS_ERASE_BACKGROUND_MODE, "0");
    SDL_SetHint(SDL_HINT_TIMER_RESOLUTION, "0"); // Already handled during platform initialization
    SDL_SetHint("SDL_BORDERLESS_RESIZABLE_STYLE", "1"); // Allow borderless windows to be resizable on Windows
    //SDL_SetHint("SDL_BORDERLESS_WINDOWED_STYLE", "1"); 
    
    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_WARP_MOTION, "0");
    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_CURSOR_VISIBLE, "1"); // Needed for tracking mode
    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_CENTER, "0"); // Relative mode can be active when cursor is shown and clipped
    SDL_SetHint(SDL_HINT_MOUSE_DOUBLE_CLICK_RADIUS, "8"); // Reduce the default mouse double-click radius

    //SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1"); // Disables raw mouse input
    SDL_SetHint(SDL_HINT_WINDOWS_RAW_KEYBOARD, "1");

    SDL_SetHint(SDL_HINT_VIDEO_WAYLAND_SCALE_TO_DISPLAY, "1");

    //if (InitInternal())
    //    return true;

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        Platform::Fatal(String::Format(TEXT("Failed to initialize SDL: {0}."), String(SDL_GetError())));

    int localesCount = 0;
    auto locales = SDL_GetPreferredLocales(&localesCount);
    for (int i = 0; i < localesCount; i++)
    {
        auto language = StringAnsiView(locales[i]->language);
        auto country = StringAnsiView(locales[i]->country);
        if (language.StartsWith("en"))
        {
            if (country != nullptr)
                UserLocale = String::Format(TEXT("{0}-{1}"), String(language), String(locales[i]->country));
            else
                UserLocale = String(language);
            break;
        }
    }
    SDL_free(locales);

    if (InitInternal())
        return true;

    if (UsesWindows() || UsesX11())
    {
        // Disable SDL clipboard support
        SDL_SetEventEnabled(SDL_EVENT_CLIPBOARD_UPDATE, false);

        // Disable SDL drag and drop support
        SDL_SetEventEnabled(SDL_EVENT_DROP_FILE, false);
        SDL_SetEventEnabled(SDL_EVENT_DROP_TEXT, false);
        SDL_SetEventEnabled(SDL_EVENT_DROP_BEGIN, false);
        SDL_SetEventEnabled(SDL_EVENT_DROP_COMPLETE, false);
        SDL_SetEventEnabled(SDL_EVENT_DROP_POSITION, false);
    }

    SDLInput::Init();
    SDLWindow::Init();

    SystemDpi = (int)(SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay()) * DefaultDPI);

    //SDL_StartTextInput(); // TODO: Call this only when text input is expected (shows virtual keyboard in some cases)

    return base::Init();
}

void SDLPlatform::LogInfo()
{
    base::LogInfo();

    const int32 runtimeVersion = SDL_GetVersion();
    LOG(Info, "Using SDL version {}.{}.{} ({}), runtime: {}.{}.{} ({})",
        SDL_VERSIONNUM_MAJOR(SDL_VERSION), SDL_VERSIONNUM_MINOR(SDL_VERSION), SDL_VERSIONNUM_MICRO(SDL_VERSION), String(SDL_REVISION),
        SDL_VERSIONNUM_MAJOR(runtimeVersion), SDL_VERSIONNUM_MINOR(runtimeVersion), SDL_VERSIONNUM_MICRO(runtimeVersion), String(SDL_GetRevision()));

    LOG(Info, "SDL video driver: {}", String(SDL_GetCurrentVideoDriver()));
}

void SDLPlatform::Tick()
{
    SDLInput::Update();

    PreHandleEvents();
    
    SDL_PumpEvents();
    SDL_Event events[32];
    int count = SDL_PeepEvents(events, SDL_arraysize(events), SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST);
    for (int i = 0; i < count; ++i)
    {
        SDLWindow* window = SDLWindow::GetWindowFromEvent(events[i]);
        if (window)
            window->HandleEvent(events[i]);
        else if (events[i].type >= SDL_EVENT_JOYSTICK_AXIS_MOTION && events[i].type <= SDL_EVENT_GAMEPAD_STEAM_HANDLE_UPDATED)
            SDLInput::HandleEvent(nullptr, events[i]);
        else
            HandleEvent(events[i]);
    }

    PostHandleEvents();
}

bool SDLPlatform::HandleEvent(SDL_Event& event)
{
    return true;
}

String SDLPlatform::GetDisplayServer()
{
#if PLATFORM_LINUX
    String driver(SDL_GetCurrentVideoDriver());
    if (driver.Length() > 0)
        driver[0] = StringUtils::ToUpper(driver[0]);
    return driver;
#else
    return String::Empty;
#endif
}

BatteryInfo SDLPlatform::GetBatteryInfo()
{
    BatteryInfo info;
    int percentage;
    SDL_PowerState powerState = SDL_GetPowerInfo(nullptr, &percentage);

    if (percentage < 0)
        info.BatteryLifePercent = 1.0f;
    else
        info.BatteryLifePercent = (float)percentage / 100.0f;

    switch (powerState)
    {
    case SDL_POWERSTATE_CHARGING:
        info.State = BatteryInfo::States::BatteryCharging;
        break;
    case SDL_POWERSTATE_ON_BATTERY:
        info.State = BatteryInfo::States::BatteryDischarging;
        break;
    case SDL_POWERSTATE_CHARGED:
        info.State = BatteryInfo::States::Connected;
        break;
    default:
        info.State = BatteryInfo::States::Unknown;
    }
    return info;
}

int32 SDLPlatform::GetDpi()
{
    return SystemDpi;
}

String SDLPlatform::GetUserLocaleName()
{
    return UserLocale;
}

void SDLPlatform::OpenUrl(const StringView& url)
{
    StringAnsi urlStr(url);
    SDL_OpenURL(urlStr.GetText());
}

Float2 SDLPlatform::GetMousePosition()
{
    Float2 pos;
    if (UsesWayland())
    {
        // Wayland doesn't support reporting global mouse position,
        // use the last known reported position we got from received window events.
        pos = Input::GetMouseScreenPosition();
        //if (!SDL_GetGlobalMouseState(&pos.X, &pos.Y))
        //    LOG(Error, "SDL_GetGlobalMouseState() failed");
    }
    else if (UsesX11())
        SDL_GetGlobalMouseState(&pos.X, &pos.Y);
    else
        pos = Input::GetMouseScreenPosition();
    return pos;
}

void SDLPlatform::SetMousePosition(const Float2& pos)
{
    SDL_WarpMouseGlobal(pos.X, pos.Y);
}

Float2 SDLPlatform::GetDesktopSize()
{
    SDL_Rect rect;
    SDL_GetDisplayBounds(SDL_GetPrimaryDisplay(), &rect);
    return Float2(static_cast<float>(rect.w), static_cast<float>(rect.h));
}

Rectangle SDLPlatform::GetMonitorBounds(const Float2& screenPos)
{
    SDL_Point point{ (int32)screenPos.X, (int32)screenPos.Y };
    SDL_DisplayID display = SDL_GetDisplayForPoint(&point);
    SDL_Rect rect;
    SDL_GetDisplayBounds(display, &rect);
    return Rectangle(static_cast<float>(rect.x), static_cast<float>(rect.y), static_cast<float>(rect.w), static_cast<float>(rect.h));
}

Rectangle SDLPlatform::GetVirtualDesktopBounds()
{
    int count;
    const SDL_DisplayID* displays = SDL_GetDisplays(&count);
    if (displays == nullptr)
        return Rectangle::Empty;

    Rectangle bounds = Rectangle::Empty;
    for (int i = 0; i < count; i++)
    {
        SDL_DisplayID display = displays[i];
        SDL_Rect rect;
        SDL_GetDisplayBounds(display, &rect);
        bounds = Rectangle::Union(bounds, Rectangle(static_cast<float>(rect.x), static_cast<float>(rect.y), static_cast<float>(rect.w), static_cast<float>(rect.h)));
    }
    SDL_free((void*)displays);
    return bounds;
}

Window* SDLPlatform::CreateWindow(const CreateWindowSettings& settings)
{
    return New<SDLWindow>(settings);
}

#endif
