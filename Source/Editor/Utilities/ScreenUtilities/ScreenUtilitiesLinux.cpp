#if PLATFORM_LINUX

#include "ScreenUtilities.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Log.h"
#include "Engine/Platform/Linux/LinuxPlatform.h"

#include "Engine/Platform/Linux/IncludeX11.h"

Color32 ScreenUtilities::GetPixelAt(int32 x, int32 y)
{
    X11::XColor color;
    Color32 outputColor;
    
    X11::Display* display = (X11::Display*) LinuxPlatform::GetXDisplay();
    int defaultScreen = X11::XDefaultScreen(display);

    X11::XImage* image;
    image = X11::XGetImage(display, X11::XRootWindow(display, defaultScreen), x, y, 1, 1, AllPlanes, XYPixmap);
    color.pixel = XGetPixel(image, 0, 0);
    X11::XFree(image);

    X11::XQueryColor(display, X11::XDefaultColormap(display, defaultScreen), &color);
    outputColor.R = color.red / 256;
    outputColor.G = color.green / 256;
    outputColor.B = color.blue / 256;

    return outputColor;
}

Int2 ScreenUtilities::GetScreenCursorPosition()
{
    Int2 cursorPosition = { 0, 0 };
    X11::Display* display = (X11::Display*) LinuxPlatform::GetXDisplay();
    X11::Window rootWindow = X11::XRootWindow(display, X11::XDefaultScreen(display));

    // Buffers (Some useful, some not.)
    X11::Window rootWindowBuffer;
    int rootX, rootY;
    int winXBuffer, winYBuffer;
    unsigned int maskBuffer;

    int gotPointer = X11::XQueryPointer(display, rootWindow, &rootWindowBuffer, &rootWindowBuffer, &rootX, &rootY, &winXBuffer, &winYBuffer, &maskBuffer);
    if (!gotPointer) {
        LOG(Error, "Failed to find the mouse pointer (Are you using multiple displays?)");
        return cursorPosition;
    }

    cursorPosition.X = rootX;
    cursorPosition.Y = rootY;

    return cursorPosition;
}

class ScreenUtilitiesLinux
{
public:
  static void BlockAndReadMouse();
  static void xEventHandler(void* event);
};

void ScreenUtilitiesLinux::xEventHandler(void* eventPtr) {
    X11::XEvent* event = (X11::XEvent*) eventPtr;

    X11::Display* display = (X11::Display*) LinuxPlatform::GetXDisplay();

    if (event->type == ButtonPress) {
        Int2 cursorPosition = ScreenUtilities::GetScreenCursorPosition();
        Color32 colorPicked = ScreenUtilities::GetPixelAt(cursorPosition.X, cursorPosition.Y);

        ScreenUtilities::PickColorDone(colorPicked); // Run the callback for picking colors being complete.
        LinuxPlatform::xEventRecieved.Unbind(xEventHandler); // Unbind the event, we only want to handle one click event
        X11::XUngrabPointer(display, CurrentTime);
    }
}

void ScreenUtilitiesLinux::BlockAndReadMouse()
{
    X11::Display* display = (X11::Display*) LinuxPlatform::GetXDisplay();
    X11::Window rootWindow = X11::XRootWindow(display, X11::XDefaultScreen(display));

    X11::Cursor cursor = XCreateFontCursor(display, 130);
    int grabbedPointer = X11::XGrabPointer(display, rootWindow, 0,  ButtonPressMask, GrabModeAsync, GrabModeAsync, rootWindow, cursor, CurrentTime);
    if (grabbedPointer != GrabSuccess) {
        LOG(Error, "Failed to grab cursor for events.");

        X11::XFreeCursor(display, cursor);
        return;
    }

    X11::XFreeCursor(display, cursor);
    LinuxPlatform::xEventRecieved.Bind(xEventHandler);
}

Delegate<Color32> ScreenUtilities::PickColorDone;

void ScreenUtilities::PickColor()
{
    ScreenUtilitiesLinux::BlockAndReadMouse();
}

#endif