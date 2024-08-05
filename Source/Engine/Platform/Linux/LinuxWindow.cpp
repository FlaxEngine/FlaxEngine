// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if PLATFORM_LINUX && !PLATFORM_SDL

#include "../Window.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Input/Keyboard.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Utilities/StringConverter.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Textures/TextureData.h"
// hack using TextureTool in Platform module -> TODO: move texture data sampling to texture data itself
#define COMPILE_WITH_TEXTURE_TOOL 1
#include "Engine/Tools/TextureTool/TextureTool.h"
#include "IncludeX11.h"

// ICCCM
#define WM_NormalState 1L // window normal state
#define WM_IconicState 3L // window minimized

// EWMH
#define _NET_WM_STATE_REMOVE 0L // remove/unset property
#define _NET_WM_STATE_ADD 1L // add/set property
#define _NET_WM_STATE_TOGGLE 2L // toggle property
#define DefaultDPI 96

// Window routines function prolog
#define LINUX_WINDOW_PROLOG X11::Display* display = (X11::Display*)LinuxPlatform::GetXDisplay(); X11::Window window =  (X11::Window)_window

extern X11::XIC IC;
extern X11::Atom xAtomDeleteWindow;
extern X11::Atom xAtomWmWindowOpacity;
extern X11::Atom xAtomWmName;
extern Dictionary<StringAnsi, X11::KeyCode> KeyNameMap;
extern Array<KeyboardKeys> KeyCodeMap;
extern X11::Cursor Cursors[(int32)CursorType::MAX];
extern Window* MouseTrackingWindow;

static constexpr uint32 MouseDoubleClickTime = 500;
static constexpr uint32 MaxDoubleClickDistanceSquared = 10;
static X11::Time MouseLastButtonPressTime = 0;
static Float2 OldMouseClickPosition;

