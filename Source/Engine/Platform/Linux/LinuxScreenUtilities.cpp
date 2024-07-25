// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if USE_EDITOR && PLATFORM_LINUX

#include "Engine/Platform/Types.h"
#include "Engine/Platform/ScreenUtilities.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Log.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Platform/Linux/LinuxPlatform.h"
#include "Engine/Platform/Linux/IncludeX11.h"

Delegate<Color32> ScreenUtilitiesBase::PickColorDone;

Color32 LinuxScreenUtilities::GetColorAt(const Float2& pos)
{
    X11::Display* display = (X11::Display*)Platform::GetXDisplay();
    if (display)
    {
        int defaultScreen = X11::XDefaultScreen(display);
        X11::Window rootWindow = X11::XRootWindow(display, defaultScreen);
        X11::XImage* image = X11::XGetImage(display, rootWindow, (int)pos.X, (int)pos.Y, 1, 1, AllPlanes, XYPixmap);
        if (image)
        {
            X11::XColor color;
            color.pixel = XGetPixel(image, 0, 0);
            X11::XFree(image);

            X11::XQueryColor(display, X11::XDefaultColormap(display, defaultScreen), &color);

            Color32 outputColor;
            outputColor.R = color.red / 256;
            outputColor.G = color.green / 256;
            outputColor.B = color.blue / 256;
            outputColor.A = 255;
            return outputColor;
        }
        else
        {
            // XWayland doesn't support XGetImage...
            // TODO: Fallback to Wayland implementation here?
            return Color32::Black;
        }
    }
    else
    {
        // TODO: Wayland
        ASSERT(false);
    }
}

void OnScreenUtilsXEventCallback(void* eventPtr)
{
    X11::XEvent* event = (X11::XEvent*) eventPtr;
    X11::Display* display = (X11::Display*)Platform::GetXDisplay();
    if (event->type == ButtonPress)
    {
        const Float2 cursorPos = Platform::GetMousePosition();
        const Color32 colorPicked = ScreenUtilities::GetColorAt(cursorPos);
        X11::XUngrabPointer(display, CurrentTime);
        ScreenUtilities::PickColorDone(colorPicked);
        LinuxPlatform::xEventReceived.Unbind(OnScreenUtilsXEventCallback);
    }
}

void LinuxScreenUtilities::PickColor()
{
    PROFILE_CPU();
    X11::Display* display = (X11::Display*)Platform::GetXDisplay();
    if (display)
    {
        X11::Window rootWindow = X11::XRootWindow(display, X11::XDefaultScreen(display));

        X11::Cursor cursor = XCreateFontCursor(display, 130);
        int grabbedPointer = X11::XGrabPointer(display, rootWindow, 0,  ButtonPressMask, GrabModeAsync, GrabModeAsync, rootWindow, cursor, CurrentTime);
        if (grabbedPointer != GrabSuccess)
        {
            LOG(Error, "Failed to grab cursor for events.");
            X11::XFreeCursor(display, cursor);
            return;
        }

        X11::XFreeCursor(display, cursor);
        LinuxPlatform::xEventReceived.Bind(OnScreenUtilsXEventCallback);
    }
    else
    {
        // TODO: Wayland
        ASSERT(false);
    }
}

#endif
