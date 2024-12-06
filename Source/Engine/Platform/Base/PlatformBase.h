// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Types.h"
#include "Engine/Core/Types/BaseTypes.h"
#include <string.h>

struct Guid;
struct CPUInfo;
struct MemoryStats;
struct ProcessMemoryStats;
struct CreateProcessSettings;
struct CreateWindowSettings;
struct BatteryInfo;

// ReSharper disable CppFunctionIsNotImplemented

/// <summary>
/// Network connection types for device.
/// </summary>
API_ENUM() enum class NetworkConnectionType
{
    /// <summary>
    /// No connection.
    /// </summary>
    None,

    /// <summary>
    /// The unknown connection type.
    /// </summary>
    Unknown,

    /// <summary>
    /// The airplane mode.
    /// </summary>
    AirplaneMode,

    /// <summary>
    /// The cell connection.
    /// </summary>
    Cell,

    /// <summary>
    /// The WiFi connection.
    /// </summary>
    WiFi,

    /// <summary>
    /// The Bluetooth connection.
    /// </summary>
    Bluetooth,

    /// <summary>
    /// The Ethernet cable connection (LAN).
    /// </summary>
    Ethernet,
};

extern FLAXENGINE_API const Char* ToString(NetworkConnectionType value);

/// <summary>
/// The device screen orientation types (eg. portrait, landscape, etc.).
/// </summary>
API_ENUM() enum class ScreenOrientationType
{
    /// <summary>
    /// The unknown orientation type.
    /// </summary>
    Unknown,

    /// <summary>
    /// The portrait screen orientation with device bottom on the bottom side of the screen.
    /// </summary>
    Portrait,

    /// <summary>
    /// The portrait screen orientation but upside down with device bottom on the top side of the screen.
    /// </summary>
    PortraitUpsideDown,

    /// <summary>
    /// The landscape screen orientation with device bottom on the right side of the screen (device rotated to the left from the portrait).
    /// </summary>
    LandscapeLeft,

    /// <summary>
    /// The landscape screen orientation with device bottom on the left side of the screen (device rotated to the right from the portrait).
    /// </summary>
    LandscapeRight,
};

extern FLAXENGINE_API const Char* ToString(ScreenOrientationType value);

/// <summary>
/// Thread priority levels.
/// </summary>
enum class ThreadPriority
{
    /// <summary>
    /// The normal level.
    /// </summary>
    Normal,

    /// <summary>
    /// The above normal level.
    /// </summary>
    AboveNormal,

    /// <summary>
    /// The below normal level.
    /// </summary>
    BelowNormal,

    /// <summary>
    /// The highest level.
    /// </summary>
    Highest,

    /// <summary>
    /// The lowest level.
    /// </summary>
    Lowest,
};

extern FLAXENGINE_API const Char* ToString(ThreadPriority value);

API_INJECT_CODE(cpp, "#include \"Engine/Platform/Platform.h\"");

/// <summary>
/// Runtime platform service.
/// </summary>
API_CLASS(Static, Name="Platform", Tag="NativeInvokeUseName")
class FLAXENGINE_API PlatformBase
{
DECLARE_SCRIPTING_TYPE_MINIMAL(PlatformBase);

    /// <summary>
    /// Initializes the runtime platform service. Called on very beginning pf the engine startup.
    /// </summary>
    /// <returns>True if failed ot initialize platform, otherwise false.</returns>
    static bool Init();

    /// <summary>
    /// Writes the platform info to the log. Called after platform and logging service init but before engine services initialization.
    /// </summary>
    static void LogInfo();

    /// <summary>
    /// Called just before main game loop start.
    /// </summary>
    static void BeforeRun();

    /// <summary>
    /// Tick platform from game loop by main thread.
    /// </summary>
    static void Tick();

    /// <summary>
    /// Called before engine exit to pre dispose platform service.
    /// </summary>
    static void BeforeExit();

    /// <summary>
    /// Called after engine exit to shutdown platform service.
    /// </summary>
    static void Exit();

public:

