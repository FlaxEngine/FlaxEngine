// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_VULKAN

#include "Engine/Graphics/Textures/GPUSampler.h"
#include "GPUDeviceVulkan.h"

/// <summary>
/// Sampler object for Vulkan backend.
/// </summary>
class GPUSamplerVulkan : public GPUResourceVulkan<GPUSampler>
{
public:
    GPUSamplerVulkan(GPUDeviceVulkan* device)
        : GPUResourceVulkan<GPUSampler>(device, StringView::Empty)
    {
    }

    VkSampler Sampler = VK_NULL_HANDLE;

protected:
    // [GPUSamplerVulkan]
    bool OnInit() override;
    void OnReleaseGPU() override;
};

#endif
