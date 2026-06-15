// Copyright (c) Wojciech Figat. All rights reserved.

#include "GPUDeviceVulkan.h"
#include "GPUAdapterVulkan.h"
#include "RenderToolsVulkan.h"
#include "Config.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/ArrayExtensions.h"
#include "Engine/Core/Collections/Sorting.h"
#if VULKAN_USE_PERF_SDK
#include "Engine/Platform/FileSystem.h"
#endif

#if GRAPHICS_API_VULKAN

#if GPU_ENABLE_DEBUG_LAYER
VulkanValidationLevel ValidationLevel = VulkanValidationLevel::ErrorsAndWarningsPerf;
#else
VulkanValidationLevel ValidationLevel = VulkanValidationLevel::Disabled;
#endif

#if VULKAN_USE_DEBUG_LAYER

#define VULKAN_USE_KHRONOS_STANDARD_VALIDATION 1 // uses VK_LAYER_KHRONOS_validation
#define VULKAN_USE_LUNARG_STANDARD_VALIDATION 1 // uses VK_LAYER_LUNARG_standard_validation

static const char* GValidationLayers[] =
{
    "VK_LAYER_GOOGLE_threading",
    "VK_LAYER_LUNARG_parameter_validation",
    "VK_LAYER_LUNARG_object_tracker",
    "VK_LAYER_LUNARG_core_validation",
    nullptr
};

#endif

static const char* GInstanceExtensions[] =
{
#if PLATFORM_APPLE_FAMILY && defined(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
#endif
#if VULKAN_USE_VALIDATION_CACHE
    VK_EXT_VALIDATION_CACHE_EXTENSION_NAME,
#endif
#if defined(VK_KHR_display) && 0
    VK_KHR_DISPLAY_EXTENSION_NAME,
#endif
#if VULKAN_USE_TRACY_GPU && VK_EXT_calibrated_timestamps && VK_EXT_host_query_reset
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, // Required by VK_EXT_host_query_reset (unless using Vulkan 1.1 or newer)
#endif
    nullptr
};

static const char* GDeviceExtensions[] =
{
#if PLATFORM_APPLE_FAMILY && defined(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)
    VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
#endif
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if VK_KHR_maintenance1
    VK_KHR_MAINTENANCE1_EXTENSION_NAME,
#endif
#if VULKAN_USE_VALIDATION_CACHE
    VK_EXT_VALIDATION_CACHE_EXTENSION_NAME,
#endif
#if VK_KHR_sampler_mirror_clamp_to_edge
    VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME,
#endif
#if VULKAN_USE_TRACY_GPU && VK_EXT_calibrated_timestamps && VK_EXT_host_query_reset
    VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME,
    VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME,
#endif
    nullptr
};

struct LayerExtension
{
    VkLayerProperties Layer;
    Array<VkExtensionProperties> Extensions;

    LayerExtension()
    {
        Platform::MemoryClear(&Layer, sizeof(Layer));
    }

    void GetExtensions(Array<StringAnsi>& result)
    {
        for (auto& e : Extensions)
        {
            result.AddUnique(StringAnsi(e.extensionName));
        }
    }

    void GetExtensions(Array<const char*>& result)
    {
        for (auto& e : Extensions)
        {
            result.AddUnique(e.extensionName);
        }
    }
};

static void EnumerateInstanceExtensionProperties(const char* layerName, LayerExtension& outLayer)
{
    VkResult result;
    do
    {
        uint32 count = 0;
        result = vkEnumerateInstanceExtensionProperties(layerName, &count, nullptr);
        ASSERT(result >= VK_SUCCESS);

        if (count > 0)
        {
            outLayer.Extensions.Clear();
            outLayer.Extensions.AddDefault(count);
            result = vkEnumerateInstanceExtensionProperties(layerName, &count, outLayer.Extensions.Get());
            ASSERT(result >= VK_SUCCESS);
        }
    } while (result == VK_INCOMPLETE);
}

