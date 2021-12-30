// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if PLATFORM_MAC

#include "../Window.h"
#include "MacUtils.h"
#include "Engine/Graphics/RenderTask.h"
#include <Cocoa/Cocoa.h>

@interface MacWindowImpl : NSWindow <NSWindowDelegate>
{
    MacWindow* Window;
}

- (void)setWindow:(MacWindow*)window;

@end

@implementation MacWindowImpl

- (void)windowWillClose:(NSNotification*)notification
{
    [self setDelegate: nil];
    Window->Close(ClosingReason::User);
}

- (void)setWindow:(MacWindow*)window
{
    Window = window;
}

@end

MacWindow::MacWindow(const CreateWindowSettings& settings)
    : WindowBase(settings)
{
    _clientSize = Vector2(settings.Size.X, settings.Size.Y);
    Vector2 pos = MacUtils::PosToCoca(settings.Position);
    NSRect frame = NSMakeRect(pos.X, pos.Y - settings.Size.Y, settings.Size.X, settings.Size.Y);
    NSUInteger styleMask = NSWindowStyleMaskClosable;
    if (settings.IsRegularWindow)
    {
        styleMask |= NSWindowStyleMaskTitled | NSWindowStyleMaskFullSizeContentView;
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

    MacWindowImpl* window = [[MacWindowImpl alloc] initWithContentRect:frame
        styleMask:(styleMask)
        backing:NSBackingStoreBuffered
        defer:NO];
    window.title = (__bridge NSString*)MacUtils::ToString(settings.Title);
    [window setWindow:this];
    [window setReleasedWhenClosed:NO];
    [window setMinSize:NSMakeSize(settings.MinimumSize.X, settings.MinimumSize.Y)];
    [window setMaxSize:NSMakeSize(settings.MaximumSize.X, settings.MaximumSize.Y)];
    [window setOpaque:!settings.SupportsTransparency];
    [window setDelegate:window];
    _window = window;

    // TODO: impl Parent for MacWindow
    // TODO: impl StartPosition for MacWindow
    // TODO: impl Fullscreen for MacWindow
    // TODO: impl ShowInTaskbar for MacWindow
    // TODO: impl ActivateWhenFirstShown for MacWindow
    // TODO: impl AllowInput for MacWindow
    // TODO: impl AllowDragAndDrop for MacWindow
    // TODO: impl IsTopmost for MacWindow
}

MacWindow::~MacWindow()
{
    NSWindow* window = (NSWindow*)_window;
    [window close];
    [window release];
    _window = nullptr;
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
    return Platform::GetHasFocus();
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

void MacWindow::SetOpacity(float opacity)
{
    NSWindow* window = (NSWindow*)_window;
    [window setAlphaValue:opacity];
}

void MacWindow::Focus()
{
    NSWindow* window = (NSWindow*)_window;
    [window makeKeyAndOrderFront:window];
}

void MacWindow::SetTitle(const StringView& title)
{
    _title = title;
    NSWindow* window = (NSWindow*)_window;
    [window setTitle:(__bridge NSString*)MacUtils::ToString(_title)];
}

#endif
