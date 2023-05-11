#include "WindowsScreenUtils.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Vector2.h"

#include <Windows.h>

#if PLATFORM_WINDOWS
#pragma comment(lib, "Gdi32.lib")
#endif
#include <Engine/Core/Log.h>

Color32 ScreenUtils::GetPixelAt(int32 x, int32 y)
{
    HDC deviceContext = GetDC(NULL);
    COLORREF color = GetPixel(deviceContext, x, y);
    ReleaseDC(NULL, deviceContext);
    
    Color32 returnColor = { GetRValue(color), GetGValue(color), GetBValue(color), 255 };
    return returnColor;
}

Int2 ScreenUtils::GetScreenCursorPosition()
{
    POINT cursorPos;
    GetCursorPos(&cursorPos);

    Int2 returnCursorPos = { cursorPos.x, cursorPos.y };
    return returnCursorPos;
}

static HHOOK _mouseCallbackHook;
LRESULT CALLBACK ScreenUtilsMouseCallback(
    _In_ int nCode,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    LOG(Warning, "Hell lag. {0}", GetCurrentThreadId());
    if (wParam != WM_LBUTTONDOWN) { // Return as early as possible.
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    if (nCode < 0) {
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }


    if (nCode >= 0 && wParam == WM_LBUTTONDOWN) { // Now try to run our code.
        LOG(Warning, "Mouse callback hit. Skipping event. (hopefully)");
        UnhookWindowsHookEx(_mouseCallbackHook);
        return 1;
    }
}

void ScreenUtils::BlockAndReadMouse()
{
    _mouseCallbackHook = SetWindowsHookEx(WH_MOUSE_LL, ScreenUtilsMouseCallback, NULL, NULL);
    if (_mouseCallbackHook == NULL)
    {
        LOG(Warning, "Failed to set mouse hook.");
        LOG(Warning, "Error: {0}", GetLastError());
    }
}