    /// <summary>
    /// Application windows class name.
    /// </summary>
    static const Char* ApplicationClassName;

public:
    /// <summary>
    /// Copy memory region
    /// </summary>
    /// <param name="dst">Destination memory address. Must not be null, even if size is zero.</param>
    /// <param name="src">Source memory address. Must not be null, even if size is zero.</param>
    /// <param name="size">Size of the memory to copy in bytes</param>
    FORCE_INLINE static void MemoryCopy(void* dst, const void* src, uint64 size)
    {
        memcpy(dst, src, static_cast<size_t>(size));
    }

    /// <summary>
    /// Set memory region with given value
    /// </summary>
    /// <param name="dst">Destination memory address. Must not be null, even if size is zero.</param>
    /// <param name="size">Size of the memory to set in bytes</param>
    /// <param name="value">Value to set</param>
    FORCE_INLINE static void MemorySet(void* dst, uint64 size, int32 value)
    {
        memset(dst, value, static_cast<size_t>(size));
    }

    /// <summary>
    /// Clear memory region with zeros
    /// </summary>
    /// <param name="dst">Destination memory address. Must not be null, even if size is zero.</param>
    /// <param name="size">Size of the memory to clear in bytes</param>
    FORCE_INLINE static void MemoryClear(void* dst, uint64 size)
    {
        memset(dst, 0, static_cast<size_t>(size));
    }

    /// <summary>
    /// Compare two blocks of the memory.
    /// </summary>
    /// <param name="buf1">The first buffer address. Must not be null, even if size is zero.</param>
    /// <param name="buf2">The second buffer address. Must not be null, even if size is zero.</param>
    /// <param name="size">Size of the memory to compare in bytes.</param>
    FORCE_INLINE static int32 MemoryCompare(const void* buf1, const void* buf2, uint64 size)
    {
        return memcmp(buf1, buf2, static_cast<size_t>(size));
    }

    /// <summary>
    /// Creates a hardware memory barrier (fence) that prevents the CPU from re-ordering read and write operations
    /// </summary>
    static void MemoryBarrier() = delete;

    /// <summary>
    /// Sets a 64-bit variable to the specified value as an atomic operation. The function prevents more than one thread from using the same variable simultaneously.
    /// </summary>
    /// <param name="dst">A pointer to the first operand. This value will be replaced with the result of the operation.</param>
    /// <param name="exchange">The value to exchange.</param>
    /// <returns>The original value of the dst parameter.</returns>
    static int64 InterlockedExchange(int64 volatile* dst, int64 exchange) = delete;

    /// <summary>
    /// Performs an atomic compare-and-exchange operation on the specified values. The function compares two specified 32-bit values and exchanges with another 32-bit value based on the outcome of the comparison.
    /// </summary>
    /// <remarks>The function compares the dst value with the comperand value. If the dst value is equal to the comperand value, the value value is stored in the address specified by dst. Otherwise, no operation is performed.</remarks>
    /// <param name="dst">A pointer to the first operand. This value will be replaced with the result of the operation.</param>
    /// <param name="exchange">The value to exchange.</param>
    /// <param name="comperand">The value to compare to destination.</param>
    /// <returns>The original value of the dst parameter.</returns>
    static int32 InterlockedCompareExchange(int32 volatile* dst, int32 exchange, int32 comperand) = delete;

    /// <summary>
    /// Performs an atomic compare-and-exchange operation on the specified values. The function compares two specified 64-bit values and exchanges with another 64-bit value based on the outcome of the comparison.
    /// </summary>
    /// <remarks>The function compares the dst value with the comperand value. If the dst value is equal to the comperand value, the value value is stored in the address specified by dst. Otherwise, no operation is performed.</remarks>
    /// <param name="dst">A pointer to the first operand. This value will be replaced with the result of the operation.</param>
    /// <param name="exchange">The value to exchange.</param>
    /// <param name="comperand">The value to compare to destination.</param>
    /// <returns>The original value of the dst parameter.</returns>
    static int64 InterlockedCompareExchange(int64 volatile* dst, int64 exchange, int64 comperand) = delete;

