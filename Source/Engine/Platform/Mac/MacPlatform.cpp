// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if PLATFORM_MAC

#include "MacPlatform.h"
#include "MacWindow.h"
#include "Engine/Platform/Apple/AppleUtils.h"
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
#include "Engine/Platform/MessageBox.h"
#include "Engine/Platform/StringUtils.h"
#include "Engine/Platform/WindowsManager.h"
#include "Engine/Platform/Clipboard.h"
#include "Engine/Platform/IGuiData.h"
#include "Engine/Platform/Base/PlatformUtils.h"
#include "Engine/Platform/CreateProcessSettings.h"
#include "Engine/Utilities/StringConverter.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Input/Keyboard.h"
#include <unistd.h>
#include <cstdint>
#include <stdlib.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <mach/mach_time.h>
#include <mach-o/dyld.h>
#include <uuid/uuid.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <IOKit/IOKitLib.h>
#include <Cocoa/Cocoa.h>
#include <dlfcn.h>
#if CRASH_LOG_ENABLE
#include <execinfo.h>
#endif

Guid DeviceId;
String ComputerName;
NSAutoreleasePool* AutoreleasePool = nullptr;

DialogResult MessageBox::Show(Window* parent, const StringView& text, const StringView& caption, MessageBoxButtons buttons, MessageBoxIcon icon)
{
    if (CommandLine::Options.Headless)
        return DialogResult::None;
    CFStringRef textRef = AppleUtils::ToString(text);
    CFStringRef captionRef = AppleUtils::ToString(caption);
    CFOptionFlags flags = 0;
    switch (buttons)
    {
    case MessageBoxButtons::AbortRetryIgnore:
    case MessageBoxButtons::OKCancel:
    case MessageBoxButtons::RetryCancel:
    case MessageBoxButtons::YesNo:
    case MessageBoxButtons::YesNoCancel:
        flags |= kCFUserNotificationCancelResponse;
        break;
    }
    switch (icon)
    {
    case MessageBoxIcon::Information:
        flags |= kCFUserNotificationNoteAlertLevel;
        break;
    case MessageBoxIcon::Error:
    case MessageBoxIcon::Stop:
        flags |= kCFUserNotificationStopAlertLevel;
        break;
    case MessageBoxIcon::Warning:
        flags |= kCFUserNotificationCautionAlertLevel;
        break;
    }
    SInt32 result = CFUserNotificationDisplayNotice(0, flags, nullptr, nullptr, nullptr, captionRef, textRef, nullptr);
    CFRelease(captionRef);
    CFRelease(textRef);
    return DialogResult::OK;
}

Float2 AppleUtils::PosToCoca(const Float2& pos)
{
    // MacOS uses y-coordinate starting at the bottom of the screen
    Float2 result = pos;// / ApplePlatform::ScreenScale;
    result.Y *= -1;
    result += GetScreensOrigin();
    return result;
}

Float2 AppleUtils::CocaToPos(const Float2& pos)
{
    // MacOS uses y-coordinate starting at the bottom of the screen
    Float2 result = pos;// * ApplePlatform::ScreenScale;
    result -= GetScreensOrigin();
    result.Y *= -1;
    return result;// * ApplePlatform::ScreenScale;
}

Float2 AppleUtils::GetScreensOrigin()
{
    Float2 result = Float2::Zero;
    NSArray* screenArray = [NSScreen screens];
    for (NSUInteger i = 0; i < [screenArray count]; i++)
    {
        NSRect rect = [[screenArray objectAtIndex:i] frame];
        Float2 pos(rect.origin.x, rect.origin.y + rect.size.height);
        pos *= ApplePlatform::ScreenScale;
        if (pos.X < result.X)
            result.X = pos.X;
        if (pos.Y > result.Y)
            result.Y = pos.Y;
    }
    return result;
}

void MacClipboard::Clear()
{
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    [pasteboard clearContents];
}

void MacClipboard::SetText(const StringView& text)
{
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    [pasteboard clearContents];
    [pasteboard writeObjects:[NSArray arrayWithObject:(NSString*)AppleUtils::ToString(text)]];
}

void MacClipboard::SetRawData(const Span<byte>& data)
{
}

void MacClipboard::SetFiles(const Array<String>& files)
{
}

String MacClipboard::GetText()
{
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    NSArray* classes = [NSArray arrayWithObject:[NSString class]];
    NSDictionary* options = [NSDictionary dictionary];
    if (![pasteboard canReadObjectForClasses:classes options:options])
        return String::Empty;
    NSArray* objects = [pasteboard readObjectsForClasses:classes options:options];
    return AppleUtils::ToString((CFStringRef)[objects objectAtIndex:0]);
}

