// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX12

#include "Engine/Graphics/GPUFence.h"
#include "GPUDeviceDX12.h"
#include "../IncludeDirectXHeaders.h"

/// <summary>
/// GPU buffer for DirectX 12 backend.
/// </summary>
/// <seealso cref="GPUResourceDX12" />
class GPUFenceDX12 : public GPUResourceDX12<GPUFence>
{
private:
    ID3D12Fence* fence = nullptr;
    UINT64 fenceValue = 0;
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUFanceDX12"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="name">The resource name.</param>
    GPUFenceDX12(GPUDeviceDX12* device, const StringView& name);

    ~GPUFenceDX12();

    virtual void Signal() override final;
    virtual void Wait() override final;


};

#endif
