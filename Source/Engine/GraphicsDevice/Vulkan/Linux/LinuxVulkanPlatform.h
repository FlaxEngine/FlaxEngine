// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "../VulkanPlatformBase.h"

#if GRAPHICS_API_VULKAN && PLATFORM_LINUX

/// <summary>
/// The implementation for the Vulkan API support for Linux platform.
/// </summary>
class LinuxVulkanPlatform : public VulkanPlatformBase
{
public:
	static void GetInstanceExtensions(Array<const char*>& extensions, Array<const char*>& layers);
	static void CreateSurface(void* windowHandle, VkInstance instance, VkSurfaceKHR* outSurface);
};

typedef LinuxVulkanPlatform VulkanPlatform;

#endif
