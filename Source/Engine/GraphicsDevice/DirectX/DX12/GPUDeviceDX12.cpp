// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "GPUDeviceDX12.h"
#include "GPUShaderDX12.h"
#include "GPUContextDX12.h"
#include "GPUPipelineStateDX12.h"
#include "GPUTextureDX12.h"
#include "GPUTimerQueryDX12.h"
#include "GPUBufferDX12.h"
#include "GPUSwapChainDX12.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Async/GPUTasksExecutor.h"
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
#if PLATFORM_XBOX_SCARLETT
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
#endif

    // Create DXGI factory (CreateDXGIFactory2 is supported on Windows 8.1 or newer)
    IDXGIFactory4* dxgiFactory;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
    if (hr != S_OK)
    {
        LOG(Error, "Cannot create DXGI adapter. Error code: {0:x}.", hr);
        return nullptr;
    }

    // Enumerate the DXGIFactory's adapters
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
            VALIDATE_DIRECTX_RESULT(tempAdapter->GetDesc(&adapter.Description));
            uint32 outputs = RenderToolsDX::CountAdapterOutputs(tempAdapter);

            // Send that info to the log
            LOG(Info, "Adapter {1}: '{0}', DirectX {2}", adapter.Description.Description, index, RenderToolsDX::GetFeatureLevelString(adapter.MaxFeatureLevel));
            LOG(Info, "	Dedicated Video Memory: {0}, Dedicated System Memory: {1}, Shared System Memory: {2}, Output(s): {3}", Utilities::BytesToText(adapter.Description.DedicatedVideoMemory), Utilities::BytesToText(adapter.Description.DedicatedSystemMemory), Utilities::BytesToText(adapter.Description.SharedSystemMemory), outputs);

            adapters.Add(adapter);
        }
    }

    // Select the adapter to use
    GPUAdapterDX selectedAdapter = adapters[0];
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
    , RingHeap_CBV_SRV_UAV(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 64 * 1024, true)
{
}

#if PLATFORM_XBOX_SCARLETT
namespace XboxScarlett
{
    extern Action OnSuspend;
    extern Action OnResume;
}
#endif

bool GPUDeviceDX12::Init()
{
#if PLATFORM_XBOX_SCARLETT
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
    params.DisableDXR = TRUE;
    VALIDATE_DIRECTX_RESULT(D3D12XboxCreateDevice(nullptr, &params, IID_GRAPHICS_PPV_ARGS(&_device)));

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
    XboxScarlett::OnSuspend.Bind<GPUDeviceDX12, &GPUDeviceDX12::OnSuspend>(this);
    XboxScarlett::OnResume.Bind<GPUDeviceDX12, &GPUDeviceDX12::OnResume>(this);
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
    {
        ComPtr<IDXGIFactory5> factory5;
        _factoryDXGI->QueryInterface(IID_PPV_ARGS(&factory5));
        if (factory5)
        {
            BOOL allowTearing;
            if (SUCCEEDED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))) && allowTearing)
            {
                AllowTearing = true;
            }
        }
    }

    // Create DirectX device
    VALIDATE_DIRECTX_RESULT(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device)));

    // Debug Layer
