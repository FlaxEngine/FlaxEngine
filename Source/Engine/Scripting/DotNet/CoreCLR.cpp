// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "CoreCLR.h"

#if USE_NETCORE
#include "Engine/Core/Log.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Debug/DebugLog.h"
#include "Engine/Engine/Globals.h"
#if DOTNET_HOST_CORECRL
#include <nethost.h>
#include <coreclr_delegates.h>
#include <hostfxr.h>
#elif DOTNET_HOST_MONO
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Scripting/ManagedCLR/MDomain.h"
#include "Engine/Debug/Exceptions/CLRInnerException.h"
#include <mono/jit/jit.h>
#include <mono/jit/mono-private-unstable.h>
#include <mono/utils/mono-logger.h>
typedef char char_t;
#else
#error "Unknown .NET runtime host."
#endif
#if PLATFORM_WINDOWS
#include <combaseapi.h>
#undef SetEnvironmentVariable
#undef LoadLibrary
#endif

static Dictionary<String, void*> CachedFunctions;
static const char_t* NativeInteropTypeName = FLAX_CORECLR_TEXT("FlaxEngine.NativeInterop, FlaxEngine.CSharp");

#if DOTNET_HOST_CORECRL

hostfxr_initialize_for_runtime_config_fn hostfxr_initialize_for_runtime_config;
hostfxr_initialize_for_dotnet_command_line_fn hostfxr_initialize_for_dotnet_command_line;
hostfxr_get_runtime_delegate_fn hostfxr_get_runtime_delegate;
hostfxr_close_fn hostfxr_close;
load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer;
get_function_pointer_fn get_function_pointer;
hostfxr_set_error_writer_fn hostfxr_set_error_writer;
hostfxr_get_dotnet_environment_info_result_fn hostfxr_get_dotnet_environment_info_result;
hostfxr_run_app_fn hostfxr_run_app;

