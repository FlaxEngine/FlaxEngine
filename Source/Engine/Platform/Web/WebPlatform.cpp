// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_WEB

#include "WebPlatform.h"
#include "WebThread.h"
#include "WebFileSystem.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Types/Version.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Platform/CPUInfo.h"
#include "Engine/Platform/MemoryStats.h"
#include "Engine/Platform/MessageBox.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerMemory.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/Web/WebGame.h"
#include "Engine/Utilities/StringConverter.h"
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

void WebGame::InitMainWindowSettings(CreateWindowSettings& settings)
{
    // Set window size matching the canvas item in HTML
    settings.Fullscreen = false;
    int width = 1, height = 1;
    emscripten_get_canvas_element_size(WEB_CANVAS_ID, &width, &height);
    settings.Size.X = width;
    settings.Size.Y = height;
}

DialogResult MessageBox::Show(Window* parent, const StringView& text, const StringView& caption, MessageBoxButtons buttons, MessageBoxIcon icon)
{
    StringAnsi textAnsi(text);
    StringAnsi captionAnsi(caption);
    EM_ASM({ alert(UTF8ToString($0) + "\n\n" + UTF8ToString($1)); }, captionAnsi.Get(), textAnsi.Get());
    return DialogResult::None;
}

WebThread* WebThread::Create(IRunnable* runnable, const String& name, ThreadPriority priority, uint32 stackSize)
{
#ifdef __EMSCRIPTEN_PTHREADS__
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (stackSize != 0)
        pthread_attr_setstacksize(&attr, stackSize);
    auto thread = New<WebThread>(runnable, name, priority);
    const int result = pthread_create(&thread->_thread, &attr, ThreadProc, thread);
    if (result != 0)
    {
        LOG(Warning, "Failed to spawn a thread. Result code: {0}", result);
        Delete(thread);
        return nullptr;
    }
    return thread;
#else
    LOG(Fatal, "Threading is not supported.");
    return nullptr;
#endif
}

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

void WebPlatform::Log(const StringView& msg, int32 logType)
{
    const StringAsANSI<512> msgAnsi(*msg, msg.Length());

    // Fix % characters that should not be formatted
    auto buffer = (char*)msgAnsi.Get();
    for (int32 i = 0; buffer[i]; i++)
    {
        if (buffer[i] == '%')
            buffer[i] = 'p';
    }

    int flags = EM_LOG_CONSOLE;
    if (logType == (int32)LogType::Warning)
        flags |= EM_LOG_WARN;
    if (logType == (int32)LogType::Error)
        flags |= EM_LOG_ERROR;
    emscripten_log(flags, buffer);
}

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

#if COMPILE_WITH_PROFILER
    // Setup platform-specific memory profiler tags
    ProfilerMemory::RenameGroup(ProfilerMemory::Groups::WEB_MEM_TAG_HEAP_SIZE, TEXT("Emscripten/HeapSize"));
    ProfilerMemory::RenameGroup(ProfilerMemory::Groups::WEB_MEM_TAG_HEAP_MAX, TEXT("Emscripten/HeapMax"));
#endif

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
#if COMPILE_WITH_PROFILER
    ProfilerMemory::OnGroupSet(ProfilerMemory::Groups::WEB_MEM_TAG_HEAP_SIZE, (int64)emscripten_get_heap_size(), 1);
    ProfilerMemory::OnGroupSet(ProfilerMemory::Groups::WEB_MEM_TAG_HEAP_MAX, (int64)emscripten_get_heap_max(), 1);
#endif
}

