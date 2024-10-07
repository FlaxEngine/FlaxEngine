// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Engine/Platform/Platform.h"
#include "Engine/Platform/CPUInfo.h"
#include "Engine/Platform/MemoryStats.h"
#include "Engine/Platform/MessageBox.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/Window.h"
#include "Engine/Platform/User.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Core/Utilities.h"
#if COMPILE_WITH_PROFILER
#include "Engine/Profiler/ProfilerCPU.h"
#endif
#include "Engine/Threading/Threading.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Utilities/StringConverter.h"
#include "Engine/Platform/BatteryInfo.h"
#include <iostream>

// Check types sizes
static_assert(sizeof(int8) == 1, "Invalid int8 type size.");
static_assert(sizeof(int16) == 2, "Invalid int16 type size.");
static_assert(sizeof(int32) == 4, "Invalid int32 type size.");
static_assert(sizeof(int64) == 8, "Invalid int64 type size.");
static_assert(sizeof(uint8) == 1, "Invalid uint8 type size.");
static_assert(sizeof(uint16) == 2, "Invalid uint16 type size.");
static_assert(sizeof(uint32) == 4, "Invalid uint32 type size.");
static_assert(sizeof(uint64) == 8, "Invalid uint64 type size.");
static_assert(sizeof(Char) == 2, "Invalid Char type size.");
static_assert(sizeof(char) == 1, "Invalid char type size.");
static_assert(sizeof(bool) == 1, "Invalid bool type size.");
static_assert(sizeof(float) == 4, "Invalid float type size.");
static_assert(sizeof(double) == 8, "Invalid double type size.");

// Check configuration
static_assert((PLATFORM_THREADS_LIMIT & (PLATFORM_THREADS_LIMIT - 1)) == 0, "Threads limit must be power of two.");
static_assert(PLATFORM_THREADS_LIMIT % 4 == 0, "Threads limit must be multiple of 4.");

const Char* PlatformBase::ApplicationClassName = TEXT("FlaxWindow");
float PlatformBase::CustomDpiScale = 1.0f;
Array<User*, FixedAllocation<8>> PlatformBase::Users;
Delegate<User*> PlatformBase::UserAdded;
Delegate<User*> PlatformBase::UserRemoved;

const Char* ToString(NetworkConnectionType value)
{
    switch (value)
    {
    case NetworkConnectionType::None:
        return TEXT("None");
    case NetworkConnectionType::Unknown:
        return TEXT("Unknown");
    case NetworkConnectionType::AirplaneMode:
        return TEXT("AirplaneMode");
    case NetworkConnectionType::Cell:
        return TEXT("Cell");
    case NetworkConnectionType::WiFi:
        return TEXT("WiFi");
    case NetworkConnectionType::Bluetooth:
        return TEXT("Bluetooth");
    case NetworkConnectionType::Ethernet:
        return TEXT("Ethernet");
    default:
        return TEXT("");
    }
}

const Char* ToString(ScreenOrientationType value)
{
    switch (value)
    {
    case ScreenOrientationType::Unknown:
        return TEXT("Unknown");
    case ScreenOrientationType::Portrait:
        return TEXT("Portrait");
    case ScreenOrientationType::PortraitUpsideDown:
        return TEXT("PortraitUpsideDown");
    case ScreenOrientationType::LandscapeLeft:
        return TEXT("LandscapeLeft");
    case ScreenOrientationType::LandscapeRight:
        return TEXT("LandscapeRight");
    default:
        return TEXT("");
    }
}

const Char* ToString(ThreadPriority value)
{
    switch (value)
    {
    case ThreadPriority::Normal:
        return TEXT("Normal");
    case ThreadPriority::AboveNormal:
        return TEXT("AboveNormal");
    case ThreadPriority::BelowNormal:
        return TEXT("BelowNormal");
    case ThreadPriority::Highest:
        return TEXT("Highest");
    case ThreadPriority::Lowest:
        return TEXT("Lowest");
    default:
        return TEXT("");
    }
}

UserBase::UserBase(const String& name)
    : UserBase(SpawnParams(Guid::New(), TypeInitializer), name)
{
}

UserBase::UserBase(const SpawnParams& params, const String& name)
    : ScriptingObject(params)
    , _name(name)
{
}

String UserBase::GetName() const
{
    return _name;
}

