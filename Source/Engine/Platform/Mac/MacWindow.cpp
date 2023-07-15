// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if PLATFORM_MAC

#include "../Window.h"
#include "Engine/Platform/Apple/AppleUtils.h"
#include "Engine/Platform/IGuiData.h"
#include "Engine/Core/Log.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Input/Keyboard.h"
#include "Engine/Graphics/RenderTask.h"
#include <Cocoa/Cocoa.h>
#include <AppKit/AppKit.h>
#include <QuartzCore/CAMetalLayer.h>

KeyboardKeys GetKey(NSEvent* event)
{
    switch ([event keyCode])
    {
    case 0x00: return KeyboardKeys::A;
    case 0x01: return KeyboardKeys::S;
    case 0x02: return KeyboardKeys::D;
    case 0x03: return KeyboardKeys::F;
    case 0x04: return KeyboardKeys::H;
    case 0x05: return KeyboardKeys::G;
    case 0x06: return KeyboardKeys::Z;
    case 0x07: return KeyboardKeys::X;
    case 0x08: return KeyboardKeys::C;
    case 0x09: return KeyboardKeys::V;
    case 0x0A: return KeyboardKeys::W;
    case 0x0B: return KeyboardKeys::B;
    case 0x0C: return KeyboardKeys::Q;
    case 0x0D: return KeyboardKeys::W;
    case 0x0E: return KeyboardKeys::E;
    case 0x0F: return KeyboardKeys::R;

    case 0x10: return KeyboardKeys::Y;
    case 0x11: return KeyboardKeys::T;
    case 0x12: return KeyboardKeys::Alpha1;
    case 0x13: return KeyboardKeys::Alpha2;
    case 0x14: return KeyboardKeys::Alpha3;
    case 0x15: return KeyboardKeys::Alpha4;
    case 0x16: return KeyboardKeys::Alpha6;
    case 0x17: return KeyboardKeys::Alpha5;
    case 0x18: return KeyboardKeys::Plus;
    case 0x19: return KeyboardKeys::Alpha9;
    case 0x1A: return KeyboardKeys::Alpha7;
    case 0x1B: return KeyboardKeys::Minus;
    case 0x1C: return KeyboardKeys::Alpha8;
    case 0x1D: return KeyboardKeys::Alpha0;
    case 0x1E: return KeyboardKeys::RightBracket;
    case 0x1F: return KeyboardKeys::O;

    case 0x20: return KeyboardKeys::U;
    case 0x21: return KeyboardKeys::LeftBracket;
    case 0x22: return KeyboardKeys::I;
    case 0x23: return KeyboardKeys::P;
    case 0x24: return KeyboardKeys::Return;
    case 0x25: return KeyboardKeys::L;
    case 0x26: return KeyboardKeys::J;
    case 0x27: return KeyboardKeys::Quote;
    case 0x28: return KeyboardKeys::K;
    case 0x29: return KeyboardKeys::Colon;
    case 0x2A: return KeyboardKeys::Backslash;
    case 0x2B: return KeyboardKeys::Comma;
    case 0x2C: return KeyboardKeys::Slash;
    case 0x2D: return KeyboardKeys::N;
    case 0x2E: return KeyboardKeys::M;
    case 0x2F: return KeyboardKeys::Period;

    case 0x30: return KeyboardKeys::Tab;
    case 0x31: return KeyboardKeys::Spacebar;
    case 0x32: return KeyboardKeys::BackQuote;
    case 0x33: return KeyboardKeys::Delete;
    //case 0x34:
    case 0x35: return KeyboardKeys::Escape;
    //case 0x36:
    //case 0x37: Command
    case 0x38: return KeyboardKeys::Shift;
    case 0x39: return KeyboardKeys::Capital;
    case 0x3A: return KeyboardKeys::Alt;
    case 0x3B: return KeyboardKeys::Control;
    case 0x3C: return KeyboardKeys::Shift;
    case 0x3D: return KeyboardKeys::Alt;
    case 0x3E: return KeyboardKeys::Control;
    //case 0x3F: Function

    case 0x40: return KeyboardKeys::F17;
    case 0x41: return KeyboardKeys::NumpadDecimal;
    //case 0x42:
    case 0x43: return KeyboardKeys::NumpadMultiply;
    //case 0x44:
    case 0x45: return KeyboardKeys::NumpadAdd;
    //case 0x46:
    //case 0x47: KeypadClear
    case 0x48: return KeyboardKeys::VolumeUp;
    case 0x49: return KeyboardKeys::VolumeDown;
    case 0x4A: return KeyboardKeys::VolumeMute;
    case 0x4B: return KeyboardKeys::NumpadDivide;
    case 0x4C: return KeyboardKeys::Return;
    //case 0x4D:
    case 0x4E: return KeyboardKeys::NumpadSubtract;
    case 0x4F: return KeyboardKeys::F18;

    case 0x50: return KeyboardKeys::F19;
    //case 0x51: NumpadEquals
    case 0x52: return KeyboardKeys::Numpad0;
    case 0x53: return KeyboardKeys::Numpad1;
    case 0x54: return KeyboardKeys::Numpad2;
    case 0x55: return KeyboardKeys::Numpad3;
    case 0x56: return KeyboardKeys::Numpad4;
    case 0x57: return KeyboardKeys::Numpad5;
    case 0x58: return KeyboardKeys::Numpad6;
    case 0x59: return KeyboardKeys::Numpad7;
    case 0x5A: return KeyboardKeys::F20;
    case 0x5B: return KeyboardKeys::Numpad8;
    case 0x5C: return KeyboardKeys::Numpad9;
    //case 0x5D: Yen
    //case 0x5E: Underscore
    //case 0x5F: KeypadComma

    case 0x60: return KeyboardKeys::F5;
    case 0x61: return KeyboardKeys::F6;
    case 0x62: return KeyboardKeys::F7;
    case 0x63: return KeyboardKeys::F3;
    case 0x64: return KeyboardKeys::F8;
    case 0x65: return KeyboardKeys::F9;
    //case 0x66: Eisu
    case 0x67: return KeyboardKeys::F11;
    case 0x68: return KeyboardKeys::Kana;
    case 0x69: return KeyboardKeys::F13;
    case 0x6A: return KeyboardKeys::F16;
    case 0x6B: return KeyboardKeys::F14;
    //case 0x6C:
    case 0x6D: return KeyboardKeys::F10;
    //case 0x6E:
    case 0x6F: return KeyboardKeys::F12;

    //case 0x70:
    case 0x71: return KeyboardKeys::F15;
    case 0x72: return KeyboardKeys::Help;
    case 0x73: return KeyboardKeys::Home;
    case 0x74: return KeyboardKeys::PageUp;
    case 0x75: return KeyboardKeys::Delete;
    case 0x76: return KeyboardKeys::F4;
    case 0x77: return KeyboardKeys::End;
    case 0x78: return KeyboardKeys::F2;
    case 0x79: return KeyboardKeys::PageDown;
    case 0x7A: return KeyboardKeys::F1;
    case 0x7B: return KeyboardKeys::ArrowLeft;
    case 0x7C: return KeyboardKeys::ArrowRight;
    case 0x7D: return KeyboardKeys::ArrowDown;
    case 0x7E: return KeyboardKeys::ArrowUp;
    //case 0x7F:

    default:
        return KeyboardKeys::None;
    }
}

