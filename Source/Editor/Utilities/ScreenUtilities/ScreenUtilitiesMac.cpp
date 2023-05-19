#if PLATFORM_MAC
#include <Cocoa/Cocoa.h>
#include <AppKit/AppKit.h>

#include "ScreenUtilities.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Log.h"

Color32 ScreenUtilities::GetPixelAt(int32 x, int32 y)
{
    // Called from C# for live updates to the color.

    return { 0, 0, 0, 255 };
}

Int2 ScreenUtilities::GetScreenCursorPosition()
{
    // Called from C# for live updates to the color.

    return { 0, 0 };
}

class ScreenUtilitiesMac
{
public:
    static void BlockAndReadMouse();
};

void ScreenUtilitiesMac::BlockAndReadMouse()
{
    // Maybe you don't need this if you go with NSColorSampler
}

Delegate<Color32> ScreenUtilities::PickColorDone;

void ScreenUtilities::PickColor()
{
    // This is what C# calls to start the color picking sequence
    // This should stop mouse clicks from working for one click, and that click is on the selected color
    // There is a class called NSColorSample that might implement that for you, but maybe not.
    // It also might just work to copy the Linux Impl since Mac uses X as well, right?
}

#endif
