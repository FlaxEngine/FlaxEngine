// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if PLATFORM_SDL && PLATFORM_LINUX

#include "Engine/Platform/Platform.h"
#include "SDLWindow.h"
#include "Engine/Platform/IGuiData.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Platform/SDL/SDLClipboard.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Platform/WindowsManager.h"
#include "Engine/Platform/Linux/IncludeX11.h"

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_system.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_timer.h>

Delegate<void*> LinuxPlatform::xEventReceived;

// Missing Wayland features:
// - Application icon (xdg-toplevel-icon-v1) https://github.com/libsdl-org/SDL/pull/9584
// - Window positioning and position tracking
// - Color picker (xdg-desktop-portal?) https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Screenshot.html
// - 

namespace
{
    bool UseWayland = false;
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
    X11::Atom xAtomAtom;
    X11::Atom xAtomClipboard;
    X11::Atom xAtomPrimary;
    X11::Atom xAtomTargets;
    X11::Atom xAtomText;
    X11::Atom xAtomString;
    X11::Atom xAtomUTF8String;
    X11::Atom xAtomXselData;
    
    X11::Atom xDnDRequested = 0;
    X11::Window xDndSourceWindow = 0;
    DragDropEffect xDndResult;
    Float2 xDndPos;
    int32 xDnDVersion = 0;
    int32 XFixesSelectionNotifyEvent = 0;
}

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

struct Property
{
    unsigned char* data;
    int format, nitems;
    X11::Atom type;
};

namespace Impl
{
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
        X11::XEvent event;
        X11::XConvertSelection(xDisplay, xAtomClipboard, atom, xAtomXselData, window, CurrentTime);
        X11::XSync(xDisplay, 0);
        if (X11::XCheckTypedEvent(xDisplay, SelectionNotify, &event))
        {
            if (event.xselection.selection != xAtomClipboard)
                return;
            if (event.xselection.property)
            {
                X11::XGetWindowProperty(event.xselection.display, event.xselection.requestor, event.xselection.property, 0L,(~0L), 0, AnyPropertyType, &target, &format, &size, &N,(unsigned char**)&data);
                if (target == xAtomUTF8String || target == xAtomString)
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

    static Float2 GetX11MousePosition()
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
}

DragDropEffect Window::DoDragDrop(const StringView& data)
{
    if (CommandLine::Options.Headless)
        return DragDropEffect::None;

    if (UseWayland)
        return DoDragDropWayland(data);
    else
        return DoDragDropX11(data);
}

DragDropEffect Window::DoDragDropWayland(const StringView& data)
{
    // TODO: Wayland
    LOG(Warning, "Wayland Drag and drop is not implemented yet.");
    return DragDropEffect::None;
}

DragDropEffect Window::DoDragDropX11(const StringView& data)
{
	auto cursorWrong = X11::XCreateFontCursor(xDisplay, 54);
	auto cursorTransient = X11::XCreateFontCursor(xDisplay, 24);
	auto cursorGood = X11::XCreateFontCursor(xDisplay, 4);
    Array<X11::Atom, FixedAllocation<3>> formats;
    formats.Add(X11::XInternAtom(xDisplay, "text/plain", 0));
    formats.Add(xAtomText);
    formats.Add(xAtomString);
    StringAnsi dataAnsi(data);
    LinuxDropTextData dropData;
    dropData.Text = data;
#if !PLATFORM_SDL
    X11::Window mainWindow = _window;
#else
    X11::Window mainWindow = static_cast<X11::Window>(GetX11WindowHandle());
#endif

    // Make sure SDL hasn't grabbed the pointer, and force ungrab it
    XUngrabPointer(xDisplay, CurrentTime);
    SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "0");

    // Begin dragging
	auto screen = X11::XDefaultScreen(xDisplay);
	auto rootWindow = X11::XRootWindow(xDisplay, screen);

    if (X11::XGrabPointer(xDisplay, mainWindow, 1, Button1MotionMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, rootWindow, cursorWrong, CurrentTime) != GrabSuccess)
    {
        SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "1");
        return DragDropEffect::None;
    }
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
            if (target == xAtomTargets)
            {
                Array<X11::Atom> targets;
                targets.Add(target);
                targets.Add(X11::XInternAtom(disp, "MULTIPLE", 0));
                targets.Add(formats.Get(), formats.Count());
                X11::XChangeProperty(disp, requestor, property, xAtomAtom, 32, PropModeReplace, (unsigned char*)targets.Get(), targets.Count());
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
                    xDndPos = ww->ScreenToClient(Impl::GetX11MousePosition());
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
                    xDndPos = ww->ScreenToClient(Impl::GetX11MousePosition());
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
                    xDndPos = ww->ScreenToClient(Impl::GetX11MousePosition());
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
            LOG(Warning, "DoDragDrop timed out after 10 seconds.");
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
    X11::XFlush(xDisplay);

    SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "1");

