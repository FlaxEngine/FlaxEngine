// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX11

#include "Engine/Graphics/GPUBuffer.h"
#include "GPUDeviceDX11.h"
#include "../IncludeDirectXHeaders.h"
#include "IShaderResourceDX11.h"

/// <summary>
/// The buffer view for DirectX 11 backend.
/// </summary>
/// <seealso cref="GPUBufferView" />
/// <seealso cref="IShaderResourceDX11" />
class GPUBufferViewDX11 : public GPUBufferView, public IShaderResourceDX11
{
private:

    ID3D11ShaderResourceView* _srv = nullptr;
    ID3D11UnorderedAccessView* _uav = nullptr;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUBufferViewDX11"/> class.
    /// </summary>
    GPUBufferViewDX11()
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUBufferViewDX11"/> class.
    /// </summary>
    ~GPUBufferViewDX11()
    {
        Release();
    }

public:

    void SetParnet(GPUBuffer* parent)
    {
        _parent = parent;
    }

    /// <summary>
    /// Release the view.
    /// </summary>
    void Release()
    {
        DX_SAFE_RELEASE_CHECK(_srv, 0);
        DX_SAFE_RELEASE_CHECK(_uav, 0);
    }

public:

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
    /// Sets new unordered access view.
    /// </summary>
    /// <param name="uav">A new unordered access view.</param>
    void SetUAV(ID3D11UnorderedAccessView* uav)
    {
        DX_SAFE_RELEASE_CHECK(_uav, 0);
        _uav = uav;
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
/// GPU buffer for DirectX 11 backend.
/// </summary>
/// <seealso cref="GPUResourceDX11" />
class GPUBufferDX11 : public GPUResourceDX11<GPUBuffer>
{
private:

    ID3D11Buffer* _resource = nullptr;
    GPUBufferViewDX11 _view;
    bool _mapped = false;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUBufferDX11"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="name">The resource name.</param>
    GPUBufferDX11(GPUDeviceDX11* device, const StringView& name);

public:

    /// <summary>
    /// Gets DirectX 11 buffer object handle
    /// </summary>
    /// <returns>Buffer</returns>
    FORCE_INLINE ID3D11Buffer* GetBuffer() const
    {
        return _resource;
    }

public:

    // [GPUBuffer]
    GPUBufferView* View() const override;
    void* Map(GPUResourceMapMode mode) override;
    void Unmap() override;

    // [GPUResourceDX11]
    ID3D11Resource* GetResource() override
    {
        return _resource;
    }

protected:

    // [GPUBuffer]
    bool OnInit() override;
    void OnReleaseGPU() override;
};

#endif