LinuxWindow::LinuxWindow(const CreateWindowSettings& settings)
	: WindowBase(settings)
{
	auto display = (X11::Display*)LinuxPlatform::GetXDisplay();
    if (!display)
        return;
	auto screen = XDefaultScreen(display);

    // Cache data
	int32 width = Math::TruncToInt(settings.Size.X);
	int32 height = Math::TruncToInt(settings.Size.Y);
	_clientSize = Float2((float)width, (float)height);
    int32 x = 0, y = 0;
	switch (settings.StartPosition)
    {
    case WindowStartPosition::CenterParent:
        if (settings.Parent)
        {
            Rectangle parentBounds = settings.Parent->GetClientBounds();
            x = Math::TruncToInt(parentBounds.Location.X + (parentBounds.Size.X - _clientSize.X) * 0.5f);
            y = Math::TruncToInt(parentBounds.Location.Y + (parentBounds.Size.Y - _clientSize.Y) * 0.5f);
        }
        break;
    case WindowStartPosition::CenterScreen:
        {
			Rectangle desktopBounds;
			int event, err;
			const bool ok = X11::XineramaQueryExtension(display, &event, &err);
			if (X11::XineramaQueryExtension(display, &event, &err))
			{
				int count;
				X11::XineramaScreenInfo* xsi = X11::XineramaQueryScreens(display, &count);
				ASSERT(screen < count);
				desktopBounds = Rectangle(Float2((float)xsi[screen].x_org, (float)xsi[screen].y_org), Float2((float)xsi[screen].width, (float)xsi[screen].height));
				X11::XFree(xsi);
			}
			else
				desktopBounds = Rectangle(Float2::Zero, Platform::GetDesktopSize());
            x = Math::TruncToInt(desktopBounds.Location.X + (desktopBounds.Size.X - _clientSize.X) * 0.5f);
            y = Math::TruncToInt(desktopBounds.Location.Y + (desktopBounds.Size.Y - _clientSize.Y) * 0.5f);
        }
        break;
    case WindowStartPosition::Manual:
        x = Math::TruncToInt(settings.Position.X);
        y = Math::TruncToInt(settings.Position.Y);
        break;
	}
	_resizeDisabled = !settings.HasSizingFrame;

	auto rootWindow = XRootWindow(display, screen);

	long visualMask = VisualScreenMask;
	int numberOfVisuals;
	X11::XVisualInfo vInfoTemplate = {};
	vInfoTemplate.screen = screen;
	X11::XVisualInfo* visualInfo = X11::XGetVisualInfo(display, visualMask, &vInfoTemplate, &numberOfVisuals);

	X11::Colormap colormap = X11::XCreateColormap(display, rootWindow, visualInfo->visual, AllocNone);

	X11::XSetWindowAttributes windowAttributes = {};
	windowAttributes.colormap = colormap;
	windowAttributes.background_pixel = XBlackPixel(display, screen);
	windowAttributes.border_pixel = XBlackPixel(display, screen);
	windowAttributes.event_mask = KeyPressMask | KeyReleaseMask | StructureNotifyMask | ExposureMask;

    if (!settings.IsRegularWindow)
    {
        windowAttributes.save_under = true;
        windowAttributes.override_redirect = true;
    }

	// TODO: implement all window settings
	/*
	bool Fullscreen;
	bool AllowMinimize;
	bool AllowMaximize;
	*/

    unsigned long valueMask = CWBackPixel | CWBorderPixel | CWEventMask | CWColormap;
    if (!settings.IsRegularWindow)
    {
        valueMask |= CWOverrideRedirect | CWSaveUnder;
    }
	const X11::Window window = X11::XCreateWindow(
		display, X11::XRootWindow(display, screen), x, y,
		width, height, 0, visualInfo->depth, InputOutput,
		visualInfo->visual,
		valueMask, &windowAttributes);
	_window = window;
	LinuxWindow::SetTitle(settings.Title);

	// Link process id with window
	X11::Atom wmPid = X11::XInternAtom(display, "_NET_WM_PID", 0);
	const uint64 pid = Platform::GetCurrentProcessId();
	X11::XChangeProperty(display, window, wmPid, 6, 32, PropModeReplace, (unsigned char*)&pid, 1);

	// Position/size might have (and usually will) get overridden by the WM, so re-apply them
	X11::XSizeHints hints;
	hints.flags = PPosition | PSize | PMinSize | PMaxSize;
	hints.x = x;
	hints.y = y;
	hints.width = width;
	hints.height = height;
	if (_resizeDisabled)
	{
		// Block resizing
		hints.min_width = width;
		hints.max_width = width;
		hints.min_height = height;
		hints.max_height = height;
	}
	else
	{
		// Set resizing range
		hints.min_width = (int)settings.MinimumSize.X;
		hints.max_width = settings.MaximumSize.X > 0 ? (int)settings.MaximumSize.X : MAX_uint16;
		hints.min_height = (int)settings.MinimumSize.Y;
		hints.max_height = settings.MaximumSize.Y > 0 ? (int)settings.MaximumSize.Y : MAX_uint16;
		hints.flags |= USSize;
	}
    // honor the WM placement except for manual (overriding) placements
    if (settings.StartPosition == WindowStartPosition::Manual)
    {
        X11::XSetNormalHints(display, window, &hints);
    }

	// Ensures the child window is always on top of the parent window
	if (settings.Parent)
	{
		X11::XSetTransientForHint(display, window, (X11::Window)((LinuxWindow*)settings.Parent)->GetNativePtr());
	}

    _dpi = Platform::GetDpi();
    _dpiScale = (float)_dpi / (float)DefaultDPI;
    
	// Set input mask
	long eventMask =
			ExposureMask | FocusChangeMask |
			KeyPressMask | KeyReleaseMask |
			ButtonPressMask | ButtonReleaseMask |
			EnterWindowMask | LeaveWindowMask |
			VisibilityChangeMask | ExposureMask |
			PointerMotionMask | ButtonMotionMask |
			StructureNotifyMask | PropertyChangeMask;
	if (!settings.Parent)
		eventMask |= SubstructureNotifyMask | SubstructureRedirectMask;
	X11::XSelectInput(display, window, eventMask);

	// Make sure we get the window delete message from WM
	X11::XSetWMProtocols(display, window, &xAtomDeleteWindow, 1);

	// Adjust style for borderless windows
	if (!settings.HasBorder)
	{
        // [Reference: https://www.tonyobryan.com//index.php?article=9]
		typedef struct X11Hints
		{
			unsigned long flags;
			unsigned long functions;
			unsigned long decorations;
			long inputMode;
			unsigned long status;
		} X11Hints;
		X11Hints hints;
		hints.flags = 2;
		hints.decorations = 0;
		X11::Atom wmHints = X11::XInternAtom(display, "_MOTIF_WM_HINTS", 0);
		X11::Atom property;
		if (wmHints)
			X11::XChangeProperty(display, window, property, property, 32, PropModeReplace, (unsigned char*)&hints, 5);
	}

	// Adjust type for utility windows
	if (!settings.IsRegularWindow)
	{
		X11::Atom value = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", 0);
		X11::Atom wmType = XInternAtom(display, "_NET_WM_WINDOW_TYPE", 0);
		if (value && wmType)
			X11::XChangeProperty(display, window, wmType, (X11::Atom)4, 32, PropModeReplace, (unsigned char*)&value, 1);
	}

	// Initialize state
	X11::Atom wmState = X11::XInternAtom(display, "_NET_WM_STATE", 0);
	X11::Atom wmStateAbove = X11::XInternAtom(display, "_NET_WM_STATE_ABOVE", 0);
	X11::Atom wmSateSkipTaskbar = X11::XInternAtom(display, "_NET_WM_STATE_SKIP_TASKBAR", 0);
	X11::Atom wmStateSkipPager = X11::XInternAtom(display, "_NET_WM_STATE_SKIP_PAGER", 0);
	X11::Atom wmStateFullscreen = X11::XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", 0);
	X11::Atom states[4];
	int32 statesCount = 0;
	if (settings.IsTopmost)
	{
		states[statesCount++] = wmStateAbove;
	}
	if (!settings.ShowInTaskbar)
	{
		states[statesCount++] = wmSateSkipTaskbar;
		states[statesCount++] = wmStateSkipPager;
	}
	if (settings.Fullscreen)
	{
		states[statesCount++] = wmStateFullscreen;
	}
	X11::XChangeProperty(display, window, wmState, (X11::Atom)4, 32, PropModeReplace, (unsigned char*)states, statesCount);

    // Drag&drop support
    if (settings.AllowDragAndDrop)
    {
        auto xdndVersion = 5;
        auto xdndAware = XInternAtom(display, "XdndAware", 0);
        if (xdndAware != 0)
			X11::XChangeProperty(display, window, xdndAware, (X11::Atom)4, 32, PropModeReplace, (unsigned char*)&xdndVersion, 1);
    }

	// Sync
	X11::XFlush(display);
	X11::XSync(display, 0);
	X11::XFree(visualInfo);
}