#if GPU_ENABLE_DIAGNOSTICS
    ComPtr<ID3D12InfoQueue> infoQueue;
    VALIDATE_DIRECTX_RESULT(_device->QueryInterface(IID_PPV_ARGS(&infoQueue)));
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
    VALIDATE_DIRECTX_RESULT(_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options)));
    LOG(Info, "Tiled Resources Tier: {0}", options.TiledResourcesTier);
    LOG(Info, "Resource Binding Tier: {0}", options.ResourceBindingTier);
    LOG(Info, "Conservative Rasterization Tier: {0}", options.ConservativeRasterizationTier);
    LOG(Info, "Resource Heap Tier: {0}", options.ResourceHeapTier);
    LOG(Info, "ROVs Supported: {0}", options.ROVsSupported != 0);

    // Init device limits
    {
        auto& limits = Limits;
        limits.HasCompute = true;
        limits.HasTessellation = true;
        limits.HasGeometryShaders = true;
        limits.HasInstancing = true;
        limits.HasVolumeTextureRendering = true;
        limits.HasDrawIndirect = true;
        limits.HasAppendConsumeBuffers = true;
        limits.HasSeparateRenderTargetBlendState = true;
        limits.HasDepthAsSRV = true;
        limits.HasReadOnlyDepth = true;
        limits.HasMultisampleDepthAsSRV = true;
        limits.MaximumMipLevelsCount = D3D12_REQ_MIP_LEVELS;
        limits.MaximumTexture1DSize = D3D12_REQ_TEXTURE1D_U_DIMENSION;
        limits.MaximumTexture1DArraySize = D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION;
        limits.MaximumTexture2DSize = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        limits.MaximumTexture2DArraySize = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
        limits.MaximumTexture3DSize = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
        limits.MaximumTextureCubeSize = D3D12_REQ_TEXTURECUBE_DIMENSION;

        for (int32 i = 0; i < static_cast<int32>(PixelFormat::MAX); i++)
        {
            const PixelFormat format = static_cast<PixelFormat>(i);
            const DXGI_FORMAT dxgiFormat = RenderToolsDX::ToDxgiFormat(format);

            D3D12_FEATURE_DATA_FORMAT_SUPPORT formatInfo = { dxgiFormat };
            if (FAILED(_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatInfo, sizeof(formatInfo))))
            {
                formatInfo.Support1 = D3D12_FORMAT_SUPPORT1_NONE;
            }
            const MSAALevel maximumMultisampleCount = GetMaximumMultisampleCount(_device, dxgiFormat);

            FeaturesPerFormat[i] = FormatFeatures(format, maximumMultisampleCount, (FormatSupport)formatInfo.Support1);
        }
    }

#if BUILD_DEBUG && false
	// Prevent the GPU from overclocking or under-clocking to get consistent timings
	_device->SetStablePowerState(TRUE);
#endif

    // Create commands queue
    _commandQueue = New<CommandQueueDX12>(this, D3D12_COMMAND_LIST_TYPE_DIRECT);
    if (_commandQueue->Init())
        return true;

    // Create rendering main context
    _mainContext = New<GPUContextDX12>(this, D3D12_COMMAND_LIST_TYPE_DIRECT);

    // Create descriptors heaps
    Heap_CBV_SRV_UAV.Init();
    Heap_RTV.Init();
    Heap_DSV.Init();
    if (RingHeap_CBV_SRV_UAV.Init())
        return true;

    // Create empty views
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        _nullSrv.CreateSRV(this, nullptr, &srvDesc);
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
        D3D12_DESCRIPTOR_RANGE r[2];
        // TODO: separate ranges for pixel/vertex visibility and one shared for all?
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
            range.NumDescriptors = GPU_MAX_UA_BINDED + 1; // the last (additional) UAV register is used as a UAV output (hidden internally)
            range.BaseShaderRegister = 0;
            range.RegisterSpace = 0;
            range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        }
        static_assert(GPU_MAX_UA_BINDED == 2, "DX12 backend uses hardcoded single UAV register slot. Update code to support more.");

        // Root parameters
        D3D12_ROOT_PARAMETER rootParameters[4];
        {
            D3D12_ROOT_PARAMETER& rootParam = rootParameters[0];
            rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            rootParam.Descriptor.ShaderRegister = 0;
            rootParam.Descriptor.RegisterSpace = 0;
        }
        {
            D3D12_ROOT_PARAMETER& rootParam = rootParameters[1];
            rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            rootParam.Descriptor.ShaderRegister = 1;
            rootParam.Descriptor.RegisterSpace = 0;
        }
        {
            D3D12_ROOT_PARAMETER& rootParam = rootParameters[2];
            rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            rootParam.DescriptorTable.NumDescriptorRanges = 1;
            rootParam.DescriptorTable.pDescriptorRanges = &r[0];
        }
        {
            D3D12_ROOT_PARAMETER& rootParam = rootParameters[3];
            rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            rootParam.DescriptorTable.NumDescriptorRanges = 1;
            rootParam.DescriptorTable.pDescriptorRanges = &r[1];
        }

        // TODO: describe visibilities for the static samples, maybe use all pixel? or again pixel + all combo?

        // Static samplers
        D3D12_STATIC_SAMPLER_DESC staticSamplers[6];
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

        // TODO: static samplers for the shadow pass change into bindable samplers or sth?

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
        VALIDATE_DIRECTX_RESULT(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));

        // Create
        VALIDATE_DIRECTX_RESULT(_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&_rootSignature)));
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
    // Wait for the GPU to have at least one backbuffer to render to
    /*{
        PROFILE_CPU_NAMED("Wait For Fence");
        const uint64 nextFenceValue = _commandQueue->GetNextFenceValue();
        if (nextFenceValue >= DX12_BACK_BUFFER_COUNT)
        {
            _commandQueue->WaitForFence(nextFenceValue - DX12_BACK_BUFFER_COUNT);
        }
    }*/

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

