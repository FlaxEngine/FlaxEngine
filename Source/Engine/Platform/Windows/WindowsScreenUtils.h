#pragma once
#include "Engine/Core/Math/Color32.h"

class PlatformScreenUtils {
public:
    static Color32 GetPixelAt(int32 x, int32 y);
    static Int2 GetScreenCursorPosition();
};

