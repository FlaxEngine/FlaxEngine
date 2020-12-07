// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "Win32VulkanPlatform.h"

#if GRAPHICS_API_VULKAN && PLATFORM_WIN32

#include "../RenderToolsVulkan.h"
#include "Engine/Graphics/GPUDevice.h"

void Win32VulkanPlatform::GetInstanceExtensions(Array<const char*>& outExtensions)
{
    // Include Windows surface extension
    outExtensions.Add(VK_KHR_SURFACE_EXTENSION_NAME);
    outExtensions.Add(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
}

void Win32VulkanPlatform::CreateSurface(void* windowHandle, VkInstance instance, VkSurfaceKHR* outSurface)
{
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo;
    RenderToolsVulkan::ZeroStruct(surfaceCreateInfo, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR);
    surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
    surfaceCreateInfo.hwnd = static_cast<HWND>(windowHandle);
    VALIDATE_VULKAN_RESULT(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, outSurface));
}

#endif
