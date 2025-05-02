// Copyright (c) Wojciech Figat. All rights reserved.

#include "GPUSampler.h"
#include "GPUSamplerDescription.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Graphics/GPUDevice.h"

GPUSamplerDescription GPUSamplerDescription::New(GPUSamplerFilter filter, GPUSamplerAddressMode addressMode)
{
    GPUSamplerDescription desc;
    desc.Clear();
    desc.Filter = filter;
    desc.AddressU = desc.AddressV = desc.AddressW = addressMode;
    return desc;
}

void GPUSamplerDescription::Clear()
{
    Platform::MemoryClear(this, sizeof(GPUSamplerDescription));
    MaxMipLevel = MAX_float;
}

bool GPUSamplerDescription::Equals(const GPUSamplerDescription& other) const
{
    return Filter == other.Filter
            && AddressU == other.AddressU
            && AddressV == other.AddressV
            && AddressW == other.AddressW
            && MipBias == other.MipBias
            && MaxAnisotropy == other.MaxAnisotropy
            && MinMipLevel == other.MinMipLevel
            && MaxMipLevel == other.MaxMipLevel
            && BorderColor == other.BorderColor
            && ComparisonFunction == other.ComparisonFunction;
}

String GPUSamplerDescription::ToString() const
{
    return String::Format(TEXT("Filter: {}, Address: {}x{}x{}, MipBias: {}, MaxAnisotropy: {}, MinMipLevel: {}, MaxMipLevel: {}, BorderColor: {}, ComparisonFunction: {}"),
                          (int32)Filter,
                          (int32)AddressU,
                          (int32)AddressV,
                          (int32)AddressW,
                          MipBias,
                          MaxAnisotropy,
                          MinMipLevel,
                          MaxMipLevel,
                          (int32)BorderColor,
                          (int32)ComparisonFunction);
}

uint32 GetHash(const GPUSamplerDescription& key)
{
    uint32 hashCode = (uint32)key.Filter;
    hashCode = (hashCode * 397) ^ (uint32)key.AddressU;
    hashCode = (hashCode * 397) ^ (uint32)key.AddressV;
    hashCode = (hashCode * 397) ^ (uint32)key.AddressW;
    hashCode = (hashCode * 397) ^ (uint32)key.MipBias;
    hashCode = (hashCode * 397) ^ (uint32)key.MaxAnisotropy;
    hashCode = (hashCode * 397) ^ (uint32)key.MinMipLevel;
    hashCode = (hashCode * 397) ^ (uint32)key.MaxMipLevel;
    hashCode = (hashCode * 397) ^ (uint32)key.BorderColor;
    hashCode = (hashCode * 397) ^ (uint32)key.ComparisonFunction;
    return hashCode;
}

GPUSampler* GPUSampler::Spawn(const SpawnParams& params)
{
    return GPUDevice::Instance->CreateSampler();
}

GPUSampler* GPUSampler::New()
{
    return GPUDevice::Instance->CreateSampler();
}

GPUSampler::GPUSampler()
    : GPUResource(SpawnParams(Guid::New(), TypeInitializer))
{
}

bool GPUSampler::Init(const GPUSamplerDescription& desc)
{
    ReleaseGPU();
    _desc = desc;
    if (OnInit())
    {
        ReleaseGPU();
        LOG(Warning, "Cannot initialize sampler. Description: {0}", desc.ToString());
        return true;
    }
    return false;
}

String GPUSampler::ToString() const
{
#if GPU_ENABLE_RESOURCE_NAMING
    return String::Format(TEXT("Sampler {0}, {1}"), GetName(), _desc.ToString());
#else
	return TEXT("Sampler");
#endif
}

GPUResourceType GPUSampler::GetResourceType() const
{
    return GPUResourceType::Sampler;
}

void GPUSampler::OnReleaseGPU()
{
    _desc.Clear();
}
