// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_MAC

#include "../Apple/ApplePlatform.h"

/// <summary>
/// The Mac platform implementation and application management utilities.
/// </summary>
class FLAXENGINE_API MacPlatform : public ApplePlatform
{
public:
    // [ApplePlatform]
    static bool Init();
    static void LogInfo();
    static void BeforeRun();
    static void Tick();
    static int32 GetDpi();
    static Guid GetUniqueDeviceId();
    static String GetComputerName();
    static Float2 GetMousePosition();
    static void SetMousePosition(const Float2& pos);
    static Rectangle GetMonitorBounds(const Float2& screenPos);
    static Float2 GetDesktopSize();
    static Rectangle GetVirtualDesktopBounds();
    static String GetMainDirectory();
    static Window* CreateWindow(const CreateWindowSettings& settings);
    static int32 CreateProcess(CreateProcessSettings& settings);
};

#endif
