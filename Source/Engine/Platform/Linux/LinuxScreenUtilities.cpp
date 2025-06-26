// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if PLATFORM_LINUX

#include "Engine/Platform/Types.h"
#include "Engine/Platform/ScreenUtilities.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Platform/Linux/LinuxPlatform.h"
#include "Engine/Platform/Linux/IncludeX11.h"

#if PLATFORM_SDL
#include <libportal/portal-enums.h>
#include <libportal/screenshot.h>
#endif

Delegate<Color32> ScreenUtilitiesBase::PickColorDone;

#if PLATFORM_SDL
namespace PortalImpl
{
    XdpPortal* Portal = nullptr;
    int64 MainLoopReady = 0;

    gpointer GLibMainLoop(gpointer data);
    void PickColorCallback(GObject* source, GAsyncResult* result, gpointer data);
}
#endif

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
            // XWayland doesn't support XGetImage
        }
    }

    return Color32::Transparent;
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
        return;
    }
#if PLATFORM_SDL
    if (PortalImpl::MainLoopReady == 0)
    {
        // Initialize portal
        GError* error = nullptr;
        PortalImpl::Portal = xdp_portal_initable_new(&error);
        if (error != nullptr)
        {
            PortalImpl::MainLoopReady = 2;
            LOG(Error, "Failed to initialize XDP Portal");
            return;
        }

        // Run the GLib main loop in other thread in order to process asynchronous callbacks
        g_thread_new(nullptr, PortalImpl::GLibMainLoop, nullptr);
        while (Platform::AtomicRead(&PortalImpl::MainLoopReady) != 1)
            Platform::Sleep(1);
    }
    
    if (PortalImpl::Portal != nullptr)
    {
        // Enter color picking mode, the callback receives the final color
        xdp_portal_pick_color(PortalImpl::Portal, nullptr, nullptr, PortalImpl::PickColorCallback, nullptr);
    }
#endif
}

#if PLATFORM_SDL
gpointer PortalImpl::GLibMainLoop(gpointer data)
{
    GMainContext* mainContext = g_main_context_get_thread_default();
    GMainLoop* mainLoop = g_main_loop_new(mainContext, false);

    Platform::AtomicStore(&PortalImpl::MainLoopReady, 1);
    
    g_main_loop_run(mainLoop);
    g_main_loop_unref(mainLoop);
    return nullptr;
}

void PortalImpl::PickColorCallback(GObject* source, GAsyncResult* result, gpointer data)
{
    GError* error = nullptr;
    GVariant* variant = xdp_portal_pick_color_finish(PortalImpl::Portal, result, &error);
    if (error)
    {
        if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            LOG(Info, "XDP Portal pick color cancelled");
        else
            LOG(Error, "XDP Portal pick color failed: {}", String(error->message));
        return;
    }

    // The color is stored in a triple double variant, extract the values
    Double4 colorDouble;
    g_variant_get(variant, "(ddd)", &colorDouble.X, &colorDouble.Y, &colorDouble.Z);
    g_variant_unref(variant);
    colorDouble.W = 1.0f;
    Vector4 colorVector = colorDouble;
    Color32 color = Color32(colorVector);

    ScreenUtilities::PickColorDone(color);
}
#endif

#endif