bool CoreCLR::InitHostfxr(const String& configPath, const String& libraryPath)
{
    const FLAX_CORECLR_STRING& library_path = FLAX_CORECLR_STRING(libraryPath);

    // Get path to hostfxr library
    get_hostfxr_parameters get_hostfxr_params;
    get_hostfxr_params.size = sizeof(hostfxr_initialize_parameters);
    get_hostfxr_params.assembly_path = library_path.Get();
    FLAX_CORECLR_STRING dotnetRoot;
    // TODO: implement proper lookup for dotnet instalation folder and handle standalone build of FlaxGame
#if PLATFORM_MAC
    get_hostfxr_params.dotnet_root = "/usr/local/share/dotnet";
#else
    get_hostfxr_params.dotnet_root = nullptr;
#endif
#if !USE_EDITOR
    const String& bundledDotnetPath = Globals::ProjectFolder / TEXT("Dotnet");
    if (FileSystem::DirectoryExists(bundledDotnetPath))
    {
        dotnetRoot = FLAX_CORECLR_STRING(bundledDotnetPath);
#if PLATFORM_WINDOWS_FAMILY
        dotnetRoot.Replace('/', '\\');
#endif
        get_hostfxr_params.dotnet_root = dotnetRoot.Get();
    }
#endif
    char_t hostfxrPath[1024];
    size_t hostfxrPathSize = sizeof(hostfxrPath) / sizeof(char_t);
    int rc = get_hostfxr_path(hostfxrPath, &hostfxrPathSize, &get_hostfxr_params);
    if (rc != 0)
    {
        LOG(Error, "Failed to find hostfxr: {0:x} ({1})", (unsigned int)rc, String(get_hostfxr_params.dotnet_root));

        // Warn user about missing .Net
#if PLATFORM_DESKTOP
        Platform::OpenUrl(TEXT("https://dotnet.microsoft.com/en-us/download/dotnet/7.0"));
#endif
#if USE_EDITOR
        LOG(Fatal, "Missing .NET 7 SDK installation requried to run Flax Editor.");
#else
        LOG(Fatal, "Missing .NET 7 Runtime installation requried to run this application.");
#endif
        return true;
    }
    String path(hostfxrPath);
    LOG(Info, "Found hostfxr in {0}", path);

    // Get API from hostfxr library
    void* hostfxr = Platform::LoadLibrary(path.Get());
    if (hostfxr == nullptr)
    {
        LOG(Fatal, "Failed to load hostfxr library ({0})", path);
        return true;
    }
    hostfxr_initialize_for_runtime_config = (hostfxr_initialize_for_runtime_config_fn)Platform::GetProcAddress(hostfxr, "hostfxr_initialize_for_runtime_config");
    hostfxr_initialize_for_dotnet_command_line = (hostfxr_initialize_for_dotnet_command_line_fn)Platform::GetProcAddress(hostfxr, "hostfxr_initialize_for_dotnet_command_line");
    hostfxr_get_runtime_delegate = (hostfxr_get_runtime_delegate_fn)Platform::GetProcAddress(hostfxr, "hostfxr_get_runtime_delegate");
    hostfxr_close = (hostfxr_close_fn)Platform::GetProcAddress(hostfxr, "hostfxr_close");
    hostfxr_set_error_writer = (hostfxr_set_error_writer_fn)Platform::GetProcAddress(hostfxr, "hostfxr_set_error_writer");
    hostfxr_get_dotnet_environment_info_result = (hostfxr_get_dotnet_environment_info_result_fn)Platform::GetProcAddress(hostfxr, "hostfxr_get_dotnet_environment_info_result");
    hostfxr_run_app = (hostfxr_run_app_fn)Platform::GetProcAddress(hostfxr, "hostfxr_run_app");
    if (!hostfxr_get_runtime_delegate || !hostfxr_run_app)
    {
        LOG(Fatal, "Failed to setup hostfxr API ({0})", path);
        return true;
    }

    // Initialize hosting component
    const char_t* argv[1] = { library_path.Get() };
    hostfxr_initialize_parameters init_params;
    init_params.size = sizeof(hostfxr_initialize_parameters);
    init_params.host_path = library_path.Get();
    path = String(StringUtils::GetDirectoryName(path)) / TEXT("/../../../");
    StringUtils::PathRemoveRelativeParts(path);
    dotnetRoot = FLAX_CORECLR_STRING(path);
    init_params.dotnet_root = dotnetRoot.Get();
    hostfxr_handle handle = nullptr;
    rc = hostfxr_initialize_for_dotnet_command_line(ARRAY_COUNT(argv), argv, &init_params, &handle);
    if (rc != 0 || handle == nullptr)
    {
        hostfxr_close(handle);
        LOG(Fatal, "Failed to initialize hostfxr: {0:x} ({1})", (unsigned int)rc, String(init_params.dotnet_root));
        return true;
    }

    void* pget_function_pointer = nullptr;
    rc = hostfxr_get_runtime_delegate(handle, hdt_get_function_pointer, &pget_function_pointer);
    if (rc != 0 || pget_function_pointer == nullptr)
    {
        hostfxr_close(handle);
        LOG(Fatal, "Failed to get runtime delegate hdt_get_function_pointer: 0x{0:x}", (unsigned int)rc);
        return true;
    }

    hostfxr_close(handle);
    get_function_pointer = (get_function_pointer_fn)pget_function_pointer;
    return false;
}

void CoreCLR::ShutdownHostfxr()
{
}

void* CoreCLR::GetStaticMethodPointer(const String& methodName)
{
    void* fun;
    if (CachedFunctions.TryGet(methodName, fun))
        return fun;

    int rc = get_function_pointer(NativeInteropTypeName, FLAX_CORECLR_STRING(methodName).Get(), UNMANAGEDCALLERSONLY_METHOD, nullptr, nullptr, &fun);
    if (rc != 0)
        LOG(Fatal, "Failed to get unmanaged function pointer for method {0}: 0x{1:x}", methodName.Get(), (unsigned int)rc);

    CachedFunctions.Add(methodName, fun);
    return fun;
}

