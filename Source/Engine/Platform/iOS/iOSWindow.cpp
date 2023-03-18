// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if PLATFORM_IOS

#include "../Window.h"
#include "Engine/Platform/Apple/AppleUtils.h"
#include "Engine/Core/Log.h"
#include "Engine/Input/Input.h"
#include "Engine/Graphics/RenderTask.h"
#include <UIKit/UIKit.h>
#include <QuartzCore/CAMetalLayer.h>

@interface iOSUIWindow : UIWindow
{
}
@end

@implementation iOSUIWindow

@end

@interface iOSUIViewController : UIViewController

@end

@interface iOSUIView : UIView

@property iOSWindow* window;

@end

@implementation iOSUIView

+(Class) layerClass { return [CAMetalLayer class]; }

- (void)setFrame:(CGRect)frame
{
    [super setFrame:frame];
    if (!_window)
        return;
    float scale = [[UIScreen mainScreen] scale];
    _window->CheckForResize((float)frame.size.width * scale, (float)frame.size.height * scale);
}

@end

@implementation iOSUIViewController
{
}

- (BOOL)prefersHomeIndicatorAutoHidden
{
    return YES;
}

- (BOOL)prefersStatusBarHidden
{
    return YES;
}

 -(UIStatusBarAnimation)preferredStatusBarUpdateAnimation
{
    return UIStatusBarAnimationSlide;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
}

@end

iOSWindow::iOSWindow(const CreateWindowSettings& settings)
    : WindowBase(settings)
{
    // Fullscreen by default
	CGRect frame = [[UIScreen mainScreen] bounds];
	float scale = [[UIScreen mainScreen] scale];
    _clientSize = Float2((float)frame.size.width * scale, (float)frame.size.height * scale);

    // Setup view
	_view = [[iOSUIView alloc] initWithFrame:frame];
	iOSUIView* v = (iOSUIView*)_view;
	[v resignFirstResponder];
	[v setNeedsDisplay];
	[v setHidden:NO];
	[v setOpaque:YES];
	v.backgroundColor = [UIColor clearColor];
	v.window = this;
	
	// Setp view controller
	_viewController = [[iOSUIViewController alloc] init];
	iOSUIViewController* vc = (iOSUIViewController*)_viewController;
	[vc setView:v];
	[vc setNeedsUpdateOfHomeIndicatorAutoHidden];
	[vc setNeedsStatusBarAppearanceUpdate];

    // Setup window
	_window = [[iOSUIWindow alloc] initWithFrame:frame];
	iOSUIWindow* w = (iOSUIWindow*)_window;
	[w setRootViewController:vc];
	[w setContentMode:UIViewContentModeScaleToFill];
	[w makeKeyAndVisible];
	[w setBounds:frame];
	w.backgroundColor = [UIColor clearColor];
}

iOSWindow::~iOSWindow()
{
	[(iOSUIWindow*)_window release];
	[(iOSUIView*)_view release];
	[(iOSUIViewController*)_viewController release];

	_window = nullptr;
	_view = nullptr;
	_layer = nullptr;
}

void iOSWindow::CheckForResize(float width, float height)
{
    const Float2 clientSize(width, height);
	if (clientSize != _clientSize)
	{
        _clientSize = clientSize;
		OnResize(width, height);
	}
}

void* iOSWindow::GetNativePtr() const
{
    return _window;
}

void iOSWindow::Show()
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
        _focused = true;

        // Base
        WindowBase::Show();
    }
}

bool iOSWindow::IsClosed() const
{
    return _window != nullptr;
}

bool iOSWindow::IsForegroundWindow() const
{
    return Platform::GetHasFocus() && IsFocused();
}

void iOSWindow::BringToFront(bool force)
{
    Focus();
}

void iOSWindow::SetIsFullscreen(bool isFullscreen)
{
}

void iOSWindow::SetClientBounds(const Rectangle& clientArea)
{
}

void iOSWindow::SetPosition(const Float2& position)
{
}

Float2 iOSWindow::GetPosition() const
{
    return Float2::Zero;
}

Float2 iOSWindow::GetSize() const
{
    return _clientSize;
}

Float2 iOSWindow::GetClientSize() const
{
	return _clientSize;
}

Float2 iOSWindow::ScreenToClient(const Float2& screenPos) const
{
    return screenPos;
}

Float2 iOSWindow::ClientToScreen(const Float2& clientPos) const
{
    return clientPos;
}

void iOSWindow::SetTitle(const StringView& title)
{
    _title = title;
}

#endif