LinuxWindow::~LinuxWindow()
{
	// Cleanup
	LINUX_WINDOW_PROLOG;
    if (!display)
        return;
	X11::XDestroyWindow(display, window);
}

void* LinuxWindow::GetNativePtr() const
{
	return (void*)_window;
}

void LinuxWindow::Show()
{
	if (!_visible)
	{
        InitSwapChain();
        if (_showAfterFirstPaint)
        {
            if (RenderTask)
                RenderTask->Enabled = true;
            return;
        }

		// Show
		LINUX_WINDOW_PROLOG;
        if (display)
        {
            X11::XMapWindow(display, window);
            X11::XFlush(display);
        }
        _focusOnMapped = _settings.AllowInput && _settings.ActivateWhenFirstShown;

		// Base
		WindowBase::Show();
	}
}

void LinuxWindow::Hide()
{
	if (_visible)
	{
		// Hide
		LINUX_WINDOW_PROLOG;
        if (display)
        {
		    X11::XUnmapWindow(display, window);
        }

		// Base
		WindowBase::Hide();
	}
}

void LinuxWindow::Minimize()
{
	Minimize(true);
}

void LinuxWindow::Maximize()
{
	Maximize(true);
}

void LinuxWindow::Restore()
{
	if (IsMaximized())
		Maximize(false);
	else if (IsMinimized())
		Minimize(false);
}

