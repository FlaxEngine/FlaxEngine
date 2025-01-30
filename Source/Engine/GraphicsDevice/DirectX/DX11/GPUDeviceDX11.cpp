// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX11

#include "GPUDeviceDX11.h"
#include "GPUShaderDX11.h"
#include "GPUContextDX11.h"
#include "GPUPipelineStateDX11.h"
#include "GPUTextureDX11.h"
#include "GPUTimerQueryDX11.h"
#include "GPUBufferDX11.h"
#include "GPUSamplerDX11.h"
#include "GPUSwapChainDX11.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Threading/Threading.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"
#include "Engine/Engine/CommandLine.h"

#if !USE_EDITOR && PLATFORM_WINDOWS
#include "Engine/Core/Config/PlatformSettings.h"
#endif

#define DX11_FORCE_USE_DX10 0
#define DX11_FORCE_USE_DX10_1 0

static D3D11_COMPARISON_FUNC ToDX11(ComparisonFunc value)
{
    switch (value)
    {
    case ComparisonFunc::Never:
        return D3D11_COMPARISON_NEVER;
    case ComparisonFunc::Less:
        return D3D11_COMPARISON_LESS;
    case ComparisonFunc::Equal:
        return D3D11_COMPARISON_EQUAL;
    case ComparisonFunc::LessEqual:
        return D3D11_COMPARISON_LESS_EQUAL;
    case ComparisonFunc::Greater:
        return D3D11_COMPARISON_GREATER;
    case ComparisonFunc::NotEqual:
        return D3D11_COMPARISON_NOT_EQUAL;
    case ComparisonFunc::GreaterEqual:
        return D3D11_COMPARISON_GREATER_EQUAL;
    case ComparisonFunc::Always:
        return D3D11_COMPARISON_ALWAYS;
    default:
        return (D3D11_COMPARISON_FUNC)-1;
    }
}

static D3D11_STENCIL_OP ToDX11(StencilOperation value)
{
    switch (value)
    {
    case StencilOperation::Keep:
        return D3D11_STENCIL_OP_KEEP;
    case StencilOperation::Zero:
        return D3D11_STENCIL_OP_ZERO;
    case StencilOperation::Replace:
        return D3D11_STENCIL_OP_REPLACE;
    case StencilOperation::IncrementSaturated:
        return D3D11_STENCIL_OP_INCR_SAT;
    case StencilOperation::DecrementSaturated:
        return D3D11_STENCIL_OP_DECR_SAT;
    case StencilOperation::Invert:
        return D3D11_STENCIL_OP_INVERT;
    case StencilOperation::Increment:
        return D3D11_STENCIL_OP_INCR;
    case StencilOperation::Decrement:
        return D3D11_STENCIL_OP_DECR;
    default:
        return (D3D11_STENCIL_OP)-1;
    }
}

static bool TryCreateDevice(IDXGIAdapter* adapter, D3D_FEATURE_LEVEL maxFeatureLevel, D3D_FEATURE_LEVEL* featureLevel)
{
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    uint32 deviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if GPU_ENABLE_DIAGNOSTICS
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // Pick the first level
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    int32 levelIndex = 0;
    while (levelIndex < ARRAY_COUNT(featureLevels))
    {
        if (featureLevels[levelIndex] == maxFeatureLevel)
            break;
        levelIndex++;
    }
    if (levelIndex >= ARRAY_COUNT(featureLevels))
    {
        return false;
    }

    // Try to create device
    if (SUCCEEDED(D3D11CreateDevice(
        adapter,
        D3D_DRIVER_TYPE_UNKNOWN,
        NULL,
        deviceFlags,
        &featureLevels[levelIndex],
        ARRAY_COUNT(featureLevels) - levelIndex,
        D3D11_SDK_VERSION,
        &device,
        featureLevel,
        &context
    )))
    {
        device->Release();
        context->Release();
        return true;
    }
#if GPU_ENABLE_DIAGNOSTICS
    deviceFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
    if (SUCCEEDED(D3D11CreateDevice(
        adapter,
        D3D_DRIVER_TYPE_UNKNOWN,
        NULL,
        deviceFlags,
        &featureLevels[levelIndex],
        ARRAY_COUNT(featureLevels) - levelIndex,
        D3D11_SDK_VERSION,
        &device,
        featureLevel,
        &context
    )))
    {
        LOG(Warning, "Direct3D SDK debug layers were requested, but not available.");
        device->Release();
        context->Release();
        return true;
    }
#endif

    return false;
}

