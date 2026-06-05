// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/Textures/GPUSampler.h"
#include "GPUDeviceWebGPU.h"

#if GRAPHICS_API_WEBGPU

/// <summary>
/// Sampler object for Web GPU backend.
/// </summary>
class GPUSamplerWebGPU : public GPUResourceBase<GPUDeviceWebGPU, GPUSampler>
{
public:
    GPUSamplerWebGPU(GPUDeviceWebGPU* device)
        : GPUResourceBase<GPUDeviceWebGPU, GPUSampler>(device, StringView::Empty)
    {
    }

public:
    WGPUSampler Sampler = nullptr;

protected:
    // [GPUSampler]
    bool OnInit() override;
    void OnReleaseGPU() override;
};

#endif
