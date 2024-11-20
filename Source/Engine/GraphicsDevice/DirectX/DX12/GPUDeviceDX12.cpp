// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "GPUDeviceDX12.h"
#include "GPUShaderDX12.h"
#include "GPUContextDX12.h"
#include "GPUPipelineStateDX12.h"
#include "GPUTextureDX12.h"
#include "GPUTimerQueryDX12.h"
#include "GPUBufferDX12.h"
#include "GPUSamplerDX12.h"
#include "GPUSwapChainDX12.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Config/PlatformSettings.h"
#include "UploadBufferDX12.h"
#include "CommandQueueDX12.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Threading/Threading.h"
#include "CommandSignatureDX12.h"

static bool CheckDX12Support(IDXGIAdapter* adapter)
{
    // Try to create device
    if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
    {
        return true;
    }
    return false;
}

GPUDevice* GPUDeviceDX12::Create()
{
#if PLATFORM_XBOX_SCARLETT || PLATFORM_XBOX_ONE
    IDXGIFactory4* dxgiFactory = nullptr;
    GPUAdapterDX selectedAdapter;
    selectedAdapter.Index = 0;
    selectedAdapter.MaxFeatureLevel = D3D_FEATURE_LEVEL_12_0;
    Platform::MemoryClear(&selectedAdapter.Description, sizeof(selectedAdapter.Description));
    selectedAdapter.Description.VendorId = GPU_VENDOR_ID_AMD;
#else
#if !USE_EDITOR && PLATFORM_WINDOWS
	auto winSettings = WindowsPlatformSettings::Get();
	if (!winSettings->SupportDX12)
	{
		// Skip if there is no support
		LOG(Warning, "Cannot use DirectX 12 (support disabled).");
		return nullptr;
	}
#endif

    // Debug Layer
#if GPU_ENABLE_DIAGNOSTICS
    ComPtr<ID3D12Debug> debugLayer;
    D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
    if (debugLayer)
    {
        debugLayer->EnableDebugLayer();
        LOG(Info, "DirectX debugging layer enabled");
    }
#if 0
#ifdef __ID3D12Debug1_FWD_DEFINED__
    ComPtr<ID3D12Debug1> debugLayer1;
    D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer1));
    if (debugLayer1)
    {
        // GPU-based validation and synchronized validation for debugging only
        debugLayer1->SetEnableGPUBasedValidation(true);
        debugLayer1->SetEnableSynchronizedCommandQueueValidation(true);
    }
#endif
#endif
#ifdef __ID3D12DeviceRemovedExtendedDataSettings_FWD_DEFINED__
    ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dredSettings;
    D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings));
    if (dredSettings)
    {
        // Turn on AutoBreadcrumbs and Page Fault reporting
        dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    }
