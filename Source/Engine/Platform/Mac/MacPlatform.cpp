// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if PLATFORM_MAC

#include "MacPlatform.h"
#include "MacWindow.h"
#include "MacUtils.h"
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
#include "Engine/Platform/StringUtils.h"
#include "Engine/Platform/MessageBox.h"
#include "Engine/Platform/WindowsManager.h"
#include "Engine/Platform/Clipboard.h"
#include "Engine/Platform/IGuiData.h"
#include "Engine/Platform/Base/PlatformUtils.h"
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

CPUInfo MacCpu;
Guid DeviceId;
String UserLocale, ComputerName;
double SecondsPerCycle;
NSAutoreleasePool* AutoreleasePool = nullptr;

String MacUtils::ToString(CFStringRef str)
{
    if (!str)
        return String::Empty;
    String result;
    const int32 length = CFStringGetLength(str);
    if (length > 0)
    {
        CFRange range = CFRangeMake(0, length);
        result.ReserveSpace(length);
        CFStringGetBytes(str, range, kCFStringEncodingUTF16LE, '?', false, (uint8*)result.Get(), length * sizeof(Char), nullptr);
    }
    return result;
}

CFStringRef MacUtils::ToString(const StringView& str)
{
    return CFStringCreateWithBytes(nullptr, (const UInt8*)str.GetText(), str.Length() * sizeof(Char), kCFStringEncodingUTF16LE, false);
}

Float2 MacUtils::PosToCoca(const Float2& pos)
{
    // MacOS uses y-coordinate starting at the bottom of the screen
    Float2 result = pos;
    result.Y *= -1;
    result += GetScreensOrigin();
    return result;
}

Float2 MacUtils::CocaToPos(const Float2& pos)
{
    // MacOS uses y-coordinate starting at the bottom of the screen
    Float2 result = pos;
    result -= GetScreensOrigin();
    result.Y *= -1;
    return result;
}

Float2 MacUtils::GetScreensOrigin()
{
    Float2 result = Float2::Zero;
    NSArray* screenArray = [NSScreen screens];
    for (NSUInteger i = 0; i < [screenArray count]; i++)
    {
        NSRect rect = [[screenArray objectAtIndex:i] frame];
        Float2 pos(rect.origin.x, rect.origin.y + rect.size.height);
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
    [pasteboard writeObjects:[NSArray arrayWithObject:(NSString*)MacUtils::ToString(text)]];
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
    return MacUtils::ToString((CFStringRef)[objects objectAtIndex:0]);
}

Array<byte> MacClipboard::GetRawData()
{
    return Array<byte>();
}

Array<String> MacClipboard::GetFiles()
{
    return Array<String>();
}

DialogResult MessageBox::Show(Window* parent, const StringView& text, const StringView& caption, MessageBoxButtons buttons, MessageBoxIcon icon)
{
    if (CommandLine::Options.Headless)
        return DialogResult::None;
    CFStringRef textRef = MacUtils::ToString(text);
    CFStringRef captionRef = MacUtils::ToString(caption);
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

typedef uint16_t offset_t;
#define align_mem_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

bool MacPlatform::Is64BitPlatform()
{
    return PLATFORM_64BITS;
}

CPUInfo MacPlatform::GetCPUInfo()
{
    return MacCpu;
}

int32 MacPlatform::GetCacheLineSize()
{
    return MacCpu.CacheLineSize;
}

MemoryStats MacPlatform::GetMemoryStats()
{
    MemoryStats result;
    int64 value64;
    size_t value64Size = sizeof(value64);
    if (sysctlbyname("hw.memsize", &value64, &value64Size, nullptr, 0) != 0)
        value64 = 1024 * 1024;
    result.TotalPhysicalMemory = value64;
    int id[] = { CTL_HW, HW_MEMSIZE };
    if (sysctl(id, 2, &value64, &value64Size, nullptr, 0) != 0)
        value64Size = 1024;
    result.UsedPhysicalMemory = value64Size;
    xsw_usage swapusage;
    size_t swapusageSize = sizeof(swapusage);
    result.TotalVirtualMemory = result.TotalPhysicalMemory;
    result.UsedVirtualMemory = result.UsedPhysicalMemory;
    if (sysctlbyname("vm.swapusage", &swapusage, &swapusageSize, nullptr, 0) == 0)
    {
        result.TotalVirtualMemory += swapusage.xsu_total;
        result.UsedVirtualMemory += swapusage.xsu_used;
    }
    return result;
}

ProcessMemoryStats MacPlatform::GetProcessMemoryStats()
{
    ProcessMemoryStats result;
    result.UsedPhysicalMemory = 1024;
    result.UsedVirtualMemory = 1024;
    return result;
}

uint64 MacPlatform::GetCurrentThreadID()
{
    return (uint64)pthread_mach_thread_np(pthread_self());
}

void MacPlatform::SetThreadPriority(ThreadPriority priority)
{
    // TODO: impl this
}

void MacPlatform::SetThreadAffinityMask(uint64 affinityMask)
{
    // TODO: impl this
}

void MacPlatform::Sleep(int32 milliseconds)
{
    usleep(milliseconds * 1000);
}

double MacPlatform::GetTimeSeconds()
{
    return SecondsPerCycle * mach_absolute_time();
}

uint64 MacPlatform::GetTimeCycles()
{
    return mach_absolute_time();
}

uint64 MacPlatform::GetClockFrequency()
{
    return (uint64)(1.0 / SecondsPerCycle);
}

void MacPlatform::GetSystemTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond)
{
    // Query for calendar time
    struct timeval time;
    gettimeofday(&time, nullptr);

    // Convert to local time
    struct tm localTime;
    localtime_r(&time.tv_sec, &localTime);

    // Extract time
    year = localTime.tm_year + 1900;
    month = localTime.tm_mon + 1;
    dayOfWeek = localTime.tm_wday;
    day = localTime.tm_mday;
    hour = localTime.tm_hour;
    minute = localTime.tm_min;
    second = localTime.tm_sec;
    millisecond = time.tv_usec / 1000;
}

void MacPlatform::GetUTCTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond)
{
    // Get the calendar time
    struct timeval time;
    gettimeofday(&time, nullptr);

    // Convert to UTC time
    struct tm localTime;
    gmtime_r(&time.tv_sec, &localTime);

    // Extract time
    year = localTime.tm_year + 1900;
    month = localTime.tm_mon + 1;
    dayOfWeek = localTime.tm_wday;
    day = localTime.tm_mday;
    hour = localTime.tm_hour;
    minute = localTime.tm_min;
    second = localTime.tm_sec;
    millisecond = time.tv_usec / 1000;
}

