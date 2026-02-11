// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_SDL

#include "SDLPlatform.h"
#include "SDLWindow.h"
#include "Engine/Core/Log.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Platform/BatteryInfo.h"
#include "Engine/Platform/CreateProcessSettings.h"
#include "Engine/Platform/WindowsManager.h"
#include "Engine/Platform/SDL/SDLInput.h"
#include "Engine/Engine/CommandLine.h"

#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_locale.h>
#include <SDL3/SDL_misc.h>
#include <SDL3/SDL_power.h>
#include <SDL3/SDL_process.h>
#include <SDL3/SDL_revision.h>
#include <SDL3/SDL_system.h>
#include <SDL3/SDL_version.h>

#if PLATFORM_LINUX
#include "Engine/Engine/CommandLine.h"
#endif

#define DefaultDPI 96

namespace SDLImpl
{
    int32 SystemDpi = 96;
    String UserLocale("en");
    bool WindowDecorationsSupported = true;
    bool SupportsDecorationDragging = true;
    String WaylandDisplayEnv;
    String XDGCurrentDesktop;
}

bool SDLPlatform::Init()
{
#if PLATFORM_LINUX
    bool waylandSession = false;
    if (!GetEnvironmentVariable(String("WAYLAND_DISPLAY"), SDLImpl::WaylandDisplayEnv))
        waylandSession = true;
    GetEnvironmentVariable(String("XDG_CURRENT_DESKTOP"), SDLImpl::XDGCurrentDesktop);
    
    if (CommandLine::Options.X11.IsTrue())
    {
        SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "x11", SDL_HINT_OVERRIDE);
        waylandSession = false;
    }
    else if (CommandLine::Options.Wayland.IsTrue())
        SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "wayland", SDL_HINT_OVERRIDE);
    else if (waylandSession)
    {
        // Override the X11 preference when running in Wayland session
        SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "wayland", SDL_HINT_OVERRIDE);
    }

    // Workaround for libdecor in Gnome+Wayland causing freezes when interacting with the native decorations
    if (waylandSession && SDLImpl::XDGCurrentDesktop.Compare(String("GNOME"), StringSearchCase::IgnoreCase) == 0)
    {
        SDL_SetHint(SDL_HINT_VIDEO_WAYLAND_ALLOW_LIBDECOR, "0");
        SDLImpl::WindowDecorationsSupported = false;
    }
    if (waylandSession)
        SDLImpl::SupportsDecorationDragging = false;
#endif

#if PLATFORM_LINUX
    // The name follows the .desktop entry specification, this is used to get a fallback icon on Wayland:
    // https://specifications.freedesktop.org/desktop-entry-spec/latest/file-naming.html
#if USE_EDITOR
    SDL_SetHint(SDL_HINT_APP_ID, StringAnsi("com.FlaxEngine.FlaxEditor").Get());
#else
    // TODO: This should be read from the platform configuration (needed for desktop icon handling)
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
                SDLImpl::UserLocale = String::Format(TEXT("{0}-{1}"), String(language), String(locales[i]->country));
            else
                SDLImpl::UserLocale = String(language);
            break;
        }
    }
    SDL_free(locales);

    if (InitInternal())
        return true;

#if !PLATFORM_MAC
    if (!UsesWayland())
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
#endif

    SDLInput::Init();
    SDLWindow::Init();

    SDLImpl::SystemDpi = (int)(SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay()) * DefaultDPI);

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

bool SDLPlatform::SupportsNativeDecorations()
{
    return SDLImpl::WindowDecorationsSupported;
}

bool SDLPlatform::SupportsNativeDecorationDragging()
{
    return SDLImpl::SupportsDecorationDragging;
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
    return SDLImpl::SystemDpi;
}

String SDLPlatform::GetUserLocaleName()
{
    return SDLImpl::UserLocale;
}

void SDLPlatform::OpenUrl(const StringView& url)
{
    StringAnsi urlStr(url);
    SDL_OpenURL(urlStr.GetText());
}

Float2 SDLPlatform::GetMousePosition()
{
#if PLATFORM_LINUX
    if (UsesWayland())
    {
        // Wayland doesn't support reporting global mouse position,
        // use the last known reported position we got from received window events.
        return Input::GetMouseScreenPosition();
    }
#endif
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
    SDL_free((void*)displays);
    return bounds;
}

Window* SDLPlatform::CreateWindow(const CreateWindowSettings& settings)
{
    return New<SDLWindow>(settings);
}

bool ReadStream(SDL_IOStream*& stream, char* buffer, int64 bufferLength, int64& bufferPosition, LogType logType, CreateProcessSettings& settings)
{
    bool flushBuffer = false;
    bool success = true;
    auto read = SDL_ReadIO(stream, buffer + bufferPosition, bufferLength - bufferPosition - 1);
    if (read == 0)
    {
        SDL_IOStatus status = SDL_GetIOStatus(stream);
        if (status != SDL_IO_STATUS_NOT_READY && status != SDL_IO_STATUS_EOF)
            success = false;
        if (status != SDL_IO_STATUS_NOT_READY)
        {
            stream = nullptr;
            flushBuffer = true;
        }
    }
    else
    {
        int64 startPosition = bufferPosition;
        bufferPosition += (int64)read;
        if (bufferPosition == bufferLength - 1)
        {
            flushBuffer = true;
            buffer[bufferPosition++] = '\n'; // Make sure to flush fully filled buffer
        }
        else
        {
            for (int64 i = startPosition; i < bufferPosition; ++i)
            {
                if (buffer[i] == '\n')
                {
                    flushBuffer = true;
                    break;
                }
            }
        }
    }

    if (flushBuffer)
    {
        int64 start = 0;
        for (int64 i = 0; i < bufferPosition; ++i)
        {
            if (buffer[i] != '\n')
                continue;

            String str(&buffer[start], (int32)(i - start + 1));
#if LOG_ENABLE
            if (settings.LogOutput)
                Log::Logger::Write(logType, StringView(str.Get(), str.Length() - 1));
#endif
            if (settings.SaveOutput)
                settings.Output.Add(str.Get(), str.Length());
            start = i + 1;
        }
        int64 length = bufferPosition - start;
        if (length > 0)
        {
            // TODO: Use memmove here? Overlapped memory regions with memcpy is undefined behaviour
            char temp[2048];
            Platform::MemoryCopy(temp, buffer + start, length);
            Platform::MemoryCopy(buffer, temp, length);
            bufferPosition = length;
        }
        else
            bufferPosition = 0;
    }
    return success;
}

