// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_GDK

#include "Engine/Platform/Win32/Win32Platform.h"

/// <summary>
/// The GDK platform implementation and application management utilities.
/// </summary>
class FLAXENGINE_API GDKPlatform : public Win32Platform
{
public:

    /// <summary>
    /// Handle to Win32 application instance.
    /// </summary>
    static void* Instance;

    static Delegate<> Suspended;
    static Delegate<> Resumed;

public:

    /// <summary>
    /// Returns true if current OS version is Windows 10.
    /// </summary>
    static bool IsWindows10()
    {
        return true;
    }

    /// <summary>
    /// Pre initialize platform.
    /// </summary>
    /// <param name="hInstance">The Win32 application instance.</param>
    static void PreInit(void* hInstance);

    static bool IsRunningOnDevKit();

    static void SignInSilently();
    static void SignInWithUI();
    static User* FindUser(const struct XUserLocalId& id);

public:

    // [Win32Platform]
    static bool Init();
    static void LogInfo();
    static void BeforeRun();
    static void Tick();
    static void BeforeExit();
    static void Exit();
#if !BUILD_RELEASE
    static void Log(const StringView& msg);
    static bool IsDebuggerPresent();
#endif
    static BatteryInfo GetBatteryInfo();
    static int32 GetDpi();
    static String GetUserLocaleName();
    static String GetComputerName();
    static bool GetHasFocus();
    static bool CanOpenUrl(const StringView& url);
    static void OpenUrl(const StringView& url);
    static Float2 GetDesktopSize();
    static void GetEnvironmentVariables(Dictionary<String, String, HeapAllocation>& result);
    static bool GetEnvironmentVariable(const String& name, String& value);
    static bool SetEnvironmentVariable(const String& name, const String& value);
    static Window* CreateWindow(const CreateWindowSettings& settings);
    static void* LoadLibrary(const Char* filename);
    static void FreeLibrary(void* handle);
    static void* GetProcAddress(void* handle, const char* symbol);
};

#endif
