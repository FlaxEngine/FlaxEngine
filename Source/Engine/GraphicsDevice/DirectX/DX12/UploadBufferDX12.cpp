// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "UploadBufferDX12.h"
#include "GPUTextureDX12.h"
#include "GPUContextDX12.h"
#include "../RenderToolsDX.h"

UploadBufferDX12::UploadBufferDX12(GPUDeviceDX12* device)
    : _device(device)
    , _currentPage(nullptr)
    , _currentOffset(0)
    , _currentGeneration(0)
{
}

UploadBufferDX12::~UploadBufferDX12()
{
    _freePages.Add(_usedPages);
    for (auto page : _freePages)
    {
        page->ReleaseGPU();
        Delete(page);
    }
}

DynamicAllocation UploadBufferDX12::Allocate(uint64 size, uint64 align)
{
    const uint64 alignmentMask = align - 1;
    ASSERT((alignmentMask & align) == 0);

    // Check if use default or bigger page
    const bool useDefaultSize = size <= DX12_DEFAULT_UPLOAD_PAGE_SIZE;
    const uint64 pageSize = useDefaultSize ? DX12_DEFAULT_UPLOAD_PAGE_SIZE : size;
    const uint64 alignedSize = Math::AlignUpWithMask(size, alignmentMask);

    // Align the allocation
    _currentOffset = Math::AlignUpWithMask(_currentOffset, alignmentMask);

    // Check if there is enough space for that chunk of the data in the current page
    if (_currentPage && _currentOffset + alignedSize > _currentPage->Size)
    {
        _currentPage = nullptr;
    }

    // Check if need to get new page
    if (_currentPage == nullptr)
    {
        _currentPage = requestPage(pageSize);
        _currentOffset = 0;
    }

    // Mark page as used in this generation
    _currentPage->LastGen = _currentGeneration;

    // Create allocation result
    const DynamicAllocation result(static_cast<byte*>(_currentPage->CPUAddress) + _currentOffset, _currentOffset, size, _currentPage->GPUAddress + _currentOffset, _currentPage, _currentGeneration);

    // Move in the page
    _currentOffset += size;

    ASSERT(_currentPage->GetResource());
    return result;
}

bool UploadBufferDX12::UploadBuffer(GPUContextDX12* context, ID3D12Resource* buffer, uint32 bufferOffset, const void* data, uint64 size)
{
    // Allocate data
    const DynamicAllocation allocation = Allocate(size, 4);
    if (allocation.IsInvalid())
        return true;

    // Copy data
    Platform::MemoryCopy(allocation.CPUAddress, data, static_cast<size_t>(size));

    // Copy buffer region
    context->GetCommandList()->CopyBufferRegion(buffer, bufferOffset, allocation.Page->GetResource(), allocation.Offset, size);

    return false;
}

bool UploadBufferDX12::UploadTexture(GPUContextDX12* context, ID3D12Resource* texture, const void* srcData, uint32 srcRowPitch, uint32 srcSlicePitch, int32 mipIndex, int32 arrayIndex)
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
    const DynamicAllocation allocation = Allocate(sliceSizeAligned, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    if (allocation.Size != sliceSizeAligned)
        return true;

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
    srcLocation.pResource = allocation.Page->GetResource();
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLocation.PlacedFootprint.Offset = allocation.Offset;
    srcLocation.PlacedFootprint.Footprint = footprint.Footprint;

    // Copy texture region
    context->GetCommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

    return false;
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

UploadBufferPageDX12* UploadBufferDX12::requestPage(uint64 size)
{
    // Try to find valid page
    int32 freePageIndex = -1;
    for (int32 i = 0; i < _freePages.Count(); i++)
    {
        if (_freePages[i]->Size == size)
        {
            freePageIndex = i;
            break;
        }
    }

    // Check if create a new page
    UploadBufferPageDX12* page;
    if (freePageIndex == -1)
    {
        // Get a new page to use
        page = New<UploadBufferPageDX12>(_device, size);
    }
    else
    {
        // Remove from free pages
        page = _freePages[freePageIndex];
        _freePages.RemoveAt(freePageIndex);
    }

    // Mark page as used
    _usedPages.Add(page);

    return page;
}

UploadBufferPageDX12::UploadBufferPageDX12(GPUDeviceDX12* device, uint64 size)
    : GPUResourceDX12(device, TEXT("Upload Buffer Page"))
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
    DX_SET_DEBUG_NAME(_resource, GPUResourceDX12::GetName());
    _memoryUsage = size;
    GPUAddress = _resource->GetGPUVirtualAddress();

    // Map buffer
    VALIDATE_DIRECTX_CALL(_resource->Map(0, nullptr, &CPUAddress));
}

void UploadBufferPageDX12::OnReleaseGPU()
{
    // Unmap
    if (_resource && CPUAddress)
    {
        _resource->Unmap(0, nullptr);
    }
    GPUAddress = 0;
    CPUAddress = nullptr;

    // Release
    releaseResource();
}

#endif
