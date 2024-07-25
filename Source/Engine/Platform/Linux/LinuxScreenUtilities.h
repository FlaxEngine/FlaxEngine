// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR && PLATFORM_LINUX

#include "Engine/Platform/Base/ScreenUtilitiesBase.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Delegate.h"

/// <summary>
/// Platform-dependent screen utilities.
/// </summary>
class FLAXENGINE_API LinuxScreenUtilities : public ScreenUtilitiesBase
{
public:
    
    // [ScreenUtilitiesBase]
    static Color32 GetColorAt(const Float2& pos);
    static void PickColor();
};

#endif
