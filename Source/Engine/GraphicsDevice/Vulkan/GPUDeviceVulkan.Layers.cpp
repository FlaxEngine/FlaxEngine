// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "GPUDeviceVulkan.h"
#include "RenderToolsVulkan.h"
#include "Config.h"
#include "Engine/Core/Collections/ArrayExtensions.h"
#include "Engine/Core/Collections/Sorting.h"

#if GRAPHICS_API_VULKAN

// TODO: expose it as a command line or engine parameter to end-user
#if GPU_ENABLE_DIAGNOSTICS
VulkanValidationLevel ValidationLevel = VulkanValidationLevel::ErrorsAndWarningsPerf;
#else
VulkanValidationLevel ValidationLevel = VulkanValidationLevel::Disabled;
#endif

#if VULKAN_USE_DEBUG_LAYER

// TODO: expose it as a command line or engine parameter to end-user
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
#if VULKAN_ENABLE_DESKTOP_HMD_SUPPORT
	VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
	VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
#endif
#if VULKAN_SUPPORTS_VALIDATION_CACHE
    VK_EXT_VALIDATION_CACHE_EXTENSION_NAME,
#endif
    nullptr
};

static const char* GDeviceExtensions[] =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if VULKAN_SUPPORTS_MAINTENANCE_LAYER1
    VK_KHR_MAINTENANCE1_EXTENSION_NAME,
#endif
#if VULKAN_SUPPORTS_MAINTENANCE_LAYER2
    VK_KHR_MAINTENANCE2_EXTENSION_NAME,
#endif
#if VULKAN_SUPPORTS_VALIDATION_CACHE
    VK_EXT_VALIDATION_CACHE_EXTENSION_NAME,
#endif
    VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME,
    nullptr
};

struct LayerExtension
{
    VkLayerProperties LayerProps;
    Array<VkExtensionProperties> ExtensionProps;

    LayerExtension()
    {
        Platform::MemoryClear(&LayerProps, sizeof(LayerProps));
    }

    void AddUniqueExtensionNames(Array<StringAnsi>& result)
    {
        for (int32 index = 0; index < ExtensionProps.Count(); index++)
        {
            result.AddUnique(StringAnsi(ExtensionProps[index].extensionName));
        }
    }

    void AddAnsiExtensionNames(Array<const char*>& result)
    {
        for (int32 index = 0; index < ExtensionProps.Count(); index++)
        {
            result.AddUnique(ExtensionProps[index].extensionName);
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
            outLayer.ExtensionProps.Clear();
            outLayer.ExtensionProps.AddDefault(count);
            result = vkEnumerateInstanceExtensionProperties(layerName, &count, outLayer.ExtensionProps.Get());
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
            outLayer.ExtensionProps.Clear();
            outLayer.ExtensionProps.AddDefault(count);
            result = vkEnumerateDeviceExtensionProperties(device, layerName, &count, outLayer.ExtensionProps.Get());
            ASSERT(result >= VK_SUCCESS);
        }
    } while (result == VK_INCOMPLETE);
}

static void TrimDuplicates(Array<const char*>& array)
{
    for (int32 outerIndex = array.Count() - 1; outerIndex >= 0; --outerIndex)
    {
        bool found = false;
        for (int32 innerIndex = outerIndex - 1; innerIndex >= 0; --innerIndex)
        {
            if (!StringUtils::Compare(array[outerIndex], array[innerIndex]))
            {
                found = true;
                break;
            }
        }

        if (found)
        {
            array.RemoveAt(outerIndex);
        }
    }
}

static inline int FindLayerIndexInList(const Array<LayerExtension>& list, const char* layerName)
{
    // 0 is reserved for NULL/instance
    for (int32 i = 1; i < list.Count(); i++)
    {
        if (!StringUtils::Compare(list[i].LayerProps.layerName, layerName))
        {
            return i;
        }
    }
    return INVALID_INDEX;
}

static bool FindLayerInList(const Array<LayerExtension>& list, const char* layerName)
{
    return FindLayerIndexInList(list, layerName) != INVALID_INDEX;
}