Float2 GetWindowTitleSize(const MacWindow* window)
{
    Float2 size = Float2::Zero;
    if (window->GetSettings().HasBorder)
    {
        NSRect frameStart = [(NSWindow*)window->GetNativePtr() frameRectForContentRect:NSMakeRect(0, 0, 0, 0)];
        size.Y = frameStart.size.height;
    }
    return size * MacPlatform::ScreenScale;
}

Float2 GetMousePosition(MacWindow* window, NSEvent* event)
{
    NSRect frame = [(NSWindow*)window->GetNativePtr() frame];
    NSPoint point = [event locationInWindow];
    return Float2(point.x, frame.size.height - point.y) * MacPlatform::ScreenScale - GetWindowTitleSize(window);
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

void GetDragDropData(const MacWindow* window, id<NSDraggingInfo> sender, Float2& mousePos, MacDropData& dropData)
{
    NSRect frame = [(NSWindow*)window->GetNativePtr() frame];
    NSPoint point = [sender draggingLocation];
    Float2 titleSize = GetWindowTitleSize(window);
    mousePos = Float2(point.x, frame.size.height - point.y) - titleSize;
    NSPasteboard* pasteboard = [sender draggingPasteboard];
    if ([[pasteboard types] containsObject:NSPasteboardTypeString])
    {
        dropData.CurrentType = IGuiData::Type::Text;
        dropData.AsText = AppleUtils::ToString((CFStringRef)[pasteboard stringForType:NSPasteboardTypeString]);
    }
    else
    {
        dropData.CurrentType = IGuiData::Type::Files;
        NSArray* files = [pasteboard readObjectsForClasses:@[[NSURL class]] options:nil];
        for (int32 i = 0; i < [files count]; i++)
        {
            NSString* url = [[files objectAtIndex:i] path];
            NSString* file = [NSURL URLWithString:url].path;
            dropData.AsFiles.Add(AppleUtils::ToString((CFStringRef)file));
        }
    }
}

NSDragOperation GetDragDropOperation(DragDropEffect dragDropEffect)
{
    NSDragOperation result = NSDragOperationCopy;
    switch (dragDropEffect)
    {
    case DragDropEffect::None:
        //result = NSDragOperationNone;
        break;
    case DragDropEffect::Copy:
        result = NSDragOperationCopy;
        break;
    case DragDropEffect::Move:
        result = NSDragOperationMove;
        break;
    case DragDropEffect::Link:
        result = NSDragOperationLink;
        break;
    }
    return result;
}

@interface MacWindowImpl : NSWindow <NSWindowDelegate>
{
    MacWindow* Window;
}

- (void)setWindow:(MacWindow*)window;

@end

@implementation MacWindowImpl

- (BOOL)canBecomeKeyWindow
{
    if (Window && !Window->GetSettings().AllowInput)
    {
        return NO;
    }
    return YES;
}

- (void)windowDidBecomeKey:(NSNotification*)notification
{
	// Handle resizing to be sure that content has valid size when window was resized
	[self windowDidResize:notification];

    Window->OnGotFocus();
}

- (void)windowDidResignKey:(NSNotification*)notification
{
    Window->OnLostFocus();
}

- (void)windowWillClose:(NSNotification*)notification
{
    [self setDelegate: nil];
    Window->Close(ClosingReason::User);
}

static void ConvertNSRect(NSScreen *screen, NSRect *r)
{
    r->origin.y = CGDisplayPixelsHigh(kCGDirectMainDisplay) - r->origin.y - r->size.height;
}

- (void)windowDidResize:(NSNotification*)notification
{
    NSView* view = [self contentView];
    const float screenScale = MacPlatform::ScreenScale;
    NSWindow* nswindow = (NSWindow*)Window->GetNativePtr();
    NSRect rect = [nswindow contentRectForFrameRect:[nswindow frame]];
    ConvertNSRect([nswindow screen], &rect);

    // Rescale contents
	CALayer* layer = [view layer];
	if (layer)
		layer.contentsScale = screenScale;

    // Resize window
    Window->CheckForResize((float)rect.size.width * screenScale, (float)rect.size.height * screenScale);
}

- (void)setWindow:(MacWindow*)window
{
    Window = window;
}

@end

@interface MacViewImpl : NSView
{
    MacWindow* Window;
    NSTrackingArea* TrackingArea;
    bool IsMouseOver;
}

- (void)setWindow:(MacWindow*)window;
- (CALayer*)makeBackingLayer;
- (BOOL)wantsUpdateLayer;

@end

@implementation MacViewImpl

- (void)dealloc
{
    if (TrackingArea != nil)
    {
        [self removeTrackingArea:TrackingArea];
        [TrackingArea release];
    }
    [super dealloc];
}

- (void)setWindow:(MacWindow*)window
{
    Window = window;
    TrackingArea = nil;
    IsMouseOver = false;
}

- (CALayer*)makeBackingLayer
{
    return [[CAMetalLayer class] layer];
}

- (BOOL)wantsUpdateLayer
{
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)updateTrackingAreas
{
    [super updateTrackingAreas];

    if (TrackingArea != nil)
    {
        [self removeTrackingArea:TrackingArea];
        [TrackingArea release];
    }

    NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways;
    TrackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds] options:options owner:self userInfo:nil];
    [self addTrackingArea:TrackingArea];
}

