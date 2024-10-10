// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Engine/Scripting/Types.h"

#if USE_MONO

#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Core/Types/Stopwatch.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MDomain.h"
#include "Engine/Scripting/ManagedCLR/MEvent.h"
#include "Engine/Scripting/ManagedCLR/MField.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Scripting/ManagedCLR/MProperty.h"
#include "Engine/Scripting/ManagedCLR/MException.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Debug/Exceptions/CLRInnerException.h"
#include <mono/jit/jit.h>
#include <mono/utils/mono-counters.h>
#include <mono/utils/mono-logger.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/object.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/attrdefs.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/mono-debug.h>
#include <mono/metadata/mono-config.h>
#include <mono/metadata/mono-gc.h>
#include <mono/metadata/profiler.h>
#include <mono/metadata/tokentype.h>
#include <mono/metadata/exception.h>
#if !USE_MONO_DYNAMIC_LIB
#include <mono/utils/mono-dl-fallback.h>
#endif

#ifdef USE_MONO_AOT_MODULE
void* MonoAotModuleHandle = nullptr;
#endif

#define GET_CUSTOM_ATTR() (MonoCustomAttrInfo*)(_attrInfo ? _attrInfo : _attrInfo = mono_custom_attrs_from_class(_monoClass))

extern MDomain* MRootDomain;
extern MDomain* MActiveDomain;
extern Array<MDomain*, FixedAllocation<4>> MDomains;

// Inlined mono private types to access MonoType internals

typedef struct _MonoGenericClass MonoGenericClass;
typedef struct _MonoGenericContext MonoGenericContext;

struct _MonoGenericInst
{
    unsigned int id;
    unsigned int type_argc : 22;
    unsigned int is_open : 1;
    MonoType* type_argv[MONO_ZERO_LEN_ARRAY];
};

struct _MonoGenericContext
{
    MonoGenericInst* class_inst;
    MonoGenericInst* method_inst;
};

struct _MonoGenericClass
{
    MonoClass* container_class;
    MonoGenericContext context;
    unsigned int is_dynamic : 1;
    unsigned int is_tb_open : 1;
    unsigned int need_sync : 1;
    MonoClass* cached_class;
    class MonoImageSet* owner;
};

struct _MonoType
{
    union
    {
        MonoClass* klass;
        MonoType* type;
        MonoArrayType* array;
        MonoMethodSignature* method;
        MonoGenericParam* generic_param;
        MonoGenericClass* generic_class;
    } data;

    unsigned int attrs : 16;
    MonoTypeEnum type : 8;
    unsigned int has_cmods : 1;
    unsigned int byref : 1;
    unsigned int pinned : 1;
};

namespace
{
    static StringAnsi PlusStr("+");
    static StringAnsi DotStr(".");

    void GetClassFullname(MonoClass* monoClass, StringAnsi& fullname)
    {
        // Name
        fullname = mono_class_get_name(monoClass);

        // Outer class for nested types
        MonoClass* nestingClass = mono_class_get_nesting_type(monoClass);
        MonoClass* lastClass = monoClass;
        while (nestingClass)
        {
            lastClass = nestingClass;
            fullname = mono_class_get_name(nestingClass) + PlusStr + fullname;
            nestingClass = mono_class_get_nesting_type(nestingClass);
        }

        // Namespace
        const char* lastClassNamespace = mono_class_get_namespace(lastClass);
        if (lastClassNamespace && *lastClassNamespace)
            fullname = lastClassNamespace + DotStr + fullname;

        // Generic instance arguments
        MonoType* mType = mono_class_get_type(monoClass);
        if (mType && mType->type == MONO_TYPE_GENERICINST)
        {
            fullname += '[';
            StringAnsi tmp;
            for (unsigned int i = 0; i < mType->data.generic_class->context.class_inst->type_argc; i++)
            {
                if (i != 0)
                    fullname += ',';
                MType* argType = mType->data.generic_class->context.class_inst->type_argv[i];
                GetClassFullname(mono_class_from_mono_type(argType), tmp);
                fullname += tmp;
            }
            fullname += ']';
        }
    }

    MClass* FindClass(MonoClass* monoClass, bool addIfMissing = true)
    {
        if (monoClass == nullptr)
            return nullptr;
        PROFILE_CPU();
        auto& modules = BinaryModule::GetModules();
        for (auto module : modules)
        {
            auto managedModule = dynamic_cast<ManagedBinaryModule*>(module);
            if (managedModule && managedModule->Assembly->IsLoaded())
            {
                MClass* result = managedModule->Assembly->GetClass(monoClass);
                if (result != nullptr)
                    return result;
            }
        }
        if (addIfMissing)
        {
            MISSING_CODE("TODO: register new MClass"); // TODO: this might happen when for example querying class to array of game script type and it should add new MClass to handle this
        }
        return nullptr;
    }
}

MDomain* MCore::CreateDomain(const StringAnsi& domainName)
{
#if USE_MONO_AOT
    LOG(Fatal, "Scripts can run only in single domain mode with AOT mode enabled.");
    return nullptr;
#endif

    for (int32 i = 0; i < MDomains.Count(); i++)
    {
        if (MDomains[i]->GetName() == domainName)
            return MDomains[i];
    }

    auto domain = New<MDomain>(domainName);
    const auto monoDomain = mono_domain_create_appdomain((char*)domainName.Get(), nullptr);
#if MONO_DEBUG_ENABLE
    mono_debug_domain_create(monoDomain);
#endif
    ASSERT(monoDomain);
    domain->_monoDomain = monoDomain;
    MDomains.Add(domain);
    return domain;
}

void MCore::UnloadDomain(const StringAnsi& domainName)
{
    int32 i = 0;
    for (; i < MDomains.Count(); i++)
    {
        if (MDomains[i]->GetName() == domainName)
            break;
    }
    if (i == MDomains.Count())
        return;

    auto domain = MDomains[i];
#if MONO_DEBUG_ENABLE
    //mono_debug_domain_unload(domain->GetNative());
#endif
    //mono_domain_finalize(domain->GetNative(), 2000);
    MObject* exception = nullptr;
    mono_domain_try_unload(domain->GetNative(), &exception);
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Fatal, TEXT("Scripting::Release"));
    }
    Delete(domain);
    MDomains.RemoveAtKeepOrder(i);
}

#if 0

void* MonoMalloc(size_t size)
{
	return malloc(size);
}

void* MonoRealloc(void* mem, size_t count)
{
	return realloc(mem, count);
}

void MonoFree(void* mem)
{
	return free(mem);
}

void* MonoCalloc(size_t count, size_t size)
{
	return calloc(count, size);
}

#endif

#if USE_MONO_PROFILER

#include "Engine/Core/Types/StringBuilder.h"

struct FlaxMonoProfiler
{
};

FlaxMonoProfiler Profiler;

struct StackWalkDataResult
{
    StringBuilder Buffer;
};

mono_bool OnStackWalk(MonoMethod* method, int32_t native_offset, int32_t il_offset, mono_bool managed, void* data)
{
    auto result = (StackWalkDataResult*)data;

    if (method)
    {
        auto mName = mono_method_get_name(method);
        auto mKlassNameSpace = mono_class_get_namespace(mono_method_get_class(method));
        auto mKlassName = mono_class_get_name(mono_method_get_class(method));
        result->Buffer.Append(mKlassNameSpace);
        result->Buffer.Append(TEXT("."));
        result->Buffer.Append(mKlassName);
        result->Buffer.Append(TEXT("::"));
        result->Buffer.Append(mName);
        result->Buffer.Append(TEXT("\n"));
    }
    else if (!managed)
    {
        result->Buffer.Append(TEXT("<unmanaged>\n"));
    }

    return 0;
}

void OnGCAllocation(MonoProfiler* profiler, MonoObject* obj)
{
    // Get allocation info
    auto klass = mono_object_get_class(obj);
    //auto name_space = mono_class_get_namespace(klass);
    //auto name = mono_class_get_name(klass);
    auto size = mono_class_instance_size(klass);

    //LOG(Info, "GC new: {0}.{1} ({2} bytes)", name_space, name, size);

#if 0
    if (ProfilerCPU::IsProfilingCurrentThread())
    {
        static int details = 0;
        if (details)
        {
            StackWalkDataResult stackTrace;
            stackTrace.Buffer.SetCapacity(1024);
            mono_stack_walk(&OnStackWalk, &stackTrace);

            const auto msg = String::Format(TEXT("GC new: {0}.{1} ({2} bytes). Stack Trace:\n{3}"), String(name_space), String(name), size, stackTrace.Buffer.ToStringView());
            Platform::Log(*msg);
            //LOG_STR(Info, msg);
        }
    }
#endif

#if COMPILE_WITH_PROFILER
    // Register allocation during the current CPU event
    auto thread = ProfilerCPU::GetCurrentThread();
    if (thread != nullptr && thread->Buffer.GetCount() != 0)
    {
        auto& activeEvent = thread->Buffer.Last().Event();
        if (activeEvent.End < ZeroTolerance)
        {
            activeEvent.ManagedMemoryAllocation += size;
        }
    }
#endif
}

void OnGCEvent(MonoProfiler* profiler, MonoProfilerGCEvent event, uint32_t generation, mono_bool is_serial)
{
#if COMPILE_WITH_PROFILER
    // GC
    static int32 ActiveEventIndex;
    if (event == MONO_GC_EVENT_PRE_STOP_WORLD_LOCKED)
    {
        ActiveEventIndex = ProfilerCPU::BeginEvent(TEXT("Garbage Collection"));
    }
    else if (event == MONO_GC_EVENT_POST_START_WORLD_UNLOCKED)
    {
        ProfilerCPU::EndEvent(ActiveEventIndex);
    }
#endif
}

#endif

void OnLogCallback(const char* logDomain, const char* logLevel, const char* message, mono_bool fatal, void* userData)
{
    String currentDomain(logDomain);
    String msg(message);
    msg.Replace('\n', ' ');

    static const char* monoErrorLevels[] =
    {
        nullptr,
        "error",
        "critical",
        "warning",
        "message",
        "info",
        "debug"
    };

    uint32 errorLevel = 0;
    if (logLevel != nullptr)
    {
        for (uint32 i = 1; i < 7; i++)
        {
            if (strcmp(monoErrorLevels[i], logLevel) == 0)
            {
                errorLevel = i;
                break;
            }
        }
    }

    if (currentDomain.IsEmpty())
    {
        auto domain = MCore::GetActiveDomain();
        if (domain != nullptr)
        {
            currentDomain = domain->GetName().Get();
        }
        else
        {
            currentDomain = "null";
        }
    }

#if 0
	// Print C# stack trace (crash may be caused by the managed code)
	if (mono_domain_get() && Assemblies::FlaxEngine.Assembly->IsLoaded())
	{
		const auto managedStackTrace = DebugLog::GetStackTrace();
		if (managedStackTrace.HasChars())
		{
			LOG(Warning, "Managed stack trace:");
			LOG_STR(Warning, managedStackTrace);
		}
	}
#endif

    if (errorLevel == 0)
    {
        Log::CLRInnerException(String::Format(TEXT("Message: {0} | Domain: {1}"), msg, currentDomain)).SetLevel(LogType::Error);
    }
    else if (errorLevel <= 2)
    {
        Log::CLRInnerException(String::Format(TEXT("Message: {0} | Domain: {1}"), msg, currentDomain)).SetLevel(LogType::Error);
    }
    else if (errorLevel <= 3)
    {
        LOG(Warning, "Message: {0} | Domain: {1}", msg, currentDomain);
    }
    else
    {
        LOG(Info, "Message: {0} | Domain: {1}", msg, currentDomain);
    }
}

void OnPrintCallback(const char* string, mono_bool isStdout)
{
    LOG_STR(Warning, String(string));
}

void OnPrintErrorCallback(const char* string, mono_bool isStdout)
{
    // HACK: ignore this message
    if (string && Platform::MemoryCompare(string, "debugger-agent: Unable to listen on ", 36) == 0)
        return;

    LOG_STR(Error, String(string));
}

#if PLATFORM_LINUX && !USE_MONO_DYNAMIC_LIB

#include <dlfcn.h>

#define MONO_THIS_LIB_HANDLE ((void*)(intptr)-1)

static void* ThisLibHandle = nullptr;

static void* OnMonoLinuxDlOpen(const char* name, int flags, char** err, void* user_data)
{
    void* result = nullptr;
	if (name && StringUtils::Compare(name + StringUtils::Length(name) - 17, "libmono-native.so") == 0)
    {
        result = MONO_THIS_LIB_HANDLE;
    }
    return result;
}

static void* OnMonoLinuxDlSym(void* handle, const char* name, char** err, void* user_data)
{
	void* result = nullptr;
    if (handle == MONO_THIS_LIB_HANDLE && ThisLibHandle != nullptr)
    {
        result = dlsym(ThisLibHandle, name);
    }
	return result;
}

#endif

bool MCore::LoadEngine()
{
    PROFILE_CPU();
    ASSERT(Globals::MonoPath.IsANSI());

    // Debugging Mono GC
    //Platform::SetEnvironmentVariable(TEXT("MONO_GC_DEBUG"), TEXT("6:gc-log.txt,check-remset-consistency,nursery-canaries"));

#if 0
    // Override memory allocation callback
    // TODO: use ENABLE_OVERRIDABLE_ALLOCATORS when building Mono to support memory callbacks or use counters for memory profiling
    MonoAllocatorVTable alloc;
    alloc.version = MONO_ALLOCATOR_VTABLE_VERSION;
    alloc.malloc = MonoMalloc;
    alloc.realloc = MonoRealloc;
    alloc.free = MonoFree;
    alloc.calloc = MonoCalloc;
    mono_set_allocator_vtable(&alloc);
#endif

#if USE_MONO_AOT
    mono_jit_set_aot_mode(USE_MONO_AOT_MODE);
#endif

#ifdef USE_MONO_AOT_MODULE
    // Load AOT module
    const DateTime aotModuleLoadStartTime = DateTime::Now();
    LOG(Info, "Loading Mono AOT module...");
    void* libAotModule = Platform::LoadLibrary(TEXT(USE_MONO_AOT_MODULE));
    if (libAotModule == nullptr)
    {
        LOG(Error, "Failed to laod Mono AOT module (" TEXT(USE_MONO_AOT_MODULE) ")");
        return true;
    }
    MonoAotModuleHandle = libAotModule;
    void* getModulesPtr = Platform::GetProcAddress(libAotModule, "GetMonoModules");
    if (getModulesPtr == nullptr)
    {
        LOG(Error, "Failed to get Mono AOT modules getter.");
        return true;
    }
    typedef int (*GetMonoModulesFunc)(void** buffer, int bufferSize);
    const auto getModules = (GetMonoModulesFunc)getModulesPtr;
    const int32 moduelsCount = getModules(nullptr, 0);
    void** modules = (void**)Allocator::Allocate(moduelsCount * sizeof(void*));
    getModules(modules, moduelsCount);
    for (int32 i = 0; i < moduelsCount; i++)
    {
        mono_aot_register_module((void**)modules[i]);
    }
    Allocator::Free(modules);
    LOG(Info, "Mono AOT module loaded in {0}ms", (int32)(DateTime::Now() - aotModuleLoadStartTime).GetTotalMilliseconds());
#endif

    // Set mono assemblies path
    StringAnsi pathLib = (Globals::MonoPath / TEXT("/lib")).ToStringAnsi();
    StringAnsi pathEtc = (Globals::MonoPath / TEXT("/etc")).ToStringAnsi();
    mono_set_dirs(pathLib.Get(), pathEtc.Get());

    // Setup debugger
    {
        int32 debuggerLogLevel = 0;
        if (CommandLine::Options.MonoLog.IsTrue())
        {
            LOG(Info, "Using detailed Mono logging");
            mono_trace_set_level_string("debug");
            debuggerLogLevel = 10;
        }
        else
        {
            mono_trace_set_level_string("warning");
        }

#if MONO_DEBUG_ENABLE && !PLATFORM_SWITCH
        StringAnsi debuggerIp = "127.0.0.1";
        uint16 debuggerPort = 41000 + Platform::GetCurrentProcessId() % 1000;
        if (CommandLine::Options.DebuggerAddress.HasValue())
        {
            const auto& address = CommandLine::Options.DebuggerAddress.GetValue();
            const int32 splitIndex = address.Find(':');
            if (splitIndex == INVALID_INDEX)
            {
                debuggerIp = address.ToStringAnsi();
            }
            else
            {
                debuggerIp = address.Left(splitIndex).ToStringAnsi();
                StringUtils::Parse(address.Right(address.Length() - splitIndex - 1).Get(), &debuggerPort);
            }
        }

        char buffer[150];
        sprintf(buffer, "--debugger-agent=transport=dt_socket,address=%s:%d,embedding=1,server=y,suspend=%s,loglevel=%d", debuggerIp.Get(), debuggerPort, CommandLine::Options.WaitForDebugger ? "y,timeout=5000" : "n", debuggerLogLevel);

        const char* options[] = {
            "--soft-breakpoints",
            //"--optimize=float32",
            buffer
        };
        mono_jit_parse_options(ARRAY_COUNT(options), (char**)options);

        mono_debug_init(MONO_DEBUG_FORMAT_MONO, 0);
        LOG(Info, "Mono debugger server at {0}:{1}", ::String(debuggerIp), debuggerPort);
#endif

        // Connects to mono engine callback system
        mono_trace_set_log_handler(OnLogCallback, nullptr);
        mono_trace_set_print_handler(OnPrintCallback);
        mono_trace_set_printerr_handler(OnPrintErrorCallback);
    }

#if USE_MONO_PROFILER
    // Setup profiler options
    bool useExternalProfiler = false;
    {
        ::String monoEnvOptions;
        if (!Platform::GetEnvironmentVariable(TEXT("MONO_ENV_OPTIONS"), monoEnvOptions))
        {
            const StringView prefix(TEXT("--profile="));
            if (monoEnvOptions.StartsWith(prefix))
            {
                monoEnvOptions = monoEnvOptions.Substring(prefix.Length());
                LOG(Info, "Loading Mono profiler with options \'{0}\'", monoEnvOptions);
                StringAnsi monoEnvOptionsAnsi(monoEnvOptions);
                mono_profiler_load(monoEnvOptionsAnsi.Get());
                useExternalProfiler = true;
            }
        }
    }
#endif

#if PLATFORM_ANDROID
    // Disable any AOT code on Android
    mono_jit_set_aot_mode(MONO_AOT_MODE_NONE);

    // Hint to use default system assemblies location
    const StringAnsi assembliesPath = (Globals::MonoPath / TEXT("/lib/mono/2.1")).ToStringAnsi();
    mono_set_assemblies_path(*assembliesPath);
#elif PLATFORM_LINUX
    // Adjust GC threads suspending mode on Linux
    Platform::SetEnvironmentVariable(TEXT("MONO_THREADS_SUSPEND"), TEXT("preemptive"));

#if !USE_MONO_DYNAMIC_LIB
    // Hook for missing library (when using static linking)
    ThisLibHandle = dlopen(nullptr, RTLD_LAZY);
	mono_dl_fallback_register(OnMonoLinuxDlOpen, OnMonoLinuxDlSym, nullptr, nullptr);
#endif
#elif PLATFORM_MAC
    // Adjust GC threads suspending mode on Mac
    Platform::SetEnvironmentVariable(TEXT("MONO_THREADS_SUSPEND"), TEXT("preemptive"));
#endif
    const char* configPath = nullptr;
#if PLATFORM_SWITCH
    StringAnsi configPathBuf = (Globals::MonoPath / TEXT("/etc/mono/config")).ToStringAnsi();
    configPath = *configPathBuf;
    const StringAnsi assembliesPath = (Globals::MonoPath / TEXT("/lib/mono/4.5")).ToStringAnsi();
    mono_set_assemblies_path(*assembliesPath);
#endif
    mono_config_parse(configPath);

#if USE_MONO_PROFILER
    // Init profiler
    if (!useExternalProfiler)
    {
        const MonoProfilerHandle profilerHandle = mono_profiler_create((MonoProfiler*)&Profiler);
        mono_profiler_set_gc_allocation_callback(profilerHandle, &OnGCAllocation);
        mono_profiler_set_gc_event_callback(profilerHandle, &OnGCEvent);
        mono_profiler_enable_allocations();
    }
#endif

    // Init managed runtime
#if PLATFORM_ANDROID
    const char* monoVersion = "mobile";
#else
    const char* monoVersion = "v4.0.30319";
#endif
    auto monoRootDomain = mono_jit_init_version("Flax", monoVersion);
    ASSERT(monoRootDomain);
    MRootDomain = New<MDomain>("Root");
    MRootDomain->_monoDomain = monoRootDomain;
    MDomains.Add(MRootDomain);

    auto exePath = Platform::GetExecutableFilePath();
    auto configDir = StringUtils::GetDirectoryName(exePath).ToStringAnsi();
    auto configFilename = StringUtils::GetFileName(exePath).ToStringAnsi() + ".config";
#if PLATFORM_UWP
	// Change the app root to Mono sub directory to prevent loading .Net Core assemblies from the AppX root folder
	configDir += "\\Mono";
#elif PLATFORM_SWITCH
    // Make config file path absolute
    configFilename = exePath.ToStringAnsi() + ".config";
#endif
    mono_domain_set_config(monoRootDomain, configDir.Get(), configFilename.Get());
    mono_thread_set_main(mono_thread_current());

    // Info
    char* buildInfo = mono_get_runtime_build_info();
    LOG(Info, "Mono runtime version: {0}", ::String(buildInfo));
    mono_free(buildInfo);

    return false;
}