void LinuxWindow::BringToFront(bool force)
{
	LINUX_WINDOW_PROLOG;
    if (!display)
        return;
	X11::Atom activeWindow = X11::XInternAtom(display, "_NET_ACTIVE_WINDOW", 0);

	X11::XEvent event;
	Platform::MemoryClear(&event, sizeof(event));
	event.type = ClientMessage;
	event.xclient.window = window;
	event.xclient.message_type = activeWindow;
	event.xclient.format = 32;
	event.xclient.data.l[0] = 1;
	event.xclient.data.l[1] = CurrentTime;

	X11::XSendEvent(display, X11_DefaultRootWindow(display), 0, SubstructureRedirectMask | SubstructureNotifyMask, &event);
	X11::XFlush(display);
}

bool LinuxWindow::IsClosed() const
{
	return _isClosing;
}

bool LinuxWindow::IsForegroundWindow() const
{
	return _focused || _focusOnMapped;
}

void LinuxWindow::SetClientBounds(const Rectangle& clientArea)
{
	int32 x = Math::TruncToInt(clientArea.GetX());
	int32 y = Math::TruncToInt(clientArea.GetY());
	int32 width = Math::TruncToInt(clientArea.GetWidth());
	int32 height = Math::TruncToInt(clientArea.GetHeight());
	LINUX_WINDOW_PROLOG;
    if (!display)
        return;

	// If resize is disabled on WM level, we need to force it
	if (_resizeDisabled)
	{
		X11::XSizeHints hints;
		hints.flags = PMinSize | PMaxSize;
		hints.min_height = height;
		hints.max_height = height;
		hints.min_width = width;
		hints.max_width = width;
		X11::XSetNormalHints(display, window, &hints);
	}

	X11::XMapWindow(display, window);

	_clientSize = Float2((float)width, (float)height);
	X11::XResizeWindow(display, window, width, height);
	X11::XMoveWindow(display, window, x, y);
	X11::XFlush(display);
}

void LinuxWindow::SetPosition(const Float2& position)
{
	int32 x = Math::TruncToInt(position.X);
	int32 y = Math::TruncToInt(position.Y);
	LINUX_WINDOW_PROLOG;
    if (!display)
        return;
	X11::XMoveWindow(display, window, x, y);
	X11::XFlush(display);
}

void LinuxWindow::SetClientPosition(const Float2& position)
{
	int32 x = Math::TruncToInt(position.X);
	int32 y = Math::TruncToInt(position.Y);
	LINUX_WINDOW_PROLOG;
    if (!display)
        return;
	X11::XWindowAttributes xwa;
	X11::XGetWindowAttributes(display, window, &xwa);
	X11::XMoveWindow(display, window, x - xwa.border_width, y - xwa.border_width);
	X11::XFlush(display);
}

Float2 LinuxWindow::GetPosition() const
{
	LINUX_WINDOW_PROLOG;
    if (!display)
        return Float2::Zero;
	X11::XWindowAttributes xwa;
	X11::XGetWindowAttributes(display, window, &xwa);
	return Float2((float)xwa.x, (float)xwa.y);
}

Float2 LinuxWindow::GetSize() const
{
	LINUX_WINDOW_PROLOG;
    if (!display)
        return _clientSize;
	X11::XWindowAttributes xwa;
	X11::XGetWindowAttributes(display, window, &xwa);
	return Float2((float)(xwa.width + xwa.border_width), (float)(xwa.height + xwa.border_width));
}

Float2 LinuxWindow::GetClientSize() const
{
	return _clientSize;
}

Float2 LinuxWindow::ScreenToClient(const Float2& screenPos) const
{
	LINUX_WINDOW_PROLOG;
    if (!display)
        return screenPos;
	int32 x, y;
	X11::Window child;
	X11::XTranslateCoordinates(display, X11_DefaultRootWindow(display), window, (int32)screenPos.X, (int32)screenPos.Y, &x, &y, &child);
	return Float2((float)x, (float)y);
}

Float2 LinuxWindow::ClientToScreen(const Float2& clientPos) const
{
	LINUX_WINDOW_PROLOG;
    if (!display)
        return clientPos;
	int32 x, y;
	X11::Window child;
	X11::XTranslateCoordinates(display, window, X11_DefaultRootWindow(display), (int32)clientPos.X, (int32)clientPos.Y, &x, &y, &child);
	return Float2((float)x, (float)y);
}

