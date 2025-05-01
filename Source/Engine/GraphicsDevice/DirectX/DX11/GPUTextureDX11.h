// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "GPUDeviceDX11.h"
#include "IShaderResourceDX11.h"

#if GRAPHICS_API_DIRECTX11

/// <summary>
/// The texture view for DirectX 11 backend.
/// </summary>
/// <seealso cref="GPUTextureView" />
/// <seealso cref="IShaderResourceDX11" />
class GPUTextureViewDX11 : public GPUTextureView, public IShaderResourceDX11
{
private:

    ID3D11RenderTargetView* _rtv = nullptr;
    ID3D11ShaderResourceView* _srv = nullptr;
    ID3D11DepthStencilView* _dsv = nullptr;
    ID3D11UnorderedAccessView* _uav = nullptr;

public:
    GPUTextureViewDX11()
    {
    }

    ~GPUTextureViewDX11()
    {
        Release();
    }

public:

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="parent">Resource that owns that handle</param>
    /// <param name="rtv">Render Target View</param>
    /// <param name="srv">Shader Resource view</param>
    /// <param name="dsv">Depth Stencil View</param>
    /// <param name="uav">Unordered Access View</param>
    /// <param name="format">Parent texture format</param>
    /// <param name="msaa">Parent texture multi-sample level</param>
    void Init(GPUResource* parent, ID3D11RenderTargetView* rtv, ID3D11ShaderResourceView* srv, ID3D11DepthStencilView* dsv, ID3D11UnorderedAccessView* uav, PixelFormat format, MSAALevel msaa)
    {
        GPUTextureView::Init(parent, format, msaa);
        _rtv = rtv;
        _srv = srv;
        _dsv = dsv;
        _uav = uav;
    }

    /// <summary>
    /// Release the view.
    /// </summary>
    void Release()
    {
        DX_SAFE_RELEASE_CHECK(_rtv, 0);
        DX_SAFE_RELEASE_CHECK(_srv, 0);
        DX_SAFE_RELEASE_CHECK(_dsv, 0);
        DX_SAFE_RELEASE_CHECK(_uav, 0);
    }

public:

    /// <summary>
    /// Sets new render target view.
    /// </summary>
    /// <param name="rtv">A new render target view.</param>
    void SetRTV(ID3D11RenderTargetView* rtv)
    {
        DX_SAFE_RELEASE_CHECK(_rtv, 0);
        _rtv = rtv;
    }

    /// <summary>
    /// Sets new shader resource view.
    /// </summary>
    /// <param name="srv">A new shader resource view.</param>
    void SetSRV(ID3D11ShaderResourceView* srv)
    {
        DX_SAFE_RELEASE_CHECK(_srv, 0);
        _srv = srv;
    }

    /// <summary>
    /// Sets new depth stencil view.
    /// </summary>
    /// <param name="dsv">A new depth stencil view.</param>
    void SetDSV(ID3D11DepthStencilView* dsv)
    {
        DX_SAFE_RELEASE_CHECK(_dsv, 0);
        _dsv = dsv;
    }

    /// <summary>
    /// Sets new unordered access view.
    /// </summary>
    /// <param name="uav">A new unordered access view.</param>
    void SetUAV(ID3D11UnorderedAccessView* uav)
    {
        DX_SAFE_RELEASE_CHECK(_uav, 0);
        _uav = uav;
    }

public:

    /// <summary>
    /// Gets the render target view.
    /// </summary>
    /// <returns>The render target view.</returns>
    ID3D11RenderTargetView* RTV() const
    {
        return _rtv;
    }

    /// <summary>
    /// Gets the depth stencil view.
    /// </summary>
    /// <returns>The depth stencil view.</returns>
    ID3D11DepthStencilView* DSV() const
    {
        return _dsv;
    }

public:

    // [GPUResourceView]
    void* GetNativePtr() const override
    {
        return (void*)(IShaderResourceDX11*)this;
    }

    // [IShaderResourceDX11]
    ID3D11ShaderResourceView* SRV() const override
    {
        return _srv;
    }

    ID3D11UnorderedAccessView* UAV() const override
    {
        return _uav;
    }
};

/// <summary>
/// Texture object for DirectX 11 backend.
/// </summary>
class GPUTextureDX11 : public GPUResourceDX11<GPUTexture>
{
private:
    ID3D11Resource* _resource = nullptr;

    GPUTextureViewDX11 _handleArray;
    GPUTextureViewDX11 _handleVolume;
    GPUTextureViewDX11 _handleReadOnlyDepth;
    Array<GPUTextureViewDX11> _handlesPerSlice; // [slice]
    Array<Array<GPUTextureViewDX11>> _handlesPerMip; // [slice][mip]

    DXGI_FORMAT _dxgiFormatDSV;
    DXGI_FORMAT _dxgiFormatSRV;
    DXGI_FORMAT _dxgiFormatRTV;
    DXGI_FORMAT _dxgiFormatUAV;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUTextureDX11"/> class.
    /// </summary>
    /// <param name="device">The device.</param>
    /// <param name="name">The name.</param>
    GPUTextureDX11(GPUDeviceDX11* device, const StringView& name)
        : GPUResourceDX11<GPUTexture>(device, name)
    {
    }

public:
    /// <summary>
    /// Gets DX11 texture resource.
    /// </summary>
    FORCE_INLINE ID3D11Resource* GetResource() const
    {
        return _resource;
    }

private:
    void initHandles();

    ID3D11Texture2D* GetTexture2D() const
    {
        ASSERT(_desc.Dimensions == TextureDimensions::Texture || _desc.Dimensions == TextureDimensions::CubeTexture);
        return (ID3D11Texture2D*)_resource;
    }
    ID3D11Texture3D* GetTexture3D() const
    {
        ASSERT(_desc.Dimensions == TextureDimensions::VolumeTexture);
        return (ID3D11Texture3D*)_resource;
    }

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
        return static_cast<void*>(_resource);
    }
    bool GetData(int32 arrayIndex, int32 mipMapIndex, TextureMipData& data, uint32 mipRowPitch) override;

    // [GPUResourceDX11]
    ID3D11Resource* GetResource() override
    {
        return _resource;
    }

protected:
    // [GPUTexture]
    bool OnInit() override;
    void OnResidentMipsChanged() override;
    void OnReleaseGPU() override;
};

#endif