	return result;
}

void SDLClipboard::Clear()
{
    SetText(StringView::Empty);
}

void SDLClipboard::SetText(const StringView& text)
{
    if (CommandLine::Options.Headless)
        return;
    auto mainWindow = Engine::MainWindow;
    if (!mainWindow)
        return;

    if (xDisplay)
    {
        X11::Window window = (X11::Window)(mainWindow->GetX11WindowHandle());
        Impl::ClipboardText.Set(text.Get(), text.Length());
        X11::XSetSelectionOwner(xDisplay, xAtomClipboard, window, CurrentTime); // CLIPBOARD
        //X11::XSetSelectionOwner(xDisplay, xAtomPrimary, window, CurrentTime); // XA_PRIMARY
        X11::XFlush(xDisplay);
        X11::XGetSelectionOwner(xDisplay, xAtomClipboard);
        //X11::XGetSelectionOwner(xDisplay, xAtomPrimary);
    }
    else
    {
        LOG(Warning, "Wayland clipboard support is not implemented yet."); // TODO: Wayland
    }
}

void SDLClipboard::SetRawData(const Span<byte>& data)
{
}

void SDLClipboard::SetFiles(const Array<String>& files)
{
}

String SDLClipboard::GetText()
{
    if (CommandLine::Options.Headless)
        return String::Empty;
    String result;
    auto mainWindow = Engine::MainWindow;
    if (!mainWindow)
        return result;
    if (xDisplay)
    {
        X11::Window window = (X11::Window)mainWindow->GetX11WindowHandle();

        Impl::ClipboardGetText(result, xAtomClipboard, xAtomUTF8String, window);
        if (result.HasChars())
            return result;
        Impl::ClipboardGetText(result, xAtomClipboard, xAtomString, window);
        if (result.HasChars())
            return result;
        Impl::ClipboardGetText(result, xAtomPrimary, xAtomUTF8String, window);
        if (result.HasChars())
            return result;
        Impl::ClipboardGetText(result, xAtomPrimary, xAtomString, window);
        return result;
    }
    else
    {
        LOG(Warning, "Wayland clipboard is not implemented yet."); // TODO: Wayland
    }
}

Array<byte> SDLClipboard::GetRawData()
{
    return Array<byte>();
}

Array<String> SDLClipboard::GetFiles()
{
    return Array<String>();
}

