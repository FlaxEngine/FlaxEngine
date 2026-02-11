// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "GPUDeviceDX12.h"
#include "GPUShaderDX12.h"
#include "GPUContextDX12.h"
#include "GPUPipelineStateDX12.h"
#include "GPUTextureDX12.h"
#include "GPUTimerQueryDX12.h"
#include "GPUBufferDX12.h"
#include "GPUSamplerDX12.h"
#include "GPUVertexLayoutDX12.h"
#include "GPUSwapChainDX12.h"
#include "RootSignatureDX12.h"
#include "UploadBufferDX12.h"
#include "CommandQueueDX12.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerMemory.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Config/PlatformSettings.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Threading/Threading.h"
#include "CommandSignatureDX12.h"

static bool CheckDX12Support(IDXGIAdapter* adapter)
{
#if PLATFORM_XBOX_SCARLETT || PLATFORM_XBOX_ONE
    return true;
#else
    // Try to create device
    if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
    {
        return true;
    }
    return false;
#endif
}

GPUVertexLayoutDX12::GPUVertexLayoutDX12(GPUDeviceDX12* device, const Elements& elements, bool explicitOffsets)
    : GPUResourceDX12<GPUVertexLayout>(device, StringView::Empty)
    , InputElementsCount(elements.Count())
{
    SetElements(elements, explicitOffsets);
    for (int32 i = 0; i < elements.Count(); i++)
    {
        const VertexElement& src = GetElements().Get()[i];
        D3D12_INPUT_ELEMENT_DESC& dst = InputElements[i];
        dst.SemanticName = RenderToolsDX::GetVertexInputSemantic(src.Type, dst.SemanticIndex);
        dst.Format = RenderToolsDX::ToDxgiFormat(src.Format);
        dst.InputSlot = src.Slot;
        dst.AlignedByteOffset = src.Offset;
        dst.InputSlotClass = src.PerInstance ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        dst.InstanceDataStepRate = src.PerInstance ? 1 : 0;
    }
}

