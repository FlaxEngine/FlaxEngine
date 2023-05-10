#pragma once

#if PLATFORM_WINDOWS

#include "Engine/Core/Math/Color32.h"
#include "Engine/Platform/Base/ScreenUtilsBase.h"

class FLAXENGINE_API ScreenUtils : public ScreenUtilsBase {
public:

    // [ScreenUtilsBase]
    static Color32 GetPixelAt(int32 x, int32 y);
    static Int2 GetScreenCursorPosition();
};

#endif
