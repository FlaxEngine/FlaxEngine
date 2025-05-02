// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_IOS

#include "iOSPlatform.h"
#include "iOSWindow.h"
#include "iOSFile.h"
#include "iOSFileSystem.h"
#include "iOSApp.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Platform/Apple/AppleUtils.h"
#include "Engine/Platform/StringUtils.h"
#include "Engine/Platform/MessageBox.h"
#include "Engine/Platform/Window.h"
#include "Engine/Platform/WindowsManager.h"
#include "Engine/Platform/BatteryInfo.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/InputDevice.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Content/Storage/ContentStorageManager.h"
#include <UIKit/UIKit.h>
#include <CoreFoundation/CoreFoundation.h>
#include <QuartzCore/CAMetalLayer.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <os/proc.h>

class iOSTouchScreen : public InputDevice
{
private:
    CriticalSection _locker;

public:
    explicit iOSTouchScreen()
        : InputDevice(SpawnParams(Guid::New(), TypeInitializer), TEXT("iOS Touch Screen"))
    {
    }

    void ResetState() override
    {
        ScopeLock lock(_locker);
        InputDevice::ResetState();
    }

    bool Update(EventQueue& queue) override
    {
        ScopeLock lock(_locker);
        return InputDevice::Update(queue);
    }

    void OnTouch(EventType type, float x, float y, int32 pointerId)
    {
        ScopeLock lock(_locker);
        Event& e = _queue.AddOne();
        e.Type = type;
        e.Target = nullptr;
        e.TouchData.Position.X = x;
        e.TouchData.Position.Y = y;
        e.TouchData.PointerId = pointerId;
    }
};

int32 Dpi = 96;
Guid DeviceId;
FlaxView* MainView = nullptr;
FlaxViewController* MainViewController = nullptr;
iOSWindow* MainWindow = nullptr;
iOSTouchScreen* TouchScreen;
bool HasFocus = true;
bool IsPaused = false;

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

#define PLATFORM_IOS_MAX_TOUCHES 8

@interface FlaxView()

{
    UITouch* activeTouches[PLATFORM_IOS_MAX_TOUCHES];
}

@end

@implementation FlaxView

+ (Class)layerClass { return [CAMetalLayer class]; }

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

- (void)Init
{
    self.multipleTouchEnabled = YES;
    Platform::MemoryClear(activeTouches, sizeof(activeTouches));
}