void LinuxWindow::FlashWindow()
{
	LINUX_WINDOW_PROLOG;
    if (!display)
        return;

	const X11::Atom wmState = X11::XInternAtom(display, "_NET_WM_STATE", 0);
	const X11::Atom wmAttention = X11::XInternAtom(display, "_NET_WM_STATE_DEMANDS_ATTENTION", 0);

	X11::XEvent event;
	Platform::MemoryClear(&event, sizeof(event));
	event.type = ClientMessage;
	event.xclient.window = window;
	event.xclient.message_type = wmState;
	event.xclient.format = 32;
	event.xclient.data.l[0] = _NET_WM_STATE_ADD;
	event.xclient.data.l[1] = wmAttention;

	X11::XSendEvent(display, X11_DefaultRootWindow(display), 0, SubstructureRedirectMask | SubstructureNotifyMask, &event);
}

void LinuxWindow::GetScreenInfo(int32& x, int32& y, int32& width, int32& height) const
{
	LINUX_WINDOW_PROLOG;
	x = y = width = height = 0;
    if (!display)
        return;

	int event, err;
	const bool ok = X11::XineramaQueryExtension(display, &event, &err);
	if (!ok)
		return;

	X11::XWindowAttributes xwa;
	X11::XGetWindowAttributes(display, window, &xwa);
	int32 posX = xwa.x;
	int32 posY = xwa.y;

	int count;
	X11::XineramaScreenInfo* xsi = X11::XineramaQueryScreens(display, &count);
	for (int32 i = 0; i < count; i++)
	{
		auto& screen = xsi[i];
		if (screen.x_org <= posX && screen.y_org <= posY && posX < screen.x_org + screen.width && posY < screen.y_org + screen.height)
		{
			x = screen.x_org;
			y = screen.y_org;
			width = screen.width;
			height = screen.height;
			break;
		}
	}

	X11::XFree(xsi);
}

void LinuxWindow::CheckForWindowResize()
{
	// Skip for minimized window
	if (_minimized)
		return;

	LINUX_WINDOW_PROLOG;
    if (!display)
        return;

	// Get client size
	X11::XWindowAttributes xwa;
	X11::XGetWindowAttributes(display, window, &xwa);
	const int32 width = xwa.width;
	const int32 height = xwa.height;
	const Float2 clientSize((float)width, (float)height);

    // Check if window size has been changed
	if (clientSize != _clientSize && width > 0 && height > 0)
	{
		_clientSize = clientSize;
        OnResize(width, height);
    }
}

void LinuxWindow::OnKeyPress(void* event)
{
	auto keyEvent = (X11::XKeyPressedEvent*)event;
	auto display = (X11::Display*)LinuxPlatform::GetXDisplay();

	Input::Keyboard->OnKeyDown(KeyCodeMap[keyEvent->keycode]);

	// Process text input
	X11::KeySym keySym = X11::XkbKeycodeToKeysym(display, (X11::KeyCode)keyEvent->keycode, 0, 0);

	// Check if input manager wants this event
	if (X11::XFilterEvent((X11::XEvent*)event, 0) != 0)
		return;

	// Send a text input event
	int status;
	char buffer[16];
	int32 length = X11::Xutf8LookupString(IC, keyEvent, buffer, sizeof(buffer), nullptr, &status);
	if (length > 0)
	{
		buffer[length] = '\0';

		String utfStr(buffer);
		if (utfStr.HasChars())
		{
			Input::Keyboard->OnCharInput(utfStr[0]);
		}
	}
}

void LinuxWindow::OnKeyRelease(void* event)
{
	auto keyEvent = (X11::XKeyPressedEvent*)event;

	Input::Keyboard->OnKeyUp(KeyCodeMap[keyEvent->keycode]);
}

