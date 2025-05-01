// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_IOS

#include "../Apple/ApplePlatform.h"

/// <summary>
/// The iOS platform implementation and application management utilities.
/// </summary>
class FLAXENGINE_API iOSPlatform : public ApplePlatform
{
public:
    // Runs the callback on a main UI thread (from iOS). Can optionally wait for execution end to sync.
    static void RunOnUIThread(const Function<void()>& func, bool wait = false);
    // Runs the callback on a main thread (from Flax). Can optionally wait for execution end to sync.
    static void RunOnMainThread(const Function<void()>& func, bool wait = false);

public:
    // [ApplePlatform]
    static bool Init();
    static void LogInfo();
    static void Tick();
    static BatteryInfo GetBatteryInfo();
    static int32 GetDpi();
    static NetworkConnectionType GetNetworkConnectionType();
    static ScreenOrientationType GetScreenOrientationType();
	static Guid GetUniqueDeviceId();
    static String GetComputerName();
    static bool GetHasFocus();
    static bool GetIsPaused();
    static Float2 GetDesktopSize();
	static String GetMainDirectory();
    static Window* CreateWindow(const CreateWindowSettings& settings);
};

#endif
