#if PLATFORM_LINUX

#include "ScreenUtilities.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Log.h"

Color32 ScreenUtilities::GetPixelAt(int32 x, int32 y)
{
}

Int2 ScreenUtilities::GetScreenCursorPosition()
{
}

class ScreenUtilitiesLinux
{
public:
  static void BlockAndReadMouse();
};

void ScreenUtilitiesLinux::BlockAndReadMouse()
{
}

Delegate<Color32> ScreenUtilities::PickColorDone;

void ScreenUtilities::PickColor()
{
}

#endif