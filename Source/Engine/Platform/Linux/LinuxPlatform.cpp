// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if PLATFORM_LINUX

#include "LinuxPlatform.h"
#include "LinuxWindow.h"
#include "LinuxInput.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/HashFunctions.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Core/Collections/HashFunctions.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Platform/CPUInfo.h"
#include "Engine/Platform/MemoryStats.h"
#include "Engine/Platform/StringUtils.h"
#include "Engine/Platform/MessageBox.h"
#include "Engine/Platform/WindowsManager.h"
#include "Engine/Platform/CreateProcessSettings.h"
#include "Engine/Platform/Clipboard.h"
#include "Engine/Platform/IGuiData.h"
#include "Engine/Platform/Base/PlatformUtils.h"
#include "Engine/Utilities/StringConverter.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Input/Keyboard.h"
#include "IncludeX11.h"
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <cstdio>
#include <sys/file.h>
#include <net/if.h>
#include <errno.h>
#include <locale.h>
#include <pwd.h>
#include <inttypes.h>
#include <dlfcn.h>
#if CRASH_LOG_ENABLE
#include <signal.h>
#include <execinfo.h>
#endif

CPUInfo UnixCpu;
int ClockSource;
Guid DeviceId;
String UserLocale, ComputerName, HomeDir;
byte MacAddress[6];

#define UNIX_APP_BUFF_SIZE 256

X11::Display* xDisplay = nullptr;
X11::XIM IM = nullptr;
X11::XIC IC = nullptr;
X11::Atom xAtomDeleteWindow;
X11::Atom xAtomXdndEnter;
X11::Atom xAtomXdndPosition;
X11::Atom xAtomXdndLeave;
X11::Atom xAtomXdndDrop;
X11::Atom xAtomXdndActionCopy;
X11::Atom xAtomXdndStatus;
X11::Atom xAtomXdndSelection;
X11::Atom xAtomXdndFinished;
X11::Atom xAtomXdndAware;
X11::Atom xAtomWmState;
X11::Atom xAtomWmStateHidden;
X11::Atom xAtomWmStateMaxVert;
X11::Atom xAtomWmStateMaxHorz;
X11::Atom xAtomWmWindowOpacity;
X11::Atom xAtomWmName;
X11::Atom xAtomClipboard;
X11::Atom xDnDRequested = 0;
X11::Window xDndSourceWindow = 0;
DragDropEffect xDndResult;
Float2 xDndPos;
int32 xDnDVersion = 0;
int32 SystemDpi = 96;
X11::Cursor Cursors[(int32)CursorType::MAX];
X11::XcursorImage* CursorsImg[(int32)CursorType::MAX];
Dictionary<StringAnsi, X11::KeyCode> KeyNameMap;
Array<KeyboardKeys> KeyCodeMap;
#if !PLATFORM_SDL
Delegate<void*> LinuxPlatform::xEventReceived;
#endif
Window* MouseTrackingWindow = nullptr;

// Message boxes configuration
#define LINUX_DIALOG_MIN_BUTTON_WIDTH 64
#define LINUX_DIALOG_MIN_WIDTH 200
#define LINUX_DIALOG_MIN_HEIGHT 100
#define LINUX_DIALOG_COLOR_BACKGROUND Color32(56, 54, 53, 255)
#define LINUX_DIALOG_COLOR_TEXT Color32(209, 207, 205, 255)
#define LINUX_DIALOG_COLOR_BUTTON_BORDER Color32(140, 135, 129, 255)
#define LINUX_DIALOG_COLOR_BUTTON_BACKGROUND Color32(105, 102, 99, 255)
#define LINUX_DIALOG_COLOR_BUTTON_SELECTED Color32(205, 202, 53, 255)
#define LINUX_DIALOG_FONT "-*-*-medium-r-normal--*-120-*-*-*-*-*-*"
#define LINUX_DIALOG_PRINT(format, ...) printf(format, ##__VA_ARGS__)

typedef enum
{
	MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT = 0x00000001,
	MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT = 0x00000002
} MessageBoxButtonFlags;

typedef struct TextLineData
{
	int width; // Width of this text line
	int length; // String length of this text line
	const char* text; // Text for this line
} TextLineData;

typedef struct
{
	uint32 flags;
	const char* text;
	DialogResult result;

	int x, y; // Text position
	int length; // Text length
	int text_width; // Text width
	Rectangle rect; // Rectangle for entire button
} MessageBoxButtonData;

typedef struct
{
	Window* Parent;
	const char* title;
	const char* message;
	int numbuttons;
	MessageBoxButtonData* buttons;

	int32 resultButtonIndex; // button index or -1

	X11::Display* display;
	int screen;
	X11::Window window;
	long event_mask;
	X11::Atom wm_protocols;
	X11::Atom wm_delete_message;

	int dialog_width; // Dialog box width
	int dialog_height; // Dialog box height

	X11::XFontSet font_set; // for UTF-8 systems
	X11::XFontStruct* font_struct; // Latin1 (ASCII) fallback
	int xtext, ytext; // Text position to start drawing at
	int numlines; // Count of Text lines
	int text_height; // Height for text lines
	TextLineData* linedata;

	int button_press_index; // Index into buttondata/buttonpos for button which is pressed (or -1)
	int mouse_over_index; // Index into buttondata/buttonpos for button mouse is over (or -1)
} MessageBoxData;

// Return width and height for a string
static void GetTextWidthHeight(MessageBoxData* data, const char* str, int nbytes, int* pwidth, int* pheight)
{
	X11::XRectangle overall_ink, overall_logical;
	X11::Xutf8TextExtents(data->font_set, str, nbytes, &overall_ink, &overall_logical);
	*pwidth = overall_logical.width;
	*pheight = overall_logical.height;
}

// Return index of button if position x,y is contained therein
static int GetHitButtonIndex(MessageBoxData* data, int x, int y)
{
	int i;
	int numbuttons = data->numbuttons;
	const MessageBoxButtonData* buttonpos = data->buttons;

	for (i = 0; i < numbuttons; i++)
	{
		const Rectangle* rect = &buttonpos[i].rect;

		if ((x >= rect->GetX()) &&
			(x <= (rect->GetX() + rect->GetWidth())) &&
			(y >= rect->GetY()) &&
			(y <= (rect->GetY() + rect->GetHeight())))
		{
			return i;
		}
	}

	return -1;
}

// Initialize MessageBoxData structure and Display, etc
static int X11_MessageBoxInit(MessageBoxData* data)
{
	data->dialog_width = LINUX_DIALOG_MIN_WIDTH;
	data->dialog_height = LINUX_DIALOG_MIN_HEIGHT;
	data->resultButtonIndex = -1;

	data->display = X11::XOpenDisplay(nullptr);
	if (!data->display)
	{
		LINUX_DIALOG_PRINT("Couldn't open X11 display.\n");
		return 1;
	}

	char** missing = nullptr;
	int num_missing = 0;
	static const char MessageBoxFont[] = LINUX_DIALOG_FONT;
	data->font_set = X11::XCreateFontSet(data->display, MessageBoxFont, &missing, &num_missing, NULL);
	if (missing != nullptr)
	{
		X11::XFreeStringList(missing);
	}
	if (data->font_set == nullptr)
	{
		LINUX_DIALOG_PRINT("Couldn't load font %s", MessageBoxFont);
		data->font_set = X11::XCreateFontSet(data->display, "fixed", &missing, &num_missing, NULL);
		if (missing != nullptr) X11::XFreeStringList(missing);
	}

	return 0;
}

static int CountLinesOfText(const char* text)
{
	int retval = 0;
	while (text && *text)
	{
		const char* lf = strchr(text, '\n');
		retval++; // Even without an endline, this counts as a line
		text = lf ? lf + 1 : NULL;
	}
	return retval;
}

// Calculate and initialize text and button locations
static int X11_MessageBoxInitPositions(MessageBoxData* data)
{
	int i;
	int ybuttons;
	int text_width_max = 0;
	int button_text_height = 0;
	int button_width = LINUX_DIALOG_MIN_BUTTON_WIDTH;

	// Go over text and break linefeeds into separate lines
	if (data->message && data->message[0])
	{
		const char* text = data->message;
		const int linecount = CountLinesOfText(text);
		TextLineData* plinedata = (TextLineData *)malloc(sizeof(TextLineData) * linecount);

		if (!plinedata)
		{
			LINUX_DIALOG_PRINT("Out of memory!");
			return 1;
		}

		data->linedata = plinedata;
		data->numlines = linecount;

		for (i = 0; i < linecount; i++, plinedata++)
		{
			const char* lf = strchr(text, '\n');
			const int length = lf ? (lf - text) : strlen(text);
			int height;

			plinedata->text = text;

			GetTextWidthHeight(data, text, length, &plinedata->width, &height);

			// Text and widths are the largest we've ever seen
			data->text_height = Math::Max(data->text_height, height);
			text_width_max = Math::Max(text_width_max, plinedata->width);

			plinedata->length = length;
			if (lf && (lf > text) && (lf[-1] == '\r'))
			{
				plinedata->length--;
			}

			text += length + 1;

			// Break if there are no more linefeeds
			if (!lf)
				break;
		}

		// Bump up the text height slightly
		data->text_height += 2;
	}

	// Loop through all buttons and calculate the button widths and height
	for (i = 0; i < data->numbuttons; i++)
	{
		int height;

		data->buttons[i].length = strlen(data->buttons[i].text);

		GetTextWidthHeight(data, data->buttons[i].text, strlen(data->buttons[i].text), &data->buttons[i].text_width, &height);

		button_width = Math::Max(button_width, data->buttons[i].text_width);
		button_text_height = Math::Max(button_text_height, height);
	}

	if (data->numlines)
	{
		// x,y for this line of text
		data->xtext = data->text_height;
		data->ytext = data->text_height + data->text_height;

		// Bump button y down to bottom of text
		ybuttons = 3 * data->ytext / 2 + (data->numlines - 1) * data->text_height;

		// Bump the dialog box width and height up if needed
		data->dialog_width = Math::Max(data->dialog_width, 2 * data->xtext + text_width_max);
		data->dialog_height = Math::Max(data->dialog_height, ybuttons);
	}
	else
	{
		// Button y starts at height of button text
		ybuttons = button_text_height;
	}

	if (data->numbuttons)
	{
		const int button_spacing = button_text_height;
		const int button_height = 2 * button_text_height;

		// Bump button width up a bit
		button_width += button_text_height;

		// Get width of all buttons lined up
		const int width_of_buttons = data->numbuttons * button_width + (data->numbuttons - 1) * button_spacing;

		// Bump up dialog width and height if buttons are wider than text
		data->dialog_width = Math::Max(data->dialog_width, width_of_buttons + 2 * button_spacing);
		data->dialog_height = Math::Max(data->dialog_height, ybuttons + 2 * button_height);

		// Location for first button
		int x = (data->dialog_width - width_of_buttons) / 2;
		int y = ybuttons + (data->dialog_height - ybuttons - button_height) / 2;

		for (i = 0; i < data->numbuttons; i++)
		{
			// Button coordinates
			data->buttons[i].rect = Rectangle(x, y, button_width, button_height);

			// Button text coordinates
			data->buttons[i].x = x + (button_width - data->buttons[i].text_width) / 2;
			data->buttons[i].y = y + (button_height - button_text_height - 1) / 2 + button_text_height;

			// Scoot over for next button
			x += button_width + button_spacing;
		}
	}

	return 0;
}

