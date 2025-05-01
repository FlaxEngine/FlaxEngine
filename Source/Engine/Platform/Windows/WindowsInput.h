// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS

#include "Engine/Platform/Types.h"
#include "../Win32/WindowsMinimal.h"

/// <summary>
/// Windows platform specific implementation of the input system parts. Handles XInput devices.
/// </summary>
class WindowsInput
{
public:

    static void Init();
    static void Update();
    static bool WndProc(Window* window, Windows::UINT msg, Windows::WPARAM wParam, Windows::LPARAM lParam);
};

#endif
