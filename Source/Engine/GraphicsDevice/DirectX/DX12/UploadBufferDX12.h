// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUDevice.h"
#include "ResourceOwnerDX12.h"

#if GRAPHICS_API_DIRECTX12

class GPUDeviceDX12;
class UploadBufferPageDX12;

// Upload buffer page size
#define DX12_DEFAULT_UPLOAD_PAGE_SIZE (4 * 1014 * 1024) // 4 MB

// Upload buffer generations timeout to dispose
#define DX12_UPLOAD_PAGE_GEN_TIMEOUT DX12_BACK_BUFFER_COUNT

// Upload buffer pages that are not used for a few frames are disposed
#define DX12_UPLOAD_PAGE_NOT_USED_FRAME_TIMEOUT 60

/// <summary>
/// Uploading data to GPU buffer utility
/// </summary>
class UploadBufferDX12
{
public:
    /// <summary>
    /// Upload buffer allocation
    /// </summary>
    struct Allocation
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
        /// Upload buffer page resource that owns that allocation
        /// </summary>
        ID3D12Resource* Resource;

        /// <summary>
        /// Generation number of that allocation (generally allocation is invalid after one or two generations)
        /// </summary>
        uint64 Generation;
    };

private:
    GPUDeviceDX12* _device;
    UploadBufferPageDX12* _currentPage;
    uint64 _currentOffset;
    uint64 _currentGeneration;
    Array<UploadBufferPageDX12*, InlinedAllocation<64>> _freePages;
    Array<UploadBufferPageDX12*, InlinedAllocation<64>> _usedPages;

public:
    UploadBufferDX12(GPUDeviceDX12* device);

public:
    /// <summary>
    /// Allocates memory for custom data in the buffer.
    /// </summary>
    /// <param name="size">Size of the data in bytes</param>
    /// <param name="align">Data alignment in buffer in bytes</param>
    /// <returns>Dynamic location</returns>
    Allocation Allocate(uint64 size, uint64 align);

    /// <summary>
    /// Uploads data to the buffer.
    /// </summary>
    /// <param name="commandList">GPU command list to record upload command to it</param>
    /// <param name="buffer">Destination buffer</param>
    /// <param name="bufferOffset">Destination buffer offset in bytes.</param>
    /// <param name="data">Data to allocate</param>
    /// <param name="size">Size of the data in bytes</param>
    void UploadBuffer(ID3D12GraphicsCommandList* commandList, ID3D12Resource* buffer, uint32 bufferOffset, const void* data, uint64 size);

    /// <summary>
    /// Uploads data to the texture.
    /// </summary>
    /// <param name="commandList">GPU command list to record upload command to it</param>
    /// <param name="texture">Destination texture</param>
    /// <param name="srcData">Data to allocate</param>
    /// <param name="srcRowPitch">Source data row pitch value to upload.</param>
    /// <param name="srcSlicePitch">Source data slice pitch value to upload.</param>
    /// <param name="mipIndex">Mip map to stream index</param>
    /// <param name="arrayIndex">Texture array index</param>
    void UploadTexture(ID3D12GraphicsCommandList* commandList, ID3D12Resource* texture, const void* srcData, uint32 srcRowPitch, uint32 srcSlicePitch, int32 mipIndex, int32 arrayIndex);

public:
    void BeginGeneration(uint64 generation);
    void ReleaseGPU();
};

#endif
