// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX11

#include "GPUSamplerDX11.h"

D3D11_TEXTURE_ADDRESS_MODE ToDX11(GPUSamplerAddressMode value)
{
    switch (value)
    {
    case GPUSamplerAddressMode::Wrap:
        return D3D11_TEXTURE_ADDRESS_WRAP;
    case GPUSamplerAddressMode::Clamp:
        return D3D11_TEXTURE_ADDRESS_CLAMP;
    case GPUSamplerAddressMode::Mirror:
        return D3D11_TEXTURE_ADDRESS_MIRROR;
    case GPUSamplerAddressMode::Border:
        return D3D11_TEXTURE_ADDRESS_BORDER;
    default:
        return (D3D11_TEXTURE_ADDRESS_MODE)-1;
    }
}

bool GPUSamplerDX11::OnInit()
{
    D3D11_SAMPLER_DESC samplerDesc;
    if (_desc.ComparisonFunction == GPUSamplerCompareFunction::Never)
    {
        switch (_desc.Filter)
        {
        case GPUSamplerFilter::Point:
            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
            break;
        case GPUSamplerFilter::Bilinear:
            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            break;
        case GPUSamplerFilter::Trilinear:
            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            break;
        case GPUSamplerFilter::Anisotropic:
            samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
            break;
        default:
            return true;
        }
    }
    else
    {
        switch (_desc.Filter)
        {
        case GPUSamplerFilter::Point:
            samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
            break;
        case GPUSamplerFilter::Bilinear:
            samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
            break;
        case GPUSamplerFilter::Trilinear:
            samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
            break;
        case GPUSamplerFilter::Anisotropic:
            samplerDesc.Filter = D3D11_FILTER_COMPARISON_ANISOTROPIC;
            break;
        default:
            return true;
        }
    }
    samplerDesc.AddressU = ToDX11(_desc.AddressU);
    samplerDesc.AddressV = ToDX11(_desc.AddressV);
    samplerDesc.AddressW = ToDX11(_desc.AddressW);
    samplerDesc.MipLODBias = _desc.MipBias;
    samplerDesc.MaxAnisotropy = _desc.MaxAnisotropy;
    switch (_desc.ComparisonFunction)
    {
    case GPUSamplerCompareFunction::Never:
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        break;
    case GPUSamplerCompareFunction::Less:
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
        break;
    default:
        return true;
    }
    switch (_desc.BorderColor)
    {
    case GPUSamplerBorderColor::TransparentBlack:
        samplerDesc.BorderColor[0] = 0;
        samplerDesc.BorderColor[1] = 0;
        samplerDesc.BorderColor[2] = 0;
        samplerDesc.BorderColor[3] = 0;
        break;
    case GPUSamplerBorderColor::OpaqueBlack:
        samplerDesc.BorderColor[0] = 0;
        samplerDesc.BorderColor[1] = 0;
        samplerDesc.BorderColor[2] = 0;
        samplerDesc.BorderColor[3] = 1.0f;
        break;
    case GPUSamplerBorderColor::OpaqueWhite:
        samplerDesc.BorderColor[0] = 1.0f;
        samplerDesc.BorderColor[1] = 1.0f;
        samplerDesc.BorderColor[2] = 1.0f;
        samplerDesc.BorderColor[3] = 1.0f;
        break;
    default:
        return true;
    }
    samplerDesc.MinLOD = _desc.MinMipLevel;
    samplerDesc.MaxLOD = _desc.MaxMipLevel;
    HRESULT result = _device->GetDevice()->CreateSamplerState(&samplerDesc, &SamplerState);
    LOG_DIRECTX_RESULT_WITH_RETURN(result, true);
    ASSERT(SamplerState != nullptr);
    _memoryUsage = sizeof(D3D11_SAMPLER_DESC);

    return false;
}

void GPUSamplerDX11::OnReleaseGPU()
{
    if (SamplerState)
    {
        SamplerState->Release();
        SamplerState = nullptr;
    }

    // Base
    GPUSampler::OnReleaseGPU();
}

#endif
