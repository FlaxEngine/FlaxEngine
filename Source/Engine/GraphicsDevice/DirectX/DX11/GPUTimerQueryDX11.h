// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUTimerQuery.h"
#include "GPUDeviceDX11.h"

#if GRAPHICS_API_DIRECTX11

/// <summary>
/// GPU timer query object for DirectX 11 backend.
/// </summary>
class GPUTimerQueryDX11 : public GPUResourceDX11<GPUTimerQuery>
{
private:

    bool _finalized = false;
    bool _endCalled = false;
    float _timeDelta = 0.0f;

    ID3D11Query* _beginQuery = nullptr;
    ID3D11Query* _endQuery = nullptr;
    ID3D11Query* _disjointQuery = nullptr;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUTimerQueryDX11"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    GPUTimerQueryDX11(GPUDeviceDX11* device);

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUTimerQueryDX11"/> class.
    /// </summary>
    ~GPUTimerQueryDX11();

public:

    // [GPUResourceDX11]
    ID3D11Resource* GetResource() final override;

    // [GPUTimerQuery]
    void Begin() override;
    void End() override;
    bool HasResult() override;
    float GetResult() override;

protected:

    // [GPUResource]
    void OnReleaseGPU() override;
};

#endif