// Create and set up X11 dialog box window
static int X11_MessageBoxCreateWindow(MessageBoxData* data)
{
	int x, y;
	X11::XSetWindowAttributes wnd_attr;
	X11::Display* display = data->display;
	Window* windowdata = NULL;
	X11::Window windowdataWin = 0;
	char* title_locale = NULL;

	if (data->Parent)
	{
		windowdata = data->Parent;
		windowdataWin = (X11::Window)windowdata->GetNativePtr();
		// TODO: place popup on the screen that parent window is
		data->screen = X11_DefaultScreen(display);
	}
	else
	{
		data->screen = X11_DefaultScreen(display);
	}

	data->event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask | FocusChangeMask | PointerMotionMask;
	wnd_attr.event_mask = data->event_mask;

	data->window = X11::XCreateWindow(
		display, X11_RootWindow(display, data->screen),
		0, 0,
		data->dialog_width, data->dialog_height,
		0, CopyFromParent, InputOutput, CopyFromParent,
		CWEventMask, &wnd_attr);
	if (data->window == 0)
	{
		LINUX_DIALOG_PRINT("Couldn't create X window");
		return 1;
	}

	if (windowdata)
	{
		X11::XSetTransientForHint(display, data->window, windowdataWin);
	}

	X11::XStoreName(display, data->window, data->title);

	X11::XTextProperty titleProp;
	const int status = X11::Xutf8TextListToTextProperty(display, (char **)&data->title, 1, X11::XUTF8StringStyle, &titleProp);
	if (status == Success)
	{
		X11::XSetTextProperty(display, data->window, &titleProp, xAtomWmName);
		X11::XFree(titleProp.value);
	}

	// Let the window manager know this is a dialog box
	X11::Atom _NET_WM_WINDOW_TYPE = X11::XInternAtom(display, "_NET_WM_WINDOW_TYPE", 0);
	X11::Atom _NET_WM_WINDOW_TYPE_DIALOG = X11::XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", 0);
	X11::XChangeProperty(display, data->window, _NET_WM_WINDOW_TYPE, 4, 32, PropModeReplace, (unsigned char *)&_NET_WM_WINDOW_TYPE_DIALOG, 1);

	// Allow the window to be deleted by the window manager
	data->wm_protocols = X11::XInternAtom(display, "WM_PROTOCOLS", 0);
	data->wm_delete_message = X11::XInternAtom(display, "WM_DELETE_WINDOW", 0);
	X11::XSetWMProtocols(display, data->window, &data->wm_delete_message, 1);

	if (windowdata)
	{
		X11::XWindowAttributes attrib;
		X11::Window dummy;

		X11::XGetWindowAttributes(display, windowdataWin, &attrib);
		x = attrib.x + (attrib.width - data->dialog_width) / 2;
		y = attrib.y + (attrib.height - data->dialog_height) / 3;
		X11::XTranslateCoordinates(display, windowdataWin, X11_RootWindow(display, data->screen), x, y, &x, &y, &dummy);
	}
	else
	{
		int screenCount;
		X11::XineramaScreenInfo* xsi = X11::XineramaQueryScreens(xDisplay, &screenCount);
		ASSERT(data->screen < screenCount);
		x = (float)xsi[data->screen].x_org + ((float)xsi[data->screen].width - data->dialog_width) / 2;
		y = (float)xsi[data->screen].y_org + ((float)xsi[data->screen].height - data->dialog_height) / 2;
	}
	X11::XMoveWindow(display, data->window, x, y);

	X11::XSizeHints* sizeHints = X11::XAllocSizeHints();
	if (sizeHints)
	{
		sizeHints->flags = USPosition | USSize | PMaxSize | PMinSize;
		sizeHints->x = x;
		sizeHints->y = y;
		sizeHints->width = data->dialog_width;
		sizeHints->height = data->dialog_height;

		sizeHints->min_width = sizeHints->max_width = data->dialog_width;
		sizeHints->min_height = sizeHints->max_height = data->dialog_height;

		X11::XSetWMNormalHints(display, data->window, sizeHints);

		X11::XFree(sizeHints);
	}

	X11::XMapRaised(display, data->window);

	return 0;
}

// Draw the message box
static void X11_MessageBoxDraw(MessageBoxData* data, X11::GC ctx)
{
	const X11::Drawable window = data->window;
	X11::Display* display = data->display;

	X11::XSetForeground(display, ctx, LINUX_DIALOG_COLOR_BACKGROUND.GetAsRGB());
	X11::XFillRectangle(display, window, ctx, 0, 0, data->dialog_width, data->dialog_height);

	X11::XSetForeground(display, ctx, LINUX_DIALOG_COLOR_TEXT.GetAsRGB());
	for (int i = 0; i < data->numlines; i++)
	{
		const TextLineData* lineData = &data->linedata[i];
		X11::XDrawString(display, window, ctx, data->xtext, data->ytext + i * data->text_height, lineData->text, lineData->length);
	}

	for (int i = 0; i < data->numbuttons; i++)
	{
		MessageBoxButtonData* button = &data->buttons[i];
		const int border = (button->flags & MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT) ? 2 : 0;
		const int offset = ((data->mouse_over_index == i) && (data->button_press_index == data->mouse_over_index)) ? 1 : 0;

		X11::XSetForeground(display, ctx, LINUX_DIALOG_COLOR_BUTTON_BACKGROUND.GetAsRGB());
		X11::XFillRectangle(display, window, ctx, button->rect.GetX() - border, button->rect.GetY() - border, button->rect.GetWidth() + 2 * border, button->rect.GetHeight() + 2 * border);

		X11::XSetForeground(display, ctx, LINUX_DIALOG_COLOR_BUTTON_BORDER.GetAsRGB());
		X11::XDrawRectangle(display, window, ctx, button->rect.GetX(), button->rect.GetY(), button->rect.GetWidth(), button->rect.GetHeight());

		X11::XSetForeground(display, ctx, (data->mouse_over_index == i) ? LINUX_DIALOG_COLOR_BUTTON_SELECTED.GetAsRGB() : LINUX_DIALOG_COLOR_TEXT.GetAsRGB());

		X11::Xutf8DrawString(display, window, data->font_set, ctx, button->x + offset, button->y + offset, button->text, button->length);
	}
}

static int X11_MessageBoxEventTest(X11::Display* display, X11::XEvent* event, X11::XPointer arg)
{
	const MessageBoxData* data = (const MessageBoxData *)arg;
	return ((event->xany.display == data->display) && (event->xany.window == data->window)) ? 1 : 0;
}

// Loop and handle message box event messages until something kills it
static int X11_MessageBoxLoop(MessageBoxData* data)
{
	X11::XGCValues ctx_vals;
	bool close_dialog = false;
	bool has_focus = true;
	X11::KeySym last_key_pressed = XK_VoidSymbol;
	unsigned long gcflags = GCForeground | GCBackground;

	Platform::MemoryClear(&ctx_vals, sizeof(ctx_vals));
	ctx_vals.foreground = LINUX_DIALOG_COLOR_BACKGROUND.GetAsRGB();
	ctx_vals.background = LINUX_DIALOG_COLOR_BACKGROUND.GetAsRGB();

	const X11::GC ctx = X11::XCreateGC(data->display, data->window, gcflags, &ctx_vals);
	if (ctx == nullptr)
	{
		LINUX_DIALOG_PRINT("Couldn't create graphics context");
		return 1;
	}

	data->button_press_index = -1; // Reset what button is currently depressed
	data->mouse_over_index = -1; // Reset what button the mouse is over

	while (!close_dialog)
	{
		X11::XEvent e;
		bool draw = true;

		// Can't use XWindowEvent() because it can't handle ClientMessage events
		// Can't use XNextEvent() because we only want events for this window
		X11::XIfEvent(data->display, &e, X11_MessageBoxEventTest, (X11::XPointer)data);

		// If X11::XFilterEvent returns True, then some input method has filtered the event, and the client should discard the event
		if ((e.type != Expose) && X11::XFilterEvent(&e, 0))
			continue;

		switch (e.type)
		{
		case Expose:
			if (e.xexpose.count > 0)
			{
				draw = false;
			}
			break;
		case FocusIn:
			// Got focus.
			has_focus = true;
			break;
		case FocusOut:
			// Lost focus. Reset button and mouse info
			has_focus = false;
			data->button_press_index = -1;
			data->mouse_over_index = -1;
			break;
		case MotionNotify:
			if (has_focus)
			{
				// Mouse moved
				const int previndex = data->mouse_over_index;
				data->mouse_over_index = GetHitButtonIndex(data, e.xbutton.x, e.xbutton.y);
				if (data->mouse_over_index == previndex)
				{
					draw = false;
				}
			}
			break;
		case ClientMessage:
			if (e.xclient.message_type == data->wm_protocols &&
				e.xclient.format == 32 &&
				e.xclient.data.l[0] == data->wm_delete_message)
			{
				close_dialog = true;
			}
			break;
		case KeyPress:
			// Store key press - we make sure in key release that we got both
			last_key_pressed = X11::XLookupKeysym(&e.xkey, 0);
			break;
		case KeyRelease:
		{
			uint32 mask = 0;
			const X11::KeySym key = X11::XLookupKeysym(&e.xkey, 0);

			// If this is a key release for something we didn't get the key down for, then bail
			if (key != last_key_pressed)
				break;

			if (key == XK_Escape)
				mask = MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
			else if ((key == XK_Return) || (key == XK_KP_Enter))
				mask = MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;

			if (mask)
			{
				// Look for first button with this mask set, and return it if found
				for (int buttonIndex = 0; buttonIndex < data->numbuttons; buttonIndex++)
				{
					const auto button = &data->buttons[buttonIndex];
					if (button->flags & mask)
					{
						data->resultButtonIndex = buttonIndex;
						close_dialog = true;
						break;
					}
				}
			}
			break;
		}
		case ButtonPress:
			data->button_press_index = -1;
			if (e.xbutton.button == Button1)
			{
				// Find index of button they clicked on
				data->button_press_index = GetHitButtonIndex(data, e.xbutton.x, e.xbutton.y);
			}
			break;
		case ButtonRelease:
			// If button is released over the same button that was clicked down on, then return it
			if ((e.xbutton.button == Button1) && (data->button_press_index >= 0))
			{
				const int buttonIndex = GetHitButtonIndex(data, e.xbutton.x, e.xbutton.y);
				if (data->button_press_index == buttonIndex)
				{
					const MessageBoxButtonData* button = &data->buttons[buttonIndex];
					data->resultButtonIndex = buttonIndex;
					close_dialog = true;
				}
			}
			data->button_press_index = -1;
			break;
		}

		if (draw)
		{
			// Draw our dialog box
			X11_MessageBoxDraw(data, ctx);
		}
	}

	X11::XFreeGC(data->display, ctx);
	return 0;
}

#if !PLATFORM_SDL
DialogResult MessageBox::Show(Window* parent, const StringView& text, const StringView& caption, MessageBoxButtons buttons, MessageBoxIcon icon)
#else
DialogResult MessageBox::ShowFallback(Window* parent, const StringView& text, const StringView& caption, MessageBoxButtons buttons, MessageBoxIcon icon)
#endif
{
    if (CommandLine::Options.Headless)
        return DialogResult::None;

	// Setup for simple popup
	const StringAsANSI<127> textAnsi(text.Get(), text.Length());
	const StringAsANSI<511> captionAnsi(caption.Get(), caption.Length());
	MessageBoxData data;
	MessageBoxButtonData buttonsData[3];
	Platform::MemoryClear(&data, sizeof(data));
	data.title = captionAnsi.Get();
	data.message = textAnsi.Get();
	data.numbuttons = 1;
	data.buttons = buttonsData;
	data.Parent = parent;
	Platform::MemoryClear(&buttonsData, sizeof(buttonsData));
	switch (buttons)
	{
	case MessageBoxButtons::AbortRetryIgnore:
	{
		data.numbuttons = 3;

		// Abort
		auto& abort = buttonsData[0];
		abort.text = "Abort";
		abort.result = DialogResult::Abort;
		abort.flags |= MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
		abort.flags |= MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;

		// Retry
		auto& retry = buttonsData[1];
		retry.text = "Retry";
		retry.result = DialogResult::Retry;
		retry.flags |= MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
		retry.flags |= MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;

		// Ignore
		auto& ignore = buttonsData[2];
		ignore.text = "Ignore";
		ignore.result = DialogResult::Ignore;
		ignore.flags |= MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
		ignore.flags |= MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;

		break;
	}
	case MessageBoxButtons::OK:
	{
		data.numbuttons = 1;

		// OK
		auto& ok = buttonsData[0];
		ok.text = "OK";
		ok.result = DialogResult::OK;
		ok.flags |= MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
		ok.flags |= MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;

		break;
	}
	case MessageBoxButtons::OKCancel:
	{
		data.numbuttons = 2;

		// OK
		auto& ok = buttonsData[0];
		ok.text = "OK";
		ok.result = DialogResult::OK;
		ok.flags |= MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;

		// Cancel
		auto& cancel = buttonsData[1];
		cancel.text = "Cancel";
		cancel.result = DialogResult::Cancel;
		cancel.flags |= MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;

		break;
	}
	case MessageBoxButtons::RetryCancel:
	{
		data.numbuttons = 2;

		// Retry
		auto& retry = buttonsData[0];
		retry.text = "Retry";
		retry.result = DialogResult::Retry;
		retry.flags |= MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;

		// Cancel
		auto& cancel = buttonsData[1];
		cancel.text = "Cancel";
		cancel.result = DialogResult::Cancel;
		cancel.flags |= MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;

		break;
	}
	case MessageBoxButtons::YesNo:
	{
		data.numbuttons = 2;

		// Yes
		auto& yes = buttonsData[0];
		yes.text = "Yes";
		yes.result = DialogResult::Yes;
		yes.flags |= MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;

		// No
		auto& no = buttonsData[1];
		no.text = "No";
		no.result = DialogResult::No;
		no.flags |= MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;

		break;
	}
	case MessageBoxButtons::YesNoCancel:
	{
		data.numbuttons = 3;

		// Yes
		auto& yes = buttonsData[0];
		yes.text = "Yes";
		yes.result = DialogResult::Yes;
		yes.flags |= MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;

		// No
		auto& no = buttonsData[1];
		no.text = "No";
		no.result = DialogResult::No;
		no.flags |= MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;

		// Cancel
		auto& cancel = buttonsData[2];
		cancel.text = "Cancel";
		cancel.result = DialogResult::Cancel;

		break;
	}
	default:
		LINUX_DIALOG_PRINT("Invalid message box buttons setup.");
		return DialogResult::None;
	}
	// TODO: add support for icon

	// Init and display the message box
	int ret = X11_MessageBoxInit(&data);
	if (ret != -1)
	{
		ret = X11_MessageBoxInitPositions(&data);
		if (ret != -1)
		{
			ret = X11_MessageBoxCreateWindow(&data);
			if (ret != -1)
			{
				ret = X11_MessageBoxLoop(&data);
			}
		}
	}

	// Cleanup data
	{
		if (data.font_set != nullptr)
		{
			X11::XFreeFontSet(data.display, data.font_set);
		}

		if (data.font_struct != nullptr)
		{
			X11::XFreeFont(data.display, data.font_struct);
		}

		if (data.display)
		{
			if (data.window != 0)
			{
				X11::XWithdrawWindow(data.display, data.window, data.screen);
				X11::XDestroyWindow(data.display, data.window);
			}

			X11::XCloseDisplay(data.display);
		}

		free(data.linedata);
	}

	// Get the result
	return data.resultButtonIndex == -1 ? DialogResult::None : data.buttons[data.resultButtonIndex].result;
}

