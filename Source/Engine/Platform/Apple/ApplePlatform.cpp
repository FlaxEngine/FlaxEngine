// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_MAC || PLATFORM_IOS

#include "ApplePlatform.h"
#include "AppleUtils.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/Version.h"
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
#include "Engine/Platform/WindowsManager.h"
#include "Engine/Platform/Clipboard.h"
#include "Engine/Platform/Thread.h"
#include "Engine/Platform/IGuiData.h"
#include "Engine/Platform/Base/PlatformUtils.h"
#include "Engine/Utilities/StringConverter.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include <unistd.h>
#include <cstdint>
#include <stdlib.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <mach/mach_time.h>
#include <mach-o/dyld.h>
#include <uuid/uuid.h>
#include <os/proc.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <IOKit/IOKitLib.h>
#include <dlfcn.h>
#if CRASH_LOG_ENABLE
#include <execinfo.h>
#include <cxxabi.h>
#endif

CPUInfo Cpu;
String UserLocale;
double SecondsPerCycle;
NSAutoreleasePool* AutoreleasePool = nullptr;
int32 AutoreleasePoolInterval = 0;

float ApplePlatform::ScreenScale = 1.0f;

static void CrashHandler(int32 signal, siginfo_t* info, void* context)
{
    // Skip if engine already crashed
    if (Engine::FatalError != FatalErrorType::None)
        return;

    // Get exception info
    String errorMsg(TEXT("Unhandled exception: "));
    switch (signal)
    {
#define CASE(x) case x: errorMsg += TEXT(#x); break
    CASE(SIGABRT);
    CASE(SIGILL);
    CASE(SIGSEGV);
    CASE(SIGQUIT);
    CASE(SIGEMT);
    CASE(SIGFPE);
    CASE(SIGBUS);
    CASE(SIGSYS);
#undef CASE
    default:
        errorMsg += StringUtils::ToString(signal);
    }

    // Log exception and return to the crash location when using debugger
    if (Platform::IsDebuggerPresent())
    {
        LOG_STR(Error, errorMsg);
        const String stackTrace = Platform::GetStackTrace(3, 60, nullptr);
        LOG_STR(Error, stackTrace);
        return;
    }

    // Crash engine
    Platform::Fatal(errorMsg.Get(), nullptr, FatalErrorType::Exception);
}

String AppleUtils::ToString(CFStringRef str)
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

CFStringRef AppleUtils::ToString(const StringView& str)
{
    return CFStringCreateWithBytes(nullptr, (const UInt8*)str.GetText(), str.Length() * sizeof(Char), kCFStringEncodingUTF16LE, false);
}

NSString* AppleUtils::ToNSString(const StringView& str)
{
    NSString* ret = !str.IsEmpty() ? [[NSString alloc] initWithBytes: (const UInt8*)str.Get() length: str.Length() * sizeof(Char) encoding: NSUTF16LittleEndianStringEncoding] : nil;
    return ret ? ret : @"";
}

NSString* AppleUtils::ToNSString(const char* string)
{
    NSString* ret = string ? [NSString stringWithUTF8String: string] : nil;
    return ret ? ret : @"";
}

NSArray* AppleUtils::ParseArguments(NSString* argsString)
{
    NSMutableArray* argsArray = [NSMutableArray array];
    NSMutableString* currentArg = [NSMutableString string];
    BOOL insideQuotes = NO;

    for (NSInteger i = 0; i < argsString.length; i++)
    {
        unichar c = [argsString characterAtIndex:i];
        if (c == '\"')
        {
            if (insideQuotes)
            {
                [argsArray addObject:[currentArg copy]];
                [currentArg setString:@""];
                insideQuotes = NO;
            }
            else
            {
                insideQuotes = YES;
            }
        }
        else if (c == ' ' && !insideQuotes)
        {
            if (currentArg.length > 0)
            {
                [argsArray addObject:[currentArg copy]];
                [currentArg setString:@""];
            }
        }
        else
        {
            [currentArg appendFormat:@"%C", c];
        }
    }
    
    if (currentArg.length > 0)
    {
        [argsArray addObject:[currentArg copy]];
    }

    return [argsArray copy];
}

