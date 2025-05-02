// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "GPUTextureDX12.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/Textures/TextureData.h"

bool GPUTextureDX12::GetData(int32 arrayIndex, int32 mipMapIndex, TextureMipData& data, uint32 mipRowPitch)
{
    if (!IsStaging())
    {
        LOG(Warning, "Texture::GetData is valid only for staging resources.");
        return true;
    }

    GPUDeviceLock lock(_device);

    // Internally it's a buffer, so adapt resource index and offset
    const uint32 subresource = RenderToolsDX::CalcSubresourceIndex(mipMapIndex, arrayIndex, MipLevels());
    const int32 offsetInBytes = ComputeBufferOffset(subresource, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    const int32 lengthInBytes = ComputeSubresourceSize(subresource, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    const int32 rowPitch = ComputeRowPitch(mipMapIndex, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
    const int32 depthPitch = ComputeSlicePitch(mipMapIndex, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

    // Map the staging resource mip map for reading
    D3D12_RANGE range;
    range.Begin = offsetInBytes;
    range.End = offsetInBytes + lengthInBytes;
    void* mapped;
    const HRESULT mapResult = _resource->Map(0, &range, &mapped);
    if (FAILED(mapResult))
    {
        LOG_DIRECTX_RESULT(mapResult);
        return true;
    }
    mapped = (byte*)mapped + offsetInBytes;

    data.Copy(mapped, rowPitch, depthPitch, Depth(), mipRowPitch);

    // Unmap buffer
    _resource->Unmap(0, nullptr);

    return false;
}

bool GPUTextureDX12::OnInit()
{
    ID3D12Resource* resource;

    // Cache formats
    const PixelFormat format = Format();
    const PixelFormat typelessFormat = PixelFormatExtensions::MakeTypeless(format);
    const DXGI_FORMAT dxgiFormat = RenderToolsDX::ToDxgiFormat(typelessFormat);
    _dxgiFormatDSV = RenderToolsDX::ToDxgiFormat(PixelFormatExtensions::FindDepthStencilFormat(format));
    _dxgiFormatSRV = RenderToolsDX::ToDxgiFormat(PixelFormatExtensions::FindShaderResourceFormat(format, _sRGB));
    _dxgiFormatRTV = _dxgiFormatSRV;
    _dxgiFormatUAV = RenderToolsDX::ToDxgiFormat(PixelFormatExtensions::FindUnorderedAccessFormat(format));

    // Cache properties
    auto device = _device->GetDevice();
    bool useSRV = IsShaderResource();
    bool useDSV = IsDepthStencil();
    bool useRTV = IsRenderTarget();
    bool useUAV = IsUnorderedAccess();
    int32 depthOrArraySize = IsVolume() ? Depth() : ArraySize();

    if (IsStaging())
    {
        // Initialize as a buffer
        const int32 totalSize = ComputeBufferTotalSize(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
        D3D12_HEAP_PROPERTIES heapProperties;
        heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProperties.CreationNodeMask = 1;
        heapProperties.VisibleNodeMask = 1;
        D3D12_RESOURCE_DESC resourceDesc;
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = totalSize;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        auto result = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resource));
        LOG_DIRECTX_RESULT_WITH_RETURN(result, true);
        initResource(resource, D3D12_RESOURCE_STATE_COPY_DEST, 1);
        DX_SET_DEBUG_NAME(_resource, GetName());
        _memoryUsage = totalSize;
        return false;
    }

    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;

    // Create texture description
    D3D12_RESOURCE_DESC resourceDesc;
    resourceDesc.MipLevels = _desc.MipLevels;
    resourceDesc.Format = dxgiFormat;
    resourceDesc.Width = _desc.Width;
    resourceDesc.Height = _desc.Height;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    resourceDesc.DepthOrArraySize = depthOrArraySize;
    resourceDesc.SampleDesc.Count = static_cast<UINT>(_desc.MultiSampleLevel);
    resourceDesc.SampleDesc.Quality = IsMultiSample() ? GPUDeviceDX12::GetMaxMSAAQuality((int32)_desc.MultiSampleLevel) : 0;
    resourceDesc.Alignment = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Dimension = IsVolume() ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    if (useRTV)
    {
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        initialState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    else if (useDSV)
    {
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        initialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        if (!useSRV)
        {
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }
    }
    if (useUAV)
    {
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    // Create heap properties
    D3D12_HEAP_PROPERTIES heapProperties;
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    // Create clear value (used by render targets and depth stencil buffers)
    D3D12_CLEAR_VALUE* clearValuePtr = nullptr;
    D3D12_CLEAR_VALUE clearValue;
    if (useRTV)
    {
        clearValue.Format = _dxgiFormatSRV;
        Platform::MemoryCopy(&clearValue.Color, _desc.DefaultClearColor.Raw, sizeof(Color));
        clearValuePtr = &clearValue;
    }
    else if (useDSV)
    {
        clearValue.Format = _dxgiFormatDSV;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;
        clearValuePtr = &clearValue;
    }

    if (IsRegularTexture())
        initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    // Create texture
    auto result = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialState, clearValuePtr, IID_PPV_ARGS(&resource));
    LOG_DIRECTX_RESULT_WITH_RETURN(result, true);

    // Set state
    bool isRead = useSRV || useUAV;
    bool isWrite = useDSV || useRTV || useUAV;
    initResource(resource, initialState, resourceDesc, isRead && isWrite);
    DX_SET_DEBUG_NAME(_resource, GetName());
    _memoryUsage = calculateMemoryUsage();

    // Initialize handles to the resource
    if (IsRegularTexture())
    {
        // 'Regular' texture is using only one handle (texture/cubemap)
        _handlesPerSlice.Resize(1, false);
    }
    else
    {
        // Create all handles
        initHandles();
    }

    return false;
}

void GPUTextureDX12::OnResidentMipsChanged()
{
    const int32 firstMipIndex = MipLevels() - ResidentMipLevels();
    const int32 mipLevels = ResidentMipLevels();
    D3D12_SHADER_RESOURCE_VIEW_DESC srDesc;
    srDesc.Format = _dxgiFormatSRV;
    srDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    if (IsCubeMap())
    {
        srDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srDesc.TextureCube.MostDetailedMip = firstMipIndex;
        srDesc.TextureCube.MipLevels = mipLevels;
        srDesc.TextureCube.ResourceMinLODClamp = 0;
    }
    else if (IsVolume())
    {
        srDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srDesc.Texture3D.MostDetailedMip = firstMipIndex;
        srDesc.Texture3D.MipLevels = mipLevels;
        srDesc.Texture3D.ResourceMinLODClamp = 0;
    }
    else
    {
        srDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srDesc.Texture2D.MostDetailedMip = firstMipIndex;
        srDesc.Texture2D.MipLevels = mipLevels;
        srDesc.Texture2D.PlaneSlice = 0;
        srDesc.Texture2D.ResourceMinLODClamp = 0;
    }
    GPUTextureViewDX12& view = IsVolume() ? _handleVolume : _handlesPerSlice[0];
    if (view.GetParent() == nullptr)
        view.Init(this, _device, this, Format(), MultiSampleLevel());
    if (mipLevels != 0)
        view.SetSRV(srDesc);
}

void GPUTextureDX12::OnReleaseGPU()
{
    _handlesPerMip.Resize(0, false);
    _handlesPerSlice.Resize(0, false);
    _handleArray.Release();
    _handleVolume.Release();
    _srv.Release();
    _uav.Release();
    releaseResource();

    // Base
    GPUTexture::OnReleaseGPU();
}

void GPUTextureViewDX12::Release()
{
    _rtv.Release();
    _srv.Release();
    _dsv.Release();
    _uav.Release();
}

void GPUTextureViewDX12::SetRTV(D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc)
{
    _rtv.CreateRTV(_device, _owner->GetResource(), &rtvDesc);
}

void GPUTextureViewDX12::SetSRV(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc)
{
    SrvDimension = srvDesc.ViewDimension;
    _srv.CreateSRV(_device, _owner->GetResource(), &srvDesc);
}

void GPUTextureViewDX12::SetDSV(D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc)
{
    _dsv.CreateDSV(_device, _owner->GetResource(), &dsvDesc);
}

void GPUTextureViewDX12::SetUAV(D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, ID3D12Resource* counterResource)
{
    UavDimension = uavDesc.ViewDimension;
    _uav.CreateUAV(_device, _owner->GetResource(), &uavDesc, counterResource);
}

void GPUTextureDX12::initHandles()
{
    D3D12_RENDER_TARGET_VIEW_DESC rtDesc;
    D3D12_SHADER_RESOURCE_VIEW_DESC srDesc;
    D3D12_DEPTH_STENCIL_VIEW_DESC dsDesc;
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;

    rtDesc.Format = _dxgiFormatRTV;
    srDesc.Format = _dxgiFormatSRV;
    srDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    dsDesc.Format = _dxgiFormatDSV;
    dsDesc.Flags = D3D12_DSV_FLAG_NONE;
    uavDesc.Format = _dxgiFormatUAV;

    // Cache properties
    bool useSRV = IsShaderResource();
    bool useDSV = IsDepthStencil();
    bool useRTV = IsRenderTarget();
    bool useUAV = IsUnorderedAccess();
    int32 arraySize = ArraySize();
    int32 mipLevels = MipLevels();
    bool isArray = arraySize > 1;
    bool isCubeMap = IsCubeMap();
    bool isMsaa = IsMultiSample();
    bool isVolume = IsVolume();
    auto format = Format();
    auto msaa = MultiSampleLevel();

    // Create resource views
    if (useUAV)
    {
        if (isVolume)
        {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            uavDesc.Texture3D.MipSlice = 0;
            uavDesc.Texture3D.WSize = Depth();
            uavDesc.Texture3D.FirstWSlice = 0;
        }
        else if (isArray)
        {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            uavDesc.Texture2DArray.MipSlice = 0;
            uavDesc.Texture2DArray.ArraySize = arraySize * 6;
            uavDesc.Texture2DArray.FirstArraySlice = 0;
            uavDesc.Texture2DArray.PlaneSlice = 0;
        }
        else
        {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = 0;
            uavDesc.Texture2D.PlaneSlice = 0;
        }
        _uav.CreateUAV(_device, _resource, &uavDesc);
    }
    if (isVolume)
    {
        // Create handle for whole 3d texture
        _handleVolume.Init(this, _device, this, format, msaa);
        if (useSRV)
        {
            srDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
            srDesc.Texture3D.MostDetailedMip = 0;
            srDesc.Texture3D.MipLevels = MipLevels();
            srDesc.Texture3D.ResourceMinLODClamp = 0;
            _handleVolume.SetSRV(srDesc);
        }
        if (useRTV)
        {
            rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
            rtDesc.Texture3D.MipSlice = 0;
            rtDesc.Texture3D.FirstWSlice = 0;
            rtDesc.Texture3D.WSize = Depth();
            _handleVolume.SetRTV(rtDesc);
        }
        if (useUAV)
        {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            uavDesc.Texture2D.MipSlice = 0;
            uavDesc.Texture2D.PlaneSlice = 0;
            _handleVolume.SetUAV(uavDesc);
        }

        // Init per slice views
        _handlesPerSlice.Resize(Depth(), false);
        if (_desc.HasPerSliceViews() && useRTV)
        {
            rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
            rtDesc.Texture3D.MipSlice = 0;
            rtDesc.Texture3D.WSize = 1;

            for (int32 sliceIndex = 0; sliceIndex < Depth(); sliceIndex++)
            {
                rtDesc.Texture3D.FirstWSlice = sliceIndex;
                _handlesPerSlice[sliceIndex].Init(this, _device, this, format, msaa);
                _handlesPerSlice[sliceIndex].SetRTV(rtDesc);
            }
        }
    }
    else if (isArray)
    {
        // Resize handles
        _handlesPerSlice.Resize(ArraySize(), false);

        // Create per array slice handles
        for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
        {
            _handlesPerSlice[arrayIndex].Init(this, _device, this, format, msaa);

            if (useDSV)
            {
                /*if (isCubeMap)
                {
                    dsDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                    dsDesc.Texture2DArray.ArraySize = 6;
                    dsDesc.Texture2DArray.FirstArraySlice = arrayIndex * 6;
                    dsDesc.Texture2DArray.MipSlice = 0;
                }
                else*/
                {
                    dsDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                    dsDesc.Texture2DArray.ArraySize = 1;
                    dsDesc.Texture2DArray.FirstArraySlice = arrayIndex;
                    dsDesc.Texture2DArray.MipSlice = 0;
                }
                _handlesPerSlice[arrayIndex].SetDSV(dsDesc);
            }
            if (useRTV)
            {
                /*if (isCubeMap)
                {
                    rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                    rtDesc.Texture2DArray.ArraySize = 6;
                    rtDesc.Texture2DArray.FirstArraySlice = arrayIndex * 6;
                    rtDesc.Texture2DArray.MipSlice = 0;
                    rtDesc.Texture2DArray.PlaneSlice = 0;
                }
                else*/
                {
                    rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                    rtDesc.Texture2DArray.ArraySize = 1;
                    rtDesc.Texture2DArray.FirstArraySlice = arrayIndex;
                    rtDesc.Texture2DArray.MipSlice = 0;
                    rtDesc.Texture2DArray.PlaneSlice = 0;
                }
                _handlesPerSlice[arrayIndex].SetRTV(rtDesc);
            }
            if (useSRV)
            {
                /*if (isCubeMap)
                {
                    srDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                    srDesc.TextureCubeArray.First2DArrayFace = arrayIndex * 6;
                    srDesc.TextureCubeArray.NumCubes = 1;
                    srDesc.TextureCubeArray.MipLevels = mipLevels;
                    srDesc.TextureCubeArray.MostDetailedMip = 0;
                }
                else*/
                {
                    srDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                    srDesc.Texture2DArray.ArraySize = 1;
                    srDesc.Texture2DArray.FirstArraySlice = arrayIndex;
                    srDesc.Texture2DArray.MipLevels = mipLevels;
                    srDesc.Texture2DArray.MostDetailedMip = 0;
                    srDesc.Texture2DArray.PlaneSlice = 0;
                    srDesc.Texture2DArray.ResourceMinLODClamp = 0;
                }
                _handlesPerSlice[arrayIndex].SetSRV(srDesc);
                if (isCubeMap)
                    _handlesPerSlice[arrayIndex].SrvDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // Hack xD (to reproduce the problem comment this line and use Spot Light with a shadow)
            }
            if (useUAV)
            {
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                uavDesc.Texture2DArray.ArraySize = 1;
                uavDesc.Texture2DArray.FirstArraySlice = arrayIndex;
                uavDesc.Texture2DArray.MipSlice = 0;
                uavDesc.Texture2DArray.PlaneSlice = 0;
                _handlesPerSlice[arrayIndex].SetSRV(srDesc);
            }
        }

        // Create whole array handle
        {
            _handleArray.Init(this, _device, this, format, msaa);
            if (useDSV)
            {
                dsDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                dsDesc.Texture2DArray.ArraySize = arraySize;
                dsDesc.Texture2DArray.FirstArraySlice = 0;
                dsDesc.Texture2DArray.MipSlice = 0;
                _handleArray.SetDSV(dsDesc);
            }
            if (useRTV)
            {
                rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                rtDesc.Texture2DArray.ArraySize = arraySize;
                rtDesc.Texture2DArray.FirstArraySlice = 0;
                rtDesc.Texture2DArray.MipSlice = 0;
                rtDesc.Texture2DArray.PlaneSlice = 0;
                _handleArray.SetRTV(rtDesc);
            }
            if (useSRV)
            {
                if (isCubeMap)
                {
                    srDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                    srDesc.TextureCube.MostDetailedMip = 0;
                    srDesc.TextureCube.MipLevels = mipLevels;
                    srDesc.TextureCube.ResourceMinLODClamp = 0;
                }
                else
                {
                    srDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                    srDesc.Texture2DArray.ArraySize = arraySize;
                    srDesc.Texture2DArray.FirstArraySlice = 0;
                    srDesc.Texture2DArray.MipLevels = mipLevels;
                    srDesc.Texture2DArray.MostDetailedMip = 0;
                    srDesc.Texture2DArray.ResourceMinLODClamp = 0;
                    srDesc.Texture2DArray.PlaneSlice = 0;
                }
                _handleArray.SetSRV(srDesc);
            }
            if (useUAV)
            {
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                uavDesc.Texture2DArray.ArraySize = arraySize;
                uavDesc.Texture2DArray.FirstArraySlice = 0;
                uavDesc.Texture2DArray.MipSlice = 0;
                uavDesc.Texture2DArray.PlaneSlice = 0;
                _handleArray.SetUAV(uavDesc);
            }
        }
    }
    else
    {
        // Create single handle for the whole texture
        _handlesPerSlice.Resize(1, false);
        _handlesPerSlice[0].Init(this, _device, this, format, msaa);
        if (useDSV)
        {
            if (isCubeMap)
            {
                dsDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                dsDesc.Texture2DArray.MipSlice = 0;
                dsDesc.Texture2DArray.FirstArraySlice = 0;
                dsDesc.Texture2DArray.ArraySize = arraySize * 6;
            }
            else if (isMsaa)
            {
                dsDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                dsDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                dsDesc.Texture2D.MipSlice = 0;
            }
            _handlesPerSlice[0].SetDSV(dsDesc);
        }
        if (useRTV)
        {
            if (isCubeMap)
            {
                rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                rtDesc.Texture2DArray.MipSlice = 0;
                rtDesc.Texture2DArray.FirstArraySlice = 0;
                rtDesc.Texture2DArray.ArraySize = arraySize * 6;
                rtDesc.Texture2DArray.PlaneSlice = 0;
            }
            else if (isMsaa)
            {
                rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                rtDesc.Texture2D.MipSlice = 0;
                rtDesc.Texture2D.PlaneSlice = 0;
            }
            _handlesPerSlice[0].SetRTV(rtDesc);
        }
        if (useSRV)
        {
            if (isCubeMap)
            {
                srDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                srDesc.TextureCube.MostDetailedMip = 0;
                srDesc.TextureCube.MipLevels = mipLevels;
                srDesc.TextureCube.ResourceMinLODClamp = 0;
            }
            else if (isMsaa)
            {
                srDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                srDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srDesc.Texture2D.MostDetailedMip = 0;
                srDesc.Texture2D.MipLevels = mipLevels;
                srDesc.Texture2D.ResourceMinLODClamp = 0;
                srDesc.Texture2D.PlaneSlice = 0;
            }
            _handlesPerSlice[0].SetSRV(srDesc);
        }
        if (useUAV)
        {
            if (isCubeMap)
            {
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                uavDesc.Texture2DArray.ArraySize = arraySize;
                uavDesc.Texture2DArray.MipSlice = 0;
                uavDesc.Texture2DArray.FirstArraySlice = 0;
                uavDesc.Texture2DArray.PlaneSlice = 0;
            }
            else
            {
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                uavDesc.Texture2D.MipSlice = 0;
                uavDesc.Texture2D.PlaneSlice = 0;
            }
            _handlesPerSlice[0].SetUAV(uavDesc);
        }
    }

    // Init per mip map handles
    if (HasPerMipViews())
    {
        // Init handles
        _handlesPerMip.Resize(arraySize, false);
        for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
        {
            auto& slice = _handlesPerMip[arrayIndex];
            slice.Resize(mipLevels, false);

            for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
            {
                int32 subresourceIndex = arrayIndex * mipLevels + mipIndex;
                slice[mipIndex].Init(this, _device, this, format, msaa, subresourceIndex);

                // DSV
                if (useDSV)
                {
                    if (isArray)
                    {
                        dsDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                        dsDesc.Texture2DArray.ArraySize = 1;
                        dsDesc.Texture2DArray.FirstArraySlice = arrayIndex;
                        dsDesc.Texture2DArray.MipSlice = mipIndex;
                    }
                    else
                    {
                        dsDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                        dsDesc.Texture2D.MipSlice = mipIndex;
                    }
                    slice[mipIndex].SetDSV(dsDesc);
                }

                // RTV
                if (useRTV)
                {
                    if (isArray)
                    {
                        rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                        rtDesc.Texture2DArray.ArraySize = 1;
                        rtDesc.Texture2DArray.FirstArraySlice = arrayIndex;
                        rtDesc.Texture2DArray.MipSlice = mipIndex;
                        rtDesc.Texture2DArray.PlaneSlice = 0;
                    }
                    else
                    {
                        rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                        rtDesc.Texture2D.MipSlice = mipIndex;
                        rtDesc.Texture2D.PlaneSlice = 0;
                    }
                    slice[mipIndex].SetRTV(rtDesc);
                }

                // SRV
                if (useSRV)
                {
                    if (isArray)
                    {
                        srDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                        srDesc.Texture2DArray.ArraySize = 1;
                        srDesc.Texture2DArray.FirstArraySlice = arrayIndex;
                        srDesc.Texture2DArray.MipLevels = 1;
                        srDesc.Texture2DArray.MostDetailedMip = mipIndex;
                        srDesc.Texture2DArray.ResourceMinLODClamp = 0;
                        srDesc.Texture2DArray.PlaneSlice = 0;
                    }
                    else
                    {
                        srDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                        srDesc.Texture2D.MipLevels = 1;
                        srDesc.Texture2D.MostDetailedMip = mipIndex;
                        srDesc.Texture2D.ResourceMinLODClamp = 0;
                        srDesc.Texture2D.PlaneSlice = 0;
                    }
                    slice[mipIndex].SetSRV(srDesc);
                }
            }
        }
    }

    // Read-only depth-stencil
    if (EnumHasAnyFlags(_desc.Flags, GPUTextureFlags::ReadOnlyDepthView))
    {
        _handleReadOnlyDepth.Init(this, _device, this, format, msaa);
        _handleReadOnlyDepth.ReadOnlyDepthView = true;
        if (useDSV)
        {
            if (isCubeMap)
            {
                dsDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                dsDesc.Texture2DArray.MipSlice = 0;
                dsDesc.Texture2DArray.FirstArraySlice = 0;
                dsDesc.Texture2DArray.ArraySize = arraySize * 6;
            }
            else if (isMsaa)
            {
                dsDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                dsDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                dsDesc.Texture2D.MipSlice = 0;
            }
            dsDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
            if (PixelFormatExtensions::HasStencil(format))
                dsDesc.Flags |= D3D12_DSV_FLAG_READ_ONLY_STENCIL;
            _handleReadOnlyDepth.SetDSV(dsDesc);
        }
        ASSERT(!useRTV);
        if (useSRV)
        {
            if (isCubeMap)
            {
                srDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                srDesc.TextureCube.MostDetailedMip = 0;
                srDesc.TextureCube.MipLevels = mipLevels;
                srDesc.TextureCube.ResourceMinLODClamp = 0;
            }
            else if (isMsaa)
            {
                srDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                srDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srDesc.Texture2D.MostDetailedMip = 0;
                srDesc.Texture2D.MipLevels = mipLevels;
                srDesc.Texture2D.ResourceMinLODClamp = 0;
                srDesc.Texture2D.PlaneSlice = 0;
            }
            _handleReadOnlyDepth.SetSRV(srDesc);
        }
    }
}

#endif
