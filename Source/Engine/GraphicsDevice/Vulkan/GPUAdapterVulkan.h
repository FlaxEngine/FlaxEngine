// Copyright (c) Wojciech Figat. All rights reserved.

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
    Version GetDriverVersion() const override
    {
        Version version(VK_VERSION_MAJOR(GpuProps.driverVersion), VK_VERSION_MINOR(GpuProps.driverVersion), VK_VERSION_PATCH(GpuProps.driverVersion));
        if (IsNVIDIA())
        {
            union NvidiaDriverVersion
            {
                struct
                {
                    uint32 Tertiary : 6;
                    uint32 Secondary : 8;
                    uint32 Minor : 8;
                    uint32 Major : 10;
                };
                uint32 Packed;
            } NvidiaVersion;
            NvidiaVersion.Packed = GpuProps.driverVersion;
            version = Version(NvidiaVersion.Major, NvidiaVersion.Minor);
        }
        return version;
    }
};

#endif
