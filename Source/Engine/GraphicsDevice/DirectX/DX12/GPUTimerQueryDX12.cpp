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
    auto& heap = _device->TimestampQueryHeap;
    heap.EndQuery(context, _begin);

    _hasResult = false;
    _endCalled = false;
}

void GPUTimerQueryDX12::End()
{
    if (_endCalled)
        return;

    const auto context = _device->GetMainContextDX12();
    auto& heap = _device->TimestampQueryHeap;
    heap.EndQuery(context, _end);

    const auto queue = _device->GetCommandQueue()->GetCommandQueue();
    VALIDATE_DIRECTX_CALL(queue->GetTimestampFrequency(&_gpuFrequency));

    _endCalled = true;
}

bool GPUTimerQueryDX12::HasResult()
{
    if (!_endCalled)
        return false;
    if (_hasResult)
        return true;

    auto& heap = _device->TimestampQueryHeap;
    return heap.IsReady(_end) && heap.IsReady(_begin);
}

float GPUTimerQueryDX12::GetResult()
{
    if (_hasResult)
    {
        return _timeDelta;
    }

    const uint64 timeBegin = *(uint64*)_device->TimestampQueryHeap.ResolveQuery(_begin);
    const uint64 timeEnd = *(uint64*)_device->TimestampQueryHeap.ResolveQuery(_end);

    // Calculate event duration in milliseconds
    if (timeEnd > timeBegin)
    {
        const uint64 delta = timeEnd - timeBegin;
        const double frequency = double(_gpuFrequency);
        _timeDelta = static_cast<float>((delta / frequency) * 1000.0);
    }
    else
    {
        _timeDelta = 0.0f;
    }

    _hasResult = true;
    return _timeDelta;
}

#endif
