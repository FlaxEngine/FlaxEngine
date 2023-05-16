#pragma once

#if PLATFORM_WINDOWS

#include "Engine/Platform/Base/ScreenUtilsBase.h"
#include "Engine/Core/Math/Color32.h"

class FLAXENGINE_API ScreenUtils : public ScreenUtilsBase {
public:

    // [ScreenUtilsBase]
    static Color32 GetPixelAt(int32 x, int32 y);
    static Int2 GetScreenCursorPosition();
    static void BlockAndReadMouse();
    static void PickColor();
    static Delegate<Color32> PickColorDone;

public:
    static void PickSelected();
    static void Test(Color32 testVal);
};

#endif
