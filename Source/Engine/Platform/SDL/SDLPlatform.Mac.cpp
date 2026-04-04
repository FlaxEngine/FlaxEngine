// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_SDL && PLATFORM_MAC

#include "SDLWindow.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/Time.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Platform/IGuiData.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Platform/WindowsManager.h"
#include "Engine/Platform/Base/DragDropHelper.h"
#include "Engine/Platform/SDL/SDLClipboard.h"

#include "Engine/Platform/Apple/AppleUtils.h"
#include <Cocoa/Cocoa.h>
#include <AppKit/AppKit.h>
#include <QuartzCore/CAMetalLayer.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_system.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

namespace MacImpl
{
    Window* DraggedWindow = nullptr;
    String DraggingData = String();
    Float2 DraggingPosition;
    Nullable<Float2> LastMouseDragPosition;
    bool DraggingActive = false;
    bool DraggingIgnoreEvent = false;
    NSDraggingSession* MacDragSession = nullptr;
    int64 MacDragExitFlag = 0;
}

class MacDropData : public IGuiData
{
public:
    Type CurrentType;
    String AsText;
    Array<String> AsFiles;

    Type GetType() const override
    {
        return CurrentType;
    }
    String GetAsText() const override
    {
        return AsText;
    }
    void GetAsFiles(Array<String>* files) const override
    {
        files->Add(AsFiles);
    }
};

bool SDLPlatform::InitInternal()
{
    return false;
}

bool SDLPlatform::UsesWindows()
{
    return false;
}

bool SDLPlatform::UsesWayland()
{
    return false;
}

bool SDLPlatform::UsesX11()
{
    return false;
}

bool SDLPlatform::EventFilterCallback(void* userdata, SDL_Event* event)
{
    Window* draggedWindow = *(Window**)userdata;
    if (draggedWindow == nullptr)
    {
        if (MacImpl::DraggingActive)
        {
            // Handle events during drag operation here since the normal event loop is blocked
            if (event->type == SDL_EVENT_WINDOW_EXPOSED)
            {
                // The internal timer is sending exposed events every ~16ms
#if USE_EDITOR
                // Flush any single-frame shapes to prevent memory leaking (eg. via terrain collision debug during scene drawing with PhysicsColliders or PhysicsDebug flag)
                DebugDraw::UpdateContext(nullptr, 0.0f);
#endif
                Engine::OnUpdate(); // For docking updates
                Engine::OnDraw();
            }
            else
            {
                SDLWindow* window = SDLWindow::GetWindowFromEvent(*event);
                if (window)
                    window->HandleEvent(*event);

                // We do not receive events at steady rate to keep the engine updated...
#if USE_EDITOR
                // Flush any single-frame shapes to prevent memory leaking (eg. via terrain collision debug during scene drawing with PhysicsColliders or PhysicsDebug flag)
                DebugDraw::UpdateContext(nullptr, 0.0f);
#endif
                Engine::OnUpdate(); // For docking updates
                Engine::OnDraw();

                if (event->type == SDL_EVENT_DROP_BEGIN || event->type == SDL_EVENT_DROP_FILE || event->type == SDL_EVENT_DROP_TEXT)
                    return true; // Filtering these event stops other following events from getting added to the queue
            }
            return false;
        }
        return true;
    }
    return true;
}

void SDLPlatform::PreHandleEvents()
{
    SDL_SetEventFilter(EventFilterCallback, &MacImpl::DraggedWindow);
}

void SDLPlatform::PostHandleEvents()
{
    SDL_SetEventFilter(EventFilterCallback, &MacImpl::DraggedWindow);

    // Handle window dragging release here
    if (MacImpl::DraggedWindow != nullptr)
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
            buttonUpEvent.motion.windowID = SDL_GetWindowID(MacImpl::DraggedWindow->GetSDLWindow());
            buttonUpEvent.motion.timestamp = SDL_GetTicksNS();
            buttonUpEvent.motion.state = SDL_BUTTON_LEFT;
            buttonUpEvent.button.clicks = 1;
            buttonUpEvent.motion.x = mousePosition.X;
            buttonUpEvent.motion.y = mousePosition.Y;
            MacImpl::DraggedWindow->HandleEvent(buttonUpEvent);
            MacImpl::DraggedWindow = nullptr;
        }
    }
}

