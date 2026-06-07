// Copyright (c) Flax Engine. NVIDIA PerfSDK Vulkan device setup helpers.

#include "RenderPerfVulkanExtensions.h"

#if COMPILE_WITH_RENDER_PERF_NVPERF && GRAPHICS_API_VULKAN

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef NOGDI
#define NOGDI
#endif

#if PLATFORM_WIN32
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#endif
#include <ThirdParty/volk/volk.h>
#include "Engine/Core/Types/String.h"
#include "Engine/Platform/StringUtils.h"

#include <NvPerfVulkan.h>
#include <NvPerfPeriodicSamplerVulkan.h>
#include <vector>

namespace
{
    static bool ContainsExtension(const Array<const char*>& extensions, const char* name)
    {
        if (!name)
            return false;
        for (int32 i = 0; i < extensions.Count(); i++)
        {
            if (StringUtils::Compare(extensions[i], name) == 0)
                return true;
        }
        return false;
    }

    static void AddUniqueExtension(Array<const char*>& outExtensions, const Array<const char*>& availableExtensions, const char* name)
    {
        if (!name || !ContainsExtension(availableExtensions, name) || ContainsExtension(outExtensions, name))
            return;
        outExtensions.Add(name);
    }

    static void AppendNames(Array<const char*>& outExtensions, const Array<const char*>& availableExtensions, const std::vector<const char*>& names)
    {
        for (const char* name : names)
            AddUniqueExtension(outExtensions, availableExtensions, name);
    }

    static uint32 GetDeviceApiMajorMinor(VkPhysicalDevice physicalDevice)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        return VK_MAKE_VERSION(VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion), 0);
    }
}

void RenderPerfAppendVulkanInstanceExtensions(Array<const char*>& outInstanceExtensions, const Array<const char*>& availableExtensions, uint32 apiVersion)
{
    std::vector<const char*> requiredNames;
    if (!nv::perf::VulkanAppendInstanceRequiredExtensions(requiredNames, apiVersion))
        return;
    AppendNames(outInstanceExtensions, availableExtensions, requiredNames);
}

void RenderPerfAppendVulkanDeviceExtensions(VkInstance instance, VkPhysicalDevice physicalDevice, Array<const char*>& outDeviceExtensions, const Array<const char*>& availableExtensions, uint32 physicalDeviceApiVersion)
{
    std::vector<const char*> requiredNames;

    if (instance && physicalDevice && vkGetPhysicalDeviceProperties)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        if (properties.vendorID == 0x10DE)
        {
            nv::perf::VulkanAppendDeviceRequiredExtensions(instance, physicalDevice, (void*)vkGetInstanceProcAddr, requiredNames);
        }
    }

    // NvPerf MiniTrace only lists extensions for Vulkan <= 1.1. Request them explicitly on
    // newer GPUs too when the driver still exposes the KHR/EXT entry points.
    const uint32_t miniTraceApiVersion = physicalDeviceApiVersion <= VK_API_VERSION_1_1 ? physicalDeviceApiVersion : VK_API_VERSION_1_1;
    nv::perf::sampler::PeriodicSamplerTimeHistoryVulkan::AppendDeviceRequiredExtensions(miniTraceApiVersion, requiredNames);
    AppendNames(outDeviceExtensions, availableExtensions, requiredNames);
}

void RenderPerfChainVulkanDeviceCreateInfo(VkDeviceCreateInfo* deviceCreateInfo, VkPhysicalDevice physicalDevice, const Array<const char*>& enabledDeviceExtensions)
{
    if (!deviceCreateInfo || !physicalDevice)
        return;

    void* next = const_cast<void*>(deviceCreateInfo->pNext);
    const uint32 deviceApiMajorMinor = GetDeviceApiMajorMinor(physicalDevice);

    // Vulkan 1.2+ uses core vkGetBufferDeviceAddress; the feature must be enabled at device creation.
    if (deviceApiMajorMinor >= VK_API_VERSION_1_2)
    {
        static VkPhysicalDeviceVulkan12Features vulkan12Features;
        vulkan12Features = {};
        vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        vulkan12Features.bufferDeviceAddress = VK_TRUE;
        vulkan12Features.pNext = next;
        deviceCreateInfo->pNext = &vulkan12Features;
        return;
    }

    if (ContainsExtension(enabledDeviceExtensions, "VK_EXT_buffer_device_address"))
    {
        static VkPhysicalDeviceBufferDeviceAddressFeaturesEXT bufferDeviceAddressFeatures;
        bufferDeviceAddressFeatures = {};
        bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT;
        bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
        bufferDeviceAddressFeatures.pNext = next;
        deviceCreateInfo->pNext = &bufferDeviceAddressFeatures;
    }
}

#endif
