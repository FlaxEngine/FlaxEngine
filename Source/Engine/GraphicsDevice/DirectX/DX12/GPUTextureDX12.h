// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "GPUDeviceDX12.h"
#include "IShaderResourceDX12.h"

#if GRAPHICS_API_DIRECTX12

/// <summary>
/// The texture view for DirectX 12 backend.
/// </summary>
class GPUTextureViewDX12 : public GPUTextureView, public IShaderResourceDX12
{
private:

    GPUDeviceDX12* _device = nullptr;
    ResourceOwnerDX12* _owner = nullptr;
    DescriptorHeapWithSlotsDX12::Slot _rtv, _srv, _dsv, _uav;

public:
    GPUTextureViewDX12()
    {
    }

    ~GPUTextureViewDX12()
    {
        Release();
    }

public:

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="parent">Parent resource</param>
    /// <param name="device">Graphics Device</param>
    /// <param name="owner">Resource owner</param>
    /// <param name="format">Parent texture format</param>
    /// <param name="msaa">Parent texture multi-sample level</param>
    /// <param name="subresourceIndex">Used subresource index or -1 to cover whole resource.</param>
    void Init(GPUResource* parent, GPUDeviceDX12* device, ResourceOwnerDX12* owner, PixelFormat format, MSAALevel msaa, int32 subresourceIndex = -1)
    {
        GPUTextureView::Init(parent, format, msaa);
        SubresourceIndex = subresourceIndex;
        _device = device;
        _owner = owner;
    }

    /// <summary>
    /// Releases the view.
    /// </summary>
    void Release();

public:

    bool ReadOnlyDepthView = false;
    void SetRTV(D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc);
    void SetSRV(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc);
    void SetDSV(D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc);
    void SetUAV(D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, ID3D12Resource* counterResource = nullptr);

public:

    /// <summary>
    /// Gets the CPU handle to the render target view descriptor.
    /// </summary>
    FORCE_INLINE D3D12_CPU_DESCRIPTOR_HANDLE RTV() const
    {
        return _rtv.CPU();
    }

    /// <summary>
    /// Gets the CPU handle to the depth stencil view descriptor.
    /// </summary>
    FORCE_INLINE D3D12_CPU_DESCRIPTOR_HANDLE DSV() const
    {
        return _dsv.CPU();
    }

public:

    // [GPUResourceView]
    void* GetNativePtr() const override
    {
        return (void*)(IShaderResourceDX12*)this;
    }

    // [IShaderResourceDX12]
    bool IsDepthStencilResource() const override
    {
        return _dsv.IsValid();
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
/// Texture object for DirectX 12 backend.
/// </summary>
class GPUTextureDX12 : public GPUResourceDX12<GPUTexture>, public ResourceOwnerDX12, public IShaderResourceDX12
{
private:

    GPUTextureViewDX12 _handleArray;
    GPUTextureViewDX12 _handleVolume;
    GPUTextureViewDX12 _handleReadOnlyDepth;
    Array<GPUTextureViewDX12> _handlesPerSlice; // [slice]
    Array<Array<GPUTextureViewDX12>> _handlesPerMip; // [slice][mip]

    DescriptorHeapWithSlotsDX12::Slot _srv;
    DescriptorHeapWithSlotsDX12::Slot _uav;

    DXGI_FORMAT _dxgiFormatDSV;
    DXGI_FORMAT _dxgiFormatSRV;
    DXGI_FORMAT _dxgiFormatRTV;
    DXGI_FORMAT _dxgiFormatUAV;

public:

    GPUTextureDX12(GPUDeviceDX12* device, const StringView& name)
        : GPUResourceDX12<GPUTexture>(device, name)
    {
    }

private:

    void initHandles();

public:

    // [GPUTexture]
    GPUTextureView* View(int32 arrayOrDepthIndex) const override
    {
        return (GPUTextureView*)&_handlesPerSlice[arrayOrDepthIndex];
    }
    GPUTextureView* View(int32 arrayOrDepthIndex, int32 mipMapIndex) const override
    {
        return (GPUTextureView*)&_handlesPerMip[arrayOrDepthIndex][mipMapIndex];
    }
    GPUTextureView* ViewArray() const override
    {
        ASSERT(ArraySize() > 1);
        return (GPUTextureView*)&_handleArray;
    }
    GPUTextureView* ViewVolume() const override
    {
        ASSERT(IsVolume());
        return (GPUTextureView*)&_handleVolume;
    }
    GPUTextureView* ViewReadOnlyDepth() const override
    {
        ASSERT(_desc.Flags & GPUTextureFlags::ReadOnlyDepthView);
        return (GPUTextureView*)&_handleReadOnlyDepth;
    }
    void* GetNativePtr() const override
    {
        return (void*)_resource;
    }
    bool GetData(int32 arrayIndex, int32 mipMapIndex, TextureMipData& data, uint32 mipRowPitch) override;

    // [ResourceOwnerDX12]
    GPUResource* AsGPUResource() const override
    {
        return (GPUResource*)this;
    }

    // [IShaderResourceDX12]
    bool IsDepthStencilResource() const override
    {
        return (_desc.Flags & GPUTextureFlags::DepthStencil) != GPUTextureFlags::None;
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
        return (ResourceOwnerDX12*)this;
    }

protected:

    // [GPUTexture]
    bool OnInit() override;
    void OnResidentMipsChanged() override;
    void OnReleaseGPU() override;
};

#endif