bool MacPlatform::Init()
{
    if (UnixPlatform::Init())
        return true;

    // Init timing
    {
        mach_timebase_info_data_t info;
        mach_timebase_info(&info);
        SecondsPerCycle = 1e-9 * (double)info.numer / (double)info.denom;
    }

    // Get CPU info
    int32 value32;
    int64 value64;
    size_t value32Size = sizeof(value32), value64Size = sizeof(value64);
    if (sysctlbyname("hw.packages", &value32, &value32Size, nullptr, 0) != 0)
        value32 = 1;
    MacCpu.ProcessorPackageCount = value32;
    if (sysctlbyname("hw.physicalcpu", &value32, &value32Size, nullptr, 0) != 0)
        value32 = 1;
    MacCpu.ProcessorCoreCount = value32;
    if (sysctlbyname("hw.logicalcpu", &value32, &value32Size, nullptr, 0) != 0)
        value32 = 1;
    MacCpu.LogicalProcessorCount = value32;
    if (sysctlbyname("hw.l1icachesize", &value32, &value32Size, nullptr, 0) != 0)
        value32 = 0;
    MacCpu.L1CacheSize = value32;
    if (sysctlbyname("hw.l2cachesize", &value32, &value32Size, nullptr, 0) != 0)
        value32 = 0;
    MacCpu.L2CacheSize = value32;
    if (sysctlbyname("hw.l3cachesize", &value32, &value32Size, nullptr, 0) != 0)
        value32 = 0;
    MacCpu.L3CacheSize = value32;
    if (sysctlbyname("hw.pagesize", &value32, &value32Size, nullptr, 0) != 0)
        value32 = vm_page_size;
    MacCpu.PageSize = value32;
    if (sysctlbyname("hw.cpufrequency_max", &value64, &value64Size, nullptr, 0) != 0)
        value64 = GetClockFrequency();
    MacCpu.ClockSpeed = value64;
    if (sysctlbyname("hw.cachelinesize", &value32, &value32Size, nullptr, 0) != 0)
        value32 = PLATFORM_CACHE_LINE_SIZE;
    MacCpu.CacheLineSize = value32;

    // Get device id
    {
        io_registry_entry_t ioRegistryRoot = IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/");
        CFStringRef deviceUuid = (CFStringRef)IORegistryEntryCreateCFProperty(ioRegistryRoot, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);
        IOObjectRelease(ioRegistryRoot);
        String uuidStr = MacUtils::ToString(deviceUuid);
        Guid::Parse(uuidStr, DeviceId);
        CFRelease(deviceUuid);
    }

    // Get locale
    {
        CFLocaleRef locale = CFLocaleCopyCurrent();
        CFStringRef localeLang = (CFStringRef)CFLocaleGetValue(locale, kCFLocaleLanguageCode);
        CFStringRef localeCountry = (CFStringRef)CFLocaleGetValue(locale, kCFLocaleCountryCode);
        UserLocale = MacUtils::ToString(localeLang);
        String localeCountryStr = MacUtils::ToString(localeCountry);
        if (localeCountryStr.HasChars())
            UserLocale += TEXT("-") + localeCountryStr;
        CFRelease(locale);
        CFRelease(localeLang);
        CFRelease(localeCountry);
    }

    // Get computer name
    {
        CFStringRef computerName = SCDynamicStoreCopyComputerName(nullptr, nullptr);
        ComputerName = MacUtils::ToString(computerName);
        CFRelease(computerName);
    }

    // Init user
    {
        String username;
        GetEnvironmentVariable(TEXT("USER"), username);
        OnPlatformUserAdd(New<User>(username));
    }

    // Init application
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    NSMenu* mainMenu = [[[NSMenu alloc] initWithTitle:@""] autorelease];
    [NSApp setMainMenu:mainMenu];
    AutoreleasePool = [[NSAutoreleasePool alloc] init];

    Input::Mouse = New<MacMouse>();
    Input::Keyboard = New<MacKeyboard>();

    return false;
}

