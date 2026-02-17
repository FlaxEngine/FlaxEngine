// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_WEB

#include "WebPlatform.h"
#include "WebFileSystem.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/Version.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Platform/CPUInfo.h"
#include "Engine/Platform/MemoryStats.h"
#include "Engine/Profiler/ProfilerCPU.h"
#if !BUILD_RELEASE
#include "Engine/Core/Types/StringView.h"
#include "Engine/Utilities/StringConverter.h"
#endif
#include <chrono>
#include <dlfcn.h>
#include <unistd.h>
#include <emscripten/emscripten.h>
#include <emscripten/threading.h>
#include <emscripten/version.h>
#include <emscripten/heap.h>

namespace
{
    CPUInfo Cpu;
    double DateStart = emscripten_get_now();
};

void WebFileSystem::GetSpecialFolderPath(const SpecialFolder type, String& result)
{
    result = TEXT("/");
}

String WebPlatform::GetSystemName()
{
    return TEXT("Browser");
}

Version WebPlatform::GetSystemVersion()
{
    return Version(1, 0);
}

CPUInfo WebPlatform::GetCPUInfo()
{
    return Cpu;
}

MemoryStats WebPlatform::GetMemoryStats()
{
    MemoryStats result;
    result.TotalPhysicalMemory = emscripten_get_heap_max();
    result.UsedPhysicalMemory = emscripten_get_heap_size();
    result.TotalVirtualMemory = 2ull * 1024 * 1024 * 1024; // Max 2GB
    result.UsedVirtualMemory = result.UsedPhysicalMemory;
    result.ProgramSizeMemory = 0;
    return result;
}

ProcessMemoryStats WebPlatform::GetProcessMemoryStats()
{
    // Mock memory stats
    ProcessMemoryStats result;
    result.UsedPhysicalMemory = 1 * 1024 * 1024;
    result.UsedVirtualMemory = result.UsedPhysicalMemory;
    return result;
}

void WebPlatform::SetThreadPriority(ThreadPriority priority)
{
    // Not supported
}

void WebPlatform::SetThreadAffinityMask(uint64 affinityMask)
{
    // Not supported
}

void WebPlatform::Sleep(int32 milliseconds)
{
    //emscripten_sleep(milliseconds);
    emscripten_thread_sleep(milliseconds);
}

void WebPlatform::Yield()
{
    Sleep(0);
}

double WebPlatform::GetTimeSeconds()
{
    double time = emscripten_get_now();
    return time * 0.001;
}

uint64 WebPlatform::GetTimeCycles()
{
    return (uint64)(emscripten_get_now() * 1000.0);
}

void WebPlatform::GetSystemTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond)
{
    // Get local time
    using namespace std::chrono;
    system_clock::time_point now = system_clock::now();
    time_t tt = system_clock::to_time_t(now);
    tm time = *localtime(&tt);

    // Extract time
    year = time.tm_year + 1900;
    month = time.tm_mon + 1;
    dayOfWeek = time.tm_wday;
    day = time.tm_mday;
    hour = time.tm_hour;
    minute = time.tm_min;
    second = time.tm_sec;
    millisecond = (int64)(emscripten_get_now() - DateStart) % 1000; // Fake it based on other timer
}

void WebPlatform::GetUTCTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond)
{
    // Get UTC time
    using namespace std::chrono;
    system_clock::time_point now = system_clock::now();
    time_t tt = system_clock::to_time_t(now);
    tm time = *gmtime(&tt);

    // Extract time
    year = time.tm_year + 1900;
    month = time.tm_mon + 1;
    dayOfWeek = time.tm_wday;
    day = time.tm_mday;
    hour = time.tm_hour;
    minute = time.tm_min;
    second = time.tm_sec;
    millisecond = (int64)(emscripten_get_now() - DateStart) % 1000; // Fake it based on other timer
}

#if !BUILD_RELEASE

