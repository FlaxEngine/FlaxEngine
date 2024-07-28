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

#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_misc.h>
#include <SDL3/SDL_power.h>
#include <SDL3/SDL_revision.h>
#include <SDL3/SDL_system.h>
#include <SDL3/SDL_version.h>

#if PLATFORM_LINUX
#include "Engine/Engine/CommandLine.h"
#include "Engine/Platform/MessageBox.h"
#include <SDL3/SDL_messagebox.h>
#endif

#define DefaultDPI 96

uint32 SDLPlatform::DraggedWindowId = 0;

namespace
{
    int32 SystemDpi = 96;
}

bool SDLPlatform::Init()
{
#if PLATFORM_LINUX
    if (CommandLine::Options.X11)
        SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "x11", SDL_HINT_OVERRIDE);
    else if (CommandLine::Options.Wayland)
        SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "wayland", SDL_HINT_OVERRIDE);
    else
        SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "wayland", SDL_HINT_OVERRIDE);
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
    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_WARP_MOTION, "0");
    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_CURSOR_VISIBLE, "1"); // Needed for tracking mode
    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_CENTER, "1");

    //SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1"); // Disables raw mouse input
    SDL_SetHint(SDL_HINT_WINDOWS_RAW_KEYBOARD, "1");

    // Disable SDL clipboard support
    SDL_SetEventEnabled(SDL_EVENT_CLIPBOARD_UPDATE, SDL_FALSE);

    // Disable SDL drag and drop support
    SDL_SetEventEnabled(SDL_EVENT_DROP_FILE, SDL_FALSE);
    SDL_SetEventEnabled(SDL_EVENT_DROP_TEXT, SDL_FALSE);
    SDL_SetEventEnabled(SDL_EVENT_DROP_BEGIN, SDL_FALSE);
    SDL_SetEventEnabled(SDL_EVENT_DROP_COMPLETE, SDL_FALSE);
    SDL_SetEventEnabled(SDL_EVENT_DROP_POSITION, SDL_FALSE);

    if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD) < 0)
        Platform::Fatal(String::Format(TEXT("Failed to initialize SDL: {0}."), String(SDL_GetError())));

    if (InitPlatform())
        return true;

    SDLInput::Init();

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

bool SDLPlatform::CheckWindowDragging(Window* window, WindowHitCodes hit)
{
    bool handled = false;
    window->OnLeftButtonHit(hit, handled);
    if (handled)
    {
        DraggedWindowId = window->_windowId;
        LOG(Info, "Dragging: {}", window->_settings.Title);

        String dockHintWindow("DockHint.Window");
        Window* window = nullptr;
        WindowsManager::WindowsLocker.Lock();
        for (int32 i = 0; i < WindowsManager::Windows.Count(); i++)
        {
            if (WindowsManager::Windows[i]->_title.Compare(dockHintWindow) == 0)
                //if (WindowsManager::Windows[i]->_windowId == DraggedWindowId)
            {
                window = WindowsManager::Windows[i];
                break;
            }
        }
        WindowsManager::WindowsLocker.Unlock();

        Float2 mousePos;
        auto buttons = SDL_GetGlobalMouseState(&mousePos.X, &mousePos.Y);
        if (window != nullptr)
        {
            /*int top, left, bottom, right;
            SDL_GetWindowBordersSize(window->_window, &top, &left, &bottom, &right);
            mousePos += Float2(left, -top);
            Input::Mouse->OnMouseDown(mousePos, MouseButton::Left, window);*/
        }
    }
    return handled;
}

void SDLPlatform::Tick()
{
    SDLInput::Update();

    if (DraggedWindowId != 0)
    {
        Float2 mousePos;
        auto buttons = SDL_GetGlobalMouseState(&mousePos.X, &mousePos.Y);
        if (!(buttons & SDL_BUTTON(SDL_BUTTON_LEFT)))
        {
            Window* window = nullptr;
            WindowsManager::WindowsLocker.Lock();
            for (int32 i = 0; i < WindowsManager::Windows.Count(); i++)
            {
                if (WindowsManager::Windows[i]->_windowId == DraggedWindowId)
                {
                    window = WindowsManager::Windows[i];
                    break;
                }
            }
            WindowsManager::WindowsLocker.Unlock();

            if (window != nullptr)
            {
                int top, left, bottom, right;
                SDL_GetWindowBordersSize(window->_window, &top, &left, &bottom, &right);
                mousePos += Float2(static_cast<float>(left), static_cast<float>(-top));
                Input::Mouse->OnMouseUp(mousePos, MouseButton::Left, window);
            }
            DraggedWindowId = 0;
        }
        else
        {
#if PLATFORM_LINUX
            String dockHintWindow("DockHint.Window");
            Window* window = nullptr;
            WindowsManager::WindowsLocker.Lock();
            for (int32 i = 0; i < WindowsManager::Windows.Count(); i++)
            {
                if (WindowsManager::Windows[i]->_title.Compare(dockHintWindow) == 0)
                //if (WindowsManager::Windows[i]->_windowId == DraggedWindowId)
                {
                    window = WindowsManager::Windows[i];
                    break;
                }
            }
            WindowsManager::WindowsLocker.Unlock();

            if (window != nullptr)
            {
                int top, left, bottom, right;
                SDL_GetWindowBordersSize(window->_window, &top, &left, &bottom, &right);
                mousePos += Float2(static_cast<float>(left), static_cast<float>(-top));
                Input::Mouse->OnMouseMove(mousePos, window);
            }
#endif
        }
    }

    SDL_PumpEvents();
    SDL_Event events[32];
    int count;
    while ((count = SDL_PeepEvents(events, SDL_arraysize(events), SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST)))
    {
        for (int i = 0; i < count; ++i)
        {
            SDLWindow* window = SDLWindow::GetWindowFromEvent(events[i]);
            if (window)
                window->HandleEvent(events[i]);
            else if (events[i].type >= SDL_EVENT_JOYSTICK_AXIS_MOTION && events[i].type <= SDL_EVENT_GAMEPAD_STEAM_HANDLE_UPDATED)
                SDLInput::HandleEvent(nullptr, events[i]);
            else
                SDLPlatform::HandleEvent(events[i]);
        }
        SDL_PumpEvents();
    }
}

