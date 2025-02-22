// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN && PLATFORM_LINUX

#include "LinuxVulkanPlatform.h"
#include "../RenderToolsVulkan.h"
#include "Engine/Platform/Window.h"

#include "Engine/Platform/Linux/IncludeX11.h"
#define Display X11::Display
#define Window X11::Window
#define VisualID X11::VisualID
#include "vulkan/vulkan_xlib.h"
#undef Display
#undef Window
#undef VisualID

#include "vulkan/vulkan_wayland.h"

// Export extension from volk
extern PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
extern PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR vkGetPhysicalDeviceXlibPresentationSupportKHR;
extern PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR;
extern PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR vkGetPhysicalDeviceWaylandPresentationSupportKHR;

void LinuxVulkanPlatform::GetInstanceExtensions(Array<const char*>& extensions, Array<const char*>& layers)
{
	extensions.Add(VK_KHR_SURFACE_EXTENSION_NAME);
	extensions.Add(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
    extensions.Add(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
}

void LinuxVulkanPlatform::CreateSurface(Window* window, VkInstance instance, VkSurfaceKHR* surface)
{
#if !PLATFORM_SDL
    void* windowHandle = window->GetNativePtr();
	VkXlibSurfaceCreateInfoKHR surfaceCreateInfo;
	RenderToolsVulkan::ZeroStruct(surfaceCreateInfo, VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR);
	surfaceCreateInfo.dpy = (X11::Display*)Platform::GetXDisplay();
	surfaceCreateInfo.window = (X11::Window)windowHandle;
	VALIDATE_VULKAN_RESULT(vkCreateXlibSurfaceKHR(instance, &surfaceCreateInfo, nullptr, surface));
#else
    SDLWindow* sdlWindow = static_cast<Window*>(window);
    X11::Window x11Window = (X11::Window)sdlWindow->GetX11WindowHandle();
    wl_surface* waylandSurface = (wl_surface*)sdlWindow->GetWaylandSurfacePtr();
    if (waylandSurface != nullptr)
    {
        VkWaylandSurfaceCreateInfoKHR surfaceCreateInfo;
        RenderToolsVulkan::ZeroStruct(surfaceCreateInfo, VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR);
        surfaceCreateInfo.display = (wl_display*)sdlWindow->GetWaylandDisplay();
        surfaceCreateInfo.surface = waylandSurface;
        VALIDATE_VULKAN_RESULT(vkCreateWaylandSurfaceKHR(instance, &surfaceCreateInfo, nullptr, surface));
    }
    else if (x11Window != 0)
    {
        VkXlibSurfaceCreateInfoKHR surfaceCreateInfo;
        RenderToolsVulkan::ZeroStruct(surfaceCreateInfo, VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR);
        surfaceCreateInfo.dpy = (X11::Display*)sdlWindow->GetX11Display();
        surfaceCreateInfo.window = x11Window;
        VALIDATE_VULKAN_RESULT(vkCreateXlibSurfaceKHR(instance, &surfaceCreateInfo, nullptr, surface));
    }
#endif
}

#endif
