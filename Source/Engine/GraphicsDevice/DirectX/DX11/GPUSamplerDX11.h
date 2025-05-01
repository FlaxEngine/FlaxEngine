// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/Textures/GPUSampler.h"
#include "GPUDeviceDX11.h"

#if GRAPHICS_API_DIRECTX11

/// <summary>
/// Sampler object for DirectX 11 backend.
/// </summary>
class GPUSamplerDX11 : public GPUResourceBase<GPUDeviceDX11, GPUSampler>
{
public:

    GPUSamplerDX11(GPUDeviceDX11* device)
        : GPUResourceBase<GPUDeviceDX11, GPUSampler>(device, StringView::Empty)
    {
    }

    ID3D11SamplerState* SamplerState = nullptr;

protected:

    // [GPUSampler]
    bool OnInit() override;
    void OnReleaseGPU() override;
};

#endif
