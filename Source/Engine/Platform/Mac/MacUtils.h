// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include <CoreFoundation/CoreFoundation.h>

class FLAXENGINE_API MacUtils
{
public:
    static String ToString(CFStringRef str);
    static CFStringRef ToString(const StringView& str);
    static Float2 PosToCoca(const Float2& pos);
    static Float2 CocaToPos(const Float2& pos);
    static Float2 GetScreensOrigin();
};
