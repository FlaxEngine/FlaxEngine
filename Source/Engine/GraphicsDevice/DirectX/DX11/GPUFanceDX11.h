// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX11

#include "Engine/Graphics/GPUFence.h"
#include "GPUDeviceDX11.h"
#include "../IncludeDirectXHeaders.h"

/// <summary>
/// GPU buffer for DirectX 11 backend.
/// </summary>
/// <seealso cref="GPUResourceDX11" />
class GPUFenceDX11 : public GPUFence
{
private:
    GPUDeviceDX11* _device = nullptr;
    ID3D11Query* queryStart = nullptr;
    ID3D11Query* queryEnd = nullptr;
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUFanceDX11"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="name">The resource name.</param>
    GPUFenceDX11(GPUDeviceDX11* device);

    ~GPUFenceDX11();

    virtual void Signal() override final;
    virtual void Wait() override final;
};

#endif
