// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "CoreCLR.h"
#if USE_NETCORE
#include "Engine/Core/Log.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Debug/DebugLog.h"
#include "Engine/Core/Collections/Dictionary.h"
#include <nethost.h>
#include <coreclr_delegates.h>
#include <hostfxr.h>
#if PLATFORM_WINDOWS
#include <combaseapi.h>
#undef SetEnvironmentVariable
#undef LoadLibrary
#endif

static Dictionary<String, void*> cachedFunctions;
#if PLATFORM_WINDOWS
static const char_t* typeName = TEXT("FlaxEngine.NativeInterop, FlaxEngine.CSharp");
#else
static const char_t* typeName = "FlaxEngine.NativeInterop, FlaxEngine.CSharp";
#endif

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
    get_hostfxr_params.dotnet_root = nullptr;//dotnetRoot.Get();
    char_t hostfxrPath[1024];
    size_t hostfxrPathSize = sizeof(hostfxrPath) / sizeof(char_t);
    int rc = get_hostfxr_path(hostfxrPath, &hostfxrPathSize, &get_hostfxr_params);
    if (rc != 0)
    {
        LOG(Error, "Failed to find hostfxr: {0:x}", (unsigned int)rc);
        return true;
    }
    const String path(hostfxrPath);
    LOG(Info, "Found hostfxr in {0}", path);

    void* hostfxr = Platform::LoadLibrary(path.Get());
    hostfxr_initialize_for_runtime_config = (hostfxr_initialize_for_runtime_config_fn)Platform::GetProcAddress(hostfxr, "hostfxr_initialize_for_runtime_config");
    hostfxr_initialize_for_dotnet_command_line = (hostfxr_initialize_for_dotnet_command_line_fn)Platform::GetProcAddress(hostfxr, "hostfxr_initialize_for_dotnet_command_line");
    hostfxr_get_runtime_delegate = (hostfxr_get_runtime_delegate_fn)Platform::GetProcAddress(hostfxr, "hostfxr_get_runtime_delegate");
    hostfxr_close = (hostfxr_close_fn)Platform::GetProcAddress(hostfxr, "hostfxr_close");
    hostfxr_set_error_writer = (hostfxr_set_error_writer_fn)Platform::GetProcAddress(hostfxr, "hostfxr_set_error_writer");
    hostfxr_get_dotnet_environment_info_result = (hostfxr_get_dotnet_environment_info_result_fn)Platform::GetProcAddress(hostfxr, "hostfxr_get_dotnet_environment_info_result");
    hostfxr_run_app = (hostfxr_run_app_fn)Platform::GetProcAddress(hostfxr, "hostfxr_run_app");
    if (!hostfxr_get_runtime_delegate || !hostfxr_run_app)
    {
        LOG(Error, "Failed to setup hostfxr API");
        return true;
    }

    // Initialize hosting component
    const char_t* argv[1] = { library_path.Get() };
    hostfxr_initialize_parameters init_params;
    init_params.size = sizeof(hostfxr_initialize_parameters);
    init_params.host_path = library_path.Get();
    init_params.dotnet_root = nullptr;//dotnetRoot.Get(); // This probably must be set
    hostfxr_handle handle = nullptr;
    rc = hostfxr_initialize_for_dotnet_command_line(ARRAY_COUNT(argv), argv, &init_params, &handle);
    if (rc != 0 || handle == nullptr)
    {
        LOG(Error, "Failed to initialize hostfxr: {0:x}", (unsigned int)rc);
        hostfxr_close(handle);
        return true;
    }

    void* pget_function_pointer = nullptr;
    rc = hostfxr_get_runtime_delegate(handle, hdt_get_function_pointer, &pget_function_pointer);
    if (rc != 0 || pget_function_pointer == nullptr)
    {
        LOG(Error, "Failed to get runtime delegate hdt_get_function_pointer: 0x{0:x}", (unsigned int)rc);
        hostfxr_close(handle);
        return true;
    }

    hostfxr_close(handle);
    get_function_pointer = (get_function_pointer_fn)pget_function_pointer;
    return false;
}

void* CoreCLR::GetStaticMethodPointer(const String& methodName)
{
    void* fun;
    if (cachedFunctions.TryGet(methodName, fun))
        return fun;

    int rc = get_function_pointer(typeName, FLAX_CORECLR_STRING(methodName).Get(), UNMANAGEDCALLERSONLY_METHOD, nullptr, nullptr, &fun);
    if (rc != 0)
        LOG(Fatal, "Failed to get unmanaged function pointer for method {0}: 0x{1:x}", methodName.Get(), (unsigned int)rc);

    cachedFunctions.Add(methodName, fun);

    return fun;
}

void CoreCLR::RegisterNativeLibrary(const char* moduleName, const char* modulePath)
{
    static void* RegisterNativeLibraryPtr = CoreCLR::GetStaticMethodPointer(TEXT("RegisterNativeLibrary"));
    CoreCLR::CallStaticMethod<void, const char*, const char*>(RegisterNativeLibraryPtr, moduleName, modulePath);
}

void* CoreCLR::Allocate(int size)
{
    return Platform::Allocate(size, 16);
}

void CoreCLR::Free(void* ptr)
{
    Platform::Free(ptr);
}

#endif
