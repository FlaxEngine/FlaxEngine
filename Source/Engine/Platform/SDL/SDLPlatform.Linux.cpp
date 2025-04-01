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
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Platform/WindowsManager.h"
#include "Engine/Platform/Linux/IncludeX11.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Engine/Time.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Platform/Base/DragDropHelper.h"
#include "Engine/Platform/Unix/UnixFile.h"
#include "Engine/Platform/MessageBox.h"

#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_system.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_timer.h>

#include <errno.h>
#include <wayland/xdg-toplevel-drag-v1.h>
#include <wayland/xdg-shell.h>

// Missing Wayland features:
// - Application icon (xdg-toplevel-icon-v1) https://github.com/libsdl-org/SDL/pull/9584
// - Color picker (xdg-desktop-portal?) https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Screenshot.html
// -

class LinuxDropFilesData : public IGuiData
{
public:
    Array<String> Files;
    SDLWindow* Window;

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

namespace WaylandImpl
{
    wl_display* WaylandDisplay = nullptr;

    uint32 GrabSerial = 0;
    wl_pointer_listener PointerListener =
    {
        [](void* data, wl_pointer* wl_pointer, uint32_t serial, wl_surface* surface, wl_fixed_t surface_x, wl_fixed_t surface_y) { }, // Enter event
        [](void* data, wl_pointer* wl_pointer, uint32_t serial, wl_surface* surface) { }, // Leave event
        [](void* data, wl_pointer* wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y) { }, // Motion event
        [](void* data, wl_pointer* wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) // Button event
        {
            // Store the serial for upcoming drag-and-drop action
            if (state == 1)
                GrabSerial = serial;
        },
        [](void* data, wl_pointer* wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value) { }, // Axis event
        [](void* data, wl_pointer* wl_pointer) { }, // Frame event
        [](void* data, wl_pointer* wl_pointer, uint32_t axis_source) { }, // Axis source event
        [](void* data, wl_pointer* wl_pointer, uint32_t time, uint32_t axis) { }, // Axis stop event
        [](void* data, wl_pointer* wl_pointer, uint32_t axis, int32_t discrete) { }, // axis discrete
        [](void* data, wl_pointer* wl_pointer, uint32_t axis, int32_t value120) { }, // Scroll wheel event
        [](void* data, wl_pointer* wl_pointer, uint32_t axis, uint32_t direction) { }, // Relative direction event
    };

    wl_pointer* WaylandPointer = nullptr;
    wl_seat_listener SeatListener =
    {
        [](void* data, wl_seat* seat, uint32 capabilities) // Seat capabilities changed event
        {
            if ((capabilities & wl_seat_capability::WL_SEAT_CAPABILITY_POINTER) != 0)
            {
                WaylandPointer = wl_seat_get_pointer(seat);
                wl_pointer_add_listener(WaylandPointer, &PointerListener, nullptr);
            }
        },
        [](void* data, wl_seat* seat, const char* name) { } // Seat name event
    };

    xdg_toplevel_drag_manager_v1* DragManager = nullptr;
    wl_seat* Seat = nullptr;
    wl_data_device_manager* DataDeviceManager = nullptr;
    wl_registry_listener RegistryListener =
    {
        [](void* data, wl_registry* registry, uint32 id, const char* interface, uint32 version) // Announce global object event
        {
            StringAnsi interfaceStr(interface);
            if (interfaceStr == "xdg_toplevel_drag_manager_v1")
                DragManager = static_cast<xdg_toplevel_drag_manager_v1*>(wl_registry_bind(registry, id, &xdg_toplevel_drag_manager_v1_interface, Math::Min(1U, version)));
            else if (interfaceStr == "wl_seat")
            {
                Seat = static_cast<wl_seat*>(wl_registry_bind(registry, id, &wl_seat_interface, Math::Min(9U, version)));
                wl_seat_add_listener(Seat, &SeatListener, nullptr);
            }
            else if (interfaceStr == "wl_data_device_manager")
                DataDeviceManager = static_cast<wl_data_device_manager*>(wl_registry_bind(registry, id, &wl_data_device_manager_interface, Math::Min(3U, version)));
        },
        [] (void* data, wl_registry* registry, uint32 id) { }, // Announce global remove event
    };