- (void)OnTouchEvent:(NSSet*)touches ofType:(InputDevice::EventType)eventType
{
	float scale = [[UIScreen mainScreen] scale];
    for (UITouch* touch in touches)
    {
        // Get touch location
        CGPoint location = [touch locationInView:self];
        location.x *= scale;
        location.y *= scale;

        // Get touch index (use list of activly tracked touches)
        int32 touchIndex = 0;
        for (; touchIndex < PLATFORM_IOS_MAX_TOUCHES; touchIndex++)
        {
            if (activeTouches[touchIndex] == touch)
                break;
        }
        if (touchIndex == PLATFORM_IOS_MAX_TOUCHES && eventType == InputDevice::EventType::TouchDown)
        {
            // Find free slot for a new touches
            touchIndex = 0;
            for (; touchIndex < PLATFORM_IOS_MAX_TOUCHES; touchIndex++)
            {
                if (activeTouches[touchIndex] == nil)
                {
                    activeTouches[touchIndex] = touch;
                    break;
                }
            }
        }
        if (touchIndex == PLATFORM_IOS_MAX_TOUCHES)
            continue;

        // Send event to the input device
        TouchScreen->OnTouch(eventType, location.x, location.y, touchIndex);

        // Remove ended touches
        if (eventType == InputDevice::EventType::TouchUp)
            activeTouches[touchIndex] = nil;
    }
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event 
{
    [self OnTouchEvent:touches ofType:InputDevice::EventType::TouchDown];
}

- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event
{
    [self OnTouchEvent:touches ofType:InputDevice::EventType::TouchMove];
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
    [self OnTouchEvent:touches ofType:InputDevice::EventType::TouchUp];
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event
{
    [self OnTouchEvent:touches ofType:InputDevice::EventType::TouchUp];
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

- (UIStatusBarAnimation)preferredStatusBarUpdateAnimation
{
    return UIStatusBarAnimationSlide;
}

@end

@interface FlaxAppDelegate()

@property(strong, nonatomic) CADisplayLink* displayLink;

@end

@implementation FlaxAppDelegate

- (void)MainThreadMain:(NSDictionary*)launchOptions
{
    // Run engine on a separate game thread
    Engine::Main(TEXT(""));
}

- (void)UIThreadMain
{
    // Invoke callbacks
    UIThreadPipeline.Run();
}

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
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
    [self.view Init];
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
    self.displayLink.preferredFramesPerSecond = 60;
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

- (void)applicationWillResignActive:(UIApplication*)application
{
    LOG(Info, "[iOS] applicationWillResignActive");

    // Defocus app
    HasFocus = false;
    if (MainWindow)
        MainWindow->OnLostFocus();
}

- (void)applicationDidEnterBackground:(UIApplication*)application
{
    LOG(Info, "[iOS] applicationDidEnterBackground");

    // Pause
    IsPaused = true;
}

- (void)applicationWillEnterForeground:(UIApplication*)application
{
    LOG(Info, "[iOS] applicationWillEnterForeground");

    // Resume
    IsPaused = false;
}

- (void)applicationDidBecomeActive:(UIApplication*)application
{
    LOG(Info, "[iOS] applicationDidBecomeActive");

    // Focus app
    HasFocus = true;
    if (MainWindow)
        MainWindow->OnGotFocus();
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication*)application
{
    LOG(Warning, "[iOS] applicationDidReceiveMemoryWarning");
    LOG(Warning, "os_proc_available_memory: {}", Utilities::BytesToText(os_proc_available_memory()));
}

@end

DialogResult MessageBox::Show(Window* parent, const StringView& text, const StringView& caption, MessageBoxButtons buttons, MessageBoxIcon icon)
{
	NSString* title = (NSString*)AppleUtils::ToString(caption);
	NSString* message = (NSString*)AppleUtils::ToString(text);
    Function<void()> func = [&title, &message]()
    {
        UIAlertController* alert = [UIAlertController alertControllerWithTitle:title message:message preferredStyle:UIAlertControllerStyleAlert];
        UIAlertAction* button = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleCancel handler:^(id){ }];
        [alert addAction:button];
        [MainViewController presentViewController:alert animated:YES completion:nil];
    };
    iOSPlatform::RunOnUIThread(func, true);
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
    return IsFocused() && Platform::GetHasFocus();
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

    // Setup screen scaling
    ScreenScale = [[UIScreen mainScreen] scale];
    CustomDpiScale *= ScreenScale;
    Dpi = Math::TruncToInt(163 * ScreenScale);

	// Get device identifier
    NSString* uuid = [UIDevice currentDevice].identifierForVendor.UUIDString;
    String uuidStr = AppleUtils::ToString((CFStringRef)uuid);
    Guid::Parse(uuidStr, DeviceId);

    // Setup native platform input devices
    // TODO: add Gamepad for vibrations usability
    Input::CustomDevices.Add(TouchScreen = New<iOSTouchScreen>());

    // Use more aggressive content buffers freeing to reduce peek memory
    ContentStorageManager::UnusedDataChunksLifetime = TimeSpan::FromMilliseconds(30);

    return false;
}

void iOSPlatform::LogInfo()
{
    ApplePlatform::LogInfo();

	struct utsname systemInfo;
	uname(&systemInfo);
    NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
    LOG(Info, "{3}, iOS {0}.{1}.{2}", version.majorVersion, version.minorVersion, version.patchVersion, String(systemInfo.machine));
    LOG(Info, "os_proc_available_memory: {}", Utilities::BytesToText(os_proc_available_memory()));
}

void iOSPlatform::Tick()
{
    // Invoke callbacks
    MainThreadPipeline.Run();

    ApplePlatform::Tick();
}

BatteryInfo iOSPlatform::GetBatteryInfo()
{
    BatteryInfo result;
    UIDevice* uiDevice = [UIDevice currentDevice];
    if (uiDevice)
    {
        uiDevice.batteryMonitoringEnabled = YES;
        result.BatteryLifePercent = Math::Saturate([uiDevice batteryLevel]);
        switch ([uiDevice batteryState])
        {
        case UIDeviceBatteryStateUnknown:
            result.BatteryLifePercent = 1.0f;
            break;
        case UIDeviceBatteryStateUnplugged:
            result.State = BatteryInfo::States::BatteryDischarging;
            break;
        case UIDeviceBatteryStateCharging:
            result.State = BatteryInfo::States::BatteryCharging;
            break;
        case UIDeviceBatteryStateFull:
            result.State = BatteryInfo::States::Connected;
            break;
        }
    }
    return result;
}

int32 iOSPlatform::GetDpi()
{
    return Dpi;
}

NetworkConnectionType iOSPlatform::GetNetworkConnectionType()
{
    struct sockaddr_in emptyAddr;
    Platform::MemoryClear(&emptyAddr, sizeof(emptyAddr));
    emptyAddr.sin_len = sizeof(emptyAddr);
    emptyAddr.sin_family = AF_INET;
    SCNetworkReachabilityRef reachability = SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, (const struct sockaddr*)&emptyAddr);
    SCNetworkReachabilityFlags reachabilityFlags;
    bool connected = SCNetworkReachabilityGetFlags(reachability, &reachabilityFlags);
    CFRelease(reachability);
    if (connected)
    {
        if (reachabilityFlags == 0)
            return NetworkConnectionType::AirplaneMode;
        if ((reachabilityFlags & kSCNetworkReachabilityFlagsReachable) != 0 && 
            (reachabilityFlags & kSCNetworkReachabilityFlagsConnectionRequired) == 0 && 
            (reachabilityFlags & kSCNetworkReachabilityFlagsInterventionRequired) == 0)
        {
            if ((reachabilityFlags & kSCNetworkReachabilityFlagsIsWWAN) == 0)
                return NetworkConnectionType::WiFi;
            return NetworkConnectionType::Cell;
        }
    }
    return NetworkConnectionType::None;
}

ScreenOrientationType iOSPlatform::GetScreenOrientationType()
{
    UIInterfaceOrientation orientation = UIInterfaceOrientationUnknown;
	Function<void()> func = [&orientation]()
	{
		orientation = [[[[[UIApplication sharedApplication] delegate] window] windowScene] interfaceOrientation];
	};
	iOSPlatform::RunOnUIThread(func, true);
    switch (orientation)
    {
    case UIInterfaceOrientationPortrait: return ScreenOrientationType::Portrait;
    case UIInterfaceOrientationPortraitUpsideDown: return ScreenOrientationType::PortraitUpsideDown;
    case UIInterfaceOrientationLandscapeLeft: return ScreenOrientationType::LandscapeLeft;
    case UIInterfaceOrientationLandscapeRight: return ScreenOrientationType::LandscapeRight;
    case UIInterfaceOrientationUnknown: 
    default: return ScreenOrientationType::Unknown;
    }
}

Guid iOSPlatform::GetUniqueDeviceId()
{
    return DeviceId;
}

String iOSPlatform::GetComputerName()
{
    return TEXT("iPhone");
}

bool iOSPlatform::GetHasFocus()
{
    return HasFocus;
}

bool iOSPlatform::GetIsPaused()
{
    return IsPaused;
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