void LinuxWindow::OnButtonPress(void* event)
{
	auto buttonEvent = (X11::XButtonPressedEvent*)event;

	Float2 mousePos((float)buttonEvent->x, (float)buttonEvent->y);
	MouseButton mouseButton;
	switch (buttonEvent->button)
	{
	case Button1:
		mouseButton = MouseButton::Left;
		break;
	case Button2:
		mouseButton = MouseButton::Middle;
		break;
	case Button3:
		mouseButton = MouseButton::Right;
		break;
	case 8:
		mouseButton = MouseButton::Extended2;
		break;
	case 9:
		mouseButton = MouseButton::Extended1;
		break;
	default:
		return;
	}

	// Handle double-click
	if (buttonEvent->button == Button1 && !Input::Mouse->IsRelative())
	{
		if (
			buttonEvent->time < (MouseLastButtonPressTime + MouseDoubleClickTime) &&
			Float2::DistanceSquared(mousePos, OldMouseClickPosition) < MaxDoubleClickDistanceSquared)
		{
			Input::Mouse->OnMouseDoubleClick(ClientToScreen(mousePos), mouseButton, this);
			MouseLastButtonPressTime = 0;
			OldMouseClickPosition = mousePos;
			return;
		}
		else
		{
			MouseLastButtonPressTime = buttonEvent->time;
			OldMouseClickPosition = mousePos;
		}
	}

	Input::Mouse->OnMouseDown(ClientToScreen(mousePos), mouseButton, this);
}

void LinuxWindow::OnButtonRelease(void* event)
{
	auto buttonEvent = (X11::XButtonReleasedEvent*)event;
	Float2 mousePos((float)buttonEvent->x, (float)buttonEvent->y);
	switch (buttonEvent->button)
	{
	case Button1:
		Input::Mouse->OnMouseUp(ClientToScreen(mousePos), MouseButton::Left, this);
		break;
	case Button2:
		Input::Mouse->OnMouseUp(ClientToScreen(mousePos), MouseButton::Middle, this);
		break;
	case Button3:
		Input::Mouse->OnMouseUp(ClientToScreen(mousePos), MouseButton::Right, this);
		break;
	case Button4:
		Input::Mouse->OnMouseWheel(ClientToScreen(mousePos), 1.0f, this);
		break;
	case Button5:
		Input::Mouse->OnMouseWheel(ClientToScreen(mousePos), -1.0f, this);
		break;
	case 8:
		Input::Mouse->OnMouseUp(ClientToScreen(mousePos), MouseButton::Extended2, this);
		break;
	case 9:
		Input::Mouse->OnMouseUp(ClientToScreen(mousePos), MouseButton::Extended1, this);
		break;
	default:
		return;
	}
}

void LinuxWindow::OnMotionNotify(void* event)
{
	auto motionEvent = (X11::XMotionEvent*)event;
	Float2 mousePos((float)motionEvent->x, (float)motionEvent->y);
	Input::Mouse->OnMouseMove(ClientToScreen(mousePos), this);
}

void LinuxWindow::OnLeaveNotify(void* event)
{
	auto crossingEvent = (X11::XCrossingEvent*)event;
	Input::Mouse->OnMouseLeave(this);
}

void LinuxWindow::OnConfigureNotify(void* event)
{
	auto configureEvent = (X11::XConfigureEvent*)event;
	const Float2 clientSize((float)configureEvent->width, (float)configureEvent->height);
	if (clientSize != _clientSize)
	{
		_clientSize = clientSize;
		OnResize(configureEvent->width, configureEvent->height);
	}
}

void LinuxWindow::Maximize(bool enable)
{
	LINUX_WINDOW_PROLOG;
    if (!display)
        return;

	X11::Atom wmState = X11::XInternAtom(display, "_NET_WM_STATE", 0);
	X11::Atom wmMaxHorz = X11::XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", 0);
	X11::Atom wmMaxVert = X11::XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", 0);

	if (IsWindowMapped())
	{
		X11::XEvent event;
		Platform::MemoryClear(&event, sizeof(event));
		event.type = ClientMessage;
		event.xclient.window = window;
		event.xclient.message_type = wmState;
		event.xclient.format = 32;
		event.xclient.data.l[0] = enable ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
		event.xclient.data.l[1] = wmMaxHorz;
		event.xclient.data.l[2] = wmMaxVert;

		XSendEvent(display, X11_DefaultRootWindow(display), 0, SubstructureRedirectMask | SubstructureNotifyMask, &event);
	}
	else if (enable)
	{
		X11::Atom states[2];
        states[0] = wmMaxVert;
        states[1] = wmMaxHorz;
		X11::XChangeProperty(display, window, wmState, (X11::Atom)4, 32, PropModeReplace, (unsigned char*)states, 2);
	}
}