    wl_data_offer* DataOffer = nullptr; // The last accepted offer
    wl_data_offer* SelectionOffer = nullptr;
    wl_data_device_listener DataDeviceListener =
    {
        [](void* data, wl_data_device* data_device, wl_data_offer* id) { }, // Data offer event
        [](void* data, wl_data_device* data_device, uint32 serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y, wl_data_offer* id) // Enter event
        {
            DataOffer = id;

            SDLWindow* sourceWindow = (SDLWindow*)data;
            if (sourceWindow != nullptr)
            {
                // Let them know that we support the following action at this given point
                wl_data_offer_set_actions(id, WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE, WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE);
            }
            else
            {
                wl_data_offer_set_actions(id, WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE, WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE);
            }
        },
        [](void* data, wl_data_device* data_device) // Leave event
        {
            // The cursor left the surface area
            if (DataOffer != nullptr)
                wl_data_offer_destroy(DataOffer);
            DataOffer = nullptr;
        },
        [](void* data, wl_data_device* data_device, uint32_t time, wl_fixed_t x, wl_fixed_t y) { },  // Motion event
        [](void* data, wl_data_device* data_device) // Drop event
        {
            // The drop is accepted
            if (DataOffer != nullptr)
            {
                wl_data_offer_finish(DataOffer);
                wl_data_offer_destroy(DataOffer);
                DataOffer = nullptr;
            }
        },
        [](void* data, wl_data_device* data_device, wl_data_offer* id) // Selection event
        {
            // Clipboard: We can read the clipboard content
            if (SelectionOffer != nullptr)
                wl_data_offer_destroy(SelectionOffer);
            SelectionOffer = id;
        },
    };

    int64 DragOverFlag = 0;
    wl_data_source_listener DataSourceListener =
    {
        [](void* data, wl_data_source* source, const char* mime_type) { }, // Target event
        [](void* data, wl_data_source* source, const char* mime_type, int32_t fd) // Send event
        {
            // Clipboard: The other end has accepted and is requesting the data
            IGuiData* inputData = static_cast<IGuiData*>(data);
            if (inputData->GetType() == IGuiData::Type::Text)
            {
                UnixFile file(fd);
                StringAnsi text = StringAnsi(inputData->GetAsText());
                file.Write(text.Get(), text.Length() * sizeof(StringAnsi::CharType));
                file.Close();
            }
        },
        [](void* data, wl_data_source* source) // Cancelled event
        {
            // Clipboard: other application has replaced the content in clipboard
            IGuiData* inputData = static_cast<IGuiData*>(data);
            Platform::AtomicStore(&WaylandImpl::DragOverFlag, 1);

            wl_data_source_destroy(source);
        },
        [](void* data, wl_data_source* source) { }, // DnD drop performed event
        [](void* data, wl_data_source* source) // DnD Finished event
        {
            // The destination has finally accepted the last given dnd_action
            IGuiData* inputData = static_cast<IGuiData*>(data);
            Platform::AtomicStore(&WaylandImpl::DragOverFlag, 1);

            wl_data_source_destroy(source);
        },
        [](void* data, wl_data_source* source, uint32_t dnd_action) { }, // Action event
    };

    wl_data_device* DataDevice = nullptr;
    wl_event_queue* EventQueue = nullptr;
    wl_data_device_manager* WrappedDataDeviceManager = nullptr;
    wl_data_device* WrappedDataDevice = nullptr;
    bool DraggingActive = false;
    bool DraggingWindow = false;
    StringView DraggingData = nullptr;
    class DragDropJob : public ThreadPoolTask
    {
    public:
        int64 StartFlag = 0;
        int64 WaitFlag = 0;
        int64 ExitFlag = 0;
        SDLWindow* Window = nullptr;
        SDLWindow* DragSourceWindow = nullptr;
        Float2 DragOffset = Float2::Zero;