#if !PLATFORM_SDL

int X11ErrorHandler(X11::Display* display, X11::XErrorEvent* event)
{
    if (event->error_code == 5)
        return 0; // BadAtom (invalid Atom parameter)
	char buffer[256];
	XGetErrorText(display, event->error_code, buffer, sizeof(buffer));
	LOG(Error, "X11 Error: {0}", String(buffer));
	return 0;
}

#endif

int32 CalculateDpi()
{
	int dpi = 96;
	char* resourceString = X11::XResourceManagerString(xDisplay);
	if (resourceString == NULL)
		return dpi;

	char* type = NULL;
	X11::XrmValue value;
	X11::XrmDatabase database = X11::XrmGetStringDatabase(resourceString);
	if (X11::XrmGetResource(database, "Xft.dpi", "String", &type, &value) == 1 && value.addr != NULL)
		dpi = (int)atof(value.addr);
	return dpi;
}

// Maps Flax key codes to X11 names for physical key locations.
const char* ButtonCodeToKeyName(KeyboardKeys code)
{
	switch(code)
	{
		// Row #1
	case KeyboardKeys::Escape: return "ESC";
	case KeyboardKeys::F1: return "FK01";
	case KeyboardKeys::F2: return "FK02";
	case KeyboardKeys::F3: return "FK03";
	case KeyboardKeys::F4: return "FK04";
	case KeyboardKeys::F5: return "FK05";
	case KeyboardKeys::F6: return "FK06";
	case KeyboardKeys::F7: return "FK07";
	case KeyboardKeys::F8: return "FK08";
	case KeyboardKeys::F9: return "FK09";
	case KeyboardKeys::F10: return "FK10";
	case KeyboardKeys::F11: return "FK11";
	case KeyboardKeys::F12: return "FK12";
	case KeyboardKeys::F13: return "FK13";
	case KeyboardKeys::F14: return "FK14";
	case KeyboardKeys::F15: return "FK15";

		// Row #2
	case KeyboardKeys::BackQuote: return "TLDE";
	case KeyboardKeys::Alpha1:  return "AE01";
	case KeyboardKeys::Alpha2: return "AE02";
	case KeyboardKeys::Alpha3: return "AE03";
	case KeyboardKeys::Alpha4: return "AE04";
	case KeyboardKeys::Alpha5: return "AE05";
	case KeyboardKeys::Alpha6: return "AE06";
	case KeyboardKeys::Alpha7: return "AE07";
	case KeyboardKeys::Alpha8: return "AE08";
	case KeyboardKeys::Alpha9: return "AE09";
	case KeyboardKeys::Alpha0: return "AE10";
	case KeyboardKeys::Minus: return "AE11";
	case KeyboardKeys::Plus: return "AE12";
	case KeyboardKeys::Backspace: return "BKSP";

		// Row #3
	case KeyboardKeys::Tab: return "TAB";
	case KeyboardKeys::Q: return "AD01";
	case KeyboardKeys::W: return "AD02";
	case KeyboardKeys::E: return "AD03";
	case KeyboardKeys::R: return "AD04";
	case KeyboardKeys::T: return "AD05";
	case KeyboardKeys::Y: return "AD06";
	case KeyboardKeys::U: return "AD07";
	case KeyboardKeys::I: return "AD08";
	case KeyboardKeys::O: return "AD09";
	case KeyboardKeys::P: return "AD10";
	case KeyboardKeys::LeftBracket:	return "AD11";
	case KeyboardKeys::RightBracket: return "AD12";
	case KeyboardKeys::Return: return "RTRN";

		// Row #4
	case KeyboardKeys::Capital:	return "CAPS";
	case KeyboardKeys::A: return "AC01";
	case KeyboardKeys::S: return "AC02";
	case KeyboardKeys::D: return "AC03";
	case KeyboardKeys::F: return "AC04";
	case KeyboardKeys::G: return "AC05";
	case KeyboardKeys::H: return "AC06";
	case KeyboardKeys::J: return "AC07";
	case KeyboardKeys::K: return "AC08";
	case KeyboardKeys::L: return "AC09";
	case KeyboardKeys::Colon: return "AC10";
	case KeyboardKeys::Quote: return "AC11";
	case KeyboardKeys::Backslash: return "BKSL";

		// Row #5
	case KeyboardKeys::Shift: return "LFSH"; // Left Shift
	case KeyboardKeys::Z: return "AB01";
	case KeyboardKeys::X: return "AB02";
	case KeyboardKeys::C: return "AB03";
	case KeyboardKeys::V: return "AB04";
	case KeyboardKeys::B: return "AB05";
	case KeyboardKeys::N: return "AB06";
	case KeyboardKeys::M: return "AB07";
	case KeyboardKeys::Comma: return "AB08";
	case KeyboardKeys::Period: return "AB09";
	case KeyboardKeys::Slash: return "AB10";
	//case KeyboardKeys::Shift: return "RTSH"; // Right Shift

		// Row #6
	case KeyboardKeys::Control:	return "LCTL"; // Left Control
	case KeyboardKeys::LeftWindows: return "LWIN";
	case KeyboardKeys::Alt: return "LALT";
	case KeyboardKeys::Spacebar: return "SPCE";
	case KeyboardKeys::RightMenu: return "RALT";
	case KeyboardKeys::RightWindows: return "RWIN";
	//case KeyboardKeys::Control: return "RCTL"; // Right Control

		// Keypad
	case KeyboardKeys::Numpad0: return "KP0";
	case KeyboardKeys::Numpad1: return "KP1";
	case KeyboardKeys::Numpad2: return "KP2";
	case KeyboardKeys::Numpad3: return "KP3";
	case KeyboardKeys::Numpad4: return "KP4";
	case KeyboardKeys::Numpad5: return "KP5";
	case KeyboardKeys::Numpad6: return "KP6";
	case KeyboardKeys::Numpad7: return "KP7";
	case KeyboardKeys::Numpad8: return "KP8";
	case KeyboardKeys::Numpad9: return "KP9";

	case KeyboardKeys::Numlock: return "NMLK";
	case KeyboardKeys::NumpadDivide: return "KPDV";
	case KeyboardKeys::NumpadMultiply: return "KPMU";
	case KeyboardKeys::NumpadSubtract: return "KPSU";
	case KeyboardKeys::NumpadAdd: return "KPAD";
	case KeyboardKeys::NumpadDecimal: return "KPDL";
	//case KeyboardKeys::: return "KPEN"; // Numpad Enter
	//case KeyboardKeys::: return "KPEQ"; // Numpad Equals

		// Special keys
	case KeyboardKeys::Scroll: return "SCLK";
	case KeyboardKeys::Pause: return "PAUS";

	case KeyboardKeys::Insert: return "INS";
	case KeyboardKeys::Home: return "HOME";
	case KeyboardKeys::PageUp: return "PGUP";
	case KeyboardKeys::Delete: return "DELE";
	case KeyboardKeys::End: return "END";
	case KeyboardKeys::PageDown: return "PGDN";

	case KeyboardKeys::ArrowUp: return "UP";
	case KeyboardKeys::ArrowLeft: return "LEFT";
	case KeyboardKeys::ArrowDown: return "DOWN";
	case KeyboardKeys::ArrowRight: return "RGHT";

	case KeyboardKeys::VolumeMute: return "MUTE";
	case KeyboardKeys::VolumeDown: return "VOL-";
	case KeyboardKeys::VolumeUp: return "VOL+";

		// International keys
	case KeyboardKeys::Oem102: return "LSGT";
	case KeyboardKeys::Kana: return "AB11";

	default:
		break;
	}

	return nullptr;
}

void UnixGetMacAddress(byte result[6])
{
    struct ifreq ifr;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy((char *)ifr.ifr_name, "eth0", IFNAMSIZ - 1);

    ioctl(fd, SIOCGIFHWADDR, &ifr);

    close(fd);

    Platform::MemoryCopy(result, ifr.ifr_hwaddr.sa_data, 6);
}

static int do_scale_by_power(uintmax_t* x, int base, int power)
{
    // Reference: https://github.com/karelzak/util-linux/blob/master/lib/strutils.c

    while (power--)
    {
        if (UINTMAX_MAX / base < *x)
            return -ERANGE;
        *x *= base;
    }
    return 0;
}

int parse_size(const char* str, uintmax_t* res, int* power)
{
    // Reference: https://github.com/karelzak/util-linux/blob/master/lib/strutils.c

    const char* p;
    char* end;
    uintmax_t x, frac = 0;
    int base = 1024, rc = 0, pwr = 0, frac_zeros = 0;

    static const char* suf = "KMGTPEZY";
    static const char* suf2 = "kmgtpezy";
    const char* sp;

    *res = 0;

    if (!str || !*str)
    {
        rc = -EINVAL;
        goto err;
    }

    p = str;
    while (StringUtils::IsWhitespace((char)*p))
        p++;
    if (*p == '-')
    {
        rc = -EINVAL;
        goto err;
    }

    errno = 0, end = NULL;
    x = strtoumax(str, &end, 0);

    if (end == str ||
        (errno != 0 && (x == UINTMAX_MAX || x == 0)))
    {
        rc = errno ? -errno : -EINVAL;
        goto err;
    }
    if (!end || !*end)
        goto done;
    p = end;

check_suffix:
    if (*(p + 1) == 'i' && (*(p + 2) == 'B' || *(p + 2) == 'b') && !*(p + 3))
        base = 1024;
    else if ((*(p + 1) == 'B' || *(p + 1) == 'b') && !*(p + 2))
        base = 1000;
    else if (*(p + 1))
    {
        struct lconv const* l = localeconv();
        const char* dp = l ? l->decimal_point : NULL;
        size_t dpsz = dp ? strlen(dp) : 0;

        if (frac == 0 && *p && dp && strncmp(dp, p, dpsz) == 0)
        {
            const char* fstr = p + dpsz;

            for (p = fstr; *p == '0'; p++)
                frac_zeros++;
            fstr = p;
            if (StringUtils::IsDigit(*fstr))
            {
                errno = 0, end = NULL;
                frac = strtoumax(fstr, &end, 0);
                if (end == fstr ||
                    (errno != 0 && (frac == UINTMAX_MAX || frac == 0)))
                {
                    rc = errno ? -errno : -EINVAL;
                    goto err;
                }
            }
            else
                end = (char *)p;

            if (frac && (!end || !*end))
            {
                rc = -EINVAL;
                goto err;
            }
            p = end;
            goto check_suffix;
        }
        rc = -EINVAL;
        goto err;
    }

    sp = strchr(suf, *p);
    if (sp)
        pwr = (sp - suf) + 1;
    else
    {
        sp = strchr(suf2, *p);
        if (sp)
            pwr = (sp - suf2) + 1;
        else
        {
            rc = -EINVAL;
            goto err;
        }
    }

    rc = do_scale_by_power(&x, base, pwr);
    if (power)
        *power = pwr;
    if (frac && pwr)
    {
        int i;
        uintmax_t frac_div = 10, frac_poz = 1, frac_base = 1;

        do_scale_by_power(&frac_base, base, pwr);

        while (frac_div < frac)
            frac_div *= 10;

        for (i = 0; i < frac_zeros; i++)
            frac_div *= 10;

        do
        {
            unsigned int seg = frac % 10;
            uintmax_t seg_div = frac_div / frac_poz;

            frac /= 10;
            frac_poz *= 10;

            if (seg)
                x += frac_base / (seg_div / seg);
        } while (frac);
    }
done:
    *res = x;
err:
    if (rc < 0)
        errno = -rc;
    return rc;
}