#elif DOTNET_HOST_MONO

#ifdef USE_MONO_AOT_MODULE
void* MonoAotModuleHandle = nullptr;
#endif
MonoDomain* MonoDomainHandle = nullptr;

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

bool CoreCLR::InitHostfxr(const String& configPath, const String& libraryPath)
{
#if 1
    // Enable detailed Mono logging
    Platform::SetEnvironmentVariable(TEXT("MONO_LOG_LEVEL"), TEXT("debug"));
    Platform::SetEnvironmentVariable(TEXT("MONO_LOG_MASK"), TEXT("all"));
    //Platform::SetEnvironmentVariable(TEXT("MONO_GC_DEBUG"), TEXT("6:gc-log.txt,check-remset-consistency,nursery-canaries"));
#endif

#if defined(USE_MONO_AOT_MODE)
    // Enable AOT mode (per-platform)
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

    // TODO: setup C# debugger

    // Connect to mono engine callback system
    mono_trace_set_log_handler(OnLogCallback, nullptr);
    mono_trace_set_print_handler(OnPrintCallback);
    mono_trace_set_printerr_handler(OnPrintErrorCallback);

    // Initialize Mono VM
    StringAnsi baseDirectory(Globals::ProjectFolder);
    const char* appctxKeys[] =
    {
        "RUNTIME_IDENTIFIER",
        "APP_CONTEXT_BASE_DIRECTORY",
    };
    const char* appctxValues[] =
    {
        MACRO_TO_STR(DOTNET_HOST_RUNTIME_IDENTIFIER),
        baseDirectory.Get(),
    };
    static_assert(ARRAY_COUNT(appctxKeys) == ARRAY_COUNT(appctxValues), "Invalid appctx setup");
    monovm_initialize(ARRAY_COUNT(appctxKeys), appctxKeys, appctxValues);

    // Init managed runtime
#if PLATFORM_ANDROID || PLATFORM_IOS
    const char* monoVersion = "mobile";
#else
    const char* monoVersion = ""; // ignored
#endif
    MonoDomainHandle = mono_jit_init_version("Flax", monoVersion);
    if (!MonoDomainHandle)
    {
        LOG(Fatal, "Failed to initialize Mono.");
        return true;
    }

    // Log info
    char* buildInfo = mono_get_runtime_build_info();
    LOG(Info, "Mono runtime version: {0}", String(buildInfo));
    mono_free(buildInfo);

    return false;
}

void CoreCLR::ShutdownHostfxr()
{
    mono_jit_cleanup(MonoDomainHandle);
    MonoDomainHandle = nullptr;

#ifdef USE_MONO_AOT_MODULE
    Platform::FreeLibrary(MonoAotModuleHandle);
#endif
}

void* CoreCLR::GetStaticMethodPointer(const String& methodName)
{
    MISSING_CODE("TODO: CoreCLR::GetStaticMethodPointer for Mono host runtime");
    return nullptr;
}

#endif

void CoreCLR::RegisterNativeLibrary(const char* moduleName, const char* modulePath)
{
    static void* RegisterNativeLibraryPtr = CoreCLR::GetStaticMethodPointer(TEXT("RegisterNativeLibrary"));
    CoreCLR::CallStaticMethod<void, const char*, const char*>(RegisterNativeLibraryPtr, moduleName, modulePath);
}

void* CoreCLR::Allocate(int32 size)
{
    static void* AllocMemoryPtr = CoreCLR::GetStaticMethodPointer(TEXT("AllocMemory"));
    return CoreCLR::CallStaticMethod<void*, int32>(AllocMemoryPtr, size);
}

void CoreCLR::Free(void* ptr)
{
    if (!ptr)
        return;
    static void* FreeMemoryPtr = CoreCLR::GetStaticMethodPointer(TEXT("FreeMemory"));
    CoreCLR::CallStaticMethod<void, void*>(FreeMemoryPtr, ptr);
}

#endif
