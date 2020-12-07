// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "GPUSamplerDescription.h"
#include "Engine/Core/Types/String.h"

void GPUSamplerDescription::Clear()
{
    Filter = GPUSamplerFilter::Point;
    AddressU = GPUSamplerAddressMode::Wrap;
    AddressV = GPUSamplerAddressMode::Wrap;
    AddressW = GPUSamplerAddressMode::Wrap;
    MipBias = 0;
    MaxAnisotropy = 0;
    MinMipLevel = 0;
    MaxMipLevel = MAX_float;
    BorderColor = GPUSamplerBorderColor::TransparentBlack;
    SamplerComparisonFunction = GPUSamplerCompareFunction::Never;
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
            && SamplerComparisonFunction == other.SamplerComparisonFunction;
}

String GPUSamplerDescription::ToString() const
{
    return String::Format(TEXT("Filter: {}, Address: {}x{}x{}, MipBias: {}, MaxAnisotropy: {}, MinMipLevel: {}, MaxMipLevel: {}, BorderColor: {}, SamplerComparisonFunction: {}"),
                          (int32)Filter,
                          (int32)AddressU,
                          (int32)AddressV,
                          (int32)AddressW,
                          MipBias,
                          MaxAnisotropy,
                          MinMipLevel,
                          MaxMipLevel,
                          (int32)BorderColor,
                          (int32)SamplerComparisonFunction);
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
    hashCode = (hashCode * 397) ^ (uint32)key.SamplerComparisonFunction;
    return hashCode;
}