bool PlatformBase::Init()
{
#if BUILD_DEBUG
    // Validate atomic and interlocked operations
    int64 data = 0;
    Platform::AtomicStore(&data, 11);
    ASSERT(Platform::AtomicRead(&data) == 11);
    ASSERT(Platform::InterlockedAdd(&data, 2) == 11);
    ASSERT(Platform::AtomicRead(&data) == 13);
    ASSERT(Platform::InterlockedIncrement(&data) == 14);
    ASSERT(Platform::AtomicRead(&data) == 14);
    ASSERT(Platform::InterlockedDecrement(&data) == 13);
    ASSERT(Platform::AtomicRead(&data) == 13);
    ASSERT(Platform::InterlockedExchange(&data, 10) == 13);
    ASSERT(Platform::AtomicRead(&data) == 10);
    ASSERT(Platform::InterlockedCompareExchange(&data, 11, 0) == 10);
    ASSERT(Platform::AtomicRead(&data) == 10);
    ASSERT(Platform::InterlockedCompareExchange(&data, 11, 10) == 10);
    ASSERT(Platform::AtomicRead(&data) == 11);
#endif

    srand((unsigned int)Platform::GetTimeCycles());

    return false;
}

void PlatformBase::LogInfo()
{
    // LOG(Info, "Computer name: {0}", Platform::GetComputerName());
    // LOG(Info, "User name: {0}", Platform::GetUserName());

    const CPUInfo cpuInfo = Platform::GetCPUInfo();
    LOG(Info, "CPU package count: {0}, Core count: {1}, Logical processors: {2}", cpuInfo.ProcessorPackageCount, cpuInfo.ProcessorCoreCount, cpuInfo.LogicalProcessorCount);
    LOG(Info, "CPU Page size: {0}, cache line size: {1} bytes", Utilities::BytesToText(cpuInfo.PageSize), cpuInfo.CacheLineSize);
    LOG(Info, "L1 cache: {0}, L2 cache: {1}, L3 cache: {2}", Utilities::BytesToText(cpuInfo.L1CacheSize), Utilities::BytesToText(cpuInfo.L2CacheSize), Utilities::BytesToText(cpuInfo.L3CacheSize));
    LOG(Info, "Clock speed: {0}", Utilities::HertzToText(cpuInfo.ClockSpeed));

    const MemoryStats memStats = Platform::GetMemoryStats();
    LOG(Info, "Physical Memory: {0} total, {1} used ({2}%)", Utilities::BytesToText(memStats.TotalPhysicalMemory), Utilities::BytesToText(memStats.UsedPhysicalMemory), Utilities::RoundTo2DecimalPlaces((float)memStats.UsedPhysicalMemory * 100.0f / (float)memStats.TotalPhysicalMemory));
    LOG(Info, "Virtual Memory: {0} total, {1} used ({2}%)", Utilities::BytesToText(memStats.TotalVirtualMemory), Utilities::BytesToText(memStats.UsedVirtualMemory), Utilities::RoundTo2DecimalPlaces((float)memStats.UsedVirtualMemory * 100.0f / (float)memStats.TotalVirtualMemory));

    LOG(Info, "Main thread id: 0x{0:x}, Process id: {1}", Globals::MainThreadID, Platform::GetCurrentProcessId());
    LOG(Info, "Desktop size: {0}", Platform::GetDesktopSize());
    LOG(Info, "Virtual Desktop size: {0}", Platform::GetVirtualDesktopBounds());
    LOG(Info, "Screen DPI: {0}", Platform::GetDpi());
}

void PlatformBase::BeforeRun()
{
}

void PlatformBase::Tick()
{
}

void PlatformBase::BeforeExit()
{
}

void PlatformBase::Exit()
{
}

#if COMPILE_WITH_PROFILER

#define TRACY_ENABLE_MEMORY (TRACY_ENABLE)

void PlatformBase::OnMemoryAlloc(void* ptr, uint64 size)
{
    if (!ptr)
        return;

#if TRACY_ENABLE_MEMORY
    // Track memory allocation in Tracy
    //tracy::Profiler::MemAlloc(ptr, (size_t)size, false);
    tracy::Profiler::MemAllocCallstack(ptr, (size_t)size, 12, false);
#endif

    // Register allocation during the current CPU event
    auto thread = ProfilerCPU::GetCurrentThread();
    if (thread != nullptr && thread->Buffer.GetCount() != 0)
    {
        auto& activeEvent = thread->Buffer.Last().Event();
        if (activeEvent.End < ZeroTolerance)
        {
            activeEvent.NativeMemoryAllocation += (int32)size;
        }
    }
}

