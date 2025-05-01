// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN && PLATFORM_LINUX

#include "LinuxVulkanPlatform.h"
#include "../RenderToolsVulkan.h"

// Contents of vulkan\vulkan_xlib.h inlined here to prevent typename collisions with engine types due to X11 types
#include "Engine/Platform/Linux/IncludeX11.h"
#ifdef __cplusplus
extern "C" {
#endif
#define VK_KHR_xlib_surface 1
#define VK_KHR_XLIB_SURFACE_SPEC_VERSION  6
#define VK_KHR_XLIB_SURFACE_EXTENSION_NAME "VK_KHR_xlib_surface"
typedef VkFlags VkXlibSurfaceCreateFlagsKHR;

typedef struct VkXlibSurfaceCreateInfoKHR
{
	VkStructureType sType;
	const void* pNext;
	VkXlibSurfaceCreateFlagsKHR flags;
	X11::Display* dpy;
	X11::Window window;
} VkXlibSurfaceCreateInfoKHR;

typedef VkResult (VKAPI_PTR *PFN_vkCreateXlibSurfaceKHR)(VkInstance instance, const VkXlibSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
typedef VkBool32 (VKAPI_PTR *PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR)(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, X11::Display* dpy, X11::VisualID visualID);
#ifndef VK_NO_PROTOTYPES
VKAPI_ATTR VkResult VKAPI_CALL vkCreateXlibSurfaceKHR(
    VkInstance                                  instance,
    const VkXlibSurfaceCreateInfoKHR*           pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface);
VKAPI_ATTR VkBool32 VKAPI_CALL vkGetPhysicalDeviceXlibPresentationSupportKHR(
    VkPhysicalDevice                            physicalDevice,
    uint32_t                                    queueFamilyIndex,
    Display*                                    dpy,
    VisualID                                    visualID);
#endif
#ifdef __cplusplus
}
#endif
//

// Export X11 surface extension from volk
extern PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
extern PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR vkGetPhysicalDeviceXlibPresentationSupportKHR;

void LinuxVulkanPlatform::GetInstanceExtensions(Array<const char*>& extensions, Array<const char*>& layers)
{
	extensions.Add(VK_KHR_SURFACE_EXTENSION_NAME);
	extensions.Add(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
}

void LinuxVulkanPlatform::CreateSurface(void* windowHandle, VkInstance instance, VkSurfaceKHR* surface)
{
	VkXlibSurfaceCreateInfoKHR surfaceCreateInfo;
	RenderToolsVulkan::ZeroStruct(surfaceCreateInfo, VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR);
	surfaceCreateInfo.dpy = (X11::Display*)LinuxPlatform::GetXDisplay();
	surfaceCreateInfo.window = (X11::Window)windowHandle;
	VALIDATE_VULKAN_RESULT(vkCreateXlibSurfaceKHR(instance, &surfaceCreateInfo, nullptr, surface));
}

#endif
