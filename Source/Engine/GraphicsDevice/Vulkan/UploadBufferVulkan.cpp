// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN

#include "UploadBufferVulkan.h"
#include "GPUDeviceVulkan.h"
#include "RenderToolsVulkan.h"
#include "Engine/Graphics/GPUResource.h"
#include "Engine/Profiler/ProfilerMemory.h"

/// <summary>
/// Single page for the upload buffer
/// </summary>
class UploadBufferPageVulkan : public GPUResourceBase<GPUDeviceVulkan, GPUResource>, public ResourceOwnerVulkan
{
public:
    UploadBufferPageVulkan(GPUDeviceVulkan* device, uint64 size);

public:
    /// <summary>
    /// Last generation that has been using that page
    /// </summary>
    uint64 LastGen;

    /// <summary>
    /// Page size in bytes
    /// </summary>
    uint64 Size;

    /// <summary>
    /// CPU memory address of the page
    /// </summary>
    void* Mapped;

    /// <summary>
    /// Buffer that stored the page data
    /// </summary>
    VkBuffer Buffer;

    /// <summary>
    /// Buffer memory allocation
    /// </summary>
    VmaAllocation Allocation;

public:
    // [GPUResourceVulkan]
    GPUResourceType GetResourceType() const final override
    {
        return GPUResourceType::Buffer;
    }

    // [ResourceOwnerVulkan]
    GPUResource* AsGPUResource() const override
    {
        return (GPUResource*)this;
    }

protected:
    // [GPUResourceVulkan]
    void OnReleaseGPU() final override;
};

UploadBufferVulkan::UploadBufferVulkan(GPUDeviceVulkan* device)
    : _device(device)
    , _currentPage(nullptr)
    , _currentOffset(0)
    , _currentGeneration(0)
{
}

UploadBufferVulkan::Allocation UploadBufferVulkan::Allocate(uint64 size, uint64 align)
{
    const uint64 alignmentMask = align - 1;
    ASSERT_LOW_LAYER((alignmentMask & align) == 0);
    const uint64 pageSize = Math::Max<uint64>(size, VULKAN_DEFAULT_UPLOAD_PAGE_SIZE);
    const uint64 alignedSize = Math::AlignUpWithMask(size, alignmentMask);

    // Align the allocation
    _currentOffset = Math::AlignUpWithMask(_currentOffset, alignmentMask);

    // Check if there is enough space for that chunk of the data in the current page
    if (_currentPage && _currentOffset + alignedSize > _currentPage->Size)
        _currentPage = nullptr;

    // Check if need to get new page
    if (_currentPage == nullptr)
    {
        // Try reusing existing page
        for (int32 i = 0; i < _freePages.Count(); i++)
        {
            UploadBufferPageVulkan* page = _freePages.Get()[i];
            if (page->Size == pageSize)
            {
                _freePages.RemoveAt(i);
                _currentPage = page;
                break;
            }
        }
        if (_currentPage == nullptr)
            _currentPage = New<UploadBufferPageVulkan>(_device, pageSize);
        _usedPages.Add(_currentPage);
        ASSERT_LOW_LAYER(_currentPage->Buffer);
        _currentOffset = 0;
    }

    // Mark page as used in this generation
    _currentPage->LastGen = _currentGeneration;

    // Create allocation result
    const Allocation result{ (byte*)_currentPage->Mapped + _currentOffset, _currentOffset, size, _currentPage->Buffer, _currentGeneration };

    // Move within a page
    _currentOffset += size;
    ASSERT_LOW_LAYER(_currentOffset <= _currentPage->Size);

    return result;
}

UploadBufferVulkan::Allocation UploadBufferVulkan::Upload(const void* data, uint64 size, uint64 align)
{
    auto allocation = Allocate(size, align);
    Platform::MemoryCopy(allocation.Mapped, data, size);
    return allocation;
}

void UploadBufferVulkan::BeginGeneration(uint64 generation)
{
    // Restore ready pages to be reused
    for (int32 i = 0; _usedPages.HasItems() && i < _usedPages.Count(); i++)
    {
        auto page = _usedPages[i];
        if (page->LastGen + VULKAN_UPLOAD_PAGE_GEN_TIMEOUT < generation)
        {
            _usedPages.RemoveAt(i);
            i--;
            _freePages.Add(page);
        }
    }

    // Remove old pages
    for (int32 i = _freePages.Count() - 1; i >= 0 && _freePages.HasItems(); i--)
    {
        auto page = _freePages[i];
        if (page->LastGen + VULKAN_UPLOAD_PAGE_GEN_TIMEOUT + VULKAN_UPLOAD_PAGE_NOT_USED_FRAME_TIMEOUT < generation)
        {
            _freePages.RemoveAt(i);
            i--;
            page->ReleaseGPU();
            Delete(page);
        }
    }

    // Set new generation
    _currentGeneration = generation;
}

void UploadBufferVulkan::Dispose()
{
    _freePages.Add(_usedPages);
    for (auto page : _freePages)
    {
        page->ReleaseGPU();
        Delete(page);
    }
}

UploadBufferPageVulkan::UploadBufferPageVulkan(GPUDeviceVulkan* device, uint64 size)
    : GPUResourceBase(device, TEXT("Upload Buffer Page"))
    , LastGen(0)
    , Size(size)
{
    VkBufferCreateInfo bufferInfo;
    RenderToolsVulkan::ZeroStruct(bufferInfo, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VmaAllocationInfo allocInfo;
    vmaCreateBuffer(_device->Allocator, &bufferInfo, &allocCreateInfo, &Buffer, &Allocation, &allocInfo);
    Mapped = allocInfo.pMappedData;
    ASSERT_LOW_LAYER(Mapped);
    _memoryUsage = size;
    PROFILE_MEM_INC(GraphicsCommands, _memoryUsage);
}

void UploadBufferPageVulkan::OnReleaseGPU()
{
    PROFILE_MEM_DEC(GraphicsCommands, _memoryUsage);
    vmaDestroyBuffer(_device->Allocator, Buffer, Allocation);
    Buffer = VK_NULL_HANDLE;
    Allocation = VK_NULL_HANDLE;
    Mapped = nullptr;
}

#endif