void LinuxWindow::Minimize(bool enable)
{
	LINUX_WINDOW_PROLOG;
    if (!display)
        return;

	X11::Atom wmChange = X11::XInternAtom(display, "WM_CHANGE_STATE", 0);

	X11::XEvent event;
	Platform::MemoryClear(&event, sizeof(event));
	event.type = ClientMessage;
	event.xclient.window = window;
	event.xclient.message_type = wmChange;
	event.xclient.format = 32;
	event.xclient.data.l[0] = enable ? WM_IconicState : WM_NormalState;

	XSendEvent(display, X11_DefaultRootWindow(display), 0, SubstructureRedirectMask | SubstructureNotifyMask, &event);
}

bool LinuxWindow::IsWindowMapped()
{
	LINUX_WINDOW_PROLOG;
    if (!display)
        return false;
	X11::XWindowAttributes xwa;
	X11::XGetWindowAttributes(display, window, &xwa);
	return xwa.map_state != IsUnmapped;
}

float LinuxWindow::GetOpacity() const
{
	return _opacity;
}

void LinuxWindow::SetOpacity(const float opacity)
{
	LINUX_WINDOW_PROLOG;
    if (!display)
        return;

	if (Math::IsOne(opacity))
	{
		X11::XDeleteProperty(display, window, xAtomWmWindowOpacity);
	}
	else
	{
		const uint32 fullyOpaque = 0xFFFFFFFF;
		const long alpha = (long)((double)opacity * (double)fullyOpaque);
		X11::XChangeProperty(display, window, xAtomWmWindowOpacity, 6, 32, PropModeReplace, (unsigned char *)&alpha, 1);
	}

	_opacity = opacity;
}

void LinuxWindow::Focus()
{
	if (_focused || !IsWindowMapped())
		return;

	LINUX_WINDOW_PROLOG;
    if (!display)
        return;

	X11::XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);
	X11::XFlush(display);
	X11::XSync(display, 0);
}

void LinuxWindow::SetTitle(const StringView& title)
{
	LINUX_WINDOW_PROLOG;
	_title = title;
    if (!display)
        return;
	const StringAsANSI<512> titleAnsi(title.Get(), title.Length());
	const char* text = titleAnsi.Get();

	X11::XStoreName(display, window, text);
	X11::XSetIconName(display, window, text);

	const X11::Atom netWmName = X11::XInternAtom(display, "_NET_WM_NAME", false);
	const X11::Atom utf8String = X11::XInternAtom(display, "UTF8_STRING", false);
	X11::XChangeProperty(display, window, netWmName, utf8String, 8, PropModeReplace, (unsigned char*)text, titleAnsi.Length());

	X11::XTextProperty titleProp;
	const int status = X11::Xutf8TextListToTextProperty(display, (char**)&text, 1, X11::XUTF8StringStyle, &titleProp);
	if (status == Success)
	{
		X11::XSetTextProperty(display, window, &titleProp, xAtomWmName);
		//X11::XSetWMIconName(display, window, &titleProp);
		X11::XFree(titleProp.value);
	}
}

void LinuxWindow::StartTrackingMouse(bool useMouseScreenOffset)
{
    MouseTrackingWindow = this;
}

void LinuxWindow::EndTrackingMouse()
{
    if (MouseTrackingWindow == this)
        MouseTrackingWindow = nullptr;
}

void LinuxWindow::SetCursor(CursorType type)
{
	WindowBase::SetCursor(type);

	LINUX_WINDOW_PROLOG;
    if (!display)
        return;
	X11::XDefineCursor(display, window, Cursors[(int32)type]);
}

bool IconErrorFlag;

int SetIconErrorHandler(X11::Display *dpy, X11::XErrorEvent *ev)
{
	IconErrorFlag = true;
	return 0;
}

