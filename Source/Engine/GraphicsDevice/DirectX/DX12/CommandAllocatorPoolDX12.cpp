// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "CommandAllocatorPoolDX12.h"
#include "GPUDeviceDX12.h"
#include "Engine/Threading/Threading.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"

CommandAllocatorPoolDX12::CommandAllocatorPoolDX12(GPUDeviceDX12* device, D3D12_COMMAND_LIST_TYPE type)
    : _type(type)
    , _device(device)
{
}

CommandAllocatorPoolDX12::~CommandAllocatorPoolDX12()
{
    Release();
}

ID3D12CommandAllocator* CommandAllocatorPoolDX12::RequestAllocator(uint64 completedFenceValue)
{
    ScopeLock lock(_locker);

    ID3D12CommandAllocator* allocator = nullptr;

    if (_ready.HasItems())
    {
        PoolPair& firstPair = _ready.First();

        if (firstPair.First <= completedFenceValue)
        {
            allocator = firstPair.Second;
            VALIDATE_DIRECTX_CALL(allocator->Reset());
            _ready.RemoveAtKeepOrder(0);
        }
    }

    // If no allocators were ready to be reused, create a new one
    if (allocator == nullptr)
    {
        VALIDATE_DIRECTX_CALL(_device->GetDevice()->CreateCommandAllocator(_type, IID_PPV_ARGS(&allocator)));
#if GPU_ENABLE_RESOURCE_NAMING
        Char name[32];
        swprintf(name, 32, L"CommandAllocator %u", _pool.Count());
        allocator->SetName(name);
#endif
        _pool.Add(allocator);
    }

    return allocator;
}

void CommandAllocatorPoolDX12::DiscardAllocator(uint64 fenceValue, ID3D12CommandAllocator* allocator)
{
    ScopeLock lock(_locker);

    // That fence value indicates we are free to reset the allocator
    _ready.Add(PoolPair(fenceValue, allocator));
}

void CommandAllocatorPoolDX12::Release()
{
    for (int32 i = 0; i < _pool.Count(); i++)
    {
        DX_SAFE_RELEASE_CHECK(_pool[i], 0);
    }

    _pool.Clear();
}

#endif
