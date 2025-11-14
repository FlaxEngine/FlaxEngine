// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN && PLATFORM_MAC

#include "MacVulkanPlatform.h"
#include "../RenderToolsVulkan.h"
#include "Engine/Platform/Window.h"
#include "Engine/GraphicsDevice/Vulkan/GPUDeviceVulkan.h"
#include <Cocoa/Cocoa.h>
#include <QuartzCore/CAMetalLayer.h>

void MacVulkanPlatform::GetInstanceExtensions(Array<const char*>& extensions, Array<const char*>& layers)
{
    extensions.Add(VK_KHR_SURFACE_EXTENSION_NAME);
    extensions.Add(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
    extensions.Add(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
}

void MacVulkanPlatform::CreateSurface(Window* window, GPUDeviceVulkan* device, VkInstance instance, VkSurfaceKHR* surface)
{
    void* windowHandle = window->GetNativePtr();
    NSWindow* nswindow = (NSWindow*)windowHandle;
#if PLATFORM_SDL
    nswindow.contentView.wantsLayer = YES;
    nswindow.contentView.layer = [CAMetalLayer layer];
#endif
    if (device->InstanceExtensions.Contains(VK_EXT_METAL_SURFACE_EXTENSION_NAME))
    {
        VkMetalSurfaceCreateInfoEXT surfaceCreateInfo;
        RenderToolsVulkan::ZeroStruct(surfaceCreateInfo, VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT);
        surfaceCreateInfo.pLayer = (CAMetalLayer*)nswindow.contentView.layer;
        VALIDATE_VULKAN_RESULT(vkCreateMetalSurfaceEXT(instance, &surfaceCreateInfo, nullptr, surface));
    }
    else
    {
        VkMacOSSurfaceCreateInfoMVK surfaceCreateInfo;
        RenderToolsVulkan::ZeroStruct(surfaceCreateInfo, VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK);
        surfaceCreateInfo.pView = (void*)nswindow.contentView;
        VALIDATE_VULKAN_RESULT(vkCreateMacOSSurfaceMVK(instance, &surfaceCreateInfo, nullptr, surface));
    }
}

#endif