        // [ThreadPoolTask]
        bool Run() override
        {
            bool dragWindow = DraggingWindow;
            uint32 grabSerial = GrabSerial;

            if (EventQueue == nullptr)
            {
                if (WrappedDataDevice != nullptr)
                    wl_proxy_wrapper_destroy(WrappedDataDevice);
                if (WrappedDataDeviceManager != nullptr)
                    wl_proxy_wrapper_destroy(WrappedDataDeviceManager);
                if (DataDevice != nullptr)
                    wl_data_device_destroy(DataDevice);

                // This seems to throw bogus warnings about wl_data_source still being attached to the queue
                if (EventQueue != nullptr)
                    wl_event_queue_destroy(EventQueue);
                EventQueue = wl_display_create_queue(WaylandDisplay);
                
                WrappedDataDeviceManager = static_cast<wl_data_device_manager*>(wl_proxy_create_wrapper(DataDeviceManager));
                wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(WrappedDataDeviceManager), EventQueue);

                DataDevice = wl_data_device_manager_get_data_device(WrappedDataDeviceManager, Seat);
                wl_data_device_add_listener(DataDevice, &DataDeviceListener, nullptr);
                wl_display_roundtrip(WaylandDisplay);
                wl_data_device_set_user_data(DataDevice, dragWindow ? DragSourceWindow : Window);
                
                WrappedDataDevice = static_cast<wl_data_device*>(wl_proxy_create_wrapper(DataDevice));
                wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(WrappedDataDevice), EventQueue);
            }

            // Offer data for consumption, the data source is destroyed elsewhere
            wl_data_source* dataSource = wl_data_device_manager_create_data_source(WrappedDataDeviceManager);
            wl_data_source* wrappedDataSource = (wl_data_source*)wl_proxy_create_wrapper(dataSource);
            wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(wrappedDataSource), EventQueue);
            if (dragWindow)
            {
                wl_data_source_offer(dataSource, "flaxengine/window");
                wl_data_source_offer(dataSource, "text/plain;charset=utf-8"); // TODO: needs support for custom mime-types in SDL
                wl_data_source_set_actions(dataSource, WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE);
            }
            else
            {
                wl_data_source_offer(dataSource, "text/plain");
                wl_data_source_offer(dataSource, "text/plain;charset=utf-8");
                wl_data_source_set_actions(dataSource, WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE | WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY);
            }
            LinuxDropTextData textData;
            textData.Text = *DraggingData;
            wl_data_source_add_listener(dataSource, &DataSourceListener, &textData);

            // Begin dragging operation
            auto draggedWindow = Window->GetSDLWindow();
            auto dragStartWindow = DragSourceWindow != nullptr ? DragSourceWindow->GetSDLWindow() : draggedWindow;
            wl_surface* originSurface = static_cast<wl_surface*>(SDL_GetPointerProperty(SDL_GetWindowProperties(dragStartWindow), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr));
            wl_surface* iconSurface = nullptr;
            wl_data_device_start_drag(WrappedDataDevice, dataSource, originSurface, iconSurface, grabSerial);

            Platform::AtomicStore(&StartFlag, 1);

            xdg_toplevel_drag_v1* toplevelDrag = nullptr;
            xdg_toplevel* wrappedToplevel = nullptr;

            while (Platform::AtomicRead(&ExitFlag) == 0)
            {
                // Start dispatching events to keep data offers alive
                if (wl_display_dispatch_queue(WaylandDisplay, EventQueue) == -1)
                    LOG(Warning, "wl_display_dispatch_queue failed, errno: {}", errno);
                if (wl_display_roundtrip_queue(WaylandDisplay, EventQueue) == -1)
                    LOG(Warning, "wl_display_roundtrip_queue failed, errno: {}", errno);

                // Wait until window has showed up
                if (DragManager != nullptr && wrappedToplevel == nullptr && dragWindow && Platform::AtomicRead(&WaitFlag) != 0)
                {
                    auto toplevel = static_cast<xdg_toplevel*>(SDL_GetPointerProperty(SDL_GetWindowProperties(draggedWindow), SDL_PROP_WINDOW_WAYLAND_XDG_TOPLEVEL_POINTER, nullptr));
                    if (toplevel != nullptr)
                    {
                        // Attach the window to the ongoing drag operation
                        wrappedToplevel = static_cast<xdg_toplevel*>(wl_proxy_create_wrapper(toplevel));
                        wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(wrappedToplevel), EventQueue);
                        toplevelDrag = xdg_toplevel_drag_manager_v1_get_xdg_toplevel_drag(DragManager, dataSource);

                        Float2 scaledOffset = DragOffset / Window->GetDpiScale();
                        xdg_toplevel_drag_v1_attach(toplevelDrag, wrappedToplevel, static_cast<int32>(scaledOffset.X), static_cast<int32>(scaledOffset.Y));
                    }
                }
            }

            if (wl_display_roundtrip_queue(WaylandDisplay, EventQueue) == -1)
                LOG(Warning, "wl_display_roundtrip_queue failed, errno: {}", errno);

            if (toplevelDrag != nullptr)
            {
                wl_proxy_wrapper_destroy(wrappedToplevel);
                xdg_toplevel_drag_v1_destroy(toplevelDrag);
                toplevelDrag = nullptr;
            }

            if (wrappedDataSource != nullptr)
                wl_proxy_wrapper_destroy(wrappedDataSource);

            if (SelectionOffer != nullptr)
            {
                wl_data_offer_destroy(SelectionOffer);
                SelectionOffer = nullptr;
            }

            // We can't release the queue immediately due to some resources being still used for a while
            /*if (WaylandQueue != nullptr)
            {
                wl_event_queue_destroy(WaylandQueue);
                WaylandQueue = nullptr;
            }*/

            return false;
        }
    };
}

