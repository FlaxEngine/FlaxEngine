// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if PLATFORM_MAC

#include "MacPlatform.h"
#include "MacWindow.h"
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

CPUInfo MacCpu;
Guid DeviceId;
String UserLocale, ComputerName;
byte MacAddress[6];

DialogResult MessageBox::Show(Window* parent, const StringView& text, const StringView& caption, MessageBoxButtons buttons, MessageBoxIcon icon)
{
    if (CommandLine::Options.Headless)
        return DialogResult::None;
	todo;
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
    void SetMousePosition(const Vector2& newPosition) final override
    {
        MacPlatform::SetMousePosition(newPosition);

        OnMouseMoved(newPosition);
    }
};

typedef uint16_t offset_t;
#define align_mem_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

void* MacPlatform::Allocate(uint64 size, uint64 alignment)
{
    void* ptr = nullptr;

    // Alignment always has to be power of two
    ASSERT_LOW_LAYER((alignment & (alignment - 1)) == 0);

    if (alignment && size)
    {
        uint32_t pad = sizeof(offset_t) + (alignment - 1);
        void* p = malloc(size + pad);
        if (p)
        {
            // Add the offset size to malloc's pointer
            ptr = (void*)align_mem_up(((uintptr_t)p + sizeof(offset_t)), alignment);

            // Calculate the offset and store it behind aligned pointer
            *((offset_t*)ptr - 1) = (offset_t)((uintptr_t)ptr - (uintptr_t)p);
        }
#if COMPILE_WITH_PROFILER
        OnMemoryAlloc(ptr, size);
#endif
    }
    return ptr;
}

void MacPlatform::Free(void* ptr)
{
    if (ptr)
    {
#if COMPILE_WITH_PROFILER
        OnMemoryFree(ptr);
#endif
        // Walk backwards from the passed-in pointer to get the pointer offset
        offset_t offset = *((offset_t*)ptr - 1);

        // Get original pointer
        void* p = (void*)((uint8_t*)ptr - offset);

        // Free memory
        free(p);
    }
}

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
    MISSING_CODE("MacPlatform::GetMemoryStats");
    return MemoryStats(); // TODO: platform stats on Mac
}

ProcessMemoryStats MacPlatform::GetProcessMemoryStats()
{
    MISSING_CODE("MacPlatform::GetProcessMemoryStats");
    return ProcessMemoryStats(); // TODO: platform stats on Mac
}

uint64 UnixPlatform::GetCurrentProcessId()
{
    return getpid();
}

uint64 MacPlatform::GetCurrentThreadID()
{
    MISSING_CODE("MacPlatform::GetCurrentThreadID");
    return 0; // TODO: threading on Mac
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
    MISSING_CODE("MacPlatform::Sleep");
    return; // TODO: clock on Mac
}

double MacPlatform::GetTimeSeconds()
{
    MISSING_CODE("MacPlatform::GetTimeSeconds");
    return 0.0; // TODO: clock on Mac
}

uint64 MacPlatform::GetTimeCycles()
{
    MISSING_CODE("MacPlatform::GetTimeCycles");
    return 0; // TODO: clock on Mac
}

uint64 MacPlatform::GetClockFrequency()
{
    MISSING_CODE("MacPlatform::GetClockFrequency");
    return 0; // TODO: clock on Mac
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
    if (PlatformBase::Init())
        return true;
    
    // TODO: get MacCpu

    // TODO: get MacAddress
    // TODO: get DeviceId

    // TODO: get username
    OnPlatformUserAdd(New<User>(TEXT("User")));

    // TODO: get UserLocale
    // TODO: get ComputerName

    Input::Mouse = Impl::Mouse = New<MacMouse>();
    Input::Keyboard = Impl::Keyboard = New<MacKeyboard>();

    return false;
}

void MacPlatform::BeforeRun()
{
}

void MacPlatform::Tick()
{
    // TODO: app events
}

void MacPlatform::BeforeExit()
{
}

void MacPlatform::Exit()
{
}

int32 MacPlatform::GetDpi()
{
    return todo;
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

bool MacPlatform::CanOpenUrl(const StringView& url)
{
    return false;
}

void MacPlatform::OpenUrl(const StringView& url)
{
}

Vector2 MacPlatform::GetMousePosition()
{
    MISSING_CODE("MacPlatform::GetMousePosition");
    return Vector2(0, 0); // TODO: mouse on Mac
}

void MacPlatform::SetMousePosition(const Vector2& pos)
{
    MISSING_CODE("MacPlatform::SetMousePosition");
    // TODO: mouse on Mac
}

Vector2 MacPlatform::GetDesktopSize()
{
    MISSING_CODE("MacPlatform::GetDesktopSize");
    return Vector2(0, 0); // TODO: desktop size on Mac
}

Rectangle MacPlatform::GetMonitorBounds(const Vector2& screenPos)
{
	// TODO: do it in a proper way
	return Rectangle(Vector2::Zero, GetDesktopSize());
}

Rectangle MacPlatform::GetVirtualDesktopBounds()
{
	// TODO: do it in a proper way
	return Rectangle(Vector2::Zero, GetDesktopSize());
}

String MacPlatform::GetMainDirectory()
{
    MISSING_CODE("MacPlatform::GetMainDirectory");
    return TEXT("/"); // TODO: GetMainDirectory
}

String MacPlatform::GetExecutableFilePath()
{
    MISSING_CODE("MacPlatform::GetExecutableFilePath");
    return TEXT("/"); // TODO: GetMainDirectory
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

void* MacPlatform::LoadLibrary(const Char* filename)
{
    MISSING_CODE("MacPlatform::LoadLibrary");
    return nullptr; // TODO: dynamic libs on Mac
}

void MacPlatform::FreeLibrary(void* handle)
{
    MISSING_CODE("MacPlatform::FreeLibrary");
    return; // TODO: dynamic libs on Mac
}

void* MacPlatform::GetProcAddress(void* handle, const char* symbol)
{
    MISSING_CODE("MacPlatform::GetProcAddress");
    return nullptr; // TODO: dynamic libs on Mac
}

#endif