SDL_bool SDLCALL SDLPlatform::X11EventHook(void *userdata, _XEvent *xevent)
{
    const X11::XEvent& event = *(X11::XEvent*)xevent;
    Window* window;

    // External event handling
    xEventReceived(xevent);

    if (event.type == ClientMessage)
    {
        if ((uint32)event.xclient.message_type == (uint32)xAtomXdndEnter)
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
            return SDL_FALSE;
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
            return SDL_FALSE;
        }
        else if ((uint32)event.xclient.message_type == (uint32)xAtomXdndLeave)
        {
            window = WindowsManager::GetByNativePtr((void*)event.xany.window);
            if (window && window->_dragOver)
            {
                window->_dragOver = false;
                window->OnDragLeave();
            }
            return SDL_FALSE;
        }
        else if ((uint32)event.xclient.message_type == (uint32)xAtomXdndDrop)
        {
            auto w = event.xany.window;
            if (xDnDRequested != 0)
            {
                xDndSourceWindow = event.xclient.data.l[0];
                if (xDnDVersion >= 1)
                    XConvertSelection(xDisplay, xAtomXdndSelection, xDnDRequested, xAtomPrimary, w, event.xclient.data.l[2]);
                else
                    XConvertSelection(xDisplay, xAtomXdndSelection, xDnDRequested, xAtomPrimary, w, CurrentTime);
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
            return SDL_FALSE;
        }
    }
    else if (event.type == SelectionNotify)
    {
        if (event.xselection.target == xDnDRequested)
        {
            // Drag&drop
            window = WindowsManager::GetByNativePtr((void*)event.xany.window);
            if (window)
            {
                Property p = Impl::ReadProperty(xDisplay, event.xany.window, xAtomPrimary);
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
            return SDL_FALSE;
        }
        return SDL_FALSE;
    }
    else if (event.type == SelectionRequest)
    {
        if (event.xselectionrequest.selection != xAtomClipboard)
            return SDL_FALSE;
        
        const X11::XSelectionRequestEvent* xsr = &event.xselectionrequest;
        X11::XSelectionEvent ev = { 0 };
        ev.type = SelectionNotify;
        ev.display = xsr->display;
        ev.requestor = xsr->requestor;
        ev.selection = xsr->selection;
        ev.time = xsr->time;
        ev.target = xsr->target;
        ev.property = xsr->property;
        
        int result = 0;
        if (ev.target == xAtomTargets)
        {
            Array<X11::Atom, FixedAllocation<2>> types(2);
            types.Add(xAtomTargets);
            types.Add(xAtomUTF8String);
            result = X11::XChangeProperty(xDisplay, ev.requestor, ev.property, xAtomAtom, 32, PropModeReplace, (unsigned char*)types.Get(), types.Count());
        }
        else if (ev.target == xAtomString || ev.target == xAtomText)
            result = X11::XChangeProperty(xDisplay, ev.requestor, ev.property, xAtomString, 8, PropModeReplace, (unsigned char*)Impl::ClipboardText.Get(), Impl::ClipboardText.Length());
        else if (ev.target == xAtomUTF8String)
            result = X11::XChangeProperty(xDisplay, ev.requestor, ev.property, xAtomUTF8String, 8, PropModeReplace, (unsigned char*)Impl::ClipboardText.Get(), Impl::ClipboardText.Length());
        else
            ev.property = 0;
        if ((result & 2) == 0)
            X11::XSendEvent(xDisplay, ev.requestor, 0, 0, (X11::XEvent*)&ev);
        return SDL_FALSE;
    }
    else if (event.type == SelectionClear)
        return SDL_FALSE;
    else if (event.type == XFixesSelectionNotifyEvent)
        return SDL_FALSE;
    return SDL_TRUE;
}

int X11ErrorHandler(X11::Display* display, X11::XErrorEvent* event)
{
    if (event->error_code == 5)
        return 0; // BadAtom (invalid Atom parameter)
    char buffer[256];
    XGetErrorText(display, event->error_code, buffer, sizeof(buffer));
    LOG(Error, "X11 Error: {0}", String(buffer));
    return 0;
}

bool SDLPlatform::InitPlatform()
{
    if (LinuxPlatform::Init())
        return true;

    if (!CommandLine::Options.Headless)
        UseWayland = strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0;

    return false;
}

bool SDLPlatform::InitPlatformX11(void* display)
{
    if (xDisplay || UseWayland)
        return false;

    // The Display instance must be the same one SDL uses internally
    xDisplay = (X11::Display*)display;
    SDL_SetX11EventHook((SDL_X11EventHook)&X11EventHook, nullptr);
    X11::XSetErrorHandler(X11ErrorHandler);

    //xDisplay = X11::XOpenDisplay(nullptr);
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
    xAtomAtom = static_cast<X11::Atom>(4); // XA_ATOM
    xAtomClipboard = X11::XInternAtom(xDisplay, "CLIPBOARD", 0);
    xAtomPrimary = static_cast<X11::Atom>(1); // XA_PRIMARY
    xAtomTargets = X11::XInternAtom(xDisplay, "TARGETS", 0);
    xAtomText = X11::XInternAtom(xDisplay, "TEXT", 0);
    xAtomString = static_cast<X11::Atom>(31); // XA_STRING
    xAtomUTF8String = X11::XInternAtom(xDisplay, "UTF8_STRING", 1);
    if (xAtomUTF8String == 0)
        xAtomUTF8String = xAtomString;
    xAtomXselData = X11::XInternAtom(xDisplay, "XSEL_DATA", 0);

    // We need to override handling of the XFixes selection tracking events from SDL
    auto screen = X11::XDefaultScreen(xDisplay);
    auto rootWindow = X11::XRootWindow(xDisplay, screen);
    int eventBase = 0, errorBase = 0;
    if (X11::XFixesQueryExtension(xDisplay, &eventBase, &errorBase))
    {
        XFixesSelectionNotifyEvent = eventBase + XFixesSelectionNotify;
        X11::XFixesSelectSelectionInput(xDisplay, rootWindow, xAtomClipboard, XFixesSetSelectionOwnerNotifyMask);
        X11::XFixesSelectSelectionInput(xDisplay, rootWindow, xAtomPrimary, XFixesSetSelectionOwnerNotifyMask);
    }

    return false;
}

void* SDLPlatform::GetXDisplay()
{
    return xDisplay;
}

void SDLPlatform::SetHighDpiAwarenessEnabled(bool enable)
{
    base::SetHighDpiAwarenessEnabled(enable);
}

bool SDLPlatform::UsesWayland()
{
    return UseWayland;
}

bool SDLPlatform::UsesXWayland()
{
    return false;
}

bool SDLPlatform::UsesX11()
{
    return !UseWayland;
}

#endif
