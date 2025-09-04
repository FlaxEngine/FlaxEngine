// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUDevice.h"
#include "ResourceOwnerVulkan.h"

#if GRAPHICS_API_VULKAN

class GPUDeviceVulkan;
class UploadBufferPageVulkan;

// Upload buffer page size
#define VULKAN_DEFAULT_UPLOAD_PAGE_SIZE (4 * 1014 * 1024) // 4 MB

// Upload buffer generations timeout to dispose
#define VULKAN_UPLOAD_PAGE_GEN_TIMEOUT 3

// Upload buffer pages that are not used for a few frames are disposed
#define VULKAN_UPLOAD_PAGE_NOT_USED_FRAME_TIMEOUT 60

/// <summary>
/// Uploading data to GPU buffer utility
/// </summary>
class UploadBufferVulkan
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
        void* Mapped;

        /// <summary>
        /// Allocation offset in bytes (from the start of the heap buffer).
        /// </summary>
        uint64 Offset;

        /// <summary>
        /// Allocation size in bytes
        /// </summary>
        uint64 Size;

        /// <summary>
        /// Upload buffer page resource that owns that allocation
        /// </summary>
        VkBuffer Buffer;

        /// <summary>
        /// Generation number of that allocation (generally allocation is invalid after one or two generations)
        /// </summary>
        uint64 Generation;
    };

private:
    GPUDeviceVulkan* _device;
    UploadBufferPageVulkan* _currentPage;
    uint64 _currentOffset;
    uint64 _currentGeneration;
    Array<UploadBufferPageVulkan*, InlinedAllocation<64>> _freePages;
    Array<UploadBufferPageVulkan*, InlinedAllocation<64>> _usedPages;

public:
    UploadBufferVulkan(GPUDeviceVulkan* device);

public:
    Allocation Allocate(uint64 size, uint64 align);
    Allocation Upload(const void* data, uint64 size, uint64 align);

public:
    void BeginGeneration(uint64 generation);
    void Dispose();
};

#endif