typedef uint16_t offset_t;
#define align_mem_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

bool ApplePlatform::Is64BitPlatform()
{
    return PLATFORM_64BITS;
}

String ApplePlatform::GetSystemName()
{
	struct utsname systemInfo;
	uname(&systemInfo);
    return String(systemInfo.machine);
}

Version ApplePlatform::GetSystemVersion()
{
    NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
    return Version(version.majorVersion, version.minorVersion, version.patchVersion);
}

CPUInfo ApplePlatform::GetCPUInfo()
{
    return Cpu;
}

MemoryStats ApplePlatform::GetMemoryStats()
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

ProcessMemoryStats ApplePlatform::GetProcessMemoryStats()
{
    ProcessMemoryStats result;
    result.UsedPhysicalMemory = 1024;
    result.UsedVirtualMemory = 1024;
    return result;
}

uint64 ApplePlatform::GetCurrentThreadID()
{
    return (uint64)pthread_mach_thread_np(pthread_self());
}

void ApplePlatform::SetThreadPriority(ThreadPriority priority)
{
    struct sched_param sched;
    Platform::MemoryClear(&sched, sizeof(struct sched_param));
    int32 policy = SCHED_RR;
    pthread_getschedparam(pthread_self(), &policy, &sched);
    sched.sched_priority = AppleThread::GetAppleThreadPriority(priority);
    pthread_setschedparam(pthread_self(), policy, &sched);
}

void ApplePlatform::SetThreadAffinityMask(uint64 affinityMask)
{
#if PLATFORM_MAC
    thread_affinity_policy policy;
    policy.affinity_tag = affinityMask;
    thread_policy_set(pthread_mach_thread_np(pthread_self()), THREAD_AFFINITY_POLICY, (integer_t*)&policy, THREAD_AFFINITY_POLICY_COUNT);
#endif
}

void ApplePlatform::Sleep(int32 milliseconds)
{
    usleep(milliseconds * 1000);
}

double ApplePlatform::GetTimeSeconds()
{
    return SecondsPerCycle * mach_absolute_time();
}

uint64 ApplePlatform::GetTimeCycles()
{
    return mach_absolute_time();
}

uint64 ApplePlatform::GetClockFrequency()
{
    return (uint64)(1.0 / SecondsPerCycle);
}

void ApplePlatform::GetSystemTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond)
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

void ApplePlatform::GetUTCTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond)
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

#if !BUILD_RELEASE

void ApplePlatform::Log(const StringView& msg)
{
#if !USE_EDITOR
    NSLog(@"%s", StringAsANSI<>(*msg, msg.Length()).Get());
#endif
}

bool ApplePlatform::IsDebuggerPresent()
{
    // Reference: https://developer.apple.com/library/archive/qa/qa1361/_index.html
    int mib[4];
    struct kinfo_proc info;

    // Initialize the flags so that, if sysctl fails for some bizarre reason, we get a predictable result
    info.kp_proc.p_flag = 0;

    // Initialize mib, which tells sysctl the info we want, in this case we're looking for information about a specific process ID
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();

    // Call sysctl
    size_t size = sizeof(info);
    sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);

    // We're being debugged if the P_TRACED flag is set
    return ((info.kp_proc.p_flag & P_TRACED) != 0);
}

#endif