#endif
#endif

    // Create DXGI factory (CreateDXGIFactory2 is supported on Windows 8.1 or newer)
    IDXGIFactory4* dxgiFactory;
    IDXGIFactory6* dxgiFactory6;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory6));
    if (hr == S_OK)
        dxgiFactory = dxgiFactory6;
    else
    {
        dxgiFactory6 = nullptr;
        hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
    }
    if (hr != S_OK)
    {
        LOG(Error, "Cannot create DXGI adapter. Error code: {0:x}.", hr);
        return nullptr;
    }

    // Enumerate the DXGIFactory's adapters
    int32 selectedAdapterIndex = -1;
    Array<GPUAdapterDX> adapters;
    IDXGIAdapter* tempAdapter;
    for (uint32 index = 0; dxgiFactory->EnumAdapters(index, &tempAdapter) != DXGI_ERROR_NOT_FOUND; index++)
    {
        // Try to use that adapter
        GPUAdapterDX adapter;
        if (tempAdapter && CheckDX12Support(tempAdapter))
        {
            adapter.Index = index;
            adapter.MaxFeatureLevel = D3D_FEATURE_LEVEL_12_0;
            VALIDATE_DIRECTX_CALL(tempAdapter->GetDesc(&adapter.Description));
            uint32 outputs = RenderToolsDX::CountAdapterOutputs(tempAdapter);

            // Send that info to the log
            LOG(Info, "Adapter {1}: '{0}', DirectX {2}", adapter.Description.Description, index, RenderToolsDX::GetFeatureLevelString(adapter.MaxFeatureLevel));
            LOG(Info, "	Dedicated Video Memory: {0}, Dedicated System Memory: {1}, Shared System Memory: {2}, Output(s): {3}", Utilities::BytesToText(adapter.Description.DedicatedVideoMemory), Utilities::BytesToText(adapter.Description.DedicatedSystemMemory), Utilities::BytesToText(adapter.Description.SharedSystemMemory), outputs);

            adapters.Add(adapter);
        }
    }

    // Find the best performing adapter and prefer using it instead of the first device
    const auto gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
    if (dxgiFactory6 != nullptr && selectedAdapterIndex == -1)
    {
        if (dxgiFactory6->EnumAdapterByGpuPreference(0, gpuPreference, IID_PPV_ARGS(&tempAdapter)) != DXGI_ERROR_NOT_FOUND)
        {
            GPUAdapterDX adapter;
            if (tempAdapter && CheckDX12Support(tempAdapter))
            {
                DXGI_ADAPTER_DESC desc;
                VALIDATE_DIRECTX_CALL(tempAdapter->GetDesc(&desc));
                for (int i = 0; i < adapters.Count(); i++)
                {
                    if (adapters[i].Description.AdapterLuid.LowPart == desc.AdapterLuid.LowPart &&
                        adapters[i].Description.AdapterLuid.HighPart == desc.AdapterLuid.HighPart)
                    {
                        selectedAdapterIndex = i;
                        break;
                    }
                }
            }
        }
    }

    // Select the adapter to use
    if (selectedAdapterIndex < 0)
        selectedAdapterIndex = 0;
    if (adapters.Count() == 0 || selectedAdapterIndex >= adapters.Count())
    {
        LOG(Error, "Failed to find valid DirectX adapter!");
        return nullptr;
    }
    GPUAdapterDX selectedAdapter = adapters[selectedAdapterIndex];
    uint32 vendorId = 0;
    if (CommandLine::Options.NVIDIA)
        vendorId = GPU_VENDOR_ID_NVIDIA;
    else if (CommandLine::Options.AMD)
        vendorId = GPU_VENDOR_ID_AMD;
    else if (CommandLine::Options.Intel)
        vendorId = GPU_VENDOR_ID_INTEL;
    if (vendorId != 0)
    {
        for (const auto& adapter : adapters)
        {
            if (adapter.GetVendorId() == vendorId)
            {
                selectedAdapter = adapter;
                break;
            }
        }
    }

    // Validate adapter
    if (!selectedAdapter.IsValid())
    {
        LOG(Error, "Failed to choose valid DirectX adapter!");
        return nullptr;
    }

    // Check if selected adapter does not support DirectX 12
    if (!selectedAdapter.IsSupportingDX12())
    {
        return nullptr;
    }
#endif

    // Create device
    auto device = New<GPUDeviceDX12>(dxgiFactory, New<GPUAdapterDX>(selectedAdapter));
    if (device->Init())
    {
        LOG(Warning, "Graphics Device init failed");
        Delete(device);
        return nullptr;
    }

    return device;
}

static MSAALevel GetMaximumMultisampleCount(ID3D12Device* device, DXGI_FORMAT dxgiFormat)
{
    int32 maxCount = 1;
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels = { dxgiFormat };
    for (int32 i = 2; i <= 8; i *= 2)
    {
        qualityLevels.SampleCount = i;
        if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &qualityLevels, sizeof(qualityLevels))) && qualityLevels.NumQualityLevels > 0)
            maxCount = i;
    }
    return static_cast<MSAALevel>(maxCount);
}

GPUDeviceDX12::GPUDeviceDX12(IDXGIFactory4* dxgiFactory, GPUAdapterDX* adapter)
    : GPUDeviceDX(RendererType::DirectX12, ShaderProfile::DirectX_SM6, adapter)
    , _device(nullptr)
    , _factoryDXGI(dxgiFactory)
    , _res2Dispose(256)
    , _rootSignature(nullptr)
    , _commandQueue(nullptr)
    , _mainContext(nullptr)
    , UploadBuffer(nullptr)
    , TimestampQueryHeap(this, D3D12_QUERY_HEAP_TYPE_TIMESTAMP, DX12_BACK_BUFFER_COUNT * 1024)
    , Heap_CBV_SRV_UAV(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4 * 1024, false)
    , Heap_RTV(this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1 * 1024, false)
    , Heap_DSV(this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 64, false)
    , Heap_Sampler(this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 128, false)
    , RingHeap_CBV_SRV_UAV(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 512 * 1024, true)
    , RingHeap_Sampler(this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1 * 1024, true)
{
}

