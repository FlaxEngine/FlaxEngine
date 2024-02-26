// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
        Description = other.Description;
        return *this;
    }

public:
    /// <summary>
    /// The GPU device handle.
    /// </summary>
    VkPhysicalDevice Gpu;

    /// <summary>
    /// The GPU device properties.
    /// </summary>
    VkPhysicalDeviceProperties GpuProps;

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
    void* GetNativePtr() const override
    {
        return (void*)Gpu;
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