    /// <summary>
    /// Increments (increases by one) the value of the specified variable as an atomic operation.
    /// </summary>
    /// <param name="dst">A pointer to the variable to be incremented.</param>
    /// <returns>The incremented value of the dst parameter.</returns>
    static int64 InterlockedIncrement(int64 volatile* dst) = delete;

    /// <summary>
    /// Decrements (decreases by one) the value of the specified variable as an atomic operation.
    /// </summary>
    /// <param name="dst">A pointer to the variable to be decremented.</param>
    /// <returns>The decremented value of the decremented parameter.</returns>
    static int64 InterlockedDecrement(int64 volatile* dst) = delete;

    /// <summary>
    /// Adds value to the value of the specified variable as an atomic operation.
    /// </summary>
    /// <param name="dst">A pointer to the first operand. This value will be replaced with the result of the operation.</param>
    /// <param name="value">The second operand.</param>
    /// <returns>The result value of the operation.</returns>
    static int64 InterlockedAdd(int64 volatile* dst, int64 value) = delete;

    /// <summary>
    /// Performs an atomic 32-bit variable read operation on the specified values.
    /// </summary>
    /// <param name="dst">A pointer to the destination value.</param>
    /// <returns>The function returns the value of the destination parameter.</returns>
    static int32 AtomicRead(int32 const volatile* dst) = delete;

    /// <summary>
    /// Performs an atomic 64-bit variable read operation on the specified values.
    /// </summary>
    /// <param name="dst">A pointer to the destination value.</param>
    /// <returns>The function returns the value of the destination parameter.</returns>
    static int64 AtomicRead(int64 const volatile* dst) = delete;

    /// <summary>
    /// Sets a 32-bit variable to the specified value as an atomic operation.
    /// </summary>
    /// <param name="dst">A pointer to the value to be exchanged.</param>
    /// <param name="value">The value to be set.</param>
    static void AtomicStore(int32 volatile* dst, int32 value) = delete;

    /// <summary>
    /// Sets a 64-bit variable to the specified value as an atomic operation.
    /// </summary>
    /// <param name="dst">A pointer to the value to be exchanged.</param>
    /// <param name="value">The value to be set.</param>
    static void AtomicStore(int64 volatile* dst, int64 value) = delete;

    /// <summary>
    /// Indicates to the processor that a cache line will be needed in the near future.
    /// </summary>
    /// <param name="ptr">The address of the cache line to be loaded. This address is not required to be on a cache line boundary.</param>
    static void Prefetch(void const* ptr) = delete;

#if COMPILE_WITH_PROFILER
    static void OnMemoryAlloc(void* ptr, uint64 size);
    static void OnMemoryFree(void* ptr);
#endif

    /// <summary>
    /// Allocates memory on a specified alignment boundary.
    /// </summary>
    /// <param name="size">The size of the allocation (in bytes).</param>
    /// <param name="alignment">The memory alignment (in bytes). Must be an integer power of 2.</param>
    /// <returns>The pointer to the allocated chunk of the memory. The pointer is a multiple of alignment.</returns>
    static void* Allocate(uint64 size, uint64 alignment) = delete;

    /// <summary>
    /// Frees a block of allocated memory.
    /// </summary>
    /// <param name="ptr">A pointer to the memory block to deallocate.</param>
    static void Free(void* ptr) = delete;

    /// <summary>
    /// Allocates pages memory block.
    /// </summary>
    /// <param name="numPages">The number of pages to allocate.</param>
    /// <param name="pageSize">The size of single page. Use Platform::GetCPUInfo().PageSize or provide compatible, custom size.</param>
    /// <returns>The pointer to the allocated pages in memory.</returns>
    static void* AllocatePages(uint64 numPages, uint64 pageSize);

