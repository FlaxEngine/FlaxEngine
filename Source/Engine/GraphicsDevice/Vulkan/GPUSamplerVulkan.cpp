// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN

#include "GPUSamplerVulkan.h"
#include "RenderToolsVulkan.h"

bool GPUSamplerVulkan::OnInit()
{
    VkSamplerCreateInfo createInfo;
    RenderToolsVulkan::ZeroStruct(createInfo, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
    createInfo.magFilter = RenderToolsVulkan::ToVulkanMagFilterMode(_desc.Filter);
    createInfo.minFilter = RenderToolsVulkan::ToVulkanMinFilterMode(_desc.Filter);
    createInfo.mipmapMode = RenderToolsVulkan::ToVulkanMipFilterMode(_desc.Filter);
    const bool supportsMirrorClampToEdge = GPUDeviceVulkan::OptionalDeviceExtensions.HasMirrorClampToEdge;
    createInfo.addressModeU = RenderToolsVulkan::ToVulkanWrapMode(_desc.AddressU, supportsMirrorClampToEdge);
    createInfo.addressModeV = RenderToolsVulkan::ToVulkanWrapMode(_desc.AddressV, supportsMirrorClampToEdge);
    createInfo.addressModeW = RenderToolsVulkan::ToVulkanWrapMode(_desc.AddressW, supportsMirrorClampToEdge);
    createInfo.mipLodBias = _desc.MipBias;
    createInfo.anisotropyEnable = _desc.Filter == GPUSamplerFilter::Anisotropic ? VK_TRUE : VK_FALSE;
    createInfo.maxAnisotropy = (float)_desc.MaxAnisotropy;
    createInfo.compareEnable = _desc.ComparisonFunction != GPUSamplerCompareFunction::Never ? VK_TRUE : VK_FALSE;
    createInfo.compareOp = RenderToolsVulkan::ToVulkanSamplerCompareFunction(_desc.ComparisonFunction);
    createInfo.minLod = _desc.MinMipLevel;
    createInfo.maxLod = _desc.MaxMipLevel;
    switch (_desc.BorderColor)
    {
    case GPUSamplerBorderColor::TransparentBlack:
        createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        break;
    case GPUSamplerBorderColor::OpaqueBlack:
        createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        break;
    case GPUSamplerBorderColor::OpaqueWhite:
        createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        break;
    default:
        return true;
    }
    VALIDATE_VULKAN_RESULT(vkCreateSampler(_device->Device, &createInfo, nullptr, &Sampler));
    return false;
}

void GPUSamplerVulkan::OnReleaseGPU()
{
    if (Sampler != VK_NULL_HANDLE)
    {
        _device->DeferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Buffer, Sampler);
        Sampler = VK_NULL_HANDLE;
    }

    // Base
    GPUSampler::OnReleaseGPU();
}

#endif