void WebPlatform::Log(const StringView& msg)
{
    const StringAsANSI<512> msgAnsi(*msg, msg.Length());

    // Fix % characters that should not be formatted
    auto buffer = (char*)msgAnsi.Get();
    for (int32 i = 0; buffer[i]; i++)
    {
        if (buffer[i] == '%')
            buffer[i] = 'p';
    }

    emscripten_log(EM_LOG_CONSOLE, buffer);
}

bool WebPlatform::IsDebuggerPresent()
{
    return false;
}

#endif

String WebPlatform::GetComputerName()
{
    return TEXT("Web");
}

bool WebPlatform::GetHasFocus()
{
    return true;
}

String WebPlatform::GetMainDirectory()
{
    return TEXT("/");
}

String WebPlatform::GetExecutableFilePath()
{
    return TEXT("/index.html");
}

Guid WebPlatform::GetUniqueDeviceId()
{
    return Guid(1, 2, 3, 4);
}

String WebPlatform::GetWorkingDirectory()
{
    return GetMainDirectory();
}

bool WebPlatform::SetWorkingDirectory(const String& path)
{
    return true;
}

bool WebPlatform::Init()
{
    if (PlatformBase::Init())
        return true;

    // Set info about the CPU
    Platform::MemoryClear(&Cpu, sizeof(Cpu));
    Cpu.ProcessorPackageCount = 1;
    Cpu.ProcessorCoreCount = Math::Min(emscripten_num_logical_cores(), PLATFORM_THREADS_LIMIT);
    Cpu.LogicalProcessorCount = Cpu.ProcessorCoreCount;
    Cpu.ClockSpeed = GetClockFrequency();

    return false;
}

void WebPlatform::LogInfo()
{
    PlatformBase::LogInfo();

#ifdef __EMSCRIPTEN_major__
    LOG(Info, "Emscripten {}.{}.{}", __EMSCRIPTEN_major__, __EMSCRIPTEN_minor__, __EMSCRIPTEN_tiny__);
#elif defined(__EMSCRIPTEN_MAJOR__)
    LOG(Info, "Emscripten {}.{}.{}", __EMSCRIPTEN_MAJOR__, __EMSCRIPTEN_MINOR__, __EMSCRIPTEN_TINY__);
#else
    LOG(Info, "Emscripten");
#endif
#ifdef __EMSCRIPTEN_PTHREADS__
    LOG(Info, "Threading: pthreads");
#else
    LOG(Info, "Threading: disabled");
#endif
}

void WebPlatform::Tick()
{
}

void WebPlatform::Exit()
{
}

extern char** environ;

void WebPlatform::GetEnvironmentVariables(Dictionary<String, String, HeapAllocation>& result)
{
    char** s = environ;
    for (; *s; s++)
    {
        char* var = *s;
        int32 split = -1;
        for (int32 i = 0; var[i]; i++)
        {
            if (var[i] == '=')
            {
                split = i;
                break;
            }
        }
        if (split == -1)
            result[String(var)] = String::Empty;
        else
            result[String(var, split)] = String(var + split + 1);
    }
}

bool WebPlatform::GetEnvironmentVariable(const String& name, String& value)
{
    char* env = getenv(StringAsANSI<>(*name).Get());
    if (env)
    {
        value = String(env);
        return false;
    }
    return true;
}

bool WebPlatform::SetEnvironmentVariable(const String& name, const String& value)
{
    return setenv(StringAsANSI<>(*name).Get(), StringAsANSI<>(*value).Get(), true) != 0;
}

void* WebPlatform::LoadLibrary(const Char* filename)
{
    PROFILE_CPU();
    ZoneText(filename, StringUtils::Length(filename));
    const StringAsANSI<> filenameANSI(filename);
    void* result = dlopen(filenameANSI.Get(), RTLD_NOW);
    if (!result)
    {
        LOG(Error, "Failed to load {0} because {1}", filename, String(dlerror()));
    }
    return result;
}

void WebPlatform::FreeLibrary(void* handle)
{
    dlclose(handle);
}

void* WebPlatform::GetProcAddress(void* handle, const char* symbol)
{
    return dlsym(handle, symbol);
}

#endif