void MacPlatform::LogInfo()
{
    UnixPlatform::LogInfo();

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

void MacPlatform::BeforeExit()
{
}

void MacPlatform::Exit()
{
}

int32 MacPlatform::GetDpi()
{
    CGDirectDisplayID mainDisplay = CGMainDisplayID();
    CGSize size = CGDisplayScreenSize(mainDisplay);
    float wide = (float)CGDisplayPixelsWide(mainDisplay);
    float dpi = (wide * 25.4f) / size.width;
    return Math::Max(dpi, 72.0f);
}

String MacPlatform::GetUserLocaleName()
{
    return UserLocale;
}

String MacPlatform::GetComputerName()
{
    return ComputerName;
}

bool MacPlatform::GetHasFocus()
{
	// Check if any window is focused
	ScopeLock lock(WindowsManager::WindowsLocker);
	for (auto window : WindowsManager::Windows)
	{
		if (window->IsFocused())
			return true;
	}

	// Default to true if has no windows open
    return WindowsManager::Windows.IsEmpty();
}

void MacPlatform::CreateGuid(Guid& result)
{
    uuid_t uuid;
    uuid_generate(uuid);
    auto ptr = (uint32*)&uuid;
    result.A = ptr[0];
    result.B = ptr[1];
    result.C = ptr[2];
    result.D = ptr[3];
}

bool MacPlatform::CanOpenUrl(const StringView& url)
{
    return false;
}

void MacPlatform::OpenUrl(const StringView& url)
{
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
    return Float2((float)CGDisplayPixelsWide(mainDisplay), (float)CGDisplayPixelsHigh(mainDisplay));
}

Rectangle GetDisplayBounds(CGDirectDisplayID display)
{
    CGRect rect = CGDisplayBounds(display);
    return Rectangle(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
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

String MacPlatform::GetExecutableFilePath()
{
    char buf[PATH_MAX];
    uint32 size = PATH_MAX;
    String result;
    if (_NSGetExecutablePath(buf, &size) == 0)
        result.SetUTF8(buf, StringUtils::Length(buf));
    return result;
}

Guid MacPlatform::GetUniqueDeviceId()
{
    return DeviceId;
}

String MacPlatform::GetWorkingDirectory()
{
	char buffer[256];
	getcwd(buffer, ARRAY_COUNT(buffer));
	return String(buffer);
}

bool MacPlatform::SetWorkingDirectory(const String& path)
{
    return chdir(StringAsANSI<>(*path).Get()) != 0;
}

Window* MacPlatform::CreateWindow(const CreateWindowSettings& settings)
{
    return New<MacWindow>(settings);
}

bool MacPlatform::GetEnvironmentVariable(const String& name, String& value)
{
    char* env = getenv(StringAsANSI<>(*name).Get());
    if (env)
    {
        value = String(env);
        return false;
	}
    return true;
}

bool MacPlatform::SetEnvironmentVariable(const String& name, const String& value)
{
    return setenv(StringAsANSI<>(*name).Get(), StringAsANSI<>(*value).Get(), true) != 0;
}

int32 MacProcess(const StringView& cmdLine, const StringView& workingDir, const Dictionary<String, String>& environment, bool waitForEnd, bool logOutput)
{
    LOG(Info, "Command: {0}", cmdLine);
    String cwd;
    if (workingDir.Length() != 0)
    {
        LOG(Info, "Working directory: {0}", workingDir);
        cwd = Platform::GetWorkingDirectory();
        Platform::SetWorkingDirectory(workingDir);
    }

    StringAsANSI<> cmdLineAnsi(*cmdLine, cmdLine.Length());
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
    if (waitForEnd)
    {
        int stat_loc;
        if (logOutput)
        {
            char lineBuffer[1024];
            while (fgets(lineBuffer, sizeof(lineBuffer), pipe) != NULL)
            {
                char *p = lineBuffer + strlen(lineBuffer) - 1;
                if (*p == '\n') *p=0;
                Log::Logger::Write(LogType::Info, String(lineBuffer));
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

int32 MacPlatform::StartProcess(const StringView& filename, const StringView& args, const StringView& workingDir, bool hiddenWindow, bool waitForEnd)
{
	// hiddenWindow has hardly any meaning on UNIX/Linux/OSX as the program that is called decides whether it has a GUI or not

    // Special case if filename points to the app package (use actual executable)
    String exePath = filename;
	{
        NSString* processPath = (NSString*)MacUtils::ToString(filename);
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
                        exePath = MacUtils::ToString((CFStringRef)processPath);
                }
            }
        }
	}

    String cmdLine = String::Format(TEXT("\"{0}\" {1}"), exePath, args);
	return MacProcess(cmdLine, workingDir, Dictionary<String, String>(), waitForEnd, false);
}

int32 MacPlatform::RunProcess(const StringView& cmdLine, const StringView& workingDir, bool hiddenWindow)
{
    return MacProcess(cmdLine, workingDir, Dictionary<String, String>(), true, true);
}

int32 MacPlatform::RunProcess(const StringView& cmdLine, const StringView& workingDir, const Dictionary<String, String>& environment, bool hiddenWindow)
{
	return MacProcess(cmdLine, workingDir, environment, true, true);
}

void* MacPlatform::LoadLibrary(const Char* filename)
{
    const StringAsANSI<> filenameANSI(filename);
    void* result = dlopen(filenameANSI.Get(), RTLD_LAZY | RTLD_LOCAL);
    if (!result)
    {
        LOG(Error, "Failed to load {0} because {1}", filename, String(dlerror()));
    }
    return result;
}

void MacPlatform::FreeLibrary(void* handle)
{
	dlclose(handle);
}

void* MacPlatform::GetProcAddress(void* handle, const char* symbol)
{
    return dlsym(handle, symbol);
}

Array<MacPlatform::StackFrame> MacPlatform::GetStackFrames(int32 skipCount, int32 maxDepth, void* context)
{
    Array<StackFrame> result;
#if CRASH_LOG_ENABLE
    void* callstack[120];
    skipCount = Math::Min<int32>(skipCount, ARRAY_COUNT(callstack));
    int32 maxCount = Math::Min<int32>(ARRAY_COUNT(callstack), skipCount + maxDepth);
    int32 count = backtrace(callstack, maxCount);
    int32 useCount = count - skipCount;
    if (useCount > 0)
    {
        char** names = backtrace_symbols(callstack + skipCount, useCount);
        result.Resize(useCount);
        for (int32 i = 0; i < useCount; i++)
        {
            char* name = names[i];
            StackFrame& frame = result[i];
            frame.ProgramCounter = callstack[skipCount + i];
            frame.ModuleName[0] = 0;
            frame.FileName[0] = 0;
            frame.LineNumber = 0;
            int32 nameLen = Math::Min<int32>(StringUtils::Length(name), ARRAY_COUNT(frame.FunctionName) - 1);
            Platform::MemoryCopy(frame.FunctionName, name, nameLen);
            frame.FunctionName[nameLen] = 0;
            
        }
        free(names);
    }
#endif
    return result;
}

#endif
