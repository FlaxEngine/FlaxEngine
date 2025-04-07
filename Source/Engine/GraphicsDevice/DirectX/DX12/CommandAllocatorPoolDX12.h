// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../IncludeDirectXHeaders.h"

#if GRAPHICS_API_DIRECTX12

#include "Engine/Core/Types/Pair.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Platform/CriticalSection.h"

class GPUDeviceDX12;

class CommandAllocatorPoolDX12
{
private:

    typedef Pair<uint64, ID3D12CommandAllocator*> PoolPair;

    const D3D12_COMMAND_LIST_TYPE _type;
    GPUDeviceDX12* _device;
    Array<ID3D12CommandAllocator*> _pool;
    Array<PoolPair> _ready;
    CriticalSection _locker;

public:

    CommandAllocatorPoolDX12(GPUDeviceDX12* device, D3D12_COMMAND_LIST_TYPE type);
    ~CommandAllocatorPoolDX12();

public:

    FORCE_INLINE uint32 Size() const
    {
        return _pool.Count();
    }

public:

    ID3D12CommandAllocator* RequestAllocator(uint64 completedFenceValue);

    void DiscardAllocator(uint64 fenceValue, ID3D12CommandAllocator* allocator);

    void Release();
};

#endif