static void EnumerateDeviceExtensionProperties(VkPhysicalDevice device, const char* layerName, LayerExtension& outLayer)
{
    VkResult result;
    do
    {
        uint32 count = 0;
        result = vkEnumerateDeviceExtensionProperties(device, layerName, &count, nullptr);
        ASSERT(result >= VK_SUCCESS);

        if (count > 0)
        {
            outLayer.Extensions.Clear();
            outLayer.Extensions.AddDefault(count);
            result = vkEnumerateDeviceExtensionProperties(device, layerName, &count, outLayer.Extensions.Get());
            ASSERT(result >= VK_SUCCESS);
        }
    } while (result == VK_INCOMPLETE);
}

static void TrimDuplicates(Array<const char*>& array)
{
    for (int32 i = array.Count() - 1; i >= 0; i--)
    {
        bool found = false;
        for (int32 j = i - 1; j >= 0; j--)
        {
            if (!StringUtils::Compare(array[i], array[j]))
            {
                found = true;
                break;
            }
        }
        if (found)
        {
            array.RemoveAt(i);
        }
    }
}

static int FindLayerIndex(const Array<LayerExtension>& list, const char* layerName)
{
    for (int32 i = 1; i < list.Count(); i++)
    {
        if (!StringUtils::Compare(list[i].Layer.layerName, layerName))
        {
            return i;
        }
    }
    return -1;
}

static bool ContainsLayer(const Array<LayerExtension>& list, const char* layerName)
{
    return FindLayerIndex(list, layerName) != -1;
}

static bool FindLayerExtension(const Array<LayerExtension>& list, const char* extensionName, const char*& foundLayer)
{
    for (int32 extIndex = 0; extIndex < list.Count(); extIndex++)
    {
        for (int32 i = 0; i < list[extIndex].Extensions.Count(); i++)
        {
            if (!StringUtils::Compare(list[extIndex].Extensions[i].extensionName, extensionName))
            {
                foundLayer = list[extIndex].Layer.layerName;
                return true;
            }
        }
    }
    return false;
}

static bool FindLayerExtension(const Array<LayerExtension>& list, const char* extensionName)
{
    const char* dummy = nullptr;
    return FindLayerExtension(list, extensionName, dummy);
}

static bool ListContains(const Array<const char*>& list, const char* name)
{
    for (const char* element : list)
    {
        if (!StringUtils::Compare(element, name))
            return true;
    }
    return false;
}

static bool ListContains(const Array<StringAnsi>& list, const char* name)
{
    for (const StringAnsi& element : list)
    {
        if (element == name)
            return true;
    }
    return false;
}

#if VULKAN_USE_PERF_SDK

namespace
{
    typedef int NVPA_Status;
    typedef unsigned char NVPA_Bool;

    struct NVPW_VK_Profiler_GetRequiredInstanceExtensions_Params
    {
        size_t structSize;
        void* pPriv;
        const char* const* ppInstanceExtensionNames;
        size_t numInstanceExtensionNames;
        uint32 apiVersion;
        NVPA_Bool isOfficiallySupportedVersion;
    };

    struct NVPW_VK_Profiler_GetRequiredDeviceExtensions_Params
    {
        size_t structSize;
        void* pPriv;
        const char* const* ppDeviceExtensionNames;
        size_t numDeviceExtensionNames;
        uint32 apiVersion;
        NVPA_Bool isOfficiallySupportedVersion;
        VkInstance instance;
        VkPhysicalDevice physicalDevice;
        void* pfnGetInstanceProcAddr;
    };

    struct NVPW_InitializeHost_Params
    {
        size_t structSize;
        void* pPriv;
    };

    typedef NVPA_Status(*PFN_NVPW_VK_Profiler_GetRequiredInstanceExtensions)(NVPW_VK_Profiler_GetRequiredInstanceExtensions_Params* pParams);
    typedef NVPA_Status(*PFN_NVPW_VK_Profiler_GetRequiredDeviceExtensions)(NVPW_VK_Profiler_GetRequiredDeviceExtensions_Params* pParams);
    typedef NVPA_Status(*PFN_NVPW_InitializeHost)(NVPW_InitializeHost_Params* pParams);