#if PLATFORM_WINDOWS && USE_EDITOR
long MonoHackSehExceptionHandler(class EXCEPTION_POINTERS* ep)
{
    LOG(Error, "Mono crashed on exit");
    return 1;
}
#endif

void MCore::UnloadEngine()
{
    // Only root domain should be alive at this point
    for (auto domain : MDomains)
    {
        if (domain != MRootDomain)
            Delete(domain);
    }
    MDomains.Clear();

    if (MRootDomain)
    {
#if PLATFORM_WINDOWS && USE_EDITOR
        // TODO: reduce issues with hot-reloading C# DLLs because sometimes it crashes on exit
        __try
#endif
        {
            mono_jit_cleanup(MRootDomain->GetNative());
        }
#if PLATFORM_WINDOWS && USE_EDITOR
        __except (MonoHackSehExceptionHandler(nullptr))
        {
        }
#endif
        Delete(MRootDomain);
        MRootDomain = nullptr;
    }

#ifdef USE_MONO_AOT_MODULE
    Platform::FreeLibrary(MonoAotModuleHandle);
#endif

#if PLATFORM_LINUX && !USE_MONO_DYNAMIC_LIB
    if (ThisLibHandle)
    {
        dlclose(ThisLibHandle);
        ThisLibHandle = nullptr;
    }
#endif
}

#if USE_EDITOR

void MCore::ReloadScriptingAssemblyLoadContext()
{
}

#endif

MObject* MCore::Object::Box(void* value, const MClass* klass)
{
    return mono_value_box(mono_domain_get(), klass->GetNative(), value);
}

void* MCore::Object::Unbox(MObject* obj)
{
    return (byte*)obj + sizeof(MonoObject);
    //return mono_object_unbox(obj);
}

MObject* MCore::Object::New(const MClass* klass)
{
    return mono_object_new(mono_domain_get(), klass->GetNative());
}

void MCore::Object::Init(MObject* obj)
{
    mono_runtime_object_init(obj);
}

MClass* MCore::Object::GetClass(MObject* obj)
{
    MonoClass* mclass = mono_object_get_class(obj);
    return FindClass(mclass);
}

MString* MCore::Object::ToString(MObject* obj)
{
    return mono_object_to_string(obj, nullptr);
}

int32 MCore::Object::GetHashCode(MObject* obj)
{
    return mono_object_hash(obj);
}

MString* MCore::String::GetEmpty(MDomain* domain)
{
    MonoDomain* mdomain = domain ? domain->GetNative() : mono_domain_get();
    return mono_string_empty(mdomain);
}

MString* MCore::String::New(const char* str, int32 length, MDomain* domain)
{
    MonoDomain* mdomain = domain ? domain->GetNative() : mono_domain_get();
    return mono_string_new_len(mdomain, str, length);
}

MString* MCore::String::New(const Char* str, int32 length, MDomain* domain)
{
    MonoDomain* mdomain = domain ? domain->GetNative() : mono_domain_get();
    return mono_string_new_utf16(mdomain, str, length);
}

StringView MCore::String::GetChars(MString* obj)
{
    return StringView(mono_string_chars(obj), (int32)mono_string_length(obj));
}

MArray* MCore::Array::New(const MClass* elementKlass, int32 length)
{
    // TODO: use shared empty arrays cache
    return mono_array_new(mono_domain_get(), elementKlass->GetNative(), length);
}

MClass* MCore::Array::GetClass(MClass* elementKlass)
{
    MonoClass* monoClass = mono_array_class_get(elementKlass->GetNative(), 1);
    return FindClass(monoClass);
}

MClass* MCore::Array::GetArrayClass(const MArray* obj)
{
    CRASH; // Not applicable
    return nullptr;
}

int32 MCore::Array::GetLength(const MArray* obj)
{
    return (int32)mono_array_length((MonoArray*)obj);
}

void* MCore::Array::GetAddress(const MArray* obj)
{
    return mono_array_addr_with_size((MonoArray*)obj, 0, 0);
}

MArray* MCore::Array::Unbox(MObject* obj)
{
    return (MArray*)obj;
}

MGCHandle MCore::GCHandle::New(MObject* obj, bool pinned)
{
    return mono_gchandle_new(obj, pinned);
}

MGCHandle MCore::GCHandle::NewWeak(MObject* obj, bool trackResurrection)
{
    return mono_gchandle_new_weakref(obj, trackResurrection);
}

MObject* MCore::GCHandle::GetTarget(const MGCHandle& handle)
{
    return mono_gchandle_get_target(handle);
}

void MCore::GCHandle::Free(const MGCHandle& handle)
{
    mono_gchandle_free(handle);
}

void MCore::GC::Collect()
{
    PROFILE_CPU();
    mono_gc_collect(mono_gc_max_generation());
}

void MCore::GC::Collect(int32 generation)
{
    PROFILE_CPU();
    mono_gc_collect(generation);
}

void MCore::GC::Collect(int32 generation, MGCCollectionMode collectionMode, bool blocking, bool compacting)
{
    PROFILE_CPU();
    mono_gc_collect(generation);
}

int32 MCore::GC::MaxGeneration()
{
    PROFILE_CPU();
    return mono_gc_max_generation();
}

void MCore::GC::WaitForPendingFinalizers()
{
    PROFILE_CPU();
    if (mono_gc_pending_finalizers())
    {
        mono_gc_finalize_notify();
        do
        {
            Platform::Sleep(1);
        } while (mono_gc_pending_finalizers());
    }
}

void MCore::GC::WriteRef(void* ptr, MObject* ref)
{
    mono_gc_wbarrier_generic_store(ptr, ref);
}

void MCore::GC::WriteValue(void* dst, void* src, int32 count, const MClass* klass)
{
    mono_gc_wbarrier_value_copy(dst, src, count, klass->GetNative());
}

void MCore::GC::WriteArrayRef(MArray* dst, MObject* ref, int32 index)
{
    void* ptr = mono_array_addr_with_size(dst, 0, 0);
    mono_gc_wbarrier_set_arrayref(dst, (byte*)ptr + index * sizeof(void*), ref);
}

void MCore::GC::WriteArrayRef(MArray* dst, Span<MObject*> refs)
{
    void* ptr = mono_array_addr_with_size(dst, 0, 0);
    for (int32 index = 0; index < refs.Length(); index++)
        mono_gc_wbarrier_set_arrayref(dst, (byte*)ptr + index * sizeof(void*), refs[index]);
}

void MCore::Thread::Attach()
{
    if (!IsInMainThread() && !mono_domain_get())
    {
        const auto domain = GetActiveDomain();
        ASSERT(domain);
        mono_thread_attach(domain->GetNative());
    }
}

void MCore::Thread::Exit()
{
    if (!IsInMainThread() && mono_domain_get())
    {
        LOG(Info, "Thread 0x{0:x} exits the managed runtime", Platform::GetCurrentThreadID());
        // TODO: use mono_thread_detach but modify mono to call mono_thread_info_detach there so the thread goes into STATE_DETACHED
        mono_thread_exit();
    }
}

bool MCore::Thread::IsAttached()
{
    return mono_domain_get() && mono_thread_current();
}

void MCore::Exception::Throw(MObject* exception)
{
    mono_raise_exception((MonoException*)exception);
}

MObject* MCore::Exception::GetNullReference()
{
    return (MonoObject*)mono_get_exception_null_reference();
}

MObject* MCore::Exception::Get(const char* msg)
{
    return (MonoObject*)mono_exception_from_name_msg(mono_get_corlib(), "System", "Exception", msg);
}

MObject* MCore::Exception::GetArgument(const char* arg, const char* msg)
{
    return (MonoObject*)mono_get_exception_argument(arg, msg);
}

MObject* MCore::Exception::GetArgumentNull(const char* arg)
{
    return (MonoObject*)mono_get_exception_argument_null(arg);
}

MObject* MCore::Exception::GetArgumentOutOfRange(const char* arg)
{
    return (MonoObject*)mono_get_exception_argument_out_of_range(arg);
}

MObject* MCore::Exception::GetNotSupported(const char* msg)
{
    return (MonoObject*)mono_get_exception_not_supported(msg);
}

::String MCore::Type::ToString(MType* type)
{
    return ::String(mono_type_get_name(type));
}

MClass* MCore::Type::GetClass(MType* type)
{
    MonoClass* mclass = mono_class_from_mono_type(type);
    return FindClass(mclass);
}

MType* MCore::Type::GetElementType(MType* type)
{
    CRASH; // impl this (get type class and call GetElementClass)
    return nullptr;
}

int32 MCore::Type::GetSize(MType* type)
{
    int32 valueAlignment;
    return mono_type_stack_size(type, &valueAlignment);
}

MTypes MCore::Type::GetType(MType* type)
{
    return (MTypes)type->type;
}

bool MCore::Type::IsPointer(MType* type)
{
    return mono_type_is_pointer(type) != 0;
}

bool MCore::Type::IsReference(MType* type)
{
    return mono_type_is_reference(type) != 0;
}

MTypeObject* MCore::Type::GetObject(MType* type)
{
    return mono_type_get_object(mono_domain_get(), type);
}

MType* MCore::Type::Get(MTypeObject* type)
{
    return mono_reflection_type_get_type(type);
}

MClass* MAssembly::GetClass(MonoClass* monoClass) const
{
    if (monoClass == nullptr || !IsLoaded() || mono_class_get_image(monoClass) != _monoImage)
        return nullptr;

    // Find class by native pointer
    const auto& classes = GetClasses();
    const auto typeToken = mono_class_get_type_token(monoClass);
    for (auto i = classes.Begin(); i.IsNotEnd(); ++i)
    {
        MonoClass* e = i->Value->GetNative();
        if (e == monoClass || mono_class_get_type_token(e) == typeToken)
        {
            return i->Value;
        }
    }

#if 0
    {
        LOG(Warning, "Failed to find class {0}.{1} in assembly {2}. Classes:", String(mono_class_get_namespace(monoClass)), String(mono_class_get_name(monoClass)), ToString());
        for (auto i = classes.Begin(); i.IsNotEnd(); ++i)
        {
            LOG(Warning, " - {0}", String(i->Key));
        }
    }
#endif
    return nullptr;
}

MonoReflectionAssembly* MAssembly::GetNative() const
{
    if (!_monoAssembly)
        return nullptr;
    return mono_assembly_get_object(mono_domain_get(), _monoAssembly);
}

const MAssembly::ClassesDictionary& MAssembly::GetClasses() const
{
    if (_hasCachedClasses || !IsLoaded())
        return _classes;
    PROFILE_CPU();
    const auto startTime = DateTime::NowUTC();

#if TRACY_ENABLE
    const StringAnsiView monoImageName(mono_image_get_name(_monoImage));
    ZoneText(*monoImageName, monoImageName.Length());
#endif
    ScopeLock lock(BinaryModule::Locker);
    if (_hasCachedClasses)
        return _classes;
    ASSERT(_classes.IsEmpty());
    const int32 numRows = mono_image_get_table_rows(_monoImage, MONO_TABLE_TYPEDEF);
    _classes.EnsureCapacity(numRows * 4);
    for (int32 i = 1; i < numRows; i++) // Skip <Module> class
    {
        MonoClass* klass = mono_class_get(_monoImage, (i + 1) | MONO_TOKEN_TYPE_DEF);

        // Build the typename
        StringAnsi fullname;
        GetClassFullname(klass, fullname);

        // Create class object
        auto mclass = New<MClass>(this, klass, fullname);
        _classes.Add(fullname, mclass);
    }

    const auto endTime = DateTime::NowUTC();
    LOG(Info, "Caching classes for assembly {0} took {1}ms", String(_name), (int32)(endTime - startTime).GetTotalMilliseconds());

#if 0
    for (auto i = _classes.Begin(); i.IsNotEnd(); ++i)
        LOG(Info, "Class: {0}", String(i->Value->GetFullName()));
#endif

    _hasCachedClasses = true;
    return _classes;
}

bool MAssembly::Load(MonoImage* monoImage)
{
    if (IsLoaded())
        return false;
    PROFILE_CPU();
#if TRACY_ENABLE
    const StringAnsiView monoImageName(mono_image_get_name(monoImage));
    ZoneText(*monoImageName, monoImageName.Length());
#endif

    // Ensure to be unloaded
    Unload();

    // Start
    Stopwatch stopwatch;
    OnLoading();

    // Load
    _monoAssembly = mono_image_get_assembly(monoImage);
    if (_monoAssembly == nullptr)
    {
        OnLoadFailed();
        return true;
    }
    _monoImage = monoImage;
    _hasCachedClasses = false;

    // End
    OnLoaded(stopwatch);
    return false;
}

bool MAssembly::LoadCorlib()
{
    return Load(mono_get_corlib());
}

bool MAssembly::LoadImage(const String& assemblyPath, const StringView& nativePath)
{
    // Load assembly file data
    Array<byte> data;
    File::ReadAllBytes(assemblyPath, data);

    // Init Mono image
    MonoImageOpenStatus status;
    const auto name = assemblyPath.ToStringAnsi();
    const auto assemblyImage = mono_image_open_from_data_with_name(reinterpret_cast<char*>(data.Get()), data.Count(), true, &status, false, name.Get());
    if (status != MONO_IMAGE_OK || assemblyImage == nullptr)
    {
        Log::CLRInnerException(TEXT("Mono assembly image is invalid at ") + assemblyPath);
        return true;
    }

    // Setup assembly
    const auto assembly = mono_assembly_load_from_full(assemblyImage, name.Substring(0, name.Length() - 3).Get(), &status, false);
    mono_image_close(assemblyImage);
    if (status != MONO_IMAGE_OK || assembly == nullptr)
    {
        Log::CLRInnerException(TEXT("Mono assembly image is corrupted at ") + assemblyPath);
        return true;
    }

#if MONO_DEBUG_ENABLE
    // Try to load debug symbols (use portable PDB format)
    const auto pdbPath = String(StringUtils::GetPathWithoutExtension(assemblyPath)) + TEXT(".pdb");
    if (FileSystem::FileExists(pdbPath))
    {
        // Load .pdb file
        File::ReadAllBytes(pdbPath, _debugData);

        // Attach debugging symbols to image
        if (_debugData.HasItems())
        {
            mono_debug_open_image_from_memory(assemblyImage, _debugData.Get(), _debugData.Count());
        }
    }

    // TODO: load pdbs for custom third-party libs referenced by game assemblies for debugging
#if 0
    // Hack to load debug information for Newtonsoft.Json (enable it to debug C# code of json lib)
    if (assemblyPath.EndsWith(TEXT("FlaxEngine.CSharp.dll")))
    {
        static Array<byte> NewtonsoftJsonDebugData;
        File::ReadAllBytes(String(StringUtils::GetDirectoryName(assemblyPath)) / TEXT("Newtonsoft.Json.pdb"), NewtonsoftJsonDebugData);
        if (NewtonsoftJsonDebugData.HasItems())
        {
            StringAnsi tmp(String(StringUtils::GetDirectoryName(assemblyPath)) / TEXT("Newtonsoft.Json.dll"));
            MonoAssembly* a = mono_assembly_open(tmp.Get(), &status);
            if (a)
            {
                mono_debug_open_image_from_memory(mono_assembly_get_image(a), NewtonsoftJsonDebugData.Get(), NewtonsoftJsonDebugData.Count());
            }
        }
    }
#endif
#endif

    _monoAssembly = assembly;
    _monoImage = assemblyImage;
    _hasCachedClasses = false;
    _assemblyPath = assemblyPath;
    return false;
}

bool MAssembly::UnloadImage(bool isReloading)
{
    if (_monoImage)
    {
        if (isReloading)
        {
            LOG(Info, "Unloading managed assembly \'{0}\' (is reloading)", String(_name));

            mono_assembly_close(_monoAssembly);
        }
        else
        {
            // NOTE: do not try to close all the opened images
            //       that will cause the domain unload to crash because
            //       the images have already been closed (double free)
        }

        _monoAssembly = nullptr;
        _monoImage = nullptr;
    }
    return false;
}

bool MAssembly::ResolveMissingFile(String& assemblyPath) const
{
    return true;
}

MClass::MClass(const MAssembly* parentAssembly, MonoClass* monoClass, const StringAnsi& fullname)
    : _assembly(parentAssembly)
    , _fullname(fullname)
    , _hasCachedProperties(false)
    , _hasCachedFields(false)
    , _hasCachedMethods(false)
    , _hasCachedAttributes(false)
    , _hasCachedEvents(false)
    , _hasCachedInterfaces(false)
{
    _monoClass = monoClass;
    ASSERT(monoClass);

    const uint32_t flags = mono_class_get_flags(monoClass);

    switch (flags & MONO_TYPE_ATTR_VISIBILITY_MASK)
    {
    case MONO_TYPE_ATTR_NOT_PUBLIC:
    case MONO_TYPE_ATTR_NESTED_PRIVATE:
        _visibility = MVisibility::Private;
        break;
    case MONO_TYPE_ATTR_PUBLIC:
    case MONO_TYPE_ATTR_NESTED_PUBLIC:
        _visibility = MVisibility::Public;
        break;
    case MONO_TYPE_ATTR_NESTED_FAMILY:
    case MONO_TYPE_ATTR_NESTED_ASSEMBLY:
        _visibility = MVisibility::Internal;
        break;
    case MONO_TYPE_ATTR_NESTED_FAM_OR_ASSEM:
        _visibility = MVisibility::ProtectedInternal;
        break;
    case MONO_TYPE_ATTR_NESTED_FAM_AND_ASSEM:
        _visibility = MVisibility::PrivateProtected;
        break;
    default:
        CRASH;
    }

    const uint32_t staticClassFlags = MONO_TYPE_ATTR_ABSTRACT | MONO_TYPE_ATTR_SEALED;
    _isStatic = (flags & staticClassFlags) == staticClassFlags;
    _isSealed = !_isStatic && (flags & MONO_TYPE_ATTR_SEALED) == MONO_TYPE_ATTR_SEALED;
    _isAbstract = !_isStatic && (flags & MONO_TYPE_ATTR_ABSTRACT) == MONO_TYPE_ATTR_ABSTRACT;
    _isInterface = (flags & MONO_TYPE_ATTR_CLASS_SEMANTIC_MASK) == MONO_TYPE_ATTR_INTERFACE;
    _isValueType = mono_class_is_valuetype(monoClass);
    _isEnum = mono_class_is_enum(monoClass);
}

MClass::~MClass()
{
    if (_attrInfo)
        mono_custom_attrs_free((MonoCustomAttrInfo*)_attrInfo);
    _fields.ClearDelete();
    _properties.ClearDelete();
    _methods.ClearDelete();
    _attributes.ClearDelete();
    _events.ClearDelete();
}

StringAnsiView MClass::GetName() const
{
    return StringAnsiView(mono_class_get_name(_monoClass));
}

StringAnsiView MClass::GetNamespace() const
{
    return StringAnsiView(mono_class_get_namespace(_monoClass));
}

MType* MClass::GetType() const
{
    return mono_class_get_type(_monoClass);
}

MClass* MClass::GetBaseClass() const
{
    MonoClass* monoBase = mono_class_get_parent(_monoClass);
    if (monoBase == nullptr)
        return nullptr;
    return FindClass(monoBase);
}

bool MClass::IsSubClassOf(const MClass* klass, bool checkInterfaces) const
{
    return klass && mono_class_is_subclass_of(_monoClass, klass->GetNative(), checkInterfaces) != 0;
}

bool MClass::HasInterface(const MClass* klass) const
{
    return klass && mono_class_is_assignable_from(klass->GetNative(), _monoClass) != 0;
}

bool MClass::IsInstanceOfType(MObject* object) const
{
    if (object == nullptr)
        return false;
    MonoClass* monoClass = mono_object_get_class(object);
    return mono_class_is_subclass_of(monoClass, _monoClass, false) != 0;
}

uint32 MClass::GetInstanceSize() const
{
    uint32 align = 0;
    if (IsValueType())
        return mono_class_value_size(_monoClass, &align);
    return mono_class_instance_size(_monoClass);
}

MClass* MClass::GetElementClass() const
{
    MonoClass* monoClass = mono_class_get_element_class(_monoClass);
    return FindClass(monoClass);
}

MMethod* MClass::GetMethod(const char* name, int32 numParams) const
{
    // Lookup for cached method
    for (int32 i = 0; i < _methods.Count(); i++)
    {
        if (_methods[i]->GetName() == name && _methods[i]->GetParametersCount() == numParams)
            return _methods[i];
    }

    // Find Mono method
    MonoMethod* monoMethod = mono_class_get_method_from_name(_monoClass, name, numParams);
    if (monoMethod == nullptr)
        return nullptr;

    // Create method
    auto method = New<MMethod>(monoMethod, name, (MClass*)this);
    _methods.Add(method);
    return method;
}

const Array<MMethod*>& MClass::GetMethods() const
{
    if (_hasCachedMethods)
        return _methods;

    void* iter = nullptr;
    MonoMethod* curClassMethod;
    while ((curClassMethod = mono_class_get_methods(_monoClass, &iter)))
    {
        // Check if has not been added
        bool isMissing = true;
        for (int32 i = 0; i < _methods.Count(); i++)
        {
            if (_methods[i]->GetNative() == curClassMethod)
            {
                isMissing = false;
                break;
            }
        }

        if (isMissing)
        {
            // Create method
            auto method = New<MMethod>(curClassMethod, (MClass*)this);
            _methods.Add(method);
        }
    }

    _hasCachedMethods = true;
    return _methods;
}

