// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if PLATFORM_IOS

#include "iOSPlatform.h"
#include "iOSWindow.h"
#include "iOSFile.h"
#include "iOSFileSystem.h"
#include "iOSApp.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Platform/Apple/AppleUtils.h"
#include "Engine/Platform/StringUtils.h"
#include "Engine/Platform/MessageBox.h"
#include "Engine/Platform/Window.h"
#include "Engine/Platform/WindowsManager.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Input/Input.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/Globals.h"
#include <UIKit/UIKit.h>
#include <CoreFoundation/CoreFoundation.h>
#include <QuartzCore/CAMetalLayer.h>
#include <sys/utsname.h>

int32 Dpi = 96;
Guid DeviceId;
FlaxView* MainView = nullptr;
FlaxViewController* MainViewController = nullptr;
iOSWindow* MainWindow = nullptr;

struct MessagePipeline
{
    CriticalSection Locker;
    Array<Function<void()>> List;

    void Add(const Function<void()>& func, bool wait)
    {
        Locker.Lock();
        List.Add(func);
        Locker.Unlock();

        // TODO: use atomic counters for more optimized waiting
        while (wait)
        {
            Platform::Sleep(1);
            Locker.Lock();
            wait = List.HasItems();
            Locker.Unlock();
        }
    }

    void Run()
    {
        Locker.Lock();
        for (const auto& func : List)
        {
            func();
        }
        List.Clear();
        Locker.Unlock();
    }
};

MessagePipeline UIThreadPipeline;
MessagePipeline MainThreadPipeline;

@implementation FlaxView

+(Class) layerClass { return [CAMetalLayer class]; }

- (void)setFrame:(CGRect)frame
{
    [super setFrame:frame];
    if (!MainWindow)
        return;
    float scale = [[UIScreen mainScreen] scale];
    float width = (float)frame.size.width * scale;
    float height = (float)frame.size.height * scale;
    iOSPlatform::RunOnMainThread([width, height]() { MainWindow->CheckForResize(width, height); });
}

@end

@implementation FlaxViewController

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

@end

@interface FlaxAppDelegate()

@property(strong, nonatomic) CADisplayLink* displayLink;

@end

@implementation FlaxAppDelegate

-(void)MainThreadMain:(NSDictionary*)launchOptions
{
    // Run engine on a separate game thread
    Engine::Main(TEXT(""));
}

-(void)UIThreadMain
{
    // Invoke callbacks
    UIThreadPipeline.Run();
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    // Create window
	CGRect frame = [[UIScreen mainScreen] bounds];
    self.window = [[UIWindow alloc] initWithFrame:frame];

    // Create view
    self.view = [[FlaxView alloc] initWithFrame:frame];
	[self.view resignFirstResponder];
	[self.view setNeedsDisplay];
	[self.view setHidden:NO];
	[self.view setOpaque:YES];
	self.view.backgroundColor = [UIColor clearColor];
    MainView = self.view;

    // Create view controller
    self.viewController = [[FlaxViewController alloc] init];
    [self.viewController setView:self.view];
	[self.viewController setNeedsUpdateOfHomeIndicatorAutoHidden];
	[self.viewController setNeedsStatusBarAppearanceUpdate];
    MainViewController = self.viewController;

    // Create navigation controller
    UINavigationController* navController = [[UINavigationController alloc] initWithRootViewController:self.viewController];
    [self.window setRootViewController:navController];
    [self.window makeKeyAndVisible];

    // Create UI thread update callback
    self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(UIThreadMain)];
    self.displayLink.preferredFramesPerSecond = 30;
    [self.displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];

    // Run engine on a separate main thread
	NSThread* mainThread = [[NSThread alloc] initWithTarget:self selector:@selector(MainThreadMain:) object:launchOptions];
#if BUILD_DEBUG
    const int32 mainThreadStackSize = 4 * 1024 * 1024; // 4 MB
#else
    const int32 mainThreadStackSize = 2 * 1024 * 1024; // 2 MB
#endif
	[mainThread setStackSize:mainThreadStackSize];
	[mainThread start];

    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and invalidate graphics rendering callbacks. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    // Called as part of the transition from the background to the active state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

@end

DialogResult MessageBox::Show(Window* parent, const StringView& text, const StringView& caption, MessageBoxButtons buttons, MessageBoxIcon icon)
{
	NSString* title = (NSString*)AppleUtils::ToString(caption);
	NSString* message = (NSString*)AppleUtils::ToString(text);
	UIAlertController* alert = [UIAlertController alertControllerWithTitle:title message:message preferredStyle:UIAlertControllerStyleAlert];
	UIAlertAction* button = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleCancel handler:^(id){ }];
	[alert addAction:button];
    [MainViewController presentViewController:alert animated:YES completion:nil];
    return DialogResult::OK;
}

