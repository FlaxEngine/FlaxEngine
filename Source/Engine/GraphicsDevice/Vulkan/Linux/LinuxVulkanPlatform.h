// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "../VulkanPlatformBase.h"

#if GRAPHICS_API_VULKAN && PLATFORM_LINUX

// Support more backbuffers in case driver decides to use more (https://gitlab.freedesktop.org/apinheiro/mesa/-/issues/9)
#define VULKAN_BACK_BUFFERS_COUNT_MAX 8

// Prevent wierd error 'Invalid VkValidationCacheEXT Object'
#define VULKAN_USE_VALIDATION_CACHE 0

/// <summary>
/// The implementation for the Vulkan API support for Linux platform.
/// </summary>
class LinuxVulkanPlatform : public VulkanPlatformBase
{
public:
	static void GetInstanceExtensions(Array<const char*>& extensions, Array<const char*>& layers);
	static void CreateSurface(Window* window, VkInstance instance, VkSurfaceKHR* outSurface);
};

typedef LinuxVulkanPlatform VulkanPlatform;

#endif
