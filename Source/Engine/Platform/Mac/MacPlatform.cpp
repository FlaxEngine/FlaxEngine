// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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

DialogResult MessageBox::Show(Window* parent, const StringView& text, const StringView& caption, MessageBoxButtons buttons, MessageBoxIcon icon)
{
    if (CommandLine::Options.Headless)
        return DialogResult::None;
	NSAlert* alert = [[NSAlert alloc] init];
    ASSERT(alert);
    switch (buttons)
    {
    case MessageBoxButtons::AbortRetryIgnore:
        [alert addButtonWithTitle:@"Abort"];
        [alert addButtonWithTitle:@"Retry"];
        [alert addButtonWithTitle:@"Ignore"];
        break;
    case MessageBoxButtons::OK:
        [alert addButtonWithTitle:@"OK"];
        break;
    case MessageBoxButtons::OKCancel:
        [alert addButtonWithTitle:@"OK"];
        [alert addButtonWithTitle:@"Cancel"];
        break;
    case MessageBoxButtons::RetryCancel:
        [alert addButtonWithTitle:@"Retry"];
        [alert addButtonWithTitle:@"Cancel"];
        break;
    case MessageBoxButtons::YesNo:
        [alert addButtonWithTitle:@"Yes"];
        [alert addButtonWithTitle:@"No"];
        break;
    case MessageBoxButtons::YesNoCancel:
        [alert addButtonWithTitle:@"Yes"];
        [alert addButtonWithTitle:@"No"];
        [alert addButtonWithTitle:@"Cancel"];
        break;
    }
    switch (icon)
    {
    case MessageBoxIcon::Information:
	    [alert setAlertStyle:NSAlertStyleCritical];
        break;
    case MessageBoxIcon::Error:
    case MessageBoxIcon::Stop:
	    [alert setAlertStyle:NSAlertStyleInformational];
        break;
    case MessageBoxIcon::Warning:
	    [alert setAlertStyle:NSAlertStyleWarning];
        break;
    }
	[alert setMessageText:(NSString*)AppleUtils::ToString(caption)];
	[alert setInformativeText:(NSString*)AppleUtils::ToString(text)];
	NSInteger button = [alert runModal];
    DialogResult result = DialogResult::OK;
    switch (buttons)
    {
    case MessageBoxButtons::AbortRetryIgnore:
        if (button == NSAlertFirstButtonReturn)
            result = DialogResult::Abort;
        else if (button == NSAlertSecondButtonReturn)
            result = DialogResult::Retry;
        else
            result = DialogResult::Ignore;
        break;
    case MessageBoxButtons::OK:
        result = DialogResult::OK;
        break;
    case MessageBoxButtons::OKCancel:
        if (button == NSAlertFirstButtonReturn)
            result = DialogResult::OK;
        else
            result = DialogResult::Cancel;
        break;
    case MessageBoxButtons::RetryCancel:
        if (button == NSAlertFirstButtonReturn)
            result = DialogResult::Retry;
        else
            result = DialogResult::Cancel;
        break;
    case MessageBoxButtons::YesNo:
        if (button == NSAlertFirstButtonReturn)
            result = DialogResult::Yes;
        else
            result = DialogResult::No;
        break;
    case MessageBoxButtons::YesNoCancel:
        if (button == NSAlertFirstButtonReturn)
            result = DialogResult::Yes;
        else if (button == NSAlertSecondButtonReturn)
            result = DialogResult::No;
        else
            result = DialogResult::Cancel;
        break;
    }
    return result;
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
    NSEvent* event = nil;
    do
    {
        event = [NSApp nextEventMatchingMask: NSEventMaskAny untilDate: nil inMode: NSDefaultRunLoopMode dequeue: YES];
        if (event)
        {
            [NSApp sendEvent:event];
        }
        
    } while(event);

    ApplePlatform::Tick();
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
    return Float2((float)cursor.x, (float)cursor.y) * MacPlatform::ScreenScale;
}