GPUDevice* GPUDeviceDX11::Create()
{
    // Configuration
#if DX11_FORCE_USE_DX10
	D3D_FEATURE_LEVEL maxAllowedFeatureLevel = D3D_FEATURE_LEVEL_10_0;
#elif DX11_FORCE_USE_DX10_1
	D3D_FEATURE_LEVEL maxAllowedFeatureLevel = D3D_FEATURE_LEVEL_10_1;
#else
    D3D_FEATURE_LEVEL maxAllowedFeatureLevel = D3D_FEATURE_LEVEL_11_0;
#endif
    if (CommandLine::Options.D3D10)
        maxAllowedFeatureLevel = D3D_FEATURE_LEVEL_10_0;
    else if (CommandLine::Options.D3D11)
        maxAllowedFeatureLevel = D3D_FEATURE_LEVEL_11_0;
#if !USE_EDITOR && PLATFORM_WINDOWS
	auto winSettings = WindowsPlatformSettings::Get();
	if (!winSettings->SupportDX11 && !winSettings->SupportDX10)
	{
		// Skip if there is no support
		LOG(Warning, "Cannot use DirectX (support disabled).");
		return nullptr;
	}
	if (!winSettings->SupportDX11 && maxAllowedFeatureLevel == D3D_FEATURE_LEVEL_11_0)
	{
		// Downgrade if there is no SM5 support
		maxAllowedFeatureLevel = D3D_FEATURE_LEVEL_10_0;
		LOG(Warning, "Cannot use DirectX 11 (support disabled).");
	}
	if (!winSettings->SupportDX10 && maxAllowedFeatureLevel == D3D_FEATURE_LEVEL_10_0)
	{
		// Upgrade if there is no SM4 support
		maxAllowedFeatureLevel = D3D_FEATURE_LEVEL_11_0;
		LOG(Warning, "Cannot use DirectX 10 (support disabled).");
	}
#endif

    // Create DXGI factory
#if PLATFORM_WINDOWS
    IDXGIFactory1* dxgiFactory;
    IDXGIFactory6* dxgiFactory6;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory6));
    if (hr == S_OK)
        dxgiFactory = dxgiFactory6;
    else
    {
        dxgiFactory6 = nullptr;
        hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
    }
#else
    IDXGIFactory2* dxgiFactory;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
#endif
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
        GPUAdapterDX adapter;
        if (tempAdapter && TryCreateDevice(tempAdapter, maxAllowedFeatureLevel, &adapter.MaxFeatureLevel))
        {
            adapter.Index = index;
            VALIDATE_DIRECTX_CALL(tempAdapter->GetDesc(&adapter.Description));
            uint32 outputs = RenderToolsDX::CountAdapterOutputs(tempAdapter);

            LOG(Info, "Adapter {1}: '{0}', DirectX {2}", adapter.Description.Description, index, RenderToolsDX::GetFeatureLevelString(adapter.MaxFeatureLevel));
            LOG(Info, "	Dedicated Video Memory: {0}, Dedicated System Memory: {1}, Shared System Memory: {2}, Output(s): {3}", Utilities::BytesToText(adapter.Description.DedicatedVideoMemory), Utilities::BytesToText(adapter.Description.DedicatedSystemMemory), Utilities::BytesToText(adapter.Description.SharedSystemMemory), outputs);

            adapters.Add(adapter);
        }
    }
