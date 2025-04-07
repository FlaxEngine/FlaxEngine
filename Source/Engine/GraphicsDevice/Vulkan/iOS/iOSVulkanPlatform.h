// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../VulkanPlatformBase.h"

#if GRAPHICS_API_VULKAN && PLATFORM_IOS

#define VULKAN_BACK_BUFFERS_COUNT 3

// General/Validation Error:0 VK_ERROR_INITIALIZATION_FAILED: Could not create MTLCounterSampleBuffer for query pool of type VK_QUERY_TYPE_TIMESTAMP. Reverting to emulated behavior. (Error code 0): Cannot allocate sample buffer
#define VULKAN_USE_QUERIES 0

/// <summary>
/// The implementation for the Vulkan API support for iOS platform.
/// </summary>
class iOSVulkanPlatform : public VulkanPlatformBase
{
public:
	static void GetInstanceExtensions(Array<const char*>& extensions, Array<const char*>& layers);
	static void CreateSurface(void* windowHandle, VkInstance instance, VkSurfaceKHR* outSurface);
};

typedef iOSVulkanPlatform VulkanPlatform;

#endif