RootSignatureDX12::RootSignatureDX12()
{
    // Clear structures
    Platform::MemoryClear(this, sizeof(*this));

    // Descriptor tables
    {
        // SRVs
        D3D12_DESCRIPTOR_RANGE& range = _ranges[0];
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.NumDescriptors = GPU_MAX_SR_BINDED;
        range.BaseShaderRegister = 0;
        range.RegisterSpace = 0;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }
    {
        // UAVs
        D3D12_DESCRIPTOR_RANGE& range = _ranges[1];
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        range.NumDescriptors = GPU_MAX_UA_BINDED;
        range.BaseShaderRegister = 0;
        range.RegisterSpace = 0;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }
    {
        // Samplers
        D3D12_DESCRIPTOR_RANGE& range = _ranges[2];
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        range.NumDescriptors = GPU_MAX_SAMPLER_BINDED - GPU_STATIC_SAMPLERS_COUNT;
        range.BaseShaderRegister = GPU_STATIC_SAMPLERS_COUNT;
        range.RegisterSpace = 0;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }

    // Root parameters
    for (int32 i = 0; i < GPU_MAX_CB_BINDED; i++)
    {
        // CBs
        D3D12_ROOT_PARAMETER& param = _parameters[DX12_ROOT_SIGNATURE_CB + i];
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        param.Descriptor.ShaderRegister = i;
        param.Descriptor.RegisterSpace = 0;
    }
    {
        // SRVs
        D3D12_ROOT_PARAMETER& param = _parameters[DX12_ROOT_SIGNATURE_SR];
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        param.DescriptorTable.NumDescriptorRanges = 1;
        param.DescriptorTable.pDescriptorRanges = &_ranges[0];
    }
    {
        // UAVs
        D3D12_ROOT_PARAMETER& param = _parameters[DX12_ROOT_SIGNATURE_UA];
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        param.DescriptorTable.NumDescriptorRanges = 1;
        param.DescriptorTable.pDescriptorRanges = &_ranges[1];
    }
    {
        // Samplers
        D3D12_ROOT_PARAMETER& param = _parameters[DX12_ROOT_SIGNATURE_SAMPLER];
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        param.DescriptorTable.NumDescriptorRanges = 1;
        param.DescriptorTable.pDescriptorRanges = &_ranges[2];
    }

    // Static samplers
    static_assert(GPU_STATIC_SAMPLERS_COUNT == ARRAY_COUNT(_staticSamplers), "Update static samplers setup.");
    // Linear Clamp
    InitSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    // Point Clamp
    InitSampler(1, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    // Linear Wrap
    InitSampler(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    // Point Wrap
    InitSampler(3, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    // Shadow
    InitSampler(4, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_COMPARISON_FUNC_LESS_EQUAL);
    // Shadow PCF
    InitSampler(5, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_COMPARISON_FUNC_LESS_EQUAL);

    // Init
    _desc.NumParameters = ARRAY_COUNT(_parameters);
    _desc.pParameters = _parameters;
    _desc.NumStaticSamplers = ARRAY_COUNT(_staticSamplers);
    _desc.pStaticSamplers = _staticSamplers;
    _desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
}

void RootSignatureDX12::InitSampler(int32 i, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE address, D3D12_COMPARISON_FUNC comparisonFunc)
{
    auto& sampler = _staticSamplers[i];
    sampler.Filter = filter;
    sampler.AddressU = address;
    sampler.AddressV = address;
    sampler.AddressW = address;
    sampler.MipLODBias = 0.0f;
    sampler.MaxAnisotropy = 1;
    sampler.ComparisonFunc = comparisonFunc;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
    sampler.MinLOD = 0;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = i;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
}

ComPtr<ID3DBlob> RootSignatureDX12::Serialize() const
{
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    VALIDATE_DIRECTX_CALL(D3D12SerializeRootSignature(&_desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &error));
    if (error.Get())
    {
        LOG(Error, "D3D12SerializeRootSignature failed with error: {}", String((const char*)error->GetBufferPointer()));
    }
    return signature;
}

#if USE_EDITOR

const Char* GetRootSignatureShaderVisibility(D3D12_SHADER_VISIBILITY visibility)
{
    switch (visibility)
    {
    case D3D12_SHADER_VISIBILITY_VERTEX:
        return TEXT(", visibility=SHADER_VISIBILITY_VERTEX");
    case D3D12_SHADER_VISIBILITY_HULL:
        return TEXT(", visibility=SHADER_VISIBILITY_HULL");
    case D3D12_SHADER_VISIBILITY_DOMAIN:
        return TEXT(", visibility=SHADER_VISIBILITY_DOMAIN");
    case D3D12_SHADER_VISIBILITY_GEOMETRY:
        return TEXT(", visibility=SHADER_VISIBILITY_GEOMETRY");
    case D3D12_SHADER_VISIBILITY_PIXEL:
        return TEXT(", visibility=SHADER_VISIBILITY_PIXEL");
    case D3D12_SHADER_VISIBILITY_AMPLIFICATION:
        return TEXT(", visibility=SHADER_VISIBILITY_AMPLIFICATION");
    case D3D12_SHADER_VISIBILITY_MESH:
        return TEXT(", visibility=SHADER_VISIBILITY_MESH");
    case D3D12_SHADER_VISIBILITY_ALL:
    default:
        return TEXT(""); // Default
    }
}

const Char* GetRootSignatureSamplerFilter(D3D12_FILTER filter)
{
    switch (filter)
    {
    case D3D12_FILTER_MIN_MAG_MIP_POINT:
        return TEXT("FILTER_MIN_MAG_MIP_POINT");
    case D3D12_FILTER_MIN_MAG_MIP_LINEAR:
        return TEXT("FILTER_MIN_MAG_MIP_LINEAR");
    case D3D12_FILTER_ANISOTROPIC:
        return TEXT("FILTER_ANISOTROPIC");
    case D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT:
        return TEXT("FILTER_COMPARISON_MIN_MAG_MIP_POINT");
    case D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR:
        return TEXT("FILTER_COMPARISON_MIN_MAG_MIP_LINEAR");
    default:
        CRASH; // Not implemented
    }
}

const Char* GetRootSignatureSamplerAddress(D3D12_TEXTURE_ADDRESS_MODE address)
{
    switch (address)
    {
    case D3D12_TEXTURE_ADDRESS_MODE_WRAP:
        return TEXT("TEXTURE_ADDRESS_WRAP");
    case D3D12_TEXTURE_ADDRESS_MODE_MIRROR:
        return TEXT("TEXTURE_ADDRESS_MIRROR");
    case D3D12_TEXTURE_ADDRESS_MODE_CLAMP:
        return TEXT("TEXTURE_ADDRESS_CLAMP");
    case D3D12_TEXTURE_ADDRESS_MODE_BORDER:
        return TEXT("TEXTURE_ADDRESS_BORDER");
    case D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE:
        return TEXT("TEXTURE_ADDRESS_MIRROR_ONCE");
    default:
        return TEXT("");
    }
}

const Char* GetRootSignatureSamplerComparisonFunc(D3D12_COMPARISON_FUNC func)
{
    switch (func)
    {
    case D3D12_COMPARISON_FUNC_NEVER:
        return TEXT("COMPARISON_NEVER");
    case D3D12_COMPARISON_FUNC_LESS:
        return TEXT("COMPARISON_LESS");
    case D3D12_COMPARISON_FUNC_EQUAL:
        return TEXT("COMPARISON_EQUAL");
    case D3D12_COMPARISON_FUNC_LESS_EQUAL:
        return TEXT("COMPARISON_LESS_EQUAL");
    case D3D12_COMPARISON_FUNC_GREATER:
        return TEXT("COMPARISON_GREATER");
    case D3D12_COMPARISON_FUNC_NOT_EQUAL:
        return TEXT("COMPARISON_NOT_EQUAL");
    case D3D12_COMPARISON_FUNC_GREATER_EQUAL:
        return TEXT("COMPARISON_GREATER_EQUAL");
    case D3D12_COMPARISON_FUNC_ALWAYS:
    default:
        return TEXT("COMPARISON_ALWAYS");
    }
}

void RootSignatureDX12::ToString(StringBuilder& sb, bool singleLine) const
{
    // Flags
    sb.Append(TEXT("RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)"));

    // Parameters
    const Char newLine = singleLine ? ' ' : '\n';
    for (const D3D12_ROOT_PARAMETER& param : _parameters)
    {
        const Char* visibility = GetRootSignatureShaderVisibility(param.ShaderVisibility);
        switch (param.ParameterType)
        {
        case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
            sb.AppendFormat(TEXT(",{}DescriptorTable("), newLine);
            for (uint32 rangeIndex = 0; rangeIndex < param.DescriptorTable.NumDescriptorRanges; rangeIndex++)
            {
                if (rangeIndex)
                    sb.Append(TEXT(", "));
                const D3D12_DESCRIPTOR_RANGE& range = param.DescriptorTable.pDescriptorRanges[rangeIndex];
                switch (range.RangeType)
                {
                case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
                    sb.AppendFormat(TEXT("SRV(t{}"), range.BaseShaderRegister);
                    break;
                case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
                    sb.AppendFormat(TEXT("UAV(u{}"), range.BaseShaderRegister);
                    break;
                case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
                    sb.AppendFormat(TEXT("CBV(b{}"), range.BaseShaderRegister);
                    break;
                case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
                    sb.AppendFormat(TEXT("Sampler(s{}"), range.BaseShaderRegister);
                    break;
                }
                if (range.NumDescriptors != 1)
                {
                    if (range.NumDescriptors == UINT_MAX)
                        sb.Append(TEXT(", numDescriptors=unbounded"));
                    else
                        sb.AppendFormat(TEXT(", numDescriptors={}"), range.NumDescriptors);
                }
                if (range.OffsetInDescriptorsFromTableStart != D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
                    sb.AppendFormat(TEXT(", offset={}"), range.OffsetInDescriptorsFromTableStart);
                sb.Append(')');
            }
            sb.AppendFormat(TEXT("{})"), visibility);
            break;
        case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
            sb.AppendFormat(TEXT(",{}RootConstants(num32BitConstants={}, b{}{})"), newLine, param.Constants.Num32BitValues, param.Constants.ShaderRegister, visibility);
            break;
        case D3D12_ROOT_PARAMETER_TYPE_CBV:
            sb.AppendFormat(TEXT(",{}CBV(b{}{})"), newLine, param.Descriptor.ShaderRegister, visibility);
            break;
        case D3D12_ROOT_PARAMETER_TYPE_SRV:
            sb.AppendFormat(TEXT(",{}SRV(t{}{})"), newLine, param.Descriptor.ShaderRegister, visibility);
            break;
        case D3D12_ROOT_PARAMETER_TYPE_UAV:
            sb.AppendFormat(TEXT(",{}UAV(u{}{})"), newLine, param.Descriptor.ShaderRegister, visibility);
            break;
        }
    }

    // Static Samplers
    for (const D3D12_STATIC_SAMPLER_DESC& sampler : _staticSamplers)
    {
        const Char* visibility = GetRootSignatureShaderVisibility(sampler.ShaderVisibility);
        sb.AppendFormat(TEXT(",{}StaticSampler(s{}"), newLine, sampler.ShaderRegister);
        sb.AppendFormat(TEXT(", filter={}"), GetRootSignatureSamplerFilter(sampler.Filter));
        sb.AppendFormat(TEXT(", addressU={}"), GetRootSignatureSamplerAddress(sampler.AddressU));
        sb.AppendFormat(TEXT(", addressV={}"), GetRootSignatureSamplerAddress(sampler.AddressV));
        sb.AppendFormat(TEXT(", addressW={}"), GetRootSignatureSamplerAddress(sampler.AddressW));
        sb.AppendFormat(TEXT(", comparisonFunc={}"), GetRootSignatureSamplerComparisonFunc(sampler.ComparisonFunc));
        sb.AppendFormat(TEXT(", maxAnisotropy={}"), sampler.MaxAnisotropy);
        sb.Append(TEXT(", borderColor=STATIC_BORDER_COLOR_OPAQUE_BLACK"));
        sb.AppendFormat(TEXT("{})"), visibility);
    }
}

String RootSignatureDX12::ToString() const
{
    StringBuilder sb;
    ToString(sb);
    return sb.ToString();
}

StringAnsi RootSignatureDX12::ToStringAnsi() const
{
    StringBuilder sb;
    ToString(sb);
    return sb.ToStringAnsi();
}

#endif

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
    if (CommandLine::Options.NVIDIA.IsTrue())
        vendorId = GPU_VENDOR_ID_NVIDIA;
    else if (CommandLine::Options.AMD.IsTrue())
        vendorId = GPU_VENDOR_ID_AMD;
    else if (CommandLine::Options.Intel.IsTrue())
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
    if (!selectedAdapter.IsValid())
    {
        LOG(Error, "Failed to choose valid DirectX adapter!");
        return nullptr;
    }
    if (selectedAdapter.MaxFeatureLevel < D3D_FEATURE_LEVEL_12_0)
    {
        LOG(Error, "Failed to choose valid DirectX adapter!");
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
    , _factoryDXGI(dxgiFactory)
    , _res2Dispose(256)
    , _rootSignature(nullptr)
    , _commandQueue(nullptr)
    , _mainContext(nullptr)
    , UploadBuffer(this)
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

    // Setup display output
    auto& videoOutput = VideoOutputs.AddOne();
    videoOutput.Name = hwVer;
    ComPtr<IDXGIDevice1> dxgiDevice;
    VALIDATE_DIRECTX_CALL(_device->QueryInterface(IID_GRAPHICS_PPV_ARGS(&dxgiDevice)));
    ComPtr<IDXGIAdapter> dxgiAdapter;
    VALIDATE_DIRECTX_CALL(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));
    ComPtr<IDXGIOutput> dxgiOutput;
    VALIDATE_DIRECTX_CALL(dxgiAdapter->EnumOutputs(0, dxgiOutput.GetAddressOf()));
    DXGI_FORMAT backbufferFormat = RenderToolsDX::ToDxgiFormat(GPU_BACK_BUFFER_PIXEL_FORMAT);
    UINT modesCount = 0;
#ifdef _GAMING_XBOX_SCARLETT
    VALIDATE_DIRECTX_CALL(dxgiOutput->GetDisplayModeList(backbufferFormat, 0, &modesCount, NULL));
    Array<DXGIXBOX_MODE_DESC> modes;
    modes.Resize((int32)modesCount);
    VALIDATE_DIRECTX_CALL(dxgiOutput->GetDisplayModeListX(backbufferFormat, 0, &modesCount, modes.Get()));
    for (const DXGIXBOX_MODE_DESC& mode : modes)
    {
        if (mode.Width > videoOutput.Width)
        {
            videoOutput.Width = mode.Width;
            videoOutput.Height = mode.Height;
        }
        videoOutput.RefreshRate = Math::Max(videoOutput.RefreshRate, mode.RefreshRate.Numerator / (float)mode.RefreshRate.Denominator);
    }
    modes.Resize(0);
#else
    videoOutput.Width = 1920;
    videoOutput.Height = 1080;
    videoOutput.RefreshRate = 60;
#endif

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
    if (!IsDebugToolAttached && GetModuleHandleA("renderdoc.dll"))
        IsDebugToolAttached = true;
    if (!IsDebugToolAttached && (GetModuleHandleA("Nvda.Graphics.Interception.dll") || GetModuleHandleA("WarpViz.Injection.dll") || GetModuleHandleA("nvperf_grfx_target.dll")))
        IsDebugToolAttached = true;
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
    HRESULT result = _device->QueryInterface(IID_PPV_ARGS(&infoQueue));
    LOG_DIRECTX_RESULT(result);
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

    // Get newer device interfaces
#ifdef __ID3D12Device1_FWD_DEFINED__
    _device->QueryInterface(IID_PPV_ARGS(&_device1));
#endif
#ifdef __ID3D12Device2_FWD_DEFINED__
    _device->QueryInterface(IID_PPV_ARGS(&_device2));
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

        D3D12_FEATURE_DATA_D3D12_OPTIONS2 options2 = {};
        if (SUCCEEDED(_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &options2, sizeof(options2))))
            limits.HasDepthBounds = !!options2.DepthBoundsTestSupported;

    }

#if !BUILD_RELEASE
	// Prevent the GPU from overclocking or under-clocking to get consistent timings
    if (CommandLine::Options.ShaderProfile.IsTrue())
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
    {
        RootSignatureDX12 signature;
        ComPtr<ID3DBlob> signatureBlob = signature.Serialize();
        VALIDATE_DIRECTX_CALL(_device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&_rootSignature)));
    }

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
    UploadBuffer.BeginGeneration(Engine::FrameCount);
}

