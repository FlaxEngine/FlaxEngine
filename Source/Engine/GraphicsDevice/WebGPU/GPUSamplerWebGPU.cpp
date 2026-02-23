// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_WEBGPU

#include "GPUSamplerWebGPU.h"

WGPUAddressMode ToAddressMode(GPUSamplerAddressMode value)
{
    switch (value)
    {
    case GPUSamplerAddressMode::Wrap:
        return WGPUAddressMode_Repeat;
    case GPUSamplerAddressMode::Clamp:
        return WGPUAddressMode_ClampToEdge;
    case GPUSamplerAddressMode::Mirror:
        return WGPUAddressMode_MirrorRepeat;
    default:
        return WGPUAddressMode_Undefined;
    }
}

WGPUCompareFunction ToCompareFunction(GPUSamplerCompareFunction value)
{
    switch (value)
    {
    case GPUSamplerCompareFunction::Never:
        return WGPUCompareFunction_Never;
    case GPUSamplerCompareFunction::Less:
        return WGPUCompareFunction_Less;
    default:
        return WGPUCompareFunction_Undefined;
    }
}

bool GPUSamplerWebGPU::OnInit()
{
    WGPUSamplerDescriptor samplerDesc = WGPU_SAMPLER_DESCRIPTOR_INIT;
    samplerDesc.addressModeU = ToAddressMode(_desc.AddressU);
    samplerDesc.addressModeV = ToAddressMode(_desc.AddressV);
    samplerDesc.addressModeW = ToAddressMode(_desc.AddressW);
    switch (_desc.Filter)
    {
    case GPUSamplerFilter::Point:
        samplerDesc.magFilter = samplerDesc.magFilter = WGPUFilterMode_Nearest;
        samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
        break;
    case GPUSamplerFilter::Bilinear:
        samplerDesc.magFilter = samplerDesc.magFilter = WGPUFilterMode_Linear;
        samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
        break;
    case GPUSamplerFilter::Trilinear:
        samplerDesc.magFilter = samplerDesc.magFilter = WGPUFilterMode_Linear;
        samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Linear;
        break;
    case GPUSamplerFilter::Anisotropic:
        samplerDesc.magFilter = samplerDesc.magFilter = WGPUFilterMode_Linear;
        samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Linear;
        break;
    }
    samplerDesc.lodMinClamp = _desc.MinMipLevel;
    samplerDesc.lodMaxClamp = _desc.MaxMipLevel;
    samplerDesc.compare = ToCompareFunction(_desc.ComparisonFunction);
    samplerDesc.maxAnisotropy = _desc.MaxAnisotropy;
    Sampler = wgpuDeviceCreateSampler(_device->Device, &samplerDesc);
    if (Sampler == nullptr)
        return true;
    _memoryUsage = 100;
    
    return false;
}

void GPUSamplerWebGPU::OnReleaseGPU()
{
    if (Sampler)
    {
        wgpuSamplerRelease(Sampler);
        Sampler = nullptr;
    }

    // Base
    GPUSampler::OnReleaseGPU();
}

#endif