bool GPUDeviceDX12::Init()
{
#if PLATFORM_XBOX_SCARLETT || PLATFORM_XBOX_ONE
    // Create DirectX device
    D3D12XBOX_CREATE_DEVICE_PARAMETERS params = {};
    params.Version = D3D12_SDK_VERSION;
#if GPU_ENABLE_DIAGNOSTICS
    params.ProcessDebugFlags = D3D12_PROCESS_DEBUG_FLAG_DEBUG_LAYER_ENABLED;
#elif !BUILD_RELEASE
    params.ProcessDebugFlags = D3D12XBOX_PROCESS_DEBUG_FLAG_INSTRUMENTED;
#endif
    params.GraphicsCommandQueueRingSizeBytes = static_cast<UINT>(D3D12XBOX_DEFAULT_SIZE_BYTES);
    params.GraphicsScratchMemorySizeBytes = static_cast<UINT>(D3D12XBOX_DEFAULT_SIZE_BYTES);
    params.ComputeScratchMemorySizeBytes = static_cast<UINT>(D3D12XBOX_DEFAULT_SIZE_BYTES);
#if PLATFORM_XBOX_SCARLETT
    params.DisableDXR = TRUE;
#endif
    VALIDATE_DIRECTX_CALL(D3D12XboxCreateDevice(nullptr, &params, IID_GRAPHICS_PPV_ARGS(&_device)));

    // Setup adapter
    D3D12XBOX_GPU_HARDWARE_CONFIGURATION hwConfig = {};
    _device->GetGpuHardwareConfigurationX(&hwConfig);
    const wchar_t* hwVer = L"Unknown";
    switch (hwConfig.HardwareVersion)
    {
    case D3D12XBOX_HARDWARE_VERSION_XBOX_ONE:
        hwVer = L"Xbox One";
        break;
    case D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_S:
        hwVer = L"Xbox One S";
        break;
    case D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X:
        hwVer = L"Xbox One X";
        break;
    case D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X_DEVKIT:
        hwVer = L"Xbox One X (DevKit)";
        break;
#ifdef _GAMING_XBOX_SCARLETT
    case D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART:
        hwVer = L"Scarlett Lockhart";
        break;
    case D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_ANACONDA:
        hwVer = L"Scarlett Anaconda";
        break;
    case D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_DEVKIT:
        hwVer = L"Scarlett Dev Kit";
        break;
#endif
    }
    LOG(Info, "Hardware Version: {0}", hwVer);
    updateFrameEvents();

#if PLATFORM_GDK
    GDKPlatform::Suspended.Bind<GPUDeviceDX12, &GPUDeviceDX12::OnSuspended>(this);
    GDKPlatform::Resumed.Bind<GPUDeviceDX12, &GPUDeviceDX12::OnResumed>(this);
#endif
#else
    // Get DXGI adapter
    IDXGIAdapter1* adapter;
    ASSERT(_factoryDXGI);
    if (_factoryDXGI->EnumAdapters1(_adapter->Index, &adapter) == DXGI_ERROR_NOT_FOUND || adapter == nullptr)
    {
        LOG(Warning, "Cannot get the adapter.");
        return true;
    }
    UpdateOutputs(adapter);

    // Create DirectX device
    VALIDATE_DIRECTX_CALL(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device)));

#if PLATFORM_WINDOWS
    // Detect RenderDoc usage (UUID {A7AA6116-9C8D-4BBA-9083-B4D816B71B78})
    IUnknown* unknown = nullptr;
    const GUID uuidRenderDoc = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
    HRESULT hr = _device->QueryInterface(uuidRenderDoc, (void**)&unknown);
    if (SUCCEEDED(hr) && unknown)
    {
        IsDebugToolAttached = true;
        unknown->Release();
    }
    if (!IsDebugToolAttached && GetModuleHandleA("renderdoc.dll") != nullptr)
    {
        IsDebugToolAttached = true;
    }
#endif

    // Check if can use screen tearing on a swapchain
    ComPtr<IDXGIFactory5> factory5;
    _factoryDXGI->QueryInterface(IID_PPV_ARGS(&factory5));
    if (factory5)
    {
        BOOL allowTearing;
        if (SUCCEEDED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)))
            && allowTearing
