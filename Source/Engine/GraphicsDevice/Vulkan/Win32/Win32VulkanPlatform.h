// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "../VulkanPlatformBase.h"

#if GRAPHICS_API_VULKAN && PLATFORM_WIN32

#define VULKAN_USE_PLATFORM_WIN32_KHR 1
#define VULKAN_USE_PLATFORM_WIN32_KHX 1
#define VULKAN_HAS_PHYSICAL_DEVICE_PROPERTIES2 1
#define VULKAN_USE_CREATE_WIN32_SURFACE 1

/// <summary>
/// The implementation for the Vulkan API support for Win32 platform.
/// </summary>
class Win32VulkanPlatform : public VulkanPlatformBase
{
public:

    static void GetInstanceExtensions(Array<const char*>& extensions);
    static void CreateSurface(void* windowHandle, VkInstance instance, VkSurfaceKHR* surface);
};

typedef Win32VulkanPlatform VulkanPlatform;

#endif