int32 SDLPlatform::CreateProcess(CreateProcessSettings& settings)
{
    LOG(Info, "Command: {0} {1}", settings.FileName, settings.Arguments);
    if (settings.WorkingDirectory.HasChars())
        LOG(Info, "Working directory: {0}", settings.WorkingDirectory);

    int32 result = 0;
    const bool captureStdOut = settings.LogOutput || settings.SaveOutput;
    const StringAnsi cmdLine = StringAnsi::Format("\"{0}\" {1}", StringAnsi(settings.FileName), StringAnsi(settings.Arguments));
    StringAnsi workingDirectory(settings.WorkingDirectory);

    // Populate environment with current values from parent environment.
    // SDL does not populate the environment with the latest values but with a snapshot captured during initialization.
    Dictionary<String, String> envDictionary;
    GetEnvironmentVariables(envDictionary);
    SDL_Environment* env = SDL_CreateEnvironment(false);
    for (auto iter = envDictionary.Begin(); iter != envDictionary.End(); ++iter)
        SDL_SetEnvironmentVariable(env, StringAnsi(iter->Key).Get(), StringAnsi(iter->Value).Get(), true);
    for (auto iter = settings.Environment.Begin(); iter != settings.Environment.End(); ++iter)
        SDL_SetEnvironmentVariable(env, StringAnsi(iter->Key).Get(), StringAnsi(iter->Value).Get(), true);

    // Parse argument list with possible quotes included
    Array<StringAnsi> arguments;
    arguments.Add(StringAnsi(settings.FileName));
    if (CommandLine::ParseArguments(settings.Arguments, arguments))
    {
        LOG(Error, "Failed to parse arguments for process {}: '{}'", settings.FileName.Get(), settings.Arguments.Get());
        return -1;
    }
    Array<const char*> cmd;
    for (const StringAnsi& str : arguments)
        cmd.Add(str.Get());
    cmd.Add((const char*)0);

#if PLATFORM_WINDOWS
    bool background = !settings.WaitForEnd || settings.HiddenWindow; // This also hides the window on Windows
#else
    bool background = !settings.WaitForEnd;
#endif

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, cmd.Get());
    SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ENVIRONMENT_POINTER, env);
    SDL_SetBooleanProperty(props, SDL_PROP_PROCESS_CREATE_BACKGROUND_BOOLEAN, background);
    if (workingDirectory.HasChars())
        SDL_SetStringProperty(props, SDL_PROP_PROCESS_CREATE_WORKING_DIRECTORY_STRING, workingDirectory.Get());
    if (captureStdOut)
    {
        SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_APP);
        SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDERR_NUMBER, SDL_PROCESS_STDIO_APP);
    }
    SDL_Process* process = SDL_CreateProcessWithProperties(props);
    SDL_DestroyProperties(props);
    SDL_DestroyEnvironment(env);
    if (process == nullptr)
    {
        LOG(Error, "Failed to run process {}: {}", settings.FileName.Get(), String(SDL_GetError()));
        return -1;
    }

    props = SDL_GetProcessProperties(process);
    int64 pid = SDL_GetNumberProperty(props, SDL_PROP_PROCESS_PID_NUMBER, 0);
    SDL_IOStream* stdoutStream = nullptr;
    SDL_IOStream* stderrStream = nullptr;
    if (captureStdOut)
    {
        stdoutStream = static_cast<SDL_IOStream*>(SDL_GetPointerProperty(props, SDL_PROP_PROCESS_STDOUT_POINTER, nullptr));
        stderrStream = static_cast<SDL_IOStream*>(SDL_GetPointerProperty(props, SDL_PROP_PROCESS_STDERR_POINTER, nullptr));
    }

    // Handle process output in realtime
    char stdoutBuffer[2049];
    int64 stdoutPosition = 0;
    char stderrBuffer[2049];
    int64 stderrPosition = 0;
    while (stdoutStream != nullptr && stderrStream != nullptr)
    {
        if (stdoutStream != nullptr && !ReadStream(stdoutStream, stdoutBuffer, sizeof(stdoutBuffer), stdoutPosition, LogType::Info, settings))
            LOG(Warning, "Failed to read process {} stdout: {}", pid, String(SDL_GetError()));
        if (stderrStream != nullptr && !ReadStream(stderrStream, stderrBuffer, sizeof(stderrBuffer), stderrPosition, LogType::Error, settings))
            LOG(Warning, "Failed to read process {} stderr: {}", pid, String(SDL_GetError()));
        Sleep(1);
    }

    if (settings.WaitForEnd)
        SDL_WaitProcess(process, true, &result);

    SDL_DestroyProcess(process);
    return result;
}

#endif