    struct NvPerfHostApi
    {
        String Path;
        void* Module = nullptr;
        PFN_NVPW_VK_Profiler_GetRequiredInstanceExtensions GetRequiredInstanceExtensions = nullptr;
        PFN_NVPW_VK_Profiler_GetRequiredDeviceExtensions GetRequiredDeviceExtensions = nullptr;
    };

    NvPerfHostApi PerfSDK;
}

bool GPUDeviceVulkan::UsePerfSDK = false;

void GPUDeviceVulkan::PerfSDKInit()
{
    // Check if PerfSDK path has been set and is correct
    if (Platform::GetEnvironmentVariable(TEXT("NVPERF_SDK_PATH"), PerfSDK.Path) || PerfSDK.Path.IsEmpty() || !FileSystem::DirectoryExists(PerfSDK.Path))
        return;

    // The Nsight Perf SDK host library is named 'nvperf_grfx_host.dll' (see nvperf_host_impl.h). Older/other
    // packages used 'nvperf.dll'. Try the modern name first, then fall back, so the engine can query the
    // Range Profiler's REQUIRED Vulkan instance/device extensions at device creation. Without these the
    // VkDevice is created with only the mini-trace fallback extensions and NVPW_VK_Profiler_Queue_BeginSession
    // fails with NVPA_STATUS_INVALID_CONTEXT_STATE.
    const String binDir = PerfSDK.Path / TEXT("NvPerf") / TEXT("bin") / TEXT("x64");
    const Char* dllNames[] = { TEXT("nvperf_grfx_host.dll"), TEXT("nvperf.dll") };
    String loadedPath;
    for (const Char* dllName : dllNames)
    {
        const String dllPath = binDir / dllName;
        PerfSDK.Module = Platform::LoadLibrary(dllPath.Get());
        if (PerfSDK.Module)
        {
            loadedPath = dllPath;
            break;
        }
    }
    if (!PerfSDK.Module)
    {
        LOG(Warning, "Nsight PerfSDK: failed to load NvPerf host library from '{0}' (tried nvperf_grfx_host.dll and nvperf.dll)", binDir);
        return;
    }

    // Initialize host library
    auto initializeHost = (PFN_NVPW_InitializeHost)Platform::GetProcAddress(PerfSDK.Module, "NVPW_InitializeHost");
    if (initializeHost)
    {
        NVPW_InitializeHost_Params initParams = {};
        initParams.structSize = sizeof(NVPW_InitializeHost_Params);
        const NVPA_Status initStatus = initializeHost(&initParams);
        if (initStatus != 0)
        {
            LOG(Warning, "Nsight PerfSDK: NVPW_InitializeHost failed (status={0})", (int32)initStatus);
            return;
        }
    }
    else
    {
        LOG(Warning, "Nsight PerfSDK: NVPW_InitializeHost not found in host library");
        return;
    }
    PerfSDK.GetRequiredInstanceExtensions = (PFN_NVPW_VK_Profiler_GetRequiredInstanceExtensions)Platform::GetProcAddress(PerfSDK.Module, "NVPW_VK_Profiler_GetRequiredInstanceExtensions");
    PerfSDK.GetRequiredDeviceExtensions = (PFN_NVPW_VK_Profiler_GetRequiredDeviceExtensions)Platform::GetProcAddress(PerfSDK.Module, "NVPW_VK_Profiler_GetRequiredDeviceExtensions");
    LOG(Info, "Nsight PerfSDK: loaded NvPerf host library '{0}' (GetRequiredInstanceExtensions={1}, GetRequiredDeviceExtensions={2}).", loadedPath, PerfSDK.GetRequiredInstanceExtensions ? 1 : 0, PerfSDK.GetRequiredDeviceExtensions ? 1 : 0);

    // Inform PerfSDK plugin that the Vulkan backend has been setup to use it
    Platform::SetEnvironmentVariable(TEXT("VULKAN_USE_PERF_SDK"), TEXT("*"));
    UsePerfSDK = true;

    // The Vulkan loader reads VK_LOADER_LAYERS_DISABLE / VK_LOADER_DRIVERS_DISABLE exactly once, when it
    // initializes its global config (the first call into vulkan-1.dll, i.e. volkInitialize). Setting them later
    // (e.g. during instance-extension enumeration) is silently ignored. NvPerf's Vulkan Range Profiler is not
    // compatible with object-wrapping layers, so the implicit capture/overlay/Optimus layers must be gone from
    // the device dispatch chain before the loader caches its config, otherwise BeginSession fails with
    // NVPA_STATUS_INVALID_CONTEXT_STATE. Likewise the non-NVIDIA ICD must be dropped so there is no multi-ICD
    // trampoline. Always force these values when PerfSDK is active: a partial user-provided
    // VK_LOADER_LAYERS_DISABLE (e.g. ~implicit~) still leaves validation/capture wrappers and breaks BeginSession.
    String existingLayersDisable;
    Platform::GetEnvironmentVariable(TEXT("VK_LOADER_LAYERS_DISABLE"), existingLayersDisable);
    if (!existingLayersDisable.IsEmpty() && existingLayersDisable != TEXT("*"))
        LOG(Warning, "Nsight PerfSDK: overriding VK_LOADER_LAYERS_DISABLE='{0}' with '*' for Range Profiler.", existingLayersDisable);
    Platform::SetEnvironmentVariable(TEXT("VK_LOADER_LAYERS_DISABLE"), TEXT("*"));
#if PLATFORM_WINDOWS
    _wputenv_s(TEXT("VK_LOADER_LAYERS_DISABLE"), TEXT("*"));
#endif
    LOG(Info, "Nsight PerfSDK: disabled all Vulkan layers before loader init (VK_LOADER_LAYERS_DISABLE=*).");

    // Intel's ICD manifest is igvk64.json (does NOT contain "intel" in the filename), so *intel* alone is not enough.
    const Char* disablePatterns = TEXT("*igvk*,*igd*,*intel*,*Intel*,*amd*,*amdvlk*,*radeon*");
    String existingDriversDisable;
    Platform::GetEnvironmentVariable(TEXT("VK_LOADER_DRIVERS_DISABLE"), existingDriversDisable);
    if (!existingDriversDisable.IsEmpty() && existingDriversDisable != disablePatterns)
        LOG(Warning, "Nsight PerfSDK: overriding VK_LOADER_DRIVERS_DISABLE for Range Profiler.");
    Platform::SetEnvironmentVariable(TEXT("VK_LOADER_DRIVERS_DISABLE"), disablePatterns);
#if PLATFORM_WINDOWS
    _wputenv_s(TEXT("VK_LOADER_DRIVERS_DISABLE"), disablePatterns);
#endif
    LOG(Info, "Nsight PerfSDK: disabled non-NVIDIA Vulkan ICDs before loader init (VK_LOADER_DRIVERS_DISABLE={0}).", String(disablePatterns));
}

