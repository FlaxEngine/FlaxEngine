// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN

#include "GPUAdapterVulkan.h"
#include "GPUDeviceVulkan.h"

GPUAdapterVulkan::GPUAdapterVulkan(VkPhysicalDevice gpu)
    : Gpu(gpu)
{
    vkGetPhysicalDeviceProperties(gpu, &GpuProps);
    Description = GpuProps.deviceName;
}

#endif