static bool FindLayerExtensionInList(const Array<LayerExtension>& list, const char* extensionName, const char*& foundLayer)
{
    for (int32 extIndex = 0; extIndex < list.Count(); extIndex++)
    {
        for (int32 i = 0; i < list[extIndex].ExtensionProps.Count(); i++)
        {
            if (!StringUtils::Compare(list[extIndex].ExtensionProps[i].extensionName, extensionName))
            {
                foundLayer = list[extIndex].LayerProps.layerName;
                return true;
            }
        }
    }
    return false;
}

static bool FindLayerExtensionInList(const Array<LayerExtension>& list, const char* extensionName)
{
    const char* dummy = nullptr;
    return FindLayerExtensionInList(list, extensionName, dummy);
}

void GPUDeviceVulkan::GetInstanceLayersAndExtensions(Array<const char*>& outInstanceExtensions, Array<const char*>& outInstanceLayers, bool& outDebugUtils)
{
    VkResult result;
    outDebugUtils = false;

    Array<LayerExtension> globalLayerExtensions;
    // 0 is reserved for NULL/instance
    globalLayerExtensions.AddDefault(1);

    // Global extensions
    EnumerateInstanceExtensionProperties(nullptr, globalLayerExtensions[0]);

    Array<StringAnsi> foundUniqueExtensions;
    Array<StringAnsi> foundUniqueLayers;
    for (int32 i = 0; i < globalLayerExtensions[0].ExtensionProps.Count(); i++)
    {
        foundUniqueExtensions.AddUnique(StringAnsi(globalLayerExtensions[0].ExtensionProps[i].extensionName));
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
            layer->LayerProps = prop;
            EnumerateInstanceExtensionProperties(prop.layerName, *layer);
            layer->AddUniqueExtensionNames(foundUniqueExtensions);
            foundUniqueLayers.AddUnique(StringAnsi(prop.layerName));
        }
    }

    if (foundUniqueLayers.HasItems())
    {
        LOG(Info, "Found instance layers:");
        Sorting::QuickSort(foundUniqueLayers.Get(), foundUniqueLayers.Count());
        for (const StringAnsi& name : foundUniqueLayers)
        {
            LOG(Info, "- {0}", String(name));
        }
    }

    if (foundUniqueExtensions.HasItems())
    {
        LOG(Info, "Found instance extensions:");
        Sorting::QuickSort(foundUniqueExtensions.Get(), foundUniqueExtensions.Count());
        for (const StringAnsi& name : foundUniqueExtensions)
        {
            LOG(Info, "- {0}", String(name));
        }
    }

    VulkanPlatform::NotifyFoundInstanceLayersAndExtensions(foundUniqueLayers, foundUniqueExtensions);

    // TODO: expose as a command line parameter or sth
    const bool useVkTrace = false;
    bool vkTrace = false;
    if (useVkTrace)
    {
        const char* VkTraceName = "VK_LAYER_LUNARG_vktrace";
        if (FindLayerInList(globalLayerExtensions, VkTraceName))
        {
            outInstanceLayers.Add(VkTraceName);
            vkTrace = true;
        }
    }

#if VULKAN_USE_DEBUG_LAYER
#if VULKAN_ENABLE_API_DUMP
	if (!vkTrace)
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
        hasKhronosStandardValidationLayer = FindLayerInList(globalLayerExtensions, vkLayerKhronosValidation);
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
            hasLunargStandardValidationLayer = FindLayerInList(globalLayerExtensions, vkLayerLunargStandardValidation);
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
                const char* currValidationLayer = GValidationLayers[i];
                const bool validationFound = FindLayerInList(globalLayerExtensions, currValidationLayer);
                if (validationFound)
                {
                    outInstanceLayers.Add(currValidationLayer);
                }
                else
                {
                    LOG(Warning, "Unable to find Vulkan instance validation layer {0}", String(currValidationLayer));
                }
            }
        }
    }

#if VULKAN_SUPPORTS_DEBUG_UTILS
    if (!vkTrace && ValidationLevel != VulkanValidationLevel::Disabled)
    {
        const char* foundDebugUtilsLayer = nullptr;
        outDebugUtils = FindLayerExtensionInList(globalLayerExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME, foundDebugUtilsLayer);
        if (outDebugUtils && *foundDebugUtilsLayer)
        {
            outInstanceLayers.Add(foundDebugUtilsLayer);
        }
    }
