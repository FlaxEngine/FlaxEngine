// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUBuffer.h"
#include "GPUDeviceDX12.h"
#include "IShaderResourceDX12.h"
#include "../IncludeDirectXHeaders.h"

#if GRAPHICS_API_DIRECTX12

/// <summary>
/// The buffer view for DirectX 12 backend.
/// </summary>
class GPUBufferViewDX12 : public GPUBufferView, public IShaderResourceDX12
{
private:

    GPUDeviceDX12* _device = nullptr;
    ResourceOwnerDX12* _owner = nullptr;
    DescriptorHeapWithSlotsDX12::Slot _srv, _uav;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUBufferViewDX12"/> class.
    /// </summary>
    GPUBufferViewDX12()
    {
        SrvDimension = D3D12_SRV_DIMENSION_BUFFER;
        UavDimension = D3D12_UAV_DIMENSION_BUFFER;
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUBufferViewDX12"/> class.
    /// </summary>
    ~GPUBufferViewDX12()
    {
        Release();
    }

public:

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="owner">The resource owner.</param>
    /// <param name="parent">The parent resource.</param>
    void Init(GPUDeviceDX12* device, ResourceOwnerDX12* owner, GPUResource* parent)
    {
        _device = device;
        _owner = owner;
        _parent = parent;
    }

    /// <summary>
    /// Releases the view.
    /// </summary>
    void Release()
    {
        _srv.Release();
        _uav.Release();
    }

public:

    /// <summary>
    /// Sets the shader resource view.
    /// </summary>
    /// <param name="srvDesc">The SRV desc.</param>
    void SetSRV(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc);

    /// <summary>
    /// Sets the unordered access view.
    /// </summary>
    /// <param name="uavDesc">The UAV desc.</param>
    /// <param name="counterResource">The counter buffer resource.</param>
    void SetUAV(D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, ID3D12Resource* counterResource = nullptr);

public:

    // [GPUResourceView]
    void* GetNativePtr() const override
    {
        return (void*)(IShaderResourceDX12*)this;
    }

    // [IShaderResourceDX12]
    bool IsDepthStencilResource() const override
    {
        return false;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE SRV() const override
    {
        return _srv.CPU();
    }
    D3D12_CPU_DESCRIPTOR_HANDLE UAV() const override
    {
        return _uav.CPU();
    }
    ResourceOwnerDX12* GetResourceOwner() const override
    {
        return _owner;
    }
};

/// <summary>
/// GPU buffer for DirectX 12 backend.
/// </summary>
/// <seealso cref="GPUResourceDX12" />
class GPUBufferDX12 : public GPUResourceDX12<GPUBuffer>, public ResourceOwnerDX12
{
private:

    GPUBufferViewDX12 _view;
    GPUBufferDX12* _counter = nullptr;
    GPUResourceMapMode _lastMapMode = (GPUResourceMapMode)255;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUBufferDX12"/> class.
    /// </summary>
    /// <param name="device">The device.</param>
    /// <param name="name">The name.</param>
    GPUBufferDX12(GPUDeviceDX12* device, const StringView& name)
        : GPUResourceDX12<GPUBuffer>(device, name)
    {
    }

public:

    /// <summary>
    /// Gets vertex buffer view descriptor. Valid only for the vertex buffers.
    /// </summary>
    FORCE_INLINE void GetVBView(D3D12_VERTEX_BUFFER_VIEW& view) const
    {
        view.StrideInBytes = GetStride();
        view.SizeInBytes = (UINT)GetSizeInBytes();
        view.BufferLocation = GetLocation();
    }

    /// <summary>
    /// Gets index buffer view descriptor. Valid only for the index buffers.
    /// </summary>
    FORCE_INLINE void GetIBView(D3D12_INDEX_BUFFER_VIEW& view) const
    {
        view.Format = GetStride() == 4 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
        view.SizeInBytes = (UINT)GetSizeInBytes();
        view.BufferLocation = GetLocation();
    }

    /// <summary>
    /// Gets buffer size in a GPU memory in bytes.
    /// </summary>
    uint64 GetSizeInBytes() const;

    /// <summary>
    /// Gets buffer location in a GPU memory.
    /// </summary>
    D3D12_GPU_VIRTUAL_ADDRESS GetLocation() const;

    /// <summary>
    /// Gets the counter resource.
    /// </summary>
    FORCE_INLINE GPUBufferDX12* GetCounter() const
    {
        return _counter;
    }

public:

    // [GPUBuffer]
    GPUBufferView* View() const override;
    void* Map(GPUResourceMapMode mode) override;
    void Unmap() override;

    // [ResourceOwnerDX12]
    GPUResource* AsGPUResource() const override;

protected:

    // [GPUBuffer]
    bool OnInit() override;
    void OnReleaseGPU() override;
};

#endif
