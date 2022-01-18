// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include <CoreFoundation/CoreFoundation.h>

class FLAXENGINE_API MacUtils
{
public:
    static String ToString(CFStringRef str);
    static CFStringRef ToString(const StringView& str);
    static Vector2 PosToCoca(const Vector2& pos);
    static Vector2 GetScreensOrigin();
};
