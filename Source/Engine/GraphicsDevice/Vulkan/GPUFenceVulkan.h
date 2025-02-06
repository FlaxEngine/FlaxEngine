// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_VULKAN

#include "Engine/Graphics/GPUFence.h"
#include "GPUDeviceVulkan.h"

/// <summary>
/// GPU buffer for GPUFence backend.
/// </summary>
/// <seealso cref="GPUFence" />
class GPUFenceVulkan : public GPUFence
{
private:
    GPUDeviceVulkan* _device;
    StringView _name;
    VkFence fence = VK_NULL_HANDLE;
    uint64_t fenceValue = 0;
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUFenceVulkan"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    GPUFenceVulkan(GPUDeviceVulkan* device);

    ~GPUFenceVulkan();

    virtual void Signal() override final;
    virtual void Wait() override final;

};

#endif
