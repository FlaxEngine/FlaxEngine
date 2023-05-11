#include "ScreenUtilsBase.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Vector2.h"

Color32 ScreenUtilsBase::GetPixelAt(int32 x, int32 y) {
    return Color32::Black;
}

Int2 ScreenUtilsBase::GetScreenCursorPosition() {
    return { 0, 0 };
}

void ScreenUtilsBase::BlockAndReadMouse() {
}