MField* MClass::GetField(const char* name) const
{
    // Lookup for cached field
    for (int32 i = 0; i < _fields.Count(); i++)
    {
        if (_fields[i]->GetName() == name)
            return _fields[i];
    }

    // Find mono field
    MonoClassField* field = mono_class_get_field_from_name(_monoClass, name);
    if (field == nullptr)
        return nullptr;

    // Create field
    auto mfield = New<MField>(field, name, (MClass*)this);
    _fields.Add(mfield);
    return mfield;
}

const Array<MField*>& MClass::GetFields() const
{
    if (_hasCachedFields)
        return _fields;

    void* iter = nullptr;
    MonoClassField* curClassField;
    while ((curClassField = mono_class_get_fields(_monoClass, &iter)))
    {
        const char* fieldName = mono_field_get_name(curClassField);
        GetField(fieldName);
    }

    _hasCachedFields = true;
    return _fields;
}

const Array<MEvent*>& MClass::GetEvents() const
{
    if (_hasCachedEvents)
        return _events;

    void* iter = nullptr;
    MonoEvent* curEvent;
    while ((curEvent = mono_class_get_events(_monoClass, &iter)))
    {
        const char* name = mono_event_get_name(curEvent);
        bool missing = true;
        for (int32 i = 0; i < _events.Count(); i++)
        {
            if (_events[i]->GetName() == name)
            {
                missing = false;
                break;
            }
        }
        if (missing)
        {
            auto result = New<MEvent>(curEvent, name, (MClass*)this);
            _events.Add(result);
        }
    }

    _hasCachedEvents = true;
    return _events;
}

MProperty* MClass::GetProperty(const char* name) const
{
    // Lookup for cached property
    for (int32 i = 0; i < _properties.Count(); i++)
    {
        if (_properties[i]->GetName() == name)
            return _properties[i];
    }

    // Find mono property
    MonoProperty* monoProperty = mono_class_get_property_from_name(_monoClass, name);
    if (monoProperty == nullptr)
        return nullptr;

    // Create method
    auto mproperty = New<MProperty>(monoProperty, name, (MClass*)this);
    _properties.Add(mproperty);

    return mproperty;
}

class PhysicsColliderActorInternal
{
public:
    void CollisionEnter_ManagedWrapper(const int32& arg0)
    {
        MMethod* mmethod = nullptr;
        CHECK(mmethod);
        PROFILE_CPU_NAMED("FlaxEngine.PhysicsColliderActor::OnCollisionEnter");
        MonoObject* exception = nullptr;
        void* params[1];
        MonoObject* instance;
        mono_runtime_invoke(mmethod->GetNative(), instance, params, &exception);
        mmethod->Invoke(instance, params, &exception);
    }
};

const Array<MProperty*>& MClass::GetProperties() const
{
    if (_hasCachedProperties)
        return _properties;

    void* iter = nullptr;
    MonoProperty* curClassProperty;
    while ((curClassProperty = mono_class_get_properties(_monoClass, &iter)))
    {
        const char* propertyName = mono_property_get_name(curClassProperty);
        GetProperty(propertyName);
    }

    _hasCachedProperties = true;
    return _properties;
}

const Array<MClass*>& MClass::GetInterfaces() const
{
    if (_hasCachedInterfaces)
        return _interfaces;
    
    void* iter = nullptr;
    MonoClass* curInterface;
    while ((curInterface = mono_class_get_interfaces(_monoClass, &iter)))
    {
        _interfaces.Add(FindClass(curInterface));
    }

    _hasCachedInterfaces = true;
    return _interfaces;
}

bool MClass::HasAttribute(const MClass* klass) const
{
    MonoCustomAttrInfo* attrInfo = GET_CUSTOM_ATTR();
    return attrInfo != nullptr && mono_custom_attrs_has_attr(attrInfo, klass->GetNative()) != 0;
}

bool MClass::HasAttribute() const
{
    MonoCustomAttrInfo* attrInfo = GET_CUSTOM_ATTR();
    return attrInfo && attrInfo->num_attrs > 0;
}

MObject* MClass::GetAttribute(const MClass* klass) const
{
    MonoCustomAttrInfo* attrInfo = GET_CUSTOM_ATTR();
    return attrInfo ? mono_custom_attrs_get_attr(attrInfo, klass->GetNative()) : nullptr;
}

const Array<MObject*>& MClass::GetAttributes() const
{
    if (_hasCachedAttributes)
        return _attributes;
    _hasCachedAttributes = true;

    MonoCustomAttrInfo* attrInfo = GET_CUSTOM_ATTR();
    if (attrInfo == nullptr)
        return _attributes;

    MonoArray* monoAttributesArray = mono_custom_attrs_construct(attrInfo);
    const uint32 length = (uint32)mono_array_length(monoAttributesArray);
    _attributes.Resize(length);
    for (uint32 i = 0; i < length; i++)
        _attributes[i] = mono_array_get(monoAttributesArray, MonoObject*, i);
    mono_custom_attrs_free(attrInfo);
    return _attributes;
}

bool MDomain::SetCurrentDomain(bool force)
{
    if (mono_domain_set(_monoDomain, force) == 0)
        return false;
    MActiveDomain = this;
    return true;
}

void MDomain::Dispatch() const
{
    if (!IsInMainThread())
    {
        mono_thread_attach(_monoDomain);
    }
}

MEvent::MEvent(MonoEvent* monoEvent, const char* name, MClass* parentClass)
    : _monoEvent(monoEvent)
    , _addMethod(nullptr)
    , _removeMethod(nullptr)
    , _parentClass(parentClass)
    , _name(name)
    , _hasCachedAttributes(false)
    , _hasAddMonoMethod(true)
    , _hasRemoveMonoMethod(true)
{
#if BUILD_DEBUG
    // Validate input name
    ASSERT(StringUtils::Compare(name, mono_event_get_name(monoEvent)) == 0);
#endif
}

MMethod* MEvent::GetAddMethod() const
{
    if (!_hasAddMonoMethod)
        return nullptr;
    if (_addMethod == nullptr)
    {
        MonoMethod* addMonoMethod = mono_event_get_add_method(_monoEvent);
        if (addMonoMethod != nullptr)
        {
            _hasAddMonoMethod = true;
            return _addMethod = New<MMethod>(addMonoMethod, (MClass*)_parentClass);
        }
    }
    return _addMethod;
}

MMethod* MEvent::GetRemoveMethod() const
{
    if (!_hasRemoveMonoMethod)
        return nullptr;
    if (_removeMethod == nullptr)
    {
        MonoMethod* removeMonoMethod = mono_event_get_remove_method(_monoEvent);
        if (removeMonoMethod)
        {
            _hasRemoveMonoMethod = true;
            return _removeMethod = New<MMethod>(removeMonoMethod, (MClass*)_parentClass);
        }
    }
    return _removeMethod;
}

bool MEvent::HasAttribute(const MClass* klass) const
{
    MonoClass* parentClass = mono_event_get_parent(_monoEvent);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_event(parentClass, _monoEvent);
    if (attrInfo == nullptr)
        return false;

    const bool hasAttr = mono_custom_attrs_has_attr(attrInfo, klass->GetNative()) != 0;
    mono_custom_attrs_free(attrInfo);
    return hasAttr;
}

bool MEvent::HasAttribute() const
{
    MonoClass* parentClass = mono_event_get_parent(_monoEvent);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_event(parentClass, _monoEvent);
    if (attrInfo == nullptr)
        return false;

    if (attrInfo->num_attrs > 0)
    {
        mono_custom_attrs_free(attrInfo);
        return true;
    }
    mono_custom_attrs_free(attrInfo);
    return false;
}

MObject* MEvent::GetAttribute(const MClass* klass) const
{
    MonoClass* parentClass = mono_event_get_parent(_monoEvent);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_event(parentClass, _monoEvent);
    if (attrInfo == nullptr)
        return nullptr;

    MonoObject* foundAttr = mono_custom_attrs_get_attr(attrInfo, klass->GetNative());
    mono_custom_attrs_free(attrInfo);
    return foundAttr;
}

const Array<MObject*>& MEvent::GetAttributes() const
{
    if (_hasCachedAttributes)
        return _attributes;
    _hasCachedAttributes = true;

    MonoClass* parentClass = mono_event_get_parent(_monoEvent);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_event(parentClass, _monoEvent);
    if (attrInfo == nullptr)
        return _attributes;

    MonoArray* monoAttributesArray = mono_custom_attrs_construct(attrInfo);
    const uint32 length = (uint32)mono_array_length(monoAttributesArray);
    _attributes.Resize(length);
    for (uint32 i = 0; i < length; i++)
        _attributes[i] = mono_array_get(monoAttributesArray, MonoObject*, i);
    mono_custom_attrs_free(attrInfo);
    return _attributes;
}

MException::MException(MObject* exception)
    : InnerException(nullptr)
{
    ASSERT(exception);

    MonoClass* exceptionClass = mono_object_get_class(exception);
    MonoProperty* exceptionMsgProp = mono_class_get_property_from_name(exceptionClass, "Message");
    MonoMethod* exceptionMsgGetter = mono_property_get_get_method(exceptionMsgProp);
    MonoString* exceptionMsg = (MonoString*)mono_runtime_invoke(exceptionMsgGetter, exception, nullptr, nullptr);
    Message = MUtils::ToString(exceptionMsg);

    MonoProperty* exceptionStackProp = mono_class_get_property_from_name(exceptionClass, "StackTrace");
    MonoMethod* exceptionStackGetter = mono_property_get_get_method(exceptionStackProp);
    MonoString* exceptionStackTrace = (MonoString*)mono_runtime_invoke(exceptionStackGetter, exception, nullptr, nullptr);
    StackTrace = MUtils::ToString(exceptionStackTrace);

    MonoProperty* innerExceptionProp = mono_class_get_property_from_name(exceptionClass, "InnerException");
    MonoMethod* innerExceptionGetter = mono_property_get_get_method(innerExceptionProp);
    MonoObject* innerException = (MonoObject*)mono_runtime_invoke(innerExceptionGetter, exception, nullptr, nullptr);
    if (innerException)
        InnerException = New<MException>(innerException);
}

MException::~MException()
{
    if (InnerException)
        Delete(InnerException);
}

MField::MField(MonoClassField* monoField, const char* name, MClass* parentClass)
    : _monoField(monoField)
    , _monoType(mono_field_get_type(monoField))
    , _parentClass(parentClass)
    , _name(name)
    , _hasCachedAttributes(false)
{
#if BUILD_DEBUG
    // Validate input name
    ASSERT(StringUtils::Compare(name, mono_field_get_name(monoField)) == 0);
#endif

    const uint32_t flags = mono_field_get_flags(monoField);
    switch (flags & MONO_FIELD_ATTR_FIELD_ACCESS_MASK)
    {
    case MONO_FIELD_ATTR_PRIVATE:
        _visibility = MVisibility::Private;
        break;
    case MONO_FIELD_ATTR_FAM_AND_ASSEM:
        _visibility = MVisibility::PrivateProtected;
        break;
    case MONO_FIELD_ATTR_ASSEMBLY:
        _visibility = MVisibility::Internal;
        break;
    case MONO_FIELD_ATTR_FAMILY:
        _visibility = MVisibility::Protected;
        break;
    case MONO_FIELD_ATTR_FAM_OR_ASSEM:
        _visibility = MVisibility::ProtectedInternal;
        break;
    case MONO_FIELD_ATTR_PUBLIC:
        _visibility = MVisibility::Public;
        break;
    default:
        CRASH;
    }
    _isStatic = (flags & MONO_FIELD_ATTR_STATIC) != 0;
}

MType* MField::GetType() const
{
    return _monoType;
}

int32 MField::GetOffset() const
{
    return (int32)(mono_field_get_offset(_monoField) - sizeof(MonoObject));
}

void MField::GetValue(MObject* instance, void* result) const
{
    mono_field_get_value(instance, _monoField, result);
}

MObject* MField::GetValueBoxed(MObject* instance) const
{
    return mono_field_get_value_object(mono_domain_get(), _monoField, instance);
}

void MField::SetValue(MObject* instance, void* value) const
{
    mono_field_set_value(instance, _monoField, value);
}

bool MField::HasAttribute(const MClass* klass) const
{
    MonoClass* parentClass = mono_field_get_parent(_monoField);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_field(parentClass, _monoField);
    if (attrInfo == nullptr)
        return false;

    const bool hasAttr = mono_custom_attrs_has_attr(attrInfo, klass->GetNative()) != 0;
    mono_custom_attrs_free(attrInfo);
    return hasAttr;
}

bool MField::HasAttribute() const
{
    MonoClass* parentClass = mono_field_get_parent(_monoField);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_field(parentClass, _monoField);
    if (attrInfo == nullptr)
        return false;

    if (attrInfo->num_attrs > 0)
    {
        mono_custom_attrs_free(attrInfo);
        return true;
    }
    mono_custom_attrs_free(attrInfo);
    return false;
}

MObject* MField::GetAttribute(const MClass* klass) const
{
    MonoClass* parentClass = mono_field_get_parent(_monoField);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_field(parentClass, _monoField);
    if (attrInfo == nullptr)
        return nullptr;

    MonoObject* foundAttr = mono_custom_attrs_get_attr(attrInfo, klass->GetNative());
    mono_custom_attrs_free(attrInfo);
    return foundAttr;
}

const Array<MObject*>& MField::GetAttributes() const
{
    if (_hasCachedAttributes)
        return _attributes;
    _hasCachedAttributes = true;

    MonoClass* parentClass = mono_field_get_parent(_monoField);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_field(parentClass, _monoField);
    if (attrInfo == nullptr)
        return _attributes;

    MonoArray* monoAttributesArray = mono_custom_attrs_construct(attrInfo);
    const uint32 length = (uint32)mono_array_length(monoAttributesArray);
    _attributes.Resize(length);
    for (uint32 i = 0; i < length; i++)
        _attributes[i] = mono_array_get(monoAttributesArray, MonoObject*, i);
    mono_custom_attrs_free(attrInfo);
    return _attributes;
}

MMethod::MMethod(MonoMethod* monoMethod, MClass* parentClass)
    : MMethod(monoMethod, mono_method_get_name(monoMethod), parentClass)
{
}

MMethod::MMethod(MonoMethod* monoMethod, const char* name, MClass* parentClass)
    : _monoMethod(monoMethod)
    , _parentClass(parentClass)
    , _name(name)
    , _hasCachedAttributes(false)
{
#if BUILD_DEBUG
    // Validate input name
    ASSERT(StringUtils::Compare(name, mono_method_get_name(monoMethod)) == 0);
#endif

    const uint32_t flags = mono_method_get_flags(monoMethod, nullptr);
    _isStatic = (flags & MONO_METHOD_ATTR_STATIC) != 0;
    switch (flags & MONO_METHOD_ATTR_ACCESS_MASK)
    {
    case MONO_METHOD_ATTR_PRIVATE:
        _visibility = MVisibility::Private;
        break;
    case MONO_METHOD_ATTR_FAM_AND_ASSEM:
        _visibility = MVisibility::PrivateProtected;
        break;
    case MONO_METHOD_ATTR_ASSEM:
        _visibility = MVisibility::Internal;
        break;
    case MONO_METHOD_ATTR_FAMILY:
        _visibility = MVisibility::Protected;
        break;
    case MONO_METHOD_ATTR_FAM_OR_ASSEM:
        _visibility = MVisibility::ProtectedInternal;
        break;
    case MONO_METHOD_ATTR_PUBLIC:
        _visibility = MVisibility::Public;
        break;
    default:
        CRASH;
    }

#if COMPILE_WITH_PROFILER
    const StringAnsi& className = parentClass->GetFullName();
    ProfilerName.Resize(className.Length() + 2 + _name.Length());
    Platform::MemoryCopy(ProfilerName.Get(), className.Get(), className.Length());
    ProfilerName.Get()[className.Length()] = ':';
    ProfilerName.Get()[className.Length() + 1] = ':';
    Platform::MemoryCopy(ProfilerName.Get() + className.Length() + 2, _name.Get(), _name.Length());
    ProfilerData.name = ProfilerName.Get();
    ProfilerData.function = _name.Get();
    ProfilerData.file = nullptr;
    ProfilerData.line = 0;
    ProfilerData.color = 0;
#endif
}

MObject* MMethod::Invoke(void* instance, void** params, MObject** exception) const
{
    PROFILE_CPU_SRC_LOC(ProfilerData);
    return mono_runtime_invoke(_monoMethod, instance, params, exception);
}

MObject* MMethod::InvokeVirtual(MObject* instance, void** params, MObject** exception) const
{
    PROFILE_CPU_SRC_LOC(ProfilerData);
    MonoMethod* virtualMethod = mono_object_get_virtual_method(instance, _monoMethod);
    return mono_runtime_invoke(virtualMethod, instance, params, exception);
}

#if !USE_MONO_AOT

void* MMethod::GetThunk()
{
    if (!_cachedThunk)
        _cachedThunk = mono_method_get_unmanaged_thunk(_monoMethod);
    return _cachedThunk;
}

#endif

MMethod* MMethod::InflateGeneric() const
{
    MonoMethod* inflatedMethod = mono_class_inflate_generic_method(_monoMethod, nullptr);
    // TODO: don't leak created method
    return New<MMethod>(inflatedMethod, _parentClass);
}

MType* MMethod::GetReturnType() const
{
    MonoMethodSignature* sig = mono_method_signature(_monoMethod);
    return mono_signature_get_return_type(sig);
}

int32 MMethod::GetParametersCount() const
{
    MonoMethodSignature* sig = mono_method_signature(_monoMethod);
    return (int32)mono_signature_get_param_count(sig);
}

MType* MMethod::GetParameterType(int32 paramIdx) const
{
    MonoMethodSignature* sig = mono_method_signature(_monoMethod);
    ASSERT_LOW_LAYER(paramIdx >= 0 && paramIdx < (int32)mono_signature_get_param_count(sig));
    void* it = nullptr;
    mono_signature_get_params(sig, &it);
    return ((MonoType**)it)[paramIdx];
}

bool MMethod::GetParameterIsOut(int32 paramIdx) const
{
    MonoMethodSignature* sig = mono_method_signature(_monoMethod);
    ASSERT_LOW_LAYER(paramIdx >= 0 && paramIdx < (int32)mono_signature_get_param_count(sig));
    return mono_signature_param_is_out(sig, paramIdx) != 0;
}

bool MMethod::HasAttribute(const MClass* klass) const
{
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_method(_monoMethod);
    if (attrInfo == nullptr)
        return false;

    const bool hasAttr = mono_custom_attrs_has_attr(attrInfo, klass->GetNative()) != 0;
    mono_custom_attrs_free(attrInfo);
    return hasAttr;
}

bool MMethod::HasAttribute() const
{
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_method(_monoMethod);
    if (attrInfo == nullptr)
        return false;

    if (attrInfo->num_attrs > 0)
    {
        mono_custom_attrs_free(attrInfo);
        return true;
    }
    mono_custom_attrs_free(attrInfo);
    return false;
}

MObject* MMethod::GetAttribute(const MClass* klass) const
{
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_method(_monoMethod);
    if (attrInfo == nullptr)
        return nullptr;

    MonoObject* foundAttr = mono_custom_attrs_get_attr(attrInfo, klass->GetNative());
    mono_custom_attrs_free(attrInfo);
    return foundAttr;
}

const Array<MObject*>& MMethod::GetAttributes() const
{
    if (_hasCachedAttributes)
        return _attributes;
    _hasCachedAttributes = true;

    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_method(_monoMethod);
    if (attrInfo == nullptr)
        return _attributes;

    MonoArray* monoAttributesArray = mono_custom_attrs_construct(attrInfo);
    const uint32 length = (uint32)mono_array_length(monoAttributesArray);
    _attributes.Resize(length);
    for (uint32 i = 0; i < length; i++)
        _attributes[i] = mono_array_get(monoAttributesArray, MonoObject*, i);
    mono_custom_attrs_free(attrInfo);
    return _attributes;
}

MProperty::MProperty(MonoProperty* monoProperty, const char* name, MClass* parentClass)
    : _monoProperty(monoProperty)
    , _getMethod(nullptr)
    , _setMethod(nullptr)
    , _parentClass(parentClass)
    , _name(name)
    , _hasCachedAttributes(false)
    , _hasSetMethod(true)
    , _hasGetMethod(true)
{
#if BUILD_DEBUG
    // Validate input name
    ASSERT(StringUtils::Compare(name, mono_property_get_name(monoProperty)) == 0);
#endif

    GetGetMethod();
    GetSetMethod();
}

MProperty::~MProperty()
{
    if (_getMethod)
        Delete(_getMethod);
    if (_setMethod)
        Delete(_setMethod);
}

MMethod* MProperty::GetGetMethod() const
{
    if (!_hasGetMethod)
        return nullptr;
    if (_getMethod == nullptr)
    {
        MonoMethod* method = mono_property_get_get_method(_monoProperty);
        if (method != nullptr)
        {
            _hasGetMethod = true;
            return _getMethod = New<MMethod>(method, (MClass*)_parentClass);
        }
    }
    return _getMethod;
}