void MacPlatform::SetMousePosition(const Float2& pos)
{
    CGPoint cursor;
    cursor.x = (CGFloat)(pos.X / MacPlatform::ScreenScale);
    cursor.y = (CGFloat)(pos.Y / MacPlatform::ScreenScale);
    CGWarpMouseCursorPosition(cursor);
    CGAssociateMouseAndMouseCursorPosition(true);
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
        path = path.Left(path.Length() - 6);
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

    NSTask *task = [[NSTask alloc] init];
    task.launchPath = AppleUtils::ToNSString(exePath);
    task.arguments  = AppleUtils::ParseArguments(AppleUtils::ToNSString(settings.Arguments));
    if (settings.WorkingDirectory.HasChars())
        task.currentDirectoryPath = AppleUtils::ToNSString(settings.WorkingDirectory);

    int32 returnCode = 0;
    if (settings.WaitForEnd)
    {
        id<NSObject> outputObserver = nil;
        id<NSObject> outputObserverError = nil;
        
        if (captureStdOut)
        {
            NSPipe* stdoutPipe = [NSPipe pipe];
            [task setStandardOutput:stdoutPipe];
            outputObserver = [[NSNotificationCenter defaultCenter]
                                          addObserverForName: NSFileHandleDataAvailableNotification
                                          object: [stdoutPipe fileHandleForReading]
                                          queue: nil
                              usingBlock:^(NSNotification* notification)
            {
                NSData* data = [stdoutPipe fileHandleForReading].availableData;
                if (data.length)
                {
                      String line((const char*)data.bytes, data.length);
                      if (settings.SaveOutput)
                        settings.Output.Add(line.Get(), line.Length());
                      if (settings.LogOutput)
                      {
                        StringView lineView(line);
                        if (line[line.Length() - 1] == '\n')
                            lineView = StringView(line.Get(), line.Length() - 1);
                        Log::Logger::Write(LogType::Info, lineView);
                      }
                    [[stdoutPipe fileHandleForReading] waitForDataInBackgroundAndNotify];
                }
            }
                ];
            [[stdoutPipe fileHandleForReading] waitForDataInBackgroundAndNotify];

            NSPipe *stderrPipe = [NSPipe pipe];
            [task setStandardError:stderrPipe];
            outputObserverError = [[NSNotificationCenter defaultCenter]
                                          addObserverForName: NSFileHandleDataAvailableNotification
                                          object: [stderrPipe fileHandleForReading]
                                          queue: nil
                              usingBlock:^(NSNotification* notification)
            {
                NSData* data = [stderrPipe fileHandleForReading].availableData;
                if (data.length)
                {
                      String line((const char*)data.bytes, data.length);
                      if (settings.SaveOutput)
                        settings.Output.Add(line.Get(), line.Length());
                      if (settings.LogOutput)
                      {
                        StringView lineView(line);
                        if (line[line.Length() - 1] == '\n')
                            lineView = StringView(line.Get(), line.Length() - 1);
                        Log::Logger::Write(LogType::Error, lineView);
                      }
                    [[stderrPipe fileHandleForReading] waitForDataInBackgroundAndNotify];
                }
            }
                ];
            [[stderrPipe fileHandleForReading] waitForDataInBackgroundAndNotify];
        }

        String exception;
        @try
        {
            [task launch];
            [task waitUntilExit];
            returnCode = [task terminationStatus];
        }
        @catch (NSException* e)
        {
            exception = e.reason.UTF8String;
        }
        if (!exception.IsEmpty())
        {
            LOG(Error, "Failed to run command {0} {1} with error {2}", settings.FileName, settings.Arguments, exception);
            returnCode = -1;
        }
    }
    else
    {
        String exception;  
        @try
        {
            [task launch];
        }
        @catch (NSException* e)
        {
            exception = e.reason.UTF8String;
        }
        if (!exception.IsEmpty())
        {
            LOG(Error, "Failed to run command {0} {1} with error {2}", settings.FileName, settings.Arguments, exception);
            returnCode = -1;
        }
    }

	return returnCode;
}

#endif
