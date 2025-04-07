// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN && PLATFORM_MAC

#include "MacVulkanPlatform.h"
#include "../RenderToolsVulkan.h"
#include <Cocoa/Cocoa.h>

void MacVulkanPlatform::GetInstanceExtensions(Array<const char*>& extensions, Array<const char*>& layers)
{
    extensions.Add(VK_KHR_SURFACE_EXTENSION_NAME);
	extensions.Add(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
}

void MacVulkanPlatform::CreateSurface(void* windowHandle, VkInstance instance, VkSurfaceKHR* surface)
{
    NSWindow* window = (NSWindow*)windowHandle;
	VkMacOSSurfaceCreateInfoMVK surfaceCreateInfo;
	RenderToolsVulkan::ZeroStruct(surfaceCreateInfo, VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK);
	surfaceCreateInfo.pView = (void*)window.contentView;
	VALIDATE_VULKAN_RESULT(vkCreateMacOSSurfaceMVK(instance, &surfaceCreateInfo, nullptr, surface));
}

#endif
