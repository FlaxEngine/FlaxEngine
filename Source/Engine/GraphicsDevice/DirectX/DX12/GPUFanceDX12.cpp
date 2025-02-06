// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "GPUFanceDX12.h"
#include "../RenderToolsDX.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"
#include "GPUContextDX12.h"

GPUFenceDX12::GPUFenceDX12(GPUDeviceDX12* device, const StringView& name)
    : GPUResourceDX12<GPUFence>(device, name)
{
    // Create a DX12 fence
    ID3D12Device* DX12Device = device->GetDevice();

    // Initialize fence value
    fenceValue = 0;

    // Create the fence object
    if (FAILED(DX12Device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
    {
        fence = nullptr;
        return;
    }
}
void GPUFenceDX12::Signal()
{
    if (fence == nullptr)
        return;

    // Signal the fence that the GPU has completed a task
    ID3D12CommandQueue* commandQueue = _device->GetCommandQueueDX12();

    // Signal the fence to indicate the GPU has completed tasks
    commandQueue->Signal(fence, ++fenceValue);
    SignalCalled = true;
}
void GPUFenceDX12::Wait()
{
    // Only wait if Signal() has been called
    if (!SignalCalled || fence == nullptr)
        return;

    // Wait until the GPU has finished processing the fence signal
    while (fence->GetCompletedValue() < fenceValue)
    {
        Platform::Sleep(1);
    }
}

GPUFenceDX12::~GPUFenceDX12()
{
    if (fence)
        fence->Release();
}

#endif