    /// <summary>
    /// Frees allocated pages memory block.
    /// </summary>
    /// <param name="ptr">The pointer to the pages to deallocate.</param>
    static void FreePages(void* ptr);

public:
    /// <summary>
    /// Returns the current runtime platform type. It's compile-time constant.
    /// </summary>
    API_PROPERTY() static PlatformType GetPlatformType();

    /// <summary>
    /// Returns true if is running 64 bit application (otherwise 32 bit). It's compile-time constant.
    /// </summary>
    API_PROPERTY() static bool Is64BitApp();

    /// <summary>
    /// Returns true if running on 64-bit computer
    /// </summary>
    /// <returns>True if running on 64-bit computer, otherwise false.</returns>
    API_PROPERTY() static bool Is64BitPlatform() = delete;

    /// <summary>
    /// Gets the CPU information.
    /// </summary>
    /// <returns>The CPU info.</returns>
    API_PROPERTY() static CPUInfo GetCPUInfo() = delete;

    /// <summary>
    /// Gets the CPU cache line size.
    /// </summary>
    /// <returns>The cache line size.</returns>
    API_PROPERTY() static int32 GetCacheLineSize() = delete;

    /// <summary>
    /// Gets the current memory stats.
    /// </summary>
    /// <returns>The memory stats.</returns>
    API_PROPERTY() static MemoryStats GetMemoryStats() = delete;

    /// <summary>
    /// Gets the process current memory stats.
    /// </summary>
    /// <returns>The process memory stats.</returns>
    API_PROPERTY() static ProcessMemoryStats GetProcessMemoryStats() = delete;

    /// <summary>
    /// Gets the current process unique identifier.
    /// </summary>
    /// <returns>The process id.</returns>
    API_PROPERTY() static uint64 GetCurrentProcessId() = delete;

    /// <summary>
    /// Gets the current thread unique identifier.
    /// </summary>
    /// <returns>The thread id.</returns>
    API_PROPERTY() static uint64 GetCurrentThreadID() = delete;

    /// <summary>
    /// Sets the current thread priority.
    /// </summary>
    /// <param name="priority">The priority.</param>
    static void SetThreadPriority(ThreadPriority priority) = delete;

    /// <summary>
    /// Sets a processor affinity mask for the specified thread.
    /// </summary>
    /// <remarks>
    /// A thread affinity mask is a bit vector in which each bit represents a logical processor that a thread is allowed to run on. A thread affinity mask must be a subset of the process affinity mask for the containing process of a thread. A thread can only run on the processors its process can run on. Therefore, the thread affinity mask cannot specify a 1 bit for a processor when the process affinity mask specifies a 0 bit for that processor.
    /// </remarks>
    /// <param name="affinityMask">The affinity mask for the thread.</param>
    static void SetThreadAffinityMask(uint64 affinityMask) = delete;

    /// <summary>
    /// Suspends the execution of the current thread until the time-out interval elapses
    /// </summary>
    /// <param name="milliseconds">The time interval for which execution is to be suspended, in milliseconds.</param>
    static void Sleep(int32 milliseconds) = delete;

public:
    /// <summary>
    /// Gets the current time in seconds.
    /// </summary>
    /// <returns>The current time.</returns>
    API_PROPERTY() static double GetTimeSeconds() = delete;

    /// <summary>
    /// Gets the current time as CPU cycles counter.
    /// </summary>
    /// <returns>The CPU cycles counter value.</returns>
    API_PROPERTY() static uint64 GetTimeCycles() = delete;

    /// <summary>
    /// Gets the system clock frequency.
    /// </summary>
    /// <returns>The clock frequency.</returns>
    API_PROPERTY() static uint64 GetClockFrequency() = delete;