void PlatformBase::OnMemoryFree(void* ptr)
{
    if (!ptr)
        return;

#if TRACY_ENABLE_MEMORY
    // Track memory allocation in Tracy
    tracy::Profiler::MemFree(ptr, false);
#endif
}

#endif

void* PlatformBase::AllocatePages(uint64 numPages, uint64 pageSize)
{
    // Fallback to the default memory allocation
    const uint64 numBytes = numPages * pageSize;
    return Platform::Allocate(numBytes, pageSize);
}

void PlatformBase::FreePages(void* ptr)
{
    // Fallback to free
    Platform::Free(ptr);
}

PlatformType PlatformBase::GetPlatformType()
{
    return PLATFORM_TYPE;
}

bool PlatformBase::Is64BitApp()
{
#if PLATFORM_64BITS
    return true;
#else
    return false;
#endif
}

void PlatformBase::Fatal(const Char* msg, void* context)
{
    // Check if is already during fatal state
    if (Globals::FatalErrorOccurred)
    {
        // Just send one more error to the log and back
        LOG(Error, "Error after fatal error: {0}", msg);
        return;
    }

    // Set flags
    Globals::FatalErrorOccurred = true;
    Globals::IsRequestingExit = true;
    Globals::ExitCode = -1;
    Engine::RequestingExit();

    // Collect crash info (platform-dependant implementation that might collect stack trace and/or create memory dump)
    {
        // Log separation for crash info
        Log::Logger::WriteFloor();
        LOG(Error, "");
        LOG(Error, "Critical error! Reason: {0}", msg);
        LOG(Error, "");

        // Log stack trace
        const auto stackFrames = Platform::GetStackFrames(context ? 0 : 1, 60, context);
        if (stackFrames.HasItems())
        {
            LOG(Error, "Stack trace:");
            for (const auto& frame : stackFrames)
            {
                // Remove any path from the module name
                int32 num = StringUtils::Length(frame.ModuleName);
                while (num > 0 && frame.ModuleName[num - 1] != '\\' && frame.ModuleName[num - 1] != '/' && frame.ModuleName[num - 1] != ':')
                    num--;
                StringAsUTF16<ARRAY_COUNT(StackFrame::ModuleName)> moduleName(frame.ModuleName + num);
                num = moduleName.Length();
                if (num != 0 && num < ARRAY_COUNT(StackFrame::ModuleName) - 2)
                {
                    // Append separator between module name and the function name
                    ((Char*)moduleName.Get())[num++] = '!';
                    ((Char*)moduleName.Get())[num] = 0;
                }

                StringAsUTF16<ARRAY_COUNT(StackFrame::FunctionName)> functionName(frame.FunctionName);
                if (StringUtils::Length(frame.FileName) != 0)
                {
                    StringAsUTF16<ARRAY_COUNT(StackFrame::FileName)> fileName(frame.FileName);
                    LOG(Error, "    at {0}{1}() in {2}:line {3}", moduleName.Get(), functionName.Get(), fileName.Get(), frame.LineNumber);
                }
                else if (StringUtils::Length(frame.FunctionName) != 0)
                {
                    LOG(Error, "    at {0}{1}()", moduleName.Get(), functionName.Get());
                }
                else if (StringUtils::Length(frame.ModuleName) != 0)
                {
                    LOG(Error, "    at {0}0x{1:x}", moduleName.Get(), (uint64)frame.ProgramCounter);
                }
                else
                {
                    LOG(Error, "    at 0x{0:x}", (uint64)frame.ProgramCounter);
                }
            }
            LOG(Error, "");
        }

        // Log process memory stats
        {
            const MemoryStats memoryStats = Platform::GetMemoryStats();
            LOG(Error, "Used Physical Memory: {0} ({1}%)", Utilities::BytesToText(memoryStats.UsedPhysicalMemory), (int32)(100 * memoryStats.UsedPhysicalMemory / memoryStats.TotalPhysicalMemory));
            LOG(Error, "Used Virtual Memory: {0} ({1}%)", Utilities::BytesToText(memoryStats.UsedVirtualMemory), (int32)(100 * memoryStats.UsedVirtualMemory / memoryStats.TotalVirtualMemory));
            const ProcessMemoryStats processMemoryStats = Platform::GetProcessMemoryStats();
            LOG(Error, "Process Used Physical Memory: {0}", Utilities::BytesToText(processMemoryStats.UsedPhysicalMemory));
            LOG(Error, "Process Used Virtual Memory: {0}", Utilities::BytesToText(processMemoryStats.UsedVirtualMemory));
        }
    }
    if (Log::Logger::LogFilePath.HasChars())
    {
        // Create separate folder with crash info
        const String crashDataFolder = String(StringUtils::GetDirectoryName(Log::Logger::LogFilePath)) / TEXT("Crash_") + StringUtils::GetFileNameWithoutExtension(Log::Logger::LogFilePath).Substring(4);
        FileSystem::CreateDirectory(crashDataFolder);

        // Capture the platform-dependant crash info (eg. memory dump)
        Platform::CollectCrashData(crashDataFolder, context);

        // Capture the original log file
        LOG(Error, "");
        Log::Logger::WriteFloor();
        LOG_FLUSH();
        FileSystem::CopyFile(crashDataFolder / TEXT("Log.txt"), Log::Logger::LogFilePath);

        LOG(Error, "Crash info collected.");
        Log::Logger::WriteFloor();
    }

    // Show error message
    Error(msg);

    // Only main thread can call exit directly
    if (IsInMainThread())
    {
        Engine::Exit(-1);
    }
}