#if PLATFORM_WINDOWS
            && !IsDebugToolAttached // Disable tearing with RenderDoc (prevents crashing)
#endif
        )
            AllowTearing = true;
    }

    // Debug Layer
#if GPU_ENABLE_DIAGNOSTICS
    ComPtr<ID3D12InfoQueue> infoQueue;
    VALIDATE_DIRECTX_CALL(_device->QueryInterface(IID_PPV_ARGS(&infoQueue)));
    if (infoQueue)
    {
        D3D12_INFO_QUEUE_FILTER filter;
        Platform::MemoryClear(&filter, sizeof(filter));

        D3D12_MESSAGE_SEVERITY denySeverity = D3D12_MESSAGE_SEVERITY_INFO;
        filter.DenyList.NumSeverities = 1;
        filter.DenyList.pSeverityList = &denySeverity;

        D3D12_MESSAGE_ID disabledMessages[] =
        {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,
            D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_PS_OUTPUT_RT_OUTPUT_MISMATCH,
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_CREATEINPUTLAYOUT_EMPTY_LAYOUT,
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_DUPLICATE_SUBRESOURCE_TRANSITIONS,
            D3D12_MESSAGE_ID_DRAW_EMPTY_SCISSOR_RECTANGLE,
        };

        filter.DenyList.NumIDs = ARRAY_COUNT(disabledMessages);
        filter.DenyList.pIDList = disabledMessages;

        infoQueue->AddStorageFilterEntries(&filter);

        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        //infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
    }
#endif
#endif

    // Change state
    _state = DeviceState::Created;

    // Spawn some info about the hardware
    D3D12_FEATURE_DATA_D3D12_OPTIONS options;
    VALIDATE_DIRECTX_CALL(_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options)));
    LOG(Info, "Tiled Resources Tier: {0}", (int32)options.TiledResourcesTier);
    LOG(Info, "Resource Binding Tier: {0}", (int32)options.ResourceBindingTier);
    LOG(Info, "Conservative Rasterization Tier: {0}", (int32)options.ConservativeRasterizationTier);
    LOG(Info, "Resource Heap Tier: {0}", (int32)options.ResourceHeapTier);

    // Init device limits
    {
        auto& limits = Limits;
        limits.HasCompute = true;
        limits.HasTessellation = GPU_ALLOW_TESSELLATION_SHADERS;
        limits.HasGeometryShaders = GPU_ALLOW_GEOMETRY_SHADERS;
        limits.HasInstancing = true;
        limits.HasVolumeTextureRendering = true;
        limits.HasDrawIndirect = true;
        limits.HasAppendConsumeBuffers = true;
        limits.HasSeparateRenderTargetBlendState = true;
        limits.HasDepthAsSRV = true;
        limits.HasDepthClip = true;
        limits.HasReadOnlyDepth = true;
        limits.HasMultisampleDepthAsSRV = true;
        limits.HasTypedUAVLoad = options.TypedUAVLoadAdditionalFormats != 0;
        limits.MaximumMipLevelsCount = D3D12_REQ_MIP_LEVELS;
        limits.MaximumTexture1DSize = D3D12_REQ_TEXTURE1D_U_DIMENSION;
        limits.MaximumTexture1DArraySize = D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION;
        limits.MaximumTexture2DSize = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        limits.MaximumTexture2DArraySize = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
        limits.MaximumTexture3DSize = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
        limits.MaximumTextureCubeSize = D3D12_REQ_TEXTURECUBE_DIMENSION;
        limits.MaximumSamplerAnisotropy = D3D12_DEFAULT_MAX_ANISOTROPY;

        for (int32 i = 0; i < static_cast<int32>(PixelFormat::MAX); i++)
        {
            const PixelFormat format = static_cast<PixelFormat>(i);
            const DXGI_FORMAT dxgiFormat = RenderToolsDX::ToDxgiFormat(format);
            D3D12_FEATURE_DATA_FORMAT_SUPPORT formatInfo = { dxgiFormat };
            if (FAILED(_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatInfo, sizeof(formatInfo))))
                formatInfo.Support1 = D3D12_FORMAT_SUPPORT1_NONE;
            const MSAALevel maximumMultisampleCount = GetMaximumMultisampleCount(_device, dxgiFormat);
            FeaturesPerFormat[i] = FormatFeatures(format, maximumMultisampleCount, (FormatSupport)formatInfo.Support1);
        }
    }

