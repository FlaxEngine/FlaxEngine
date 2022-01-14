// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "../VulkanPlatformBase.h"

#if GRAPHICS_API_VULKAN && PLATFORM_MAC

#define VULKAN_BACK_BUFFERS_COUNT 3

/// <summary>
/// The implementation for the Vulkan API support for Mac platform.
/// </summary>
class MacVulkanPlatform : public VulkanPlatformBase
{
public:
	static void GetInstanceExtensions(Array<const char*>& extensions, Array<const char*>& layers);
	static void CreateSurface(void* windowHandle, VkInstance instance, VkSurfaceKHR* outSurface);
};

typedef MacVulkanPlatform VulkanPlatform;

#endif
