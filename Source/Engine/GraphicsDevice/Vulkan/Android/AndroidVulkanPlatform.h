// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../VulkanPlatformBase.h"

#if GRAPHICS_API_VULKAN && PLATFORM_ANDROID

// Android has 99.4% of Vulkan 1.1 and API level 29 (Android 10.0)
#define VULKAN_API_VERSION VK_API_VERSION_1_1

// Support more backbuffers in case driver decides to use more
#define VULKAN_BACK_BUFFERS_COUNT_MAX 8

/// <summary>
/// The implementation for the Vulkan API support for Android platform.
/// </summary>
class AndroidVulkanPlatform : public VulkanPlatformBase
{
public:
	static void GetInstanceExtensions(Array<const char*>& extensions, Array<const char*>& layers);
	static void GetDeviceExtensions(Array<const char*>& extensions);
	static void CreateSurface(Window* window, GPUDeviceVulkan* device, VkInstance instance, VkSurfaceKHR* surface);
};

typedef AndroidVulkanPlatform VulkanPlatform;

#endif
