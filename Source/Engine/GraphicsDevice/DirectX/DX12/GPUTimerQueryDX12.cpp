// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "GPUTimerQueryDX12.h"
#include "GPUContextDX12.h"

GPUTimerQueryDX12::GPUTimerQueryDX12(GPUDeviceDX12* device)
    : GPUResourceDX12<GPUTimerQuery>(device, String::Empty)
{
}

void GPUTimerQueryDX12::OnReleaseGPU()
{
    _hasResult = false;
    _endCalled = false;
    _timeDelta = 0.0f;
}

void GPUTimerQueryDX12::Begin()
{
    const auto context = _device->GetMainContextDX12();
    _query = context->BeginQuery(GPUQueryType::Timer);
    _hasResult = false;
    _endCalled = false;
}

void GPUTimerQueryDX12::End()
{
    if (_endCalled)
        return;
    const auto context = _device->GetMainContextDX12();
    context->EndQuery(_query);
    _endCalled = true;
}

bool GPUTimerQueryDX12::HasResult()
{
    if (!_endCalled)
        return false;
    if (_hasResult)
        return true;
    uint64 result;
    return _device->GetQueryResult(_query, result, false);
}

float GPUTimerQueryDX12::GetResult()
{
    if (_hasResult)
        return _timeDelta;
    uint64 result;
    _timeDelta = _device->GetQueryResult(_query, result, true) ? (float)((double)result / 1000.0) : 0.0f;
    _hasResult = true;
    return _timeDelta;
}

#endif
