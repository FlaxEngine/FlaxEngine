// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if USE_EDITOR && PLATFORM_WINDOWS

#include "Engine/Platform/Types.h"
#include "Engine/Platform/ScreenUtilities.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Log.h"
#include "Engine/Profiler/ProfilerCPU.h"

#include <Windows.h>

#pragma comment(lib, "Gdi32.lib")

Delegate<Color32> ScreenUtilitiesBase::PickColorDone;

static HHOOK MouseCallbackHook;

LRESULT CALLBACK OnScreenUtilsMouseCallback(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    if (nCode >= 0 && wParam == WM_LBUTTONDOWN)
    {
        UnhookWindowsHookEx(MouseCallbackHook);

        // Push event with the picked color
        const Float2 cursorPos = Platform::GetMousePosition();
        const Color32 colorPicked = ScreenUtilities::GetColorAt(cursorPos);
        ScreenUtilities::PickColorDone(colorPicked);
        return 1;
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

Color32 WindowsScreenUtilities::GetColorAt(const Float2& pos)
{
    PROFILE_CPU();
    HDC deviceContext = GetDC(NULL);
    COLORREF color = GetPixel(deviceContext, (int)pos.X, (int)pos.Y);
    ReleaseDC(NULL, deviceContext);
    return Color32(GetRValue(color), GetGValue(color), GetBValue(color), 255);
}

void WindowsScreenUtilities::PickColor()
{
    MouseCallbackHook = SetWindowsHookEx(WH_MOUSE_LL, OnScreenUtilsMouseCallback, NULL, NULL);
    if (MouseCallbackHook == NULL)
    {
        LOG(Warning, "Failed to set mouse hook.");
        LOG(Warning, "Error: {0}", GetLastError());
    }
}

#endif
