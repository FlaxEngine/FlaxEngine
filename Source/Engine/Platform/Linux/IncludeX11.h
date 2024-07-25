// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_LINUX

// Hide warning from old headers
#define register

// Pre-include X11 required headers
#include <stdio.h>

// Include X11
namespace X11
{
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xcms.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/Xresource.h>
#include <X11/extensions/Xinerama.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/extensions/Xfixes.h>
}

// Helper macros
#define X11_ScreenOfDisplay(dpy, scr) (&((X11::_XPrivDisplay)(dpy))->screens[scr])
#define X11_RootWindow(dpy, scr) (X11_ScreenOfDisplay(dpy, scr)->root)
#define X11_DefaultRootWindow(dpy) (((&((X11::_XPrivDisplay)(dpy))->screens[(((X11::_XPrivDisplay)(dpy))->default_screen)]))->root)
#define X11_DefaultScreen(dpy) (((X11::_XPrivDisplay)(dpy))->default_screen)
#define X11_DisplayWidth(dpy, scr) (X11_ScreenOfDisplay(dpy, scr)->width)
#define X11_DisplayHeight(dpy, scr) (X11_ScreenOfDisplay(dpy, scr)->height)
#define X11_DisplayWidthMM(dpy, scr) (X11_ScreenOfDisplay(dpy, scr)->mwidth)
#define X11_DisplayHeightMM(dpy, scr) (X11_ScreenOfDisplay(dpy, scr)->mheight)

// Remove some ugly stuff to prevent pollution
#undef None
#undef Always
#undef Bool
#undef Status
#undef True
#undef False

#endif
