// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX11

#include "GPUDeviceDX11.h"
#include "GPUShaderDX11.h"
#include "GPUContextDX11.h"
#include "GPUPipelineStateDX11.h"
#include "GPUTextureDX11.h"
#include "GPUTimerQueryDX11.h"
#include "GPUBufferDX11.h"
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

    return false;
}

GPUDevice* GPUDeviceDX11::Create()
{
    // Configuration
#if PLATFORM_XBOX_ONE
	D3D_FEATURE_LEVEL maxAllowedFeatureLevel = D3D_FEATURE_LEVEL_10_0;
#elif DX11_FORCE_USE_DX10
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
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
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
    Array<GPUAdapterDX> adapters;
    IDXGIAdapter* tmpAdapter;
    for (uint32 index = 0; dxgiFactory->EnumAdapters(index, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; index++)
    {
        GPUAdapterDX adapter;
        if (tmpAdapter && TryCreateDevice(tmpAdapter, maxAllowedFeatureLevel, &adapter.MaxFeatureLevel))
        {
            adapter.Index = index;
            VALIDATE_DIRECTX_RESULT(tmpAdapter->GetDesc(&adapter.Description));
            uint32 outputs = RenderToolsDX::CountAdapterOutputs(tmpAdapter);

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
    , _device(nullptr)
    , _imContext(nullptr)
    , _factoryDXGI(dxgiFactory)
    , _mainContext(nullptr)
    , _samplerLinearClamp(nullptr)
    , _samplerPointClamp(nullptr)
    , _samplerLinearWrap(nullptr)
    , _samplerPointWrap(nullptr)
    , _samplerShadow(nullptr)
    , _samplerShadowPCF(nullptr)
{
    Platform::MemoryClear(RasterizerStates, sizeof(RasterizerStates));
    Platform::MemoryClear(DepthStencilStates, sizeof(DepthStencilStates));
}

ID3D11BlendState* GPUDeviceDX11::GetBlendState(const BlendingMode& blending)
{
    // Use lookup
    ID3D11BlendState* blendState = nullptr;
    if (BlendStates.TryGet(blending, blendState))
        return blendState;

    // Make it safe
    ScopeLock lock(BlendStatesWriteLocker);

    // Try again to prevent race condition with double-adding the same thing
    if (BlendStates.TryGet(blending, blendState))
        return blendState;

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

    // Create object
    VALIDATE_DIRECTX_RESULT(_device->CreateBlendState(&desc, &blendState));

    // Cache blend state
    BlendStates.Add(blending, blendState);

    return blendState;
}

static MSAALevel GetMaximumMultisampleCount(ID3D11Device* device, DXGI_FORMAT dxgiFormat)
{
    int32 maxCount = 1;
    UINT numQualityLevels;
    for (int32 i = 2; i <= 8; i *= 2)
    {
        if (SUCCEEDED(device->CheckMultisampleQualityLevels(dxgiFormat, i, &numQualityLevels)) && numQualityLevels > 0)
            maxCount = i;
    }
    return static_cast<MSAALevel>(maxCount);
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
    VALIDATE_DIRECTX_RESULT(D3D11CreateDevice(
        adapter,
        D3D_DRIVER_TYPE_UNKNOWN,
        NULL,
        flags,
        &targetFeatureLevel,
        1,
        D3D11_SDK_VERSION,
        &_device,
        &createdFeatureLevel,
        &_imContext
    ));

    // Validate result
    ASSERT(_device);
    ASSERT(_imContext);
    ASSERT(createdFeatureLevel == targetFeatureLevel);
    _state = DeviceState::Created;

    // Verify compute shader is supported on DirectX 11
    if (createdFeatureLevel >= D3D_FEATURE_LEVEL_11_0)
    {
        D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS options = { 0 };
        _device->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &options, sizeof(options));
        if (!options.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x)
        {
            _device->Release();
            _device = nullptr;
            LOG(Fatal, "DirectCompute is not supported by this device (DirectX 11 level).");
            return true;
        }
    }

    // Init device limits
    {
        auto& limits = Limits;
        if (createdFeatureLevel >= D3D_FEATURE_LEVEL_11_0)
        {
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
            limits.MaximumMipLevelsCount = D3D11_REQ_MIP_LEVELS;
            limits.MaximumTexture1DSize = D3D11_REQ_TEXTURE1D_U_DIMENSION;
            limits.MaximumTexture1DArraySize = D3D11_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION;
            limits.MaximumTexture2DSize = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            limits.MaximumTexture2DArraySize = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
            limits.MaximumTexture3DSize = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
            limits.MaximumTextureCubeSize = D3D11_REQ_TEXTURECUBE_DIMENSION;
        }
        else
        {
            limits.HasCompute = false;
            limits.HasTessellation = false;
            limits.HasGeometryShaders = true;
            limits.HasInstancing = true;
            limits.HasVolumeTextureRendering = false;
            limits.HasDrawIndirect = false;
            limits.HasAppendConsumeBuffers = false;
            limits.HasSeparateRenderTargetBlendState = false;
            limits.HasDepthAsSRV = false;
            limits.HasReadOnlyDepth = createdFeatureLevel == D3D_FEATURE_LEVEL_10_1;
            limits.HasMultisampleDepthAsSRV = false;
            limits.MaximumMipLevelsCount = D3D10_REQ_MIP_LEVELS;
            limits.MaximumTexture1DSize = D3D10_REQ_TEXTURE1D_U_DIMENSION;
            limits.MaximumTexture1DArraySize = D3D10_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION;
            limits.MaximumTexture2DSize = D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            limits.MaximumTexture2DArraySize = D3D10_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
            limits.MaximumTexture3DSize = D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
            limits.MaximumTextureCubeSize = D3D11_REQ_TEXTURECUBE_DIMENSION;
        }

        for (int32 i = 0; i < static_cast<int32>(PixelFormat::MAX); i++)
        {
            auto format = static_cast<PixelFormat>(i);
            auto dxgiFormat = RenderToolsDX::ToDxgiFormat(format);
            auto maximumMultisampleCount = GetMaximumMultisampleCount(_device, dxgiFormat);
            UINT formatSupport = 0;
            _device->CheckFormatSupport(dxgiFormat, &formatSupport);
            FeaturesPerFormat[i] = FormatFeatures(format, maximumMultisampleCount, (FormatSupport)formatSupport);
        }
    }

    // Init debug layer
#if GPU_ENABLE_DIAGNOSTICS
    ComPtr<ID3D11InfoQueue> infoQueue;
    VALIDATE_DIRECTX_RESULT(_device->QueryInterface(IID_PPV_ARGS(&infoQueue)));
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

        // Linear Clamp
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        result = _device->CreateSamplerState(&samplerDesc, &_samplerLinearClamp);
        LOG_DIRECTX_RESULT_WITH_RETURN(result);

        // Point Clamp
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        result = _device->CreateSamplerState(&samplerDesc, &_samplerPointClamp);
        LOG_DIRECTX_RESULT_WITH_RETURN(result);

        // Linear Wrap
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        result = _device->CreateSamplerState(&samplerDesc, &_samplerLinearWrap);
        LOG_DIRECTX_RESULT_WITH_RETURN(result);

        // Point Wrap
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        result = _device->CreateSamplerState(&samplerDesc, &_samplerPointWrap);
        LOG_DIRECTX_RESULT_WITH_RETURN(result);

        // Shadow
        samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        result = _device->CreateSamplerState(&samplerDesc, &_samplerShadow);
        LOG_DIRECTX_RESULT_WITH_RETURN(result);

        // Shadow PCF
        samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
        samplerDesc.BorderColor[0] = samplerDesc.BorderColor[1] = samplerDesc.BorderColor[2] = samplerDesc.BorderColor[3] = 0;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        result = _device->CreateSamplerState(&samplerDesc, &_samplerShadowPCF);
        LOG_DIRECTX_RESULT_WITH_RETURN(result);
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
			LOG_DIRECTX_RESULT_WITH_RETURN(result)
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

    // Depth Stencil States
    {
        D3D11_DEPTH_STENCIL_DESC dsDesc;
        dsDesc.StencilEnable = FALSE;
        dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
        dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
        const D3D11_DEPTH_STENCILOP_DESC defaultStencilOp = { D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS };
        dsDesc.FrontFace = defaultStencilOp;
        dsDesc.BackFace = defaultStencilOp;
        int32 index;
#define CREATE_DEPTH_STENCIL_STATE(depthTextEnable, depthWrite) \
			dsDesc.DepthEnable = depthTextEnable; \
			dsDesc.DepthWriteMask = depthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO; \
			for(int32 depthFunc = 1; depthFunc <= 8; depthFunc++) { \
			dsDesc.DepthFunc = (D3D11_COMPARISON_FUNC)depthFunc; \
			index = (int32)depthFunc + (depthTextEnable ? 0 : 9) + (depthWrite ? 0 : 18); \
			HRESULT result = _device->CreateDepthStencilState(&dsDesc, &DepthStencilStates[index]); \
			LOG_DIRECTX_RESULT_WITH_RETURN(result); }
        CREATE_DEPTH_STENCIL_STATE(false, false);
        CREATE_DEPTH_STENCIL_STATE(false, true);
        CREATE_DEPTH_STENCIL_STATE(true, true);
        CREATE_DEPTH_STENCIL_STATE(true, false);
#undef CREATE_DEPTH_STENCIL_STATE
    }

    _state
            =
            DeviceState::Ready;
    return
            GPUDeviceDX::Init();
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
    SAFE_RELEASE(_samplerShadowPCF);
    //
    for (auto i = BlendStates.Begin(); i.IsNotEnd(); ++i)
    {
        i->Value->Release();
    }
    BlendStates.Clear();
    for (uint32 i = 0; i < ARRAY_COUNT(RasterizerStates); i++)
    {
        SAFE_RELEASE(RasterizerStates[i]);
    }
    for (uint32 i = 0; i < ARRAY_COUNT(DepthStencilStates); i++)
    {
        SAFE_RELEASE(DepthStencilStates[i]);
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
    VALIDATE_DIRECTX_RESULT(_device->QueryInterface(IID_PPV_ARGS(&infoQueue)));
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

GPUSwapChain* GPUDeviceDX11::CreateSwapChain(Window* window)
{
    return New<GPUSwapChainDX11>(this, window);
}

#endif