MMethod* MProperty::GetSetMethod() const
{
    if (!_hasSetMethod)
        return nullptr;
    if (_setMethod == nullptr)
    {
        MonoMethod* method = mono_property_get_set_method(_monoProperty);
        if (method != nullptr)
        {
            _hasSetMethod = true;
            return _setMethod = New<MMethod>(method, (MClass*)_parentClass);
        }
    }
    return _setMethod;
}

MObject* MProperty::GetValue(MObject* instance, MObject** exception) const
{
    return mono_property_get_value(_monoProperty, instance, nullptr, exception);
}

void MProperty::SetValue(MObject* instance, void* value, MObject** exception) const
{
    void* params[1];
    params[0] = value;
    mono_property_set_value(_monoProperty, instance, params, exception);
}

bool MProperty::HasAttribute(const MClass* klass) const
{
    MonoClass* parentClass = mono_property_get_parent(_monoProperty);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_property(parentClass, _monoProperty);
    if (attrInfo == nullptr)
        return false;

    const bool hasAttr = mono_custom_attrs_has_attr(attrInfo, klass->GetNative()) != 0;
    mono_custom_attrs_free(attrInfo);
    return hasAttr;
}

bool MProperty::HasAttribute() const
{
    MonoClass* parentClass = mono_property_get_parent(_monoProperty);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_property(parentClass, _monoProperty);
    if (attrInfo == nullptr)
        return false;

    if (attrInfo->num_attrs > 0)
    {
        mono_custom_attrs_free(attrInfo);
        return true;
    }
    mono_custom_attrs_free(attrInfo);
    return false;
}

MObject* MProperty::GetAttribute(const MClass* klass) const
{
    MonoClass* parentClass = mono_property_get_parent(_monoProperty);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_property(parentClass, _monoProperty);
    if (attrInfo == nullptr)
        return nullptr;

    MonoObject* foundAttr = mono_custom_attrs_get_attr(attrInfo, klass->GetNative());
    mono_custom_attrs_free(attrInfo);
    return foundAttr;
}

const Array<MObject*>& MProperty::GetAttributes() const
{
    if (_hasCachedAttributes)
        return _attributes;
    _hasCachedAttributes = true;

    MonoClass* parentClass = mono_property_get_parent(_monoProperty);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_property(parentClass, _monoProperty);
    if (attrInfo == nullptr)
        return _attributes;

    MonoArray* monoAttributesArray = mono_custom_attrs_construct(attrInfo);
    const uint32 length = (uint32)mono_array_length(monoAttributesArray);
    _attributes.Resize(length);
    for (uint32 i = 0; i < length; i++)
        _attributes[i] = mono_array_get(monoAttributesArray, MonoObject*, i);
    mono_custom_attrs_free(attrInfo);
    return _attributes;
}

void MCore::ScriptingObject::SetInternalValues(MClass* klass, MObject* object, void* unmanagedPtr, const Guid* id)
{
    // Set handle to unmanaged object
    const MField* monoUnmanagedPtrField = klass->GetField("__unmanagedPtr");
    if (monoUnmanagedPtrField)
    {
        const void* param = unmanagedPtr;
        monoUnmanagedPtrField->SetValue(managedInstance, &param);
    }
    if (id != nullptr)
    {
        // Set object id
        const MField* monoIdField = klass->GetField("__internalId");
        if (monoIdField)
        {
            monoIdField->SetValue(managedInstance, (void*)id);
        }
    }
}

MObject* MCore::ScriptingObject::CreateScriptingObject(MClass* klass, void* unmanagedPtr, const Guid* id)
{
    // Ensure to have managed domain attached (this can be called from custom native thread, eg. content loader)
    MCore::Thread::Attach();

    // Allocate managed instance
    MObject* managedInstance = MCore::Object::New(klass);
    if (managedInstance)
    {
        // Set unmanaged object handle and id
        MCore::ScriptingObject::SetInternalValues(klass, managedInstance, unmanagedPtr, id);

        // Initialize managed instance (calls constructor)
        MCore::Object::Init(managedInstance);
    }
    return managedInstance;
}

#endif

#if USE_MONO && PLATFORM_WIN32 && !USE_MONO_DYNAMIC_LIB