void PlatformBase::Error(const Char* msg)
{
#if PLATFORM_HAS_HEADLESS_MODE
    if (CommandLine::Options.Headless)
    {
#if PLATFORM_TEXT_IS_CHAR16
        StringAnsi ansi(msg);
        ansi += PLATFORM_LINE_TERMINATOR;
        printf("Error: %s\n", ansi.Get());
#else
        std::cout << "Error: " << msg << std::endl;
#endif
    }
    else
#endif
    {
        MessageBox::Show(nullptr, msg, TEXT("Error"), MessageBoxButtons::OK, MessageBoxIcon::Error);
    }
}

void PlatformBase::Warning(const Char* msg)
{
#if PLATFORM_HAS_HEADLESS_MODE
    if (CommandLine::Options.Headless)
    {
        std::cout << "Warning: " << msg << std::endl;
    }
    else
#endif
    {
        MessageBox::Show(nullptr, msg, TEXT("Warning"), MessageBoxButtons::OK, MessageBoxIcon::Warning);
    }
}

void PlatformBase::Info(const Char* msg)
{
#if PLATFORM_HAS_HEADLESS_MODE
    if (CommandLine::Options.Headless)
    {
        std::cout << "Info: " << msg << std::endl;
    }
    else
#endif
    {
        MessageBox::Show(nullptr, msg, TEXT("Info"), MessageBoxButtons::OK, MessageBoxIcon::Information);
    }
}

void PlatformBase::Fatal(const StringView& msg)
{
    Fatal(*msg);
}

void PlatformBase::Error(const StringView& msg)
{
    Error(*msg);
}

void PlatformBase::Warning(const StringView& msg)
{
    Warning(*msg);
}

void PlatformBase::Info(const StringView& msg)
{
    Info(*msg);
}

void PlatformBase::Log(const StringView& msg)
{
}

bool PlatformBase::IsDebuggerPresent()
{
    return false;
}

void PlatformBase::Crash(int32 line, const char* file)
{
    const StringAsUTF16<256> fileUTF16(file);
    const String msg = String::Format(TEXT("Fatal crash!\nFile: {0}\nLine: {1}"), fileUTF16.Get(), line);
    LOG_STR(Fatal, msg);
}

void PlatformBase::OutOfMemory(int32 line, const char* file)
{
    const StringAsUTF16<256> fileUTF16(file);
    const String msg = String::Format(TEXT("Out of memory error!\nFile: {0}\nLine: {1}"), fileUTF16.Get(), line);
    LOG_STR(Fatal, msg);
}

void PlatformBase::MissingCode(int32 line, const char* file, const char* info)
{
    const StringAsUTF16<256> fileUTF16(file);
    const String msg = String::Format(TEXT("TODO: {0}\nFile: {1}\nLine: {2}"), String(info), fileUTF16.Get(), line);
    LOG_STR(Fatal, msg);
}

