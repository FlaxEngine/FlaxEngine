// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_VULKAN

#include "Engine/Graphics/GPUAdapter.h"
#include "VulkanPlatform.h"
#include "IncludeVulkanHeaders.h"

/// <summary>
/// Graphics Device adapter implementation for Vulkan backend.
/// </summary>
class GPUAdapterVulkan : public GPUAdapter
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUAdapterVulkan"/> class.
    /// </summary>
    GPUAdapterVulkan()
        : Gpu(VK_NULL_HANDLE)
    {
    }

    GPUAdapterVulkan(const GPUAdapterVulkan& other)
        : GPUAdapterVulkan()
    {
        *this = other;
    }

    GPUAdapterVulkan& operator=(const GPUAdapterVulkan& other)
    {
        Gpu = other.Gpu;
        GpuProps = other.GpuProps;
#if VULKAN_ENABLE_DESKTOP_HMD_SUPPORT
        GpuIdProps = other.GpuIdProps;
#endif
        Description = other.Description;
        return *this;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUAdapterVulkan"/> class.
    /// </summary>
    /// <param name="gpu">The GPU device handle.</param>
    GPUAdapterVulkan(VkPhysicalDevice gpu);

public:

    /// <summary>
    /// The GPU device handle.
    /// </summary>
    VkPhysicalDevice Gpu;

    /// <summary>
    /// The GPU device properties.
    /// </summary>
    VkPhysicalDeviceProperties GpuProps;

#if VULKAN_ENABLE_DESKTOP_HMD_SUPPORT

	/// <summary>
	/// The GPU device extended properties.
	/// </summary>
	VkPhysicalDeviceIDPropertiesKHR GpuIdProps;

#endif

    /// <summary>
    /// The GPU description.
    /// </summary>
    String Description;

public:

    // [GPUAdapter]
    bool IsValid() const override
    {
        return Gpu != VK_NULL_HANDLE;
    }

    uint32 GetVendorId() const override
    {
        return GpuProps.vendorID;
    }

    String GetDescription() const override
    {
        return Description;
    }
};

#endif