void GPUDeviceVulkan::PerfSDKInitDeviceInfo(VkDeviceCreateInfo& deviceInfo, const Array<const char*>& deviceExtensions, VkPhysicalDeviceFeatures2* enabledFeatures2)
{
    // Chains required feature structs onto VkDeviceCreateInfo for enabled PerfSDK extensions
    const uint32 deviceApiMajorMinor = VK_MAKE_VERSION(VK_VERSION_MAJOR(Adapter->GpuProps.apiVersion), VK_VERSION_MINOR(Adapter->GpuProps.apiVersion), 0);
    if (deviceApiMajorMinor >= VK_API_VERSION_1_2)
    {
        void* next = const_cast<void*>(deviceInfo.pNext);
        PhysicalDeviceFeatures12.bufferDeviceAddress = VK_TRUE;
        PhysicalDeviceFeatures12.pNext = next;

        if (enabledFeatures2)
        {
            enabledFeatures2->pNext = &PhysicalDeviceFeatures12;
            deviceInfo.pNext = enabledFeatures2;
            deviceInfo.pEnabledFeatures = nullptr;
        }
        else
        {
            deviceInfo.pNext = &PhysicalDeviceFeatures12;
        }

        LOG(Info, "Nsight PerfSDK: enabled Vulkan bufferDeviceAddress feature.");
        return;
    }

    if (ListContains(deviceExtensions, "VK_EXT_buffer_device_address"))
    {
        static VkPhysicalDeviceBufferDeviceAddressFeaturesEXT bufferDeviceAddressFeatures;
        void* next = const_cast<void*>(deviceInfo.pNext);
        bufferDeviceAddressFeatures = {};
        bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT;
        bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
        bufferDeviceAddressFeatures.pNext = next;
        deviceInfo.pNext = &bufferDeviceAddressFeatures;
        LOG(Info, "Nsight PerfSDK: enabled VK_EXT_buffer_device_address feature.");
    }
}