class LinuxKeyboard : public Keyboard
{
public:
	explicit LinuxKeyboard()
		: Keyboard()
	{
	}
};

class LinuxMouse : public Mouse
{
public:
	explicit LinuxMouse()
		: Mouse()
	{
	}

public:

    // [Mouse]
    void SetMousePosition(const Float2& newPosition) final override
    {
        LinuxPlatform::SetMousePosition(newPosition);

        OnMouseMoved(newPosition);
    }
};

#if !PLATFORM_SDL
struct Property
{
    unsigned char* data;
    int format, nitems;
    X11::Atom type;
};
#endif

namespace Impl
{
	LinuxKeyboard* Keyboard;
	LinuxMouse* Mouse;
#if !PLATFORM_SDL
    StringAnsi ClipboardText;

    void ClipboardGetText(String& result, X11::Atom source, X11::Atom atom, X11::Window window)
    {
        X11::Window selectionOwner = X11::XGetSelectionOwner(xDisplay, source);
        if (selectionOwner == 0)
        {
            // No copy owner
            return;
        }
        if (selectionOwner == window)
        {
            // Copy/paste from self
            result.Set(ClipboardText.Get(), ClipboardText.Length());
            return;
        }

        // Send event to get data from the owner
        int format;
        unsigned long N, size;
        char* data;
        X11::Atom target;
        X11::Atom CLIPBOARD = X11::XInternAtom(xDisplay, "CLIPBOARD", 0);
        X11::Atom XSEL_DATA = X11::XInternAtom(xDisplay, "XSEL_DATA", 0);
        X11::Atom UTF8 = X11::XInternAtom(xDisplay, "UTF8_STRING", 1);
        X11::XEvent event;
        X11::XConvertSelection(xDisplay, CLIPBOARD, atom, XSEL_DATA, window, CurrentTime);
        X11::XSync(xDisplay, 0);
        X11::XNextEvent(xDisplay, &event);
        switch(event.type)
        {
        case SelectionNotify:
            if (event.xselection.selection != CLIPBOARD)
                break;
            if (event.xselection.property)
            {
                X11::XGetWindowProperty(event.xselection.display, event.xselection.requestor, event.xselection.property, 0L,(~0L), 0, AnyPropertyType, &target, &format, &size, &N,(unsigned char**)&data);
                if (target == UTF8 || target == (X11::Atom)31)
                {
                    // Got text to paste
                    result.Set(data , size);
                    X11::XFree(data);
                }
                X11::XDeleteProperty(event.xselection.display, event.xselection.requestor, event.xselection.property);
            }
        }
    }

    Property ReadProperty(X11::Display* display, X11::Window window, X11::Atom property)
    {
        X11::Atom readType = 0;
        int readFormat = 0;
        unsigned long nitems = 0;
        unsigned long readBytes = 0;
        unsigned char* result = nullptr;
        int bytesCount = 1024;
        if (property != 0)
        {
            do
            {
                if (result != nullptr)
                    X11::XFree(result);
                XGetWindowProperty(display, window, property, 0, bytesCount, 0, AnyPropertyType, &readType, &readFormat, &nitems, &readBytes, &result);
                bytesCount *= 2;
            } while (readBytes != 0);
        }
        Property p = { result, readFormat, (int)nitems, readType };
        return p;
    }

    static X11::Atom SelectTargetFromList(X11::Display* display, const char* targetType, X11::Atom* list, int count)
    {
        for (int i = 0; i < count; i++)
        {
            X11::Atom atom = list[i];
            if (atom != 0 && StringAnsi(XGetAtomName(display, atom)) == targetType)
                return atom;
        }
        return 0;
    }

    static X11::Atom SelectTargetFromAtoms(X11::Display* display, const char* targetType, X11::Atom t1, X11::Atom t2, X11::Atom t3)
    {
        if (t1 != 0 && StringAnsi(XGetAtomName(display, t1)) == targetType)
            return t1;
        if (t2 != 0 && StringAnsi(XGetAtomName(display, t2)) == targetType)
            return t2;
        if (t3 != 0 && StringAnsi(XGetAtomName(display, t3)) == targetType)
            return t3;
        return 0;
    }

    static X11::Window FindAppWindow(X11::Display* display, X11::Window w)
    {
        int nprops, i = 0;
        X11::Atom* a;
        if (w == 0)
            return 0;
        a = X11::XListProperties(display, w, &nprops);
        for (i = 0; i < nprops; i++)
        {
            if (a[i] == xAtomXdndAware)
                break;
        }
        if (nprops)
            X11::XFree(a);
        if (i != nprops)
            return w;
        X11::Window child, wtmp;
        int tmp;
        unsigned int utmp;
        X11::XQueryPointer(display, w, &wtmp, &child, &tmp, &tmp, &tmp, &tmp, &utmp);
        return FindAppWindow(display, child);
    }
#endif
}

#if !PLATFORM_SDL
class LinuxDropFilesData : public IGuiData
{
public:
    Array<String> Files;

    Type GetType() const override
    {
        return Type::Files;
    }
    String GetAsText() const override
    {
        return String::Empty;
    }
    void GetAsFiles(Array<String>* files) const override
    {
        files->Add(Files);
    }
};

class LinuxDropTextData : public IGuiData
{
public:
    StringView Text;

    Type GetType() const override
    {
        return Type::Text;
    }
    String GetAsText() const override
    {
        return String(Text);
    }
    void GetAsFiles(Array<String>* files) const override
    {
    }
};

DragDropEffect Window::DoDragDrop(const StringView& data)
{
    if (CommandLine::Options.Headless)
        return DragDropEffect::None;
	auto cursorWrong = X11::XCreateFontCursor(xDisplay, 54);
	auto cursorTransient = X11::XCreateFontCursor(xDisplay, 24);
	auto cursorGood = X11::XCreateFontCursor(xDisplay, 4);
    Array<X11::Atom, FixedAllocation<3>> formats;
    formats.Add(X11::XInternAtom(xDisplay, "text/plain", 0));
    formats.Add(X11::XInternAtom(xDisplay, "TEXT", 0));
    formats.Add((X11::Atom)31);
    StringAnsi dataAnsi(data);
    LinuxDropTextData dropData;
    dropData.Text = data;
#if !PLATFORM_SDL
    unsigned long mainWindow = _window;
#else
    unsigned long mainWindow = (unsigned long)_handle;
#endif

    // Begin dragging
	auto screen = X11::XDefaultScreen(xDisplay);
	auto rootWindow = X11::XRootWindow(xDisplay, screen);
    if (X11::XGrabPointer(xDisplay, mainWindow, 1, Button1MotionMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, rootWindow, cursorWrong, CurrentTime) != GrabSuccess)
	    return DragDropEffect::None;
    X11::XSetSelectionOwner(xDisplay, xAtomXdndSelection, mainWindow, CurrentTime);

    // Process events
    X11::XEvent event;
    enum Status
    {
        Unaware,
        Unreceptive,
        CanDrop,
    };
    int status = Unaware, previousVersion = -1;
    X11::Window previousWindow = 0;
    DragDropEffect result = DragDropEffect::None;
    float lastDraw = Platform::GetTimeSeconds();
    float startTime = lastDraw;
    while (true)
    {
        X11::XNextEvent(xDisplay, &event);

        if (event.type == SelectionClear)
            break;
        if (event.type == SelectionRequest)
        {
            // Extract the relavent data
            X11::Window owner = event.xselectionrequest.owner;
            X11::Atom selection = event.xselectionrequest.selection;
            X11::Atom target = event.xselectionrequest.target;
            X11::Atom property = event.xselectionrequest.property;
            X11::Window requestor = event.xselectionrequest.requestor;
            X11::Time timestamp = event.xselectionrequest.time;
            X11::Display* disp = event.xselection.display;
            X11::XEvent s;
            s.xselection.type = SelectionNotify;
            s.xselection.requestor = requestor;
            s.xselection.selection = selection;
            s.xselection.target = target;
            s.xselection.property = 0;
            s.xselection.time = timestamp;
            if (target == X11::XInternAtom(disp, "TARGETS", 0))
            {
                Array<X11::Atom> targets;
                targets.Add(target);
                targets.Add(X11::XInternAtom(disp, "MULTIPLE", 0));
                targets.Add(formats.Get(), formats.Count());
                X11::XChangeProperty(disp, requestor, property, (X11::Atom)4, 32, PropModeReplace, (unsigned char*)targets.Get(), targets.Count());
                s.xselection.property = property;
            }
            else if (formats.Contains(target))
            {
                s.xselection.property = property;
                X11::XChangeProperty(disp, requestor, property, target, 8, PropModeReplace, reinterpret_cast<const unsigned char*>(dataAnsi.Get()), dataAnsi.Length());
            }
            X11::XSendEvent(event.xselection.display, event.xselectionrequest.requestor, 1, 0, &s);
        }
        else if (event.type == MotionNotify)
        {
            // Find window under mouse
            auto window = Impl::FindAppWindow(xDisplay, rootWindow);
			int fmt, version = -1;
			X11::Atom atmp;
			unsigned long nitems, bytesLeft;
			unsigned char* data = nullptr;
            if (window == previousWindow)
                version = previousVersion;
            else if(window == 0)
                ;
            else if (X11::XGetWindowProperty(xDisplay, window, xAtomXdndAware, 0, 2, 0, AnyPropertyType, &atmp, &fmt, &nitems, &bytesLeft, &data) != Success)
                continue;
            else if (data == 0)
                continue;
            else if (fmt != 32)
                continue;
            else if (nitems != 1)
                continue;
            else
				version = data[0];
            if (status == Unaware && version != -1)
            	status = Unreceptive;
            else if(version == -1)
            	status = Unaware;
            xDndPos = Float2((float)event.xmotion.x_root, (float)event.xmotion.y_root);

            // Update mouse grab
            if (status == Unaware)
                X11::XChangeActivePointerGrab(xDisplay, Button1MotionMask | ButtonReleaseMask, cursorWrong, CurrentTime);
            else if(status == Unreceptive)
                X11::XChangeActivePointerGrab(xDisplay, Button1MotionMask | ButtonReleaseMask, cursorTransient, CurrentTime);
            else
                X11::XChangeActivePointerGrab(xDisplay, Button1MotionMask | ButtonReleaseMask, cursorGood, CurrentTime);

            if (window != previousWindow && previousVersion != -1)
            {
                // Send drag left event
                auto ww = WindowsManager::GetByNativePtr((void*)previousWindow);
                if (ww)
                {
                    ww->_dragOver = false;
                    ww->OnDragLeave();
                }
                else
                {
                    X11::XClientMessageEvent m;
                    memset(&m, 0, sizeof(m));
                    m.type = ClientMessage;
                    m.display = event.xclient.display;
                    m.window = previousWindow;
                    m.message_type = xAtomXdndLeave;
                    m.format = 32;
                    m.data.l[0] = mainWindow;
                    m.data.l[1] = 0;
                    m.data.l[2] = 0;
                    m.data.l[3] = 0;
                    m.data.l[4] = 0;
                    X11::XSendEvent(xDisplay, previousWindow, 0, NoEventMask, (X11::XEvent*)&m);
                    X11::XFlush(xDisplay);
                }
            }

            if (window != previousWindow && version != -1)
            {
                // Send drag enter event
                auto ww = WindowsManager::GetByNativePtr((void*)window);
                if (ww)
                {
                    xDndPos = ww->ScreenToClient(Platform::GetMousePosition());
                    xDndResult = DragDropEffect::None;
                    ww->OnDragEnter(&dropData, xDndPos, xDndResult);
                }
                else
                {
                    X11::XClientMessageEvent m;
                    memset(&m, 0, sizeof(m));
                    m.type = ClientMessage;
                    m.display = event.xclient.display;
                    m.window = window;
                    m.message_type = xAtomXdndEnter;
                    m.format = 32;
                    m.data.l[0] = mainWindow;
                    m.data.l[1] = Math::Min(5, version) << 24  | (formats.Count() > 3);
                    m.data.l[2] = formats.Count() > 0 ? formats[0] : 0;
                    m.data.l[3] = formats.Count() > 1 ? formats[1] : 0;
                    m.data.l[4] = formats.Count() > 2 ? formats[2] : 0;
                    X11::XSendEvent(xDisplay, window, 0, NoEventMask, (X11::XEvent*)&m);
                    X11::XFlush(xDisplay);
                }
            }

            if (version != -1)
            {
                // Send position event
                auto ww = WindowsManager::GetByNativePtr((void*)window);
                if (ww)
                {
                    xDndPos = ww->ScreenToClient(Platform::GetMousePosition());
                    ww->_dragOver = true;
                    xDndResult = DragDropEffect::None;
                    ww->OnDragOver(&dropData, xDndPos, xDndResult);
                    status = CanDrop;
                }
                else
                {
                    int x, y, tmp;
                    unsigned int utmp;
                    X11::Window wtmp;
                    X11::XQueryPointer(xDisplay, window, &wtmp, &wtmp, &tmp, &tmp, &x, &y, &utmp);
                    X11::XClientMessageEvent m;
                    memset(&m, 0, sizeof(m));
                    m.type = ClientMessage;
                    m.display = event.xclient.display;
                    m.window = window;
                    m.message_type = xAtomXdndPosition;
                    m.format = 32;
                    m.data.l[0] = mainWindow;
                    m.data.l[1] = 0;
                    m.data.l[2] = (x << 16) | y;
                    m.data.l[3] = CurrentTime;
                    m.data.l[4] = xAtomXdndActionCopy;
                    X11::XSendEvent(xDisplay, window, 0, NoEventMask, (X11::XEvent*)&m);
                    X11::XFlush(xDisplay);
                }
            }

            previousWindow = window;
            previousVersion = version;
        }
        else if (event.type == ClientMessage && event.xclient.message_type == xAtomXdndStatus)
        {
            if ((event.xclient.data.l[1]&1) && status != Unaware)
                status = CanDrop;
            if (!(event.xclient.data.l[1]&1) && status != Unaware)
                status = Unreceptive;
        }
        else if (event.type == ButtonRelease && event.xbutton.button == Button1)
        {
            if (status == CanDrop)
            {
                // Send drop event
                auto ww = WindowsManager::GetByNativePtr((void*)previousWindow);
                if (ww)
                {
                    xDndPos = ww->ScreenToClient(Platform::GetMousePosition());
                    xDndResult = DragDropEffect::None;
                    ww->OnDragDrop(&dropData, xDndPos, xDndResult);
                    ww->Focus();
					result = xDndResult;
                }
                else
                {
                    X11::XClientMessageEvent m;
                    memset(&m, 0, sizeof(m));
                    m.type = ClientMessage;
                    m.display = event.xclient.display;
                    m.window = previousWindow;
                    m.message_type = xAtomXdndDrop;
                    m.format = 32;
                    m.data.l[0] = mainWindow;
                    m.data.l[1] = 0;
                    m.data.l[2] = CurrentTime;
                    m.data.l[3] = 0;
                    m.data.l[4] = 0;
                    X11::XSendEvent(xDisplay, previousWindow, 0, NoEventMask, (X11::XEvent*)&m);
                    X11::XFlush(xDisplay);
                	result = DragDropEffect::Copy;
                }
            }
            break;
        }

        // Redraw
        const float time = Platform::GetTimeSeconds();
        if (time - lastDraw >= 1.0f / 20.0f)
        {
            lastDraw = time;
            Engine::OnDraw();
        }

        // Prevent dead-loop
        if (time - startTime >= 10.0f)
        {
            break;
        }
    }

    // Drag end
    if (previousWindow != 0 && previousVersion != -1)
    {
        // Send drag left event
        auto ww = WindowsManager::GetByNativePtr((void*)previousWindow);
        if (ww)
        {
            ww->_dragOver = false;
            ww->OnDragLeave();
        }
        else
        {
            X11::XClientMessageEvent m;
            memset(&m, 0, sizeof(m));
            m.type = ClientMessage;
            m.display = event.xclient.display;
            m.window = previousWindow;
            m.message_type = xAtomXdndLeave;
            m.format = 32;
            m.data.l[0] = mainWindow;
            m.data.l[1] = 0;
            m.data.l[2] = 0;
            m.data.l[3] = 0;
            m.data.l[4] = 0;
            X11::XSendEvent(xDisplay, previousWindow, 0, NoEventMask, (X11::XEvent*)&m);
            X11::XFlush(xDisplay);
        }
    }

    // End grabbing
    X11::XChangeActivePointerGrab(xDisplay, Button1MotionMask | ButtonReleaseMask, 0, CurrentTime);
    XUngrabPointer(xDisplay, CurrentTime);

	return result;
}