#endif
#endif

    Array<const char*> platformExtensions;
    VulkanPlatform::GetInstanceExtensions(platformExtensions);

    for (const char* extension : platformExtensions)
    {
        if (FindLayerExtensionInList(globalLayerExtensions, extension))
        {
            outInstanceExtensions.Add(extension);
        }
    }

    for (int32 i = 0; GInstanceExtensions[i] != nullptr; i++)
    {
        if (FindLayerExtensionInList(globalLayerExtensions, GInstanceExtensions[i]))
        {
            outInstanceExtensions.Add(GInstanceExtensions[i]);
        }
    }

#if VULKAN_SUPPORTS_DEBUG_UTILS
    if (!vkTrace && outDebugUtils && FindLayerExtensionInList(globalLayerExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
    {
        outInstanceExtensions.Add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
#endif
    if (!vkTrace && ValidationLevel == VulkanValidationLevel::Disabled)
    {
        if (FindLayerExtensionInList(globalLayerExtensions, VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
        {
            outInstanceExtensions.Add(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }
    }

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

void GPUDeviceVulkan::GetDeviceExtensionsAndLayers(VkPhysicalDevice gpu, Array<const char*>& outDeviceExtensions, Array<const char*>& outDeviceLayers)
{
    Array<LayerExtension> deviceLayerExtensions;
    // 0 is reserved for regular device
    deviceLayerExtensions.AddDefault(1);
    {
        uint32 count = 0;
        Array<VkLayerProperties> properties;
        VALIDATE_VULKAN_RESULT(vkEnumerateDeviceLayerProperties(gpu, &count, nullptr));
        properties.AddZeroed(count);
        VALIDATE_VULKAN_RESULT(vkEnumerateDeviceLayerProperties(gpu, &count, properties.Get()));
        ASSERT(count == properties.Count());
        for (const VkLayerProperties& Property : properties)
        {
            deviceLayerExtensions.AddDefault(1);
            deviceLayerExtensions.Last().LayerProps = Property;
        }
    }

    Array<StringAnsi> foundUniqueLayers;
    Array<StringAnsi> foundUniqueExtensions;

    for (int32 index = 0; index < deviceLayerExtensions.Count(); index++)
    {
        if (index == 0)
        {
            EnumerateDeviceExtensionProperties(gpu, nullptr, deviceLayerExtensions[index]);
        }
        else
        {
            foundUniqueLayers.AddUnique(StringAnsi(deviceLayerExtensions[index].LayerProps.layerName));
            EnumerateDeviceExtensionProperties(gpu, deviceLayerExtensions[index].LayerProps.layerName, deviceLayerExtensions[index]);
        }

        deviceLayerExtensions[index].AddUniqueExtensionNames(foundUniqueExtensions);
    }

    if (foundUniqueLayers.HasItems())
    {
        LOG(Info, "Found device layers:");
        Sorting::QuickSort(foundUniqueLayers.Get(), foundUniqueLayers.Count());
        for (const StringAnsi& name : foundUniqueLayers)
        {
            LOG(Info, "- {0}", String(name));
        }
    }

    if (foundUniqueExtensions.HasItems())
    {
        LOG(Info, "Found device extensions:");
        Sorting::QuickSort(foundUniqueExtensions.Get(), foundUniqueExtensions.Count());
        for (const StringAnsi& name : foundUniqueExtensions)
        {
            LOG(Info, "- {0}", String(name));
        }
    }

    VulkanPlatform::NotifyFoundDeviceLayersAndExtensions(gpu, foundUniqueLayers, foundUniqueExtensions);

    // Add device layers for debugging
#if VULKAN_USE_DEBUG_LAYER
    bool hasKhronosStandardValidationLayer = false, hasLunargStandardValidationLayer = false;
#if VULKAN_USE_KHRONOS_STANDARD_VALIDATION
    const char* vkLayerKhronosValidation = "VK_LAYER_KHRONOS_validation";
    hasKhronosStandardValidationLayer = FindLayerInList(deviceLayerExtensions, vkLayerKhronosValidation);
    if (hasKhronosStandardValidationLayer)
    {
        outDeviceLayers.Add(vkLayerKhronosValidation);
    }
#endif
#if VULKAN_USE_LUNARG_STANDARD_VALIDATION
    if (!hasKhronosStandardValidationLayer)
    {
        const char* vkLayerLunargStandardValidation = "VK_LAYER_LUNARG_standard_validation";
        hasLunargStandardValidationLayer = FindLayerInList(deviceLayerExtensions, vkLayerLunargStandardValidation);
        if (hasLunargStandardValidationLayer)
        {
            outDeviceLayers.Add(vkLayerLunargStandardValidation);
        }
    }
#endif
    if (!hasKhronosStandardValidationLayer && !hasLunargStandardValidationLayer)
    {
        for (uint32 i = 0; GValidationLayers[i] != nullptr; i++)
        {
            const char* validationLayer = GValidationLayers[i];
            if (FindLayerInList(deviceLayerExtensions, validationLayer))
            {
                outDeviceLayers.Add(validationLayer);
            }
        }
    }
#endif

    // Now gather the actually used extensions based on the enabled layers
    Array<const char*> availableExtensions;
    {
        // All global
        for (int32 i = 0; i < deviceLayerExtensions[0].ExtensionProps.Count(); i++)
        {
            availableExtensions.Add(deviceLayerExtensions[0].ExtensionProps[i].extensionName);
        }

        // Now only find enabled layers
        for (int32 layerIndex = 0; layerIndex < outDeviceLayers.Count(); layerIndex++)
        {
            // Skip 0 as it's the null layer
            int32 findLayerIndex;
            for (findLayerIndex = 1; findLayerIndex < deviceLayerExtensions.Count(); findLayerIndex++)
            {
                if (!StringUtils::Compare(deviceLayerExtensions[findLayerIndex].LayerProps.layerName, outDeviceLayers[layerIndex]))
                {
                    break;
                }
            }

            if (findLayerIndex < deviceLayerExtensions.Count())
            {
                deviceLayerExtensions[findLayerIndex].AddAnsiExtensionNames(availableExtensions);
            }
        }
    }
    TrimDuplicates(availableExtensions);

    const auto ListContains = [](const Array<const char*>& list, const char* name)
    {
        for (const char* element : list)
        {
            if (!StringUtils::Compare(element, name))
            {
                return true;
            }
        }

        return false;
    };

    // Now go through the actual requested lists
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

    if (outDeviceExtensions.HasItems())
    {
        LOG(Info, "Using device extensions:");
        for (const char* extension : outDeviceExtensions)
        {
            LOG(Info, "- {0}", String(extension));
        }
    }

    if (outDeviceLayers.HasItems())
    {
        LOG(Info, "Using device layers:");
        for (const char* layer : outDeviceLayers)
        {
            LOG(Info, "- {0}", String(layer));
        }
    }
}

void GPUDeviceVulkan::ParseOptionalDeviceExtensions(const Array<const char*>& deviceExtensions)
{
    Platform::MemoryClear(&OptionalDeviceExtensions, sizeof(OptionalDeviceExtensions));

    const auto HasExtension = [&deviceExtensions](const char* name) -> bool
    {
        const std::function<bool(const char* const&)> CheckCallback = [&name](const char* const& extension) -> bool
        {
            return StringUtils::Compare(extension, name) == 0;
        };
        return ArrayExtensions::Any(deviceExtensions, CheckCallback);
    };

#if VULKAN_SUPPORTS_MAINTENANCE_LAYER1
    OptionalDeviceExtensions.HasKHRMaintenance1 = HasExtension(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
#endif
#if VULKAN_SUPPORTS_MAINTENANCE_LAYER2
    OptionalDeviceExtensions.HasKHRMaintenance2 = HasExtension(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
#endif
    OptionalDeviceExtensions.HasMirrorClampToEdge = HasExtension(VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME);
#if VULKAN_ENABLE_DESKTOP_HMD_SUPPORT
	OptionalDeviceExtensions.HasKHRExternalMemoryCapabilities = HasExtension(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
	OptionalDeviceExtensions.HasKHRGetPhysicalDeviceProperties2 = HasExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif
#if VULKAN_SUPPORTS_VALIDATION_CACHE
    OptionalDeviceExtensions.HasEXTValidationCache = HasExtension(VK_EXT_VALIDATION_CACHE_EXTENSION_NAME);
#endif
}

#endif
