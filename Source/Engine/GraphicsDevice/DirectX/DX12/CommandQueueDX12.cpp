// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "CommandQueueDX12.h"
#include "GPUDeviceDX12.h"
#include "Engine/Threading/Threading.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"

FenceDX12::FenceDX12(GPUDeviceDX12* device)
    : _currentValue(1)
    , _lastSignaledValue(0)
    , _lastCompletedValue(0)
    , _device(device)
{
}

bool FenceDX12::Init()
{
    LOG_DIRECTX_RESULT(_device->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));
#if GPU_ENABLE_RESOURCE_NAMING
    _fence->SetName(TEXT("Fence"));
#endif

    _event = CreateEvent(nullptr, false, false, nullptr);
    ASSERT(_event != INVALID_HANDLE_VALUE);
    return false;
}

void FenceDX12::Release()
{
    CloseHandle(_event);
    _event = nullptr;

    _fence->Release();
    _fence = nullptr;
}

uint64 FenceDX12::Signal(CommandQueueDX12* queue)
{
    ScopeLock lock(_locker);
    ASSERT(_lastSignaledValue != _currentValue);

    // Insert signal into command queue
    LOG_DIRECTX_RESULT(queue->GetCommandQueue()->Signal(_fence, _currentValue));

    // Update state
    _lastSignaledValue = _currentValue;
    _lastCompletedValue = _fence->GetCompletedValue();

    // Increment the current value
    _currentValue++;

    return _lastSignaledValue;
}

void FenceDX12::WaitGPU(CommandQueueDX12* queue, uint64 value)
{
    // Insert wait into command queue
    LOG_DIRECTX_RESULT(queue->GetCommandQueue()->Wait(_fence, value));
}

void FenceDX12::WaitCPU(uint64 value)
{
    if (IsFenceComplete(value))
        return;

    ScopeLock lock(_locker);

    _fence->SetEventOnCompletion(value, _event);
    WaitForSingleObject(_event, INFINITE);

    _lastCompletedValue = _fence->GetCompletedValue();
}

bool FenceDX12::IsFenceComplete(uint64 value)
{
    ASSERT(value <= _currentValue);

    if (value > _lastCompletedValue)
    {
        _lastCompletedValue = _fence->GetCompletedValue();
    }

    return value <= _lastCompletedValue;
}

CommandQueueDX12::CommandQueueDX12(GPUDeviceDX12* device, D3D12_COMMAND_LIST_TYPE type)
    : _device(device)
    , _commandQueue(nullptr)
    , _type(type)
    , _allocatorPool(device, type)
    , _fence(device)
{
}

CommandQueueDX12::~CommandQueueDX12()
{
    Release();
}

bool CommandQueueDX12::Init()
{
    ASSERT(_device != nullptr);
    ASSERT(!IsReady());
    ASSERT(_allocatorPool.Size() == 0);

    D3D12_COMMAND_QUEUE_DESC desc;
    desc.Type = _type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;
    HRESULT result = _device->GetDevice()->CreateCommandQueue(&desc, IID_PPV_ARGS(&_commandQueue));
    LOG_DIRECTX_RESULT_WITH_RETURN(result, true);
#if GPU_ENABLE_RESOURCE_NAMING
    _commandQueue->SetName(TEXT("CommandQueueDX12::CommandQueue"));
#endif

    if (_fence.Init())
        return true;

    ASSERT(IsReady());
    return false;
}

void CommandQueueDX12::Release()
{
    if (_commandQueue == nullptr)
        return;

    _allocatorPool.Release();
    _fence.Release();

    _commandQueue->Release();
    _commandQueue = nullptr;
}

void CommandQueueDX12::WaitForFence(uint64 fenceValue)
{
    _fence.WaitCPU(fenceValue);
}

void CommandQueueDX12::WaitForGPU()
{
    const uint64 value = _fence.Signal(this);
    _fence.WaitCPU(value);
}

uint64 CommandQueueDX12::ExecuteCommandList(ID3D12CommandList* list)
{
    VALIDATE_DIRECTX_CALL((static_cast<ID3D12GraphicsCommandList*>(list))->Close());

    _commandQueue->ExecuteCommandLists(1, &list);

    return _fence.Signal(this);
}

ID3D12CommandAllocator* CommandQueueDX12::RequestAllocator()
{
    const uint64 completedFence = _fence.GetLastCompletedValue();
    return _allocatorPool.RequestAllocator(completedFence);
}

void CommandQueueDX12::DiscardAllocator(uint64 fenceValue, ID3D12CommandAllocator* allocator)
{
    _allocatorPool.DiscardAllocator(fenceValue, allocator);
}

#endif