#if PLATFORM_WINDOWS
    // Find the best performing adapter and prefer using it instead of the first device
    const auto gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
    if (dxgiFactory6 != nullptr && selectedAdapterIndex == -1)
    {
        if (dxgiFactory6->EnumAdapterByGpuPreference(0, gpuPreference, IID_PPV_ARGS(&tempAdapter)) != DXGI_ERROR_NOT_FOUND)
        {
            GPUAdapterDX adapter;
            if (tempAdapter && TryCreateDevice(tempAdapter, maxAllowedFeatureLevel, &adapter.MaxFeatureLevel))
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
#endif

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

    // Create device
    auto device = New<GPUDeviceDX11>(dxgiFactory, New<GPUAdapterDX>(selectedAdapter));
    if (device->Init())
    {
        LOG(Warning, "Graphics Device init failed");
        Delete(device);
        return nullptr;
    }

    return device;
}

GPUDeviceDX11::GPUDeviceDX11(IDXGIFactory* dxgiFactory, GPUAdapterDX* adapter)
    : GPUDeviceDX(getRendererType(adapter), getShaderProfile(adapter), adapter)
    , _factoryDXGI(dxgiFactory)
{
    Platform::MemoryClear(RasterizerStates, sizeof(RasterizerStates));
}

ID3D11DepthStencilState* GPUDeviceDX11::GetDepthStencilState(const void* descriptionPtr)
{
    const GPUPipelineState::Description& description = *(const GPUPipelineState::Description*)descriptionPtr;
    DepthStencilMode key;
    Platform::MemoryClear(&key, sizeof(key)); // Ensure to clear any padding bytes for raw memory compare/hashing
    key.DepthEnable = description.DepthEnable ? 1 : 0;
    key.DepthWriteEnable = description.DepthWriteEnable ? 1 : 0;
    key.DepthClipEnable = description.DepthClipEnable ? 1 : 0;
    key.StencilEnable = description.StencilEnable ? 1 : 0;
    key.StencilReadMask = description.StencilReadMask;
    key.StencilWriteMask = description.StencilWriteMask;
    key.DepthFunc = description.DepthFunc;
    key.StencilFunc = description.StencilFunc;
    key.StencilFailOp = description.StencilFailOp;
    key.StencilDepthFailOp = description.StencilDepthFailOp;
    key.StencilPassOp = description.StencilPassOp;

    // Use lookup
    ID3D11DepthStencilState* state = nullptr;
    if (DepthStencilStates.TryGet(key, state))
        return state;
    
    // Try again but with lock to prevent race condition with double-adding the same thing
    ScopeLock lock(StatesWriteLocker);
    if (DepthStencilStates.TryGet(key, state))
        return state;

    // Prepare description
    D3D11_DEPTH_STENCIL_DESC desc;
    desc.DepthEnable = !!description.DepthEnable;
    desc.DepthWriteMask = description.DepthWriteEnable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = ToDX11(description.DepthFunc);
    desc.StencilEnable = !!description.StencilEnable;
    desc.StencilReadMask = description.StencilReadMask;
    desc.StencilWriteMask = description.StencilWriteMask;
    desc.FrontFace.StencilFailOp = ToDX11(description.StencilFailOp);
    desc.FrontFace.StencilDepthFailOp = ToDX11(description.StencilDepthFailOp);
    desc.FrontFace.StencilPassOp = ToDX11(description.StencilPassOp);
    desc.FrontFace.StencilFunc = ToDX11(description.StencilFunc);
    desc.BackFace = desc.FrontFace;

    // Create object and cache it
    VALIDATE_DIRECTX_CALL(_device->CreateDepthStencilState(&desc, &state));
    DepthStencilStates.Add(key, state);
    return state;
}

ID3D11BlendState* GPUDeviceDX11::GetBlendState(const BlendingMode& blending)
{
    // Use lookup
    ID3D11BlendState* state = nullptr;
    if (BlendStates.TryGet(blending, state))
        return state;
    
    // Try again but with lock to prevent race condition with double-adding the same thing
    ScopeLock lock(StatesWriteLocker);
    if (BlendStates.TryGet(blending, state))
        return state;

    // Prepare description
    D3D11_BLEND_DESC desc;
    desc.AlphaToCoverageEnable = blending.AlphaToCoverageEnable ? TRUE : FALSE;
    desc.IndependentBlendEnable = FALSE;
    desc.RenderTarget[0].BlendEnable = blending.BlendEnable ? TRUE : FALSE;
    desc.RenderTarget[0].SrcBlend = (D3D11_BLEND)blending.SrcBlend;
    desc.RenderTarget[0].DestBlend = (D3D11_BLEND)blending.DestBlend;
    desc.RenderTarget[0].BlendOp = (D3D11_BLEND_OP)blending.BlendOp;
    desc.RenderTarget[0].SrcBlendAlpha = (D3D11_BLEND)blending.SrcBlendAlpha;
    desc.RenderTarget[0].DestBlendAlpha = (D3D11_BLEND)blending.DestBlendAlpha;
    desc.RenderTarget[0].BlendOpAlpha = (D3D11_BLEND_OP)blending.BlendOpAlpha;
    desc.RenderTarget[0].RenderTargetWriteMask = (UINT8)blending.RenderTargetWriteMask;
#if BUILD_DEBUG
    for (byte i = 1; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
        desc.RenderTarget[i] = desc.RenderTarget[0];
#endif

    // Create object and cache it
    VALIDATE_DIRECTX_CALL(_device->CreateBlendState(&desc, &state));
    BlendStates.Add(blending, state);
    return state;
}

bool GPUDeviceDX11::Init()
{
    HRESULT result;

    // Get DXGI adapter
    ComPtr<IDXGIAdapter> adapter;
    if (_factoryDXGI->EnumAdapters(_adapter->Index, &adapter) == DXGI_ERROR_NOT_FOUND || adapter == nullptr)
    {
        LOG(Warning, "Cannot get the adapter.");
        return true;
    }
    UpdateOutputs(adapter);

    // Get flags and device type base on current configuration
    uint32 flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if GPU_ENABLE_DIAGNOSTICS
    flags |= D3D11_CREATE_DEVICE_DEBUG;
    LOG(Info, "DirectX debugging layer enabled");
#endif

    // Create DirectX device
    D3D_FEATURE_LEVEL createdFeatureLevel = static_cast<D3D_FEATURE_LEVEL>(0);
    auto targetFeatureLevel = GetD3DFeatureLevel();
    VALIDATE_DIRECTX_CALL(D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, flags, &targetFeatureLevel, 1, D3D11_SDK_VERSION, &_device, &createdFeatureLevel, &_imContext));
    ASSERT(_device);
    ASSERT(_imContext);
    ASSERT(createdFeatureLevel == targetFeatureLevel);
    _state = DeviceState::Created;

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
        {
            _allowTearing = true;
        }
    }

    // Init device limits
    {
        auto& limits = Limits;
        if (createdFeatureLevel >= D3D_FEATURE_LEVEL_11_0)
        {
            D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS d3D10XHardwareOptions = {};
            _device->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &d3D10XHardwareOptions, sizeof(d3D10XHardwareOptions));
            D3D11_FEATURE_DATA_D3D11_OPTIONS2 featureDataD3D11Options2 = {};
            _device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &featureDataD3D11Options2, sizeof(featureDataD3D11Options2));
            limits.HasCompute = d3D10XHardwareOptions.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x != 0;
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
            limits.HasTypedUAVLoad = featureDataD3D11Options2.TypedUAVLoadAdditionalFormats != 0;
            limits.MaximumMipLevelsCount = D3D11_REQ_MIP_LEVELS;
            limits.MaximumTexture1DSize = D3D11_REQ_TEXTURE1D_U_DIMENSION;
            limits.MaximumTexture1DArraySize = D3D11_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION;
            limits.MaximumTexture2DSize = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            limits.MaximumTexture2DArraySize = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
            limits.MaximumTexture3DSize = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
            limits.MaximumTextureCubeSize = D3D11_REQ_TEXTURECUBE_DIMENSION;
            limits.MaximumSamplerAnisotropy = D3D11_DEFAULT_MAX_ANISOTROPY;
        }
        else
        {
            limits.HasCompute = false;
            limits.HasTessellation = false;
            limits.HasGeometryShaders = GPU_ALLOW_GEOMETRY_SHADERS;
            limits.HasInstancing = true;
            limits.HasVolumeTextureRendering = false;
            limits.HasDrawIndirect = false;
            limits.HasAppendConsumeBuffers = false;
            limits.HasSeparateRenderTargetBlendState = false;
            limits.HasDepthAsSRV = false;
            limits.HasDepthClip = true;
            limits.HasReadOnlyDepth = createdFeatureLevel == D3D_FEATURE_LEVEL_10_1;
            limits.HasMultisampleDepthAsSRV = false;
            limits.HasTypedUAVLoad = false;
            limits.MaximumMipLevelsCount = D3D10_REQ_MIP_LEVELS;
            limits.MaximumTexture1DSize = D3D10_REQ_TEXTURE1D_U_DIMENSION;
            limits.MaximumTexture1DArraySize = D3D10_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION;
            limits.MaximumTexture2DSize = D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            limits.MaximumTexture2DArraySize = D3D10_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
            limits.MaximumTexture3DSize = D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
            limits.MaximumTextureCubeSize = D3D10_REQ_TEXTURECUBE_DIMENSION;
            limits.MaximumSamplerAnisotropy = D3D10_DEFAULT_MAX_ANISOTROPY;
        }

        for (int32 i = 0; i < static_cast<int32>(PixelFormat::MAX); i++)
        {
            auto format = static_cast<PixelFormat>(i);
            auto dxgiFormat = RenderToolsDX::ToDxgiFormat(format);
            int32 maxCount = 1;
            UINT numQualityLevels;
            for (int32 c = 2; c <= 8; c *= 2)
            {
                if (SUCCEEDED(_device->CheckMultisampleQualityLevels(dxgiFormat, c, &numQualityLevels)) && numQualityLevels > 0)
                    maxCount = c;
            }
            UINT formatSupport = 0;
            _device->CheckFormatSupport(dxgiFormat, &formatSupport);
            FeaturesPerFormat[i] = FormatFeatures(format, static_cast<MSAALevel>(maxCount), (FormatSupport)formatSupport);
        }
    }

    // Init debug layer