    /// <summary>
    /// Gets current system time based on current computer settings.
    /// </summary>
    /// <param name="year">The result year value.</param>
    /// <param name="month">The result month value.</param>
    /// <param name="dayOfWeek">The result day of the week value.</param>
    /// <param name="day">The result day value.</param>
    /// <param name="hour">The result hour value.</param>
    /// <param name="minute">The result minute value.</param>
    /// <param name="second">The result second value.</param>
    /// <param name="millisecond">The result millisecond value.</param>
    static void GetSystemTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond) = delete;

    /// <summary>
    /// Gets current UTC time based on local computer settings.
    /// </summary>
    /// <param name="year">The result year value.</param>
    /// <param name="month">The result month value.</param>
    /// <param name="dayOfWeek">The result day of the week value.</param>
    /// <param name="day">The result day value.</param>
    /// <param name="hour">The result hour value.</param>
    /// <param name="minute">The result minute value.</param>
    /// <param name="second">The result second value.</param>
    /// <param name="millisecond">The result millisecond value.</param>
    static void GetUTCTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond) = delete;

public:
    /// <summary>
    /// Shows the fatal error message to the user.
    /// </summary>
    /// <param name="msg">The message content.</param>
    /// <param name="context">The platform-dependent context for the stack trace collecting (eg. platform exception info).</param>
    static void Fatal(const Char* msg, void* context = nullptr);

    /// <summary>
    /// Shows the error message to the user.
    /// </summary>
    /// <param name="msg">The message content.</param>
    static void Error(const Char* msg);

    /// <summary>
    /// Shows the warning message to the user.
    /// </summary>
    /// <param name="msg">The message content.</param>
    static void Warning(const Char* msg);

    /// <summary>
    /// Shows the information message to the user.
    /// </summary>
    /// <param name="msg">The message content.</param>
    static void Info(const Char* msg);

public:
    /// <summary>
    /// Shows the fatal error message to the user.
    /// </summary>
    /// <param name="msg">The message content.</param>
    API_FUNCTION() static void Fatal(const StringView& msg);

    /// <summary>
    /// Shows the error message to the user.
    /// </summary>
    /// <param name="msg">The message content.</param>
    API_FUNCTION() static void Error(const StringView& msg);

    /// <summary>
    /// Shows the warning message to the user.
    /// </summary>
    /// <param name="msg">The message content.</param>
    API_FUNCTION() static void Warning(const StringView& msg);

    /// <summary>
    /// Shows the information message to the user.
    /// </summary>
    /// <param name="msg">The message content.</param>
    API_FUNCTION() static void Info(const StringView& msg);

    /// <summary>
    /// Logs the specified message to the platform-dependant logging stream.
    /// </summary>
    /// <param name="msg">The message.</param>
    static void Log(const StringView& msg);

    /// <summary>
    /// Checks whenever program is running with debugger attached.
    /// </summary>
    /// <returns>True if debugger is present, otherwise false.</returns>
    static bool IsDebuggerPresent();

public:
    /// <summary>
    /// Performs a fatal crash.
    /// </summary>
    /// <param name="line">The source line.</param>
    /// <param name="file">The source file.</param>
    NO_RETURN static void Crash(int32 line, const char* file);

    /// <summary>
    /// Performs a fatal crash occurred on memory allocation fail.
    /// </summary>
    /// <param name="line">The source line.</param>
    /// <param name="file">The source file.</param>
    NO_RETURN static void OutOfMemory(int32 line, const char* file);

    /// <summary>
    /// Performs a fatal crash due to code not being implemented.
    /// </summary>
    /// <param name="line">The source line.</param>
    /// <param name="file">The source file.</param>
    /// <param name="info">The additional info.</param>
    NO_RETURN static void MissingCode(int32 line, const char* file, const char* info);

    /// <summary>
    /// Performs a fatal crash due to assertion fail.
    /// </summary>
    /// <param name="message">The assertion message.</param>
    /// <param name="line">The source line.</param>
    /// <param name="file">The source file.</param>
    NO_RETURN static void Assert(const char* message, const char* file, int line);

    /// <summary>
    /// Performs a error message log due to runtime value check fail.
    /// </summary>
    /// <param name="message">The assertion message.</param>
    /// <param name="line">The source line.</param>
    /// <param name="file">The source file.</param>
    static void CheckFailed(const char* message, const char* file, int line);