Array<byte> MacClipboard::GetRawData()
{
    return Array<byte>();
}

Array<String> MacClipboard::GetFiles()
{
    return Array<String>();
}

class MacKeyboard : public Keyboard
{
public:
	explicit MacKeyboard()
		: Keyboard()
	{
	}
};

class MacMouse : public Mouse
{
public:
	explicit MacMouse()
		: Mouse()
	{
	}

public:

    // [Mouse]
    void SetMousePosition(const Float2& newPosition) final override
    {
        MacPlatform::SetMousePosition(newPosition);

        OnMouseMoved(newPosition);
    }
};

bool MacPlatform::Init()
{
    if (ApplePlatform::Init())
        return true;

    // Get device id
    {
        io_registry_entry_t ioRegistryRoot = IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/");
        CFStringRef deviceUuid = (CFStringRef)IORegistryEntryCreateCFProperty(ioRegistryRoot, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);
        IOObjectRelease(ioRegistryRoot);
        String uuidStr = AppleUtils::ToString(deviceUuid);
        Guid::Parse(uuidStr, DeviceId);
        CFRelease(deviceUuid);
    }

    // Get computer name
    {
        CFStringRef computerName = SCDynamicStoreCopyComputerName(nullptr, nullptr);
        ComputerName = AppleUtils::ToString(computerName);
        CFRelease(computerName);
    }

    // Find the maximum scale of the display to handle high-dpi displays scaling factor
    {
	    NSArray* screenArray = [NSScreen screens];
        for (int32 i = 0; i < (int32)[screenArray count]; i++)
        {
            if ([[screenArray objectAtIndex:i] respondsToSelector:@selector(backingScaleFactor)])
            {
				ScreenScale = Math::Max(ScreenScale, (float)[[screenArray objectAtIndex:i] backingScaleFactor]);
            }
        }
        CustomDpiScale *= ScreenScale;
    }

    // Init application
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    AutoreleasePool = [[NSAutoreleasePool alloc] init];

    // Init main menu
    NSMenu* mainMenu = [[[NSMenu alloc] initWithTitle:@""] autorelease];
    [NSApp setMainMenu:mainMenu];
    // TODO: expose main menu for app (eg. to be used by Game or Editor on macOS-only)

    Input::Mouse = New<MacMouse>();
    Input::Keyboard = New<MacKeyboard>();

    return false;
}

void MacPlatform::LogInfo()
{
    ApplePlatform::LogInfo();

    char str[250];
    size_t strSize = sizeof(str);
    if (sysctlbyname("kern.osrelease", str, &strSize, nullptr, 0) != 0)
        str[0] = 0;
    String osRelease(str);
    if (sysctlbyname("kern.osproductversion", str, &strSize, nullptr, 0) != 0)
        str[0] = 0;
    String osProductVer(str);
    LOG(Info, "macOS {1} (kernel {0})", osRelease, osProductVer);
}

void MacPlatform::BeforeRun()
{
    [NSApp finishLaunching];
}

void MacPlatform::Tick()
{
    // Process system events
    while (true)
    {
        NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
        if (event == nil)
            break;
        [NSApp sendEvent:event];
    }

    [AutoreleasePool drain];
    AutoreleasePool = [[NSAutoreleasePool alloc] init];
}

int32 MacPlatform::GetDpi()
{
    CGDirectDisplayID mainDisplay = CGMainDisplayID();
    CGSize size = CGDisplayScreenSize(mainDisplay);
    float wide = (float)CGDisplayPixelsWide(mainDisplay);
    float dpi = (wide * 25.4f) / size.width;
    return Math::Max(dpi, 72.0f);
}

Guid MacPlatform::GetUniqueDeviceId()
{
    return DeviceId;
}

String MacPlatform::GetComputerName()
{
    return ComputerName;
}

Float2 MacPlatform::GetMousePosition()
{
    CGEventRef event = CGEventCreate(nullptr);
    CGPoint cursor = CGEventGetLocation(event);
    CFRelease(event);
    return Float2((float)cursor.x, (float)cursor.y);
}

void MacPlatform::SetMousePosition(const Float2& pos)
{
    CGPoint cursor;
    cursor.x = (CGFloat)pos.X;
    cursor.y = (CGFloat)pos.Y;
    CGWarpMouseCursorPosition(cursor);
}

Float2 MacPlatform::GetDesktopSize()
{
    CGDirectDisplayID mainDisplay = CGMainDisplayID();
    return Float2((float)CGDisplayPixelsWide(mainDisplay) * ScreenScale, (float)CGDisplayPixelsHigh(mainDisplay) * ScreenScale);
}