void LinuxClipboard::Clear()
{
    SetText(StringView::Empty);
}

void LinuxClipboard::SetText(const StringView& text)
{
    if (CommandLine::Options.Headless)
        return;
    auto mainWindow = Engine::MainWindow;
    if (!mainWindow)
        return;
    X11::Window window = (X11::Window)mainWindow->GetNativePtr();

    Impl::ClipboardText.Set(text.Get(), text.Length());
    X11::XSetSelectionOwner(xDisplay, xAtomClipboard, window, CurrentTime); // CLIPBOARD
    X11::XSetSelectionOwner(xDisplay, (X11::Atom)1, window, CurrentTime); // XA_PRIMARY
}

void LinuxClipboard::SetRawData(const Span<byte>& data)
{
}

void LinuxClipboard::SetFiles(const Array<String>& files)
{
}

String LinuxClipboard::GetText()
{
    if (CommandLine::Options.Headless)
        return String::Empty;
    String result;
    auto mainWindow = Engine::MainWindow;
    if (!mainWindow)
        return result;
    X11::Window window = (X11::Window)mainWindow->GetNativePtr();

    X11::Atom UTF8 = X11::XInternAtom(xDisplay, "UTF8_STRING", 1);
    Impl::ClipboardGetText(result, xAtomClipboard, UTF8, window);
    if (result.HasChars())
        return result;
    Impl::ClipboardGetText(result, xAtomClipboard, (X11::Atom)31, window);
    if (result.HasChars())
        return result;
    Impl::ClipboardGetText(result, (X11::Atom)1, UTF8, window);
    if (result.HasChars())
        return result;
    Impl::ClipboardGetText(result, (X11::Atom)1, (X11::Atom)31, window);
    return result;
}

Array<byte> LinuxClipboard::GetRawData()
{
    return Array<byte>();
}

Array<String> LinuxClipboard::GetFiles()
{
    return Array<String>();
}

#endif

void* LinuxPlatform::GetXDisplay()
{
    return xDisplay;
}

bool LinuxPlatform::CreateMutex(const Char* name)
{
    char buffer[250];
    sprintf(buffer, "/var/run/%s.pid", StringAsANSI<>(name).Get());
    const int pidFile = open(buffer, O_CREAT | O_RDWR, 0666);
    const int rc = flock(pidFile, LOCK_EX | LOCK_NB);
    if (rc)
    {
        if (EWOULDBLOCK == errno)
            return true;
    }

    return false;
}

const String& LinuxPlatform::GetHomeDirectory()
{
    return HomeDir;
}

bool LinuxPlatform::Is64BitPlatform()
{
#ifdef PLATFORM_64BITS
    return true;
#else
#error "Implement LinuxPlatform::Is64BitPlatform for 32-bit builds."
#endif
}

CPUInfo LinuxPlatform::GetCPUInfo()
{
    return UnixCpu;
}

int32 LinuxPlatform::GetCacheLineSize()
{
    return UnixCpu.CacheLineSize;
}

MemoryStats LinuxPlatform::GetMemoryStats()
{
    // Get memory usage
    const uint64 pageSize = getpagesize();
    const uint64 totalPages = get_phys_pages();
    const uint64 availablePages = get_avphys_pages();

    // Fill result data
    MemoryStats result;
    result.TotalPhysicalMemory = totalPages * pageSize;
    result.UsedPhysicalMemory = (totalPages - availablePages) * pageSize;
    result.TotalVirtualMemory = result.TotalPhysicalMemory;
    result.UsedVirtualMemory = result.UsedPhysicalMemory;

    return result;
}

ProcessMemoryStats LinuxPlatform::GetProcessMemoryStats()
{
    // Get memory usage
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);

    // Fill result data
    ProcessMemoryStats result;
    result.UsedPhysicalMemory = usage.ru_maxrss;
    result.UsedVirtualMemory = result.UsedPhysicalMemory;

    return result;
}

void LinuxPlatform::SetThreadPriority(ThreadPriority priority)
{
    // TODO: impl this
}

void LinuxPlatform::SetThreadAffinityMask(uint64 affinityMask)
{
    // TODO: impl this
}

void LinuxPlatform::Sleep(int32 milliseconds)
{
    usleep(milliseconds * 1000);
}

double LinuxPlatform::GetTimeSeconds()
{
    struct timespec ts;
    clock_gettime(ClockSource, &ts);
    return static_cast<double>(ts.tv_sec) + static_cast<double>(ts.tv_nsec) / 1e9;
}

uint64 LinuxPlatform::GetTimeCycles()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return static_cast<uint64>(static_cast<uint64>(ts.tv_sec) * 1000000ULL + static_cast<uint64>(ts.tv_nsec) / 1000ULL);
}

void LinuxPlatform::GetSystemTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond)
{
      // Query for calendar time
    struct timeval time;
    gettimeofday(&time, nullptr);

    // Convert to local time
    struct tm localTime;
    localtime_r(&time.tv_sec, &localTime);

    // Extract time
    year = localTime.tm_year + 1900;
    month = localTime.tm_mon + 1;
    dayOfWeek = localTime.tm_wday;
    day = localTime.tm_mday;
    hour = localTime.tm_hour;
    minute = localTime.tm_min;
    second = localTime.tm_sec;
    millisecond = time.tv_usec / 1000;
}

void LinuxPlatform::GetUTCTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond)
{
    // Get the calendar time
    struct timeval time;
    gettimeofday(&time, nullptr);

    // Convert to UTC time
    struct tm localTime;
    gmtime_r(&time.tv_sec, &localTime);

    // Extract time
    year = localTime.tm_year + 1900;
    month = localTime.tm_mon + 1;
    dayOfWeek = localTime.tm_wday;
    day = localTime.tm_mday;
    hour = localTime.tm_hour;
    minute = localTime.tm_min;
    second = localTime.tm_sec;
    millisecond = time.tv_usec / 1000;
}

#if !BUILD_RELEASE

bool LinuxPlatform::IsDebuggerPresent()
{
	static int32 CachedState = -1;
	if (CachedState == -1)
	{
		CachedState = 0;

    	// Reference: https://stackoverflow.com/questions/3596781/how-to-detect-if-the-current-process-is-being-run-by-gdb
		char buf[4096];
		const int status_fd = open("/proc/self/status", O_RDONLY);
		if (status_fd == -1)
			return false;
		const ssize_t num_read = read(status_fd, buf, sizeof(buf) - 1);
		close(status_fd);
		if (num_read <= 0)
			return false;
		buf[num_read] = '\0';
		constexpr char tracerPidString[] = "TracerPid:";
		const auto tracer_pid_ptr = strstr(buf, tracerPidString);
		if (!tracer_pid_ptr)
			return false;
		for (const char* characterPtr = tracer_pid_ptr + sizeof(tracerPidString) - 1; characterPtr <= buf + num_read; ++characterPtr)
		{
			if (StringUtils::IsWhitespace(*characterPtr))
				continue;
			else
			{
				if (StringUtils::IsDigit(*characterPtr) && *characterPtr != '0')
					CachedState = 1;
				return CachedState == 1;
			}
		}
	}
	return CachedState == 1;
}

#endif

