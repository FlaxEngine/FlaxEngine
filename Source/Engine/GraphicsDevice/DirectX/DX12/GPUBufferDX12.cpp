// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "GPUBufferDX12.h"
#include "../RenderToolsDX.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Async/Tasks/GPUUploadBufferTask.h"

void GPUBufferViewDX12::SetSRV(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc)
{
    _srv.CreateSRV(_device, _owner->GetResource(), &srvDesc);
}

void GPUBufferViewDX12::SetUAV(D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, ID3D12Resource* counterResource)
{
    _uav.CreateUAV(_device, _owner->GetResource(), &uavDesc, counterResource);
}

uint64 GPUBufferDX12::GetSizeInBytes() const
{
    return _memoryUsage;
}

D3D12_GPU_VIRTUAL_ADDRESS GPUBufferDX12::GetLocation() const
{
    return _resource->GetGPUVirtualAddress();
}

GPUBufferView* GPUBufferDX12::View() const
{
    return (GPUBufferView*)&_view;
}

void* GPUBufferDX12::Map(GPUResourceMapMode mode)
{
    D3D12_RANGE readRange;
    D3D12_RANGE* readRangePtr;
    switch (mode)
    {
    case GPUResourceMapMode::Read:
        readRangePtr = nullptr;
        break;
    case GPUResourceMapMode::Write:
        readRange.Begin = readRange.End = 0;
        readRangePtr = &readRange;
        break;
    case GPUResourceMapMode::ReadWrite:
        readRangePtr = nullptr;
        break;
    default:
    CRASH;
    }
    _lastMapMode = mode;
    void* mapped = nullptr;
    const HRESULT result = _resource->Map(0, readRangePtr, &mapped);
    LOG_DIRECTX_RESULT(result);
    return mapped;
}

void GPUBufferDX12::Unmap()
{
    D3D12_RANGE writtenRange;
    D3D12_RANGE* writtenRangePtr;
    switch (_lastMapMode)
    {
    case GPUResourceMapMode::Read:
        writtenRange.Begin = writtenRange.End = 0;
        writtenRangePtr = &writtenRange;
        break;
    case GPUResourceMapMode::Write:
        writtenRangePtr = nullptr;
        break;
    case GPUResourceMapMode::ReadWrite:
        writtenRangePtr = nullptr;
        break;
    default:
        return;
    }
    _resource->Unmap(0, writtenRangePtr);
    _lastMapMode = (GPUResourceMapMode)255;
}

GPUResource* GPUBufferDX12::AsGPUResource() const
{
    return (GPUResource*)this;
}

bool GPUBufferDX12::OnInit()
{
    const bool useSRV = IsShaderResource();
    const bool useUAV = IsUnorderedAccess();

    // Create description
    D3D12_RESOURCE_DESC resourceDesc;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = _desc.Size;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    if (!useSRV)
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    if (useUAV)
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
#if PLATFORM_XBOX_SCARLETT || PLATFORM_XBOX_ONE
    if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::Argument))
        resourceDesc.Flags |= D3D12XBOX_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER;
#endif

    // Create allocation description
    D3D12_HEAP_PROPERTIES heapProperties;
    switch (_desc.Usage)
    {
    case GPUResourceUsage::StagingUpload:
    case GPUResourceUsage::Staging:
        heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
        break;
    case GPUResourceUsage::StagingReadback:
        heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
        break;
    default:
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    }
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    // Create resource
    ID3D12Resource* resource;
    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
    VALIDATE_DIRECTX_CALL(_device->GetDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialState, nullptr, IID_PPV_ARGS(&resource)));

    // Set state
    initResource(resource, initialState, 1);
    DX_SET_DEBUG_NAME(_resource, GetName());
    _memoryUsage = _desc.Size;
    int32 numElements = _desc.GetElementsCount();

    // Check if set initial data
    if (_desc.InitData)
    {
        // Here we have to upload initial data to the GPU.
        // If we are during rendering we can use device command list without an afford.
        // But if we are doing it during update or from the other thread we have to register resource data upload job.
        // In both cases options.InitData data have to exist for a few next frames.

        if (_desc.Usage == GPUResourceUsage::StagingUpload || _desc.Usage == GPUResourceUsage::Staging)
        {
            // Modify staging resource data now
            SetData(_desc.InitData, _desc.Size);
        }
        else if (_device->IsRendering() && IsInMainThread())
        {
            // Upload resource data now
            _device->GetMainContext()->UpdateBuffer(this, _desc.InitData, _desc.Size);
        }
        else
        {
            // Create async resource copy task
            auto copyTask = ::New<GPUUploadBufferTask>(this, 0, Span<byte>((const byte*)_desc.InitData, _desc.Size), true);
            ASSERT(copyTask->HasReference(this));
            copyTask->Start();
        }
    }

    // Check if need to use a counter
    if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::Counter) || EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::Append))
    {
#if GPU_ENABLE_RESOURCE_NAMING
        String name = String(GetName()) + TEXT(".Counter");
        _counter = ::New<GPUBufferDX12>(_device, name);
#else
        _counter = ::New<GPUBufferDX12>(_device, String::Empty);
#endif
        if (_counter->Init(GPUBufferDescription::Raw(4, GPUBufferFlags::UnorderedAccess)))
        {
            LOG(Error, "Cannot create counter buffer.");
            return true;
        }
    }

    // Create views
    _view.Init(_device, this, this);
    if (useSRV)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
        if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::RawBuffer))
            srvDesc.Format = RenderToolsDX::ToDxgiFormat(_desc.Format);
        else
            srvDesc.Format = RenderToolsDX::ToDxgiFormat(PixelFormatExtensions::FindShaderResourceFormat(_desc.Format, false));
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = numElements;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::Structured))
        {
            srvDesc.Buffer.StructureByteStride = _desc.Stride;
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        }
        else
        {
            srvDesc.Buffer.StructureByteStride = 0;
        }
        if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::RawBuffer))
            srvDesc.Buffer.Flags |= D3D12_BUFFER_SRV_FLAG_RAW;
        _view.SetSRV(srvDesc);
    }
    if (useUAV)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.StructureByteStride = 0;
        uavDesc.Buffer.CounterOffsetInBytes = 0;
        uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        uavDesc.Buffer.NumElements = numElements;
        if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::Structured))
            uavDesc.Buffer.StructureByteStride = _desc.Stride;
        if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::RawBuffer))
            uavDesc.Buffer.Flags |= D3D12_BUFFER_UAV_FLAG_RAW;
        if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::Structured))
            uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        else
            uavDesc.Format = RenderToolsDX::ToDxgiFormat(PixelFormatExtensions::FindUnorderedAccessFormat(_desc.Format));
        _view.SetUAV(uavDesc, _counter ? _counter->GetResource() : nullptr);
    }

    return false;
}

void GPUBufferDX12::OnReleaseGPU()
{
    _view.Release();
    releaseResource();
    SAFE_DELETE_GPU_RESOURCE(_counter);

    // Base
    GPUBuffer::OnReleaseGPU();
}

#endif
