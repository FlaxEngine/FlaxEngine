// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/Config.h"
#include "../IncludeDirectXHeaders.h"

#define DX12_ROOT_SIGNATURE_CB 0
#define DX12_ROOT_SIGNATURE_SR (GPU_MAX_CB_BINDED+0)
#define DX12_ROOT_SIGNATURE_UA (GPU_MAX_CB_BINDED+1)
#define DX12_ROOT_SIGNATURE_SAMPLER (GPU_MAX_CB_BINDED+2)

struct RootSignatureDX12
{
private:
    D3D12_ROOT_SIGNATURE_DESC _desc;
    D3D12_DESCRIPTOR_RANGE _ranges[3];
    D3D12_ROOT_PARAMETER _parameters[GPU_MAX_CB_BINDED + 3];
    D3D12_STATIC_SAMPLER_DESC _staticSamplers[6];

public:
    RootSignatureDX12();

	ComPtr<ID3DBlob> Serialize() const;
#if USE_EDITOR
    void ToString(class StringBuilder& sb, bool singleLine = false) const;
    String ToString() const;
    StringAnsi ToStringAnsi() const;
#endif

private:
    void InitSampler(int32 i, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE address, D3D12_COMPARISON_FUNC comparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL);
};