// Export Mono functions
#pragma comment(linker, "/export:mono_add_internal_call")
#pragma comment(linker, "/export:mono_array_addr_with_size")
#pragma comment(linker, "/export:mono_array_calc_byte_len")
#pragma comment(linker, "/export:mono_array_class_get")
#pragma comment(linker, "/export:mono_array_clone")
#pragma comment(linker, "/export:mono_array_clone_checked")
#pragma comment(linker, "/export:mono_array_clone_in_domain")
#pragma comment(linker, "/export:mono_array_element_size")
#pragma comment(linker, "/export:mono_array_full_copy")
#pragma comment(linker, "/export:mono_array_handle_length")
#pragma comment(linker, "/export:mono_array_handle_memcpy_refs")
#pragma comment(linker, "/export:mono_array_handle_pin_with_size")
#pragma comment(linker, "/export:mono_array_length")
#pragma comment(linker, "/export:mono_array_new")
#pragma comment(linker, "/export:mono_array_new_1")
#pragma comment(linker, "/export:mono_array_new_2")
#pragma comment(linker, "/export:mono_array_new_3")
#pragma comment(linker, "/export:mono_array_new_4")
#pragma comment(linker, "/export:mono_array_new_checked")
#pragma comment(linker, "/export:mono_array_new_full")
#pragma comment(linker, "/export:mono_array_new_full_checked")
#pragma comment(linker, "/export:mono_array_new_full_handle")
#pragma comment(linker, "/export:mono_array_new_handle")
#pragma comment(linker, "/export:mono_array_new_specific")
#pragma comment(linker, "/export:mono_array_new_specific_checked")
#pragma comment(linker, "/export:mono_array_new_specific_handle")
#pragma comment(linker, "/export:mono_array_new_va")
#pragma comment(linker, "/export:mono_array_to_byte_byvalarray")
#pragma comment(linker, "/export:mono_array_to_lparray")
#pragma comment(linker, "/export:mono_array_to_savearray")
#pragma comment(linker, "/export:mono_assembly_addref")
#pragma comment(linker, "/export:mono_assembly_binding_applies_to_image")
#pragma comment(linker, "/export:mono_assembly_candidate_predicate_sn_same_name")
#pragma comment(linker, "/export:mono_assembly_cleanup_domain_bindings")
#pragma comment(linker, "/export:mono_assembly_close")
#pragma comment(linker, "/export:mono_assembly_close_except_image_pools")
#pragma comment(linker, "/export:mono_assembly_close_finish")
#pragma comment(linker, "/export:mono_assembly_fill_assembly_name")
#pragma comment(linker, "/export:mono_assembly_fill_assembly_name_full")
#pragma comment(linker, "/export:mono_assembly_foreach")
#pragma comment(linker, "/export:mono_assembly_get_assemblyref")
#pragma comment(linker, "/export:mono_assembly_get_assemblyref_checked")
#pragma comment(linker, "/export:mono_assembly_get_image")
#pragma comment(linker, "/export:mono_assembly_get_image_internal")
#pragma comment(linker, "/export:mono_assembly_get_main")
#pragma comment(linker, "/export:mono_assembly_get_name")
#pragma comment(linker, "/export:mono_assembly_get_name_internal")
#pragma comment(linker, "/export:mono_assembly_get_object")
#pragma comment(linker, "/export:mono_assembly_get_object_handle")
#pragma comment(linker, "/export:mono_assembly_getrootdir")
#pragma comment(linker, "/export:mono_assembly_has_reference_assembly_attribute")
#pragma comment(linker, "/export:mono_assembly_has_skip_verification")
#pragma comment(linker, "/export:mono_assembly_init_weak_fields")
#pragma comment(linker, "/export:mono_assembly_invoke_load_hook")
#pragma comment(linker, "/export:mono_assembly_invoke_search_hook")
#pragma comment(linker, "/export:mono_assembly_invoke_unload_hook")
#pragma comment(linker, "/export:mono_assembly_is_problematic_version")
#pragma comment(linker, "/export:mono_assembly_is_weak_field")
#pragma comment(linker, "/export:mono_assembly_load")
#pragma comment(linker, "/export:mono_assembly_load_corlib")
#pragma comment(linker, "/export:mono_assembly_load_friends")
#pragma comment(linker, "/export:mono_assembly_load_from")
#pragma comment(linker, "/export:mono_assembly_load_from_assemblies_path")
#pragma comment(linker, "/export:mono_assembly_load_from_full")
#pragma comment(linker, "/export:mono_assembly_load_from_predicate")
#pragma comment(linker, "/export:mono_assembly_load_full")
#pragma comment(linker, "/export:mono_assembly_load_full_nosearch")
#pragma comment(linker, "/export:mono_assembly_load_module")
#pragma comment(linker, "/export:mono_assembly_load_module_checked")
#pragma comment(linker, "/export:mono_assembly_load_reference")
#pragma comment(linker, "/export:mono_assembly_load_references")
#pragma comment(linker, "/export:mono_assembly_load_with_partial_name")
#pragma comment(linker, "/export:mono_assembly_load_with_partial_name_internal")
#pragma comment(linker, "/export:mono_assembly_loaded")
#pragma comment(linker, "/export:mono_assembly_loaded_full")
#pragma comment(linker, "/export:mono_assembly_metadata_foreach_custom_attr")
#pragma comment(linker, "/export:mono_assembly_name_free")
#pragma comment(linker, "/export:mono_assembly_name_free_internal")
#pragma comment(linker, "/export:mono_assembly_name_get_culture")
#pragma comment(linker, "/export:mono_assembly_name_get_name")
#pragma comment(linker, "/export:mono_assembly_name_get_pubkeytoken")
#pragma comment(linker, "/export:mono_assembly_name_get_version")
#pragma comment(linker, "/export:mono_assembly_name_new")
#pragma comment(linker, "/export:mono_assembly_name_parse")
#pragma comment(linker, "/export:mono_assembly_name_parse_full")
#pragma comment(linker, "/export:mono_assembly_names_equal")
#pragma comment(linker, "/export:mono_assembly_names_equal_flags")
#pragma comment(linker, "/export:mono_assembly_open")
#pragma comment(linker, "/export:mono_assembly_open_a_lot")
#pragma comment(linker, "/export:mono_assembly_open_from_bundle")
#pragma comment(linker, "/export:mono_assembly_open_full")
#pragma comment(linker, "/export:mono_assembly_open_predicate")
#pragma comment(linker, "/export:mono_assembly_release_gc_roots")
#pragma comment(linker, "/export:mono_assembly_set_main")
#pragma comment(linker, "/export:mono_assembly_setrootdir")
#pragma comment(linker, "/export:mono_class_alloc")
#pragma comment(linker, "/export:mono_class_alloc0")
#pragma comment(linker, "/export:mono_class_array_element_size")
#pragma comment(linker, "/export:mono_class_bind_generic_parameters")
#pragma comment(linker, "/export:mono_class_can_access_class")
#pragma comment(linker, "/export:mono_class_check_context_used")
#pragma comment(linker, "/export:mono_class_check_vtable_constraints")
#pragma comment(linker, "/export:mono_class_compute_bitmap")
#pragma comment(linker, "/export:mono_class_compute_gc_descriptor")
#pragma comment(linker, "/export:mono_class_contextbound_bit_offset")
#pragma comment(linker, "/export:mono_class_create_array")
#pragma comment(linker, "/export:mono_class_create_array_fill_type")
#pragma comment(linker, "/export:mono_class_create_bounded_array")
#pragma comment(linker, "/export:mono_class_create_fnptr")
#pragma comment(linker, "/export:mono_class_create_from_typedef")
#pragma comment(linker, "/export:mono_class_create_generic_inst")
#pragma comment(linker, "/export:mono_class_create_generic_parameter")
#pragma comment(linker, "/export:mono_class_create_ptr")
#pragma comment(linker, "/export:mono_class_data_size")
#pragma comment(linker, "/export:mono_class_describe_statics")
#pragma comment(linker, "/export:mono_class_enum_basetype")
#pragma comment(linker, "/export:mono_class_enum_basetype_internal")
#pragma comment(linker, "/export:mono_class_field_get_special_static_type")
#pragma comment(linker, "/export:mono_class_field_is_special_static")
#pragma comment(linker, "/export:mono_class_fill_runtime_generic_context")
#pragma comment(linker, "/export:mono_class_find_enum_basetype")
#pragma comment(linker, "/export:mono_class_free_ref_info")
#pragma comment(linker, "/export:mono_class_from_generic_parameter")
#pragma comment(linker, "/export:mono_class_from_mono_type")
#pragma comment(linker, "/export:mono_class_from_mono_type_handle")
#pragma comment(linker, "/export:mono_class_from_name")
#pragma comment(linker, "/export:mono_class_from_name_case")
#pragma comment(linker, "/export:mono_class_from_name_case_checked")
#pragma comment(linker, "/export:mono_class_from_name_checked")
#pragma comment(linker, "/export:mono_class_from_typeref")
#pragma comment(linker, "/export:mono_class_from_typeref_checked")
#pragma comment(linker, "/export:mono_class_full_name")
#pragma comment(linker, "/export:mono_class_generic_sharing_enabled")
#pragma comment(linker, "/export:mono_class_get")
#pragma comment(linker, "/export:mono_class_get_and_inflate_typespec_checked")
#pragma comment(linker, "/export:mono_class_get_appdomain_unloaded_exception_class")
#pragma comment(linker, "/export:mono_class_get_byref_type")
#pragma comment(linker, "/export:mono_class_get_cached_class_info")
#pragma comment(linker, "/export:mono_class_get_cctor")
#pragma comment(linker, "/export:mono_class_get_checked")
#pragma comment(linker, "/export:mono_class_get_com_object_class")
#pragma comment(linker, "/export:mono_class_get_context")
#pragma comment(linker, "/export:mono_class_get_declsec_flags")
#pragma comment(linker, "/export:mono_class_get_default_finalize_method")
#pragma comment(linker, "/export:mono_class_get_dim_conflicts")
#pragma comment(linker, "/export:mono_class_get_element_class")
#pragma comment(linker, "/export:mono_class_get_event_info")
#pragma comment(linker, "/export:mono_class_get_event_token")
#pragma comment(linker, "/export:mono_class_get_events")
#pragma comment(linker, "/export:mono_class_get_exception_data")
#pragma comment(linker, "/export:mono_class_get_exception_for_failure")
#pragma comment(linker, "/export:mono_class_get_field")
#pragma comment(linker, "/export:mono_class_get_field_count")
#pragma comment(linker, "/export:mono_class_get_field_def_values")
#pragma comment(linker, "/export:mono_class_get_field_default_value")
#pragma comment(linker, "/export:mono_class_get_field_from_name")
#pragma comment(linker, "/export:mono_class_get_field_from_name_full")
#pragma comment(linker, "/export:mono_class_get_field_token")
#pragma comment(linker, "/export:mono_class_get_fields")
#pragma comment(linker, "/export:mono_class_get_fields_internal")
#pragma comment(linker, "/export:mono_class_get_fields_lazy")
#pragma comment(linker, "/export:mono_class_get_finalizer")
#pragma comment(linker, "/export:mono_class_get_first_field_idx")
#pragma comment(linker, "/export:mono_class_get_first_method_idx")
#pragma comment(linker, "/export:mono_class_get_flags")
#pragma comment(linker, "/export:mono_class_get_full")
#pragma comment(linker, "/export:mono_class_get_generic_class")
#pragma comment(linker, "/export:mono_class_get_generic_container")
#pragma comment(linker, "/export:mono_class_get_generic_type_definition")
#pragma comment(linker, "/export:mono_class_get_idispatch_class")
#pragma comment(linker, "/export:mono_class_get_image")
#pragma comment(linker, "/export:mono_class_get_implemented_interfaces")
#pragma comment(linker, "/export:mono_class_get_inflated_method")
#pragma comment(linker, "/export:mono_class_get_interfaces")
#pragma comment(linker, "/export:mono_class_get_interop_proxy_class")
#pragma comment(linker, "/export:mono_class_get_iunknown_class")
#pragma comment(linker, "/export:mono_class_get_marshal_info")
#pragma comment(linker, "/export:mono_class_get_method_by_index")
#pragma comment(linker, "/export:mono_class_get_method_count")
#pragma comment(linker, "/export:mono_class_get_method_from_name")
#pragma comment(linker, "/export:mono_class_get_method_from_name_checked")
#pragma comment(linker, "/export:mono_class_get_method_from_name_flags")
#pragma comment(linker, "/export:mono_class_get_method_generic")
#pragma comment(linker, "/export:mono_class_get_methods")
#pragma comment(linker, "/export:mono_class_get_methods_by_name")
#pragma comment(linker, "/export:mono_class_get_name")
#pragma comment(linker, "/export:mono_class_get_namespace")
#pragma comment(linker, "/export:mono_class_get_nested_classes_property")
#pragma comment(linker, "/export:mono_class_get_nested_types")
#pragma comment(linker, "/export:mono_class_get_nesting_type")
#pragma comment(linker, "/export:mono_class_get_nullable_param")
#pragma comment(linker, "/export:mono_class_get_object_finalize_slot")
#pragma comment(linker, "/export:mono_class_get_overrides_full")
#pragma comment(linker, "/export:mono_class_get_parent")
#pragma comment(linker, "/export:mono_class_get_properties")
#pragma comment(linker, "/export:mono_class_get_property_default_value")
#pragma comment(linker, "/export:mono_class_get_property_from_name")
#pragma comment(linker, "/export:mono_class_get_property_info")
#pragma comment(linker, "/export:mono_class_get_property_token")
#pragma comment(linker, "/export:mono_class_get_rank")
#pragma comment(linker, "/export:mono_class_get_ref_info")
#pragma comment(linker, "/export:mono_class_get_ref_info_handle")
#pragma comment(linker, "/export:mono_class_get_ref_info_raw")
#pragma comment(linker, "/export:mono_class_get_type")
#pragma comment(linker, "/export:mono_class_get_type_token")
#pragma comment(linker, "/export:mono_class_get_valuetype_class")
#pragma comment(linker, "/export:mono_class_get_variant_class")
#pragma comment(linker, "/export:mono_class_get_virtual_method")
#pragma comment(linker, "/export:mono_class_get_vtable_entry")
#pragma comment(linker, "/export:mono_class_get_vtable_size")
#pragma comment(linker, "/export:mono_class_get_weak_bitmap")
#pragma comment(linker, "/export:mono_class_gtd_get_canonical_inst")
#pragma comment(linker, "/export:mono_class_has_dim_conflicts")
#pragma comment(linker, "/export:mono_class_has_failure")
#pragma comment(linker, "/export:mono_class_has_finalizer")
#pragma comment(linker, "/export:mono_class_has_ref_info")
#pragma comment(linker, "/export:mono_class_has_special_static_fields")
#pragma comment(linker, "/export:mono_class_has_variant_generic_params")
#pragma comment(linker, "/export:mono_class_implements_interface")
#pragma comment(linker, "/export:mono_class_inflate_generic_class_checked")
#pragma comment(linker, "/export:mono_class_inflate_generic_method")
#pragma comment(linker, "/export:mono_class_inflate_generic_method_checked")
#pragma comment(linker, "/export:mono_class_inflate_generic_method_full_checked")
#pragma comment(linker, "/export:mono_class_inflate_generic_type")
#pragma comment(linker, "/export:mono_class_inflate_generic_type_checked")
#pragma comment(linker, "/export:mono_class_inflate_generic_type_with_mempool")
#pragma comment(linker, "/export:mono_class_init")
#pragma comment(linker, "/export:mono_class_init_checked")
#pragma comment(linker, "/export:mono_class_init_sizes")
#pragma comment(linker, "/export:mono_class_instance_size")
#pragma comment(linker, "/export:mono_class_interface_offset")
#pragma comment(linker, "/export:mono_class_interface_offset_with_variance")
#pragma comment(linker, "/export:mono_class_is_assignable_from")
#pragma comment(linker, "/export:mono_class_is_assignable_from_checked")
#pragma comment(linker, "/export:mono_class_is_assignable_from_internal")
#pragma comment(linker, "/export:mono_class_is_assignable_from_slow")
#pragma comment(linker, "/export:mono_class_is_delegate")
#pragma comment(linker, "/export:mono_class_is_enum")
#pragma comment(linker, "/export:mono_class_is_from_assembly")
#pragma comment(linker, "/export:mono_class_is_magic_float")
#pragma comment(linker, "/export:mono_class_is_magic_int")
#pragma comment(linker, "/export:mono_class_is_nullable")
#pragma comment(linker, "/export:mono_class_is_open_constructed_type")
#pragma comment(linker, "/export:mono_class_is_reflection_method_or_constructor")
#pragma comment(linker, "/export:mono_class_is_subclass_of")
#pragma comment(linker, "/export:mono_class_is_valid_enum")
#pragma comment(linker, "/export:mono_class_is_valuetype")
#pragma comment(linker, "/export:mono_class_is_variant_compatible")
#pragma comment(linker, "/export:mono_class_layout_fields")
#pragma comment(linker, "/export:mono_class_load_from_name")
#pragma comment(linker, "/export:mono_class_min_align")
#pragma comment(linker, "/export:mono_class_name_from_token")
#pragma comment(linker, "/export:mono_class_native_size")
#pragma comment(linker, "/export:mono_class_needs_cctor_run")
#pragma comment(linker, "/export:mono_class_num_events")
#pragma comment(linker, "/export:mono_class_num_fields")
#pragma comment(linker, "/export:mono_class_num_methods")
#pragma comment(linker, "/export:mono_class_num_properties")
#pragma comment(linker, "/export:mono_class_publish_gc_descriptor")
#pragma comment(linker, "/export:mono_class_rgctx_get_array_size")
#pragma comment(linker, "/export:mono_class_set_declsec_flags")
#pragma comment(linker, "/export:mono_class_set_dim_conflicts")
#pragma comment(linker, "/export:mono_class_set_event_info")
#pragma comment(linker, "/export:mono_class_set_exception_data")
#pragma comment(linker, "/export:mono_class_set_failure")
#pragma comment(linker, "/export:mono_class_set_field_count")
#pragma comment(linker, "/export:mono_class_set_field_def_values")
#pragma comment(linker, "/export:mono_class_set_first_field_idx")
#pragma comment(linker, "/export:mono_class_set_first_method_idx")
#pragma comment(linker, "/export:mono_class_set_flags")
#pragma comment(linker, "/export:mono_class_set_generic_container")
#pragma comment(linker, "/export:mono_class_set_is_com_object")
#pragma comment(linker, "/export:mono_class_set_marshal_info")
#pragma comment(linker, "/export:mono_class_set_method_count")
#pragma comment(linker, "/export:mono_class_set_nested_classes_property")
#pragma comment(linker, "/export:mono_class_set_nonblittable")
#pragma comment(linker, "/export:mono_class_set_property_info")
#pragma comment(linker, "/export:mono_class_set_ref_info")
#pragma comment(linker, "/export:mono_class_set_ref_info_handle")
#pragma comment(linker, "/export:mono_class_set_type_load_failure")
#pragma comment(linker, "/export:mono_class_set_type_load_failure_causedby_class")
#pragma comment(linker, "/export:mono_class_set_weak_bitmap")
#pragma comment(linker, "/export:mono_class_setup_basic_field_info")
#pragma comment(linker, "/export:mono_class_setup_events")
#pragma comment(linker, "/export:mono_class_setup_fields")
#pragma comment(linker, "/export:mono_class_setup_has_finalizer")
#pragma comment(linker, "/export:mono_class_setup_interface_id")
#pragma comment(linker, "/export:mono_class_setup_interface_offsets")
#pragma comment(linker, "/export:mono_class_setup_interfaces")
#pragma comment(linker, "/export:mono_class_setup_methods")
#pragma comment(linker, "/export:mono_class_setup_mono_type")
#pragma comment(linker, "/export:mono_class_setup_nested_types")
#pragma comment(linker, "/export:mono_class_setup_parent")
#pragma comment(linker, "/export:mono_class_setup_properties")
#pragma comment(linker, "/export:mono_class_setup_runtime_info")
#pragma comment(linker, "/export:mono_class_setup_supertypes")
#pragma comment(linker, "/export:mono_class_setup_vtable")
#pragma comment(linker, "/export:mono_class_setup_vtable_general")
#pragma comment(linker, "/export:mono_class_static_field_address")
#pragma comment(linker, "/export:mono_class_try_get_com_object_class")
#pragma comment(linker, "/export:mono_class_try_get_generic_class")
#pragma comment(linker, "/export:mono_class_try_get_generic_container")
#pragma comment(linker, "/export:mono_class_try_get_safehandle_class")
#pragma comment(linker, "/export:mono_class_try_get_vtable")
#pragma comment(linker, "/export:mono_class_try_load_from_name")
#pragma comment(linker, "/export:mono_class_value_size")
#pragma comment(linker, "/export:mono_class_vtable")
#pragma comment(linker, "/export:mono_class_vtable_checked")
#pragma comment(linker, "/export:mono_custom_attrs_construct")
#pragma comment(linker, "/export:mono_custom_attrs_free")
#pragma comment(linker, "/export:mono_custom_attrs_from_assembly")
#pragma comment(linker, "/export:mono_custom_attrs_from_assembly_checked")
#pragma comment(linker, "/export:mono_custom_attrs_from_builders")
#pragma comment(linker, "/export:mono_custom_attrs_from_class")
#pragma comment(linker, "/export:mono_custom_attrs_from_class_checked")
#pragma comment(linker, "/export:mono_custom_attrs_from_event")
#pragma comment(linker, "/export:mono_custom_attrs_from_event_checked")
#pragma comment(linker, "/export:mono_custom_attrs_from_field")
#pragma comment(linker, "/export:mono_custom_attrs_from_field_checked")
#pragma comment(linker, "/export:mono_custom_attrs_from_index")
#pragma comment(linker, "/export:mono_custom_attrs_from_index_checked")
#pragma comment(linker, "/export:mono_custom_attrs_from_method")
#pragma comment(linker, "/export:mono_custom_attrs_from_method_checked")
#pragma comment(linker, "/export:mono_custom_attrs_from_param")
#pragma comment(linker, "/export:mono_custom_attrs_from_param_checked")
#pragma comment(linker, "/export:mono_custom_attrs_from_property")
#pragma comment(linker, "/export:mono_custom_attrs_from_property_checked")
#pragma comment(linker, "/export:mono_custom_attrs_get_attr")
#pragma comment(linker, "/export:mono_custom_attrs_get_attr_checked")
#pragma comment(linker, "/export:mono_custom_attrs_has_attr")
#pragma comment(linker, "/export:mono_debug_add_aot_method")
#pragma comment(linker, "/export:mono_debug_add_delegate_trampoline")
#pragma comment(linker, "/export:mono_debug_add_method")
#pragma comment(linker, "/export:mono_debug_cleanup")
#pragma comment(linker, "/export:mono_debug_close_image")
#pragma comment(linker, "/export:mono_debug_close_method")
#pragma comment(linker, "/export:mono_debug_close_mono_symbol_file")
#pragma comment(linker, "/export:mono_debug_count")
#pragma comment(linker, "/export:mono_debug_domain_create")
#pragma comment(linker, "/export:mono_debug_domain_unload")
#pragma comment(linker, "/export:mono_debug_enabled")
#pragma comment(linker, "/export:mono_debug_find_method")
#pragma comment(linker, "/export:mono_debug_free_locals")
#pragma comment(linker, "/export:mono_debug_free_method")
#pragma comment(linker, "/export:mono_debug_free_method_async_debug_info")
#pragma comment(linker, "/export:mono_debug_free_method_jit_info")
#pragma comment(linker, "/export:mono_debug_free_source_location")
#pragma comment(linker, "/export:mono_debug_get_handle")
#pragma comment(linker, "/export:mono_debug_get_seq_points")
#pragma comment(linker, "/export:mono_debug_il_offset_from_address")
#pragma comment(linker, "/export:mono_debug_image_has_debug_info")
#pragma comment(linker, "/export:mono_debug_init")
#pragma comment(linker, "/export:mono_debug_init_method")
#pragma comment(linker, "/export:mono_debug_lookup_locals")
#pragma comment(linker, "/export:mono_debug_lookup_method")
#pragma comment(linker, "/export:mono_debug_lookup_method_addresses")
#pragma comment(linker, "/export:mono_debug_lookup_method_async_debug_info")
#pragma comment(linker, "/export:mono_debug_lookup_source_location")
#pragma comment(linker, "/export:mono_debug_lookup_source_location_by_il")
#pragma comment(linker, "/export:mono_debug_method_lookup_location")
#pragma comment(linker, "/export:mono_debug_open_block")
#pragma comment(linker, "/export:mono_debug_open_method")
#pragma comment(linker, "/export:mono_debug_open_mono_symbols")
#pragma comment(linker, "/export:mono_debug_personality")
#pragma comment(linker, "/export:mono_debug_print_stack_frame")
#pragma comment(linker, "/export:mono_debug_print_vars")
#pragma comment(linker, "/export:mono_debug_record_line_number")
#pragma comment(linker, "/export:mono_debug_remove_method")
#pragma comment(linker, "/export:mono_debug_serialize_debug_info")
#pragma comment(linker, "/export:mono_debug_symfile_free_location")
#pragma comment(linker, "/export:mono_debug_symfile_get_seq_points")
#pragma comment(linker, "/export:mono_debug_symfile_is_loaded")
#pragma comment(linker, "/export:mono_debug_symfile_lookup_locals")
#pragma comment(linker, "/export:mono_debug_symfile_lookup_location")
#pragma comment(linker, "/export:mono_debug_symfile_lookup_method")
#pragma comment(linker, "/export:mono_domain_alloc")
#pragma comment(linker, "/export:mono_domain_alloc0")
#pragma comment(linker, "/export:mono_domain_alloc0_lock_free")
#pragma comment(linker, "/export:mono_domain_assembly_open")
#pragma comment(linker, "/export:mono_domain_assembly_open_internal")
#pragma comment(linker, "/export:mono_domain_assembly_postload_search")
#pragma comment(linker, "/export:mono_domain_code_commit")
#pragma comment(linker, "/export:mono_domain_code_foreach")
#pragma comment(linker, "/export:mono_domain_code_reserve")
#pragma comment(linker, "/export:mono_domain_code_reserve_align")
#pragma comment(linker, "/export:mono_domain_create")
#pragma comment(linker, "/export:mono_domain_create_appdomain")
#pragma comment(linker, "/export:mono_domain_finalize")
#pragma comment(linker, "/export:mono_domain_foreach")
#pragma comment(linker, "/export:mono_domain_free")
#pragma comment(linker, "/export:mono_domain_from_appdomain")
#pragma comment(linker, "/export:mono_domain_get")
#pragma comment(linker, "/export:mono_domain_get_assemblies")
#pragma comment(linker, "/export:mono_domain_get_by_id")
#pragma comment(linker, "/export:mono_domain_get_friendly_name")
#pragma comment(linker, "/export:mono_domain_get_id")
#pragma comment(linker, "/export:mono_domain_has_type_resolve")
#pragma comment(linker, "/export:mono_domain_is_unloading")
#pragma comment(linker, "/export:mono_domain_lock")
#pragma comment(linker, "/export:mono_domain_owns_vtable_slot")
#pragma comment(linker, "/export:mono_domain_parse_assembly_bindings")
#pragma comment(linker, "/export:mono_domain_set")
#pragma comment(linker, "/export:mono_domain_set_config")
#pragma comment(linker, "/export:mono_domain_set_config_checked")
#pragma comment(linker, "/export:mono_domain_set_internal")
#pragma comment(linker, "/export:mono_domain_set_internal_with_options")
#pragma comment(linker, "/export:mono_domain_set_options_from_config")
#pragma comment(linker, "/export:mono_domain_try_type_resolve")
#pragma comment(linker, "/export:mono_domain_try_type_resolve_name")
#pragma comment(linker, "/export:mono_domain_try_type_resolve_typebuilder")
#pragma comment(linker, "/export:mono_domain_try_unload")
#pragma comment(linker, "/export:mono_domain_unload")
#pragma comment(linker, "/export:mono_domain_unlock")
#pragma comment(linker, "/export:mono_domain_unset")
#pragma comment(linker, "/export:mono_exception_from_name")
#pragma comment(linker, "/export:mono_exception_from_name_domain")
#pragma comment(linker, "/export:mono_exception_from_name_msg")
#pragma comment(linker, "/export:mono_exception_from_name_two_strings")
#pragma comment(linker, "/export:mono_exception_from_name_two_strings_checked")
#pragma comment(linker, "/export:mono_exception_from_token")
#pragma comment(linker, "/export:mono_exception_from_token_two_strings")
#pragma comment(linker, "/export:mono_exception_from_token_two_strings_checked")
#pragma comment(linker, "/export:mono_exception_get_managed_backtrace")
#pragma comment(linker, "/export:mono_exception_handle_get_native_backtrace")
#pragma comment(linker, "/export:mono_exception_new_argument")
#pragma comment(linker, "/export:mono_exception_new_argument_null")
#pragma comment(linker, "/export:mono_exception_new_by_name_msg")
#pragma comment(linker, "/export:mono_exception_new_invalid_operation")
#pragma comment(linker, "/export:mono_exception_new_serialization")
#pragma comment(linker, "/export:mono_exception_new_thread_abort")
#pragma comment(linker, "/export:mono_exception_new_thread_interrupted")
#pragma comment(linker, "/export:mono_exception_walk_trace")
#pragma comment(linker, "/export:mono_field_from_token")
#pragma comment(linker, "/export:mono_field_from_token_checked")
#pragma comment(linker, "/export:mono_field_full_name")
#pragma comment(linker, "/export:mono_field_get_data")
#pragma comment(linker, "/export:mono_field_get_flags")
#pragma comment(linker, "/export:mono_field_get_name")
#pragma comment(linker, "/export:mono_field_get_object")
#pragma comment(linker, "/export:mono_field_get_object_checked")
#pragma comment(linker, "/export:mono_field_get_object_handle")
#pragma comment(linker, "/export:mono_field_get_offset")
#pragma comment(linker, "/export:mono_field_get_parent")
#pragma comment(linker, "/export:mono_field_get_type")
#pragma comment(linker, "/export:mono_field_get_type_checked")
#pragma comment(linker, "/export:mono_field_get_value")
#pragma comment(linker, "/export:mono_field_get_value_internal")
#pragma comment(linker, "/export:mono_field_get_value_object")
#pragma comment(linker, "/export:mono_field_get_value_object_checked")
#pragma comment(linker, "/export:mono_field_resolve_type")
#pragma comment(linker, "/export:mono_field_set_value")
#pragma comment(linker, "/export:mono_field_static_get_value")
#pragma comment(linker, "/export:mono_field_static_get_value_checked")
#pragma comment(linker, "/export:mono_field_static_get_value_for_thread")
#pragma comment(linker, "/export:mono_field_static_set_value")
#pragma comment(linker, "/export:mono_free")
#pragma comment(linker, "/export:mono_free_address_info")
#pragma comment(linker, "/export:mono_free_altstack")
#pragma comment(linker, "/export:mono_free_bstr")
#pragma comment(linker, "/export:mono_free_loop_info")
#pragma comment(linker, "/export:mono_free_lparray")
#pragma comment(linker, "/export:mono_free_method")
#pragma comment(linker, "/export:mono_free_verify_list")
#pragma comment(linker, "/export:mono_gc_add_memory_pressure")
#pragma comment(linker, "/export:mono_gc_alloc_array")
#pragma comment(linker, "/export:mono_gc_alloc_fixed")
#pragma comment(linker, "/export:mono_gc_alloc_fixed_no_descriptor")
#pragma comment(linker, "/export:mono_gc_alloc_handle_array")
#pragma comment(linker, "/export:mono_gc_alloc_handle_mature")
#pragma comment(linker, "/export:mono_gc_alloc_handle_obj")
#pragma comment(linker, "/export:mono_gc_alloc_handle_pinned_obj")
#pragma comment(linker, "/export:mono_gc_alloc_handle_string")
#pragma comment(linker, "/export:mono_gc_alloc_handle_vector")
#pragma comment(linker, "/export:mono_gc_alloc_mature")
#pragma comment(linker, "/export:mono_gc_alloc_obj")
#pragma comment(linker, "/export:mono_gc_alloc_pinned_obj")
#pragma comment(linker, "/export:mono_gc_alloc_string")
#pragma comment(linker, "/export:mono_gc_alloc_vector")
#pragma comment(linker, "/export:mono_gc_base_cleanup")
#pragma comment(linker, "/export:mono_gc_base_init")
#pragma comment(linker, "/export:mono_gc_bzero_aligned")
#pragma comment(linker, "/export:mono_gc_bzero_atomic")
#pragma comment(linker, "/export:mono_gc_card_table_nursery_check")
#pragma comment(linker, "/export:mono_gc_cleanup")
#pragma comment(linker, "/export:mono_gc_clear_assembly")
#pragma comment(linker, "/export:mono_gc_clear_domain")
#pragma comment(linker, "/export:mono_gc_collect")
#pragma comment(linker, "/export:mono_gc_collection_count")
#pragma comment(linker, "/export:mono_gc_conservatively_scan_area")
#pragma comment(linker, "/export:mono_gc_debug_set")
#pragma comment(linker, "/export:mono_gc_deregister_root")
#pragma comment(linker, "/export:mono_gc_dllmain")
#pragma comment(linker, "/export:mono_gc_ephemeron_array_add")
#pragma comment(linker, "/export:mono_gc_finalize_assembly")
#pragma comment(linker, "/export:mono_gc_finalize_domain")
#pragma comment(linker, "/export:mono_gc_finalize_notify")
#pragma comment(linker, "/export:mono_gc_free_fixed")
#pragma comment(linker, "/export:mono_gc_get_aligned_size_for_allocator")
#pragma comment(linker, "/export:mono_gc_get_bitmap_for_descr")
#pragma comment(linker, "/export:mono_gc_get_card_table")
#pragma comment(linker, "/export:mono_gc_get_description")
#pragma comment(linker, "/export:mono_gc_get_gc_callbacks")
#pragma comment(linker, "/export:mono_gc_get_gc_name")
#pragma comment(linker, "/export:mono_gc_get_generation")
#pragma comment(linker, "/export:mono_gc_get_heap_size")
#pragma comment(linker, "/export:mono_gc_get_logfile")
#pragma comment(linker, "/export:mono_gc_get_los_limit")
#pragma comment(linker, "/export:mono_gc_get_managed_allocator")
#pragma comment(linker, "/export:mono_gc_get_managed_allocator_by_type")
#pragma comment(linker, "/export:mono_gc_get_managed_allocator_types")
#pragma comment(linker, "/export:mono_gc_get_managed_array_allocator")
#pragma comment(linker, "/export:mono_gc_get_nursery")
#pragma comment(linker, "/export:mono_gc_get_range_copy_func")
#pragma comment(linker, "/export:mono_gc_get_restart_signal")
#pragma comment(linker, "/export:mono_gc_get_specific_write_barrier")
#pragma comment(linker, "/export:mono_gc_get_suspend_signal")
#pragma comment(linker, "/export:mono_gc_get_target_card_table")
#pragma comment(linker, "/export:mono_gc_get_used_size")
#pragma comment(linker, "/export:mono_gc_get_vtable")
#pragma comment(linker, "/export:mono_gc_get_vtable_bits")
#pragma comment(linker, "/export:mono_gc_get_write_barrier")
#pragma comment(linker, "/export:mono_gc_init")
#pragma comment(linker, "/export:mono_gc_invoke_finalizers")
#pragma comment(linker, "/export:mono_gc_invoke_with_gc_lock")
#pragma comment(linker, "/export:mono_gc_is_critical_method")
#pragma comment(linker, "/export:mono_gc_is_disabled")
#pragma comment(linker, "/export:mono_gc_is_finalizer_internal_thread")
#pragma comment(linker, "/export:mono_gc_is_finalizer_thread")
#pragma comment(linker, "/export:mono_gc_is_gc_thread")
#pragma comment(linker, "/export:mono_gc_is_moving")
#pragma comment(linker, "/export:mono_gc_is_null")
#pragma comment(linker, "/export:mono_gc_make_descr_for_array")
#pragma comment(linker, "/export:mono_gc_make_descr_for_object")
#pragma comment(linker, "/export:mono_gc_make_descr_for_string")
#pragma comment(linker, "/export:mono_gc_make_descr_from_bitmap")
#pragma comment(linker, "/export:mono_gc_make_root_descr_all_refs")
#pragma comment(linker, "/export:mono_gc_make_root_descr_user")
#pragma comment(linker, "/export:mono_gc_make_vector_descr")
#pragma comment(linker, "/export:mono_gc_max_generation")
#pragma comment(linker, "/export:mono_gc_memmove_aligned")
#pragma comment(linker, "/export:mono_gc_memmove_atomic")
#pragma comment(linker, "/export:mono_gc_params_set")
#pragma comment(linker, "/export:mono_gc_parse_environment_string_extract_number")
#pragma comment(linker, "/export:mono_gc_pending_finalizers")
#pragma comment(linker, "/export:mono_gc_precise_stack_mark_enabled")
#pragma comment(linker, "/export:mono_gc_reference_queue_add")
#pragma comment(linker, "/export:mono_gc_reference_queue_foreach_remove")
#pragma comment(linker, "/export:mono_gc_reference_queue_foreach_remove2")
#pragma comment(linker, "/export:mono_gc_reference_queue_free")
#pragma comment(linker, "/export:mono_gc_reference_queue_new")
#pragma comment(linker, "/export:mono_gc_register_altstack")
#pragma comment(linker, "/export:mono_gc_register_bridge_callbacks")
#pragma comment(linker, "/export:mono_gc_register_finalizer_callbacks")
#pragma comment(linker, "/export:mono_gc_register_for_finalization")
#pragma comment(linker, "/export:mono_gc_register_obj_with_weak_fields")
#pragma comment(linker, "/export:mono_gc_register_object_with_weak_fields")
#pragma comment(linker, "/export:mono_gc_register_root")
#pragma comment(linker, "/export:mono_gc_register_root_wbarrier")
#pragma comment(linker, "/export:mono_gc_run_finalize")
#pragma comment(linker, "/export:mono_gc_scan_for_specific_ref")
#pragma comment(linker, "/export:mono_gc_scan_object")
#pragma comment(linker, "/export:mono_gc_set_desktop_mode")
#pragma comment(linker, "/export:mono_gc_set_gc_callbacks")
#pragma comment(linker, "/export:mono_gc_set_stack_end")
#pragma comment(linker, "/export:mono_gc_set_string_length")
#pragma comment(linker, "/export:mono_gc_skip_thread_changed")
#pragma comment(linker, "/export:mono_gc_skip_thread_changing")
#pragma comment(linker, "/export:mono_gc_stats")
#pragma comment(linker, "/export:mono_gc_suspend_finalizers")
#pragma comment(linker, "/export:mono_gc_thread_attach")
#pragma comment(linker, "/export:mono_gc_thread_detach_with_lock")
#pragma comment(linker, "/export:mono_gc_thread_in_critical_region")
#pragma comment(linker, "/export:mono_gc_toggleref_add")
#pragma comment(linker, "/export:mono_gc_toggleref_register_callback")
#pragma comment(linker, "/export:mono_gc_user_markers_supported")
#pragma comment(linker, "/export:mono_gc_wait_for_bridge_processing")
#pragma comment(linker, "/export:mono_gc_walk_heap")
#pragma comment(linker, "/export:mono_gc_wbarrier_arrayref_copy")
#pragma comment(linker, "/export:mono_gc_wbarrier_generic_nostore")
#pragma comment(linker, "/export:mono_gc_wbarrier_generic_store")
#pragma comment(linker, "/export:mono_gc_wbarrier_generic_store_atomic")
#pragma comment(linker, "/export:mono_gc_wbarrier_object_copy")
#pragma comment(linker, "/export:mono_gc_wbarrier_object_copy_handle")
#pragma comment(linker, "/export:mono_gc_wbarrier_range_copy")
#pragma comment(linker, "/export:mono_gc_wbarrier_set_arrayref")
#pragma comment(linker, "/export:mono_gc_wbarrier_set_field")
#pragma comment(linker, "/export:mono_gc_wbarrier_value_copy")
#pragma comment(linker, "/export:mono_gchandle_free")
#pragma comment(linker, "/export:mono_gchandle_free_domain")
#pragma comment(linker, "/export:mono_gchandle_from_handle")
#pragma comment(linker, "/export:mono_gchandle_get_target")
#pragma comment(linker, "/export:mono_gchandle_get_target_handle")
#pragma comment(linker, "/export:mono_gchandle_is_in_domain")
#pragma comment(linker, "/export:mono_gchandle_new")
#pragma comment(linker, "/export:mono_gchandle_new_weakref")
#pragma comment(linker, "/export:mono_gchandle_set_target")
#pragma comment(linker, "/export:mono_gchandle_set_target_handle")
#pragma comment(linker, "/export:mono_get_addr_from_ftnptr")
#pragma comment(linker, "/export:mono_get_address_info")
#pragma comment(linker, "/export:mono_get_anonymous_container_for_image")
#pragma comment(linker, "/export:mono_get_aot_cache_config")
#pragma comment(linker, "/export:mono_get_array_class")
#pragma comment(linker, "/export:mono_get_assembly_object")
#pragma comment(linker, "/export:mono_get_boolean_class")
#pragma comment(linker, "/export:mono_get_byte_class")
#pragma comment(linker, "/export:mono_get_cached_unwind_info")
#pragma comment(linker, "/export:mono_get_call_filter")
#pragma comment(linker, "/export:mono_get_char_class")
#pragma comment(linker, "/export:mono_get_config_dir")
#pragma comment(linker, "/export:mono_get_constant_value_from_blob")
#pragma comment(linker, "/export:mono_get_context_capture_method")
#pragma comment(linker, "/export:mono_get_corlib")
#pragma comment(linker, "/export:mono_get_dbnull_object")
#pragma comment(linker, "/export:mono_get_delegate_begin_invoke")
#pragma comment(linker, "/export:mono_get_delegate_begin_invoke_checked")
#pragma comment(linker, "/export:mono_get_delegate_end_invoke")
#pragma comment(linker, "/export:mono_get_delegate_end_invoke_checked")
#pragma comment(linker, "/export:mono_get_delegate_invoke")
#pragma comment(linker, "/export:mono_get_delegate_invoke_checked")
#pragma comment(linker, "/export:mono_get_delegate_virtual_invoke_impl")
#pragma comment(linker, "/export:mono_get_delegate_virtual_invoke_impl_name")
#pragma comment(linker, "/export:mono_get_double_class")
#pragma comment(linker, "/export:mono_get_eh_callbacks")
#pragma comment(linker, "/export:mono_get_enum_class")
#pragma comment(linker, "/export:mono_get_exception_appdomain_unloaded")
#pragma comment(linker, "/export:mono_get_exception_argument")
#pragma comment(linker, "/export:mono_get_exception_argument_null")
#pragma comment(linker, "/export:mono_get_exception_argument_out_of_range")
#pragma comment(linker, "/export:mono_get_exception_arithmetic")
#pragma comment(linker, "/export:mono_get_exception_array_type_mismatch")
#pragma comment(linker, "/export:mono_get_exception_bad_image_format")
#pragma comment(linker, "/export:mono_get_exception_bad_image_format2")
#pragma comment(linker, "/export:mono_get_exception_cannot_unload_appdomain")
#pragma comment(linker, "/export:mono_get_exception_class")
#pragma comment(linker, "/export:mono_get_exception_divide_by_zero")
#pragma comment(linker, "/export:mono_get_exception_execution_engine")
#pragma comment(linker, "/export:mono_get_exception_field_access")
#pragma comment(linker, "/export:mono_get_exception_field_access_msg")
#pragma comment(linker, "/export:mono_get_exception_file_not_found")
#pragma comment(linker, "/export:mono_get_exception_file_not_found2")
#pragma comment(linker, "/export:mono_get_exception_index_out_of_range")
#pragma comment(linker, "/export:mono_get_exception_invalid_cast")
#pragma comment(linker, "/export:mono_get_exception_invalid_operation")
#pragma comment(linker, "/export:mono_get_exception_io")
#pragma comment(linker, "/export:mono_get_exception_method_access")
#pragma comment(linker, "/export:mono_get_exception_method_access_msg")
#pragma comment(linker, "/export:mono_get_exception_missing_field")
#pragma comment(linker, "/export:mono_get_exception_missing_method")
#pragma comment(linker, "/export:mono_get_exception_not_implemented")
#pragma comment(linker, "/export:mono_get_exception_not_supported")
#pragma comment(linker, "/export:mono_get_exception_null_reference")
#pragma comment(linker, "/export:mono_get_exception_out_of_memory")
#pragma comment(linker, "/export:mono_get_exception_out_of_memory_handle")
#pragma comment(linker, "/export:mono_get_exception_overflow")
#pragma comment(linker, "/export:mono_get_exception_reflection_type_load")
#pragma comment(linker, "/export:mono_get_exception_reflection_type_load_checked")
#pragma comment(linker, "/export:mono_get_exception_runtime_wrapped")
#pragma comment(linker, "/export:mono_get_exception_runtime_wrapped_handle")
#pragma comment(linker, "/export:mono_get_exception_security")
#pragma comment(linker, "/export:mono_get_exception_serialization")
#pragma comment(linker, "/export:mono_get_exception_stack_overflow")
#pragma comment(linker, "/export:mono_get_exception_synchronization_lock")
#pragma comment(linker, "/export:mono_get_exception_thread_abort")
#pragma comment(linker, "/export:mono_get_exception_thread_interrupted")
#pragma comment(linker, "/export:mono_get_exception_thread_state")
#pragma comment(linker, "/export:mono_get_exception_type_initialization")
#pragma comment(linker, "/export:mono_get_exception_type_initialization_handle")
#pragma comment(linker, "/export:mono_get_exception_type_load")
#pragma comment(linker, "/export:mono_get_generic_trampoline_name")
#pragma comment(linker, "/export:mono_get_generic_trampoline_simple_name")
#pragma comment(linker, "/export:mono_get_hazardous_pointer")
#pragma comment(linker, "/export:mono_get_image_for_generic_param")
#pragma comment(linker, "/export:mono_get_inflated_method")
#pragma comment(linker, "/export:mono_get_int16_class")
#pragma comment(linker, "/export:mono_get_int32_class")
#pragma comment(linker, "/export:mono_get_int64_class")
#pragma comment(linker, "/export:mono_get_intptr_class")
#pragma comment(linker, "/export:mono_get_jit_icall_info")
#pragma comment(linker, "/export:mono_get_lmf")
#pragma comment(linker, "/export:mono_get_local_interfaces")
#pragma comment(linker, "/export:mono_get_machine_config")
#pragma comment(linker, "/export:mono_get_method")
#pragma comment(linker, "/export:mono_get_method_checked")
#pragma comment(linker, "/export:mono_get_method_constrained")
#pragma comment(linker, "/export:mono_get_method_constrained_checked")
#pragma comment(linker, "/export:mono_get_method_constrained_with_method")
#pragma comment(linker, "/export:mono_get_method_from_ip")
#pragma comment(linker, "/export:mono_get_method_full")
#pragma comment(linker, "/export:mono_get_method_object")
#pragma comment(linker, "/export:mono_get_module_file_name")
#pragma comment(linker, "/export:mono_get_native_calli_wrapper")
#pragma comment(linker, "/export:mono_get_object_class")
#pragma comment(linker, "/export:mono_get_object_from_blob")
#pragma comment(linker, "/export:mono_get_optimizations_for_method")
#pragma comment(linker, "/export:mono_get_restore_context")
#pragma comment(linker, "/export:mono_get_rethrow_exception")
#pragma comment(linker, "/export:mono_get_rgctx_fetch_trampoline_name")
#pragma comment(linker, "/export:mono_get_root_domain")
#pragma comment(linker, "/export:mono_get_runtime_build_info")
#pragma comment(linker, "/export:mono_get_runtime_callbacks")
#pragma comment(linker, "/export:mono_get_runtime_info")
#pragma comment(linker, "/export:mono_get_sbyte_class")
#pragma comment(linker, "/export:mono_get_seq_points")
#pragma comment(linker, "/export:mono_get_shared_generic_inst")
#pragma comment(linker, "/export:mono_get_single_class")
#pragma comment(linker, "/export:mono_get_special_static_data")
#pragma comment(linker, "/export:mono_get_special_static_data_for_thread")
#pragma comment(linker, "/export:mono_get_string_class")
#pragma comment(linker, "/export:mono_get_thread_class")
#pragma comment(linker, "/export:mono_get_throw_corlib_exception")
#pragma comment(linker, "/export:mono_get_throw_exception")
#pragma comment(linker, "/export:mono_get_throw_exception_addr")
#pragma comment(linker, "/export:mono_get_trampoline_code")
#pragma comment(linker, "/export:mono_get_trampoline_func")
#pragma comment(linker, "/export:mono_get_uint16_class")
#pragma comment(linker, "/export:mono_get_uint32_class")
#pragma comment(linker, "/export:mono_get_uint64_class")
#pragma comment(linker, "/export:mono_get_uintptr_class")
#pragma comment(linker, "/export:mono_get_void_class")
#pragma comment(linker, "/export:mono_image_add_to_name_cache")
#pragma comment(linker, "/export:mono_image_addref")
#pragma comment(linker, "/export:mono_image_alloc")
#pragma comment(linker, "/export:mono_image_alloc0")
#pragma comment(linker, "/export:mono_image_append_class_to_reflection_info_set")
#pragma comment(linker, "/export:mono_image_build_metadata")
#pragma comment(linker, "/export:mono_image_check_for_module_cctor")
#pragma comment(linker, "/export:mono_image_close")
#pragma comment(linker, "/export:mono_image_close_except_pools")
#pragma comment(linker, "/export:mono_image_close_finish")
#pragma comment(linker, "/export:mono_image_create_pefile")
#pragma comment(linker, "/export:mono_image_create_token")
#pragma comment(linker, "/export:mono_image_ensure_section")
#pragma comment(linker, "/export:mono_image_ensure_section_idx")
#pragma comment(linker, "/export:mono_image_fixup_vtable")
#pragma comment(linker, "/export:mono_image_g_malloc0")
#pragma comment(linker, "/export:mono_image_get_assembly")
#pragma comment(linker, "/export:mono_image_get_entry_point")
#pragma comment(linker, "/export:mono_image_get_filename")
#pragma comment(linker, "/export:mono_image_get_guid")
#pragma comment(linker, "/export:mono_image_get_methodref_token")
#pragma comment(linker, "/export:mono_image_get_name")
#pragma comment(linker, "/export:mono_image_get_public_key")
#pragma comment(linker, "/export:mono_image_get_resource")
#pragma comment(linker, "/export:mono_image_get_strong_name")
#pragma comment(linker, "/export:mono_image_get_table_info")
#pragma comment(linker, "/export:mono_image_get_table_rows")
#pragma comment(linker, "/export:mono_image_has_authenticode_entry")
#pragma comment(linker, "/export:mono_image_init")
#pragma comment(linker, "/export:mono_image_init_name_cache")
#pragma comment(linker, "/export:mono_image_insert_string")
#pragma comment(linker, "/export:mono_image_is_dynamic")
#pragma comment(linker, "/export:mono_image_load_cli_data")
#pragma comment(linker, "/export:mono_image_load_cli_header")
#pragma comment(linker, "/export:mono_image_load_file_for_image")
#pragma comment(linker, "/export:mono_image_load_file_for_image_checked")
#pragma comment(linker, "/export:mono_image_load_metadata")
#pragma comment(linker, "/export:mono_image_load_module")
#pragma comment(linker, "/export:mono_image_load_module_checked")
#pragma comment(linker, "/export:mono_image_load_names")
#pragma comment(linker, "/export:mono_image_load_pe_data")
#pragma comment(linker, "/export:mono_image_loaded")
#pragma comment(linker, "/export:mono_image_loaded_by_guid")
#pragma comment(linker, "/export:mono_image_loaded_by_guid_full")
#pragma comment(linker, "/export:mono_image_loaded_full")
#pragma comment(linker, "/export:mono_image_loaded_internal")
#pragma comment(linker, "/export:mono_image_lock")
#pragma comment(linker, "/export:mono_image_lookup_resource")
#pragma comment(linker, "/export:mono_image_open")
#pragma comment(linker, "/export:mono_image_open_a_lot")
#pragma comment(linker, "/export:mono_image_open_from_data")
#pragma comment(linker, "/export:mono_image_open_from_data_full")
#pragma comment(linker, "/export:mono_image_open_from_data_internal")
#pragma comment(linker, "/export:mono_image_open_from_data_with_name")
#pragma comment(linker, "/export:mono_image_open_from_module_handle")
#pragma comment(linker, "/export:mono_image_open_full")
#pragma comment(linker, "/export:mono_image_open_metadata_only")
#pragma comment(linker, "/export:mono_image_open_raw")
#pragma comment(linker, "/export:mono_image_property_insert")
#pragma comment(linker, "/export:mono_image_property_lookup")
#pragma comment(linker, "/export:mono_image_property_remove")
#pragma comment(linker, "/export:mono_image_rva_map")
#pragma comment(linker, "/export:mono_image_set_alloc")
#pragma comment(linker, "/export:mono_image_set_alloc0")
#pragma comment(linker, "/export:mono_image_set_description")
#pragma comment(linker, "/export:mono_image_set_lock")
#pragma comment(linker, "/export:mono_image_set_strdup")
#pragma comment(linker, "/export:mono_image_set_unlock")
#pragma comment(linker, "/export:mono_image_strdup")
#pragma comment(linker, "/export:mono_image_strdup_printf")
#pragma comment(linker, "/export:mono_image_strdup_vprintf")
#pragma comment(linker, "/export:mono_image_strerror")
#pragma comment(linker, "/export:mono_image_strong_name_position")
#pragma comment(linker, "/export:mono_image_unlock")
#pragma comment(linker, "/export:mono_metadata_blob_heap")
#pragma comment(linker, "/export:mono_metadata_blob_heap_checked")
#pragma comment(linker, "/export:mono_metadata_clean_for_image")
#pragma comment(linker, "/export:mono_metadata_cleanup")
#pragma comment(linker, "/export:mono_metadata_compute_size")
#pragma comment(linker, "/export:mono_metadata_compute_table_bases")
#pragma comment(linker, "/export:mono_metadata_create_anon_gparam")
#pragma comment(linker, "/export:mono_metadata_cross_helpers_run")
#pragma comment(linker, "/export:mono_metadata_custom_attrs_from_index")
#pragma comment(linker, "/export:mono_metadata_declsec_from_index")
#pragma comment(linker, "/export:mono_metadata_decode_blob_size")
#pragma comment(linker, "/export:mono_metadata_decode_row")
#pragma comment(linker, "/export:mono_metadata_decode_row_checked")
#pragma comment(linker, "/export:mono_metadata_decode_row_col")
#pragma comment(linker, "/export:mono_metadata_decode_signed_value")
#pragma comment(linker, "/export:mono_metadata_decode_table_row")
#pragma comment(linker, "/export:mono_metadata_decode_table_row_col")
#pragma comment(linker, "/export:mono_metadata_decode_value")
#pragma comment(linker, "/export:mono_metadata_encode_value")
#pragma comment(linker, "/export:mono_metadata_events_from_typedef")
#pragma comment(linker, "/export:mono_metadata_field_info")
#pragma comment(linker, "/export:mono_metadata_field_info_with_mempool")
#pragma comment(linker, "/export:mono_metadata_free_array")
#pragma comment(linker, "/export:mono_metadata_free_inflated_signature")
#pragma comment(linker, "/export:mono_metadata_free_marshal_spec")
#pragma comment(linker, "/export:mono_metadata_free_method_signature")
#pragma comment(linker, "/export:mono_metadata_free_mh")
#pragma comment(linker, "/export:mono_metadata_free_type")
#pragma comment(linker, "/export:mono_metadata_generic_class_is_valuetype")
#pragma comment(linker, "/export:mono_metadata_generic_context_equal")
#pragma comment(linker, "/export:mono_metadata_generic_context_hash")
#pragma comment(linker, "/export:mono_metadata_generic_inst_equal")
#pragma comment(linker, "/export:mono_metadata_generic_inst_hash")
#pragma comment(linker, "/export:mono_metadata_generic_param_equal")
#pragma comment(linker, "/export:mono_metadata_generic_param_hash")
#pragma comment(linker, "/export:mono_metadata_get_canonical_generic_inst")
#pragma comment(linker, "/export:mono_metadata_get_constant_index")
#pragma comment(linker, "/export:mono_metadata_get_corresponding_event_from_generic_type_definition")
#pragma comment(linker, "/export:mono_metadata_get_corresponding_field_from_generic_type_definition")
#pragma comment(linker, "/export:mono_metadata_get_corresponding_property_from_generic_type_definition")
#pragma comment(linker, "/export:mono_metadata_get_generic_inst")
#pragma comment(linker, "/export:mono_metadata_get_generic_param_row")
#pragma comment(linker, "/export:mono_metadata_get_image_set_for_class")
#pragma comment(linker, "/export:mono_metadata_get_image_set_for_method")
#pragma comment(linker, "/export:mono_metadata_get_inflated_signature")
#pragma comment(linker, "/export:mono_metadata_get_marshal_info")
#pragma comment(linker, "/export:mono_metadata_get_param_attrs")
#pragma comment(linker, "/export:mono_metadata_get_shared_type")
#pragma comment(linker, "/export:mono_metadata_guid_heap")
#pragma comment(linker, "/export:mono_metadata_has_generic_params")
#pragma comment(linker, "/export:mono_metadata_implmap_from_method")
#pragma comment(linker, "/export:mono_metadata_inflate_generic_inst")
#pragma comment(linker, "/export:mono_metadata_init")
#pragma comment(linker, "/export:mono_metadata_interfaces_from_typedef")
#pragma comment(linker, "/export:mono_metadata_interfaces_from_typedef_full")
#pragma comment(linker, "/export:mono_metadata_load_generic_param_constraints_checked")
#pragma comment(linker, "/export:mono_metadata_load_generic_params")
#pragma comment(linker, "/export:mono_metadata_localscope_from_methoddef")
#pragma comment(linker, "/export:mono_metadata_locate")
#pragma comment(linker, "/export:mono_metadata_locate_token")
#pragma comment(linker, "/export:mono_metadata_lookup_generic_class")
#pragma comment(linker, "/export:mono_metadata_method_has_param_attrs")
#pragma comment(linker, "/export:mono_metadata_methods_from_event")
#pragma comment(linker, "/export:mono_metadata_methods_from_property")
#pragma comment(linker, "/export:mono_metadata_nested_in_typedef")
#pragma comment(linker, "/export:mono_metadata_nesting_typedef")
#pragma comment(linker, "/export:mono_metadata_packing_from_typedef")
#pragma comment(linker, "/export:mono_metadata_parse_array")
#pragma comment(linker, "/export:mono_metadata_parse_custom_mod")
#pragma comment(linker, "/export:mono_metadata_parse_field_type")
#pragma comment(linker, "/export:mono_metadata_parse_generic_inst")
#pragma comment(linker, "/export:mono_metadata_parse_marshal_spec")
#pragma comment(linker, "/export:mono_metadata_parse_marshal_spec_full")
#pragma comment(linker, "/export:mono_metadata_parse_method_signature")
#pragma comment(linker, "/export:mono_metadata_parse_method_signature_full")
#pragma comment(linker, "/export:mono_metadata_parse_mh")
#pragma comment(linker, "/export:mono_metadata_parse_mh_full")
#pragma comment(linker, "/export:mono_metadata_parse_param")
#pragma comment(linker, "/export:mono_metadata_parse_signature")
#pragma comment(linker, "/export:mono_metadata_parse_signature_checked")
#pragma comment(linker, "/export:mono_metadata_parse_type")
#pragma comment(linker, "/export:mono_metadata_parse_type_checked")
#pragma comment(linker, "/export:mono_metadata_parse_typedef_or_ref")
#pragma comment(linker, "/export:mono_metadata_properties_from_typedef")
#pragma comment(linker, "/export:mono_metadata_read_constant_value")
#pragma comment(linker, "/export:mono_metadata_signature_alloc")
#pragma comment(linker, "/export:mono_metadata_signature_deep_dup")
#pragma comment(linker, "/export:mono_metadata_signature_dup")
#pragma comment(linker, "/export:mono_metadata_signature_dup_add_this")
#pragma comment(linker, "/export:mono_metadata_signature_dup_full")
#pragma comment(linker, "/export:mono_metadata_signature_dup_mempool")
#pragma comment(linker, "/export:mono_metadata_signature_equal")
#pragma comment(linker, "/export:mono_metadata_signature_size")
#pragma comment(linker, "/export:mono_metadata_str_hash")
#pragma comment(linker, "/export:mono_metadata_string_heap")
#pragma comment(linker, "/export:mono_metadata_string_heap_checked")
#pragma comment(linker, "/export:mono_metadata_token_from_dor")
#pragma comment(linker, "/export:mono_metadata_translate_token_index")
#pragma comment(linker, "/export:mono_metadata_type_dup")
#pragma comment(linker, "/export:mono_metadata_type_dup_with_cmods")
#pragma comment(linker, "/export:mono_metadata_type_equal")
#pragma comment(linker, "/export:mono_metadata_type_equal_full")
#pragma comment(linker, "/export:mono_metadata_type_hash")
#pragma comment(linker, "/export:mono_metadata_typedef_from_field")
#pragma comment(linker, "/export:mono_metadata_typedef_from_method")
#pragma comment(linker, "/export:mono_metadata_user_string")
#pragma comment(linker, "/export:mono_method_add_generic_virtual_invocation")
#pragma comment(linker, "/export:mono_method_alloc_generic_virtual_trampoline")
#pragma comment(linker, "/export:mono_method_body_get_object")
#pragma comment(linker, "/export:mono_method_body_get_object_handle")
#pragma comment(linker, "/export:mono_method_builder_ilgen_init")
#pragma comment(linker, "/export:mono_method_call_message_new")
#pragma comment(linker, "/export:mono_method_can_access_field")
#pragma comment(linker, "/export:mono_method_can_access_field_full")
#pragma comment(linker, "/export:mono_method_can_access_method")
#pragma comment(linker, "/export:mono_method_can_access_method_full")
#pragma comment(linker, "/export:mono_method_check_context_used")
#pragma comment(linker, "/export:mono_method_clear_object")
#pragma comment(linker, "/export:mono_method_construct_object_context")
#pragma comment(linker, "/export:mono_method_desc_free")
#pragma comment(linker, "/export:mono_method_desc_from_method")
#pragma comment(linker, "/export:mono_method_desc_full_match")
#pragma comment(linker, "/export:mono_method_desc_is_full")
#pragma comment(linker, "/export:mono_method_desc_match")
#pragma comment(linker, "/export:mono_method_desc_new")
#pragma comment(linker, "/export:mono_method_desc_search_in_class")
#pragma comment(linker, "/export:mono_method_desc_search_in_image")
#pragma comment(linker, "/export:mono_method_fill_runtime_generic_context")
#pragma comment(linker, "/export:mono_method_from_method_def_or_ref")
#pragma comment(linker, "/export:mono_method_full_name")
#pragma comment(linker, "/export:mono_method_get_base_method")
#pragma comment(linker, "/export:mono_method_get_class")
#pragma comment(linker, "/export:mono_method_get_context")
#pragma comment(linker, "/export:mono_method_get_context_general")
#pragma comment(linker, "/export:mono_method_get_declaring_generic_method")
#pragma comment(linker, "/export:mono_method_get_flags")
#pragma comment(linker, "/export:mono_method_get_full_name")
#pragma comment(linker, "/export:mono_method_get_generic_container")
#pragma comment(linker, "/export:mono_method_get_header")
#pragma comment(linker, "/export:mono_method_get_header_checked")
#pragma comment(linker, "/export:mono_method_get_header_internal")
#pragma comment(linker, "/export:mono_method_get_header_summary")
#pragma comment(linker, "/export:mono_method_get_imt_slot")
#pragma comment(linker, "/export:mono_method_get_index")
#pragma comment(linker, "/export:mono_method_get_last_managed")
#pragma comment(linker, "/export:mono_method_get_marshal_info")
#pragma comment(linker, "/export:mono_method_get_name")
#pragma comment(linker, "/export:mono_method_get_name_full")
#pragma comment(linker, "/export:mono_method_get_object")
#pragma comment(linker, "/export:mono_method_get_object_checked")
#pragma comment(linker, "/export:mono_method_get_object_handle")
#pragma comment(linker, "/export:mono_method_get_param_names")
#pragma comment(linker, "/export:mono_method_get_param_token")
#pragma comment(linker, "/export:mono_method_get_reflection_name")
#pragma comment(linker, "/export:mono_method_get_signature")
#pragma comment(linker, "/export:mono_method_get_signature_checked")
#pragma comment(linker, "/export:mono_method_get_signature_full")
#pragma comment(linker, "/export:mono_method_get_token")
#pragma comment(linker, "/export:mono_method_get_unmanaged_thunk")
#pragma comment(linker, "/export:mono_method_get_vtable_index")
#pragma comment(linker, "/export:mono_method_get_vtable_slot")
#pragma comment(linker, "/export:mono_method_get_wrapper_cache")
#pragma comment(linker, "/export:mono_method_get_wrapper_data")
#pragma comment(linker, "/export:mono_method_has_marshal_info")
#pragma comment(linker, "/export:mono_method_has_no_body")
#pragma comment(linker, "/export:mono_method_header_get_clauses")
#pragma comment(linker, "/export:mono_method_header_get_code")
#pragma comment(linker, "/export:mono_method_header_get_locals")
#pragma comment(linker, "/export:mono_method_header_get_num_clauses")
#pragma comment(linker, "/export:mono_method_is_from_assembly")
#pragma comment(linker, "/export:mono_method_is_generic_impl")
#pragma comment(linker, "/export:mono_method_is_generic_sharable")
#pragma comment(linker, "/export:mono_method_is_generic_sharable_full")
#pragma comment(linker, "/export:mono_method_lookup_or_register_info")
#pragma comment(linker, "/export:mono_method_needs_static_rgctx_invoke")
#pragma comment(linker, "/export:mono_method_print_code")
#pragma comment(linker, "/export:mono_method_return_message_restore")
#pragma comment(linker, "/export:mono_method_same_domain")
#pragma comment(linker, "/export:mono_method_search_in_array_class")
#pragma comment(linker, "/export:mono_method_set_generic_container")
#pragma comment(linker, "/export:mono_method_signature")
#pragma comment(linker, "/export:mono_method_signature_checked")
#pragma comment(linker, "/export:mono_method_verify")
#pragma comment(linker, "/export:mono_method_verify_with_current_settings")
#pragma comment(linker, "/export:mono_object_castclass_mbyref")
#pragma comment(linker, "/export:mono_object_castclass_unbox")
#pragma comment(linker, "/export:mono_object_castclass_with_cache")
#pragma comment(linker, "/export:mono_object_clone")
#pragma comment(linker, "/export:mono_object_clone_checked")
#pragma comment(linker, "/export:mono_object_clone_handle")
#pragma comment(linker, "/export:mono_object_describe")
#pragma comment(linker, "/export:mono_object_describe_fields")
#pragma comment(linker, "/export:mono_object_get_class")
#pragma comment(linker, "/export:mono_object_get_data")
#pragma comment(linker, "/export:mono_object_get_domain")
#pragma comment(linker, "/export:mono_object_get_size")
#pragma comment(linker, "/export:mono_object_get_virtual_method")
#pragma comment(linker, "/export:mono_object_get_vtable")
#pragma comment(linker, "/export:mono_object_handle_get_virtual_method")
#pragma comment(linker, "/export:mono_object_handle_isinst")
#pragma comment(linker, "/export:mono_object_handle_isinst_mbyref")
#pragma comment(linker, "/export:mono_object_handle_pin_unbox")
#pragma comment(linker, "/export:mono_object_hash")
#pragma comment(linker, "/export:mono_object_is_alive")
#pragma comment(linker, "/export:mono_object_is_from_assembly")
#pragma comment(linker, "/export:mono_object_isinst")
#pragma comment(linker, "/export:mono_object_isinst_checked")
#pragma comment(linker, "/export:mono_object_isinst_icall")
#pragma comment(linker, "/export:mono_object_isinst_mbyref")
#pragma comment(linker, "/export:mono_object_isinst_with_cache")
#pragma comment(linker, "/export:mono_object_new")
#pragma comment(linker, "/export:mono_object_new_alloc_by_vtable")
#pragma comment(linker, "/export:mono_object_new_alloc_specific")
#pragma comment(linker, "/export:mono_object_new_alloc_specific_checked")
#pragma comment(linker, "/export:mono_object_new_checked")
#pragma comment(linker, "/export:mono_object_new_fast")
#pragma comment(linker, "/export:mono_object_new_from_token")
#pragma comment(linker, "/export:mono_object_new_handle")
#pragma comment(linker, "/export:mono_object_new_handle_mature")
#pragma comment(linker, "/export:mono_object_new_mature")
#pragma comment(linker, "/export:mono_object_new_pinned")
#pragma comment(linker, "/export:mono_object_new_pinned_handle")
#pragma comment(linker, "/export:mono_object_new_specific")
#pragma comment(linker, "/export:mono_object_new_specific_checked")
#pragma comment(linker, "/export:mono_object_register_finalizer")
#pragma comment(linker, "/export:mono_object_register_finalizer_handle")
#pragma comment(linker, "/export:mono_object_to_string")
#pragma comment(linker, "/export:mono_object_try_to_string")
#pragma comment(linker, "/export:mono_object_unbox")
#pragma comment(linker, "/export:mono_object_xdomain_representation")
#pragma comment(linker, "/export:mono_profiler_call_context_free_buffer")
#pragma comment(linker, "/export:mono_profiler_call_context_get_argument")
#pragma comment(linker, "/export:mono_profiler_call_context_get_local")
#pragma comment(linker, "/export:mono_profiler_call_context_get_result")
#pragma comment(linker, "/export:mono_profiler_call_context_get_this")
#pragma comment(linker, "/export:mono_profiler_cleanup")
#pragma comment(linker, "/export:mono_profiler_coverage_alloc")
#pragma comment(linker, "/export:mono_profiler_coverage_instrumentation_enabled")
#pragma comment(linker, "/export:mono_profiler_create")
#pragma comment(linker, "/export:mono_profiler_enable_allocations")
#pragma comment(linker, "/export:mono_profiler_enable_call_context_introspection")
#pragma comment(linker, "/export:mono_profiler_enable_clauses")
#pragma comment(linker, "/export:mono_profiler_enable_coverage")
#pragma comment(linker, "/export:mono_profiler_enable_sampling")
#pragma comment(linker, "/export:mono_profiler_get_call_instrumentation_flags")
#pragma comment(linker, "/export:mono_profiler_get_coverage_data")
#pragma comment(linker, "/export:mono_profiler_get_sample_mode")
#pragma comment(linker, "/export:mono_profiler_install")
#pragma comment(linker, "/export:mono_profiler_install_allocation")
#pragma comment(linker, "/export:mono_profiler_install_enter_leave")
#pragma comment(linker, "/export:mono_profiler_install_exception")
#pragma comment(linker, "/export:mono_profiler_install_gc")
#pragma comment(linker, "/export:mono_profiler_install_jit_end")
#pragma comment(linker, "/export:mono_profiler_install_thread")
#pragma comment(linker, "/export:mono_profiler_load")
#pragma comment(linker, "/export:mono_profiler_raise_assembly_loaded")
#pragma comment(linker, "/export:mono_profiler_raise_assembly_loading")
#pragma comment(linker, "/export:mono_profiler_raise_assembly_unloaded")
#pragma comment(linker, "/export:mono_profiler_raise_assembly_unloading")
#pragma comment(linker, "/export:mono_profiler_raise_class_failed")
#pragma comment(linker, "/export:mono_profiler_raise_class_loaded")
#pragma comment(linker, "/export:mono_profiler_raise_class_loading")
#pragma comment(linker, "/export:mono_profiler_raise_context_loaded")
#pragma comment(linker, "/export:mono_profiler_raise_context_unloaded")
#pragma comment(linker, "/export:mono_profiler_raise_domain_loaded")
#pragma comment(linker, "/export:mono_profiler_raise_domain_loading")
#pragma comment(linker, "/export:mono_profiler_raise_domain_name")
#pragma comment(linker, "/export:mono_profiler_raise_domain_unloaded")
#pragma comment(linker, "/export:mono_profiler_raise_domain_unloading")
#pragma comment(linker, "/export:mono_profiler_raise_exception_clause")
#pragma comment(linker, "/export:mono_profiler_raise_exception_throw")
#pragma comment(linker, "/export:mono_profiler_raise_gc_allocation")
#pragma comment(linker, "/export:mono_profiler_raise_gc_event")
#pragma comment(linker, "/export:mono_profiler_raise_gc_finalized")
#pragma comment(linker, "/export:mono_profiler_raise_gc_finalized_object")
#pragma comment(linker, "/export:mono_profiler_raise_gc_finalizing")
#pragma comment(linker, "/export:mono_profiler_raise_gc_finalizing_object")
#pragma comment(linker, "/export:mono_profiler_raise_gc_handle_created")
#pragma comment(linker, "/export:mono_profiler_raise_gc_handle_deleted")
#pragma comment(linker, "/export:mono_profiler_raise_gc_moves")
#pragma comment(linker, "/export:mono_profiler_raise_gc_resize")
#pragma comment(linker, "/export:mono_profiler_raise_gc_root_register")
#pragma comment(linker, "/export:mono_profiler_raise_gc_root_unregister")
#pragma comment(linker, "/export:mono_profiler_raise_gc_roots")
#pragma comment(linker, "/export:mono_profiler_raise_image_failed")
#pragma comment(linker, "/export:mono_profiler_raise_image_loaded")
#pragma comment(linker, "/export:mono_profiler_raise_image_loading")
#pragma comment(linker, "/export:mono_profiler_raise_image_unloaded")
#pragma comment(linker, "/export:mono_profiler_raise_image_unloading")
#pragma comment(linker, "/export:mono_profiler_raise_jit_begin")
#pragma comment(linker, "/export:mono_profiler_raise_jit_chunk_created")
#pragma comment(linker, "/export:mono_profiler_raise_jit_chunk_destroyed")
#pragma comment(linker, "/export:mono_profiler_raise_jit_code_buffer")
#pragma comment(linker, "/export:mono_profiler_raise_jit_done")
#pragma comment(linker, "/export:mono_profiler_raise_jit_failed")
#pragma comment(linker, "/export:mono_profiler_raise_method_begin_invoke")
#pragma comment(linker, "/export:mono_profiler_raise_method_end_invoke")
#pragma comment(linker, "/export:mono_profiler_raise_method_enter")
#pragma comment(linker, "/export:mono_profiler_raise_method_exception_leave")
#pragma comment(linker, "/export:mono_profiler_raise_method_free")
#pragma comment(linker, "/export:mono_profiler_raise_method_leave")
#pragma comment(linker, "/export:mono_profiler_raise_method_tail_call")
#pragma comment(linker, "/export:mono_profiler_raise_monitor_acquired")
#pragma comment(linker, "/export:mono_profiler_raise_monitor_contention")
#pragma comment(linker, "/export:mono_profiler_raise_monitor_failed")
#pragma comment(linker, "/export:mono_profiler_raise_runtime_initialized")
#pragma comment(linker, "/export:mono_profiler_raise_runtime_shutdown_begin")
#pragma comment(linker, "/export:mono_profiler_raise_runtime_shutdown_end")
#pragma comment(linker, "/export:mono_profiler_raise_sample_hit")
#pragma comment(linker, "/export:mono_profiler_raise_thread_exited")
#pragma comment(linker, "/export:mono_profiler_raise_thread_name")
#pragma comment(linker, "/export:mono_profiler_raise_thread_started")
#pragma comment(linker, "/export:mono_profiler_raise_thread_stopped")
#pragma comment(linker, "/export:mono_profiler_raise_thread_stopping")
#pragma comment(linker, "/export:mono_profiler_raise_vtable_failed")
#pragma comment(linker, "/export:mono_profiler_raise_vtable_loaded")
#pragma comment(linker, "/export:mono_profiler_raise_vtable_loading")
#pragma comment(linker, "/export:mono_profiler_sampling_enabled")
#pragma comment(linker, "/export:mono_profiler_sampling_thread_post")
#pragma comment(linker, "/export:mono_profiler_sampling_thread_wait")
#pragma comment(linker, "/export:mono_profiler_set_assembly_loaded_callback")
#pragma comment(linker, "/export:mono_profiler_set_assembly_loading_callback")
#pragma comment(linker, "/export:mono_profiler_set_assembly_unloaded_callback")
#pragma comment(linker, "/export:mono_profiler_set_assembly_unloading_callback")
#pragma comment(linker, "/export:mono_profiler_set_call_instrumentation_filter_callback")
#pragma comment(linker, "/export:mono_profiler_set_class_failed_callback")
#pragma comment(linker, "/export:mono_profiler_set_class_loaded_callback")
#pragma comment(linker, "/export:mono_profiler_set_class_loading_callback")
#pragma comment(linker, "/export:mono_profiler_set_cleanup_callback")
#pragma comment(linker, "/export:mono_profiler_set_context_loaded_callback")
#pragma comment(linker, "/export:mono_profiler_set_context_unloaded_callback")
#pragma comment(linker, "/export:mono_profiler_set_coverage_filter_callback")
#pragma comment(linker, "/export:mono_profiler_set_domain_loaded_callback")
#pragma comment(linker, "/export:mono_profiler_set_domain_loading_callback")
#pragma comment(linker, "/export:mono_profiler_set_domain_name_callback")
#pragma comment(linker, "/export:mono_profiler_set_domain_unloaded_callback")
#pragma comment(linker, "/export:mono_profiler_set_domain_unloading_callback")
#pragma comment(linker, "/export:mono_profiler_set_events")
#pragma comment(linker, "/export:mono_profiler_set_exception_clause_callback")
#pragma comment(linker, "/export:mono_profiler_set_exception_throw_callback")
#pragma comment(linker, "/export:mono_profiler_set_gc_allocation_callback")
#pragma comment(linker, "/export:mono_profiler_set_gc_event_callback")
#pragma comment(linker, "/export:mono_profiler_set_gc_finalized_callback")
#pragma comment(linker, "/export:mono_profiler_set_gc_finalized_object_callback")
#pragma comment(linker, "/export:mono_profiler_set_gc_finalizing_callback")
#pragma comment(linker, "/export:mono_profiler_set_gc_finalizing_object_callback")
#pragma comment(linker, "/export:mono_profiler_set_gc_handle_created_callback")
#pragma comment(linker, "/export:mono_profiler_set_gc_handle_deleted_callback")
#pragma comment(linker, "/export:mono_profiler_set_gc_moves_callback")
#pragma comment(linker, "/export:mono_profiler_set_gc_resize_callback")
#pragma comment(linker, "/export:mono_profiler_set_gc_root_register_callback")
#pragma comment(linker, "/export:mono_profiler_set_gc_root_unregister_callback")
#pragma comment(linker, "/export:mono_profiler_set_gc_roots_callback")
#pragma comment(linker, "/export:mono_profiler_set_image_failed_callback")
#pragma comment(linker, "/export:mono_profiler_set_image_loaded_callback")
#pragma comment(linker, "/export:mono_profiler_set_image_loading_callback")
#pragma comment(linker, "/export:mono_profiler_set_image_unloaded_callback")
#pragma comment(linker, "/export:mono_profiler_set_image_unloading_callback")
#pragma comment(linker, "/export:mono_profiler_set_jit_begin_callback")
#pragma comment(linker, "/export:mono_profiler_set_jit_chunk_created_callback")
#pragma comment(linker, "/export:mono_profiler_set_jit_chunk_destroyed_callback")
#pragma comment(linker, "/export:mono_profiler_set_jit_code_buffer_callback")
#pragma comment(linker, "/export:mono_profiler_set_jit_done_callback")
#pragma comment(linker, "/export:mono_profiler_set_jit_failed_callback")
#pragma comment(linker, "/export:mono_profiler_set_method_begin_invoke_callback")
#pragma comment(linker, "/export:mono_profiler_set_method_end_invoke_callback")
#pragma comment(linker, "/export:mono_profiler_set_method_enter_callback")
#pragma comment(linker, "/export:mono_profiler_set_method_exception_leave_callback")
#pragma comment(linker, "/export:mono_profiler_set_method_free_callback")
#pragma comment(linker, "/export:mono_profiler_set_method_leave_callback")
#pragma comment(linker, "/export:mono_profiler_set_method_tail_call_callback")
#pragma comment(linker, "/export:mono_profiler_set_monitor_acquired_callback")
#pragma comment(linker, "/export:mono_profiler_set_monitor_contention_callback")
#pragma comment(linker, "/export:mono_profiler_set_monitor_failed_callback")
#pragma comment(linker, "/export:mono_profiler_set_runtime_initialized_callback")
#pragma comment(linker, "/export:mono_profiler_set_runtime_shutdown_begin_callback")
#pragma comment(linker, "/export:mono_profiler_set_runtime_shutdown_end_callback")
#pragma comment(linker, "/export:mono_profiler_set_sample_hit_callback")
#pragma comment(linker, "/export:mono_profiler_set_sample_mode")
#pragma comment(linker, "/export:mono_profiler_set_thread_exited_callback")
#pragma comment(linker, "/export:mono_profiler_set_thread_name_callback")
#pragma comment(linker, "/export:mono_profiler_set_thread_started_callback")
#pragma comment(linker, "/export:mono_profiler_set_thread_stopped_callback")
#pragma comment(linker, "/export:mono_profiler_set_thread_stopping_callback")
#pragma comment(linker, "/export:mono_profiler_set_vtable_failed_callback")
#pragma comment(linker, "/export:mono_profiler_set_vtable_loaded_callback")
#pragma comment(linker, "/export:mono_profiler_set_vtable_loading_callback")
#pragma comment(linker, "/export:mono_profiler_started")
#pragma comment(linker, "/export:mono_profiler_state")
#pragma comment(linker, "/export:mono_property_bag_add")
#pragma comment(linker, "/export:mono_property_bag_get")
#pragma comment(linker, "/export:mono_property_get_flags")
#pragma comment(linker, "/export:mono_property_get_get_method")
#pragma comment(linker, "/export:mono_property_get_name")
#pragma comment(linker, "/export:mono_property_get_object")
#pragma comment(linker, "/export:mono_property_get_object_checked")
#pragma comment(linker, "/export:mono_property_get_object_handle")
#pragma comment(linker, "/export:mono_property_get_parent")
#pragma comment(linker, "/export:mono_property_get_set_method")
#pragma comment(linker, "/export:mono_property_get_value")
#pragma comment(linker, "/export:mono_property_get_value_checked")
#pragma comment(linker, "/export:mono_property_hash_destroy")
#pragma comment(linker, "/export:mono_property_hash_insert")
#pragma comment(linker, "/export:mono_property_hash_lookup")
#pragma comment(linker, "/export:mono_property_hash_new")
#pragma comment(linker, "/export:mono_property_hash_remove_object")
#pragma comment(linker, "/export:mono_property_set_value")
#pragma comment(linker, "/export:mono_property_set_value_handle")
#pragma comment(linker, "/export:mono_raise_exception")
#pragma comment(linker, "/export:mono_raise_exception_deprecated")
#pragma comment(linker, "/export:mono_raise_exception_with_context")
#pragma comment(linker, "/export:mono_reflection_assembly_get_assembly")
#pragma comment(linker, "/export:mono_reflection_bind_generic_parameters")
#pragma comment(linker, "/export:mono_reflection_call_is_assignable_to")
#pragma comment(linker, "/export:mono_reflection_cleanup_assembly")
#pragma comment(linker, "/export:mono_reflection_cleanup_domain")
#pragma comment(linker, "/export:mono_reflection_create_custom_attr_data_args")
#pragma comment(linker, "/export:mono_reflection_create_custom_attr_data_args_noalloc")
#pragma comment(linker, "/export:mono_reflection_dynimage_basic_init")
#pragma comment(linker, "/export:mono_reflection_emit_init")
#pragma comment(linker, "/export:mono_reflection_free_type_info")
#pragma comment(linker, "/export:mono_reflection_get_custom_attrs")
#pragma comment(linker, "/export:mono_reflection_get_custom_attrs_blob")
#pragma comment(linker, "/export:mono_reflection_get_custom_attrs_blob_checked")
#pragma comment(linker, "/export:mono_reflection_get_custom_attrs_by_type")
#pragma comment(linker, "/export:mono_reflection_get_custom_attrs_by_type_handle")
#pragma comment(linker, "/export:mono_reflection_get_custom_attrs_data")
#pragma comment(linker, "/export:mono_reflection_get_custom_attrs_data_checked")
#pragma comment(linker, "/export:mono_reflection_get_custom_attrs_info")
#pragma comment(linker, "/export:mono_reflection_get_custom_attrs_info_checked")
#pragma comment(linker, "/export:mono_reflection_get_dynamic_overrides")
#pragma comment(linker, "/export:mono_reflection_get_token")
#pragma comment(linker, "/export:mono_reflection_get_token_checked")
#pragma comment(linker, "/export:mono_reflection_get_type")
#pragma comment(linker, "/export:mono_reflection_get_type_checked")
#pragma comment(linker, "/export:mono_reflection_init")
#pragma comment(linker, "/export:mono_reflection_is_usertype")
#pragma comment(linker, "/export:mono_reflection_lookup_dynamic_token")
#pragma comment(linker, "/export:mono_reflection_lookup_signature")
#pragma comment(linker, "/export:mono_reflection_marshal_as_attribute_from_marshal_spec")
#pragma comment(linker, "/export:mono_reflection_method_count_clauses")
#pragma comment(linker, "/export:mono_reflection_methodbuilder_from_ctor_builder")
#pragma comment(linker, "/export:mono_reflection_methodbuilder_from_method_builder")
#pragma comment(linker, "/export:mono_reflection_parse_type")
#pragma comment(linker, "/export:mono_reflection_parse_type_checked")
#pragma comment(linker, "/export:mono_reflection_resolution_scope_from_image")
#pragma comment(linker, "/export:mono_reflection_resolve_object")
#pragma comment(linker, "/export:mono_reflection_resolve_object_handle")
#pragma comment(linker, "/export:mono_reflection_type_from_name")
#pragma comment(linker, "/export:mono_reflection_type_from_name_checked")
#pragma comment(linker, "/export:mono_reflection_type_get_handle")
#pragma comment(linker, "/export:mono_reflection_type_get_type")
#pragma comment(linker, "/export:mono_reflection_type_handle_mono_type")
#pragma comment(linker, "/export:mono_runtime_class_init")
#pragma comment(linker, "/export:mono_runtime_class_init_full")
#pragma comment(linker, "/export:mono_runtime_cleanup")
#pragma comment(linker, "/export:mono_runtime_cleanup_handlers")
#pragma comment(linker, "/export:mono_runtime_create_delegate_trampoline")
#pragma comment(linker, "/export:mono_runtime_create_jump_trampoline")
#pragma comment(linker, "/export:mono_runtime_delegate_invoke")
#pragma comment(linker, "/export:mono_runtime_delegate_invoke_checked")
#pragma comment(linker, "/export:mono_runtime_delegate_try_invoke")
#pragma comment(linker, "/export:mono_runtime_exec_main")
#pragma comment(linker, "/export:mono_runtime_exec_main_checked")
#pragma comment(linker, "/export:mono_runtime_exec_managed_code")
#pragma comment(linker, "/export:mono_runtime_free_method")
#pragma comment(linker, "/export:mono_runtime_get_aotid")
#pragma comment(linker, "/export:mono_runtime_get_caller_no_system_or_reflection")
#pragma comment(linker, "/export:mono_runtime_get_main_args")
#pragma comment(linker, "/export:mono_runtime_get_main_args_handle")
#pragma comment(linker, "/export:mono_runtime_get_no_exec")
#pragma comment(linker, "/export:mono_runtime_init")
#pragma comment(linker, "/export:mono_runtime_init_checked")
#pragma comment(linker, "/export:mono_runtime_init_tls")
#pragma comment(linker, "/export:mono_runtime_install_custom_handlers")
#pragma comment(linker, "/export:mono_runtime_install_custom_handlers_usage")
#pragma comment(linker, "/export:mono_runtime_install_handlers")
#pragma comment(linker, "/export:mono_runtime_invoke")
#pragma comment(linker, "/export:mono_runtime_invoke_array")
#pragma comment(linker, "/export:mono_runtime_invoke_array_checked")
#pragma comment(linker, "/export:mono_runtime_invoke_checked")
#pragma comment(linker, "/export:mono_runtime_invoke_handle")
#pragma comment(linker, "/export:mono_runtime_is_shutting_down")
#pragma comment(linker, "/export:mono_runtime_load")
#pragma comment(linker, "/export:mono_runtime_object_init")
#pragma comment(linker, "/export:mono_runtime_object_init_checked")
#pragma comment(linker, "/export:mono_runtime_object_init_handle")
#pragma comment(linker, "/export:mono_runtime_quit")
#pragma comment(linker, "/export:mono_runtime_resource_check_limit")
#pragma comment(linker, "/export:mono_runtime_resource_limit")
#pragma comment(linker, "/export:mono_runtime_resource_set_callback")
#pragma comment(linker, "/export:mono_runtime_run_main")
#pragma comment(linker, "/export:mono_runtime_run_main_checked")
#pragma comment(linker, "/export:mono_runtime_run_module_cctor")
#pragma comment(linker, "/export:mono_runtime_set_main_args")
#pragma comment(linker, "/export:mono_runtime_set_no_exec")
#pragma comment(linker, "/export:mono_runtime_set_pending_exception")
#pragma comment(linker, "/export:mono_runtime_set_shutting_down")
#pragma comment(linker, "/export:mono_runtime_setup_stat_profiler")
#pragma comment(linker, "/export:mono_runtime_shutdown_stat_profiler")
#pragma comment(linker, "/export:mono_runtime_try_exec_main")
#pragma comment(linker, "/export:mono_runtime_try_invoke")
#pragma comment(linker, "/export:mono_runtime_try_invoke_array")
#pragma comment(linker, "/export:mono_runtime_try_invoke_handle")
#pragma comment(linker, "/export:mono_runtime_try_run_main")
#pragma comment(linker, "/export:mono_runtime_try_shutdown")
#pragma comment(linker, "/export:mono_runtime_unhandled_exception_policy_get")
#pragma comment(linker, "/export:mono_runtime_unhandled_exception_policy_set")
#pragma comment(linker, "/export:mono_signature_explicit_this")
#pragma comment(linker, "/export:mono_signature_full_name")
#pragma comment(linker, "/export:mono_signature_get_call_conv")
#pragma comment(linker, "/export:mono_signature_get_desc")
#pragma comment(linker, "/export:mono_signature_get_param_count")
#pragma comment(linker, "/export:mono_signature_get_params")
#pragma comment(linker, "/export:mono_signature_get_return_type")
#pragma comment(linker, "/export:mono_signature_hash")
#pragma comment(linker, "/export:mono_signature_is_instance")
#pragma comment(linker, "/export:mono_signature_no_pinvoke")
#pragma comment(linker, "/export:mono_signature_param_is_out")
#pragma comment(linker, "/export:mono_signature_vararg_start")
#pragma comment(linker, "/export:mono_stack_mark_pop_value")
#pragma comment(linker, "/export:mono_stack_mark_record_size")
#pragma comment(linker, "/export:mono_stack_walk")
#pragma comment(linker, "/export:mono_stack_walk_async_safe")
#pragma comment(linker, "/export:mono_stack_walk_no_il")
#pragma comment(linker, "/export:mono_string_builder_to_utf16")
#pragma comment(linker, "/export:mono_string_builder_to_utf8")
#pragma comment(linker, "/export:mono_string_chars")
#pragma comment(linker, "/export:mono_string_empty")
#pragma comment(linker, "/export:mono_string_empty_handle")
#pragma comment(linker, "/export:mono_string_empty_wrapper")
#pragma comment(linker, "/export:mono_string_equal")
#pragma comment(linker, "/export:mono_string_from_blob")
#pragma comment(linker, "/export:mono_string_from_bstr")
#pragma comment(linker, "/export:mono_string_from_bstr_icall")
#pragma comment(linker, "/export:mono_string_from_byvalstr")
#pragma comment(linker, "/export:mono_string_from_byvalwstr")
#pragma comment(linker, "/export:mono_string_from_utf16")
#pragma comment(linker, "/export:mono_string_from_utf16_checked")
#pragma comment(linker, "/export:mono_string_from_utf32")
#pragma comment(linker, "/export:mono_string_from_utf32_checked")
#pragma comment(linker, "/export:mono_string_handle_length")
#pragma comment(linker, "/export:mono_string_handle_pin_chars")
#pragma comment(linker, "/export:mono_string_handle_to_utf8")
#pragma comment(linker, "/export:mono_string_hash")
#pragma comment(linker, "/export:mono_string_intern")
#pragma comment(linker, "/export:mono_string_intern_checked")
#pragma comment(linker, "/export:mono_string_is_interned")
#pragma comment(linker, "/export:mono_string_length")
#pragma comment(linker, "/export:mono_string_new")
#pragma comment(linker, "/export:mono_string_new_checked")
#pragma comment(linker, "/export:mono_string_new_handle")
#pragma comment(linker, "/export:mono_string_new_len")
#pragma comment(linker, "/export:mono_string_new_len_checked")
#pragma comment(linker, "/export:mono_string_new_len_wrapper")
#pragma comment(linker, "/export:mono_string_new_size")
#pragma comment(linker, "/export:mono_string_new_size_checked")
#pragma comment(linker, "/export:mono_string_new_utf16")
#pragma comment(linker, "/export:mono_string_new_utf16_checked")
#pragma comment(linker, "/export:mono_string_new_utf16_handle")
#pragma comment(linker, "/export:mono_string_new_utf32")
#pragma comment(linker, "/export:mono_string_new_utf8_len_handle")
#pragma comment(linker, "/export:mono_string_new_wrapper")
#pragma comment(linker, "/export:mono_string_new_wtf8_len_checked")
#pragma comment(linker, "/export:mono_string_to_ansibstr")
#pragma comment(linker, "/export:mono_string_to_bstr")
#pragma comment(linker, "/export:mono_string_to_byvalstr")
#pragma comment(linker, "/export:mono_string_to_byvalwstr")
#pragma comment(linker, "/export:mono_string_to_utf16")
#pragma comment(linker, "/export:mono_string_to_utf32")
#pragma comment(linker, "/export:mono_string_to_utf8")
#pragma comment(linker, "/export:mono_string_to_utf8_checked")
#pragma comment(linker, "/export:mono_string_to_utf8_ignore")
#pragma comment(linker, "/export:mono_string_to_utf8_image")
#pragma comment(linker, "/export:mono_string_to_utf8str")
#pragma comment(linker, "/export:mono_string_to_utf8str_handle")
#pragma comment(linker, "/export:mono_string_utf16_to_builder")
#pragma comment(linker, "/export:mono_string_utf16_to_builder2")
#pragma comment(linker, "/export:mono_string_utf8_to_builder")
#pragma comment(linker, "/export:mono_string_utf8_to_builder2")
#pragma comment(linker, "/export:mono_thread_attach")
#pragma comment(linker, "/export:mono_thread_attach_aborted_cb")
#pragma comment(linker, "/export:mono_thread_callbacks_init")
#pragma comment(linker, "/export:mono_thread_cleanup")
#pragma comment(linker, "/export:mono_thread_cleanup_apartment_state")
#pragma comment(linker, "/export:mono_thread_clear_and_set_state")
#pragma comment(linker, "/export:mono_thread_clr_state")
#pragma comment(linker, "/export:mono_thread_create")
#pragma comment(linker, "/export:mono_thread_create_checked")
#pragma comment(linker, "/export:mono_thread_create_internal")
#pragma comment(linker, "/export:mono_thread_create_internal_handle")
#pragma comment(linker, "/export:mono_thread_current")
#pragma comment(linker, "/export:mono_thread_current_check_pending_interrupt")
#pragma comment(linker, "/export:mono_thread_detach")
#pragma comment(linker, "/export:mono_thread_detach_if_exiting")
#pragma comment(linker, "/export:mono_thread_exit")
#pragma comment(linker, "/export:mono_thread_force_interruption_checkpoint_noraise")
#pragma comment(linker, "/export:mono_thread_get_main")
#pragma comment(linker, "/export:mono_thread_get_managed_id")
#pragma comment(linker, "/export:mono_thread_get_name")
#pragma comment(linker, "/export:mono_thread_get_name_utf8")
#pragma comment(linker, "/export:mono_thread_get_undeniable_exception")
#pragma comment(linker, "/export:mono_thread_has_appdomain_ref")
#pragma comment(linker, "/export:mono_thread_hazardous_queue_free")
#pragma comment(linker, "/export:mono_thread_hazardous_try_free")
#pragma comment(linker, "/export:mono_thread_hazardous_try_free_all")
#pragma comment(linker, "/export:mono_thread_hazardous_try_free_some")
#pragma comment(linker, "/export:mono_thread_init")
#pragma comment(linker, "/export:mono_thread_init_apartment_state")
#pragma comment(linker, "/export:mono_thread_interruption_checkpoint")
#pragma comment(linker, "/export:mono_thread_interruption_checkpoint_bool")
#pragma comment(linker, "/export:mono_thread_interruption_checkpoint_void")
#pragma comment(linker, "/export:mono_thread_interruption_request_flag")
#pragma comment(linker, "/export:mono_thread_interruption_requested")
#pragma comment(linker, "/export:mono_thread_is_foreign")
#pragma comment(linker, "/export:mono_thread_is_gc_unsafe_mode")
#pragma comment(linker, "/export:mono_thread_join")
#pragma comment(linker, "/export:mono_thread_manage")
#pragma comment(linker, "/export:mono_thread_new_init")
#pragma comment(linker, "/export:mono_thread_platform_create_thread")
#pragma comment(linker, "/export:mono_thread_pop_appdomain_ref")
#pragma comment(linker, "/export:mono_thread_push_appdomain_ref")
#pragma comment(linker, "/export:mono_thread_set_main")
#pragma comment(linker, "/export:mono_thread_set_manage_callback")
#pragma comment(linker, "/export:mono_thread_set_name_internal")
#pragma comment(linker, "/export:mono_thread_set_state")
#pragma comment(linker, "/export:mono_thread_small_id_alloc")
#pragma comment(linker, "/export:mono_thread_small_id_free")
#pragma comment(linker, "/export:mono_thread_smr_cleanup")
#pragma comment(linker, "/export:mono_thread_smr_init")
#pragma comment(linker, "/export:mono_thread_stop")
#pragma comment(linker, "/export:mono_thread_test_and_set_state")
#pragma comment(linker, "/export:mono_thread_test_state")
#pragma comment(linker, "/export:mono_type_array_get_and_resolve")
#pragma comment(linker, "/export:mono_type_create_from_typespec")
#pragma comment(linker, "/export:mono_type_create_from_typespec_checked")
#pragma comment(linker, "/export:mono_type_full_name")
#pragma comment(linker, "/export:mono_type_generic_inst_is_valuetype")
#pragma comment(linker, "/export:mono_type_get_array_type")
#pragma comment(linker, "/export:mono_type_get_basic_type_from_generic")
#pragma comment(linker, "/export:mono_type_get_checked")
#pragma comment(linker, "/export:mono_type_get_class")
#pragma comment(linker, "/export:mono_type_get_cmods")
#pragma comment(linker, "/export:mono_type_get_desc")
#pragma comment(linker, "/export:mono_type_get_full_name")
#pragma comment(linker, "/export:mono_type_get_modifiers")
#pragma comment(linker, "/export:mono_type_get_name")
#pragma comment(linker, "/export:mono_type_get_name_full")
#pragma comment(linker, "/export:mono_type_get_object")
#pragma comment(linker, "/export:mono_type_get_object_checked")
#pragma comment(linker, "/export:mono_type_get_object_handle")
#pragma comment(linker, "/export:mono_type_get_ptr_type")
#pragma comment(linker, "/export:mono_type_get_signature")
#pragma comment(linker, "/export:mono_type_get_type")
#pragma comment(linker, "/export:mono_type_get_underlying_type")
#pragma comment(linker, "/export:mono_type_has_exceptions")
#pragma comment(linker, "/export:mono_type_in_image")
#pragma comment(linker, "/export:mono_type_initialization_cleanup")
#pragma comment(linker, "/export:mono_type_initialization_init")
#pragma comment(linker, "/export:mono_type_is_byref")
#pragma comment(linker, "/export:mono_type_is_from_assembly")
#pragma comment(linker, "/export:mono_type_is_generic_parameter")
#pragma comment(linker, "/export:mono_type_is_pointer")
#pragma comment(linker, "/export:mono_type_is_primitive")
#pragma comment(linker, "/export:mono_type_is_reference")
#pragma comment(linker, "/export:mono_type_is_struct")
#pragma comment(linker, "/export:mono_type_is_valid_enum_basetype")
#pragma comment(linker, "/export:mono_type_is_void")
#pragma comment(linker, "/export:mono_type_native_stack_size")
#pragma comment(linker, "/export:mono_type_set_alignment")
#pragma comment(linker, "/export:mono_type_size")
#pragma comment(linker, "/export:mono_type_stack_size")
#pragma comment(linker, "/export:mono_type_stack_size_internal")
#pragma comment(linker, "/export:mono_value_box")
#pragma comment(linker, "/export:mono_value_copy")
#pragma comment(linker, "/export:mono_value_copy_array")
#pragma comment(linker, "/export:mono_jit_info_get_code_start")
#pragma comment(linker, "/export:mono_jit_info_get_code_size")

#endif
