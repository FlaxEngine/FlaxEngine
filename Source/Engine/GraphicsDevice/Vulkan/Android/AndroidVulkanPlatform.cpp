// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN && PLATFORM_ANDROID

#include "AndroidVulkanPlatform.h"
#include "../RenderToolsVulkan.h"
#include "Engine/Platform/Window.h"

void AndroidVulkanPlatform::GetInstanceExtensions(Array<const char*>& extensions, Array<const char*>& layers)
{
	extensions.Add(VK_KHR_SURFACE_EXTENSION_NAME);
	extensions.Add(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
}

void AndroidVulkanPlatform::GetDeviceExtensions(Array<const char*>& extensions, Array<const char*>& layers)
{
	extensions.Add(VK_KHR_SURFACE_EXTENSION_NAME);
	extensions.Add(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
}

void AndroidVulkanPlatform::CreateSurface(Window* window, VkInstance instance, VkSurfaceKHR* surface)
{
    ASSERT(window);
    void* windowHandle = window->GetNativePtr();
    ASSERT(windowHandle);
    VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo;
    RenderToolsVulkan::ZeroStruct(surfaceCreateInfo, VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR);
    surfaceCreateInfo.window = (struct ANativeWindow*)windowHandle;
    VALIDATE_VULKAN_RESULT(vkCreateAndroidSurfaceKHR(instance, &surfaceCreateInfo, nullptr, surface));
}

#endif