// X11
Delegate<void*> LinuxPlatform::xEventReceived;

namespace X11Impl
{
    Window* DraggedWindow = nullptr;
    
    struct Property
    {
        unsigned char* data;
        int format, nitems;
        X11::Atom type;
    };
    
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

    static StringAnsi GetAtomName(X11::Display* display, X11::Atom atom)
    {
        char* atomNamePtr = X11::XGetAtomName(display, atom);
        StringAnsi atomName = StringAnsi(atomNamePtr);
        X11::XFree(atomNamePtr);
        return atomName;
    }
    
    static X11::Atom SelectTargetFromList(X11::Display* display, const char* targetType, X11::Atom* list, int count)
    {
        for (int i = 0; i < count; i++)
        {
            X11::Atom atom = list[i];
            if (atom != 0 && GetAtomName(display, atom) == targetType)
                return atom;
        }
        return 0;
    }

    static X11::Atom SelectTargetFromAtoms(X11::Display* display, const char* targetType, X11::Atom t1, X11::Atom t2, X11::Atom t3)
    {
        if (t1 != 0 && GetAtomName(display, t1) == targetType)
            return t1;
        if (t2 != 0 && GetAtomName(display, t2) == targetType)
            return t2;
        if (t3 != 0 && GetAtomName(display, t3) == targetType)
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

    if (SDLPlatform::UsesWayland())
        return DoDragDropWayland(data);
    else
        return DoDragDropX11(data);
}

DragDropEffect Window::DoDragDropWayland(const StringView& data, Window* dragSourceWindow, Float2 dragOffset)
{
    // For drag-and-drop, we need to run another event queue in a separate thread to avoid racing issues
    // while SDL is dispatching the main Wayland event queue when receiving the data offer from us.

    Engine::OnDraw();

    WaylandImpl::DraggingActive = true;
    WaylandImpl::DraggingData = StringView(data.Get(), data.Length());
    WaylandImpl::DragOverFlag = 0;

    auto task = New<WaylandImpl::DragDropJob>();
    task->Window = this;
    task->DragSourceWindow = dragSourceWindow; // Needs to be the parent window when dragging a tab to window
    task->DragOffset = dragOffset;
    Task::StartNew(task);
    while (task->GetState() == TaskState::Queued)
        Platform::Sleep(1);

    while (Platform::AtomicRead(&task->StartFlag) == 0)
        Platform::Sleep(1);

    while (Platform::AtomicRead(&WaylandImpl::DragOverFlag) == 0)
    {
        SDLPlatform::Tick();
        Engine::OnUpdate();//Scripting::Update(); // For docking updates
        Engine::OnDraw();

        // The window needs to be finished showing up before we can start dragging it
        if (IsVisible() && Platform::AtomicRead(&task->WaitFlag) == 0)
            Platform::AtomicStore(&task->WaitFlag, 1);

        Platform::Sleep(1);
    }

    // The mouse up event was ignored earlier, release the button now
    Input::Mouse->OnMouseUp(Platform::GetMousePosition(), MouseButton::Left, this);

    Platform::AtomicStore(&task->ExitFlag, 1);
    task->Wait();

    WaylandImpl::DraggingActive = false;
    WaylandImpl::DraggingData = nullptr;
    
    return DragDropEffect::None;
}

DragDropEffect Window::DoDragDropX11(const StringView& data)
{
    using namespace X11Impl;
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
    auto hintAutoCapture = SDL_GetHint(SDL_HINT_MOUSE_AUTO_CAPTURE);
    SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "0");

    // Begin dragging
	auto screen = X11::XDefaultScreen(xDisplay);
	auto rootWindow = X11::XRootWindow(xDisplay, screen);

