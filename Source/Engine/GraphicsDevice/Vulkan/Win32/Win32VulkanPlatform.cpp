// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Win32VulkanPlatform.h"

#if GRAPHICS_API_VULKAN && PLATFORM_WIN32

#include "../RenderToolsVulkan.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Platform/Window.h"

void Win32VulkanPlatform::GetInstanceExtensions(Array<const char*>& extensions, Array<const char*>& layers)
{
    extensions.Add(VK_KHR_SURFACE_EXTENSION_NAME);
    extensions.Add(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
}

void Win32VulkanPlatform::CreateSurface(Window* window, VkInstance instance, VkSurfaceKHR* surface)
{
    void* windowHandle = window->GetNativePtr();
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo;
    RenderToolsVulkan::ZeroStruct(surfaceCreateInfo, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR);
    surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
    surfaceCreateInfo.hwnd = static_cast<HWND>(windowHandle);
    VALIDATE_VULKAN_RESULT(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, surface));
}

#endif
