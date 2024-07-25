// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if USE_EDITOR && PLATFORM_MAC

#include "Engine/Platform/Types.h"
#include "Engine/Platform/ScreenUtilities.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Log.h"

#include <Cocoa/Cocoa.h>
#include <AppKit/AppKit.h>

Delegate<Color32> ScreenUtilitiesBase::PickColorDone;

Color32 MacScreenUtilities::GetColorAt(const Float2& pos)
{
    // TODO: implement ScreenUtilities for macOS
    return { 0, 0, 0, 255 };
}

void MacScreenUtilities::PickColor()
{
    // This is what C# calls to start the color picking sequence
    // This should stop mouse clicks from working for one click, and that click is on the selected color
    // There is a class called NSColorSample that might implement that for you, but maybe not.
}

#endif