bool ApplePlatform::Init()
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
    Cpu.ProcessorPackageCount = value32;
    if (sysctlbyname("hw.physicalcpu", &value32, &value32Size, nullptr, 0) != 0)
        value32 = 1;
    Cpu.ProcessorCoreCount = value32;
    if (sysctlbyname("hw.logicalcpu", &value32, &value32Size, nullptr, 0) != 0)
        value32 = 1;
    Cpu.LogicalProcessorCount = value32;
    if (sysctlbyname("hw.l1icachesize", &value32, &value32Size, nullptr, 0) != 0)
        value32 = 0;
    Cpu.L1CacheSize = value32;
    if (sysctlbyname("hw.l2cachesize", &value32, &value32Size, nullptr, 0) != 0)
        value32 = 0;
    Cpu.L2CacheSize = value32;
    if (sysctlbyname("hw.l3cachesize", &value32, &value32Size, nullptr, 0) != 0)
        value32 = 0;
    Cpu.L3CacheSize = value32;
    if (sysctlbyname("hw.pagesize", &value32, &value32Size, nullptr, 0) != 0)
        value32 = vm_page_size;
    Cpu.PageSize = value32;
    if (sysctlbyname("hw.cpufrequency_max", &value64, &value64Size, nullptr, 0) != 0)
        value64 = GetClockFrequency();
    Cpu.ClockSpeed = value64;
    if (sysctlbyname("hw.cachelinesize", &value32, &value32Size, nullptr, 0) != 0)
        value32 = PLATFORM_CACHE_LINE_SIZE;
    Cpu.CacheLineSize = value32;

    // Get locale
    {
        CFLocaleRef locale = CFLocaleCopyCurrent();
        CFStringRef localeLang = (CFStringRef)CFLocaleGetValue(locale, kCFLocaleLanguageCode);
        CFStringRef localeCountry = (CFStringRef)CFLocaleGetValue(locale, kCFLocaleCountryCode);
        UserLocale = AppleUtils::ToString(localeLang);
        String localeCountryStr = AppleUtils::ToString(localeCountry);
        if (localeCountryStr.HasChars())
            UserLocale += TEXT("-") + localeCountryStr;
        CFRelease(locale);
        CFRelease(localeLang);
        CFRelease(localeCountry);
    }

    // Init user
    {
        String username;
        GetEnvironmentVariable(TEXT("USER"), username);
        OnPlatformUserAdd(New<User>(username));
    }

    // Increase the maximum number of simultaneously open files
    {
        struct rlimit limit;
        limit.rlim_cur = OPEN_MAX;
        limit.rlim_max = RLIM_INFINITY;
        setrlimit(RLIMIT_NOFILE, &limit);
    }

    // Register crash handler
    struct sigaction action;
    Platform::MemoryClear(&action, sizeof(action));
    action.sa_sigaction = CrashHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigaction(SIGABRT, &action, nullptr);
    sigaction(SIGILL, &action, nullptr);
    sigaction(SIGSEGV, &action, nullptr);
    sigaction(SIGQUIT, &action, nullptr);
    sigaction(SIGEMT, &action, nullptr);
    sigaction(SIGFPE, &action, nullptr);
    sigaction(SIGBUS, &action, nullptr);
    sigaction(SIGSYS, &action, nullptr);

    AutoreleasePool = [[NSAutoreleasePool alloc] init];

    return false;
}

void ApplePlatform::Tick()
{
    AutoreleasePoolInterval++;
    if (AutoreleasePoolInterval >= 60)
    {
        AutoreleasePoolInterval = 0;
        [AutoreleasePool drain];
        AutoreleasePool = [[NSAutoreleasePool alloc] init];
    }
}

void ApplePlatform::BeforeExit()
{
}

void ApplePlatform::Exit()
{
    if (AutoreleasePool)
    {
        [AutoreleasePool drain];
        AutoreleasePool = nullptr;
    }
}

void ApplePlatform::SetHighDpiAwarenessEnabled(bool enable)
{
    // Disable resolution scaling in low dpi mode
    if (!enable)
    {
        CustomDpiScale /= ScreenScale;
        ScreenScale = 1.0f;
    }
}

