#if PLATFORM_WINDOWS

#include "ScreenUtilities.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Log.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"

#include <Windows.h>


#pragma comment(lib, "Gdi32.lib")


Color32 ScreenUtilities::GetPixelAt(int32 x, int32 y)
{
    HDC deviceContext = GetDC(NULL);
    COLORREF color = GetPixel(deviceContext, x, y);
    ReleaseDC(NULL, deviceContext);

    Color32 returnColor = { GetRValue(color), GetGValue(color), GetBValue(color), 255 };
    return returnColor;
}

Int2 ScreenUtilities::GetScreenCursorPosition()
{
    POINT cursorPos;
    GetCursorPos(&cursorPos);

    Int2 returnCursorPos = { cursorPos.x, cursorPos.y };
    return returnCursorPos;
}

class ScreenUtilitiesWindows
{
public:
    static void PickSelected();
    static void BlockAndReadMouse();
};

void ScreenUtilitiesWindows::PickSelected() {
    Int2 cursorPos = ScreenUtilities::GetScreenCursorPosition();
    Color32 colorPicked = ScreenUtilities::GetPixelAt(cursorPos.X, cursorPos.Y);

    // Push event with the picked color.
    ScreenUtilities::PickColorDone(colorPicked);
}

static HHOOK _mouseCallbackHook;
LRESULT CALLBACK ScreenUtilsMouseCallback(
    _In_ int nCode,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    if (wParam != WM_LBUTTONDOWN) { // Return as early as possible.
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    if (nCode < 0) {
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }


    if (nCode >= 0 && wParam == WM_LBUTTONDOWN) { // Now try to run our code.
        UnhookWindowsHookEx(_mouseCallbackHook);

        ScreenUtilitiesWindows::PickSelected();
        return 1;
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void ScreenUtilitiesWindows::BlockAndReadMouse()
{
    _mouseCallbackHook = SetWindowsHookEx(WH_MOUSE_LL, ScreenUtilsMouseCallback, NULL, NULL);
    if (_mouseCallbackHook == NULL)
    {
        LOG(Warning, "Failed to set mouse hook.");
        LOG(Warning, "Error: {0}", GetLastError());
    }
}

Delegate<Color32> ScreenUtilities::PickColorDone;

void ScreenUtilities::PickColor()
{
    ScreenUtilitiesWindows::BlockAndReadMouse();
}


#endif
