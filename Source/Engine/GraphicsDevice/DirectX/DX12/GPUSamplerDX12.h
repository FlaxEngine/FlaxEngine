// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/Textures/GPUSampler.h"
#include "GPUDeviceDX12.h"

#if GRAPHICS_API_DIRECTX12

/// <summary>
/// Sampler object for DirectX 12 backend.
/// </summary>
class GPUSamplerDX12 : public GPUResourceDX12<GPUSampler>
{
public:

    GPUSamplerDX12(GPUDeviceDX12* device)
        : GPUResourceDX12<GPUSampler>(device, StringView::Empty)
    {
    }

    DescriptorHeapWithSlotsDX12::Slot Slot;
    D3D12_CPU_DESCRIPTOR_HANDLE HandleCPU;

protected:

    // [GPUSampler]
    bool OnInit() override;
    void OnReleaseGPU() override;
};

#endif
