// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN && PLATFORM_IOS

#include "iOSVulkanPlatform.h"
#include "../RenderToolsVulkan.h"
#include "Engine/Core/Delegate.h"
#include <UIKit/UIKit.h>

void iOSVulkanPlatform::GetInstanceExtensions(Array<const char*>& extensions, Array<const char*>& layers)
{
    extensions.Add(VK_KHR_SURFACE_EXTENSION_NAME);
	extensions.Add(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
}

void iOSVulkanPlatform::CreateSurface(void* windowHandle, VkInstance instance, VkSurfaceKHR* surface)
{
	// Create surface on a main UI Thread
	Function<void()> func = [&windowHandle, &instance, &surface]()
	{
		VkIOSSurfaceCreateInfoMVK surfaceCreateInfo;
		RenderToolsVulkan::ZeroStruct(surfaceCreateInfo, VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK);
		surfaceCreateInfo.pView = windowHandle; // UIView or CAMetalLayer
		VALIDATE_VULKAN_RESULT(vkCreateIOSSurfaceMVK(instance, &surfaceCreateInfo, nullptr, surface));
	};
	iOSPlatform::RunOnUIThread(func, true);
}

#endif
