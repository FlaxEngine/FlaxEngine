// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "DescriptorHeapDX12.h"
#include "GPUDeviceDX12.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"

void DescriptorHeapWithSlotsDX12::Slot::CreateSRV(GPUDeviceDX12* device, ID3D12Resource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC* desc)
{
    if (Heap == nullptr)
        device->Heap_CBV_SRV_UAV.AllocateSlot(Heap, Index);
    device->GetDevice()->CreateShaderResourceView(resource, desc, CPU());
}

void DescriptorHeapWithSlotsDX12::Slot::CreateRTV(GPUDeviceDX12* device, ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC* desc)
{
    if (Heap == nullptr)
        device->Heap_RTV.AllocateSlot(Heap, Index);
    device->GetDevice()->CreateRenderTargetView(resource, desc, CPU());
}

void DescriptorHeapWithSlotsDX12::Slot::CreateDSV(GPUDeviceDX12* device, ID3D12Resource* resource, D3D12_DEPTH_STENCIL_VIEW_DESC* desc)
{
    if (Heap == nullptr)
        device->Heap_DSV.AllocateSlot(Heap, Index);
    device->GetDevice()->CreateDepthStencilView(resource, desc, CPU());
}

void DescriptorHeapWithSlotsDX12::Slot::CreateUAV(GPUDeviceDX12* device, ID3D12Resource* resource, D3D12_UNORDERED_ACCESS_VIEW_DESC* desc, ID3D12Resource* counterResource)
{
    if (Heap == nullptr)
        device->Heap_CBV_SRV_UAV.AllocateSlot(Heap, Index);
    device->GetDevice()->CreateUnorderedAccessView(resource, counterResource, desc, CPU());
}

void DescriptorHeapWithSlotsDX12::Slot::Release()
{
    if (Heap)
    {
        Heap->ReleaseSlot(Index);
        Heap = nullptr;
    }
}

DescriptorHeapWithSlotsDX12::DescriptorHeapWithSlotsDX12(GPUDeviceDX12* device)
    : _device(device)
    , _heap(nullptr)
    , _descriptorsCount(0)
{
}

bool DescriptorHeapWithSlotsDX12::Create(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 descriptorsCount, bool shaderVisible)
{
    // Create description
    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.Type = type;
    desc.NumDescriptors = descriptorsCount;
    desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;

    // Create heap
    const HRESULT result = _device->GetDevice()->CreateDescriptorHeap(&desc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&_heap));
    LOG_DIRECTX_RESULT_WITH_RETURN(result, true);

    // Setup
    _type = type;
    _shaderVisible = shaderVisible;
    _descriptorsCount = descriptorsCount;
    _beginCPU = _heap->GetCPUDescriptorHandleForHeapStart();
    if (shaderVisible)
        _beginGPU = _heap->GetGPUDescriptorHandleForHeapStart();
    else
        _beginGPU.ptr = 0;
    _incrementSize = _device->GetDevice()->GetDescriptorHandleIncrementSize(desc.Type);

    // Setup usage cache
    _usage.Resize(static_cast<int32>(_descriptorsCount / 32), false);
    Platform::MemorySet(_usage.Get(), _usage.Count() * sizeof(uint32), 0);

    _memoryUsage = 1;

    return false;
}

bool DescriptorHeapWithSlotsDX12::TryToGetUnusedSlot(uint32& index)
{
    for (int32 i = 0; i < _usage.Count(); i++)
    {
        uint32& value = _usage[i];
        if (value != MAX_uint32)
        {
            // TODO: make it better?
            for (int32 bit = 0; bit < 32; bit++)
            {
                const uint32 mask = 1 << bit;
                if ((value & mask) == 0)
                {
                    // Found
                    index = i * 32 + bit;
                    value |= mask;
                    return true;
                }
            }
        }
    }

    return false;
}

void DescriptorHeapWithSlotsDX12::ReleaseSlot(uint32 index)
{
    uint32& value = _usage[index / 32];
    const uint32 mask = 1 << (index & 31);
    ASSERT_LOW_LAYER((value & mask) == mask);
    value &= ~mask;
}

GPUResourceType DescriptorHeapWithSlotsDX12::GetResourceType() const
{
    return GPUResourceType::Descriptor;
}

DescriptorHeapPoolDX12::DescriptorHeapPoolDX12(GPUDeviceDX12* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 descriptorsCountPerHeap, bool shaderVisible)
    : _device(device)
    , _type(type)
    , _descriptorsCountPerHeap(descriptorsCountPerHeap)
    , _shaderVisible(shaderVisible)
{
}

void DescriptorHeapPoolDX12::AllocateSlot(DescriptorHeapWithSlotsDX12*& heap, uint32& slot)
{
    for (int32 i = 0; i < _heaps.Count(); i++)
    {
        if (_heaps[i]->TryToGetUnusedSlot(slot))
        {
            heap = _heaps[i];
            return;
        }
    }

    heap = New<DescriptorHeapWithSlotsDX12>(_device);
    if (heap->Create(_type, _descriptorsCountPerHeap, _shaderVisible))
    {
        Platform::Fatal(TEXT("Failed to allocate descriptor heap."));
    }
    _heaps.Add(heap);
    heap->TryToGetUnusedSlot(slot);
}

void DescriptorHeapPoolDX12::ReleaseGPU()
{
    for (auto heap : _heaps)
    {
        heap->ReleaseGPU();
        Delete(heap);
    }
    _heaps.Clear();
}

void DescriptorHeapWithSlotsDX12::OnReleaseGPU()
{
    _usage.SetCapacity(0, false);
    DX_SAFE_RELEASE_CHECK(_heap, 0);
    _descriptorsCount = 0;
}

DescriptorHeapRingBufferDX12::DescriptorHeapRingBufferDX12(GPUDeviceDX12* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 descriptorsCount, bool shaderVisible)
    : _device(device)
    , _heap(nullptr)
    , _type(type)
    , _descriptorsCount(descriptorsCount)
    , _shaderVisible(shaderVisible)
{
}

bool DescriptorHeapRingBufferDX12::Init()
{
    // Create heap
    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.Type = _type;
    desc.NumDescriptors = _descriptorsCount;
    desc.Flags = _shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;
    const HRESULT result = _device->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_heap));
    LOG_DIRECTX_RESULT_WITH_RETURN(result, true);

    // Setup
    _firstFree = 0;
    _beginCPU = _heap->GetCPUDescriptorHandleForHeapStart();
    if (_shaderVisible)
        _beginGPU = _heap->GetGPUDescriptorHandleForHeapStart();
    else
        _beginGPU.ptr = 0;
    _incrementSize = _device->GetDevice()->GetDescriptorHandleIncrementSize(desc.Type);
    _memoryUsage = 1;

    return false;
}

DescriptorHeapRingBufferDX12::Allocation DescriptorHeapRingBufferDX12::AllocateTable(uint32 numDesc)
{
    Allocation result;

    // Move the ring buffer pointer
    uint32 index = _firstFree;
    _firstFree += numDesc;

    // Check for overflow
    if (_firstFree >= _descriptorsCount)
    {
        // Move to the begin
        index = 0;
        _firstFree = numDesc;
    }

    // Set pointers
    result.CPU.ptr = _beginCPU.ptr + static_cast<SIZE_T>(index * _incrementSize);
    result.GPU.ptr = _shaderVisible ? _beginGPU.ptr + index * _incrementSize : 0;

    return result;
}

void DescriptorHeapRingBufferDX12::OnReleaseGPU()
{
    DX_SAFE_RELEASE_CHECK(_heap, 0);
    _firstFree = 0;
}

#endif