bool SDLWindow::HandleEventInternal(SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_EVENT_WINDOW_MOVED:
    {
        // Quartz doesn't report any mouse events when mouse is over the caption area, send a simulated event instead...
        Float2 mousePosition;
        auto buttons = SDL_GetGlobalMouseState(&mousePosition.X, &mousePosition.Y);
        if ((buttons & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0)
        {
            if (MacImpl::DraggedWindow == nullptr)
            {
                // TODO: verify mouse position, window focus
                bool result = false;
                OnLeftButtonHit(WindowHitCodes::Caption, result);
                if (result)
                    MacImpl::DraggedWindow = this;
            }
            else
            {
                Float2 mousePos = Platform::GetMousePosition();
                Input::Mouse->OnMouseMove(mousePos, this);
            }
        }
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    {
        if (MacImpl::LastMouseDragPosition.HasValue())
        {
            // SDL reports wrong mouse position after dragging has ended
            Float2 mouseClientPosition = ScreenToClient(MacImpl::LastMouseDragPosition.GetValue());
            event.button.x = mouseClientPosition.X;
            event.button.y = mouseClientPosition.Y;
        }
        break;
    }
    case SDL_EVENT_MOUSE_MOTION:
    {
        if (MacImpl::LastMouseDragPosition.HasValue())
            MacImpl::LastMouseDragPosition.Reset();
        if (MacImpl::DraggedWindow != nullptr)
            return true;
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
        auto dpiScale = GetDpiScale();
        Float2 mousePos = Float2(event.drop.x * dpiScale, event.drop.y * dpiScale);
        DragDropEffect effect = DragDropEffect::None;
        String text(event.drop.data);
        MacDropData dropData;

        if (MacImpl::DraggingActive)
        {
            // We don't have the window dragging data during these events...
            text = MacImpl::DraggingData;
            mousePos = ScreenToClient(MacImpl::DraggingPosition);

            // Ensure mouse position is updated while dragging
            Input::Mouse->OnMouseMove(MacImpl::DraggingPosition, this);
            MacImpl::LastMouseDragPosition = MacImpl::DraggingPosition;
        }
        dropData.AsText = text;

        if (event.type == SDL_EVENT_DROP_BEGIN)
        {
            // We don't know the type of dragged data at this point, so call the events for both types
            if (!MacImpl::DraggingActive)
            {
                dropData.CurrentType = IGuiData::Type::Files;
                OnDragEnter(&dropData, mousePos, effect);
            }
            if (effect == DragDropEffect::None)
            {
                dropData.CurrentType = IGuiData::Type::Text;
                OnDragEnter(&dropData, mousePos, effect);
            }
        }
        else if (event.type == SDL_EVENT_DROP_POSITION)
        {
            Input::Mouse->OnMouseMove(ClientToScreen(mousePos), this);

            // We don't know the type of dragged data at this point, so call the events for both types
            if (!MacImpl::DraggingActive)
            {
                dropData.CurrentType = IGuiData::Type::Files;
                OnDragOver(&dropData, mousePos, effect);
            }
            if (effect == DragDropEffect::None)
            {
                dropData.CurrentType = IGuiData::Type::Text;
                OnDragOver(&dropData, mousePos, effect);
            }
        }
        else if (event.type == SDL_EVENT_DROP_FILE)
        {
            text.Split('\n', dropData.AsFiles);
            dropData.CurrentType = IGuiData::Type::Files;
            OnDragDrop(&dropData, mousePos, effect);
        }
        else if (event.type == SDL_EVENT_DROP_TEXT)
        {
            dropData.CurrentType = IGuiData::Type::Text;
            OnDragDrop(&dropData, mousePos, effect);
        }
        else if (event.type == SDL_EVENT_DROP_COMPLETE)
        {
            OnDragLeave();
            if (MacImpl::DraggingActive)
            {
                // The previous drop events needs to be flushed to avoid processing them twice
                SDL_FlushEvents(SDL_EVENT_DROP_FILE, SDL_EVENT_DROP_POSITION);
            }
        }

        // TODO: Implement handling for feedback effect result (https://github.com/libsdl-org/SDL/issues/10448)
        break;
    }
    }
    return false;
}

void SDLPlatform::SetHighDpiAwarenessEnabled(bool enable)
{
    // TODO: This is now called before Platform::Init, ensure the scaling is changed accordingly during Platform::Init (see ApplePlatform::SetHighDpiAwarenessEnabled)
}

inline bool IsWindowInvalid(Window* win)
{
    WindowsManager::WindowsLocker.Lock();
    const bool hasWindow = WindowsManager::Windows.Contains(win);
    WindowsManager::WindowsLocker.Unlock();
    return !hasWindow || !win;
}

Float2 GetWindowTitleSize(const SDLWindow* window)
{
    Float2 size = Float2::Zero;
    if (window->GetSettings().HasBorder)
    {
        NSRect frameStart = [(NSWindow*)window->GetNativePtr() frameRectForContentRect:NSMakeRect(0, 0, 0, 0)];
        size.Y = frameStart.size.height;
    }
    return size * MacPlatform::ScreenScale;
}

Float2 GetMousePosition(SDLWindow* window, NSEvent* event)
{
    NSRect frame = [(NSWindow*)window->GetNativePtr() frame];
    NSPoint point = [event locationInWindow];
    return Float2(point.x, frame.size.height - point.y) * MacPlatform::ScreenScale - GetWindowTitleSize(window);
}

Float2 GetMousePosition(SDLWindow* window, const NSPoint& point)
{
    NSRect frame = [(NSWindow*)window->GetNativePtr() frame];
    CGRect screenBounds = CGDisplayBounds(CGMainDisplayID());
    return Float2(point.x, screenBounds.size.height - point.y) * MacPlatform::ScreenScale;
}

@interface ClipboardDataProviderImpl : NSObject <NSPasteboardItemDataProvider, NSDraggingSource>
{
@public
    SDLWindow* Window;
}
@end

@implementation ClipboardDataProviderImpl

// NSPasteboardItemDataProvider
// ---

- (void)pasteboard:(nullable NSPasteboard*)pasteboard item:(NSPasteboardItem*)item provideDataForType:(NSPasteboardType)type
{
    if (IsWindowInvalid(Window)) return;
    [pasteboard setString:(NSString*)AppleUtils::ToString(MacImpl::DraggingData) forType:NSPasteboardTypeString];
}

- (void)pasteboardFinishedWithDataProvider:(NSPasteboard*)pasteboard
{
}

// NSDraggingSource
// ---

- (NSDragOperation)draggingSession:(NSDraggingSession*)session sourceOperationMaskForDraggingContext:(NSDraggingContext)context
{
    if (IsWindowInvalid(Window))
        return NSDragOperationNone;

    switch(context)
    {
    case NSDraggingContextOutsideApplication:
        return NSDragOperationCopy;
    case NSDraggingContextWithinApplication:
        return NSDragOperationCopy;
    default:
        return NSDragOperationMove;
    }
}

- (void)draggingSession:(NSDraggingSession*)session willBeginAtPoint:(NSPoint)screenPoint
{
    MacImpl::DraggingPosition = GetMousePosition(Window, screenPoint);
}

- (void)draggingSession:(NSDraggingSession*)session movedToPoint:(NSPoint)screenPoint
{
    MacImpl::DraggingPosition = GetMousePosition(Window, screenPoint);
}

- (void)draggingSession:(NSDraggingSession*)session endedAtPoint:(NSPoint)screenPoint operation:(NSDragOperation)operation
{
    MacImpl::DraggingPosition = GetMousePosition(Window, screenPoint);
#if USE_EDITOR
    // Stop background worker once the drag ended
    if (MacImpl::MacDragSession && MacImpl::MacDragSession == session)
        Platform::AtomicStore(&MacImpl::MacDragExitFlag, 1);
#endif
}

@end

DragDropEffect SDLWindow::DoDragDrop(const StringView& data)
{
    NSWindow* window = (NSWindow*)_handle;
    ClipboardDataProviderImpl* clipboardDataProvider = [ClipboardDataProviderImpl alloc];
    clipboardDataProvider->Window = this;

    // Create mouse drag event
    NSEvent* event = [NSEvent
        mouseEventWithType:NSEventTypeLeftMouseDragged
        location:window.mouseLocationOutsideOfEventStream
        modifierFlags:0
        timestamp:NSApp.currentEvent.timestamp
        windowNumber:window.windowNumber
        context:nil
        eventNumber:0
        clickCount:1
        pressure:1.0];

    // Create drag item
    NSPasteboardItem* pasteItem = [NSPasteboardItem new];
    [pasteItem setDataProvider:clipboardDataProvider forTypes:[NSArray arrayWithObjects:NSPasteboardTypeString, nil]];
    NSDraggingItem* dragItem = [[NSDraggingItem alloc] initWithPasteboardWriter:pasteItem];
    [dragItem setDraggingFrame:NSMakeRect(event.locationInWindow.x, event.locationInWindow.y, 100, 100) contents:nil];

    // Start dragging session
    NSDraggingSession* draggingSession = [window.contentView beginDraggingSessionWithItems:[NSArray arrayWithObject:dragItem] event:event source:clipboardDataProvider];
    DragDropEffect result = DragDropEffect::None;

#if USE_EDITOR
    ASSERT(!MacImpl::MacDragSession);
    MacImpl::MacDragSession = draggingSession;
    MacImpl::MacDragExitFlag = 0;
    MacImpl::DraggingData = data;
    MacImpl::DraggingActive = true;
    while (Platform::AtomicRead(&MacImpl::MacDragExitFlag) == 0)
    {
        // The internal event loop will block here during the drag operation,
        // events are processed in the event filter callback instead.
        SDLPlatform::Tick();
        Platform::Sleep(1);
    }
    MacImpl::DraggingActive = false;
    MacImpl::DraggingData.Clear();
    MacImpl::MacDragSession = nullptr;
#endif

    return result;
}

DragDropEffect SDLWindow::DoDragDrop(const StringView& data, const Float2& offset, Window* dragSourceWindow)
{
    Show();
    return DragDropEffect::None;
}

#endif
