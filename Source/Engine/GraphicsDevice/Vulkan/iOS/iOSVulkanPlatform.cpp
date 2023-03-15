// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN && PLATFORM_IOS

#include "iOSVulkanPlatform.h"
#include "../RenderToolsVulkan.h"
#include <UIKit/UIKit.h>

void iOSVulkanPlatform::GetInstanceExtensions(Array<const char*>& extensions, Array<const char*>& layers)
{
    extensions.Add(VK_KHR_SURFACE_EXTENSION_NAME);
	extensions.Add(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
}

void iOSVulkanPlatform::CreateSurface(void* windowHandle, VkInstance instance, VkSurfaceKHR* surface)
{
	MISSING_CODE("TODO: Vulkan via MoltenVK on iOS");
	VkIOSSurfaceCreateInfoMVK surfaceCreateInfo;
	RenderToolsVulkan::ZeroStruct(surfaceCreateInfo, VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK);
	surfaceCreateInfo.pView = nullptr; // UIView or CAMetalLayer
	VALIDATE_VULKAN_RESULT(vkCreateIOSSurfaceMVK(instance, &surfaceCreateInfo, nullptr, surface));
}

#endif
