// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUResource.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "../GPUDeviceDX.h"
#include "../IncludeDirectXHeaders.h"

#if GRAPHICS_API_DIRECTX11

class Engine;
class GPUContextDX11;
class GPUSwapChainDX11;

/// <summary>
/// Implementation of Graphics Device for DirectX 11 backend.
/// </summary>
class GPUDeviceDX11 : public GPUDeviceDX
{
    friend GPUContextDX11;
    friend GPUSwapChainDX11;
private:

    // Private Stuff
    ID3D11Device* _device;
    ID3D11DeviceContext* _imContext;
    IDXGIFactory* _factoryDXGI;

    GPUContextDX11* _mainContext;
    bool _allowTearing = false;

    // Static Samplers
    ID3D11SamplerState* _samplerLinearClamp;
    ID3D11SamplerState* _samplerPointClamp;
    ID3D11SamplerState* _samplerLinearWrap;
    ID3D11SamplerState* _samplerPointWrap;
    ID3D11SamplerState* _samplerShadow;
    ID3D11SamplerState* _samplerShadowPCF;

    // Shared data for pipeline states
    CriticalSection BlendStatesWriteLocker;
    Dictionary<BlendingMode, ID3D11BlendState*> BlendStates;
    ID3D11RasterizerState* RasterizerStates[3 * 2 * 2]; // Index =  CullMode[0-2] + Wireframe[0?3] + DepthClipEnable[0?6]
    ID3D11DepthStencilState* DepthStencilStates[9 * 2 * 2]; // Index = ComparisonFunc[0-8] + DepthTestEnable[0?9] + DepthWriteEnable[0?18]

public:
    static GPUDevice* Create();
    GPUDeviceDX11(IDXGIFactory* dxgiFactory, GPUAdapterDX* adapter);
    ~GPUDeviceDX11();

public:

    // Gets DX11 device
    ID3D11Device* GetDevice() const
    {
        return _device;
    }

    // Gets DXGI factory
    IDXGIFactory* GetDXGIFactory() const
    {
        return _factoryDXGI;
    }

    // Gets immediate context
    ID3D11DeviceContext* GetIM() const
    {
        return _imContext;
    }

    GPUContextDX11* GetMainContextDX11()
    {
        return _mainContext;
    }

    ID3D11BlendState* GetBlendState(const BlendingMode& blending);

public:

    // [GPUDeviceDX]
    GPUContext* GetMainContext() override
    {
        return reinterpret_cast<GPUContext*>(_mainContext);
    }
    void* GetNativePtr() const override
    {
        return _device;
    }
    bool Init() override;
    void Dispose() override;
    void WaitForGPU() override;
    void DrawEnd() override;
    GPUTexture* CreateTexture(const StringView& name) override;
    GPUShader* CreateShader(const StringView& name) override;
    GPUPipelineState* CreatePipelineState() override;
    GPUTimerQuery* CreateTimerQuery() override;
    GPUBuffer* CreateBuffer(const StringView& name) override;
    GPUSampler* CreateSampler() override;
    GPUSwapChain* CreateSwapChain(Window* window) override;
    GPUConstantBuffer* CreateConstantBuffer(uint32 size, const StringView& name) override;
};

/// <summary>
/// Base interface for GPU resources on DirectX 11
/// </summary>
class IGPUResourceDX11
{
public:

    /// <summary>
    /// Gets DirectX 11 resource object handle.
    /// </summary>
    virtual ID3D11Resource* GetResource() = 0;
};

/// <summary>
/// GPU resource implementation for DirectX 11 backend.
/// </summary>
template<class BaseType>
class GPUResourceDX11 : public GPUResourceBase<GPUDeviceDX11, BaseType>, public IGPUResourceDX11
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUResourceDX11"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="name">The resource name.</param>
    GPUResourceDX11(GPUDeviceDX11* device, const StringView& name) noexcept
        : GPUResourceBase<GPUDeviceDX11, BaseType>(device, name)
    {
    }
};

extern GPUDevice* CreateGPUDeviceDX11();

#endif