public:
    /// <summary>
    /// Sets the High DPI awareness.
    /// </summary>
    static void SetHighDpiAwarenessEnabled(bool enable);

    /// <summary>
    /// Gets the battery information.
    /// </summary>
    API_PROPERTY() static BatteryInfo GetBatteryInfo();

    /// <summary>
    /// Gets the primary monitor's DPI setting.
    /// </summary>
    API_PROPERTY() static int32 GetDpi();

    /// <summary>
    /// Gets the primary monitor's DPI setting scale factor (1 is default). Includes custom DPI scale.
    /// </summary>
    API_PROPERTY() static float GetDpiScale();

    /// <summary>
    /// The custom DPI scale factor to apply globally. Can be used to adjust the User Interface scale (resolution).
    /// </summary>
    API_FIELD() static float CustomDpiScale;

    /// <summary>
    /// Gets the current network connection type.
    /// </summary>
    API_PROPERTY() static NetworkConnectionType GetNetworkConnectionType();

    /// <summary>
    /// Gets the current screen orientation type.
    /// </summary>
    API_PROPERTY() static ScreenOrientationType GetScreenOrientationType();

    /// <summary>
    /// Gets the current locale culture (eg. "pl-PL" or "en-US").
    /// </summary>
    API_PROPERTY() static String GetUserLocaleName() = delete;

    /// <summary>
    /// Gets the computer machine name.
    /// </summary>
    API_PROPERTY() static String GetComputerName() = delete;

    /// <summary>
    /// Gets the user name.
    /// </summary>
    API_PROPERTY() static String GetUserName();

    /// <summary>
    /// Returns true if app has user focus.
    /// </summary>
    API_PROPERTY() static bool GetHasFocus() = delete;

    /// <summary>
    /// Returns true if app is paused. Engine ticking (update/physics/drawing) is disabled in that state, only platform is updated until app end or resume.
    /// </summary>
    static bool GetIsPaused();

    /// <summary>
    /// Creates the unique identifier.
    /// </summary>
    /// <param name="result">The result.</param>
    static void CreateGuid(Guid& result);

public:
    /// <summary>
    /// The list of users.
    /// </summary>
    API_FIELD(ReadOnly) static Array<User*, FixedAllocation<8>> Users;

    /// <summary>
    /// Event called when user gets added (eg. logged in).
    /// </summary>
    API_EVENT() static Delegate<User*> UserAdded;

    /// <summary>
    /// Event called when user gets removed (eg. logged out).
    /// </summary>
    API_EVENT() static Delegate<User*> UserRemoved;

public:
    /// <summary>
    /// Returns a value indicating whether can open a given URL in a web browser.
    /// </summary>
    /// <param name="url">The URI to assign to web browser.</param>
    /// <returns>True if can open URL, otherwise false.</returns>
    API_FUNCTION() static bool CanOpenUrl(const StringView& url);

    /// <summary>
    /// Launches a web browser and opens a given URL.
    /// </summary>
    /// <param name="url">The URI to assign to web browser.</param>
    API_FUNCTION() static void OpenUrl(const StringView& url);

public:
    /// <summary>
    /// Gets the mouse cursor position in screen-space coordinates.
    /// </summary>
    /// <returns>Mouse cursor coordinates.</returns>
    API_PROPERTY() static Float2 GetMousePosition();

    /// <summary>
    /// Sets the mouse cursor position in screen-space coordinates.
    /// </summary>
    /// <param name="position">Cursor position to set.</param>
    API_PROPERTY() static void SetMousePosition(const Float2& position);

    /// <summary>
    /// Gets the origin position and size of the monitor at the given screen-space location.
    /// </summary>
    /// <param name="screenPos">The screen position (in pixels).</param>
    /// <returns>The monitor bounds.</returns>
    API_FUNCTION() static Rectangle GetMonitorBounds(const Float2& screenPos);

    /// <summary>
    /// Gets size of the primary desktop.
    /// </summary>
    /// <returns>Desktop size.</returns>
    API_PROPERTY() static Float2 GetDesktopSize() = delete;

    /// <summary>
    /// Gets virtual bounds of the desktop made of all the monitors outputs attached.
    /// </summary>
    /// <returns>Whole desktop size.</returns>
    API_PROPERTY() static Rectangle GetVirtualDesktopBounds();

    /// <summary>
    /// Gets virtual size of the desktop made of all the monitors outputs attached.
    /// </summary>
    API_PROPERTY() static Float2 GetVirtualDesktopSize();

