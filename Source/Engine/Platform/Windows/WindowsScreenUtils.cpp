#include "WindowsScreenUtils.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Delegate.h"

#include <Windows.h>

#if PLATFORM_WINDOWS
#pragma comment(lib, "Gdi32.lib")
#endif
#include <Engine/Core/Log.h>
#include <Engine/Scripting/ManagedCLR/MCore.h>

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

        ScreenUtils::PickSelected();
        return 1;
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
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

Delegate<Color32> ScreenUtils::PickColorDone;

void ScreenUtils::Test(Color32 testVal) {
    LOG(Warning, "GOT IT");
}

void ScreenUtils::PickSelected() {
    // Push event with color.
    Int2 cursorPos = ScreenUtils::GetScreenCursorPosition();
    Color32 colorPicked = ScreenUtils::GetPixelAt(cursorPos.X, cursorPos.Y);

    LOG(Warning, "REAL: {0}", PickColorDone.Count());
    PickColorDone(colorPicked);
    LOG(Warning, "FAKE");
}

void ScreenUtils::PickColor()
{
    MCore::AttachThread();
    BlockAndReadMouse();
}