void GPUDeviceDX12::RenderEnd()
{
    // Base
    GPUDeviceDX::RenderEnd();

    // Resolve the queries
    for (auto heap : QueryHeaps)
        heap->EndQueryBatchAndResolveQueryData(_mainContext);
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

GPUQueryDX12 GPUDeviceDX12::AllocQuery(GPUQueryType type)
{
    // Get query heap with free space
    int32 heapIndex = 0;
    int32 count = GPUQueryDX12::GetQueriesCount(type);
    for (; heapIndex < QueryHeaps.Count(); heapIndex++)
    {
        auto heap = QueryHeaps[heapIndex];
        if (heap->Type == type && heap->CanAlloc(count))
            break;
    }
    if (heapIndex == QueryHeaps.Count())
    {
        // Allocate a new query heap
        auto heap = New<QueryHeapDX12>();
        int32 size = type == GPUQueryType::Occlusion ? 4096 : 1024;
        if (heap->Init(this, type, size))
        {
            Delete(heap);
            return {};
        }
        QueryHeaps.Add(heap);
    }

    // Alloc query from the heap
    GPUQueryDX12 query = {};
    {
        static_assert(sizeof(GPUQueryDX12) == sizeof(uint64), "Invalid DX12 query size.");
        query.Type = (uint16)type;
        query.Heap = heapIndex;
        auto heap = QueryHeaps[heapIndex];
        heap->Alloc(query.Element);
        if (count == 2)
            heap->Alloc(query.SecondaryElement);
    }
    return query;
}

void GPUDeviceDX12::Dispose()
{
    GPUDeviceLock lock(this);
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
    SAFE_DELETE_GPU_RESOURCE(DummyVB);
    for (auto& srv : _nullSrv)
        srv.Release();
    _nullUav.Release();
    for (auto* heap : QueryHeaps)
    {
        heap->Destroy();
        Delete(heap);
    }
    QueryHeaps.Clear();
    DX_SAFE_RELEASE_CHECK(_rootSignature, 0);
    Heap_CBV_SRV_UAV.ReleaseGPU();
    Heap_RTV.ReleaseGPU();
    Heap_DSV.ReleaseGPU();
    Heap_Sampler.ReleaseGPU();
    RingHeap_CBV_SRV_UAV.ReleaseGPU();
    RingHeap_Sampler.ReleaseGPU();
    UploadBuffer.ReleaseGPU();
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

bool GPUDeviceDX12::GetQueryResult(uint64 queryID, uint64& result, bool wait)
{
    GPUQueryDX12 query;
    query.Raw = queryID;
    auto heap = QueryHeaps[query.Heap];
    int32 count = GPUQueryDX12::GetQueriesCount((GPUQueryType)query.Type);
    if (!wait && (!heap->IsReady(query.Element) || (count != 2 || !heap->IsReady(query.SecondaryElement))))
        return false;
    if (query.Type == (uint16)GPUQueryType::Timer)
    {
        uint64 timestampFrequency = 1;
        const uint64 timeBegin = *(uint64*)heap->Resolve(query.SecondaryElement);
        const uint64 timeEnd = *(uint64*)heap->Resolve(query.Element, &timestampFrequency);
        result = timeEnd > timeBegin ? (timeEnd - timeBegin) * 1000000ull / timestampFrequency : 0;
    }
    else
    {
        result = *(uint64*)heap->Resolve(query.Element);
    }
    return true;
}

GPUTexture* GPUDeviceDX12::CreateTexture(const StringView& name)
{
    PROFILE_MEM(GraphicsTextures);
    return New<GPUTextureDX12>(this, name);
}

GPUShader* GPUDeviceDX12::CreateShader(const StringView& name)
{
    PROFILE_MEM(GraphicsShaders);
    return New<GPUShaderDX12>(this, name);
}

GPUPipelineState* GPUDeviceDX12::CreatePipelineState()
{
    PROFILE_MEM(GraphicsCommands);
    return New<GPUPipelineStateDX12>(this);
}

GPUTimerQuery* GPUDeviceDX12::CreateTimerQuery()
{
    return New<GPUTimerQueryDX12>(this);
}

GPUBuffer* GPUDeviceDX12::CreateBuffer(const StringView& name)
{
    PROFILE_MEM(GraphicsBuffers);
    return New<GPUBufferDX12>(this, name);
}

GPUSampler* GPUDeviceDX12::CreateSampler()
{
    return New<GPUSamplerDX12>(this);
}

GPUVertexLayout* GPUDeviceDX12::CreateVertexLayout(const VertexElements& elements, bool explicitOffsets)
{
    return New<GPUVertexLayoutDX12>(this, elements, explicitOffsets);
}

GPUSwapChain* GPUDeviceDX12::CreateSwapChain(Window* window)
{
    return New<GPUSwapChainDX12>(this, window);
}

GPUConstantBuffer* GPUDeviceDX12::CreateConstantBuffer(uint32 size, const StringView& name)
{
    PROFILE_MEM(GraphicsShaders);
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
    // TODO: support 120/40/30/24 fps
    VALIDATE_DIRECTX_CALL(_device->SetFrameIntervalX(dxgiOutput.Get(), D3D12XBOX_FRAME_INTERVAL_60_HZ, DX12_BACK_BUFFER_COUNT - 1u, D3D12XBOX_FRAME_INTERVAL_FLAG_NONE));
    VALIDATE_DIRECTX_CALL(_device->ScheduleFrameEventX(D3D12XBOX_FRAME_EVENT_ORIGIN, 0U, nullptr, D3D12XBOX_SCHEDULE_FRAME_EVENT_FLAG_NONE));
}

#endif

GPUDevice* CreateGPUDeviceDX12()
{
    return GPUDeviceDX12::Create();
}

#endif