bool LinuxPlatform::Init()
{
    if (PlatformBase::Init())
        return true;
    char fileNameBuffer[1024];

    // Init timing
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1)
    {
        ClockSource = CLOCK_REALTIME;
    }
    else
    {
        ClockSource = CLOCK_MONOTONIC;
    }

    // Set info about the CPU
    cpu_set_t cpus;
    CPU_ZERO(&cpus);
    if (sched_getaffinity(0, sizeof(cpus), &cpus) == 0)
    {
        int32 numberOfCores = 0;
        struct CpuInfo
        {
            int32 Core;
            int32 Package;
        } cpusInfo[CPU_SETSIZE];
        Platform::MemoryClear(cpusInfo, sizeof(cpusInfo));
        int32 maxCoreId = 0;
        int32 maxPackageId = 0;
        int32 cpuCountAvailable = 0;

        for (int32 cpuIdx = 0; cpuIdx < CPU_SETSIZE; cpuIdx++)
        {
            if (CPU_ISSET(cpuIdx, &cpus))
            {
                cpuCountAvailable++;

                sprintf(fileNameBuffer, "/sys/devices/system/cpu/cpu%d/topology/core_id", cpuIdx);
                if (FILE* coreIdFile = fopen(fileNameBuffer, "r"))
                {
                    if (fscanf(coreIdFile, "%d", &cpusInfo[cpuIdx].Core) != 1)
                    {
                        cpusInfo[cpuIdx].Core = 0;
                    }
                    fclose(coreIdFile);
                }

                sprintf(fileNameBuffer, "/sys/devices/system/cpu/cpu%d/topology/physical_package_id", cpuIdx);
                if (FILE* packageIdFile = fopen(fileNameBuffer, "r"))
                {
                    if (fscanf(packageIdFile, "%d", &cpusInfo[cpuIdx].Package) != 1 || cpusInfo[cpuIdx].Package < 0)
                    {
                        cpusInfo[cpuIdx].Package = cpusInfo[cpuIdx].Core;
                    }
                    fclose(packageIdFile);
                }

                maxCoreId = Math::Max(maxCoreId, cpusInfo[cpuIdx].Core);
                maxPackageId = Math::Max(maxPackageId, cpusInfo[cpuIdx].Package);
            }
        }

        int32 coresCount = maxCoreId + 1;
        int32 packagesCount = maxPackageId + 1;
        int32 pairsCount = packagesCount * coresCount;

        if (coresCount * 2 < cpuCountAvailable)
        {
            numberOfCores = cpuCountAvailable;
        }
        else
        {
            byte* pairs = (byte*)Allocator::Allocate(pairsCount);
            Platform::MemoryClear(pairs, pairsCount * sizeof(unsigned char));

            for (int32 cpuIdx = 0; cpuIdx < CPU_SETSIZE; cpuIdx++)
            {
                if (CPU_ISSET(cpuIdx, &cpus))
                {
                    pairs[cpusInfo[cpuIdx].Package * coresCount + cpusInfo[cpuIdx].Core] = 1;
                }
            }

            for (int32 i = 0; i < pairsCount; i++)
            {
                numberOfCores += pairs[i];
            }

            Allocator::Free(pairs);
        }

        UnixCpu.ProcessorPackageCount = packagesCount;
        UnixCpu.ProcessorCoreCount = Math::Max(numberOfCores, 1);
        UnixCpu.LogicalProcessorCount = CPU_COUNT(&cpus);
    }
    else
    {
        UnixCpu.ProcessorPackageCount = 1;
        UnixCpu.ProcessorCoreCount = 1;
        UnixCpu.LogicalProcessorCount = 1;
    }

    // Get cache sizes
    UnixCpu.L1CacheSize = 0;
    UnixCpu.L2CacheSize = 0;
    UnixCpu.L3CacheSize = 0;
    for (int32 cacheLevel = 1; cacheLevel <= 3; cacheLevel++)
    {
        sprintf(fileNameBuffer, "/sys/devices/system/cpu/cpu0/cache/index%d/size", cacheLevel);
        if (FILE* file = fopen(fileNameBuffer, "r"))
        {
            int32 count = fread(fileNameBuffer, 1, sizeof(fileNameBuffer) - 1, file);
            if (count == 0)
                break;
            if (fileNameBuffer[count - 1] == '\n')
                fileNameBuffer[count - 1] = 0;
            else
                fileNameBuffer[count] = 0;
            uintmax_t res;
            parse_size(fileNameBuffer, &res, nullptr);
            switch (cacheLevel)
            {
            case 1:
                UnixCpu.L1CacheSize = res;
                break;
            case 2:
                UnixCpu.L2CacheSize = res;
                break;
            case 3:
                UnixCpu.L3CacheSize = res;
                break;
            }
            fclose(file);
        }
    }

    // Get page size
    UnixCpu.PageSize = sysconf(_SC_PAGESIZE);

    // Get clock speed
    UnixCpu.ClockSpeed = GetClockFrequency();

    // Get cache line size
    UnixCpu.CacheLineSize = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    ASSERT(UnixCpu.CacheLineSize && Math::IsPowerOfTwo(UnixCpu.CacheLineSize));

    // Get user name string
    char buffer[UNIX_APP_BUFF_SIZE];
    getlogin_r(buffer, UNIX_APP_BUFF_SIZE);
    OnPlatformUserAdd(New<User>(String(buffer)));

    UnixGetMacAddress(MacAddress);

    // Generate unique device ID
    {
        DeviceId = Guid::Empty;

        // A - Computer Name and User Name
        uint32 hash = GetHash(Platform::GetComputerName());
        CombineHash(hash, GetHash(Platform::GetUserName()));
        DeviceId.A = hash;

        // B - MAC address
        hash = MacAddress[0];
        for (uint32 i = 0; i < 6; i++)
            CombineHash(hash, MacAddress[i]);
        DeviceId.B = hash;

        // C - memory
        DeviceId.C = (uint32)Platform::GetMemoryStats().TotalPhysicalMemory;

        // D - cpuid
        DeviceId.D = (uint32)UnixCpu.ClockSpeed * UnixCpu.LogicalProcessorCount * UnixCpu.ProcessorCoreCount * UnixCpu.CacheLineSize;
    }

    // Get user locale string
    setlocale(LC_ALL, "");
    const char* locale = setlocale(LC_CTYPE, NULL);
    UserLocale = String(locale);
    if (UserLocale.FindLast('.') != -1)
        UserLocale = UserLocale.Left(UserLocale.Find('.'));
    UserLocale.Replace('_', '-');
    if (UserLocale == TEXT("C"))
        UserLocale = TEXT("en");

    // Get computer name string
    gethostname(buffer, UNIX_APP_BUFF_SIZE);
    ComputerName = String(buffer);

    // Get home dir
    struct passwd pw;
    struct passwd* result = NULL;
    if (getpwuid_r(getuid(), &pw, buffer, UNIX_APP_BUFF_SIZE, &result) == 0 && result)
    {
        HomeDir = pw.pw_dir;
    }
    if (HomeDir.IsEmpty())
        HomeDir = TEXT("/");

	Platform::MemoryClear(Cursors, sizeof(Cursors));
	Platform::MemoryClear(CursorsImg, sizeof(CursorsImg));


    // Skip setup if running in headless mode (X11 might not be available on servers)
    if (CommandLine::Options.Headless)
        return false;
#if PLATFORM_SDL
    xDisplay = X11::XOpenDisplay(nullptr);
#endif
    
#if !PLATFORM_SDL
	X11::XInitThreads();

	xDisplay = X11::XOpenDisplay(nullptr);
	X11::XSetErrorHandler(X11ErrorHandler);

	if (X11::XSupportsLocale())
	{
		X11::XSetLocaleModifiers("@im=none");
		IM = X11::XOpenIM(xDisplay, nullptr, nullptr, nullptr);
		IC = X11::XCreateIC(IM, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, nullptr);
	}

	xAtomDeleteWindow = X11::XInternAtom(xDisplay, "WM_DELETE_WINDOW", 0);
	xAtomXdndEnter = X11::XInternAtom(xDisplay, "XdndEnter", 0);
	xAtomXdndPosition = X11::XInternAtom(xDisplay, "XdndPosition", 0);
	xAtomXdndLeave = X11::XInternAtom(xDisplay, "XdndLeave", 0);
	xAtomXdndDrop = X11::XInternAtom(xDisplay, "XdndDrop", 0);
    xAtomXdndActionCopy = X11::XInternAtom(xDisplay, "XdndActionCopy", 0);
    xAtomXdndStatus = X11::XInternAtom(xDisplay, "XdndStatus", 0);
    xAtomXdndSelection = X11::XInternAtom(xDisplay, "XdndSelection", 0);
    xAtomXdndFinished = X11::XInternAtom(xDisplay, "XdndFinished", 0);
    xAtomXdndAware = X11::XInternAtom(xDisplay, "XdndAware", 0);
	xAtomWmStateHidden = X11::XInternAtom(xDisplay, "_NET_WM_STATE_HIDDEN", 0);
	xAtomWmStateMaxHorz = X11::XInternAtom(xDisplay, "_NET_WM_STATE_MAXIMIZED_HORZ", 0);
	xAtomWmStateMaxVert = X11::XInternAtom(xDisplay, "_NET_WM_STATE_MAXIMIZED_VERT", 0);
	xAtomWmWindowOpacity = X11::XInternAtom(xDisplay, "_NET_WM_WINDOW_OPACITY", 0);
	xAtomWmName = X11::XInternAtom(xDisplay, "_NET_WM_NAME", 0);
	xAtomClipboard = X11::XInternAtom(xDisplay, "CLIPBOARD", 0);

	X11::XrmInitialize();
	SystemDpi = CalculateDpi();

	int cursorSize = X11::XcursorGetDefaultSize(xDisplay);
	const char* cursorTheme = X11::XcursorGetTheme(xDisplay);
	if (!cursorTheme)
		cursorTheme = "default";

	// Load default cursors
	for (int32 i = 0; i < (int32)CursorType::MAX; i++)
	{
		const char* cursorFile = nullptr;
		switch ((CursorType)i)
		{
		case CursorType::Default:
			cursorFile = "left_ptr";
			break;
		case CursorType::Cross:
			cursorFile = "cross";
			break;
		case CursorType::Hand:
			cursorFile = "hand2";
			break;
		case CursorType::Help:
			cursorFile = "question_arrow";
			break;
		case CursorType::IBeam:
			cursorFile = "xterm";
			break;
		case CursorType::No:
			cursorFile = "X_cursor";
			break;
		case CursorType::Wait:
			cursorFile = "watch";
			break;
		case CursorType::SizeAll:
			cursorFile = "tcross";
			break;
		case CursorType::SizeNESW:
			cursorFile = "size_bdiag";
			break;
		case CursorType::SizeNS:
			cursorFile = "sb_V_double_arrow";
			break;
		case CursorType::SizeNWSE:
			cursorFile = "size_fdiag";
			break;
		case CursorType::SizeWE:
			cursorFile = "sb_h_double_arrow";
			break;
		default:
			break;
		}
		if (cursorFile == nullptr)
			continue;

		CursorsImg[i] = X11::XcursorLibraryLoadImage(cursorFile, cursorTheme, cursorSize);
		if (CursorsImg[i])
			Cursors[i] = X11::XcursorImageLoadCursor(xDisplay, CursorsImg[i]);
	}

	// Create empty cursor
	{
		char data[1];
		memset(data, 0, sizeof(data));
		X11::Pixmap pixmap = X11::XCreateBitmapFromData(xDisplay, XDefaultRootWindow(xDisplay), data, 1, 1);
		X11::XColor color;
		color.red = color.green = color.blue = 0;
		Cursors[(int32)CursorType::Hidden] = X11::XCreatePixmapCursor(xDisplay, pixmap, pixmap, &color, &color, 0, 0);
		X11::XFreePixmap(xDisplay, pixmap);
	}

	// Initialize "X11 keyname" -> "X11 keycode" map
	char name[XkbKeyNameLength + 1];
	X11::XkbDescPtr desc = X11::XkbGetMap(xDisplay, 0, XkbUseCoreKbd);
	X11::XkbGetNames(xDisplay, XkbKeyNamesMask, desc);
	for (uint32 keyCode = desc->min_key_code; keyCode <= desc->max_key_code; keyCode++)
	{
		memcpy(name, desc->names->keys[keyCode].name, XkbKeyNameLength);
		name[XkbKeyNameLength] = '\0';
		KeyNameMap[StringAnsi(name)] = keyCode;
	}

	// Initialize "X11 keycode" -> "Flax KeyboardKeys" map
	KeyCodeMap.Resize(desc->max_key_code + 1);
    Platform::MemoryClear(KeyCodeMap.Get(), KeyCodeMap.Count() * sizeof(KeyboardKeys));
	XkbFreeNames(desc, XkbKeyNamesMask, 1);
	X11::XkbFreeKeyboard(desc, 0, 1);
	for (int32 keyIdx = (int32)KeyboardKeys::None; keyIdx < MAX_uint8; keyIdx++)
	{
		KeyboardKeys key = (KeyboardKeys)keyIdx;
		const char* keyNameCStr = ButtonCodeToKeyName(key);
		if (keyNameCStr != nullptr)
		{
			StringAnsi keyName(keyNameCStr);
			X11::KeyCode keyCode;
			if (KeyNameMap.TryGet(keyName, keyCode))
			{
				KeyCodeMap[keyCode] = key;
			}
		}
	}

	//Patch in numpad enter to normal enter, just like on windows.
	KeyCodeMap[104] = KeyboardKeys::Return;

    Input::Mouse = Impl::Mouse = New<LinuxMouse>();
    Input::Keyboard = Impl::Keyboard = New<LinuxKeyboard>();
	LinuxInput::Init();
#endif
    return false;
}

void LinuxPlatform::BeforeRun()
{
}

