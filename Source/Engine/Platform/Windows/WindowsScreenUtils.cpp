#include "WindowsScreenUtils.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Vector2.h"

#include <Windows.h>

#if PLATFORM_WINDOWS
#pragma comment(lib, "Gdi32.lib")
#endif

Color32 ScreenUtils::GetPixelAt(int32 x, int32 y) {
    HDC deviceContext = GetDC(NULL);
    COLORREF color = GetPixel(deviceContext, x, y);
    ReleaseDC(NULL, deviceContext);
    
    Color32 returnColor = { GetRValue(color), GetGValue(color), GetBValue(color), 255 };
    return returnColor;
}

Int2 ScreenUtils::GetScreenCursorPosition() {
    POINT cursorPos;
    GetCursorPos(&cursorPos);

    Int2 returnCursorPos = { cursorPos.x, cursorPos.y };
    return returnCursorPos;
}
