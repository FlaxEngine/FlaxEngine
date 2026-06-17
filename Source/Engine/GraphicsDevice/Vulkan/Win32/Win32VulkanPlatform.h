// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../VulkanPlatformBase.h"

#if GRAPHICS_API_VULKAN && PLATFORM_WIN32

// Vulkan 1.3 is required so the device accepts SPIR-V 1.6 modules used by the
// NV cooperative-vector neural shading path (SPV_NV_cooperative_vector + VulkanMemoryModel).
#define VULKAN_API_VERSION VK_API_VERSION_1_3
#define VULKAN_USE_PLATFORM_WIN32_KHR 1
#define VULKAN_USE_PLATFORM_WIN32_KHX 1
#define VULKAN_USE_CREATE_WIN32_SURFACE 1

/// <summary>
/// The implementation for the Vulkan API support for Win32 platform.
/// </summary>
class Win32VulkanPlatform : public VulkanPlatformBase
{
public:
    static void GetInstanceExtensions(Array<const char*>& extensions, Array<const char*>& layers);
    static void CreateSurface(Window* window, GPUDeviceVulkan* device, VkInstance instance, VkSurfaceKHR* surface);
};

typedef Win32VulkanPlatform VulkanPlatform;

#endif