iOSWindow::iOSWindow(const CreateWindowSettings& settings)
    : WindowBase(settings)
{
 	CGRect frame = [[UIScreen mainScreen] bounds];
	float scale = [[UIScreen mainScreen] scale];
    _clientSize = Float2((float)frame.size.width * scale, (float)frame.size.height * scale);

    MainWindow = this;
}

iOSWindow::~iOSWindow()
{
    MainWindow = nullptr;
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
    return MainView;
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
    return false;
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

// Fallback to file placed side-by-side with application
#define IOS_FALLBACK_PATH(path) (Globals::ProjectFolder / StringUtils::GetFileName(path))

iOSFile* iOSFile::Open(const StringView& path, FileMode mode, FileAccess access, FileShare share)
{
    iOSFile* file;
    if (mode == FileMode::OpenExisting && !AppleFileSystem::FileExists(path))
        file = (iOSFile*)UnixFile::Open(IOS_FALLBACK_PATH(path), mode, access, share);
    else
        file = (iOSFile*)UnixFile::Open(path, mode, access, share);
    return file;
}

bool iOSFileSystem::FileExists(const StringView& path)
{
    return AppleFileSystem::FileExists(path) || AppleFileSystem::FileExists(IOS_FALLBACK_PATH(path));
}

uint64 iOSFileSystem::GetFileSize(const StringView& path)
{
    if (AppleFileSystem::FileExists(path))
        return AppleFileSystem::GetFileSize(path);
    return AppleFileSystem::GetFileSize(IOS_FALLBACK_PATH(path));
}

bool iOSFileSystem::IsReadOnly(const StringView& path)
{
    if (AppleFileSystem::FileExists(path))
        return AppleFileSystem::IsReadOnly(path);
    return AppleFileSystem::IsReadOnly(IOS_FALLBACK_PATH(path));
}

#undef IOS_FALLBACK_PATH

void iOSPlatform::RunOnUIThread(const Function<void()>& func, bool wait)
{
    UIThreadPipeline.Add(func, wait);
}

void iOSPlatform::RunOnMainThread(const Function<void()>& func, bool wait)
{
    MainThreadPipeline.Add(func, wait);
}

bool iOSPlatform::Init()
{
    if (ApplePlatform::Init())
        return true;

    ScreenScale = [[UIScreen mainScreen] scale];
    CustomDpiScale *= ScreenScale;
    Dpi = 72; // TODO: calculate screen dpi (probably hardcoded map for iPhone model)

	// Get device identifier
    NSString* uuid = [UIDevice currentDevice].identifierForVendor.UUIDString;
    String uuidStr = AppleUtils::ToString((CFStringRef)uuid);
    Guid::Parse(uuidStr, DeviceId);

    // TODO: add Gamepad for vibrations usability

    return false;
}

void iOSPlatform::LogInfo()
{
    ApplePlatform::LogInfo();

	struct utsname systemInfo;
	uname(&systemInfo);
    NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
    LOG(Info, "{3}, iOS {0}.{1}.{2}", version.majorVersion, version.minorVersion, version.patchVersion, String(systemInfo.machine));
}

void iOSPlatform::Tick()
{
    // Invoke callbacks
    MainThreadPipeline.Run();

    ApplePlatform::Tick();
}

int32 iOSPlatform::GetDpi()
{
    return Dpi;
}

Guid iOSPlatform::GetUniqueDeviceId()
{
    return DeviceId;
}

String iOSPlatform::GetComputerName()
{
    return TEXT("iPhone");
}

Float2 iOSPlatform::GetDesktopSize()
{
    CGRect frame = [[UIScreen mainScreen] bounds];
	float scale = [[UIScreen mainScreen] scale];
    return Float2((float)frame.size.width * scale, (float)frame.size.height * scale);
}

String iOSPlatform::GetMainDirectory()
{
    String path = StringUtils::GetDirectoryName(GetExecutableFilePath());
    if (path.EndsWith(TEXT("/Contents/iOS")))
    {
        // If running from executable in a package, go up to the Contents
        path = StringUtils::GetDirectoryName(path);
    }
    return path;
}

Window* iOSPlatform::CreateWindow(const CreateWindowSettings& settings)
{
    return New<iOSWindow>(settings);
}

#endif
