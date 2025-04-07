// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_UWP

#include "Engine/Platform/Platform.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Platform/MessageBox.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Platform/BatteryInfo.h"
#include "Engine/Input/Input.h"
#include "Engine/Core/Log.h"
#include "UWPWindow.h"

namespace
{
    String UserLocale, ComputerName;
    Float2 VirtualScreenSize = Float2(0.0f);
    int32 SystemDpi = 96;
}

UWPPlatformImpl* CUWPPlatform = nullptr;

namespace Impl
{
    extern UWPWindow::UWPKeyboard* Keyboard;
    extern UWPWindow::UWPMouse* Mouse;
    extern UWPWindow* Window;
}

void RunUWP()
{
    // Run engine's main
    Engine::Main(TEXT(""));
}

DialogResult MessageBox::Show(Window* parent, const StringView& text, const StringView& caption, MessageBoxButtons buttons, MessageBoxIcon icon)
{
    return (DialogResult)CUWPPlatform->ShowMessageDialog(parent ? parent->GetImpl() : nullptr, String(text).GetText(), String(caption).GetText(), (UWPPlatformImpl::MessageBoxButtons)buttons, (UWPPlatformImpl::MessageBoxIcon)icon);
}

bool UWPPlatform::Init()
{
    if (Win32Platform::Init())
        return true;

    DWORD tmp;
    Char buffer[256];

    // Get user locale string
    if (GetUserDefaultLocaleName(buffer, LOCALE_NAME_MAX_LENGTH))
    {
        UserLocale = String(buffer);
    }

    // Get computer name string
    if (GetComputerNameW(buffer, &tmp))
    {
        ComputerName = String(buffer);
    }

    SystemDpi = CUWPPlatform->GetDpi();
    Input::Mouse = Impl::Mouse = New<UWPWindow::UWPMouse>();
    Input::Keyboard = Impl::Keyboard = New<UWPWindow::UWPKeyboard>();

    return false;
}

void UWPPlatform::BeforeRun()
{
}

void UWPPlatform::Tick()
{
    PROFILE_CPU_NAMED("Application.Tick");

    // Process all messages
    CUWPPlatform->Tick();

    // Update gamepads
    auto win = Engine::MainWindow;
    if (win)
    {
        int32 count = win->GetImpl()->GetGamepadsCount();
        bool modified = false;

        // Remove old ones
        for (int32 i = count; i < Input::Gamepads.Count(); i++)
        {
            Input::Gamepads[i]->DeleteObject();
            modified = true;
        }
        Input::Gamepads.Resize(count);

        // Add new ones
        for (int32 i = Input::Gamepads.Count(); i < count && i < MAX_GAMEPADS; i++)
        {
            Input::Gamepads.Add(New<UWPWindow::UWPGamepad>(win, i));
            modified = true;
        }

        if (modified)
            Input::OnGamepadsChanged();
    }
}

void UWPPlatform::BeforeExit()
{
}

void UWPPlatform::Exit()
{
    Win32Platform::Exit();
}

BatteryInfo UWPPlatform::GetBatteryInfo()
{
    BatteryInfo info;
    SYSTEM_POWER_STATUS status;
    GetSystemPowerStatus(&status);
    info.BatteryLifePercent = (float)status.BatteryLifePercent / 255.0f;
    if (status.BatteryFlag & 8)
        info.State = BatteryInfo::States::BatteryCharging;
    else if (status.BatteryFlag & 1 || status.BatteryFlag & 2 || status.BatteryFlag & 4)
        info.State = BatteryInfo::States::BatteryDischarging;
    else if (status.ACLineStatus == 1 || status.BatteryFlag & 128)
        info.State = BatteryInfo::States::Connected;
    return info;
}

int32 UWPPlatform::GetDpi()
{
    return SystemDpi;
}

String UWPPlatform::GetUserLocaleName()
{
    return UserLocale;
}

String UWPPlatform::GetComputerName()
{
    return ComputerName;
}

bool UWPPlatform::GetHasFocus()
{
    // TODO: impl this
    return true;
}

Float2 UWPPlatform::GetDesktopSize()
{
    Float2 result;
    CUWPPlatform->GetDisplaySize(&result.X, &result.Y);
    return result;
}

Window* UWPPlatform::CreateWindow(const CreateWindowSettings& settings)
{
    // Settings with provided UWPWindowImpl are only valid
    if (settings.Data == nullptr)
    {
        CRASH;
        return nullptr;
    }

    return New<UWPWindow>(settings, (UWPWindowImpl*)settings.Data);
}

void* UWPPlatform::LoadLibrary(const Char* filename)
{
    void* handle = ::LoadPackagedLibrary(filename, 0);
    if (handle)
    {
        LOG(Warning, "Failed to load '{0}' (GetLastError={1})", filename, GetLastError());
    }
    return handle;
}

#endif
