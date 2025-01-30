// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX12

#include "Engine/Core/Collections/Array.h"
#include "Engine/Graphics/GPUResource.h"
#include "../IncludeDirectXHeaders.h"

class DescriptorHeapPoolDX12;
class GPUDeviceDX12;

/// <summary>
/// Descriptors heap for DirectX 12 that bit array concept to implement descriptor slots allocation.
/// </summary>
class DescriptorHeapWithSlotsDX12 : public GPUResource
{
public:

    struct Slot
    {
        DescriptorHeapWithSlotsDX12* Heap = nullptr;
        uint32 Index;

        FORCE_INLINE bool IsValid() const
        {
            return Heap != nullptr;
        }

#if BUILD_DEBUG
        ~Slot()
        {
            ASSERT(Heap == nullptr);
        }
#endif

        FORCE_INLINE D3D12_CPU_DESCRIPTOR_HANDLE CPU() const
        {
            return Heap ? Heap->CPU(Index) : D3D12_CPU_DESCRIPTOR_HANDLE {};
        }

        FORCE_INLINE D3D12_GPU_DESCRIPTOR_HANDLE GPU() const
        {
            return Heap ? Heap->GPU(Index) : D3D12_GPU_DESCRIPTOR_HANDLE {};
        }

        void CreateSRV(GPUDeviceDX12* device, ID3D12Resource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC* desc = nullptr);
        void CreateRTV(GPUDeviceDX12* device, ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC* desc = nullptr);
        void CreateDSV(GPUDeviceDX12* device, ID3D12Resource* resource, D3D12_DEPTH_STENCIL_VIEW_DESC* desc = nullptr);
        void CreateUAV(GPUDeviceDX12* device, ID3D12Resource* resource, D3D12_UNORDERED_ACCESS_VIEW_DESC* desc = nullptr, ID3D12Resource* counterResource = nullptr);

        void Release();
    };

private:

    GPUDeviceDX12* _device;
    ID3D12DescriptorHeap* _heap;
    D3D12_CPU_DESCRIPTOR_HANDLE _beginCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE _beginGPU;
    D3D12_DESCRIPTOR_HEAP_TYPE _type;
    uint32 _incrementSize;
    uint32 _descriptorsCount;
    bool _shaderVisible;
    Array<uint32> _usage;

public:

    DescriptorHeapWithSlotsDX12(GPUDeviceDX12* device);

public:

    FORCE_INLINE D3D12_CPU_DESCRIPTOR_HANDLE CPU(uint32 index)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle;
        handle.ptr = _beginCPU.ptr + (SIZE_T)(index * _incrementSize);
        return handle;
    }

    FORCE_INLINE D3D12_GPU_DESCRIPTOR_HANDLE GPU(uint32 index)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE handle;
        handle.ptr = _beginGPU.ptr + index * _incrementSize;
        return handle;
    }

public:

    bool Create(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 descriptorsCount, bool shaderVisible = false);
    bool TryToGetUnusedSlot(uint32& index);
    void ReleaseSlot(uint32 index);

public:

    // [GPUResourceDX12]
    GPUResourceType GetResourceType() const final override;

protected:

    // [GPUResourceDX12]
    void OnReleaseGPU() override;
};

/// <summary>
/// Descriptors heap pool for DirectX 12.
/// </summary>
class DescriptorHeapPoolDX12
{
private:

    GPUDeviceDX12* _device;
    D3D12_DESCRIPTOR_HEAP_TYPE _type;
    uint32 _descriptorsCountPerHeap;
    bool _shaderVisible;
    Array<DescriptorHeapWithSlotsDX12*, InlinedAllocation<32>> _heaps;

public:

    DescriptorHeapPoolDX12(GPUDeviceDX12* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 descriptorsCountPerHeap, bool shaderVisible);

public:

    void AllocateSlot(DescriptorHeapWithSlotsDX12*& heap, uint32& slot);
    void ReleaseGPU();
};

/// <summary>
/// Descriptors heap for DirectX 12 that uses a ring buffer concept to implement descriptor tables allocation.
/// </summary>
class DescriptorHeapRingBufferDX12 : public GPUResource
{
public:

    /// <summary>
    /// Heap allocation info
    /// </summary>
    struct Allocation
    {
        /// <summary>
        /// Handle in CPU memory
        /// </summary>
        D3D12_CPU_DESCRIPTOR_HANDLE CPU;

        /// <summary>
        /// Handle in GPU memory
        /// </summary>
        D3D12_GPU_DESCRIPTOR_HANDLE GPU;
    };

private:

    GPUDeviceDX12* _device;
    ID3D12DescriptorHeap* _heap;
    D3D12_CPU_DESCRIPTOR_HANDLE _beginCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE _beginGPU;
    D3D12_DESCRIPTOR_HEAP_TYPE _type;
    uint32 _incrementSize;
    uint32 _descriptorsCount;
    uint32 _firstFree;
    bool _shaderVisible;

public:

    DescriptorHeapRingBufferDX12(GPUDeviceDX12* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 descriptorsCount, bool shaderVisible);

public:

    FORCE_INLINE ID3D12DescriptorHeap* GetHeap() const
    {
        return _heap;
    }

    bool Init();
    Allocation AllocateTable(uint32 numDesc);

public:

    // [GPUResourceDX12]
    GPUResourceType GetResourceType() const final override
    {
        return GPUResourceType::Descriptor;
    }

protected:

    // [GPUResourceDX12]
    void OnReleaseGPU() override;
};

#endif