public:
    /// <summary>
    /// Gets full path of the main engine directory.
    /// </summary>
    /// <returns>Main engine directory path</returns>
    API_PROPERTY() static String GetMainDirectory() = delete;

    /// <summary>
    /// Gets full path of the main engine executable file.
    /// </summary>
    /// <returns>The main engine executable file path.</returns>
    API_PROPERTY() static String GetExecutableFilePath() = delete;

    /// <summary>
    /// Gets the (almost) unique ID of the current user device.
    /// </summary>
    /// <returns>ID of the current user device</returns>
    API_PROPERTY() static Guid GetUniqueDeviceId() = delete;

    /// <summary>
    /// Gets the current working directory of the process.
    /// </summary>
    /// <returns>The workspace directory path.</returns>
    API_PROPERTY() static String GetWorkingDirectory() = delete;

    /// <summary>
    /// Sets the current working directory of the process.
    /// </summary>
    /// <param name="path">The directory to set.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool SetWorkingDirectory(const String& path);

public:
    /// <summary>
    /// Gets the process environment variables (pairs of key and value).
    /// </summary>
    /// <param name="result">The result.</param>
    static void GetEnvironmentVariables(Dictionary<String, String, HeapAllocation>& result);

    /// <summary>
    /// Gets the environment variable value.
    /// </summary>
    /// <param name="name">The variable name.</param>
    /// <param name="value">The result value.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool GetEnvironmentVariable(const String& name, String& value);

    /// <summary>
    /// Sets the environment variable value.
    /// </summary>
    /// <param name="name">The variable name.</param>
    /// <param name="value">The value to assign.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool SetEnvironmentVariable(const String& name, const String& value);