void LinuxPlatform::Tick()
{
#if !PLATFORM_SDL
	UnixPlatform::Tick();

	LinuxInput::UpdateState();

    if (!xDisplay)
        return;

	// Check to see if any messages are waiting in the queue
	while (X11::XPending(xDisplay) > 0)
	{
		X11::XEvent event;
		X11::XNextEvent(xDisplay, &event);
		if (X11::XFilterEvent(&event, 0))
			continue;

        // External event handling
		xEventReceived(&event);

		Window* window;
		switch (event.type)
		{
			case ClientMessage:
				// User requested the window to close
				if ((X11::Atom)event.xclient.data.l[0] == xAtomDeleteWindow)
				{
					window = WindowsManager::GetByNativePtr((void*)event.xclient.window);
					if (window)
					{
						window->Close(ClosingReason::User);
					}
				}
                else if ((uint32)event.xclient.message_type == (uint32)xAtomXdndEnter)
                {
                    // Drag&drop enter
                    X11::Window source = event.xclient.data.l[0];
                    xDnDVersion = (int32)(event.xclient.data.l[1] >> 24);
                    const char* targetTypeFiles = "text/uri-list";
                    if (event.xclient.data.l[1] & 1)
                    {
						Property p = Impl::ReadProperty(xDisplay, source, XInternAtom(xDisplay, "XdndTypeList", 0));
						xDnDRequested = Impl::SelectTargetFromList(xDisplay, targetTypeFiles, (X11::Atom*)p.data, p.nitems);
                        X11::XFree(p.data);
                    }
                    else
                    {
                        xDnDRequested = Impl::SelectTargetFromAtoms(xDisplay, targetTypeFiles, event.xclient.data.l[2], event.xclient.data.l[3], event.xclient.data.l[4]);
                    }
                }
                else if ((uint32)event.xclient.message_type == (uint32)xAtomXdndPosition)
                {
                    // Drag&drop move
                    X11::XClientMessageEvent m;
                    memset(&m, 0, sizeof(m));
                    m.type = ClientMessage;
                    m.display = event.xclient.display;
                    m.window = event.xclient.data.l[0];
                    m.message_type = xAtomXdndStatus;
                    m.format = 32;
                    m.data.l[0] = event.xany.window;
                    m.data.l[1] = (xDnDRequested != 0);
                    m.data.l[2] = 0;
                    m.data.l[3] = 0;
                    m.data.l[4] = xAtomXdndActionCopy;
                    X11::XSendEvent(xDisplay, event.xclient.data.l[0], 0, NoEventMask, (X11::XEvent*)&m);
                    X11::XFlush(xDisplay);
                    xDndPos = Float2((float)(event.xclient.data.l[2]  >> 16), (float)(event.xclient.data.l[2] & 0xffff));
                    window = WindowsManager::GetByNativePtr((void*)event.xany.window);
                    if (window)
                    {
                        LinuxDropFilesData dropData;
                        xDndResult = DragDropEffect::None;
                        if (window->_dragOver)
                        {
                            window->OnDragOver(&dropData, xDndPos, xDndResult);
                        }
                        else
                        {
                            window->_dragOver = true;
                            window->OnDragEnter(&dropData, xDndPos, xDndResult);
                        }
                    }
                }
                else if ((uint32)event.xclient.message_type == (uint32)xAtomXdndLeave)
                {
                    window = WindowsManager::GetByNativePtr((void*)event.xany.window);
                    if (window && window->_dragOver)
                    {
                        window->_dragOver = false;
                        window->OnDragLeave();
                    }
                }
                else if ((uint32)event.xclient.message_type == (uint32)xAtomXdndDrop)
                {
                    auto w = event.xany.window;
                    if (xDnDRequested != 0)
                    {
                        xDndSourceWindow = event.xclient.data.l[0];
                        auto primary = XInternAtom(xDisplay, "PRIMARY", 0);
                        if (xDnDVersion >= 1)
                            XConvertSelection(xDisplay, xAtomXdndSelection, xDnDRequested, primary, w, event.xclient.data.l[2]);
                        else
                            XConvertSelection(xDisplay, xAtomXdndSelection, xDnDRequested, primary, w, CurrentTime);
                    }
                    else
                    {
                        X11::XClientMessageEvent m;
                        memset(&m, 0, sizeof(m));
                        m.type = ClientMessage;
                        m.display = event.xclient.display;
                        m.window = event.xclient.data.l[0];
                        m.message_type = xAtomXdndFinished;
                        m.format = 32;
                        m.data.l[0] = w;
                        m.data.l[1] = 0;
                        m.data.l[2] = 0;
                        X11::XSendEvent(xDisplay, event.xclient.data.l[0], 0, NoEventMask, (X11::XEvent*)&m);
                    }
                }
			break;
			case MapNotify:
				// Auto-focus shown windows
				window = WindowsManager::GetByNativePtr((void*)event.xmap.window);
				if (window && window->_focusOnMapped)
				{
					window->_focusOnMapped = false;
					window->Focus();
				}
			break;
			case FocusIn:
				// Update input context focus
				X11::XSetICFocus(IC);
				window = WindowsManager::GetByNativePtr((void*)event.xfocus.window);
				if (window && MouseTrackingWindow == nullptr)
				{
					window->OnGotFocus();
				}
			break;
			case FocusOut:
				// Update input context focus
				X11::XUnsetICFocus(IC);
				window = WindowsManager::GetByNativePtr((void*)event.xfocus.window);
				if (window && MouseTrackingWindow == nullptr)
				{
					window->OnLostFocus();
				}
			break;
			case ConfigureNotify:
				// Handle window resizing
				window = WindowsManager::GetByNativePtr((void*)event.xclient.window);
				if (window)
				{
					window->OnConfigureNotify(&event.xconfigure);
				}
			break;
			case PropertyNotify:
				// Report minimize, maximize and restore events
				if (event.xproperty.atom == xAtomWmState)
				{
					window = WindowsManager::GetByNativePtr((void*)event.xclient.window);
					if (window == nullptr)
						break;

					X11::Atom type;
					int32 format;
					unsigned long count, bytesRemaining;
					byte* data = nullptr;

					const int32 result = X11::XGetWindowProperty(xDisplay, event.xproperty.window, xAtomWmState, 0, 1024, 0, AnyPropertyType, &type, &format, &count, &bytesRemaining, &data);
					if (result == Success)
					{
						window = WindowsManager::GetByNativePtr((void*)event.xproperty.window);
						if (window == nullptr)
							continue;
						X11::Atom* atoms = (X11::Atom*)data;

						bool foundHorz = false;
						bool foundVert = false;
						for (unsigned long i = 0; i < count; i++)
						{
							if (atoms[i] == xAtomWmStateMaxHorz)
								foundHorz = true;
							if (atoms[i] == xAtomWmStateMaxVert)
								foundVert = true;

							if (foundVert && foundHorz)
							{
								if (event.xproperty.state == PropertyNewValue)
								{
									// Maximized
									window->_minimized = false;
									window->_maximized = true;
									window->CheckForWindowResize();
								}
								else
								{
									// Restored
									if (window->_maximized)
									{
										window->_maximized = false;
									}
									else if (window->_minimized)
									{
										window->_minimized = false;
									}
									window->CheckForWindowResize();
								}
							}

							if (atoms[i] == xAtomWmStateHidden)
							{
								if (event.xproperty.state == PropertyNewValue)
								{
									// Minimized
									window->_minimized = true;
									window->_maximized = false;
								}
								else
								{
									// Restored
									if (window->_maximized)
									{
										window->_maximized = false;
									}
									else if (window->_minimized)
									{
										window->_minimized = false;
									}
									window->CheckForWindowResize();
								}
							}
						}

						X11::XFree(atoms);
					}
				}
				break;
			case KeyPress:
				window = WindowsManager::GetByNativePtr((void*)event.xkey.window);
				if (window)
					window->OnKeyPress(&event.xkey);
				break;
			case KeyRelease:
				window = WindowsManager::GetByNativePtr((void*)event.xkey.window);
				if (window)
					window->OnKeyRelease(&event.xkey);
				break;
			case ButtonPress:
				window = WindowsManager::GetByNativePtr((void*)event.xbutton.window);
		        if (MouseTrackingWindow)
		            MouseTrackingWindow->OnButtonPress(&event.xbutton);
				else if (window)
					window->OnButtonPress(&event.xbutton);
				break;
			case ButtonRelease:
				window = WindowsManager::GetByNativePtr((void*)event.xbutton.window);
		        if (MouseTrackingWindow)
		            MouseTrackingWindow->OnButtonRelease(&event.xbutton);
				else if (window)
					window->OnButtonRelease(&event.xbutton);
				break;
			case MotionNotify:
				window = WindowsManager::GetByNativePtr((void*)event.xmotion.window);
		        if (MouseTrackingWindow)
		            MouseTrackingWindow->OnMotionNotify(&event.xmotion);
				else if (window)
					window->OnMotionNotify(&event.xmotion);
				break;
			case EnterNotify:
			    // nothing?
				break;
			case LeaveNotify:
				window = WindowsManager::GetByNativePtr((void*)event.xcrossing.window);
		        if (MouseTrackingWindow)
		            MouseTrackingWindow->OnLeaveNotify(&event.xcrossing);
				if (window)
					window->OnLeaveNotify(&event.xcrossing);
				break;
            case SelectionRequest:
            {
                if (event.xselectionrequest.selection != xAtomClipboard)
                    break;
                X11::Atom targets_atom = X11::XInternAtom(xDisplay, "TARGETS", 0);
                X11::Atom text_atom = X11::XInternAtom(xDisplay, "TEXT", 0);
                X11::Atom UTF8 = X11::XInternAtom(xDisplay, "UTF8_STRING", 1);
                if (UTF8 == 0)
                    UTF8 = (X11::Atom)31;
                X11::XSelectionRequestEvent* xsr = &event.xselectionrequest;
                int result = 0;
                X11::XSelectionEvent ev = { 0 };
                ev.type = SelectionNotify;
                ev.display = xsr->display;
                ev.requestor = xsr->requestor;
                ev.selection = xsr->selection;
                ev.time = xsr->time;
                ev.target = xsr->target;
                ev.property = xsr->property;
                if (ev.target == targets_atom)
                    result = X11::XChangeProperty(ev.display, ev.requestor, ev.property, (X11::Atom)4, 32, PropModeReplace, (unsigned char*)&UTF8, 1);
                else if (ev.target == (X11::Atom)31 || ev.target == text_atom) 
                    result = X11::XChangeProperty(ev.display, ev.requestor, ev.property, (X11::Atom)31, 8, PropModeReplace, (unsigned char*)Impl::ClipboardText.Get(), Impl::ClipboardText.Length());
                else if (ev.target == UTF8)
                    result = X11::XChangeProperty(ev.display, ev.requestor, ev.property, UTF8, 8, PropModeReplace, (unsigned char*)Impl::ClipboardText.Get(), Impl::ClipboardText.Length());
                else
                    ev.property = 0;
                if ((result & 2) == 0)
                    X11::XSendEvent(xDisplay, ev.requestor, 0, 0, (X11::XEvent*)&ev);
                break;
            }
            case SelectionNotify:
                if (event.xselection.target == xDnDRequested)
                {
                    // Drag&drop
				    window = WindowsManager::GetByNativePtr((void*)event.xany.window);
                    if (window)
                    {
                        Property p = Impl::ReadProperty(xDisplay, event.xany.window, X11::XInternAtom(xDisplay, "PRIMARY", 0));
                        if (xDndResult != DragDropEffect::None)
                        {
                            LinuxDropFilesData dropData;
                            const String filesList((const char*)p.data);
                            filesList.Split('\n', dropData.Files);
                            for (auto& e : dropData.Files)
                            {
                                e.Replace(TEXT("file://"), TEXT(""));
                                e.Replace(TEXT("%20"), TEXT(" "));
                                e = e.TrimTrailing();
                            }
                            xDndResult = DragDropEffect::None;
                            window->OnDragDrop(&dropData, xDndPos, xDndResult);
                        }
                    }
                    X11::XClientMessageEvent m;
                    memset(&m, 0, sizeof(m));
                    m.type = ClientMessage;
                    m.display = xDisplay;
                    m.window = xDndSourceWindow;
                    m.message_type = xAtomXdndFinished;
                    m.format = 32;
                    m.data.l[0] = event.xany.window;
                    m.data.l[1] = 1;
                    m.data.l[2] = xAtomXdndActionCopy;
                    XSendEvent(xDisplay, xDndSourceWindow, 0, NoEventMask, (X11::XEvent*)&m);
                }
                break;
			default:
				break;
		}
	}

	//X11::XFlush(xDisplay);
#endif
}

void LinuxPlatform::BeforeExit()
{
}

void LinuxPlatform::Exit()
{
#if !PLATFORM_SDL
	for (int32 i = 0; i < (int32)CursorType::MAX; i++)
	{
		if (Cursors[i])
			X11::XFreeCursor(xDisplay, Cursors[i]);
		if (CursorsImg[i])
			X11::XcursorImageDestroy(CursorsImg[i]);
	}

	if (IC)
	{
		X11::XDestroyIC(IC);
		IC = 0;
	}

	if (IM)
	{
		X11::XCloseIM(IM);
		IM = 0;
	}

	if (xDisplay)
	{
		X11::XCloseDisplay(xDisplay);
		xDisplay = nullptr;
	}
#endif
}

int32 LinuxPlatform::GetDpi()
{
    return SystemDpi;
}

String LinuxPlatform::GetUserLocaleName()
{
    return UserLocale;
}

String LinuxPlatform::GetComputerName()
{
    return ComputerName;
}

bool LinuxPlatform::GetHasFocus()
{
	// Check if any window is focused
	ScopeLock lock(WindowsManager::WindowsLocker);
	for (auto window : WindowsManager::Windows)
	{
		if (window->IsFocused())
			return true;
	}
	return false;
}

bool LinuxPlatform::CanOpenUrl(const StringView& url)
{
    return true;
}