void WebPlatform::Exit()
{
    // Exit runtime
    emscripten_cancel_main_loop();
    emscripten_force_exit(Engine::ExitCode);
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

Array<PlatformBase::StackFrame> WebPlatform::GetStackFrames(int32 skipCount, int32 maxDepth, void* context)
{
    Array<StackFrame> result;
#if CRASH_LOG_ENABLE
    // Get callstack
    char callstack[2000];
    emscripten_get_callstack(EM_LOG_JS_STACK, callstack, sizeof(callstack));

    // Parse callstack
    int32 pos = 0;
    while (callstack[pos])
    {
        StackFrame& frame = result.AddOne();
        frame.ProgramCounter = 0;
        frame.ModuleName[0] = 0;
        frame.FileName[0] = 0;
        frame.LineNumber = 0;

        // Skip leading spaces
        while (callstack[pos] == ' ')
            pos++;

        // Skip 'at '
        pos += 3;

        // Read function name
        int32 dst = 0;
        while (callstack[pos] && callstack[pos] != '\n' && dst < ARRAY_COUNT(frame.FileName) - 1)
            frame.FunctionName[dst++] = callstack[pos++];
        frame.FunctionName[dst] = 0;

        // Skip '\n'
        if (callstack[pos])
            pos++;
    }

    // Adjust frames range
    skipCount += 2; // Skip this function and utility JS frames
    while (skipCount-- && result.HasItems())
        result.RemoveAtKeepOrder(0);
    if (result.Count() > maxDepth)
        result.Resize(maxDepth);
#endif
    return result;
}

#if 1
// Fix linker errors when using '-sAUTO_JS_LIBRARIES=0' due to unused WebGL functions (referenced by SDL3 port itself from emscripten: SDL_emscriptenopengles.c)
extern "C" EMSCRIPTEN_WEBGL_CONTEXT_HANDLE emscripten_webgl_create_context(const char* target, const EmscriptenWebGLContextAttributes* attributes)
{
    return 0;
}

extern "C" EMSCRIPTEN_RESULT emscripten_webgl_make_context_current(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context)
{
    return 0;
}

extern "C" EMSCRIPTEN_RESULT emscripten_webgl_destroy_context(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context)
{
    return 0;
}

extern "C" void* emscripten_webgl_get_proc_address(const char* name)
{
    return nullptr;
}

extern "C" void emscripten_webgl_init_context_attributes(EmscriptenWebGLContextAttributes* attributes)
{
}

// Reference: emscripten\system\lib\html5\dom_pk_codes.c
#include <emscripten/dom_pk_codes.h>
extern "C" DOM_PK_CODE_TYPE emscripten_compute_dom_pk_code(const char* keyCodeString)
{
    if (!keyCodeString) return 0;

    /* Compute the collision free hash. */
    unsigned int hash = 0;
    while (*keyCodeString) hash = ((hash ^ 0x7E057D79U) << 3) ^ (unsigned int)*keyCodeString++;

    /*
     * Don't expose the hash values out to the application, but map to fixed IDs.
     * This is useful for mapping back codes to MDN documentation page at
     *
     *   https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent/code
     */
    switch (hash) {
    case 0x98051284U /* Unidentified       */: return DOM_PK_UNKNOWN;              /* 0x0000 */
    case 0x67243A2DU /* Escape             */: return DOM_PK_ESCAPE;               /* 0x0001 */
    case 0x67251058U /* Digit0             */: return DOM_PK_0;                    /* 0x0002 */
    case 0x67251059U /* Digit1             */: return DOM_PK_1;                    /* 0x0003 */
    case 0x6725105AU /* Digit2             */: return DOM_PK_2;                    /* 0x0004 */
    case 0x6725105BU /* Digit3             */: return DOM_PK_3;                    /* 0x0005 */
    case 0x6725105CU /* Digit4             */: return DOM_PK_4;                    /* 0x0006 */
    case 0x6725105DU /* Digit5             */: return DOM_PK_5;                    /* 0x0007 */
    case 0x6725105EU /* Digit6             */: return DOM_PK_6;                    /* 0x0008 */
    case 0x6725105FU /* Digit7             */: return DOM_PK_7;                    /* 0x0009 */
    case 0x67251050U /* Digit8             */: return DOM_PK_8;                    /* 0x000A */
    case 0x67251051U /* Digit9             */: return DOM_PK_9;                    /* 0x000B */
    case 0x92E14DD3U /* Minus              */: return DOM_PK_MINUS;                /* 0x000C */
    case 0x92E1FBACU /* Equal              */: return DOM_PK_EQUAL;                /* 0x000D */
    case 0x36BF1CB5U /* Backspace          */: return DOM_PK_BACKSPACE;            /* 0x000E */
    case 0x7B8E51E2U /* Tab                */: return DOM_PK_TAB;                  /* 0x000F */
    case 0x2C595B51U /* KeyQ               */: return DOM_PK_Q;                    /* 0x0010 */
    case 0x2C595B57U /* KeyW               */: return DOM_PK_W;                    /* 0x0011 */
    case 0x2C595B45U /* KeyE               */: return DOM_PK_E;                    /* 0x0012 */
    case 0x2C595B52U /* KeyR               */: return DOM_PK_R;                    /* 0x0013 */
    case 0x2C595B54U /* KeyT               */: return DOM_PK_T;                    /* 0x0014 */
    case 0x2C595B59U /* KeyY               */: return DOM_PK_Y;                    /* 0x0015 */
    case 0x2C595B55U /* KeyU               */: return DOM_PK_U;                    /* 0x0016 */
    case 0x2C595B49U /* KeyI               */: return DOM_PK_I;                    /* 0x0017 */
    case 0x2C595B4FU /* KeyO               */: return DOM_PK_O;                    /* 0x0018 */
    case 0x2C595B50U /* KeyP               */: return DOM_PK_P;                    /* 0x0019 */
    case 0x45D8158CU /* BracketLeft        */: return DOM_PK_BRACKET_LEFT;         /* 0x001A */
    case 0xDEEABF7CU /* BracketRight       */: return DOM_PK_BRACKET_RIGHT;        /* 0x001B */
    case 0x92E1C5D2U /* Enter              */: return DOM_PK_ENTER;                /* 0x001C */
    case 0xE058958CU /* ControlLeft        */: return DOM_PK_CONTROL_LEFT;         /* 0x001D */
    case 0x2C595B41U /* KeyA               */: return DOM_PK_A;                    /* 0x001E */
    case 0x2C595B53U /* KeyS               */: return DOM_PK_S;                    /* 0x001F */
    case 0x2C595B44U /* KeyD               */: return DOM_PK_D;                    /* 0x0020 */
    case 0x2C595B46U /* KeyF               */: return DOM_PK_F;                    /* 0x0021 */
    case 0x2C595B47U /* KeyG               */: return DOM_PK_G;                    /* 0x0022 */
    case 0x2C595B48U /* KeyH               */: return DOM_PK_H;                    /* 0x0023 */
    case 0x2C595B4AU /* KeyJ               */: return DOM_PK_J;                    /* 0x0024 */
    case 0x2C595B4BU /* KeyK               */: return DOM_PK_K;                    /* 0x0025 */
    case 0x2C595B4CU /* KeyL               */: return DOM_PK_L;                    /* 0x0026 */
    case 0x2707219EU /* Semicolon          */: return DOM_PK_SEMICOLON;            /* 0x0027 */
    case 0x92E0B58DU /* Quote              */: return DOM_PK_QUOTE;                /* 0x0028 */
    case 0x36BF358DU /* Backquote          */: return DOM_PK_BACKQUOTE;            /* 0x0029 */
    case 0x26B1958CU /* ShiftLeft          */: return DOM_PK_SHIFT_LEFT;           /* 0x002A */
    case 0x36BF2438U /* Backslash          */: return DOM_PK_BACKSLASH;            /* 0x002B */
    case 0x2C595B5AU /* KeyZ               */: return DOM_PK_Z;                    /* 0x002C */
    case 0x2C595B58U /* KeyX               */: return DOM_PK_X;                    /* 0x002D */
    case 0x2C595B43U /* KeyC               */: return DOM_PK_C;                    /* 0x002E */
    case 0x2C595B56U /* KeyV               */: return DOM_PK_V;                    /* 0x002F */
    case 0x2C595B42U /* KeyB               */: return DOM_PK_B;                    /* 0x0030 */
    case 0x2C595B4EU /* KeyN               */: return DOM_PK_N;                    /* 0x0031 */
    case 0x2C595B4DU /* KeyM               */: return DOM_PK_M;                    /* 0x0032 */
    case 0x92E1A1C1U /* Comma              */: return DOM_PK_COMMA;                /* 0x0033 */
    case 0x672FFAD4U /* Period             */: return DOM_PK_PERIOD;               /* 0x0034 */
    case 0x92E0A438U /* Slash              */: return DOM_PK_SLASH;                /* 0x0035 */
    case 0xC5A6BF7CU /* ShiftRight         */: return DOM_PK_SHIFT_RIGHT;          /* 0x0036 */
    case 0x5D64DA91U /* NumpadMultiply     */: return DOM_PK_NUMPAD_MULTIPLY;      /* 0x0037 */
    case 0xC914958CU /* AltLeft            */: return DOM_PK_ALT_LEFT;             /* 0x0038 */
    case 0x92E09CB5U /* Space              */: return DOM_PK_SPACE;                /* 0x0039 */
    case 0xB8FAE73BU /* CapsLock           */: return DOM_PK_CAPS_LOCK;            /* 0x003A */
    case 0x7174B789U /* F1                 */: return DOM_PK_F1;                   /* 0x003B */
    case 0x7174B78AU /* F2                 */: return DOM_PK_F2;                   /* 0x003C */
    case 0x7174B78BU /* F3                 */: return DOM_PK_F3;                   /* 0x003D */
    case 0x7174B78CU /* F4                 */: return DOM_PK_F4;                   /* 0x003E */
    case 0x7174B78DU /* F5                 */: return DOM_PK_F5;                   /* 0x003F */
    case 0x7174B78EU /* F6                 */: return DOM_PK_F6;                   /* 0x0040 */
    case 0x7174B78FU /* F7                 */: return DOM_PK_F7;                   /* 0x0041 */
    case 0x7174B780U /* F8                 */: return DOM_PK_F8;                   /* 0x0042 */
    case 0x7174B781U /* F9                 */: return DOM_PK_F9;                   /* 0x0043 */
    case 0x7B8E57B0U /* F10                */: return DOM_PK_F10;                  /* 0x0044 */
    case 0x92E08B35U /* Pause              */: return DOM_PK_PAUSE;                /* 0x0045 */
    case 0xCDED173BU /* ScrollLock         */: return DOM_PK_SCROLL_LOCK;          /* 0x0046 */
    case 0xC925FCDFU /* Numpad7            */: return DOM_PK_NUMPAD_7;             /* 0x0047 */
    case 0xC925FCD0U /* Numpad8            */: return DOM_PK_NUMPAD_8;             /* 0x0048 */
    case 0xC925FCD1U /* Numpad9            */: return DOM_PK_NUMPAD_9;             /* 0x0049 */
    case 0x5EA3E8A4U /* NumpadSubtract     */: return DOM_PK_NUMPAD_SUBTRACT;      /* 0x004A */
    case 0xC925FCDCU /* Numpad4            */: return DOM_PK_NUMPAD_4;             /* 0x004B */
    case 0xC925FCDDU /* Numpad5            */: return DOM_PK_NUMPAD_5;             /* 0x004C */
    case 0xC925FCDEU /* Numpad6            */: return DOM_PK_NUMPAD_6;             /* 0x004D */
    case 0x380B9C8CU /* NumpadAdd          */: return DOM_PK_NUMPAD_ADD;           /* 0x004E */
    case 0xC925FCD9U /* Numpad1            */: return DOM_PK_NUMPAD_1;             /* 0x004F */
    case 0xC925FCDAU /* Numpad2            */: return DOM_PK_NUMPAD_2;             /* 0x0050 */
    case 0xC925FCDBU /* Numpad3            */: return DOM_PK_NUMPAD_3;             /* 0x0051 */
    case 0xC925FCD8U /* Numpad0            */: return DOM_PK_NUMPAD_0;             /* 0x0052 */
    case 0x95852DACU /* NumpadDecimal      */: return DOM_PK_NUMPAD_DECIMAL;       /* 0x0053 */
    case 0xCC1E198EU /* PrintScreen        */: return DOM_PK_PRINT_SCREEN;         /* 0x0054 */
    case 0x16BF2438U /* IntlBackslash      */: return DOM_PK_INTL_BACKSLASH;       /* 0x0056 */
    case 0x7B8E57B1U /* F11                */: return DOM_PK_F11;                  /* 0x0057 */
    case 0x7B8E57B2U /* F12                */: return DOM_PK_F12;                  /* 0x0058 */
    case 0x7393FBACU /* NumpadEqual        */: return DOM_PK_NUMPAD_EQUAL;         /* 0x0059 */
    case 0x7B8E57B3U /* F13                */: return DOM_PK_F13;                  /* 0x0064 */
    case 0x7B8E57B4U /* F14                */: return DOM_PK_F14;                  /* 0x0065 */
    case 0x7B8E57B5U /* F15                */: return DOM_PK_F15;                  /* 0x0066 */
    case 0x7B8E57B6U /* F16                */: return DOM_PK_F16;                  /* 0x0067 */
    case 0x7B8E57B7U /* F17                */: return DOM_PK_F17;                  /* 0x0068 */
    case 0x7B8E57B8U /* F18                */: return DOM_PK_F18;                  /* 0x0069 */
    case 0x7B8E57B9U /* F19                */: return DOM_PK_F19;                  /* 0x006A */
    case 0x7B8E57A8U /* F20                */: return DOM_PK_F20;                  /* 0x006B */
    case 0x7B8E57A9U /* F21                */: return DOM_PK_F21;                  /* 0x006C */
    case 0x7B8E57AAU /* F22                */: return DOM_PK_F22;                  /* 0x006D */
    case 0x7B8E57ABU /* F23                */: return DOM_PK_F23;                  /* 0x006E */
    case 0xB9F4C50DU /* KanaMode           */: return DOM_PK_KANA_MODE;            /* 0x0070 */
    case 0x92E14D02U /* Lang2              */: return DOM_PK_LANG_2;               /* 0x0071 */
    case 0x92E14D01U /* Lang1              */: return DOM_PK_LANG_1;               /* 0x0072 */
    case 0x6723C677U /* IntlRo             */: return DOM_PK_INTL_RO;              /* 0x0073 */
    case 0x7B8E57ACU /* F24                */: return DOM_PK_F24;                  /* 0x0076 */
    case 0xC91CC12CU /* Convert            */: return DOM_PK_CONVERT;              /* 0x0079 */
    case 0x2ADCC12CU /* NonConvert         */: return DOM_PK_NON_CONVERT;          /* 0x007B */
    case 0xC935DA8EU /* IntlYen            */: return DOM_PK_INTL_YEN;             /* 0x007D */
    case 0x7393A1C1U /* NumpadComma        */: return DOM_PK_NUMPAD_COMMA;         /* 0x007E */
    case 0x92E08A8DU /* Paste              */: return DOM_PK_PASTE;                /* 0xE00A */
    case 0x01DC7D93U /* MediaTrackPrevious */: return DOM_PK_MEDIA_TRACK_PREVIOUS; /* 0xE010 */
    case 0x7B8E5494U /* Cut                */: return DOM_PK_CUT;                  /* 0xE017 */
    case 0x2C5949B1U /* Copy               */: return DOM_PK_COPY;                 /* 0xE018 */
    case 0x2AD2E17CU /* MediaTrackNext     */: return DOM_PK_MEDIA_TRACK_NEXT;     /* 0xE019 */
    case 0x7393C5D2U /* NumpadEnter        */: return DOM_PK_NUMPAD_ENTER;         /* 0xE01C */
    case 0xF2EEBF7CU /* ControlRight       */: return DOM_PK_CONTROL_RIGHT;        /* 0xE01D */
    case 0x2A45030DU /* AudioVolumeMute    */: return DOM_PK_AUDIO_VOLUME_MUTE;    /* 0xE020 */
    case 0xEA45030DU /* VolumeMute         */: return DOM_PK_AUDIO_VOLUME_MUTE;    /* 0xE020 */
    case 0x370ECA3AU /* LaunchApp2         */: return DOM_PK_LAUNCH_APP_2;         /* 0xE021 */
    case 0x2D1C0B35U /* MediaPlayPause     */: return DOM_PK_MEDIA_PLAY_PAUSE;     /* 0xE022 */
    case 0x39237F80U /* MediaStop          */: return DOM_PK_MEDIA_STOP;           /* 0xE024 */
    case 0x92E1C9A4U /* Eject              */: return DOM_PK_EJECT;                /* 0xE02C */
    case 0x2A45179EU /* AudioVolumeDown    */: return DOM_PK_AUDIO_VOLUME_DOWN;    /* 0xE02E */
    case 0xEA45179EU /* VolumeDown         */: return DOM_PK_AUDIO_VOLUME_DOWN;    /* 0xE02E */
    case 0x156CC610U /* AudioVolumeUp      */: return DOM_PK_AUDIO_VOLUME_UP;      /* 0xE030 */
    case 0xBA6CC610U /* VolumeUp           */: return DOM_PK_AUDIO_VOLUME_UP;      /* 0xE030 */
    case 0x49387F45U /* BrowserHome        */: return DOM_PK_BROWSER_HOME;         /* 0xE032 */
    case 0x6CB5328DU /* NumpadDivide       */: return DOM_PK_NUMPAD_DIVIDE;        /* 0xE035 */
    case 0xB88EBF7CU /* AltRight           */: return DOM_PK_ALT_RIGHT;            /* 0xE038 */
    case 0x2C595DD8U /* Help               */: return DOM_PK_HELP;                 /* 0xE03B */
    case 0xC925873BU /* NumLock            */: return DOM_PK_NUM_LOCK;             /* 0xE045 */
    case 0x2C595F45U /* Home               */: return DOM_PK_HOME;                 /* 0xE047 */
    case 0xC91BB690U /* ArrowUp            */: return DOM_PK_ARROW_UP;             /* 0xE048 */
    case 0x672F9210U /* PageUp             */: return DOM_PK_PAGE_UP;              /* 0xE049 */
    case 0x3799258CU /* ArrowLeft          */: return DOM_PK_ARROW_LEFT;           /* 0xE04B */
    case 0x4CE33F7CU /* ArrowRight         */: return DOM_PK_ARROW_RIGHT;          /* 0xE04D */
    case 0x7B8E55DCU /* End                */: return DOM_PK_END;                  /* 0xE04F */
    case 0x3799379EU /* ArrowDown          */: return DOM_PK_ARROW_DOWN;           /* 0xE050 */
    case 0xBA90179EU /* PageDown           */: return DOM_PK_PAGE_DOWN;            /* 0xE051 */
    case 0x6723CB2CU /* Insert             */: return DOM_PK_INSERT;               /* 0xE052 */
    case 0x6725C50DU /* Delete             */: return DOM_PK_DELETE;               /* 0xE053 */
    case 0xB929C58CU /* MetaLeft           */: return DOM_PK_META_LEFT;            /* 0xE05B */
    case 0x6723658CU /* OSLeft             */: return DOM_PK_OS_LEFT;              /* 0xE05B */
    case 0x39643F7CU /* MetaRight          */: return DOM_PK_META_RIGHT;           /* 0xE05C */
    case 0xC9313F7CU /* OSRight            */: return DOM_PK_OS_RIGHT;             /* 0xE05C */
    case 0xE00E97CDU /* ContextMenu        */: return DOM_PK_CONTEXT_MENU;         /* 0xE05D */
    case 0x92E09712U /* Power              */: return DOM_PK_POWER;                /* 0xE05E */
    case 0x3F665A78U /* BrowserSearch      */: return DOM_PK_BROWSER_SEARCH;       /* 0xE065 */
    case 0xA2E93BD3U /* BrowserFavorites   */: return DOM_PK_BROWSER_FAVORITES;    /* 0xE066 */
    case 0x0B1D4938U /* BrowserRefresh     */: return DOM_PK_BROWSER_REFRESH;      /* 0xE067 */
    case 0x49384F80U /* BrowserStop        */: return DOM_PK_BROWSER_STOP;         /* 0xE068 */
    case 0x0B49023CU /* BrowserForward     */: return DOM_PK_BROWSER_FORWARD;      /* 0xE069 */
    case 0x493868BBU /* BrowserBack        */: return DOM_PK_BROWSER_BACK;         /* 0xE06A */
    case 0x370ECA39U /* LaunchApp1         */: return DOM_PK_LAUNCH_APP_1;         /* 0xE06B */
    case 0x370ED6ECU /* LaunchMail         */: return DOM_PK_LAUNCH_MAIL;          /* 0xE06C */
    case 0x39AB4892U /* LaunchMediaPlayer  */: return DOM_PK_LAUNCH_MEDIA_PLAYER;  /* 0xE06D */
    case 0x39AA45A4U /* MediaSelect        */: return DOM_PK_MEDIA_SELECT;         /* 0xE06D */
    default: return DOM_PK_UNKNOWN;
    }
}
#endif

#endif
