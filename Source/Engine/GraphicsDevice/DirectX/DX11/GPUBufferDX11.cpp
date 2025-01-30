// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX11

#include "GPUBufferDX11.h"
#include "../RenderToolsDX.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Threading/Threading.h"

GPUBufferDX11::GPUBufferDX11(GPUDeviceDX11* device, const StringView& name)
    : GPUResourceDX11(device, name)
{
    _view.SetParnet(this);
}

GPUBufferView* GPUBufferDX11::View() const
{
    return (GPUBufferView*)&_view;
}

void* GPUBufferDX11::Map(GPUResourceMapMode mode)
{
    const bool isMainThread = IsInMainThread();
    if (!isMainThread)
        _device->Locker.Lock();
    ASSERT(!_mapped);

    D3D11_MAPPED_SUBRESOURCE map;
    map.pData = nullptr;
    D3D11_MAP mapType;
    UINT mapFlags = 0;
    switch (mode)
    {
    case GPUResourceMapMode::Read:
        mapType = D3D11_MAP_READ;
        if (_desc.Usage == GPUResourceUsage::StagingReadback && isMainThread)
            mapFlags = D3D11_MAP_FLAG_DO_NOT_WAIT;
        break;
    case GPUResourceMapMode::Write:
        mapType = D3D11_MAP_WRITE_DISCARD;
        break;
    case GPUResourceMapMode::ReadWrite:
        mapType = D3D11_MAP_READ_WRITE;
        break;
    default:
        return nullptr;
    }

    const HRESULT result = _device->GetIM()->Map(_resource, 0, mapType, mapFlags, &map);
    if (result != DXGI_ERROR_WAS_STILL_DRAWING)
        LOG_DIRECTX_RESULT(result);

    _mapped = map.pData != nullptr;
    if (!_mapped && !isMainThread)
        _device->Locker.Unlock();

    return map.pData;
}

void GPUBufferDX11::Unmap()
{
    ASSERT(_mapped);
    _mapped = false;
    _device->GetIM()->Unmap(_resource, 0);
    if (!IsInMainThread())
        _device->Locker.Unlock();
}

bool GPUBufferDX11::OnInit()
{
    bool useSRV = IsShaderResource();
    bool useUAV = IsUnorderedAccess();

    // Create buffer description
    D3D11_BUFFER_DESC bufferDesc;
    bufferDesc.ByteWidth = _desc.Size;
    bufferDesc.Usage = RenderToolsDX::ToD3D11Usage(_desc.Usage);
    bufferDesc.BindFlags = 0;
    bufferDesc.CPUAccessFlags = RenderToolsDX::GetDX11CpuAccessFlagsFromUsage(_desc.Usage);
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;
    if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::VertexBuffer))
        bufferDesc.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
    if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::IndexBuffer))
        bufferDesc.BindFlags |= D3D11_BIND_INDEX_BUFFER;
    if (useSRV)
        bufferDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
    if (useUAV)
        bufferDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

    if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::Argument))
        bufferDesc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
    if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::RawBuffer))
        bufferDesc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::Structured))
    {
        bufferDesc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        bufferDesc.StructureByteStride = _desc.Stride;
    }

    // Create resource
    D3D11_SUBRESOURCE_DATA data;
    if (_desc.InitData)
    {
        data.pSysMem = _desc.InitData;
        data.SysMemPitch = bufferDesc.ByteWidth;
        data.SysMemSlicePitch = 0;
    }
    VALIDATE_DIRECTX_CALL(_device->GetDevice()->CreateBuffer(&bufferDesc, _desc.InitData ? &data : nullptr, &_resource));

    // Set state
    DX_SET_DEBUG_NAME(_resource, GetName());
    _memoryUsage = _desc.Size;
    int32 numElements = _desc.GetElementsCount();

    // Create views
    if (useSRV)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::RawBuffer))
        {
            srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
            srvDesc.BufferEx.FirstElement = 0;
            srvDesc.BufferEx.NumElements = numElements;
            srvDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
        }
        else
        {
            if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::Structured))
                srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            else
                srvDesc.Format = RenderToolsDX::ToDxgiFormat(PixelFormatExtensions::FindShaderResourceFormat(_desc.Format, false));
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
            srvDesc.Buffer.FirstElement = 0;
            srvDesc.Buffer.NumElements = numElements;
        }
        ID3D11ShaderResourceView* srv;
        VALIDATE_DIRECTX_CALL(_device->GetDevice()->CreateShaderResourceView(_resource, &srvDesc, &srv));
        _view.SetSRV(srv);
    }
    if (useUAV)
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = numElements;
        uavDesc.Buffer.Flags = 0;
        if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::RawBuffer))
            uavDesc.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_RAW;
        if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::Append))
            uavDesc.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_APPEND;
        if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::Counter))
            uavDesc.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_COUNTER;
        if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::Structured))
            uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        else
            uavDesc.Format = RenderToolsDX::ToDxgiFormat(PixelFormatExtensions::FindUnorderedAccessFormat(_desc.Format));
        ID3D11UnorderedAccessView* uav;
        VALIDATE_DIRECTX_CALL(_device->GetDevice()->CreateUnorderedAccessView(_resource, &uavDesc, &uav));
        _view.SetUAV(uav);
    }

    return false;
}

void GPUBufferDX11::OnReleaseGPU()
{
    _view.Release();
    DX_SAFE_RELEASE_CHECK(_resource, 0);

    // Base
    GPUBuffer::OnReleaseGPU();
}

#endif