void PlatformBase::Assert(const char* message, const char* file, int line)
{
    const StringAsUTF16<256> fileUTF16(file);
    const String msg = String::Format(TEXT("Assertion failed!\nFile: {0}\nLine: {1}\n\nExpression: {2}"), fileUTF16.Get(), line, String(message));
    LOG_STR(Fatal, msg);
}

void PlatformBase::CheckFailed(const char* message, const char* file, int line)
{
    const StringAsUTF16<256> fileUTF16(file);
    const String msg = String::Format(TEXT("Check failed!\nFile: {0}\nLine: {1}\n\nExpression: {2}"), fileUTF16.Get(), line, String(message));
    LOG_STR(Error, msg);
}

void PlatformBase::SetHighDpiAwarenessEnabled(bool enable)
{
}

BatteryInfo PlatformBase::GetBatteryInfo()
{
    return BatteryInfo();
}

int32 PlatformBase::GetDpi()
{
    return 96;
}

float PlatformBase::GetDpiScale()
{
    return CustomDpiScale * (float)Platform::GetDpi() / 96.0f;
}

NetworkConnectionType PlatformBase::GetNetworkConnectionType()
{
    return NetworkConnectionType::Unknown;
}

ScreenOrientationType PlatformBase::GetScreenOrientationType()
{
    return ScreenOrientationType::Unknown;
}

String PlatformBase::GetUserName()
{
    return Users.Count() != 0 ? Users[0]->GetName() : String::Empty;
}

bool PlatformBase::GetIsPaused()
{
    return false;
}

void PlatformBase::CreateGuid(Guid& result)
{
    static uint16 guidCounter = 0;
    static DateTime guidStartTime;
    static double guidStartSeconds;

    DateTime estimatedCurrentDateTime;
    if (guidCounter == 0)
    {
        guidStartTime = DateTime::Now();
        guidStartSeconds = Platform::GetTimeSeconds();
        estimatedCurrentDateTime = guidStartTime;
    }
    else
    {
        const TimeSpan elapsedTime = TimeSpan::FromSeconds(Platform::GetTimeSeconds() - guidStartSeconds);
        estimatedCurrentDateTime = guidStartTime + elapsedTime;
    }

    const uint16 sequentialThing = static_cast<uint32>(guidCounter++);
    const uint16 randomThing = rand() & 0xFFFF;
    const uint32 dateThingHigh = estimatedCurrentDateTime.Ticks >> 32;
    const uint32 dateThingLow = estimatedCurrentDateTime.Ticks & 0xffffffff;
    const uint32 cyclesThing = Platform::GetTimeCycles() & 0xffffffff;

    result = Guid(dateThingHigh, randomThing | (sequentialThing << 16), cyclesThing, dateThingLow);
}

bool PlatformBase::CanOpenUrl(const StringView& url)
{
    return false;
}

void PlatformBase::OpenUrl(const StringView& url)
{
}

Float2 PlatformBase::GetMousePosition()
{
    const Window* win = Engine::MainWindow;
    if (win)
        return win->ClientToScreen(win->GetMousePosition());
    return Float2::Minimum;
}

void PlatformBase::SetMousePosition(const Float2& position)
{
    const Window* win = Engine::MainWindow;
    if (win)
        win->SetMousePosition(win->ScreenToClient(position));
}

Rectangle PlatformBase::GetMonitorBounds(const Float2& screenPos)
{
    return Rectangle(Float2::Zero, Platform::GetDesktopSize());
}

Rectangle PlatformBase::GetVirtualDesktopBounds()
{
    return Rectangle(Float2::Zero, Platform::GetDesktopSize());
}

Float2 PlatformBase::GetVirtualDesktopSize()
{
    return Platform::GetVirtualDesktopBounds().Size;
}

void PlatformBase::GetEnvironmentVariables(Dictionary<String, String>& result)
{
    // Not supported
}

bool PlatformBase::GetEnvironmentVariable(const String& name, String& value)
{
    // Not supported
    return true;
}

bool PlatformBase::SetEnvironmentVariable(const String& name, const String& value)
{
    // Not supported
    return true;
}