    if (X11::XGrabPointer(xDisplay, mainWindow, 1, Button1MotionMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, rootWindow, cursorWrong, CurrentTime) != GrabSuccess)
    {
        SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, hintAutoCapture);
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
            auto window = X11Impl::FindAppWindow(xDisplay, rootWindow);
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
                    xDndPos = ww->ScreenToClient(X11Impl::GetX11MousePosition());
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
                    xDndPos = ww->ScreenToClient(X11Impl::GetX11MousePosition());
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
                    xDndPos = ww->ScreenToClient(X11Impl::GetX11MousePosition());
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

    SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, hintAutoCapture);

	return result;
}

void SDLPlatform::PreHandleEvents()
{
}

void SDLPlatform::PostHandleEvents()
{
    // Handle window dragging release here
    if (X11Impl::DraggedWindow != nullptr)
    {
        Float2 mousePosition;
        auto buttons = SDL_GetGlobalMouseState(&mousePosition.X, &mousePosition.Y);
        bool buttonReleased = (buttons & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) == 0;
        if (buttonReleased)
        {
            // Send simulated mouse up event
            SDL_Event buttonUpEvent { 0 };
            buttonUpEvent.motion.type = SDL_EVENT_MOUSE_BUTTON_UP;
            buttonUpEvent.button.down = false;
            buttonUpEvent.motion.windowID = SDL_GetWindowID(X11Impl::DraggedWindow->GetSDLWindow());
            buttonUpEvent.motion.timestamp = SDL_GetTicksNS();
            buttonUpEvent.motion.state = SDL_BUTTON_LEFT;
            buttonUpEvent.button.clicks = 1;
            buttonUpEvent.motion.x = mousePosition.X;
            buttonUpEvent.motion.y = mousePosition.Y;
            X11Impl::DraggedWindow->HandleEvent(buttonUpEvent);
            X11Impl::DraggedWindow = nullptr;
        }
    }
}