void LinuxPlatform::OpenUrl(const StringView& url)
{
    const StringAsANSI<> urlAnsi(*url, url.Length());
    char cmd[2048];
    sprintf(cmd, "xdg-open %s", urlAnsi.Get());
    system(cmd);
}

Float2 LinuxPlatform::GetMousePosition()
{
    if (!xDisplay)
        return Float2::Zero;
	int32 x = 0, y = 0;
	uint32 screenCount = (uint32)X11::XScreenCount(xDisplay);
	for (uint32 i = 0; i < screenCount; i++)
	{
		X11::Window outRoot, outChild;
		int32 childX, childY;
		uint32 mask;
		if (X11::XQueryPointer(xDisplay, X11::XRootWindow(xDisplay, i), &outRoot, &outChild, &x, &y, &childX, &childY, &mask))
			break;
	}
	return Float2((float)x, (float)y);
}

void LinuxPlatform::SetMousePosition(const Float2& pos)
{
    if (!xDisplay)
        return;

	int32 x = (int32)pos.X;
	int32 y = (int32)pos.Y;
	uint32 screenCount = (uint32)X11::XScreenCount(xDisplay);

	// Note assuming screens are laid out horizontally left to right
	int32 screenX = 0;
	for(uint32 i = 0; i < screenCount; i++)
	{
		X11::Window root = X11::XRootWindow(xDisplay, i);
		int32 screenXEnd = screenX + X11::XDisplayWidth(xDisplay, i);

		if (pos.X >= screenX && pos.X < screenXEnd)
		{
			X11::XWarpPointer(xDisplay, 0, root, 0, 0, 0, 0, x, y);
			X11::XFlush(xDisplay);
			return;
		}

		screenX = screenXEnd;
	}
}

Float2 LinuxPlatform::GetDesktopSize()
{
    if (!xDisplay)
        return Float2::Zero;

	int event, err;
	const bool ok = X11::XineramaQueryExtension(xDisplay, &event, &err);
	if (!ok)
		return Float2::Zero;

	int count;
	int screenIdx = 0;
	X11::XineramaScreenInfo* xsi = X11::XineramaQueryScreens(xDisplay, &count);
	if (screenIdx >= count)
		return Float2::Zero;

    // this function is used as a fallback to place a window at the center of
    // a screen so we report only one screen instead of the real desktop
	Float2 size((float)xsi[screenIdx].width, (float)xsi[screenIdx].height);
	X11::XFree(xsi);
	return size;
}

Rectangle LinuxPlatform::GetMonitorBounds(const Float2& screenPos)
{
    if (!xDisplay)
        return Rectangle::Empty;

    int event, err;
    const bool ok = X11::XineramaQueryExtension(xDisplay, &event, &err);
    if (!ok)
        return Rectangle::Empty;

    int count;
    int screenIdx = 0;
    X11::XineramaScreenInfo* xsi = X11::XineramaQueryScreens(xDisplay, &count);
    if (screenIdx >= count)
        return Rectangle::Empty;
    // find the screen for this screenPos
    for (int i = 0; i < count; i++)
    {
        if (screenPos.X >= xsi[i].x_org && screenPos.X < xsi[i].x_org+xsi[i].width
            && screenPos.Y >= xsi[i].y_org && screenPos.Y < xsi[i].y_org+xsi[i].height)
        {
            screenIdx = i;
            break;
        }
    }

    Float2 org((float)xsi[screenIdx].x_org, (float)xsi[screenIdx].y_org);
    Float2 size((float)xsi[screenIdx].width, (float)xsi[screenIdx].height);
    X11::XFree(xsi);
	return Rectangle(org, size);
}

Rectangle LinuxPlatform::GetVirtualDesktopBounds()
{
    if (!xDisplay)
        return Rectangle::Empty;

    int event, err;
    const bool ok = X11::XineramaQueryExtension(xDisplay, &event, &err);
    if (!ok)
        return Rectangle::Empty;

    int count;
    X11::XineramaScreenInfo* xsi = X11::XineramaQueryScreens(xDisplay, &count);
    if (count <= 0)
        return Rectangle::Empty;
    // get all screen dimensions and assume the monitors form a rectangle
    // as you can arrange monitors to your liking this is not necessarily the case
    int minX = INT32_MAX, minY = INT32_MAX;
    int maxX = 0, maxY = 0;
    for (int i = 0; i < count; i++)
    {
        int maxScreenX = xsi[i].x_org + xsi[i].width;
        int maxScreenY = xsi[i].y_org + xsi[i].height;
        if (maxScreenX > maxX)
            maxX = maxScreenX;
        if (maxScreenY > maxY)
            maxY = maxScreenY;
        if (minX > xsi[i].x_org)
            minX = xsi[i].x_org;
        if (minY > xsi[i].y_org)
            minY = xsi[i].y_org;
    }

    Float2 org(static_cast<float>(minX), static_cast<float>(minY));
    Float2 size(static_cast<float>(maxX - minX), static_cast<float>(maxY - minY));
    X11::XFree(xsi);
    return Rectangle(org, size);
}

String LinuxPlatform::GetMainDirectory()
{
    char buffer[UNIX_APP_BUFF_SIZE];
    const int32 len = readlink("/proc/self/exe", buffer, UNIX_APP_BUFF_SIZE);
	if (len <= 0)
		return String::Empty;
    const String str(buffer, len);
    int32 pos = str.FindLast(TEXT('/'));
    if (pos != -1 && ++pos < str.Length())
        return str.Left(pos);
    return str;
}

String LinuxPlatform::GetExecutableFilePath()
{
    char buffer[UNIX_APP_BUFF_SIZE];
    const int32 len = readlink("/proc/self/exe", buffer, UNIX_APP_BUFF_SIZE);
	if (len <= 0)
		return String::Empty;
    return String(buffer, len);
}

Guid LinuxPlatform::GetUniqueDeviceId()
{
    return DeviceId;
}

String LinuxPlatform::GetWorkingDirectory()
{
	char buffer[256];
	getcwd(buffer, ARRAY_COUNT(buffer));
	return String(buffer);
}

bool LinuxPlatform::SetWorkingDirectory(const String& path)
{
    return chdir(StringAsANSI<>(*path).Get()) != 0;
}

Window* LinuxPlatform::CreateWindow(const CreateWindowSettings& settings)
{
#if PLATFORM_SDL
    return New<SDLWindow>(settings);
#else
    return New<LinuxWindow>(settings);
#endif
}

extern char **environ;

void LinuxPlatform::GetEnvironmentVariables(Dictionary<String, String, HeapAllocation>& result)
{
	char **s = environ;
	for (; *s; s++)
	{
		char* var = *s;
		int32 split = -1;
		for (int32 i = 0; var[i]; i++)
		{
			if (var[i] == '=')
			{
				split = i;
				break;
			}
		}
		if (split == -1)
			result[String(var)] = String::Empty;
		else
			result[String(var, split)] = String(var + split + 1);
	}
}

bool LinuxPlatform::GetEnvironmentVariable(const String& name, String& value)
{
    char* env = getenv(StringAsANSI<>(*name).Get());
    if (env)
    {
        value = String(env);
        return false;
	}
    return true;
}

bool LinuxPlatform::SetEnvironmentVariable(const String& name, const String& value)
{
    return setenv(StringAsANSI<>(*name).Get(), StringAsANSI<>(*value).Get(), true) != 0;
}

int32 LinuxPlatform::CreateProcess(CreateProcessSettings& settings)
{
    LOG(Info, "Command: {0} {1}", settings.FileName, settings.Arguments);
    if (settings.WorkingDirectory.HasChars())
    {
        LOG(Info, "Working directory: {0}", settings.WorkingDirectory);
    }
    const bool captureStdOut = settings.LogOutput || settings.SaveOutput;
    const String cmdLine = String::Format(TEXT("\"{0}\" {1}"), settings.FileName, settings.Arguments);

	int fildes[2];
	int32 returnCode = 0;
	if (captureStdOut && pipe(fildes) < 0)
    {
		LOG(Warning, "Failed to create a pipe, errno={}", errno);
	}

	pid_t pid = fork();
	if (pid < 0)
    {
		LOG(Warning, "Failed to fork a process, errno={}", errno);
		return errno;
	}
    else if (pid == 0)
    {
		// child process
		int ret;
		const char* const cmd[] = { "sh", "-c", StringAnsi(cmdLine).GetText(), (char *)0 };
		// we could use the execve and supply a list of variable assignments but as we would have to build
		// and quote the values there is hardly any benefit over using setenv() calls
        for (auto& e : settings.Environment)
        {
            setenv(StringAnsi(e.Key).GetText(), StringAnsi(e.Value).GetText(), 1);
        }

        if (settings.WorkingDirectory.HasChars() && chdir(StringAnsi(settings.WorkingDirectory).GetText()) != 0)
        {
            LOG(Warning, "Failed to set working directory to {}, errno={}", settings.WorkingDirectory, errno);
        }
		if (captureStdOut)
        {
			close(fildes[0]); // close the reading end of the pipe
			dup2(fildes[1], STDOUT_FILENO); // redirect stdout to pipe
			close(fildes[1]);
			dup2(STDOUT_FILENO, STDERR_FILENO); // redirect stderr to stdout
		}

		ret = execv("/bin/sh", (char **)cmd);
		if (ret < 0)
        {
			LOG(Warning, " failed, errno={}", errno);
		}
		fflush(stdout);
		_exit(1);
	}
    else
    {
		// parent process
		LOG(Info, "{} started, pid={}", cmdLine, pid);
		if (settings.WaitForEnd)
        {
			if (captureStdOut)
            {
				char lineBuffer[1024];
				close(fildes[1]); // close the writing end of the pipe
				FILE* stdPipe = fdopen(fildes[0], "r");
				while (fgets(lineBuffer, sizeof(lineBuffer), stdPipe) != NULL)
                {
					char *p = lineBuffer + strlen(lineBuffer) - 1;
					if (*p == '\n') *p = 0;
                    String line(lineBuffer);
                    if (settings.SaveOutput)
                        settings.Output.Add(line.Get(), line.Length());
                    if (settings.LogOutput)
                        Log::Logger::Write(LogType::Info, line);
				}
			}
			int stat_loc;
			if (waitpid(pid, &stat_loc, 0) < 0)
            {
				LOG(Warning, "Waiting for pid {} failed, errno={}", pid, errno);
				returnCode = errno;
			}
            else
            {
				if (WIFEXITED(stat_loc))
                {
					int error = WEXITSTATUS(stat_loc);
					if (error != 0)
                    {
						LOG(Warning, "Command exited with error code={}", error);
						returnCode = error;
					}
				}
                else if (WIFSIGNALED(stat_loc))
                {
					LOG(Warning, "Command was killed by signal#{}", WTERMSIG(stat_loc));
					returnCode = EPIPE;
				}
                else if (WIFSTOPPED(stat_loc))
                {
					LOG(Warning, "Command was stopped by signal#{}", WSTOPSIG(stat_loc));
					returnCode = EPIPE;
				}
			}
			close(fildes[0]);
		}
	}

	return returnCode;
}

void* LinuxPlatform::LoadLibrary(const Char* filename)
{
    const StringAsANSI<> filenameANSI(filename);
    void* result = dlopen(filenameANSI.Get(), RTLD_LAZY | RTLD_LOCAL);
    if (!result)
    {
        LOG(Error, "Failed to load {0} because {1}", filename, String(dlerror()));
    }
    return result;
}

void LinuxPlatform::FreeLibrary(void* handle)
{
	dlclose(handle);
}

void* LinuxPlatform::GetProcAddress(void* handle, const char* symbol)
{
    return dlsym(handle, symbol);
}

Array<LinuxPlatform::StackFrame> LinuxPlatform::GetStackFrames(int32 skipCount, int32 maxDepth, void* context)
{
    Array<StackFrame> result;
#if CRASH_LOG_ENABLE
    void* callstack[120];
    skipCount = Math::Min<int32>(skipCount, ARRAY_COUNT(callstack));
    int32 maxCount = Math::Min<int32>(ARRAY_COUNT(callstack), skipCount + maxDepth);
    int32 count = backtrace(callstack, maxCount);
    int32 useCount = count - skipCount;
    if (useCount > 0)
    {
        char** names = backtrace_symbols(callstack + skipCount, useCount);
        result.Resize(useCount);
        for (int32 i = 0; i < useCount; i++)
        {
            char* name = names[i];
            StackFrame& frame = result[i];
            frame.ProgramCounter = callstack[skipCount + i];
            frame.ModuleName[0] = 0;
            frame.FileName[0] = 0;
            frame.LineNumber = 0;
            int32 nameLen = Math::Min<int32>(StringUtils::Length(name), ARRAY_COUNT(frame.FunctionName) - 1);
            Platform::MemoryCopy(frame.FunctionName, name, nameLen);
            frame.FunctionName[nameLen] = 0;
            
        }
        free(names);
    }
#endif
    return result;
}

#endif
