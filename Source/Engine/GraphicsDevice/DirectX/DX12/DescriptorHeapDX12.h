// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX12

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

        D3D12_CPU_DESCRIPTOR_HANDLE CPU() const;
        D3D12_GPU_DESCRIPTOR_HANDLE GPU() const;

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

    // Get heap
    FORCE_INLINE operator ID3D12DescriptorHeap*() const
    {
        return _heap;
    }

public:

    // Create heap data
    // @param type Heap data type
    // @param descriptorsCount Amount of descriptors to use
    // @param shaderVisible True if allow shaders to access heap data
    bool Create(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 descriptorsCount, bool shaderVisible = false);

public:

    // Tries to find free descriptor slot
    // @param index Result index to use
    // @returns True if can assign descriptor to the heap
    bool TryToGetUnusedSlot(uint32& index);

    // Release descriptor slot
    // @param index Descriptor index in the heap
    void ReleaseSlot(uint32 index);

public:

    // Get handle to the CPU view at given index
    // @param index Descriptor index
    // @returns CPU address
    FORCE_INLINE D3D12_CPU_DESCRIPTOR_HANDLE CPU(uint32 index)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle;
        handle.ptr = _beginCPU.ptr + (SIZE_T)(index * _incrementSize);
        return handle;
    }

    // Get handle to the GPU view at given index
    // @param index Descriptor index
    // @returns GPU address
    FORCE_INLINE D3D12_GPU_DESCRIPTOR_HANDLE GPU(uint32 index)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE handle;
        handle.ptr = _beginGPU.ptr + index * _incrementSize;
        return handle;
    }

public:

    // [GPUResourceDX12]
    ResourceType GetResourceType() const final override
    {
        return ResourceType::Descriptor;
    }

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

    /// <summary>
    /// Gets DirectX 12 heap object
    /// </summary>
    /// <returns>Heap object</returns>
    FORCE_INLINE ID3D12DescriptorHeap* GetHeap() const
    {
        return _heap;
    }

public:

    // Setup heap
    // @returns True if cannot setup heap, otherwise false
    bool Init();

    // Allocate memory for descriptors table
    // @param numDesc Amount of descriptors in table
    // @returns Allocated data (GPU param is valid only for shader visible heaps)
    Allocation AllocateTable(uint32 numDesc);

public:

    // [GPUResourceDX12]
    ResourceType GetResourceType() const final override
    {
        return ResourceType::Descriptor;
    }

protected:

    // [GPUResourceDX12]
    void OnReleaseGPU() override;
};

#endif