#if GPU_ENABLE_DIAGNOSTICS
    ComPtr<ID3D11InfoQueue> infoQueue;
    VALIDATE_DIRECTX_CALL(_device->QueryInterface(IID_PPV_ARGS(&infoQueue)));
    if (infoQueue)
    {
        D3D11_INFO_QUEUE_FILTER filter;
        Platform::MemoryClear(&filter, sizeof(filter));

        D3D11_MESSAGE_SEVERITY denySeverity = D3D11_MESSAGE_SEVERITY_INFO;
        filter.DenyList.NumSeverities = 1;
        filter.DenyList.pSeverityList = &denySeverity;

        D3D11_MESSAGE_ID disabledMessages[] =
        {
            D3D11_MESSAGE_ID_OMSETRENDERTARGETS_INVALIDVIEW,
            D3D11_MESSAGE_ID_QUERY_BEGIN_ABANDONING_PREVIOUS_RESULTS,
            D3D11_MESSAGE_ID_QUERY_END_ABANDONING_PREVIOUS_RESULTS,
            D3D11_MESSAGE_ID_CREATEINPUTLAYOUT_EMPTY_LAYOUT,
            D3D11_MESSAGE_ID_DEVICE_DRAW_INDEX_BUFFER_TOO_SMALL,
            D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET,
            D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
        };

        filter.DenyList.NumIDs = ARRAY_COUNT(disabledMessages);
        filter.DenyList.pIDList = disabledMessages;

        infoQueue->PushStorageFilter(&filter);

        infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
        //infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, TRUE);
    }
