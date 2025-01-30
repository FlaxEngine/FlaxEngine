// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUResource.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "../GPUDeviceDX.h"
#include "../IncludeDirectXHeaders.h"

#if GRAPHICS_API_DIRECTX11

class Engine;
enum class StencilOperation : byte;
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

    struct DepthStencilMode
    {
        int8 DepthEnable : 1;
        int8 DepthWriteEnable : 1;
        int8 DepthClipEnable : 1;
        int8 StencilEnable : 1;
        uint8 StencilReadMask;
        uint8 StencilWriteMask;
        ComparisonFunc DepthFunc;
        ComparisonFunc StencilFunc;
        StencilOperation StencilFailOp;
        StencilOperation StencilDepthFailOp;
        StencilOperation StencilPassOp;

        bool operator==(const DepthStencilMode& other) const
        {
            return Platform::MemoryCompare(this, &other, sizeof(DepthStencilMode)) == 0;
        }

        friend uint32 GetHash(const DepthStencilMode& key)
        {
            uint32 hash = 0;
            for (int32 i = 0; i < sizeof(DepthStencilMode) / 4; i++)
                CombineHash(hash, ((uint32*)&key)[i]);
            return hash;
        }
    };

    // Private Stuff
    ID3D11Device* _device = nullptr;
    ID3D11DeviceContext* _imContext = nullptr;
    IDXGIFactory* _factoryDXGI;

    GPUContextDX11* _mainContext = nullptr;
    bool _allowTearing = false;

    // Static Samplers
    ID3D11SamplerState* _samplerLinearClamp = nullptr;
    ID3D11SamplerState* _samplerPointClamp = nullptr;
    ID3D11SamplerState* _samplerLinearWrap = nullptr;
    ID3D11SamplerState* _samplerPointWrap = nullptr;
    ID3D11SamplerState* _samplerShadow = nullptr;
    ID3D11SamplerState* _samplerShadowLinear = nullptr;

    // Shared data for pipeline states
    CriticalSection StatesWriteLocker;
    Dictionary<BlendingMode, ID3D11BlendState*> BlendStates;
    Dictionary<DepthStencilMode, ID3D11DepthStencilState*> DepthStencilStates;
    ID3D11RasterizerState* RasterizerStates[3 * 2 * 2]; // Index =  CullMode[0-2] + Wireframe[0?3] + DepthClipEnable[0?6]

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

    ID3D11DepthStencilState* GetDepthStencilState(const void* descriptionPtr);
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