- (void)keyDown:(NSEvent*)event
{
    KeyboardKeys key = GetKey(event);
    if (key != KeyboardKeys::None)
	    Input::Keyboard->OnKeyDown(key);

	// Send a text input event
    switch (key)
    {
        // Ignore text from special keys
    case KeyboardKeys::Delete:
    case KeyboardKeys::Backspace:
    case KeyboardKeys::ArrowLeft:
    case KeyboardKeys::ArrowRight:
    case KeyboardKeys::ArrowUp:
    case KeyboardKeys::ArrowDown:
        return;
    }
    NSString* text = [event characters];
    int32 length = [text length];
    if (length > 0)
    {
        unichar buffer[16];
        if (length >= 16)
            length = 15;
        [text getCharacters:buffer range:NSMakeRange(0, length)];
        Input::Keyboard->OnCharInput((Char)buffer[0]);
    }
}

- (void)keyUp:(NSEvent*)event
{
    KeyboardKeys key = GetKey(event);
    if (key != KeyboardKeys::None)
	    Input::Keyboard->OnKeyUp(key);
}

- (void)scrollWheel:(NSEvent*)event
{
	Float2 mousePos = GetMousePosition(Window, event);
    double deltaX = [event scrollingDeltaX];
    double deltaY = [event scrollingDeltaY];
    if ([event hasPreciseScrollingDeltas])
    {
        deltaX *= 0.03;
        deltaY *= 0.03;
    }
    // TODO: add support for scroll delta as Float2 in the engine
    Input::Mouse->OnMouseWheel(Window->ClientToScreen(mousePos), deltaY, Window);
}

