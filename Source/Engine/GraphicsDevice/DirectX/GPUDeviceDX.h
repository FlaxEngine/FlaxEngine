// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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

namespace Windows
{
    typedef struct _devicemodeW
    {
        WCHAR dmDeviceName[32];
        WORD dmSpecVersion;
        WORD dmDriverVersion;
        WORD dmSize;
        WORD dmDriverExtra;
        DWORD dmFields;

        union
        {
            struct
            {
                short dmOrientation;
                short dmPaperSize;
                short dmPaperLength;
                short dmPaperWidth;
                short dmScale;
                short dmCopies;
                short dmDefaultSource;
                short dmPrintQuality;
            } DUMMYSTRUCTNAME;

            POINTL dmPosition;

            struct
            {
                POINTL dmPosition;
                DWORD dmDisplayOrientation;
                DWORD dmDisplayFixedOutput;
            } DUMMYSTRUCTNAME2;
        } DUMMYUNIONNAME;

        short dmColor;
        short dmDuplex;
        short dmYResolution;
        short dmTTOption;
        short dmCollate;
        WCHAR dmFormName[32];
        WORD dmLogPixels;
        DWORD dmBitsPerPel;
        DWORD dmPelsWidth;
        DWORD dmPelsHeight;

        union
        {
            DWORD dmDisplayFlags;
            DWORD dmNup;
        } DUMMYUNIONNAME2;

        DWORD dmDisplayFrequency;
        DWORD dmICMMethod;
        DWORD dmICMIntent;
        DWORD dmMediaType;
        DWORD dmDitherType;
        DWORD dmReserved1;
        DWORD dmReserved2;
        DWORD dmPanningWidth;
        DWORD dmPanningHeight;
    } DEVMODEW, *PDEVMODEW, *NPDEVMODEW, *LPDEVMODEW;

    WIN_API BOOL WIN_API_CALLCONV EnumDisplaySettingsW(LPCWSTR lpszDeviceName, DWORD iModeNum, DEVMODEW* lpDevMode);
}

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
    }

public:

    /// <summary>
    /// Gets DirectX device feature level
    /// </summary>
    /// <returns>D3D feature level</returns>
    FORCE_INLINE D3D_FEATURE_LEVEL GetD3DFeatureLevel() const
    {
        return _adapter->MaxFeatureLevel;
    }

    /// <summary>
    /// The video outputs.
    /// </summary>
    Array<VideoOutputDX> Outputs;

protected:

    void UpdateOutputs(IDXGIAdapter* adapter)
    {
#if PLATFORM_WINDOWS
        // Collect output devices
        uint32 outputIdx = 0;
        ComPtr<IDXGIOutput> output;
        DXGI_FORMAT defaultBackbufferFormat = RenderToolsDX::ToDxgiFormat(GPU_BACK_BUFFER_PIXEL_FORMAT);
        Array<DXGI_MODE_DESC> modeDesc;
        while (adapter->EnumOutputs(outputIdx, &output) != DXGI_ERROR_NOT_FOUND)
        {
            auto& outputDX11 = Outputs.AddOne();

            outputDX11.Output = output;
            output->GetDesc(&outputDX11.Desc);

            uint32 numModes = 0;
            HRESULT hr = output->GetDisplayModeList(defaultBackbufferFormat, 0, &numModes, nullptr);
            if (FAILED(hr))
            {
                LOG(Warning, "Error while enumerating adapter output video modes.");
                continue;
            }

            modeDesc.Resize(numModes, false);
            hr = output->GetDisplayModeList(defaultBackbufferFormat, 0, &numModes, modeDesc.Get());
            if (FAILED(hr))
            {
                LOG(Warning, "Error while enumerating adapter output video modes.");
                continue;
            }

            for (auto& mode : modeDesc)
            {
                bool foundVideoMode = false;
                for (auto& videoMode : outputDX11.VideoModes)
                {
                    if (videoMode.Width == mode.Width &&
                        videoMode.Height == mode.Height &&
                        videoMode.RefreshRate.Numerator == mode.RefreshRate.Numerator &&
                        videoMode.RefreshRate.Denominator == mode.RefreshRate.Denominator)
                    {
                        foundVideoMode = true;
                        break;
                    }
                }

                if (!foundVideoMode)
                {
                    outputDX11.VideoModes.Add(mode);

                    // Collect only from the main monitor
                    if (Outputs.Count() == 1)
                    {
                        VideoOutputModes.Add({ mode.Width, mode.Height, (uint32)(mode.RefreshRate.Numerator / (float)mode.RefreshRate.Denominator) });
                    }
                }
            }

            // Get desktop display mode
            HMONITOR hMonitor = outputDX11.Desc.Monitor;
            MONITORINFOEX monitorInfo;
            monitorInfo.cbSize = sizeof(MONITORINFOEX);
            GetMonitorInfo(hMonitor, &monitorInfo);

            Windows::DEVMODEW devMode;
            devMode.dmSize = sizeof(Windows::DEVMODEW);
            devMode.dmDriverExtra = 0;
            Windows::EnumDisplaySettingsW(monitorInfo.szDevice, ((DWORD)-1), &devMode);

            DXGI_MODE_DESC currentMode;
            currentMode.Width = devMode.dmPelsWidth;
            currentMode.Height = devMode.dmPelsHeight;
            bool useDefaultRefreshRate = 1 == devMode.dmDisplayFrequency || 0 == devMode.dmDisplayFrequency;
            currentMode.RefreshRate.Numerator = useDefaultRefreshRate ? 0 : devMode.dmDisplayFrequency;
            currentMode.RefreshRate.Denominator = useDefaultRefreshRate ? 0 : 1;
            currentMode.Format = defaultBackbufferFormat;
            currentMode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
            currentMode.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

            if (output->FindClosestMatchingMode(&currentMode, &outputDX11.DesktopViewMode, nullptr) != S_OK)
                outputDX11.DesktopViewMode = currentMode;

            float refreshRate = outputDX11.DesktopViewMode.RefreshRate.Numerator / (float)outputDX11.DesktopViewMode.RefreshRate.Denominator;
            LOG(Info, "Video output '{0}' {1}x{2} {3} Hz", outputDX11.Desc.DeviceName, devMode.dmPelsWidth, devMode.dmPelsHeight, refreshRate);
            outputIdx++;
        }
#endif
    }

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