String ApplePlatform::GetUserLocaleName()
{
    return UserLocale;
}

bool ApplePlatform::GetHasFocus()
{
	// Check if any window is focused
	ScopeLock lock(WindowsManager::WindowsLocker);
	for (auto window : WindowsManager::Windows)
	{
		if (window->IsFocused())
			return true;
	}
    return false;
}

void ApplePlatform::CreateGuid(Guid& result)
{
    uuid_t uuid;
    uuid_generate(uuid);
    auto ptr = (uint32*)&uuid;
    result.A = ptr[0];
    result.B = ptr[1];
    result.C = ptr[2];
    result.D = ptr[3];
}

String ApplePlatform::GetExecutableFilePath()
{
    char buf[PATH_MAX];
    uint32 size = PATH_MAX;
    String result;
    if (_NSGetExecutablePath(buf, &size) == 0)
        result.SetUTF8(buf, StringUtils::Length(buf));
    return result;
}

String ApplePlatform::GetWorkingDirectory()
{
	char buffer[256];
	getcwd(buffer, ARRAY_COUNT(buffer));
	return String(buffer);
}

bool ApplePlatform::SetWorkingDirectory(const String& path)
{
    return chdir(StringAsANSI<>(*path).Get()) != 0;
}

bool ApplePlatform::GetEnvironmentVariable(const String& name, String& value)
{
    char* env = getenv(StringAsANSI<>(*name).Get());
    if (env)
    {
        value = String(env);
        return false;
	}
    return true;
}

bool ApplePlatform::SetEnvironmentVariable(const String& name, const String& value)
{
    return setenv(StringAsANSI<>(*name).Get(), StringAsANSI<>(*value).Get(), true) != 0;
}

void* ApplePlatform::LoadLibrary(const Char* filename)
{
    PROFILE_CPU();
    ZoneText(filename, StringUtils::Length(filename));
    const StringAsANSI<> filenameANSI(filename);
    void* result = dlopen(filenameANSI.Get(), RTLD_LAZY | RTLD_LOCAL);
    if (!result)
    {
        LOG(Error, "Failed to load {0} because {1}", filename, String(dlerror()));
    }
    return result;
}

void ApplePlatform::FreeLibrary(void* handle)
{
	dlclose(handle);
}

void* ApplePlatform::GetProcAddress(void* handle, const char* symbol)
{
    return dlsym(handle, symbol);
}

Array<ApplePlatform::StackFrame> ApplePlatform::GetStackFrames(int32 skipCount, int32 maxDepth, void* context)
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
        Array<StringAnsi> parts;
        int32 len;
#define COPY_STR(str, dst) \
    len = Math::Min<int32>(str.Length(), ARRAY_COUNT(frame.dst) - 1); \
    Platform::MemoryCopy(frame.dst, str.Get(), len); \
    frame.dst[len] = 0
        for (int32 i = 0; i < useCount; i++)
        {
            const StringAnsi name(names[i]);
            StackFrame& frame = result[i];
            frame.ProgramCounter = callstack[skipCount + i];
            frame.ModuleName[0] = 0;
            frame.FileName[0] = 0;
            frame.LineNumber = 0;

            // Decode name
            parts.Clear();
            name.Split(' ', parts);
            if (parts.Count() == 6)
            {
                COPY_STR(parts[1], ModuleName);
                const StringAnsiView toDemangle(parts[3]);
                int status = 0;
                char* demangled = __cxxabiv1::__cxa_demangle(*toDemangle, 0, 0, &status);
                const StringAnsiView toCopy = demangled && status == 0 ? StringAnsiView(demangled) : StringAnsiView(toDemangle);
                COPY_STR(toCopy, FunctionName);
                if (demangled)
                    free(demangled);
            }
            else
            {
                COPY_STR(name, FunctionName);
            }
        }
        free(names);
#undef COPY_STR
    }
#endif
    return result;
}

#endif