#if !BUILD_RELEASE
	// Prevent the GPU from overclocking or under-clocking to get consistent timings
    if (CommandLine::Options.ShaderProfile)
    {
	    _device->SetStablePowerState(TRUE);
    }
#endif

    // Setup resources
    _commandQueue = New<CommandQueueDX12>(this, D3D12_COMMAND_LIST_TYPE_DIRECT);
    if (_commandQueue->Init())
        return true;
    _mainContext = New<GPUContextDX12>(this, D3D12_COMMAND_LIST_TYPE_DIRECT);
    if (RingHeap_CBV_SRV_UAV.Init())
        return true;
    if (RingHeap_Sampler.Init())
        return true;

    // Create empty views
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    for (int32 i = 0; i < ARRAY_COUNT(_nullSrv); i++)
    {
        srvDesc.ViewDimension = (D3D12_SRV_DIMENSION)i;
        switch (srvDesc.ViewDimension)
        {
        case D3D12_SRV_DIMENSION_BUFFER:
            srvDesc.Buffer.FirstElement = 0;
            srvDesc.Buffer.NumElements = 0;
            srvDesc.Buffer.StructureByteStride = 0;
            srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
            break;
        case D3D12_SRV_DIMENSION_TEXTURE1D:
            srvDesc.Texture1D.MostDetailedMip = 0;
            srvDesc.Texture1D.MipLevels = 1;
            srvDesc.Texture1D.ResourceMinLODClamp = 0.0f;
            break;
        case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
            srvDesc.Texture1DArray.MostDetailedMip = 0;
            srvDesc.Texture1DArray.MipLevels = 1;
            srvDesc.Texture1DArray.FirstArraySlice = 0;
            srvDesc.Texture1DArray.ArraySize = 1;
            srvDesc.Texture1DArray.ResourceMinLODClamp = 0.0f;
            break;
        case D3D12_SRV_DIMENSION_UNKNOWN: // Map Unknown into Texture2D
        case D3D12_SRV_DIMENSION_TEXTURE2D:
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = 1;
            srvDesc.Texture2D.PlaneSlice = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            break;
        case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
            srvDesc.Texture2DArray.MostDetailedMip = 0;
            srvDesc.Texture2DArray.MipLevels = 1;
            srvDesc.Texture2DArray.FirstArraySlice = 0;
            srvDesc.Texture2DArray.ArraySize = 0;
            srvDesc.Texture2DArray.PlaneSlice = 0;
            srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
            break;
        case D3D12_SRV_DIMENSION_TEXTURE3D:
            srvDesc.Texture3D.MostDetailedMip = 0;
            srvDesc.Texture3D.MipLevels = 1;
            srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
            break;
        case D3D12_SRV_DIMENSION_TEXTURECUBE:
            srvDesc.TextureCube.MostDetailedMip = 0;
            srvDesc.TextureCube.MipLevels = 1;
            srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
            break;
        case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
            srvDesc.TextureCubeArray.MostDetailedMip = 0;
            srvDesc.TextureCubeArray.MipLevels = 1;
            srvDesc.TextureCubeArray.First2DArrayFace = 0;
            srvDesc.TextureCubeArray.NumCubes = 0;
            srvDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
            break;
        default:
            continue;
        }
        _nullSrv[i].CreateSRV(this, nullptr, &srvDesc);
    }
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
        uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = 0;
        uavDesc.Texture2D.PlaneSlice = 0;
        _nullUav.CreateUAV(this, nullptr, &uavDesc);
    }

    // Create root signature
    // TODO: maybe create set of different root signatures? for UAVs, for compute, for simple drawing, for post fx?
    {
        // Descriptor tables
        D3D12_DESCRIPTOR_RANGE r[3]; // SRV+UAV+Sampler
        {
            D3D12_DESCRIPTOR_RANGE& range = r[0];
            range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            range.NumDescriptors = GPU_MAX_SR_BINDED;
            range.BaseShaderRegister = 0;
            range.RegisterSpace = 0;
            range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        }
        {
            D3D12_DESCRIPTOR_RANGE& range = r[1];
            range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
            range.NumDescriptors = GPU_MAX_UA_BINDED;
            range.BaseShaderRegister = 0;
            range.RegisterSpace = 0;
            range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        }
        {
            D3D12_DESCRIPTOR_RANGE& range = r[2];
            range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            range.NumDescriptors = GPU_MAX_SAMPLER_BINDED - GPU_STATIC_SAMPLERS_COUNT;
            range.BaseShaderRegister = GPU_STATIC_SAMPLERS_COUNT;
            range.RegisterSpace = 0;
            range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        }

        // Root parameters
        D3D12_ROOT_PARAMETER rootParameters[GPU_MAX_CB_BINDED + 3];
        for (int32 i = 0; i < GPU_MAX_CB_BINDED; i++)
        {
            // CB
            D3D12_ROOT_PARAMETER& rootParam = rootParameters[DX12_ROOT_SIGNATURE_CB + i];
            rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            rootParam.Descriptor.ShaderRegister = i;
            rootParam.Descriptor.RegisterSpace = 0;
        }
        {
            // SRVs
            D3D12_ROOT_PARAMETER& rootParam = rootParameters[DX12_ROOT_SIGNATURE_SR];
            rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            rootParam.DescriptorTable.NumDescriptorRanges = 1;
            rootParam.DescriptorTable.pDescriptorRanges = &r[0];
        }
        {
            // UAVs
            D3D12_ROOT_PARAMETER& rootParam = rootParameters[DX12_ROOT_SIGNATURE_UA];
            rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            rootParam.DescriptorTable.NumDescriptorRanges = 1;
            rootParam.DescriptorTable.pDescriptorRanges = &r[1];
        }
        {
            // Samplers
            D3D12_ROOT_PARAMETER& rootParam = rootParameters[DX12_ROOT_SIGNATURE_SAMPLER];
            rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            rootParam.DescriptorTable.NumDescriptorRanges = 1;
            rootParam.DescriptorTable.pDescriptorRanges = &r[2];
        }

        // Static samplers
        D3D12_STATIC_SAMPLER_DESC staticSamplers[6];
        static_assert(GPU_STATIC_SAMPLERS_COUNT == ARRAY_COUNT(staticSamplers), "Update static samplers setup.");
        // Linear Clamp
        staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[0].MipLODBias = 0.0f;
        staticSamplers[0].MaxAnisotropy = 1;
        staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        staticSamplers[0].MinLOD = 0;
        staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplers[0].ShaderRegister = 0;
        staticSamplers[0].RegisterSpace = 0;
        staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        // Point Clamp
        staticSamplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[1].MipLODBias = 0.0f;
        staticSamplers[1].MaxAnisotropy = 1;
        staticSamplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        staticSamplers[1].MinLOD = 0;
        staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplers[1].ShaderRegister = 1;
        staticSamplers[1].RegisterSpace = 0;
        staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        // Linear Wrap
        staticSamplers[2].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticSamplers[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[2].MipLODBias = 0.0f;
        staticSamplers[2].MaxAnisotropy = 1;
        staticSamplers[2].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        staticSamplers[2].MinLOD = 0;
        staticSamplers[2].MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplers[2].ShaderRegister = 2;
        staticSamplers[2].RegisterSpace = 0;
        staticSamplers[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        // Point Wrap
        staticSamplers[3].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        staticSamplers[3].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[3].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[3].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[3].MipLODBias = 0.0f;
        staticSamplers[3].MaxAnisotropy = 1;
        staticSamplers[3].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        staticSamplers[3].MinLOD = 0;
        staticSamplers[3].MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplers[3].ShaderRegister = 3;
        staticSamplers[3].RegisterSpace = 0;
        staticSamplers[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        // Shadow
        staticSamplers[4].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
        staticSamplers[4].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[4].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[4].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[4].MipLODBias = 0.0f;
        staticSamplers[4].MaxAnisotropy = 1;
        staticSamplers[4].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        staticSamplers[4].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        staticSamplers[4].MinLOD = 0;
        staticSamplers[4].MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplers[4].ShaderRegister = 4;
        staticSamplers[4].RegisterSpace = 0;
        staticSamplers[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        // Shadow PCF
        staticSamplers[5].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        staticSamplers[5].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[5].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[5].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[5].MipLODBias = 0.0f;
        staticSamplers[5].MaxAnisotropy = 1;
        staticSamplers[5].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        staticSamplers[5].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        staticSamplers[5].MinLOD = 0;
        staticSamplers[5].MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplers[5].ShaderRegister = 5;
        staticSamplers[5].RegisterSpace = 0;
        staticSamplers[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        // Init
        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.NumParameters = ARRAY_COUNT(rootParameters);
        rootSignatureDesc.pParameters = rootParameters;
        rootSignatureDesc.NumStaticSamplers = ARRAY_COUNT(staticSamplers);
        rootSignatureDesc.pStaticSamplers = staticSamplers;
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        // Serialize
        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        VALIDATE_DIRECTX_CALL(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));

        // Create
        VALIDATE_DIRECTX_CALL(_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&_rootSignature)));
    }

    // Upload buffer
    UploadBuffer = New<UploadBufferDX12>(this);

    if (TimestampQueryHeap.Init())
        return true;

    // Cached command signatures
    {
        DrawIndirectCommandSignature = New<CommandSignatureDX12>(this, 1);
        DrawIndirectCommandSignature->At(0).Draw();
        DrawIndirectCommandSignature->Finalize();
    }
    {
        DrawIndexedIndirectCommandSignature = New<CommandSignatureDX12>(this, 1);
        DrawIndexedIndirectCommandSignature->At(0).DrawIndexed();
        DrawIndexedIndirectCommandSignature->Finalize();
    }
    {
        DispatchIndirectCommandSignature = New<CommandSignatureDX12>(this, 1);
        DispatchIndirectCommandSignature->At(0).Dispatch();
        DispatchIndirectCommandSignature->Finalize();
    }

    _state = DeviceState::Ready;
    return GPUDeviceDX::Init();
}

void GPUDeviceDX12::DrawBegin()
{
    {
        PROFILE_CPU_NAMED("Wait For GPU");
        //_commandQueue->WaitForGPU();
        _commandQueue->WaitForFence(_mainContext->FrameFenceValues[1]);
    }

    // Base
    GPUDeviceDX::DrawBegin();

    updateRes2Dispose();
    UploadBuffer->BeginGeneration(Engine::FrameCount);
}

void GPUDeviceDX12::RenderEnd()
{
    // Base
    GPUDeviceDX::RenderEnd();

    // Resolve the timestamp queries
    TimestampQueryHeap.EndQueryBatchAndResolveQueryData(_mainContext);
}

GPUDeviceDX12::~GPUDeviceDX12()
{
    // Ensure to be disposed
    Dispose();
}

D3D12_CPU_DESCRIPTOR_HANDLE GPUDeviceDX12::NullSRV(D3D12_SRV_DIMENSION dimension) const
{
    return _nullSrv[dimension].CPU();
}

D3D12_CPU_DESCRIPTOR_HANDLE GPUDeviceDX12::NullUAV() const
{
    return _nullUav.CPU();
}

ID3D12GraphicsCommandList* GPUDeviceDX12::GetCommandList() const
{
    return _mainContext->GetCommandList();
}

ID3D12CommandQueue* GPUDeviceDX12::GetCommandQueueDX12() const
{
    return _commandQueue->GetCommandQueue();
}

void GPUDeviceDX12::Dispose()
{
    GPUDeviceLock lock(this);

    // Check if has been disposed already
    if (_state == DeviceState::Disposed)
        return;

    // Set current state
    _state = DeviceState::Disposing;

    // Wait for rendering end
    WaitForGPU();

    // Pre dispose
    preDispose();

    // Release all late dispose resources (if state is Disposing all are released)
    updateRes2Dispose();

    // Clear pipeline objects
    for (auto& srv : _nullSrv)
        srv.Release();
    _nullUav.Release();
    TimestampQueryHeap.Destroy();
    DX_SAFE_RELEASE_CHECK(_rootSignature, 0);
    Heap_CBV_SRV_UAV.ReleaseGPU();
    Heap_RTV.ReleaseGPU();
    Heap_DSV.ReleaseGPU();
    Heap_Sampler.ReleaseGPU();
    RingHeap_CBV_SRV_UAV.ReleaseGPU();
    RingHeap_Sampler.ReleaseGPU();
    SAFE_DELETE(UploadBuffer);
    SAFE_DELETE(DrawIndirectCommandSignature);
    SAFE_DELETE(_mainContext);
    SAFE_DELETE(_commandQueue);

    // Clear DirectX stuff
    SAFE_DELETE(_adapter);
    SAFE_RELEASE(_device);
    SAFE_RELEASE(_factoryDXGI);

    // Base
    GPUDeviceDX::Dispose();

    // Set current state
    _state = DeviceState::Disposed;
}

void GPUDeviceDX12::WaitForGPU()
{
    _commandQueue->WaitForGPU();
}

GPUTexture* GPUDeviceDX12::CreateTexture(const StringView& name)
{
    return New<GPUTextureDX12>(this, name);
}

GPUShader* GPUDeviceDX12::CreateShader(const StringView& name)
{
    return New<GPUShaderDX12>(this, name);
}

GPUPipelineState* GPUDeviceDX12::CreatePipelineState()
{
    return New<GPUPipelineStateDX12>(this);
}

GPUTimerQuery* GPUDeviceDX12::CreateTimerQuery()
{
    return New<GPUTimerQueryDX12>(this);
}

GPUBuffer* GPUDeviceDX12::CreateBuffer(const StringView& name)
{
    return New<GPUBufferDX12>(this, name);
}

GPUSampler* GPUDeviceDX12::CreateSampler()
{
    return New<GPUSamplerDX12>(this);
}

GPUSwapChain* GPUDeviceDX12::CreateSwapChain(Window* window)
{
    return New<GPUSwapChainDX12>(this, window);
}

GPUConstantBuffer* GPUDeviceDX12::CreateConstantBuffer(uint32 size, const StringView& name)
{
    return New<GPUConstantBufferDX12>(this, size, name);
}

void GPUDeviceDX12::AddResourceToLateRelease(IGraphicsUnknown* resource, uint32 safeFrameCount)
{
    if (resource == nullptr)
        return;

    ScopeLock lock(_res2DisposeLock);

    // Add to the list
    DisposeResourceEntry entry;
    entry.Resource = resource;
    entry.TargetFrame = Engine::FrameCount + safeFrameCount;
    _res2Dispose.Add(entry);
}

void GPUDeviceDX12::updateRes2Dispose()
{
    uint64 currentFrame = Engine::FrameCount;
    if (_state == DeviceState::Disposing)
    {
        // During device disposing we want to remove all resources
        currentFrame = MAX_uint32;
    }

    _res2DisposeLock.Lock();
    for (int32 i = _res2Dispose.Count() - 1; i >= 0 && i < _res2Dispose.Count(); i--)
    {
        const DisposeResourceEntry& entry = _res2Dispose[i];
        if (entry.TargetFrame <= currentFrame)
        {
            auto refs = entry.Resource->Release();
            if (refs != 0)
            {
                LOG(Error, "Late release resource has not been fully released. References left: {0}", refs);
            }
            _res2Dispose.RemoveAt(i);
        }
    }
    _res2DisposeLock.Unlock();
}

#if PLATFORM_XBOX_SCARLETT || PLATFORM_XBOX_ONE

void GPUDeviceDX12::OnSuspended()
{
    _commandQueue->GetCommandQueue()->SuspendX(0);
}

void GPUDeviceDX12::OnResumed()
{
    _commandQueue->GetCommandQueue()->ResumeX();

    updateFrameEvents();
}

void GPUDeviceDX12::updateFrameEvents()
{
    ComPtr<IDXGIDevice1> dxgiDevice;
    VALIDATE_DIRECTX_CALL(_device->QueryInterface(IID_GRAPHICS_PPV_ARGS(&dxgiDevice)));
    ComPtr<IDXGIAdapter> dxgiAdapter;
    VALIDATE_DIRECTX_CALL(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));
    dxgiAdapter->GetDesc(&_adapter->Description);
    ComPtr<IDXGIOutput> dxgiOutput;
    VALIDATE_DIRECTX_CALL(dxgiAdapter->EnumOutputs(0, dxgiOutput.GetAddressOf()));
    VALIDATE_DIRECTX_CALL(_device->SetFrameIntervalX(dxgiOutput.Get(), D3D12XBOX_FRAME_INTERVAL_60_HZ, DX12_BACK_BUFFER_COUNT - 1u, D3D12XBOX_FRAME_INTERVAL_FLAG_NONE));
    VALIDATE_DIRECTX_CALL(_device->ScheduleFrameEventX(D3D12XBOX_FRAME_EVENT_ORIGIN, 0U, nullptr, D3D12XBOX_SCHEDULE_FRAME_EVENT_FLAG_NONE));
}

#endif

GPUDevice* CreateGPUDeviceDX12()
{
    return GPUDeviceDX12::Create();
}

#endif