bool SDLPlatform::HandleEvent(SDL_Event& event)
{
    return true;
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

void SDLPlatform::OpenUrl(const StringView& url)
{
    StringAnsi urlStr(url);
    SDL_OpenURL(urlStr.GetText());
}

Float2 SDLPlatform::GetMousePosition()
{
    Float2 pos;
    SDL_GetGlobalMouseState(&pos.X, &pos.Y);
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
    return bounds;
}

Window* SDLPlatform::CreateWindow(const CreateWindowSettings& settings)
{
    return New<SDLWindow>(settings);
}

#if !PLATFORM_LINUX

bool SDLPlatform::UsesWayland()
{
    return false;
}

bool SDLPlatform::UsesXWayland()
{
    return false;
}

bool SDLPlatform::UsesX11()
{
    return false;
}

#endif

#if PLATFORM_LINUX
DialogResult MessageBox::Show(Window* parent, const StringView& text, const StringView& caption, MessageBoxButtons buttons, MessageBoxIcon icon)
{
    StringAnsi textAnsi(text);
    StringAnsi captionAnsi(caption);

    SDL_MessageBoxData data;
    SDL_MessageBoxButtonData dataButtons[3];
    data.window = parent ? static_cast<SDLWindow*>(parent)->_window : nullptr;
    data.title = captionAnsi.GetText();
    data.message = textAnsi.GetText();
    data.colorScheme = nullptr;

    switch (icon)
    {
    case MessageBoxIcon::Error:
    case MessageBoxIcon::Hand:
    case MessageBoxIcon::Stop:
        data.flags |= SDL_MESSAGEBOX_ERROR;
        break;
    case MessageBoxIcon::Asterisk:
    case MessageBoxIcon::Information:
    case MessageBoxIcon::Question:
        data.flags |= SDL_MESSAGEBOX_INFORMATION;
        break;
    case MessageBoxIcon::Exclamation:
    case MessageBoxIcon::Warning:
        data.flags |= SDL_MESSAGEBOX_WARNING;
        break;
    default:
        break;
    }

    switch (buttons)
    {
    case MessageBoxButtons::AbortRetryIgnore:
        dataButtons[0] =
        {
            SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            (int)DialogResult::Abort,
            "Abort"
        };
        dataButtons[1] =
        {
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
            (int)DialogResult::Retry,
            "Retry"
        };
        dataButtons[2] =
        {
            0,
            (int)DialogResult::Ignore,
            "Ignore"
        };
        data.numbuttons = 3;
        break;
    case MessageBoxButtons::OK:
        dataButtons[0] =
        {
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            (int)DialogResult::OK,
            "OK"
        };
        data.numbuttons = 1;
        break;
    case MessageBoxButtons::OKCancel:
        dataButtons[0] =
        {
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
            (int)DialogResult::OK,
            "OK"
        };
        dataButtons[1] =
        {
            SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            (int)DialogResult::Cancel,
            "Cancel"
        };
        data.numbuttons = 2;
        break;
    case MessageBoxButtons::RetryCancel:
        dataButtons[0] =
        {
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
            (int)DialogResult::Retry,
            "Retry"
        };
        dataButtons[1] =
        {
            SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            (int)DialogResult::Cancel,
            "Cancel"
        };
        data.numbuttons = 2;
        break;
    case MessageBoxButtons::YesNo:
        dataButtons[0] =
        {
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
            (int)DialogResult::Yes,
            "Yes"
        };
        dataButtons[1] =
        {
            SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            (int)DialogResult::No,
            "No"
        };
        data.numbuttons = 2;
        break;
    case MessageBoxButtons::YesNoCancel:
    {
        dataButtons[0] =
        {
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
            (int)DialogResult::Yes,
            "Yes"
        };
        dataButtons[1] =
        {
            0,
            (int)DialogResult::No,
            "No"
        };
        dataButtons[2] =
        {
            SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            (int)DialogResult::Cancel,
            "Cancel"
        };
        data.numbuttons = 3;
        break;
    }
    default:
        break;
    }
    data.buttons = dataButtons;

    int result = -1;
    if (SDL_ShowMessageBox(&data, &result) != 0)
    {
#if PLATFORM_LINUX
        // Fallback to native messagebox implementation in case some system fonts are missing
        if (SDLPlatform::UsesX11())
        {
            LOG(Warning, "Failed to show SDL message box: {0}", String(SDL_GetError()));
            return ShowFallback(parent, text, caption, buttons, icon);
        }
#endif
        LOG(Error, "Failed to show SDL message box: {0}", String(SDL_GetError()));
        return DialogResult::Abort;
    }
    if (result < 0)
        return DialogResult::None;
    return (DialogResult)result;
}
#endif

#endif
