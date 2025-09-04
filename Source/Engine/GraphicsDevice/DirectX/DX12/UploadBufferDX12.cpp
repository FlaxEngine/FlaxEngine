// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "UploadBufferDX12.h"
#include "GPUTextureDX12.h"
#include "../RenderToolsDX.h"
#include "Engine/Graphics/GPUResource.h"
#include "Engine/Profiler/ProfilerMemory.h"

/// <summary>
/// Single page for the upload buffer
/// </summary>
class UploadBufferPageDX12 : public GPUResourceBase<GPUDeviceDX12, GPUResource>, public ResourceOwnerDX12
{
public:
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

UploadBufferDX12::UploadBufferDX12(GPUDeviceDX12* device)
    : _device(device)
    , _currentPage(nullptr)
    , _currentOffset(0)
    , _currentGeneration(0)
{
}

UploadBufferDX12::Allocation UploadBufferDX12::Allocate(uint64 size, uint64 align)
{
    const uint64 alignmentMask = align - 1;
    ASSERT_LOW_LAYER((alignmentMask & align) == 0);
    const uint64 pageSize = Math::Max<uint64>(size, DX12_DEFAULT_UPLOAD_PAGE_SIZE);
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
            UploadBufferPageDX12* page = _freePages.Get()[i];
            if (page->Size == pageSize)
            {
                _freePages.RemoveAt(i);
                _currentPage = page;
                break;
            }
        }
        if (_currentPage == nullptr)
            _currentPage = New<UploadBufferPageDX12>(_device, pageSize);
        _usedPages.Add(_currentPage);
        ASSERT_LOW_LAYER(_currentPage->GetResource());
        _currentOffset = 0;
    }

    // Mark page as used in this generation
    _currentPage->LastGen = _currentGeneration;

    // Create allocation result
    const Allocation result { (byte*)_currentPage->CPUAddress + _currentOffset, _currentOffset, size, _currentPage->GPUAddress + _currentOffset, _currentPage->GetResource(), _currentGeneration };

    // Move within a page
    _currentOffset += size;

    return result;
}

void UploadBufferDX12::UploadBuffer(ID3D12GraphicsCommandList* commandList, ID3D12Resource* buffer, uint32 bufferOffset, const void* data, uint64 size)
{
    // Allocate data
    const auto allocation = Allocate(size, GPU_SHADER_DATA_ALIGNMENT);

    // Copy data
    Platform::MemoryCopy(allocation.CPUAddress, data, size);

    // Copy buffer region
    commandList->CopyBufferRegion(buffer, bufferOffset, allocation.Resource, allocation.Offset, size);
}

void UploadBufferDX12::UploadTexture(ID3D12GraphicsCommandList* commandList, ID3D12Resource* texture, const void* srcData, uint32 srcRowPitch, uint32 srcSlicePitch, int32 mipIndex, int32 arrayIndex)
{
    D3D12_RESOURCE_DESC resourceDesc = texture->GetDesc();
    const UINT subresourceIndex = RenderToolsDX::CalcSubresourceIndex(mipIndex, arrayIndex, resourceDesc.MipLevels);
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
    uint32 numRows;
    uint64 rowPitchAligned, mipSizeAligned;
    _device->GetDevice()->GetCopyableFootprints(&resourceDesc, subresourceIndex, 1, 0, &footprint, &numRows, &rowPitchAligned, &mipSizeAligned);
    rowPitchAligned = footprint.Footprint.RowPitch;
    mipSizeAligned = rowPitchAligned * footprint.Footprint.Height;
    const uint32 numSlices = resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? Math::Max(1, resourceDesc.DepthOrArraySize >> mipIndex) : 1;
    const uint64 sliceSizeAligned = numSlices * mipSizeAligned;

    // Allocate data
    const auto allocation = Allocate(sliceSizeAligned, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

    byte* ptr = (byte*)srcData;
    ASSERT(srcSlicePitch <= sliceSizeAligned);
    if (srcSlicePitch == sliceSizeAligned)
    {
        // Copy data at once
        Platform::MemoryCopy(allocation.CPUAddress, ptr, srcSlicePitch);
    }
    else
    {
        // Copy data per-row
        byte* dst = static_cast<byte*>(allocation.CPUAddress);
        ASSERT(srcRowPitch <= rowPitchAligned);
        const uint32 numCopies = numSlices * numRows;
        for (uint32 i = 0; i < numCopies; i++)
        {
            Platform::MemoryCopy(dst, ptr, srcRowPitch);
            dst += rowPitchAligned;
            ptr += srcRowPitch;
        }
    }

    // Destination texture copy location description
    D3D12_TEXTURE_COPY_LOCATION dstLocation;
    dstLocation.pResource = texture;
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = subresourceIndex;

    // Source buffer copy location description
    D3D12_TEXTURE_COPY_LOCATION srcLocation;
    srcLocation.pResource = allocation.Resource;
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLocation.PlacedFootprint.Offset = allocation.Offset;
    srcLocation.PlacedFootprint.Footprint = footprint.Footprint;

    // Copy texture region
    commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
}

void UploadBufferDX12::BeginGeneration(uint64 generation)
{
    // Restore ready pages to be reused
    for (int32 i = 0; _usedPages.HasItems() && i < _usedPages.Count(); i++)
    {
        auto page = _usedPages[i];
        if (page->LastGen + DX12_UPLOAD_PAGE_GEN_TIMEOUT < generation)
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
        if (page->LastGen + DX12_UPLOAD_PAGE_GEN_TIMEOUT + DX12_UPLOAD_PAGE_NOT_USED_FRAME_TIMEOUT < generation)
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

void UploadBufferDX12::ReleaseGPU()
{
    _freePages.Add(_usedPages);
    for (auto page : _freePages)
    {
        page->ReleaseGPU();
        Delete(page);
    }
}

UploadBufferPageDX12::UploadBufferPageDX12(GPUDeviceDX12* device, uint64 size)
    : GPUResourceBase(device, TEXT("Upload Buffer Page"))
    , LastGen(0)
    , CPUAddress(nullptr)
    , GPUAddress(0)
    , Size(size)
{
    // Create page buffer
    D3D12_HEAP_PROPERTIES heapProperties;
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;
    D3D12_RESOURCE_DESC resourceDesc;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = size;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    ID3D12Resource* resource;
    VALIDATE_DIRECTX_CALL(_device->GetDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource)));

    // Set state
    initResource(resource, D3D12_RESOURCE_STATE_GENERIC_READ, 1);
    DX_SET_DEBUG_NAME(_resource, GetName());
    _memoryUsage = size;
    PROFILE_MEM_INC(GraphicsCommands, _memoryUsage);
    GPUAddress = _resource->GetGPUVirtualAddress();

    // Map buffer
    VALIDATE_DIRECTX_CALL(_resource->Map(0, nullptr, &CPUAddress));
}

void UploadBufferPageDX12::OnReleaseGPU()
{
    PROFILE_MEM_DEC(GraphicsCommands, _memoryUsage);

    // Unmap
    if (_resource && CPUAddress)
        _resource->Unmap(0, nullptr);
    GPUAddress = 0;
    CPUAddress = nullptr;

    // Release
    releaseResource();
}

#endif
