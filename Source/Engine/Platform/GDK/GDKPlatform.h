// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_GDK

#include "Engine/Platform/Win32/Win32Platform.h"

/// <summary>
/// The GDK platform implementation and application management utilities.
/// </summary>
API_CLASS(Static, Tag="NoTypeInitializer")
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

    // True, if game is running Xbox Devkit.
    API_PROPERTY() static bool IsRunningOnDevKit();
    // Signs in user without showing UI. If user is not signed in, it will fail and return false. Use SignInWithUI to show UI and let user sign in.
    API_FUNCTION() static void SignInSilently();
    // Signs in user with showing UI. If user is already signed in, it will succeed and return true. If user is not signed in, it will show UI and let user sign in.
    API_FUNCTION() static void SignInWithUI();
    // Searches for a user with a specific local ID.
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
    static String GetSystemName();
    static Version GetSystemVersion();
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