bool SDLWindow::HandleEventInternal(SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_EVENT_WINDOW_MOVED:
    {
        if (SDLPlatform::UsesX11())
        {
            // X11 doesn't report any mouse events when mouse is over the caption area, send a simulated event instead...
            Float2 mousePosition;
            auto buttons = SDL_GetGlobalMouseState(&mousePosition.X, &mousePosition.Y);
            if ((buttons & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0 && X11Impl::DraggedWindow == nullptr)
            {
                // TODO: verify mouse position, window focus
                bool result = false;
                OnLeftButtonHit(WindowHitCodes::Caption, result);
                if (result)
                    X11Impl::DraggedWindow = this;
            }
        }
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_MOUSE_MOTION:
    {
        if (SDLPlatform::UsesWayland() && WaylandImpl::DraggingActive)
        {
            // Ignore mouse events in dragged window
            return true;
        }
        break;
    }
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
    {
        OnDragLeave(); // Check for release of mouse button too?
        break;
    }
    case SDL_EVENT_DROP_BEGIN:
    case SDL_EVENT_DROP_POSITION:
    case SDL_EVENT_DROP_FILE:
    case SDL_EVENT_DROP_TEXT:
    case SDL_EVENT_DROP_COMPLETE:
    {
        if (SDLPlatform::UsesWayland())
        {
            // HACK: We can't use Wayland listeners due to SDL also using them at the same time causes
            // some of the events to drop and make it impossible to implement dragging on application side.
            // We can get enough information through SDL_EVENT_DROP_* events to fill in the blanks for the
            // drag and drop implementation.

            auto dpiScale = GetDpiScale();
            const Float2 mousePos = Float2(event.drop.x * dpiScale, event.drop.y * dpiScale);
            DragDropEffect effect = DragDropEffect::None;
            String text(event.drop.data);
            LinuxDropTextData textData;
            LinuxDropFilesData filesData;

            if (WaylandImpl::DraggingActive && (event.type == SDL_EVENT_DROP_BEGIN || event.type == SDL_EVENT_DROP_POSITION))
            {
                // We don't have the window dragging data during these events...
                text = WaylandImpl::DraggingData;
            }
            textData.Text = text;

            if (event.type == SDL_EVENT_DROP_BEGIN)
            {
                // We don't know the type of dragged data at this point, so call the events for both types
                OnDragEnter(&filesData, mousePos, effect);
                if (effect == DragDropEffect::None)
                    OnDragEnter(&textData, mousePos, effect);
            }
            else if (event.type == SDL_EVENT_DROP_POSITION)
            {
                Input::Mouse->OnMouseMove(ClientToScreen(mousePos), this);

                // We don't know the type of dragged data at this point, so call the events for both types
                OnDragOver(&filesData, mousePos, effect);
                if (effect == DragDropEffect::None)
                    OnDragOver(&textData, mousePos, effect);
            }
            else if (event.type == SDL_EVENT_DROP_FILE)
            {
                text.Split('\n', filesData.Files);
                OnDragDrop(&filesData, mousePos, effect);
            }
            else if (event.type == SDL_EVENT_DROP_TEXT)
                OnDragDrop(&textData, mousePos, effect);
            else if (event.type == SDL_EVENT_DROP_COMPLETE)
                OnDragLeave();

            // TODO: Implement handling for feedback effect result (https://github.com/libsdl-org/SDL/issues/10448)
        }
        break;
    }
    default:
        break;
    }
    
    return false;
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

    if (X11Impl::xDisplay)
    {
        X11::Window window = (X11::Window)(mainWindow->GetX11WindowHandle());
        X11Impl::ClipboardText.Set(text.Get(), text.Length());
        X11::XSetSelectionOwner(X11Impl::xDisplay, X11Impl::xAtomClipboard, window, CurrentTime); // CLIPBOARD
        //X11::XSetSelectionOwner(xDisplay, xAtomPrimary, window, CurrentTime); // XA_PRIMARY
        X11::XFlush(X11Impl::xDisplay);
        X11::XGetSelectionOwner(X11Impl::xDisplay, X11Impl::xAtomClipboard);
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
    if (X11Impl::xDisplay)
    {
        X11::Window window = static_cast<X11::Window>(mainWindow->GetX11WindowHandle());

        X11Impl::ClipboardGetText(result, X11Impl::xAtomClipboard, X11Impl::xAtomUTF8String, window);
        if (result.HasChars())
            return result;
        X11Impl::ClipboardGetText(result, X11Impl::xAtomClipboard, X11Impl::xAtomString, window);
        if (result.HasChars())
            return result;
        X11Impl::ClipboardGetText(result, X11Impl::xAtomPrimary, X11Impl::xAtomUTF8String, window);
        if (result.HasChars())
            return result;
        X11Impl::ClipboardGetText(result, X11Impl::xAtomPrimary, X11Impl::xAtomString, window);
        return result;
    }
    else
    {
        LOG(Warning, "Wayland clipboard is not implemented yet."); // TODO: Wayland
        return String::Empty;
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

bool SDLCALL SDLPlatform::X11EventHook(void* userdata, _XEvent* xevent)
{
    using namespace X11Impl;
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
                Property p = X11Impl::ReadProperty(xDisplay, source, XInternAtom(xDisplay, "XdndTypeList", 0));
                xDnDRequested = X11Impl::SelectTargetFromList(xDisplay, targetTypeFiles, (X11::Atom*)p.data, p.nitems);
                X11::XFree(p.data);
            }
            else
            {
                xDnDRequested = X11Impl::SelectTargetFromAtoms(xDisplay, targetTypeFiles, event.xclient.data.l[2], event.xclient.data.l[3], event.xclient.data.l[4]);
            }
            return false;
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
            xDndPos = Float2((float)(event.xclient.data.l[2] >> 16), (float)(event.xclient.data.l[2] & 0xffff));
            window = WindowsManager::GetByNativePtr((void*)event.xany.window);
            if (window)
            {
                xDndPos = window->ScreenToClient(xDndPos);
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
            return false;
        }
        else if ((uint32)event.xclient.message_type == (uint32)xAtomXdndLeave)
        {
            window = WindowsManager::GetByNativePtr((void*)event.xany.window);
            if (window && window->_dragOver)
            {
                window->_dragOver = false;
                window->OnDragLeave();
            }
            return false;
        }
        else if ((uint32)event.xclient.message_type == (uint32)xAtomXdndDrop)
        {
            if (xDnDRequested != 0)
            {
                xDndSourceWindow = event.xclient.data.l[0];
                XConvertSelection(xDisplay, xAtomXdndSelection, xDnDRequested, xAtomPrimary,
                    event.xany.window, xDnDVersion >= 1 ? event.xclient.data.l[2] : CurrentTime);
                X11::XFlush(xDisplay);
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
                m.data.l[0] = event.xany.window;
                m.data.l[1] = 0;
                m.data.l[2] = 0;
                X11::XSendEvent(xDisplay, event.xclient.data.l[0], 0, NoEventMask, (X11::XEvent*)&m);
            }
            return false;
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
                Property p = X11Impl::ReadProperty(xDisplay, event.xany.window, xAtomPrimary);
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
            return false;
        }
        return false;
    }
    else if (event.type == SelectionRequest)
    {
        if (event.xselectionrequest.selection != xAtomClipboard)
            return false;
        
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
            result = X11::XChangeProperty(xDisplay, ev.requestor, ev.property, xAtomString, 8, PropModeReplace, (unsigned char*)X11Impl::ClipboardText.Get(), X11Impl::ClipboardText.Length());
        else if (ev.target == xAtomUTF8String)
            result = X11::XChangeProperty(xDisplay, ev.requestor, ev.property, xAtomUTF8String, 8, PropModeReplace, (unsigned char*)X11Impl::ClipboardText.Get(), X11Impl::ClipboardText.Length());
        else
            ev.property = 0;
        if ((result & 2) == 0)
            X11::XSendEvent(xDisplay, ev.requestor, 0, 0, (X11::XEvent*)&ev);
        return false;
    }
    else if (event.type == SelectionClear)
        return false;
    else if (event.type == XFixesSelectionNotifyEvent)
        return false;
    return true;
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

bool SDLPlatform::InitInternal()
{
    bool waylandRequested = (!CommandLine::Options.X11 || CommandLine::Options.Wayland) && StringAnsi(SDL_GetHint(SDL_HINT_VIDEO_DRIVER)) == "wayland";
    if (!CommandLine::Options.Headless && waylandRequested)
    {
        // Ignore in X11 session
        String waylandDisplayEnv;
        if (!GetEnvironmentVariable(String("WAYLAND_DISPLAY"), waylandDisplayEnv))
        {
            WaylandImpl::WaylandDisplay = (wl_display*)SDL_GetPointerProperty(SDL_GetGlobalProperties(), SDL_PROP_GLOBAL_VIDEO_WAYLAND_WL_DISPLAY_POINTER, nullptr);
            if (WaylandImpl::WaylandDisplay != nullptr)
            {
                // Tap into Wayland registry so we can start listening for events
                wl_registry* registry = wl_display_get_registry(WaylandImpl::WaylandDisplay);
                wl_registry_add_listener(registry, &WaylandImpl::RegistryListener, nullptr);
                wl_display_roundtrip(WaylandImpl::WaylandDisplay);
            }
        }
    }

    return false;
}

bool SDLPlatform::InitX11(void* display)
{
    using namespace X11Impl;
    if (xDisplay || WaylandImpl::WaylandDisplay)
        return false;

    // The Display instance must be the same one SDL uses internally
    xDisplay = (X11::Display*)display;
    SDL_SetX11EventHook((SDL_X11EventHook)&X11EventHook, nullptr);
    X11::XSetErrorHandler(X11ErrorHandler);

    xDisplay = X11::XOpenDisplay(nullptr);
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
    return X11Impl::xDisplay;
}

void SDLPlatform::SetHighDpiAwarenessEnabled(bool enable)
{
    base::SetHighDpiAwarenessEnabled(enable);
}

bool SDLPlatform::UsesWindows()
{
    return false;
}

bool SDLPlatform::UsesWayland()
{
    if (X11Impl::xDisplay == nullptr && WaylandImpl::WaylandDisplay == nullptr)
    {
        // In case the X11 display pointer has not been updated yet
        return strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0;
    }
    return WaylandImpl::WaylandDisplay != nullptr;
}

bool SDLPlatform::UsesX11()
{
    if (X11Impl::xDisplay == nullptr && WaylandImpl::WaylandDisplay == nullptr)
    {
        // In case the X11 display pointer has not been updated yet
        return strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0;
    }
    return X11Impl::xDisplay != nullptr;
}

DragDropEffect SDLWindow::DoDragDrop(const StringView& data, const Float2& offset, Window* dragSourceWindow)
{
    if (SDLPlatform::UsesWayland())
    {
        Float2 dragOffset = offset;
        if (_settings.HasBorder && dragSourceWindow == this)
        {
            // Wayland includes the decorations in the client-space coordinates, adjust the offset for it.
            // Assume the title decoration is 25px thick...
            float topOffset = 25.0f;
            dragOffset += Float2(0.0f, topOffset);
        }

        // Show the window without changing focus
        if (!_visible)
        {
            if (_showAfterFirstPaint)
            {
                if (RenderTask)
                    RenderTask->Enabled = true;
            }
            else
                SDL_ShowWindow(_window);
        }
        // Only show the window if toplevel dragging is supported
        if (WaylandImpl::DragManager != nullptr)
            WindowBase::Show();
        else
            Hide();
        
        WaylandImpl::DraggingWindow = true;
        DoDragDropWayland(String(""), dragSourceWindow, dragOffset);
        WaylandImpl::DraggingWindow = false;
    }
    else
        Show();
    return DragDropEffect::None;
}

DialogResult MessageBox::Show(Window* parent, const StringView& text, const StringView& caption, MessageBoxButtons buttons, MessageBoxIcon icon)
{
    StringAnsi textAnsi(text);
    StringAnsi captionAnsi(caption);

    SDL_MessageBoxData data;
    SDL_MessageBoxButtonData dataButtons[3];
    data.window = parent ? static_cast<SDLWindow*>(parent)->_window : nullptr;
    data.title = captionAnsi.GetText();
    data.message = textAnsi.GetText();
    data.colorScheme = nullptr;

    switch (icon)
    {
    case MessageBoxIcon::Error:
    case MessageBoxIcon::Hand:
    case MessageBoxIcon::Stop:
        data.flags |= SDL_MESSAGEBOX_ERROR;
        break;
    case MessageBoxIcon::Asterisk:
    case MessageBoxIcon::Information:
    case MessageBoxIcon::Question:
        data.flags |= SDL_MESSAGEBOX_INFORMATION;
        break;
    case MessageBoxIcon::Exclamation:
    case MessageBoxIcon::Warning:
        data.flags |= SDL_MESSAGEBOX_WARNING;
        break;
    default:
        break;
    }

    switch (buttons)
    {
    case MessageBoxButtons::AbortRetryIgnore:
        dataButtons[0] =
        {
            SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            (int)DialogResult::Abort,
            "Abort"
        };
        dataButtons[1] =
        {
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
            (int)DialogResult::Retry,
            "Retry"
        };
        dataButtons[2] =
        {
            0,
            (int)DialogResult::Ignore,
            "Ignore"
        };
        data.numbuttons = 3;
        break;
    case MessageBoxButtons::OK:
        dataButtons[0] =
        {
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            (int)DialogResult::OK,
            "OK"
        };
        data.numbuttons = 1;
        break;
    case MessageBoxButtons::OKCancel:
        dataButtons[0] =
        {
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
            (int)DialogResult::OK,
            "OK"
        };
        dataButtons[1] =
        {
            SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            (int)DialogResult::Cancel,
            "Cancel"
        };
        data.numbuttons = 2;
        break;
    case MessageBoxButtons::RetryCancel:
        dataButtons[0] =
        {
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
            (int)DialogResult::Retry,
            "Retry"
        };
        dataButtons[1] =
        {
            SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            (int)DialogResult::Cancel,
            "Cancel"
        };
        data.numbuttons = 2;
        break;
    case MessageBoxButtons::YesNo:
        dataButtons[0] =
        {
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
            (int)DialogResult::Yes,
            "Yes"
        };
        dataButtons[1] =
        {
            SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            (int)DialogResult::No,
            "No"
        };
        data.numbuttons = 2;
        break;
    case MessageBoxButtons::YesNoCancel:
    {
        dataButtons[0] =
        {
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
            (int)DialogResult::Yes,
            "Yes"
        };
        dataButtons[1] =
        {
            0,
            (int)DialogResult::No,
            "No"
        };
        dataButtons[2] =
        {
            SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            (int)DialogResult::Cancel,
            "Cancel"
        };
        data.numbuttons = 3;
        break;
    }
    default:
        break;
    }
    data.buttons = dataButtons;

    int result = -1;
    if (!SDL_ShowMessageBox(&data, &result))
    {
#if PLATFORM_LINUX
        // Fallback to native messagebox implementation in case some system fonts are missing
        if (SDLPlatform::UsesX11())
        {
            LOG(Warning, "Failed to show SDL message box: {0}", String(SDL_GetError()));
            return ShowFallback(parent, text, caption, buttons, icon);
        }
#endif
        LOG(Error, "Failed to show SDL message box: {0}", String(SDL_GetError()));
        return DialogResult::Abort;
    }
    if (result < 0)
        return DialogResult::None;
    return (DialogResult)result;
}

#endif