Rectangle GetDisplayBounds(CGDirectDisplayID display)
{
    CGRect rect = CGDisplayBounds(display);
    float screnScale = ApplePlatform::ScreenScale;
    return Rectangle(rect.origin.x * screnScale, rect.origin.y * screnScale, rect.size.width * screnScale, rect.size.height * screnScale);
}

Rectangle MacPlatform::GetMonitorBounds(const Float2& screenPos)
{
    CGPoint point;
    point.x = screenPos.X;
    point.y = screenPos.Y;
    CGDirectDisplayID display;
    uint32_t count = 0;
    CGGetDisplaysWithPoint(point, 1, &display, &count);
    if (count == 1)
        return GetDisplayBounds(display);
	return Rectangle(Float2::Zero, GetDesktopSize());
}

Rectangle MacPlatform::GetVirtualDesktopBounds()
{
    CGDirectDisplayID displays[16];
    uint32_t count = 0;
    CGGetOnlineDisplayList(ARRAY_COUNT(displays), displays, &count);
    if (count == 0)
	    return Rectangle(Float2::Zero, GetDesktopSize());
    Rectangle result = GetDisplayBounds(displays[0]);
    for (uint32_t i = 1; i < count; i++)
        result = Rectangle::Union(result, GetDisplayBounds(displays[i]));
    return result;
}

String MacPlatform::GetMainDirectory()
{
    String path = StringUtils::GetDirectoryName(GetExecutableFilePath());
    if (path.EndsWith(TEXT("/Contents/MacOS")))
    {
        // If running from executable in a package, go up to the Contents
        path = StringUtils::GetDirectoryName(path);
    }
    return path;
}

Window* MacPlatform::CreateWindow(const CreateWindowSettings& settings)
{
    return New<MacWindow>(settings);
}

int32 MacPlatform::CreateProcess(CreateProcessSettings& settings)
{
    LOG(Info, "Command: {0} {1}", settings.FileName, settings.Arguments);
    String cwd;
    if (settings.WorkingDirectory.HasChars())
    {
        LOG(Info, "Working directory: {0}", settings.WorkingDirectory);
        cwd = Platform::GetWorkingDirectory();
        Platform::SetWorkingDirectory(settings.WorkingDirectory);
    }
    const bool captureStdOut = settings.LogOutput || settings.SaveOutput;
    
    // Special case if filename points to the app package (use actual executable)
    String exePath = settings.FileName;
	{
        NSString* processPath = (NSString*)AppleUtils::ToString(exePath);
        if (![[NSFileManager defaultManager] fileExistsAtPath: processPath])
        {
            NSString* appName = [[processPath lastPathComponent] stringByDeletingPathExtension];
            processPath = [[NSWorkspace sharedWorkspace] fullPathForApplication:appName];
        }
        if ([[NSFileManager defaultManager] fileExistsAtPath: processPath])
        {
            if([[NSWorkspace sharedWorkspace] isFilePackageAtPath: processPath])
            {
                NSBundle* bundle = [NSBundle bundleWithPath:processPath];
                if (bundle != nil)
                {
                    processPath = [bundle executablePath];
                    if (processPath != nil)
                        exePath = AppleUtils::ToString((CFStringRef)processPath);
                }
            }
        }
	}

    const String cmdLine = exePath + TEXT(" ") + settings.Arguments;
    const StringAsANSI<> cmdLineAnsi(*cmdLine, cmdLine.Length());
    FILE* pipe = popen(cmdLineAnsi.Get(), "r");
    if (cwd.Length() != 0)
    {
        Platform::SetWorkingDirectory(cwd);
    }
    if (!pipe)
    {
		LOG(Warning, "Failed to start process, errno={}", errno);
        return -1;
    }

    // TODO: environment

    int32 returnCode = 0;
    if (settings.WaitForEnd)
    {
        if (captureStdOut)
        {
            char lineBuffer[1024];
            while (fgets(lineBuffer, sizeof(lineBuffer), pipe) != NULL)
            {
                char* p = lineBuffer + strlen(lineBuffer) - 1;
                if (*p == '\n') *p = 0;
                String line(lineBuffer);
                if (settings.SaveOutput)
                    settings.Output.Add(line.Get(), line.Length());
                if (settings.LogOutput)
                    Log::Logger::Write(LogType::Info, line);
            }
        }
        else
        {
            while (!feof(pipe))
            {
                sleep(1);
            }
        }
    }

	return returnCode;
}

#endif