public:
    /// <summary>
    /// Starts a new process (runs app).
    /// [Deprecated in v1.6]
    /// </summary>
    /// <param name="filename">The path to the executable file.</param>
    /// <param name="args">Custom arguments for command line</param>
    /// <param name="workingDir">The custom name of the working directory</param>
    /// <param name="hiddenWindow">True if start process with hidden window</param>
    /// <param name="waitForEnd">True if wait for process competition</param>
    /// <returns>Retrieves the termination status of the specified process. Valid only if processed ended.</returns>
    API_FUNCTION() DEPRECATED("Use CreateProcess instead") static int32 StartProcess(const StringView& filename, const StringView& args, const StringView& workingDir, bool hiddenWindow = false, bool waitForEnd = false);

    /// <summary>
    /// Starts a new process (runs commandline). Waits for it's end and captures its output.
    /// [Deprecated in v1.6]
    /// </summary>
    /// <param name="cmdLine">Command line to execute</param>
    /// <param name="workingDir">The custom path of the working directory.</param>
    /// <param name="hiddenWindow">True if start process with hidden window.</param>
    /// <returns>Retrieves the termination status of the specified process. Valid only if processed ended.</returns>
    API_FUNCTION() DEPRECATED("Use CreateProcess instead") static int32 RunProcess(const StringView& cmdLine, const StringView& workingDir, bool hiddenWindow = true);

    /// <summary>
    /// Starts a new process (runs commandline). Waits for it's end and captures its output.
    /// [Deprecated in v1.6]
    /// </summary>
    /// <param name="cmdLine">Command line to execute</param>
    /// <param name="workingDir">The custom path of the working directory.</param>
    /// <param name="environment">The process environment variables. If null the current process environment is used.</param>
    /// <param name="hiddenWindow">True if start process with hidden window.</param>
    /// <returns>Retrieves the termination status of the specified process. Valid only if processed ended.</returns>
    API_FUNCTION() DEPRECATED("Use CreateProcess instead") static int32 RunProcess(const StringView& cmdLine, const StringView& workingDir, const Dictionary<String, String, HeapAllocation>& environment, bool hiddenWindow = true);

    /// <summary>
    /// Creates a new process.
    /// </summary>
    /// <param name="settings">The process settings.</param>
    /// <returns>Retrieves the termination status of the specified process. Valid only if processed ended.</returns>
    API_FUNCTION() static int32 CreateProcess(API_PARAM(Ref) CreateProcessSettings& settings);

    /// <summary>
    /// Creates the window.
    /// </summary>
    /// <param name="settings">The window settings.</param>
    /// <returns>The created native window object or null if failed.</returns>
    API_FUNCTION() static Window* CreateWindow(API_PARAM(Ref) CreateWindowSettings& settings) = delete;

    /// <summary>
    /// Loads the library.
    /// </summary>
    /// <param name="filename">The filename of the library to load (relative to workspace or absolute path).</param>
    /// <returns>The loaded library handle (native) or null if failed.</returns>
    static void* LoadLibrary(const Char* filename) = delete;

    /// <summary>
    /// Frees the library. 
    /// </summary>
    /// <param name="handle">The loaded library handle.</param>
    static void FreeLibrary(void* handle) = delete;

    /// <summary>
    /// Gets the address of an exported function or variable from the specified library.
    /// </summary>
    /// <param name="handle">The library handle.</param>
    /// <param name="symbol">The name of the symbol to locate (exported function or a variable).</param>
    /// <returns>The address of the exported symbol, or null if failed.</returns>
    static void* GetLibraryExportSymbol(void* handle, const char* symbol) = delete;

    /// <summary>
    /// Stack trace frame location.
    /// </summary>
    struct StackFrame
    {
        char ModuleName[256];
        char FunctionName[256];
        char FileName[256];
        int32 LineNumber;
        void* ProgramCounter;
    };

    /// <summary>
    /// Gets current native stack trace information.
    /// </summary>
    /// <param name="skipCount">The amount of stack frames to skip from the beginning. Can be used to skip the callee function from the trace (eg. in crash handler).</param>
    /// <param name="maxDepth">The maximum depth of the stack to collect. Can be used to prevent too long stack traces in case of stack overflow exception.</param>
    /// <param name="context">The platform-dependent context for the stack trace collecting (eg. platform exception info).</param>
    /// <returns>The collected stack trace frames. Empty if not supported (eg. platform not implements this feature or not supported in the distributed build).</returns>
    static Array<StackFrame, HeapAllocation> GetStackFrames(int32 skipCount = 0, int32 maxDepth = 60, void* context = nullptr);

    /// <summary>
    /// Gets current native stack trace information as string.
    /// </summary>
    /// <param name="skipCount">The amount of stack frames to skip from the beginning. Can be used to skip the callee function from the trace (eg. in crash handler).</param>
    /// <param name="maxDepth">The maximum depth of the stack to collect. Can be used to prevent too long stack traces in case of stack overflow exception.</param>
    /// <param name="context">The platform-dependent context for the stack trace collecting (eg. platform exception info).</param>
    /// <returns>The collected stack trace printed into string. Empty if not supported (eg. platform not implements this feature or not supported in the distributed build).</returns>
    static String GetStackTrace(int32 skipCount = 0, int32 maxDepth = 60, void* context = nullptr);

    // Crash dump data handling
    static void CollectCrashData(const String& crashDataFolder, void* context = nullptr);
};

extern FLAXENGINE_API const Char* ToString(PlatformType type);
extern FLAXENGINE_API const Char* ToString(ArchitectureType type);
