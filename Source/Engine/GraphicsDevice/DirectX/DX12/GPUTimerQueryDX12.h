// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX12

#include "Engine/Graphics/GPUTimerQuery.h"
#include "GPUDeviceDX12.h"

/// <summary>
/// GPU timer query object for DirectX 12 backend.
/// </summary>
class GPUTimerQueryDX12 : public GPUResourceDX12<GPUTimerQuery>
{
private:

    bool _hasResult = false;
    bool _endCalled = false;
    float _timeDelta = 0.0f;
    uint64 _gpuFrequency = 0;
    QueryHeapDX12::ElementHandle _begin;
    QueryHeapDX12::ElementHandle _end;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUTimerQueryDX12"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    GPUTimerQueryDX12(GPUDeviceDX12* device);

public:

    // [GPUTimerQuery]
    void Begin() override;
    void End() override;
    bool HasResult() override;
    float GetResult() override;

protected:

    // [GPUResourceDX12]
    void OnReleaseGPU() override;
};

#endif
