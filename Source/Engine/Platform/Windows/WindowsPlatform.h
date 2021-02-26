// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS

#include "Engine/Platform/Win32/Win32Platform.h"

/// <summary>
/// The Windows platform implementation and application management utilities.
/// </summary>
class FLAXENGINE_API WindowsPlatform : public Win32Platform
{
public:

    /// <summary>
    /// Win32 application windows class name.
    /// </summary>
    static const Char* ApplicationWindowClass;

    /// <summary>
    /// Handle to Win32 application instance.
    /// </summary>
    static void* Instance;

public:

    /// <summary>
    /// Creates the system-wide mutex.
    /// </summary>
    /// <param name="name">The name of the mutex.</param>
    /// <returns>True if mutex already exists, otherwise false.</returns>
    static bool CreateMutex(const Char* name);

    /// <summary>
    /// Releases the mutex.
    /// </summary>
    static void ReleaseMutex();

    /// <summary>
    /// Pre initialize platform.
    /// </summary>
    /// <param name="hInstance">The Win32 application instance.</param>
    static void PreInit(void* hInstance);

    /// <summary>
    /// Returns true if current OS version is Windows 10.
    /// </summary>
    /// <returns>True if running on Windows 10 or later, otherwise false.</returns>
    static bool IsWindows10();

    static bool ReadRegValue(void* root, const String& key, const String& name, String* result);

public:

    // [Win32Platform]
    static bool Init();
    static void LogInfo();
    static void Tick();
    static void BeforeExit();
    static void Exit();
#if !BUILD_RELEASE
    static void Log(const StringView& msg);
    static bool IsDebuggerPresent();
#endif
    static void SetHighDpiAwarenessEnabled(bool enable);
    static BatteryInfo GetBatteryInfo();
    static int32 GetDpi();
    static String GetUserLocaleName();
    static String GetComputerName();
    static String GetUserName();
    static bool GetHasFocus();
    static bool CanOpenUrl(const StringView& url);
    static void OpenUrl(const StringView& url);
    static Rectangle GetMonitorBounds(const Vector2& screenPos);
    static Vector2 GetDesktopSize();
    static Rectangle GetVirtualDesktopBounds();
    static void GetEnvironmentVariables(Dictionary<String, String>& result);
    static bool GetEnvironmentVariable(const String& name, String& value);
    static bool SetEnvironmentVariable(const String& name, const String& value);
    static int32 StartProcess(const StringView& filename, const StringView& args, const StringView& workingDir, bool hiddenWindow = false, bool waitForEnd = false);
    static int32 RunProcess(const StringView& cmdLine, const StringView& workingDir, bool hiddenWindow = true);
    static int32 RunProcess(const StringView& cmdLine, const StringView& workingDir, const Dictionary<String, String>& environment, bool hiddenWindow = true);
    static Window* CreateWindow(const CreateWindowSettings& settings);
    static void* LoadLibrary(const Char* filename);
    static Array<StackFrame, HeapAllocation> GetStackFrames(int32 skipCount = 0, int32 maxDepth = 60, void* context = nullptr);
#if CRASH_LOG_ENABLE
    static void CollectCrashData(const String& crashDataFolder, void* context = nullptr);
#endif
};

#endif
