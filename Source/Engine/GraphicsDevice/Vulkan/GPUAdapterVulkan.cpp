// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN

#include "GPUAdapterVulkan.h"
#include "GPUDeviceVulkan.h"
#include "RenderToolsVulkan.h"

GPUAdapterVulkan::GPUAdapterVulkan(VkPhysicalDevice gpu)
    : Gpu(gpu)
{
    // Query device information
    vkGetPhysicalDeviceProperties(gpu, &GpuProps);
#if VULKAN_ENABLE_DESKTOP_HMD_SUPPORT
	if (GPUDeviceVulkan::OptionalDeviceExtensions.HasKHRGetPhysicalDeviceProperties2)
	{
		VkPhysicalDeviceProperties2KHR GpuProps2;
		RenderToolsVulkan::ZeroStruct(GpuProps2, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR);
		GpuProps2.pNext = &GpuProps2;
		RenderToolsVulkan::ZeroStruct(GpuIdProps, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES_KHR);
		vkGetPhysicalDeviceProperties2KHR(Gpu, &GpuProps2);
	}
#endif

    Description = GpuProps.deviceName;
}

#endif