- (void)mouseMoved:(NSEvent*)event
{
	Float2 mousePos = GetMousePosition(Window, event);
    if (!Window->IsMouseTracking() && !IsMouseOver)
        return;
	Input::Mouse->OnMouseMove(Window->ClientToScreen(mousePos), Window);
}

- (void)mouseEntered:(NSEvent*)event
{
    IsMouseOver = true;
    Window->SetIsMouseOver(true);
}

- (void)mouseExited:(NSEvent*)event
{
    IsMouseOver = false;
    Window->SetIsMouseOver(false);
}

- (void)mouseDown:(NSEvent*)event
{
	Float2 mousePos = GetMousePosition(Window, event);
    MouseButton mouseButton = MouseButton::Left;
    if ([event clickCount] == 2)
        Input::Mouse->OnMouseDoubleClick(Window->ClientToScreen(mousePos), mouseButton, Window);
    else
	    Input::Mouse->OnMouseDown(Window->ClientToScreen(mousePos), mouseButton, Window);
}

- (void)mouseDragged:(NSEvent*)event
{
    [self mouseMoved:event];
}

- (void)mouseUp:(NSEvent*)event
{
	Float2 mousePos = GetMousePosition(Window, event);
    MouseButton mouseButton = MouseButton::Left;
    Input::Mouse->OnMouseUp(Window->ClientToScreen(mousePos), mouseButton, Window);
}

- (void)rightMouseDown:(NSEvent*)event
{
	Float2 mousePos = GetMousePosition(Window, event);
    MouseButton mouseButton = MouseButton::Right;
    if ([event clickCount] == 2)
        Input::Mouse->OnMouseDoubleClick(Window->ClientToScreen(mousePos), mouseButton, Window);
    else
	    Input::Mouse->OnMouseDown(Window->ClientToScreen(mousePos), mouseButton, Window);
}

- (void)rightMouseDragged:(NSEvent*)event
{
    [self mouseMoved:event];
}

- (void)rightMouseUp:(NSEvent*)event
{
	Float2 mousePos = GetMousePosition(Window, event);
    MouseButton mouseButton = MouseButton::Right;
    Input::Mouse->OnMouseUp(Window->ClientToScreen(mousePos), mouseButton, Window);
}