#endif

    // Create main context from Immediate Context
    _mainContext = New<GPUContextDX11>(this, _imContext);

    // Static Samplers
    {
        D3D11_SAMPLER_DESC samplerDesc;
        Platform::MemoryClear(&samplerDesc, sizeof(samplerDesc));
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;

        // Linear Clamp
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        result = _device->CreateSamplerState(&samplerDesc, &_samplerLinearClamp);
        LOG_DIRECTX_RESULT_WITH_RETURN(result, true);

        // Point Clamp
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        result = _device->CreateSamplerState(&samplerDesc, &_samplerPointClamp);
        LOG_DIRECTX_RESULT_WITH_RETURN(result, true);

        // Linear Wrap
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        result = _device->CreateSamplerState(&samplerDesc, &_samplerLinearWrap);
        LOG_DIRECTX_RESULT_WITH_RETURN(result, true);

        // Point Wrap
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        result = _device->CreateSamplerState(&samplerDesc, &_samplerPointWrap);
        LOG_DIRECTX_RESULT_WITH_RETURN(result, true);

        // Shadow
        samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        result = _device->CreateSamplerState(&samplerDesc, &_samplerShadow);
        LOG_DIRECTX_RESULT_WITH_RETURN(result, true);

        // Shadow Linear
        samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        result = _device->CreateSamplerState(&samplerDesc, &_samplerShadowLinear);
        LOG_DIRECTX_RESULT_WITH_RETURN(result, true);
    }

    // Rasterizer States
    {
        D3D11_RASTERIZER_DESC rDesc;
        rDesc.ScissorEnable = TRUE;
        rDesc.MultisampleEnable = TRUE;
        rDesc.FrontCounterClockwise = FALSE;
        rDesc.DepthBias = 0;
        rDesc.DepthBiasClamp = 0.0f;
        rDesc.SlopeScaledDepthBias = 0.0f;
        int32 index;
#define CREATE_RASTERIZER_STATE(cullMode, dxCullMode, wireframe, depthClip) \
			index = (int32)cullMode + (wireframe ? 0 : 3) + (depthClip ? 0 : 6); \
			rDesc.CullMode = dxCullMode; \
			rDesc.FillMode = wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID; \
			rDesc.AntialiasedLineEnable = !!wireframe; \
			rDesc.DepthClipEnable = !!depthClip; \
			result = _device->CreateRasterizerState(&rDesc, &RasterizerStates[index]); \
			LOG_DIRECTX_RESULT_WITH_RETURN(result, true)
        CREATE_RASTERIZER_STATE(CullMode::Normal, D3D11_CULL_BACK, false, false);
        CREATE_RASTERIZER_STATE(CullMode::Inverted, D3D11_CULL_FRONT, false, false);
        CREATE_RASTERIZER_STATE(CullMode::TwoSided, D3D11_CULL_NONE, false, false);
        CREATE_RASTERIZER_STATE(CullMode::Normal, D3D11_CULL_BACK, true, false);
        CREATE_RASTERIZER_STATE(CullMode::Inverted, D3D11_CULL_FRONT, true, false);
        CREATE_RASTERIZER_STATE(CullMode::TwoSided, D3D11_CULL_NONE, true, false);
        //
        CREATE_RASTERIZER_STATE(CullMode::Normal, D3D11_CULL_BACK, false, true);
        CREATE_RASTERIZER_STATE(CullMode::Inverted, D3D11_CULL_FRONT, false, true);
        CREATE_RASTERIZER_STATE(CullMode::TwoSided, D3D11_CULL_NONE, false, true);
        CREATE_RASTERIZER_STATE(CullMode::Normal, D3D11_CULL_BACK, true, true);
        CREATE_RASTERIZER_STATE(CullMode::Inverted, D3D11_CULL_FRONT, true, true);
        CREATE_RASTERIZER_STATE(CullMode::TwoSided, D3D11_CULL_NONE, true, true);
#undef CREATE_RASTERIZER_STATE
    }

    _state = DeviceState::Ready;
    return GPUDeviceDX::Init();
}