D3D12_CPU_DESCRIPTOR_HANDLE GPUDeviceDX12::NullSRV() const
{
    return _nullSrv.CPU();
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
    _nullSrv.Release();
    _nullUav.Release();
    TimestampQueryHeap.Destroy();
    DX_SAFE_RELEASE_CHECK(_rootSignature, 0);
    Heap_CBV_SRV_UAV.ReleaseGPU();
    Heap_RTV.ReleaseGPU();
    Heap_DSV.ReleaseGPU();
    RingHeap_CBV_SRV_UAV.ReleaseGPU();
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

GPUSwapChain* GPUDeviceDX12::CreateSwapChain(Window* window)
{
    return New<GPUSwapChainDX12>(this, window);
}

void GPUDeviceDX12::AddResourceToLateRelease(IGraphicsUnknown* resource, uint32 safeFrameCount)
{
    ASSERT(safeFrameCount < 32);
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

#if PLATFORM_XBOX_SCARLETT

void GPUDeviceDX12::OnSuspend()
{
    _commandQueue->GetCommandQueue()->SuspendX(0);
}

void GPUDeviceDX12::OnResume()
{
    _commandQueue->GetCommandQueue()->ResumeX();

    updateFrameEvents();
}

void GPUDeviceDX12::updateFrameEvents()
{
    ComPtr<IDXGIDevice1> dxgiDevice;
    VALIDATE_DIRECTX_RESULT(_device->QueryInterface(IID_GRAPHICS_PPV_ARGS(&dxgiDevice)));
    ComPtr<IDXGIAdapter> dxgiAdapter;
    VALIDATE_DIRECTX_RESULT(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));
    dxgiAdapter->GetDesc(&_adapter->Description);
    ComPtr<IDXGIOutput> dxgiOutput;
    VALIDATE_DIRECTX_RESULT(dxgiAdapter->EnumOutputs(0, dxgiOutput.GetAddressOf()));
    VALIDATE_DIRECTX_RESULT(_device->SetFrameIntervalX(dxgiOutput.Get(), D3D12XBOX_FRAME_INTERVAL_60_HZ, DX12_BACK_BUFFER_COUNT - 1u, D3D12XBOX_FRAME_INTERVAL_FLAG_NONE));
    VALIDATE_DIRECTX_RESULT(_device->ScheduleFrameEventX(D3D12XBOX_FRAME_EVENT_ORIGIN, 0U, nullptr, D3D12XBOX_SCHEDULE_FRAME_EVENT_FLAG_NONE));
}

#endif

GPUDevice* CreateGPUDeviceDX12()
{
    return GPUDeviceDX12::Create();
}

#endif
