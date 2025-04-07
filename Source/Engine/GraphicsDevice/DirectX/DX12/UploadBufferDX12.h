// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "GPUDeviceDX12.h"
#include "ResourceOwnerDX12.h"

#if GRAPHICS_API_DIRECTX12

#define DX12_DEFAULT_UPLOAD_PAGE_SIZE (4 * 1014 * 1024) // 4 MB

// Upload buffer generations timeout to dispose
#define DX12_UPLOAD_PAGE_GEN_TIMEOUT DX12_BACK_BUFFER_COUNT

// Upload buffer pages that are not used for a few frames are disposed
#define DX12_UPLOAD_PAGE_NOT_USED_FRAME_TIMEOUT 60

class GPUTextureDX12;

/// <summary>
/// Single page for the upload buffer
/// </summary>
class UploadBufferPageDX12 : public GPUResourceDX12<GPUResource>, public ResourceOwnerDX12
{
public:

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="device">Graphics Device</param>
    /// <param name="size">Page size</param>
    UploadBufferPageDX12(GPUDeviceDX12* device, uint64 size);

public:

    /// <summary>
    /// Last generation that has been using that page
    /// </summary>
    uint64 LastGen;

    /// <summary>
    /// CPU memory address of the page
    /// </summary>
    void* CPUAddress;

    /// <summary>
    /// GPU memory address of the page
    /// </summary>
    D3D12_GPU_VIRTUAL_ADDRESS GPUAddress;

    /// <summary>
    /// Page size in bytes
    /// </summary>
    uint64 Size;

public:

    // [GPUResourceDX12]
    GPUResourceType GetResourceType() const final override
    {
        return GPUResourceType::Buffer;
    }

    // [ResourceOwnerDX12]
    GPUResource* AsGPUResource() const override
    {
        return (GPUResource*)this;
    }

protected:

    // [GPUResourceDX12]
    void OnReleaseGPU() final override;
};

/// <summary>
/// Upload buffer allocation
/// </summary>
struct DynamicAllocation
{
    /// <summary>
    /// CPU memory address of the allocation start.
    /// </summary>
    void* CPUAddress;

    /// <summary>
    /// Allocation offset in bytes (from the start of the heap buffer).
    /// </summary>
    uint64 Offset;

    /// <summary>
    /// Allocation size in bytes
    /// </summary>
    uint64 Size;

    /// <summary>
    /// GPU virtual memory address of the allocation start.
    /// </summary>
    D3D12_GPU_VIRTUAL_ADDRESS GPUAddress;

    /// <summary>
    /// Upload buffer page that owns that allocation
    /// </summary>
    UploadBufferPageDX12* Page;

    /// <summary>
    /// Generation number of that allocation (generally allocation is invalid after one or two generations)
    /// </summary>
    uint64 Generation;

    /// <summary>
    /// Init
    /// </summary>
    DynamicAllocation()
        : CPUAddress(nullptr)
        , Offset(0)
        , Size(0)
        , GPUAddress(0)
        , Page(nullptr)
        , Generation(0)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="address">CPU memory address</param>
    /// <param name="offset">Offset in byes</param>
    /// <param name="size">Size in byes</param>
    /// <param name="gpuAddress">GPU memory address</param>
    /// <param name="page">Parent page</param>
    /// <param name="generation">Generation</param>
    DynamicAllocation(void* address, uint64 offset, uint64 size, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, UploadBufferPageDX12* page, uint64 generation)
        : CPUAddress(address)
        , Offset(offset)
        , Size(size)
        , GPUAddress(gpuAddress)
        , Page(page)
        , Generation(generation)
    {
    }

    /// <summary>
    /// Returns true if allocation is invalid.
    /// </summary>
    bool IsInvalid() const
    {
        return CPUAddress == nullptr || Size == 0 || Page == nullptr;
    }
};

/// <summary>
/// Uploading data to GPU buffer utility
/// </summary>
class UploadBufferDX12
{
private:

    GPUDeviceDX12* _device;
    UploadBufferPageDX12* _currentPage;
    uint64 _currentOffset;
    uint64 _currentGeneration;

    Array<UploadBufferPageDX12*, InlinedAllocation<64>> _freePages;
    Array<UploadBufferPageDX12*, InlinedAllocation<64>> _usedPages;

public:

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="device">Graphics Device</param>
    UploadBufferDX12(GPUDeviceDX12* device);

    /// <summary>
    /// Destructor
    /// </summary>
    ~UploadBufferDX12();

public:

    /// <summary>
    /// Gets the current generation number.
    /// </summary>
    FORCE_INLINE uint64 GetCurrentGeneration() const
    {
        return _currentGeneration;
    }

public:

    /// <summary>
    /// Allocates memory for custom data in the buffer.
    /// </summary>
    /// <param name="size">Size of the data in bytes</param>
    /// <param name="align">Data alignment in buffer in bytes</param>
    /// <returns>Dynamic location</returns>
    DynamicAllocation Allocate(uint64 size, uint64 align);

    /// <summary>
    /// Uploads data to the buffer.
    /// </summary>
    /// <param name="context">GPU context to record upload command to it</param>
    /// <param name="buffer">Destination buffer</param>
    /// <param name="bufferOffset">Destination buffer offset in bytes.</param>
    /// <param name="data">Data to allocate</param>
    /// <param name="size">Size of the data in bytes</param>
    /// <returns>True if cannot upload data, otherwise false.</returns>
    bool UploadBuffer(GPUContextDX12* context, ID3D12Resource* buffer, uint32 bufferOffset, const void* data, uint64 size);

    /// <summary>
    /// Uploads data to the texture.
    /// </summary>
    /// <param name="context">GPU context to record upload command to it</param>
    /// <param name="texture">Destination texture</param>
    /// <param name="srcData">Data to allocate</param>
    /// <param name="srcRowPitch">Source data row pitch value to upload.</param>
    /// <param name="srcSlicePitch">Source data slice pitch value to upload.</param>
    /// <param name="mipIndex">Mip map to stream index</param>
    /// <param name="arrayIndex">Texture array index</param>
    /// <returns>True if cannot upload data, otherwise false.</returns>
    bool UploadTexture(GPUContextDX12* context, ID3D12Resource* texture, const void* srcData, uint32 srcRowPitch, uint32 srcSlicePitch, int32 mipIndex, int32 arrayIndex);

public:

    /// <summary>
    /// Begins new generation.
    /// </summary>
    /// <param name="generation">The generation ID to begin.</param>
    void BeginGeneration(uint64 generation);

private:

    UploadBufferPageDX12* requestPage(uint64 size);
};

#endif
