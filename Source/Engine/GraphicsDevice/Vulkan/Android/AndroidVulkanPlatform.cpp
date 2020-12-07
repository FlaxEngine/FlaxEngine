// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN && PLATFORM_ANDROID

#include "AndroidVulkanPlatform.h"
#include "../RenderToolsVulkan.h"

void AndroidVulkanPlatform::GetInstanceExtensions(Array<const char*>& outExtensions)
{
	outExtensions.Add(VK_KHR_SURFACE_EXTENSION_NAME);
	outExtensions.Add(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
}

void AndroidVulkanPlatform::GetDeviceExtensions(Array<const char*>& outExtensions)
{
	outExtensions.Add(VK_KHR_SURFACE_EXTENSION_NAME);
	outExtensions.Add(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
}

void AndroidVulkanPlatform::CreateSurface(void* windowHandle, VkInstance instance, VkSurfaceKHR* outSurface)
{
    ASSERT(windowHandle);
    VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo;
    RenderToolsVulkan::ZeroStruct(surfaceCreateInfo, VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR);
    surfaceCreateInfo.window = (struct ANativeWindow*)windowHandle;
    VALIDATE_VULKAN_RESULT(vkCreateAndroidSurfaceKHR(instance, &surfaceCreateInfo, nullptr, outSurface));
}

#endif