- (void)otherMouseDown:(NSEvent*)event
{
	Float2 mousePos = GetMousePosition(Window, event);
    MouseButton mouseButton;
    switch ([event buttonNumber])
    {
    case 2:
        mouseButton = MouseButton::Middle;
        break;
    case 3:
        mouseButton = MouseButton::Extended1;
        break;
    case 4:
        mouseButton = MouseButton::Extended2;
        break;
    default:
        return;
    }
    if ([event clickCount] == 2)
        Input::Mouse->OnMouseDoubleClick(Window->ClientToScreen(mousePos), mouseButton, Window);
    else
	    Input::Mouse->OnMouseDown(Window->ClientToScreen(mousePos), mouseButton, Window);
}

- (void)otherMouseDragged:(NSEvent*)event
{
    [self mouseMoved:event];
}

- (void)otherMouseUp:(NSEvent*)event
{
	Float2 mousePos = GetMousePosition(Window, event);
    MouseButton mouseButton;
    switch ([event buttonNumber])
    {
    case 2:
        mouseButton = MouseButton::Middle;
        break;
    case 3:
        mouseButton = MouseButton::Extended1;
        break;
    case 4:
        mouseButton = MouseButton::Extended2;
        break;
    default:
        return;
    }
    Input::Mouse->OnMouseUp(Window->ClientToScreen(mousePos), mouseButton, Window);
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender
{
    Float2 mousePos;
    MacDropData dropData;
    GetDragDropData(Window, sender, mousePos, dropData);
    DragDropEffect dragDropEffect = DragDropEffect::None;
    Window->OnDragEnter(&dropData, mousePos, dragDropEffect);
    return GetDragDropOperation(dragDropEffect);
}

- (BOOL)wantsPeriodicDraggingUpdates:(id<NSDraggingInfo>)sender
{
    return YES;
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender
{
    Float2 mousePos;
    MacDropData dropData;
    GetDragDropData(Window, sender, mousePos, dropData);
    DragDropEffect dragDropEffect = DragDropEffect::None;
    Window->OnDragOver(&dropData, mousePos, dragDropEffect);
    return GetDragDropOperation(dragDropEffect);
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender
{
    Float2 mousePos;
    MacDropData dropData;
    GetDragDropData(Window, sender, mousePos, dropData);
    DragDropEffect dragDropEffect = DragDropEffect::None;
    Window->OnDragDrop(&dropData, mousePos, dragDropEffect);
    return NO;
}

- (void)draggingExited:(id<NSDraggingInfo>)sender
{
    Window->OnDragLeave();
}

@end

MacWindow::MacWindow(const CreateWindowSettings& settings)
    : WindowBase(settings)
{
    _clientSize = Float2(settings.Size.X, settings.Size.Y);
    Float2 pos = AppleUtils::PosToCoca(settings.Position);
    NSRect frame = NSMakeRect(pos.X, pos.Y - settings.Size.Y, settings.Size.X, settings.Size.Y);
    NSUInteger styleMask = NSWindowStyleMaskClosable;
    if (settings.IsRegularWindow)
    {
        styleMask |= NSWindowStyleMaskTitled;
        if (settings.AllowMinimize)
            styleMask |= NSWindowStyleMaskMiniaturizable;
        if (settings.HasSizingFrame || settings.AllowMaximize)
            styleMask |= NSWindowStyleMaskResizable;
    }
    else
    {
        styleMask |= NSWindowStyleMaskBorderless;
    }
    if (settings.HasBorder)
    {
        styleMask |= NSWindowStyleMaskTitled;
        styleMask &= ~NSWindowStyleMaskFullSizeContentView;
    }

    const float screenScale = MacPlatform::ScreenScale;
    frame.origin.x /= screenScale;
    frame.origin.y /= screenScale;
    frame.size.width /= screenScale;
    frame.size.height /= screenScale;

    MacWindowImpl* window = [[MacWindowImpl alloc] initWithContentRect:frame
        styleMask:(styleMask)
        backing:NSBackingStoreBuffered
        defer:NO];
    MacViewImpl* view = [[MacViewImpl alloc] init];
    view.wantsLayer = YES;
    [view setWindow:this];
    window.title = (__bridge NSString*)AppleUtils::ToString(settings.Title);
    [window setWindow:this];
    [window setReleasedWhenClosed:NO];
    [window setMinSize:NSMakeSize(settings.MinimumSize.X, settings.MinimumSize.Y)];
    [window setMaxSize:NSMakeSize(settings.MaximumSize.X, settings.MaximumSize.Y)];
    [window setOpaque:!settings.SupportsTransparency];
    [window setContentView:view];
    [window setAcceptsMouseMovedEvents:YES];
    [window setDelegate:window];
    _window = window;
    if (settings.AllowDragAndDrop)
    {
        [view registerForDraggedTypes:@[NSPasteboardTypeFileURL, NSPasteboardTypeString]];
    }

    // Rescale contents
	CALayer* layer = [view layer];
	if (layer)
		layer.contentsScale = screenScale;

    // TODO: impl Parent for MacWindow
    // TODO: impl StartPosition for MacWindow
    // TODO: impl Fullscreen for MacWindow
    // TODO: impl ShowInTaskbar for MacWindow
    // TODO: impl IsTopmost for MacWindow
}

MacWindow::~MacWindow()
{
    NSWindow* window = (NSWindow*)_window;
    [window close];
    [window release];
    _window = nullptr;
}

void MacWindow::CheckForResize(float width, float height)
{
    const Float2 clientSize(width, height);
	if (clientSize != _clientSize)
	{
        _clientSize = clientSize;
		OnResize(width, height);
	}
}

void MacWindow::SetIsMouseOver(bool value)
{
    if (_isMouseOver == value)
        return;
    _isMouseOver = value;
    CursorType cursor = _cursor;
    if (value)
    {
        // Refresh cursor typet
        SetCursor(CursorType::Default);
        SetCursor(cursor);
        
    }
    else
    {
	    Input::Mouse->OnMouseLeave(this);
        SetCursor(CursorType::Default);
        _cursor = cursor;
    }
}

void* MacWindow::GetNativePtr() const
{
    return _window;
}

void MacWindow::Show()
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
        NSWindow* window = (NSWindow*)_window;
        [window makeKeyAndOrderFront:window];
        if (_settings.ActivateWhenFirstShown)
            [NSApp activateIgnoringOtherApps:YES];
        _focused = true;

        // Base
        WindowBase::Show();
    }
}

void MacWindow::Hide()
{
    if (_visible)
    {
        SetCursor(CursorType::Default);

        // Hide
        NSWindow* window = (NSWindow*)_window;
        [window orderOut:nil];

        // Base
        WindowBase::Hide();
    }
}

void MacWindow::Minimize()
{
    if (!_settings.AllowMinimize)
        return;
    NSWindow* window = (NSWindow*)_window;
    if (!window)
        return;
    if (!window.miniaturized)
        [window miniaturize:nil];
}

void MacWindow::Maximize()
{
    if (!_settings.AllowMaximize)
        return;
    NSWindow* window = (NSWindow*)_window;
    if (!window.zoomed)
        [window zoom:nil];
}

void MacWindow::Restore()
{
    NSWindow* window = (NSWindow*)_window;
    if (window.miniaturized)
        [window deminiaturize:nil];
    else if (window.zoomed)
        [window zoom:nil];
}

bool MacWindow::IsClosed() const
{
    return _window != nullptr;
}

bool MacWindow::IsForegroundWindow() const
{
    return Platform::GetHasFocus() && IsFocused();
}

void MacWindow::BringToFront(bool force)
{
    Focus();
    [NSApp activateIgnoringOtherApps: NO];
}

void MacWindow::SetIsFullscreen(bool isFullscreen)
{
    // TODO: fullscreen mode on Mac
}

void MacWindow::SetClientBounds(const Rectangle& clientArea)
{
    NSWindow* window = (NSWindow*)_window;
    if (!window)
        return;
    NSRect oldRect = [window frame];
    NSRect newRect = NSMakeRect(0, 0, clientArea.Size.X, clientArea.Size.Y);
    newRect = [window frameRectForContentRect:newRect];

    //newRect.origin.x = oldRect.origin.x;
    //newRect.origin.y = NSMaxY(oldRect) - newRect.size.height;

    Float2 pos = AppleUtils::PosToCoca(clientArea.Location);
    Float2 titleSize = GetWindowTitleSize(this);
    newRect.origin.x = pos.X + titleSize.X;
    newRect.origin.y = pos.Y - newRect.size.height + titleSize.Y;

    [window setFrame:newRect display:YES];
}

void MacWindow::SetPosition(const Float2& position)
{
    NSWindow* window = (NSWindow*)_window;
    if (!window)
        return;
    Float2 pos = AppleUtils::PosToCoca(position) / MacPlatform::ScreenScale;
    NSRect rect = [window frame];
    [window setFrameOrigin:NSMakePoint(pos.X, pos.Y - rect.size.height)];
}

Float2 MacWindow::GetPosition() const
{
    NSWindow* window = (NSWindow*)_window;
    if (!window)
        return Float2::Zero;
    NSRect rect = [window frame];
    return AppleUtils::CocaToPos(Float2(rect.origin.x, rect.origin.y + rect.size.height) * MacPlatform::ScreenScale);
}

Float2 MacWindow::GetSize() const
{
    NSWindow* window = (NSWindow*)_window;
    if (!window)
        return Float2::Zero;
    NSRect rect = [window frame];
    return Float2(rect.size.width, rect.size.height) * MacPlatform::ScreenScale;
}

Float2 MacWindow::GetClientSize() const
{
	return _clientSize;
}

Float2 MacWindow::ScreenToClient(const Float2& screenPos) const
{
    NSWindow* window = (NSWindow*)_window;
    if (!window)
        return screenPos;
    Float2 titleSize = GetWindowTitleSize(this);
    return screenPos - GetPosition() - titleSize;
}

Float2 MacWindow::ClientToScreen(const Float2& clientPos) const
{
    NSWindow* window = (NSWindow*)_window;
    if (!window)
        return clientPos;
    Float2 titleSize = GetWindowTitleSize(this);
    return GetPosition() + titleSize + clientPos;
}

void MacWindow::FlashWindow()
{
    NSWindow* window = (NSWindow*)_window;
    if (!window)
        return;
    [NSApp requestUserAttention:NSInformationalRequest];
}

void MacWindow::SetOpacity(float opacity)
{
    NSWindow* window = (NSWindow*)_window;
    if (!window)
        return;
    [window setAlphaValue:opacity];
}

void MacWindow::Focus()
{
    NSWindow* window = (NSWindow*)_window;
    if (!window)
        return;
    [window makeKeyAndOrderFront:window];
}

void MacWindow::SetTitle(const StringView& title)
{
    _title = title;
    NSWindow* window = (NSWindow*)_window;
    if (!window)
        return;
    [window setTitle:(__bridge NSString*)AppleUtils::ToString(_title)];
}

DragDropEffect MacWindow::DoDragDrop(const StringView& data)
{
    // TODO: implement using beginDraggingSession and NSDraggingSource
    return DragDropEffect::None;
}

void MacWindow::SetCursor(CursorType type)
{
    CursorType prev = _cursor;
    if (prev == type)
        return;
	WindowBase::SetCursor(type);
    //if (!_isMouseOver)
    //    return;
    NSCursor* cursor = nullptr;
    switch (type)
    {
    case CursorType::Cross:
        cursor = [NSCursor crosshairCursor];
        break;
    case CursorType::Hand:
        cursor = [NSCursor pointingHandCursor];
        break;
    case CursorType::IBeam:
        cursor = [NSCursor IBeamCursor];
        break;
    case CursorType::No:
        cursor = [NSCursor operationNotAllowedCursor];
        break;
    case CursorType::SizeAll:
    case CursorType::SizeNESW:
    case CursorType::SizeNWSE:
        cursor = [NSCursor resizeUpDownCursor];
        break;
    case CursorType::SizeNS:
        cursor = [NSCursor resizeUpDownCursor];
        break;
    case CursorType::SizeWE:
        cursor = [NSCursor resizeLeftRightCursor];
        break;
    case CursorType::Hidden:
        [NSCursor hide];
        return;
    default:
        cursor = [NSCursor arrowCursor];
        break;
    }
    if (cursor)
    {
        if (prev == CursorType::Hidden)
        {
            [NSCursor unhide];
        }
        [cursor set];
    }
}

#endif
