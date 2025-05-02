// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Platform/CriticalSection.h"
#include "CommandAllocatorPoolDX12.h"
#include "../IncludeDirectXHeaders.h"

#if GRAPHICS_API_DIRECTX12

class GPUDeviceDX12;
class GPUContextDX12;
class CommandQueueDX12;

/// <summary>
/// Wraps a fence object and provides functionality for common operations for GPU/CPU operations synchronization.
/// </summary>
class FenceDX12
{
private:

    uint64 _currentValue;
    uint64 _lastSignaledValue;
    uint64 _lastCompletedValue;
    HANDLE _event;
    ID3D12Fence* _fence;
    GPUDeviceDX12* _device;
    CriticalSection _locker;

public:

    FenceDX12(GPUDeviceDX12* device);

public:

    FORCE_INLINE uint64 GetCurrentValue() const
    {
        return _currentValue;
    }

    FORCE_INLINE uint64 GetLastSignaledValue() const
    {
        return _lastSignaledValue;
    }

    FORCE_INLINE uint64 GetLastCompletedValue() const
    {
        return _lastCompletedValue;
    }

public:

    bool Init();
    void Release();
    uint64 Signal(CommandQueueDX12* queue);
    void WaitGPU(CommandQueueDX12* queue, uint64 value);
    void WaitCPU(uint64 value);
    bool IsFenceComplete(uint64 value);
};

/// <summary>
/// GPU commands execution sync point for DirectX 12.
/// </summary>
struct SyncPointDX12
{
    FenceDX12* Fence;
    uint64 Value;

    SyncPointDX12()
        : Fence(nullptr)
        , Value(0)
    {
    }

    SyncPointDX12(FenceDX12* fence, uint64 value)
        : Fence(fence)
        , Value(value)
    {
    }

    SyncPointDX12(const SyncPointDX12& other)
        : Fence(other.Fence)
        , Value(other.Value)
    {
    }

    SyncPointDX12& operator=(const SyncPointDX12& other)
    {
        Fence = other.Fence;
        Value = other.Value;
        return *this;
    }

    bool operator!() const
    {
        return Fence == nullptr;
    }

    bool IsValid() const
    {
        return Fence != nullptr;
    }

    bool IsOpen() const
    {
        return Value == Fence->GetCurrentValue();
    }

    bool IsComplete() const
    {
        return Fence->IsFenceComplete(Value);
    }

    void WaitForCompletion() const
    {
        Fence->WaitCPU(Value);
    }
};

class CommandQueueDX12
{
    friend GPUDeviceDX12;
    friend GPUContextDX12;

private:

    GPUDeviceDX12* _device;
    ID3D12CommandQueue* _commandQueue;
    const D3D12_COMMAND_LIST_TYPE _type;
    CommandAllocatorPoolDX12 _allocatorPool;
    FenceDX12 _fence;

public:

    CommandQueueDX12(GPUDeviceDX12* device, D3D12_COMMAND_LIST_TYPE type);
    ~CommandQueueDX12();

public:

    FORCE_INLINE bool IsReady() const
    {
        return _commandQueue != nullptr;
    }

    FORCE_INLINE ID3D12CommandQueue* GetCommandQueue() const
    {
        return _commandQueue;
    }

    FORCE_INLINE CommandAllocatorPoolDX12& GetAllocatorPool()
    {
        return _allocatorPool;
    }

    FORCE_INLINE SyncPointDX12 GetSyncPoint()
    {
        return SyncPointDX12(&_fence, _fence.GetCurrentValue());
    }

public:

    /// <summary>
    /// Init resources
    /// </summary>
    /// <returns>True if cannot init, otherwise false</returns>
    bool Init();

    /// <summary>
    /// Cleanup all stuff
    /// </summary>
    void Release();

public:

    /// <summary>
    /// Stalls the execution on current thread to wait for the GPU to step over given fence value.
    /// </summary>
    /// <param name="fenceValue">The fence value to wait.</param>
    void WaitForFence(uint64 fenceValue);

    /// <summary>
    /// Stalls the execution on current thread to wait for the GPU to finish it's job.
    /// </summary>
    void WaitForGPU();

public:

    /// <summary>
    /// Executes a command list.
    /// </summary>
    /// <param name="list">The command list to execute.</param>
    /// <returns>The fence value after execution (can be used to wait for it to sync parallel execution).</returns>
    uint64 ExecuteCommandList(ID3D12CommandList* list);

    /// <summary>
    /// Requests new clean allocator to use.
    /// </summary>
    /// <returns>The allocator.</returns>
    ID3D12CommandAllocator* RequestAllocator();

    /// <summary>
    /// Discards used allocator.
    /// </summary>
    /// <param name="fenceValueForReset">Sync fence value on reset event.</param>
    /// <param name="allocator">The allocator to discard.</param>
    void DiscardAllocator(uint64 fenceValueForReset, ID3D12CommandAllocator* allocator);
};

#endif
