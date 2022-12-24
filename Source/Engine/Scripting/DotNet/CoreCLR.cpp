#include "CoreCLR.h"

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
#include <combaseapi.h> // CoTask*
#undef SetEnvironmentVariable
#undef LoadLibrary
#endif

#if COMPILE_WITH_PROFILER
#endif

static Dictionary<String, void*> cachedFunctions;
static String assemblyName = TEXT("FlaxEngine.CSharp");
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

bool CoreCLR::LoadHostfxr(const String& library_path_)
{
    const FLAX_CORECLR_STRING& library_path = FLAX_CORECLR_STRING(library_path_);


    char_t hostfxrPath[1024];
    size_t hostfxrPathSize = sizeof(hostfxrPath) / sizeof(char_t);

    get_hostfxr_parameters params;
    params.size = sizeof(hostfxr_initialize_parameters);
    params.assembly_path = library_path.Get();
    params.dotnet_root = nullptr;//dotnetRoot.Get();

    int rc = get_hostfxr_path(hostfxrPath, &hostfxrPathSize, &params);
    if (rc != 0)
    {
        LOG(Error, "Failed to find hostfxr: {0:x}", (unsigned int)rc);
        return false;
    }
    String path(hostfxrPath);
    LOG(Info, "Found hostfxr in {0}", path);

    void *hostfxr = Platform::LoadLibrary(path.Get());
    hostfxr_initialize_for_runtime_config = (hostfxr_initialize_for_runtime_config_fn)Platform::GetProcAddress(hostfxr, "hostfxr_initialize_for_runtime_config");
    hostfxr_initialize_for_dotnet_command_line = (hostfxr_initialize_for_dotnet_command_line_fn)Platform::GetProcAddress(hostfxr, "hostfxr_initialize_for_dotnet_command_line");
    hostfxr_get_runtime_delegate = (hostfxr_get_runtime_delegate_fn)Platform::GetProcAddress(hostfxr, "hostfxr_get_runtime_delegate");
    hostfxr_close = (hostfxr_close_fn)Platform::GetProcAddress(hostfxr, "hostfxr_close");
    hostfxr_set_error_writer = (hostfxr_set_error_writer_fn)Platform::GetProcAddress(hostfxr, "hostfxr_set_error_writer");
    hostfxr_get_dotnet_environment_info_result = (hostfxr_get_dotnet_environment_info_result_fn)Platform::GetProcAddress(hostfxr, "hostfxr_get_dotnet_environment_info_result");
    hostfxr_run_app = (hostfxr_run_app_fn)Platform::GetProcAddress(hostfxr, "hostfxr_run_app");
    
    return true;
}

bool CoreCLR::InitHostfxr(const String& config_path, const String& library_path_)
{
    const FLAX_CORECLR_STRING& library_path = FLAX_CORECLR_STRING(library_path_);
    const char_t* argv[1] = { library_path.Get() };

    hostfxr_initialize_parameters params;
    params.size = sizeof(hostfxr_initialize_parameters);
    params.host_path = library_path.Get();
    params.dotnet_root = nullptr;//dotnetRoot.Get(); // This probably must be set

    hostfxr_handle handle = nullptr;

    // Initialize hosting component, hostfxr_initialize_for_dotnet_command_line is used here
    // to allow self-contained engine installation to be used when needed.

    int rc = hostfxr_initialize_for_dotnet_command_line(1, argv, &params, &handle);
    if (rc != 0 || handle == nullptr)
    {
        LOG(Error, "Failed to initialize hostfxr: {0:x}", (unsigned int)rc);
        hostfxr_close(handle);
        return false;
    }

    void* pget_function_pointer = nullptr;
    rc = hostfxr_get_runtime_delegate(handle, hdt_get_function_pointer, &pget_function_pointer);
    if (rc != 0 || pget_function_pointer == nullptr)
        LOG(Error, "Failed to get runtime delegate hdt_get_function_pointer: {0:x}", (unsigned int)rc);

    hostfxr_close(handle);
    get_function_pointer = (get_function_pointer_fn)pget_function_pointer;

    return true;
}

void* CoreCLR::GetStaticMethodPointer(const String& methodName)
{
    void* fun;
    if (cachedFunctions.TryGet(methodName, fun))
        return fun;

    int rc = get_function_pointer(typeName, FLAX_CORECLR_STRING(methodName).Get(), UNMANAGEDCALLERSONLY_METHOD, nullptr, nullptr, &fun);
    if (rc != 0)
        LOG(Fatal, "Failed to get unmanaged function pointer for method {0}: 0x{1:x}", methodName.Get(), (unsigned int)rc);

    cachedFunctions.Add(String(methodName), fun);

    return fun;
}

void CoreCLR::RegisterNativeLibrary(const char* moduleName, const char* modulePath)
{
    static void* RegisterNativeLibraryPtr = CoreCLR::GetStaticMethodPointer(TEXT("RegisterNativeLibrary"));
    CoreCLR::CallStaticMethod<void, const char*, const char*>(RegisterNativeLibraryPtr, moduleName, modulePath);
}

void* CoreCLR::Allocate(int size)
{
#if PLATFORM_WINDOWS
    void* ptr = CoTaskMemAlloc(size);
#else
    void* ptr = malloc(size);
#endif

#if COMPILE_WITH_PROFILER
    Platform::OnMemoryAlloc(ptr, size);
#endif
    return ptr;
}

void CoreCLR::Free(void* ptr)
{
#if COMPILE_WITH_PROFILER
    Platform::OnMemoryFree(ptr);
#endif
#if PLATFORM_WINDOWS
    CoTaskMemFree(ptr);
#else
    free(ptr);
#endif
}
