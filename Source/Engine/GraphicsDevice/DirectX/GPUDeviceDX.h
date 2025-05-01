// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX11 || GRAPHICS_API_DIRECTX12

#include "GPUAdapterDX.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Platform/MemoryStats.h"
#include "RenderToolsDX.h"

struct VideoOutputDX
{
public:

    ComPtr<IDXGIOutput> Output;
    uint32 RefreshRateNumerator;
    uint32 RefreshRateDenominator;
    DXGI_OUTPUT_DESC Desc;
    DXGI_MODE_DESC DesktopViewMode;
    Array<DXGI_MODE_DESC> VideoModes;
};

/// <summary>
/// Base for all DirectX graphics devices.
/// </summary>
class GPUDeviceDX : public GPUDevice
{
protected:
    GPUAdapterDX* _adapter;

protected:
    GPUDeviceDX(RendererType type, ShaderProfile profile, GPUAdapterDX* adapter)
        : GPUDevice(type, profile)
        , _adapter(adapter)
    {
        adapter->GetDriverVersion();
    }

public:
    /// <summary>
    /// The video outputs.
    /// </summary>
    Array<VideoOutputDX> Outputs;

protected:

    void UpdateOutputs(IDXGIAdapter* adapter);

    static RendererType getRendererType(GPUAdapterDX* adapter)
    {
        switch (adapter->MaxFeatureLevel)
        {
        case D3D_FEATURE_LEVEL_10_0:
            return RendererType::DirectX10;
        case D3D_FEATURE_LEVEL_10_1:
            return RendererType::DirectX10_1;
        case D3D_FEATURE_LEVEL_11_0:
        case D3D_FEATURE_LEVEL_11_1:
            return RendererType::DirectX11;
#if GRAPHICS_API_DIRECTX12
        case D3D_FEATURE_LEVEL_12_0:
        case D3D_FEATURE_LEVEL_12_1:
            return RendererType::DirectX12;
#endif
        default:
            return RendererType::Unknown;
        }
    }

    static ShaderProfile getShaderProfile(GPUAdapterDX* adapter)
    {
        switch (adapter->MaxFeatureLevel)
        {
        case D3D_FEATURE_LEVEL_10_0:
        case D3D_FEATURE_LEVEL_10_1:
            return ShaderProfile::DirectX_SM4;
        case D3D_FEATURE_LEVEL_11_0:
        case D3D_FEATURE_LEVEL_11_1:
            return ShaderProfile::DirectX_SM5;
#if GRAPHICS_API_DIRECTX12
        case D3D_FEATURE_LEVEL_12_0:
        case D3D_FEATURE_LEVEL_12_1:
            return ShaderProfile::DirectX_SM5;
#endif
        default:
            return ShaderProfile::Unknown;
        }
    }

public:

    // [GPUDevice]
    GPUAdapter* GetAdapter() const override
    {
        return _adapter;
    }

protected:

    // [GPUDevice]
    bool Init() override
    {
        const DXGI_ADAPTER_DESC& adapterDesc = _adapter->Description;
        const uint64 dedicatedVideoMemory = static_cast<uint64>(adapterDesc.DedicatedVideoMemory);
        const uint64 dedicatedSystemMemory = static_cast<uint64>(adapterDesc.DedicatedSystemMemory);
        const uint64 sharedSystemMemory = static_cast<uint64>(adapterDesc.SharedSystemMemory);

        // Calculate total GPU memory
        const uint64 totalPhysicalMemory = Math::Min(Platform::GetMemoryStats().TotalPhysicalMemory, 16Ull * 1024Ull * 1024Ull * 1024Ull);
        const uint64 totalSystemMemory = Math::Min(sharedSystemMemory / 2Ull, totalPhysicalMemory / 4Ull);
        TotalGraphicsMemory = 0;
        if (_adapter->IsIntel())
        {
            TotalGraphicsMemory = dedicatedVideoMemory + dedicatedSystemMemory + totalSystemMemory;
        }
        else if (dedicatedVideoMemory >= 200 * 1024 * 1024)
        {
            TotalGraphicsMemory = dedicatedVideoMemory;
        }
        else if (dedicatedSystemMemory >= 200 * 1024 * 1024)
        {
            TotalGraphicsMemory = dedicatedSystemMemory;
        }
        else if (sharedSystemMemory >= 400 * 1024 * 1024)
        {
            TotalGraphicsMemory = totalSystemMemory;
        }
        else
        {
            TotalGraphicsMemory = totalPhysicalMemory / 4Ull;
        }

        // Base
        return GPUDevice::Init();
    }

    void Dispose() override
    {
        Outputs.Resize(0);

        GPUDevice::Dispose();
    }
};

#endif
