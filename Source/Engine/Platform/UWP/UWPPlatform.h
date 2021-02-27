// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_UWP

#include "Engine/Platform/Win32/Win32Platform.h"
#include "UWPPlatformImpl.h"

/// <summary>
/// The Universal Windows Platform (UWP) platform implementation and application management utilities.
/// </summary>
class FLAXENGINE_API UWPPlatform : public Win32Platform
{
public:

    /// <summary>
    /// Returns true if current OS version is Windows 10.
    /// </summary>
    static bool IsWindows10()
    {
        return true;
    }

public:

    // [Win32Platform]
    static bool Init();
    static void BeforeRun();
    static void Tick();
    static void BeforeExit();
    static void Exit();
    static BatteryInfo GetBatteryInfo();
    static int32 GetDpi();
    static String GetUserLocaleName();
    static String GetComputerName();
    static String GetUserName();
    static bool GetHasFocus();
    static bool CanOpenUrl(const StringView& url);
    static void OpenUrl(const StringView& url);
    static Vector2 GetMousePosition();
    static void SetMousePosition(const Vector2& pos);
    static Rectangle GetMonitorBounds(const Vector2& screenPos);
    static Vector2 GetDesktopSize();
    static Rectangle GetVirtualDesktopBounds();
    static Window* CreateWindow(const CreateWindowSettings& settings);
    static void* LoadLibrary(const Char* filename);
};

#endif
