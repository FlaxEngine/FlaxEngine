// Copyright (c) Wojciech Figat. All rights reserved.

#include "GPUDeviceVulkan.h"
#include "RenderToolsVulkan.h"
#include "Config.h"
#include "Engine/Core/Log.h"
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

void GPUDeviceVulkan::GetInstanceLayersAndExtensions(Array<const char*>& outInstanceExtensions, Array<const char*>& outInstanceLayers, bool& outDebugUtils)
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

    // TODO: expose as a command line parameter or sth
    const bool useVkTrace = false;
    bool vkTrace = false;
    if (useVkTrace)
    {
        const char* VkTraceName = "VK_LAYER_LUNARG_vktrace";
        if (ContainsLayer(globalLayerExtensions, VkTraceName))
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
#ifdef VK_EXT_tooling_info
    if (ListContains(foundUniqueExtensions, "VK_EXT_tooling_info"))
    {
        IsDebugToolAttached = true;
    }
#endif
#if VULKAN_USE_DEBUG_LAYER
    bool hasKhronosStandardValidationLayer = false, hasLunargStandardValidationLayer = false;
#if VULKAN_USE_KHRONOS_STANDARD_VALIDATION
    const char* vkLayerKhronosValidation = "VK_LAYER_KHRONOS_validation";
    hasKhronosStandardValidationLayer = ContainsLayer(deviceLayerExtensions, vkLayerKhronosValidation);
    if (hasKhronosStandardValidationLayer)
    {
        outDeviceLayers.Add(vkLayerKhronosValidation);
    }
#endif
#if VULKAN_USE_LUNARG_STANDARD_VALIDATION
    if (!hasKhronosStandardValidationLayer)
    {
        const char* vkLayerLunargStandardValidation = "VK_LAYER_LUNARG_standard_validation";
        hasLunargStandardValidationLayer = ContainsLayer(deviceLayerExtensions, vkLayerLunargStandardValidation);
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
            if (ContainsLayer(deviceLayerExtensions, validationLayer))
            {
                outDeviceLayers.Add(validationLayer);
            }
        }
    }
#endif

    // Find all extensions
    Array<const char*> availableExtensions;
    {
        for (int32 i = 0; i < deviceLayerExtensions[0].Extensions.Count(); i++)
        {
            availableExtensions.Add(deviceLayerExtensions[0].Extensions[i].extensionName);
        }

        for (int32 layerIndex = 0; layerIndex < outDeviceLayers.Count(); layerIndex++)
        {
            int32 findLayerIndex;
            for (findLayerIndex = 1; findLayerIndex < deviceLayerExtensions.Count(); findLayerIndex++)
            {
                if (!StringUtils::Compare(deviceLayerExtensions[findLayerIndex].Layer.layerName, outDeviceLayers[layerIndex]))
                {
                    break;
                }
            }

            if (findLayerIndex < deviceLayerExtensions.Count())
            {
                deviceLayerExtensions[findLayerIndex].GetExtensions(availableExtensions);
            }
        }
    }
    TrimDuplicates(availableExtensions);

    // Pick extensions to use
    Array<const char*> platformExtensions;
    VulkanPlatform::GetDeviceExtensions(platformExtensions, outDeviceLayers);
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
#if VK_KHR_maintenance1
    OptionalDeviceExtensions.HasKHRMaintenance1 = RenderToolsVulkan::HasExtension(deviceExtensions, VK_KHR_MAINTENANCE1_EXTENSION_NAME);
#endif
#if VK_KHR_maintenance2
    OptionalDeviceExtensions.HasKHRMaintenance2 = RenderToolsVulkan::HasExtension(deviceExtensions, VK_KHR_MAINTENANCE2_EXTENSION_NAME);
#endif
    OptionalDeviceExtensions.HasMirrorClampToEdge = RenderToolsVulkan::HasExtension(deviceExtensions, VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME);
#if VULKAN_USE_VALIDATION_CACHE
    OptionalDeviceExtensions.HasEXTValidationCache = RenderToolsVulkan::HasExtension(deviceExtensions, VK_EXT_VALIDATION_CACHE_EXTENSION_NAME);
#endif
}

#endif