void LinuxWindow::SetIcon(TextureData& icon)
{
	WindowBase::SetIcon(icon);

	LINUX_WINDOW_PROLOG;
    if (!display)
        return;
	IconErrorFlag = false;
	int (*oldHandler)(X11::Display*, X11::XErrorEvent*) = X11::XSetErrorHandler(&SetIconErrorHandler);
	X11::Atom iconAtom = X11::XInternAtom(display, "_NET_WM_ICON", 0);
    X11::Atom cardinalAtom = X11::XInternAtom(display, "CARDINAL", 0);

	if (icon.Width != 0)
	{
		TextureData img;
		if (icon.Format == PixelFormat::R8G8B8A8_UNorm)
		{
			img = icon;
		}
		else
		{
			img.Width = icon.Width;
			img.Height = icon.Height;
			img.Depth = icon.Depth;
			img.Format = PixelFormat::R8G8B8A8_UNorm;
			img.Items.Resize(1);
			auto& item = img.Items[0];
			item.Mips.Resize(1);

			auto& mip = item.Mips[0];
			mip.RowPitch = img.Width * sizeof(Color32);
			mip.DepthPitch = mip.RowPitch * img.Height;
			mip.Lines = img.Height;
			mip.Data.Allocate(mip.DepthPitch);

			const auto mipData = mip.Data.Get<Color32>();
			const auto iconData = icon.GetData(0, 0);
			const Int2 iconSize(icon.Width, icon.Height);
			const auto sampler = TextureTool::GetSampler(icon.Format);
			for (int32 y = 0; y < icon.Height; y++)
			{
				for (int32 x = 0; x < icon.Width; x++)
				{
					const Float2 uv((float)x / icon.Width, (float)y / icon.Height);
					Color color = TextureTool::SampleLinear(sampler, uv, iconData->Data.Get(), iconSize, iconData->RowPitch);
					*(mipData + y * icon.Width + x) = Color32(color);
				}
			}
		}

		while (true)
		{
			if (IconErrorFlag)
			{
				IconErrorFlag = false;

				LOG(Warning, "Icon too large, attempting to resize icon.");

				int32 newWidth, newHeight;
				if (img.Width > img.Height)
				{
					newWidth = img.Width / 2;
					newHeight = img.Height * newWidth / img.Width;
				} else
				{
					newHeight = img.Height / 2;
					newWidth = img.Width * newHeight / img.Height;
				}

				if (!newWidth || !newHeight)
				{
					LOG(Warning, "Unable to set icon.");
					break;
				}

				BytesContainer newData;
				newData.Allocate(newWidth * newHeight * sizeof(Color32));

				auto& mip = img.Items[0].Mips[0];
				const auto mipData = mip.Data.Get<Color32>();
				const auto iconData = icon.GetData(0, 0);
				const Int2 iconSize(icon.Width, icon.Height);
				const auto sampler = TextureTool::GetSampler(img.Format);
				for (int32 y = 0; y < newHeight; y++)
				{
					for (int32 x = 0; x < newWidth; x++)
					{
						const Float2 uv((float)x / newWidth, (float)y / newHeight);
						Color color = TextureTool::SampleLinear(sampler, uv, iconData->Data.Get(), iconSize, iconData->RowPitch);
						*(mipData + y * newWidth + x) = Color32(color);
					}
				}

				img.Width = newWidth;
				img.Height = newHeight;
				mip.RowPitch = img.Width * sizeof(Color32);
				mip.DepthPitch = mip.RowPitch * img.Height;
				mip.Lines = img.Height;
				mip.Data.Swap(newData);
			}

			// Use long to have wordsize (32bit build -> 32 bits, 64bit build -> 64 bits)
			Array<long> pd;
			pd.Resize(2 + img.Width * img.Height);
			pd[0] = img.Width;
			pd[1] = img.Height;
			const uint8_t* pr = (const uint8_t*)img.Items[0].Mips[0].Data.Get();
			long *wr = &pd[2];
			for (int i = 0; i < img.Width * img.Height; i++)
			{
				long v = 0;
				//    A             R             G            B
				v |= pr[3] << 24 | pr[0] << 16 | pr[1] << 8 | pr[2];
				*wr++ = v;
				pr += 4;
			}

			X11::XChangeProperty(display, window, iconAtom, cardinalAtom, 32, PropModeReplace, (unsigned char*)pd.Get(), pd.Count());

			if (!IconErrorFlag)
				break;
		}
	}
	else
	{
		X11::XDeleteProperty(display, window, iconAtom);
	}

	X11::XFlush(display);
	X11::XSetErrorHandler(oldHandler);
}

#endif