GPUDeviceDX11::~GPUDeviceDX11()
{
    // Ensure to be disposed
    GPUDeviceDX11::Dispose();
}

GPUDevice* CreateGPUDeviceDX11()
{
    return GPUDeviceDX11::Create();
}

void GPUDeviceDX11::Dispose()
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

    // Clear device resources
    SAFE_RELEASE(_samplerLinearClamp);
    SAFE_RELEASE(_samplerPointClamp);
    SAFE_RELEASE(_samplerLinearWrap);
    SAFE_RELEASE(_samplerPointWrap);
    SAFE_RELEASE(_samplerShadow);
    SAFE_RELEASE(_samplerShadowLinear);
    for (auto i = BlendStates.Begin(); i.IsNotEnd(); ++i)
        i->Value->Release();
    for (auto i = DepthStencilStates.Begin(); i.IsNotEnd(); ++i)
        i->Value->Release();
    BlendStates.Clear();
    for (uint32 i = 0; i < ARRAY_COUNT(RasterizerStates); i++)
    {
        SAFE_RELEASE(RasterizerStates[i]);
    }

    // Clear DirectX stuff
    SAFE_DELETE(_mainContext);
    SAFE_DELETE(_adapter);
    SAFE_RELEASE(_imContext);
#if GPU_ENABLE_DIAGNOSTICS && 0
    ID3D11Debug* debugLayer = nullptr;
    _device->QueryInterface(IID_PPV_ARGS(&debugLayer));
    if (debugLayer)
    {
        // Report any DirectX object leaks
        debugLayer->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
        debugLayer->Release();
    }