#endif

void GPUDeviceVulkan::GetInstanceLayersAndExtensions(Array<const char*>& outInstanceExtensions, Array<const char*>& outInstanceLayers, bool& outDebugUtils, bool useDebugLayer)
{
    VkResult result;
    outDebugUtils = false;

    Array<LayerExtension> globalLayerExtensions;
    globalLayerExtensions.AddDefault(1);
    EnumerateInstanceExtensionProperties(nullptr, globalLayerExtensions[0]);

    Array<StringAnsi> foundUniqueExtensions;
    Array<StringAnsi> foundUniqueLayers;
    for (int32 i = 0; i < globalLayerExtensions[0].Extensions.Count(); i++)
    {
        foundUniqueExtensions.AddUnique(StringAnsi(globalLayerExtensions[0].Extensions[i].extensionName));
    }

    {
        Array<VkLayerProperties> globalLayerProperties;
        do
        {
            uint32 instanceLayerCount = 0;
            result = vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
            ASSERT(result >= VK_SUCCESS);

            if (instanceLayerCount > 0)
            {
                globalLayerProperties.AddZeroed(instanceLayerCount);
                result = vkEnumerateInstanceLayerProperties(&instanceLayerCount, &globalLayerProperties[globalLayerProperties.Count() - instanceLayerCount]);
                ASSERT(result >= VK_SUCCESS);
            }
        } while (result == VK_INCOMPLETE);

        globalLayerExtensions.AddDefault(globalLayerProperties.Count());
        for (int32 i = 0; i < globalLayerProperties.Count(); i++)
        {
            LayerExtension* layer = &globalLayerExtensions[i + 1];
            auto& prop = globalLayerProperties[i];
            layer->Layer = prop;
            EnumerateInstanceExtensionProperties(prop.layerName, *layer);
            layer->GetExtensions(foundUniqueExtensions);
            foundUniqueLayers.AddUnique(StringAnsi(prop.layerName));
        }
    }

    if (foundUniqueLayers.HasItems())
    {
        LOG(Info, "Found instance layers:");
        Sorting::QuickSort(foundUniqueLayers);
        for (const StringAnsi& name : foundUniqueLayers)
        {
            LOG(Info, "- {0}", String(name));
        }
    }

    if (foundUniqueExtensions.HasItems())
    {
        LOG(Info, "Found instance extensions:");
        Sorting::QuickSort(foundUniqueExtensions);
        for (const StringAnsi& name : foundUniqueExtensions)
        {
            LOG(Info, "- {0}", String(name));
        }
    }

    bool vkTrace = false;
#if VULKAN_USE_DEBUG_LAYER
    if (!useDebugLayer)
        ValidationLevel = VulkanValidationLevel::Disabled;
    const bool useVkTrace = false;

    if (useVkTrace && useDebugLayer)
    {
        const char* VkTraceName = "VK_LAYER_LUNARG_vktrace";
        if (ContainsLayer(globalLayerExtensions, VkTraceName))
        {
            outInstanceLayers.Add(VkTraceName);
            vkTrace = true;
        }
    }

#if VULKAN_ENABLE_API_DUMP
	if (!vkTrace && useDebugLayer)
	{
		const char* VkApiDumpName = "VK_LAYER_LUNARG_api_dump";
		if (FindLayerInList(globalLayerExtensions, VkApiDumpName))
		{
			outInstanceLayers.Add(VkApiDumpName);
		}
		else
		{
			LOG(Warning, "Unable to find Vulkan instance layer {0}", String(VkApiDumpName));
		}
	}
#endif

    if (!vkTrace && ValidationLevel != VulkanValidationLevel::Disabled)
    {
        bool hasKhronosStandardValidationLayer = false, hasLunargStandardValidationLayer = false;
#if VULKAN_USE_KHRONOS_STANDARD_VALIDATION
        const char* vkLayerKhronosValidation = "VK_LAYER_KHRONOS_validation";
        hasKhronosStandardValidationLayer = ContainsLayer(globalLayerExtensions, vkLayerKhronosValidation);
        if (hasKhronosStandardValidationLayer)
        {
            outInstanceLayers.Add(vkLayerKhronosValidation);
        }
        else
        {
            LOG(Warning, "Unable to find Vulkan instance validation layer {0}", String(vkLayerKhronosValidation));
        }
#endif
#if VULKAN_USE_LUNARG_STANDARD_VALIDATION
        if (!hasKhronosStandardValidationLayer)
        {
            const char* vkLayerLunargStandardValidation = "VK_LAYER_LUNARG_standard_validation";
            hasLunargStandardValidationLayer = ContainsLayer(globalLayerExtensions, vkLayerLunargStandardValidation);
            if (hasLunargStandardValidationLayer)
            {
                outInstanceLayers.Add(vkLayerLunargStandardValidation);
            }
            else
            {
                LOG(Warning, "Unable to find Vulkan instance validation layer {0}", String(vkLayerLunargStandardValidation));
            }
        }
#endif
        if (!hasKhronosStandardValidationLayer && !hasLunargStandardValidationLayer)
        {
            for (uint32 i = 0; GValidationLayers[i] != nullptr; i++)
            {
                const char* validationLayer = GValidationLayers[i];
                const bool validationFound = ContainsLayer(globalLayerExtensions, validationLayer);
                if (validationFound)
                {
                    outInstanceLayers.Add(validationLayer);
                }
                else
                {
                    LOG(Warning, "Unable to find Vulkan instance validation layer {0}", String(validationLayer));
                }
            }
        }
    }

#if VK_EXT_debug_utils
    if (!vkTrace && ValidationLevel != VulkanValidationLevel::Disabled)
    {
        const char* foundDebugUtilsLayer = nullptr;
        outDebugUtils = FindLayerExtension(globalLayerExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME, foundDebugUtilsLayer);
        if (outDebugUtils && *foundDebugUtilsLayer)
        {
            outInstanceLayers.Add(foundDebugUtilsLayer);
        }
    }
#endif
#endif

    Array<const char*> platformExtensions;
    VulkanPlatform::GetInstanceExtensions(platformExtensions, outInstanceLayers);

    for (const char* extension : platformExtensions)
    {
        if (FindLayerExtension(globalLayerExtensions, extension))
        {
            outInstanceExtensions.Add(extension);
        }
    }

    for (int32 i = 0; GInstanceExtensions[i] != nullptr; i++)
    {
        if (FindLayerExtension(globalLayerExtensions, GInstanceExtensions[i]))
        {
            outInstanceExtensions.Add(GInstanceExtensions[i]);
        }
    }

#if VK_EXT_debug_utils
    if (!vkTrace && FindLayerExtension(globalLayerExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
    {
        outInstanceExtensions.Add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
#endif
    if (!vkTrace && ValidationLevel == VulkanValidationLevel::Disabled)
    {
        if (FindLayerExtension(globalLayerExtensions, VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
        {
            outInstanceExtensions.Add(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }
    }

#if VULKAN_USE_PERF_SDK
    if (UsePerfSDK && PerfSDK.GetRequiredInstanceExtensions)
    {
        NVPW_VK_Profiler_GetRequiredInstanceExtensions_Params params = {};
        params.structSize = sizeof(NVPW_VK_Profiler_GetRequiredInstanceExtensions_Params);
        params.apiVersion = VULKAN_API_VERSION;
        const NVPA_Status status = PerfSDK.GetRequiredInstanceExtensions(&params);
        if (status == 0)
        {
            for (size_t i = 0; i < params.numInstanceExtensionNames; i++)
            {
                const char* name = params.ppInstanceExtensionNames[i];
                const bool available = ListContains(foundUniqueExtensions, name);
                LOG(Info, "Nsight PerfSDK: Range Profiler requires instance extension '{0}' (available={1}).", String(name), available ? 1 : 0);
                if (available)
                    outInstanceExtensions.Add(name);
            }
        }
        else
            LOG(Warning, "Nsight PerfSDK: NVPW_VK_Profiler_GetRequiredInstanceExtensions failed (status={0}); instance will be missing Range Profiler extensions.", (int32)status);
    }
#endif

    TrimDuplicates(outInstanceLayers);
    if (outInstanceLayers.HasItems())
    {
        LOG(Info, "Using instance layers:");
        for (const char* layer : outInstanceLayers)
        {
            LOG(Info, "- {0}", String(layer));
        }
    }
    else
    {
        LOG(Info, "Not using instance layers");
    }

    TrimDuplicates(outInstanceExtensions);
    if (outInstanceExtensions.HasItems())
    {
        LOG(Info, "Using instance extensions:");
        for (const char* extension : outInstanceExtensions)
        {
            LOG(Info, "- {0}", String(extension));
        }
    }
    else
    {
        LOG(Info, "Not using instance extensions");
    }
}

void GPUDeviceVulkan::GetDeviceExtensions(VkPhysicalDevice gpu, Array<const char*>& outDeviceExtensions)
{
    Array<LayerExtension> deviceLayerExtensions;
    deviceLayerExtensions.AddDefault(1);
    {
        uint32 count = 0;
        Array<VkLayerProperties> properties;
        VALIDATE_VULKAN_RESULT(vkEnumerateDeviceLayerProperties(gpu, &count, nullptr));
        properties.AddZeroed(count);
        VALIDATE_VULKAN_RESULT(vkEnumerateDeviceLayerProperties(gpu, &count, properties.Get()));
        ASSERT(count == properties.Count());
        for (const VkLayerProperties& property : properties)
        {
            deviceLayerExtensions.AddDefault(1);
            deviceLayerExtensions.Last().Layer = property;
        }
    }

    Array<StringAnsi> foundUniqueLayers;
    Array<StringAnsi> foundUniqueExtensions;

    for (int32 i = 0; i < deviceLayerExtensions.Count(); i++)
    {
        if (i == 0)
        {
            EnumerateDeviceExtensionProperties(gpu, nullptr, deviceLayerExtensions[i]);
        }
        else
        {
            foundUniqueLayers.AddUnique(StringAnsi(deviceLayerExtensions[i].Layer.layerName));
            EnumerateDeviceExtensionProperties(gpu, deviceLayerExtensions[i].Layer.layerName, deviceLayerExtensions[i]);
        }

        deviceLayerExtensions[i].GetExtensions(foundUniqueExtensions);
    }

    if (foundUniqueLayers.HasItems())
    {
        LOG(Info, "Found device layers:");
        Sorting::QuickSort(foundUniqueLayers);
        for (const StringAnsi& name : foundUniqueLayers)
        {
            LOG(Info, "- {0}", String(name));
        }
    }

    if (foundUniqueExtensions.HasItems())
    {
        LOG(Info, "Found device extensions:");
        Sorting::QuickSort(foundUniqueExtensions);
        for (const StringAnsi& name : foundUniqueExtensions)
        {
            LOG(Info, "- {0}", String(name));
        }
    }

    // Add device layers for debugging
    if (ListContains(foundUniqueLayers, "VK_LAYER_RENDERDOC_Capture"))
    {
        IsDebugToolAttached = true;
    }

    // Find all extensions
    Array<const char*> availableExtensions;
    availableExtensions.Resize(deviceLayerExtensions[0].Extensions.Count());
    for (int32 i = 0; i < deviceLayerExtensions[0].Extensions.Count(); i++)
    {
        availableExtensions[i] = deviceLayerExtensions[0].Extensions[i].extensionName;
    }
    TrimDuplicates(availableExtensions);

    // Pick extensions to use
    Array<const char*> platformExtensions;
    VulkanPlatform::GetDeviceExtensions(platformExtensions);
    for (const char* extension : platformExtensions)
    {
        if (ListContains(availableExtensions, extension))
        {
            outDeviceExtensions.Add(extension);
            break;
        }
    }
    for (uint32 i = 0; i < ARRAY_COUNT(GDeviceExtensions) && GDeviceExtensions[i] != nullptr; i++)
    {
        if (ListContains(availableExtensions, GDeviceExtensions[i]))
        {
            outDeviceExtensions.Add(GDeviceExtensions[i]);
        }
    }

#if defined(VK_KHR_acceleration_structure) && defined(VK_KHR_ray_query)
    static const char* rayTracingExtensions[] =
    {
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_QUERY_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    };
    bool hasAllRayTracingExtensions = true;
    for (const char* extension : rayTracingExtensions)
    {
        if (!ListContains(availableExtensions, extension))
        {
            hasAllRayTracingExtensions = false;
            break;
        }
    }
    if (hasAllRayTracingExtensions)
    {
        for (const char* extension : rayTracingExtensions)
            outDeviceExtensions.Add(extension);
    }
#endif

#if VULKAN_USE_PERF_SDK
    if (UsePerfSDK && PerfSDK.GetRequiredDeviceExtensions && Adapter->IsNVIDIA())
    {
        NVPW_VK_Profiler_GetRequiredDeviceExtensions_Params params = {};
        params.structSize = sizeof(NVPW_VK_Profiler_GetRequiredDeviceExtensions_Params);
        params.apiVersion = Adapter->GpuProps.apiVersion;
        params.instance = Instance;
        params.physicalDevice = gpu;
        params.pfnGetInstanceProcAddr = (void*)vkGetInstanceProcAddr;
        const NVPA_Status status = PerfSDK.GetRequiredDeviceExtensions(&params);
        if (status == 0)
        {
            for (size_t i = 0; i < params.numDeviceExtensionNames; i++)
            {
                const char* name = params.ppDeviceExtensionNames[i];
                const bool available = ListContains(availableExtensions, name);
                LOG(Info, "Nsight PerfSDK: Range Profiler requires device extension '{0}' (available={1}).", String(name), available ? 1 : 0);
                if (available)
                    outDeviceExtensions.Add(name);
            }
        }
        else
            LOG(Warning, "Nsight PerfSDK: NVPW_VK_Profiler_GetRequiredDeviceExtensions failed (status={0}); device will be missing Range Profiler extensions.", (int32)status);
    }
#endif

    if (outDeviceExtensions.HasItems())
    {
        LOG(Info, "Using device extensions:");
        for (const char* extension : outDeviceExtensions)
        {
            LOG(Info, "- {0}", String(extension));
        }
    }
}

void GPUDeviceVulkan::ParseOptionalDeviceExtensions(const Array<const char*>& deviceExtensions)
{
    Platform::MemoryClear(&OptionalDeviceExtensions, sizeof(OptionalDeviceExtensions));
#if VK_KHR_maintenance1
    OptionalDeviceExtensions.HasKHRMaintenance1 = RenderToolsVulkan::HasExtension(deviceExtensions, VK_KHR_MAINTENANCE1_EXTENSION_NAME);
#endif
#if VK_KHR_maintenance2
    OptionalDeviceExtensions.HasKHRMaintenance2 = RenderToolsVulkan::HasExtension(deviceExtensions, VK_KHR_MAINTENANCE2_EXTENSION_NAME);
#endif
    OptionalDeviceExtensions.HasMirrorClampToEdge = RenderToolsVulkan::HasExtension(deviceExtensions, VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME);
#if defined(VK_KHR_acceleration_structure) && defined(VK_KHR_ray_query)
    OptionalDeviceExtensions.HasRayTracingExtensions =
        RenderToolsVulkan::HasExtension(deviceExtensions, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) &&
        RenderToolsVulkan::HasExtension(deviceExtensions, VK_KHR_RAY_QUERY_EXTENSION_NAME) &&
        RenderToolsVulkan::HasExtension(deviceExtensions, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) &&
        RenderToolsVulkan::HasExtension(deviceExtensions, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
#endif
#if VULKAN_USE_VALIDATION_CACHE
    OptionalDeviceExtensions.HasEXTValidationCache = RenderToolsVulkan::HasExtension(deviceExtensions, VK_EXT_VALIDATION_CACHE_EXTENSION_NAME);
#endif
}

#endif
