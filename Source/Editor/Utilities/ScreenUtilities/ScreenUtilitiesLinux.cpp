#if PLATFORM_LINUX

#include "ScreenUtilities.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Log.h"

#include "Engine/Platform/Linux/IncludeX11.h"

Color32 ScreenUtilities::GetPixelAt(int32 x, int32 y)
{
    X11::XColor color;
    Color32 outputColor;
    
    X11::Display* display = X11::XOpenDisplay((char *) NULL);
    int defaultScreen = X11::XDefaultScreen(display);

    X11::XImage* image;
    image = X11::XGetImage(display, X11::XRootWindow(display, defaultScreen), x, y, 1, 1, AllPlanes, XYPixmap);
    color.pixel = XGetPixel(image, 0, 0);
    X11::XFree(image);

    X11::XQueryColor(display, X11::XDefaultColormap(display, defaultScreen), &color);
    outputColor.R = color.red / 256;
    outputColor.G = color.green / 256;
    outputColor.B = color.blue / 256;

    X11::XCloseDisplay(display);
    return outputColor;
}

Int2 ScreenUtilities::GetScreenCursorPosition()
{
    Int2 cursorPosition = { 0, 0 };
    X11::Display* display = X11::XOpenDisplay(NULL);
    X11::Window rootWindow = X11::XRootWindow(display, X11::XDefaultScreen(display));

    // Buffers (Some useful, some not.)
    X11::Window rootWindowBuffer;
    int rootX, rootY;
    int winXBuffer, winYBuffer;
    uint maskBuffer;

    int gotPointer = X11::XQueryPointer(display, rootWindow, &rootWindowBuffer, &rootWindowBuffer, &rootX, &rootY, &winXBuffer, &winYBuffer, &maskBuffer);
    if (!gotPointer) {
        LOG(Error, "Failed to read mouse pointer (Are you using multiple displays?)");
        return cursorPosition;
    }

    cursorPosition.X = rootX;
    cursorPosition.Y = rootY;

    X11::XCloseDisplay(display);
    return cursorPosition;
}

class ScreenUtilitiesLinux
{
public:
  static void BlockAndReadMouse();
};

void ScreenUtilitiesLinux::BlockAndReadMouse()
{
    return;
}

Delegate<Color32> ScreenUtilities::PickColorDone;

void ScreenUtilities::PickColor()
{
    return;
}

#endif