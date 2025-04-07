// Copyright (c) Wojciech Figat. All rights reserved.

#include "ScreenUtilities.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Log.h"
#include "Engine/Profiler/ProfilerCPU.h"

Delegate<Color32> ScreenUtilities::PickColorDone;

#if PLATFORM_WINDOWS

#include <Windows.h>

#pragma comment(lib, "Gdi32.lib")

static HHOOK MouseCallbackHook;

LRESULT CALLBACK OnScreenUtilsMouseCallback(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    if (nCode >= 0 && wParam == WM_LBUTTONDOWN)
    {
        UnhookWindowsHookEx(MouseCallbackHook);

        // Push event with the picked color
        const Float2 cursorPos = Platform::GetMousePosition();
        const Color32 colorPicked = ScreenUtilities::GetColorAt(cursorPos);
        ScreenUtilities::PickColorDone(colorPicked);
        return 1;
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

Color32 ScreenUtilities::GetColorAt(const Float2& pos)
{
    PROFILE_CPU();
    HDC deviceContext = GetDC(NULL);
    COLORREF color = GetPixel(deviceContext, (int)pos.X, (int)pos.Y);
    ReleaseDC(NULL, deviceContext);
    return Color32(GetRValue(color), GetGValue(color), GetBValue(color), 255);
}

void ScreenUtilities::PickColor()
{
    MouseCallbackHook = SetWindowsHookEx(WH_MOUSE_LL, OnScreenUtilsMouseCallback, NULL, NULL);
    if (MouseCallbackHook == NULL)
    {
        LOG(Warning, "Failed to set mouse hook.");
        LOG(Warning, "Error: {0}", GetLastError());
    }
}

#elif PLATFORM_LINUX

#include "Engine/Platform/Linux/LinuxPlatform.h"
#include "Engine/Platform/Linux/IncludeX11.h"

Color32 ScreenUtilities::GetColorAt(const Float2& pos)
{
    X11::XColor color;
    
    X11::Display* display = (X11::Display*) LinuxPlatform::GetXDisplay();
    int defaultScreen = X11::XDefaultScreen(display);

    X11::XImage* image;
    image = X11::XGetImage(display, X11::XRootWindow(display, defaultScreen), (int)pos.X, (int)pos.Y, 1, 1, AllPlanes, XYPixmap);
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

void OnScreenUtilsXEventCallback(void* eventPtr)
{
    X11::XEvent* event = (X11::XEvent*) eventPtr;
    X11::Display* display = (X11::Display*)LinuxPlatform::GetXDisplay();
    if (event->type == ButtonPress)
    {
        const Float2 cursorPos = Platform::GetMousePosition();
        const Color32 colorPicked = ScreenUtilities::GetColorAt(cursorPos);
        X11::XUngrabPointer(display, CurrentTime);
        ScreenUtilities::PickColorDone(colorPicked);
        LinuxPlatform::xEventRecieved.Unbind(OnScreenUtilsXEventCallback);
    }
}

void ScreenUtilities::PickColor()
{
    PROFILE_CPU();
    X11::Display* display = (X11::Display*) LinuxPlatform::GetXDisplay();
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
    LinuxPlatform::xEventRecieved.Bind(OnScreenUtilsXEventCallback);
}

#elif PLATFORM_MAC

#include <Cocoa/Cocoa.h>
#include <AppKit/AppKit.h>

Color32 ScreenUtilities::GetColorAt(const Float2& pos)
{
    // TODO: implement ScreenUtilities for macOS
    return { 0, 0, 0, 255 };
}

void ScreenUtilities::PickColor()
{
    // This is what C# calls to start the color picking sequence
    // This should stop mouse clicks from working for one click, and that click is on the selected color
    // There is a class called NSColorSample that might implement that for you, but maybe not.
}

#endif
