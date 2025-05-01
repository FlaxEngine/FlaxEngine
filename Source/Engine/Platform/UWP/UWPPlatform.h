// Copyright (c) Wojciech Figat. All rights reserved.

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
    static bool GetHasFocus();
    static Float2 GetDesktopSize();
    static Window* CreateWindow(const CreateWindowSettings& settings);
    static void* LoadLibrary(const Char* filename);
};

#endif
