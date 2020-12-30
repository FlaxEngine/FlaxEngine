// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "../VulkanPlatformBase.h"

#if GRAPHICS_API_VULKAN && PLATFORM_ANDROID

/// <summary>
/// The implementation for the Vulkan API support for Android platform.
/// </summary>
class AndroidVulkanPlatform : public VulkanPlatformBase
{
public:

	static void GetInstanceExtensions(Array<const char*>& extensions);
	static void GetDeviceExtensions(Array<const char*>& extensions);
	static void CreateSurface(void* windowHandle, VkInstance instance, VkSurfaceKHR* surface);
};

typedef AndroidVulkanPlatform VulkanPlatform;

#endif