#endif
    SAFE_RELEASE(_device);
    SAFE_RELEASE(_factoryDXGI);

    // Base
    GPUDeviceDX::Dispose();

    // Set current state
    _state = DeviceState::Disposed;
}

void GPUDeviceDX11::WaitForGPU()
{
    // In DirectX 11 driver manages CPU/GPU work synchronization and work submission
}

void GPUDeviceDX11::DrawEnd()
{
    GPUDeviceDX::DrawEnd();

#if GPU_ENABLE_DIAGNOSTICS
    // Flush debug messages queue
    ComPtr<ID3D11InfoQueue> infoQueue;
    VALIDATE_DIRECTX_CALL(_device->QueryInterface(IID_PPV_ARGS(&infoQueue)));
    if (infoQueue)
    {
        Array<uint8> data;
        const uint64 messagesCount = infoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();
        for (uint64 i = 0; i < messagesCount; i++)
        {
            SIZE_T length = 0;
            if (SUCCEEDED(infoQueue->GetMessage(i, nullptr, &length)))
            {
                data.Resize((int32)length);
                auto messageData = (D3D11_MESSAGE*)data.Get();
                if (SUCCEEDED(infoQueue->GetMessage(i, messageData, &length)))
                {
                    LogType logType;
                    switch (messageData->Severity)
                    {
                    case D3D11_MESSAGE_SEVERITY_CORRUPTION:
                        logType = LogType::Fatal;
                        break;
                    case D3D11_MESSAGE_SEVERITY_ERROR:
                        logType = LogType::Error;
                        break;
                    case D3D11_MESSAGE_SEVERITY_WARNING:
                        logType = LogType::Warning;
                        break;
                    case D3D11_MESSAGE_SEVERITY_INFO:
                    case D3D11_MESSAGE_SEVERITY_MESSAGE:
                    default:
                        logType = LogType::Info;
                        break;
                    }
                    Log::Logger::Write(logType, String(messageData->pDescription));
                }
            }
        }
        infoQueue->ClearStoredMessages();
    }
#endif
}

GPUTexture* GPUDeviceDX11::CreateTexture(const StringView& name)
{
    return New<GPUTextureDX11>(this, name);
}

GPUShader* GPUDeviceDX11::CreateShader(const StringView& name)
{
    return New<GPUShaderDX11>(this, name);
}

GPUPipelineState* GPUDeviceDX11::CreatePipelineState()
{
    return New<GPUPipelineStateDX11>(this);
}

GPUTimerQuery* GPUDeviceDX11::CreateTimerQuery()
{
    return New<GPUTimerQueryDX11>(this);
}

GPUBuffer* GPUDeviceDX11::CreateBuffer(const StringView& name)
{
    return New<GPUBufferDX11>(this, name);
}

GPUSampler* GPUDeviceDX11::CreateSampler()
{
    return New<GPUSamplerDX11>(this);
}

GPUSwapChain* GPUDeviceDX11::CreateSwapChain(Window* window)
{
    return New<GPUSwapChainDX11>(this, window);
}

GPUConstantBuffer* GPUDeviceDX11::CreateConstantBuffer(uint32 size, const StringView& name)
{
    ID3D11Buffer* buffer = nullptr;
    uint32 memorySize = 0;
    if (size)
    {
        // Create buffer
        D3D11_BUFFER_DESC cbDesc;
        cbDesc.ByteWidth = Math::AlignUp<uint32>(size, 16);
        cbDesc.Usage = D3D11_USAGE_DEFAULT;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = 0;
        cbDesc.MiscFlags = 0;
        cbDesc.StructureByteStride = 0;
        const HRESULT result = _device->CreateBuffer(&cbDesc, nullptr, &buffer);
        if (FAILED(result))
        {
            LOG_DIRECTX_RESULT(result);
            return nullptr;
        }
        memorySize = cbDesc.ByteWidth;
    }
    return New<GPUConstantBufferDX11>(this, size, memorySize, buffer, name);
}

#endif