int32 PlatformBase::CreateProcess(CreateProcessSettings& settings)
{
    // Not supported
    return -1;
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS

#include "Engine/Platform/CreateProcessSettings.h"

int32 PlatformBase::StartProcess(const StringView& filename, const StringView& args, const StringView& workingDir, bool hiddenWindow, bool waitForEnd)
{
    CreateProcessSettings procSettings;
    procSettings.FileName = filename;
    procSettings.Arguments = args;
    procSettings.WorkingDirectory = workingDir;
    procSettings.HiddenWindow = hiddenWindow;
    procSettings.WaitForEnd = waitForEnd;
    procSettings.LogOutput = waitForEnd;
    procSettings.ShellExecute = true;
    return Platform::CreateProcess(procSettings);
}

int32 PlatformBase::RunProcess(const StringView& cmdLine, const StringView& workingDir, bool hiddenWindow)
{
    CreateProcessSettings procSettings;
    procSettings.FileName = cmdLine;
    procSettings.WorkingDirectory = workingDir;
    procSettings.HiddenWindow = hiddenWindow;
    return Platform::CreateProcess(procSettings);
}

int32 PlatformBase::RunProcess(const StringView& cmdLine, const StringView& workingDir, const Dictionary<String, String>& environment, bool hiddenWindow)
{
    CreateProcessSettings procSettings;
    procSettings.FileName = cmdLine;
    procSettings.WorkingDirectory = workingDir;
    procSettings.Environment = environment;
    procSettings.HiddenWindow = hiddenWindow;
    return Platform::CreateProcess(procSettings);
}

PRAGMA_ENABLE_DEPRECATION_WARNINGS

Array<PlatformBase::StackFrame> PlatformBase::GetStackFrames(int32 skipCount, int32 maxDepth, void* context)
{
    return Array<StackFrame>();
}

String PlatformBase::GetStackTrace(int32 skipCount, int32 maxDepth, void* context)
{
    StringBuilder result;
    Array<StackFrame> stackFrames = Platform::GetStackFrames(skipCount, maxDepth, context);
    for (const auto& frame : stackFrames)
    {
        StringAsUTF16<ARRAY_COUNT(StackFrame::FunctionName)> functionName(frame.FunctionName);
        const StringView functionNameStr(functionName.Get());
        if (StringUtils::Length(frame.FileName) != 0)
        {
            StringAsUTF16<ARRAY_COUNT(StackFrame::FileName)> fileName(frame.FileName);
            result.Append(TEXT("   at ")).Append(functionNameStr);
            if (!functionNameStr.EndsWith(')'))
                result.Append(TEXT("()"));
            result.AppendFormat(TEXT("in {0}:line {1}\n"),fileName.Get(), frame.LineNumber);
        }
        else if (StringUtils::Length(frame.FunctionName) != 0)
        {
            result.Append(TEXT("   at ")).Append(functionNameStr);
            if (!functionNameStr.EndsWith(')'))
                result.Append(TEXT("()"));
            result.Append('\n');
        }
        else
        {
            result.AppendFormat(TEXT("   at 0x{0:x}\n"), (uint64)frame.ProgramCounter);
        }
    }
    return result.ToString();
}

void PlatformBase::CollectCrashData(const String& crashDataFolder, void* context)
{
}

const Char* ToString(PlatformType type)
{
    switch (type)
    {
    case PlatformType::Windows:
        return TEXT("Windows");
    case PlatformType::XboxOne:
        return TEXT("Xbox One");
    case PlatformType::UWP:
        return TEXT("Windows Store");
    case PlatformType::Linux:
        return TEXT("Linux");
    case PlatformType::PS4:
        return TEXT("PlayStation 4");
    case PlatformType::XboxScarlett:
        return TEXT("Xbox Scarlett");
    case PlatformType::Android:
        return TEXT("Android");
    case PlatformType::Switch:
        return TEXT("Switch");
    case PlatformType::PS5:
        return TEXT("PlayStation 5");
    case PlatformType::Mac:
        return TEXT("Mac");
    case PlatformType::iOS:
        return TEXT("iOS");
    default:
        return TEXT("");
    }
}

const Char* ToString(ArchitectureType type)
{
    switch (type)
    {
    case ArchitectureType::AnyCPU:
        return TEXT("AnyCPU");
    case ArchitectureType::x86:
        return TEXT("x86");
    case ArchitectureType::x64:
        return TEXT("x64");
    case ArchitectureType::ARM:
        return TEXT("ARM");
    case ArchitectureType::ARM64:
        return TEXT("ARM64");
    default:
        return TEXT("");
    }
}
