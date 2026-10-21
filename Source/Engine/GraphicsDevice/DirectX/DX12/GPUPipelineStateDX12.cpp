// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "GPUPipelineStateDX12.h"
#include "GPUShaderProgramDX12.h"
#include "GPUVertexLayoutDX12.h"
#include "GPUTextureDX12.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#if GPU_D3D12_PSO_STREAM
// TODO: migrate to Agility SDK and remove that custom header
#include <ThirdParty/DirectX12Agility/d3dx12/d3dx12_pipeline_state_stream_custom.h>
#endif

#if GPU_D3D12_PSO_STREAM
struct alignas(void*) GraphicsPipelineStateStreamDX12
{
    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
    CD3DX12_PIPELINE_STATE_STREAM_VS VS;
    CD3DX12_PIPELINE_STATE_STREAM_PS PS;
#if GPU_ALLOW_GEOMETRY_SHADERS
    CD3DX12_PIPELINE_STATE_STREAM_GS GS;
#endif
#if GPU_ALLOW_TESSELLATION_SHADERS
    CD3DX12_PIPELINE_STATE_STREAM_HS HS;
    CD3DX12_PIPELINE_STATE_STREAM_DS DS;
#endif
    CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC BlendState;
    CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK SampleMask;
    CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER RasterizerState;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1 DepthStencilState;
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTFormats;
    CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
};
#else
typedef D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateStreamDX12;
#endif

static D3D12_STENCIL_OP ToStencilOp(StencilOperation value)
{
    switch (value)
    {
    case StencilOperation::Keep:
        return D3D12_STENCIL_OP_KEEP;
    case StencilOperation::Zero:
        return D3D12_STENCIL_OP_ZERO;
    case StencilOperation::Replace:
        return D3D12_STENCIL_OP_REPLACE;
    case StencilOperation::IncrementSaturated:
        return D3D12_STENCIL_OP_INCR_SAT;
    case StencilOperation::DecrementSaturated:
        return D3D12_STENCIL_OP_DECR_SAT;
    case StencilOperation::Invert:
        return D3D12_STENCIL_OP_INVERT;
    case StencilOperation::Increment:
        return D3D12_STENCIL_OP_INCR;
    case StencilOperation::Decrement:
        return D3D12_STENCIL_OP_DECR;
    default:
        return D3D12_STENCIL_OP_KEEP;
    };
}

GPUPipelineStateDX12::GPUPipelineStateDX12(GPUDeviceDX12* device)
    : GPUResourceDX12(device, StringView::Empty)
    , _states(16)
{
}

bool GPUPipelineStateDX12::IsValid() const
{
    return !!_memoryUsage;
}

ID3D12PipelineState* GPUPipelineStateDX12::GetState(GPUTextureViewDX12* depth, int32 rtCount, GPUTextureViewDX12** rtHandles, GPUVertexLayoutDX12* vertexLayout)
{
    ASSERT(depth || rtCount);

    // Prepare key
    GPUPipelineStateKeyDX12 key;
    key.RTsCount = rtCount;
    key.VertexLayout = vertexLayout;
    key.DepthFormat = depth ? depth->GetFormat() : PixelFormat::Unknown;
    key.MSAA = depth ? depth->GetMSAA() : (rtCount ? rtHandles[0]->GetMSAA() : MSAALevel::None);
    for (int32 i = 0; i < rtCount; i++)
        key.RTVsFormats[i] = rtHandles[i]->GetFormat();
    for (int32 i = rtCount; i < GPU_MAX_RT_BINDED; i++)
        key.RTVsFormats[i] = PixelFormat::Unknown;

    // Try reuse cached version
    ID3D12PipelineState* state = nullptr;
    if (_states.TryGet(key, state))
    {
#if BUILD_DEBUG
        // Verify
        GPUPipelineStateKeyDX12 refKey;
        _states.KeyOf(state, &refKey);
        ASSERT(refKey == key);
#endif
        return state;
    }
    PROFILE_CPU();
#if !BUILD_RELEASE
    DebugName name;
    GetDebugName(name);
    ZoneText(name.Get(), name.Count() - 1);
#endif

    // Setup description to match the pipeline
    GraphicsPipelineStateStreamDX12 desc = {};
    desc.pRootSignature = _device->GetRootSignature();
    desc.PrimitiveTopologyType = _primitiveTopology;
    desc.DepthStencilState = _depthStencil;
    desc.RasterizerState = _rasterizer;
    desc.BlendState = _blend;
#if GPU_D3D12_PSO_STREAM
    D3D12_RT_FORMAT_ARRAY rtFormats = {};
    rtFormats.NumRenderTargets = key.RTsCount;
    for (int32 i = 0; i < GPU_MAX_RT_BINDED; i++)
        rtFormats.RTFormats[i] = RenderToolsDX::ToDxgiFormat(key.RTVsFormats[i]);
    desc.RTFormats = rtFormats;
#else
    desc.NumRenderTargets = key.RTsCount;
    for (int32 i = 0; i < GPU_MAX_RT_BINDED; i++)
        desc.RTVFormats[i] = RenderToolsDX::ToDxgiFormat(key.RTVsFormats[i]);
#endif
    desc.SampleDesc = { (UINT)key.MSAA, key.MSAA == MSAALevel::None ? 0 : GPUDeviceDX12::GetMaxMSAAQuality((int32)key.MSAA) };
    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    desc.DSVFormat = RenderToolsDX::ToDxgiFormat(PixelFormatExtensions::FindDepthStencilFormat(key.DepthFormat));
    if (!vertexLayout)
        vertexLayout = VertexBufferLayout; // Fallback to shader-specified layout (if using old APIs)
    if (vertexLayout)
    {
        int32 missingSlotOverride = GPU_MAX_VB_BINDED; // Use additional slot with empty VB
        if (VertexInputLayout)
            vertexLayout = (GPUVertexLayoutDX12*)GPUVertexLayout::Merge(vertexLayout, VertexInputLayout, false, true, missingSlotOverride);
        desc.InputLayout = { vertexLayout->InputElements, vertexLayout->InputElementsCount };
    }
    else
    {
        desc.InputLayout = { nullptr, 0 };
    }
#if GPU_ALLOW_TESSELLATION_SHADERS
    desc.HS = _shaderHS;
    desc.DS = _shaderDS;
#endif
#if GPU_ALLOW_GEOMETRY_SHADERS
    desc.GS = _shaderGS;
#endif
    desc.VS = _shaderVS;
    desc.PS = _shaderPS;

    // Create object
#if GPU_D3D12_PSO_STREAM
    D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = { sizeof(desc), &desc };
    const HRESULT result = _device->GetDevice2()->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&state));
#else
    const HRESULT result = _device->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&state));
#endif
    LOG_DIRECTX_RESULT(result);
    if (FAILED(result))
    {
#if !BUILD_RELEASE
        LOG(Error, "CreateGraphicsPipelineState failed for {}", String(name.Get(), name.Count() - 1));
#endif
        return nullptr;
    }
#if GPU_ENABLE_RESOURCE_NAMING && !BUILD_RELEASE
    SetDebugObjectName(state, name.Get(), name.Count() - 1);
#endif

    // Cache it
    _states.Add(key, state);

    return state;
}

void GPUPipelineStateDX12::OnReleaseGPU()
{
    for (auto i = _states.Begin(); i.IsNotEnd(); ++i)
    {
        _device->AddResourceToLateRelease(i->Value);
    }
    _states.Clear();
}

bool GPUPipelineStateDX12::Init(const Description& desc)
{
    if (IsValid())
        OnReleaseGPU();

    // Shaders
    Platform::MemoryClear(&Header, sizeof(Header));
#define INIT_SHADER_STAGE(stage, type) \
    if (desc.stage) \
    { \
        _shader##stage = { desc.stage->GetBufferHandle(), desc.stage->GetBufferSize() }; \
        auto shader = (type*)desc.stage; \
        auto srCount = Math::FloorLog2(shader->GetBindings().UsedSRsMask) + 1; \
        for (uint32 i = 0; i < srCount; i++) \
            if (shader->Header.SrDimensions[i]) \
                Header.SrDimensions[i] = shader->Header.SrDimensions[i]; \
        auto uaCount = Math::FloorLog2(shader->GetBindings().UsedUAsMask) + 1; \
        for (uint32 i = 0; i < uaCount; i++) \
            if (shader->Header.UaDimensions[i]) \
                Header.UaDimensions[i] = shader->Header.UaDimensions[i]; \
    } \
    else _shader##stage = {};
#if GPU_ALLOW_TESSELLATION_SHADERS
    INIT_SHADER_STAGE(HS, GPUShaderProgramHSDX12);
    INIT_SHADER_STAGE(DS, GPUShaderProgramDSDX12);
#endif
#if GPU_ALLOW_GEOMETRY_SHADERS
    INIT_SHADER_STAGE(GS, GPUShaderProgramGSDX12);
#endif
    INIT_SHADER_STAGE(VS, GPUShaderProgramVSDX12);
    INIT_SHADER_STAGE(PS, GPUShaderProgramPSDX12);

    // Input Assembly
    if (desc.VS)
    {
        VertexBufferLayout = (GPUVertexLayoutDX12*)desc.VS->Layout;
        VertexInputLayout = (GPUVertexLayoutDX12*)desc.VS->InputLayout;
    }
    const D3D12_PRIMITIVE_TOPOLOGY_TYPE primTypes1[] =
    {
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
    };
    const D3D_PRIMITIVE_TOPOLOGY primTypes2[] =
    {
        D3D_PRIMITIVE_TOPOLOGY_UNDEFINED,
        D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
        D3D_PRIMITIVE_TOPOLOGY_LINELIST,
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    };
    _primitiveTopology = primTypes1[(int32)desc.PrimitiveTopology];
    PrimitiveTopology = primTypes2[(int32)desc.PrimitiveTopology];
#if GPU_ALLOW_TESSELLATION_SHADERS
    if (desc.HS)
    {
        _primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        PrimitiveTopology = (D3D_PRIMITIVE_TOPOLOGY)((int32)D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + (desc.HS->GetControlPointsCount() - 1));
    }
#endif

    // Depth State
    Platform::MemoryClear(&_depthStencil, sizeof(_depthStencil));
    _depthStencil.DepthEnable = !!desc.DepthEnable;
    _depthStencil.DepthWriteMask = desc.DepthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
#if GPU_D3D12_PSO_STREAM
    _depthStencil.DepthBoundsTestEnable = !!desc.DepthBoundsEnable;
#endif
    _depthStencil.DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(desc.DepthFunc);
    _depthStencil.StencilEnable = !!desc.StencilEnable;
    _depthStencil.StencilReadMask = desc.StencilReadMask;
    _depthStencil.StencilWriteMask = desc.StencilWriteMask;
    _depthStencil.FrontFace.StencilFailOp = ToStencilOp(desc.StencilFailOp);
    _depthStencil.FrontFace.StencilDepthFailOp = ToStencilOp(desc.StencilDepthFailOp);
    _depthStencil.FrontFace.StencilPassOp = ToStencilOp(desc.StencilPassOp);
    _depthStencil.FrontFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(desc.StencilFunc);
    _depthStencil.BackFace = _depthStencil.FrontFace;

    // Rasterizer State
    Platform::MemoryClear(&_rasterizer, sizeof(_rasterizer));
    _rasterizer.FillMode = desc.Wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
    const D3D12_CULL_MODE cullModes[] =
    {
        D3D12_CULL_MODE_BACK,
        D3D12_CULL_MODE_FRONT,
        D3D12_CULL_MODE_NONE,
    };
    _rasterizer.CullMode = cullModes[(int32)desc.CullMode];
    _rasterizer.FrontCounterClockwise = FALSE;
    _rasterizer.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    _rasterizer.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    _rasterizer.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    _rasterizer.DepthClipEnable = !!desc.DepthClipEnable;
    _rasterizer.MultisampleEnable = TRUE;
    _rasterizer.AntialiasedLineEnable = !!desc.Wireframe;

    // Blend State
    Platform::MemoryClear(&_blend, sizeof(_blend));
    _blend.AlphaToCoverageEnable = desc.BlendMode.AlphaToCoverageEnable ? TRUE : FALSE;
    _blend.IndependentBlendEnable = FALSE;
    _blend.RenderTarget[0].BlendEnable = desc.BlendMode.BlendEnable ? TRUE : FALSE;
    _blend.RenderTarget[0].SrcBlend = (D3D12_BLEND)desc.BlendMode.SrcBlend;
    _blend.RenderTarget[0].DestBlend = (D3D12_BLEND)desc.BlendMode.DestBlend;
    _blend.RenderTarget[0].BlendOp = (D3D12_BLEND_OP)desc.BlendMode.BlendOp;
    _blend.RenderTarget[0].SrcBlendAlpha = (D3D12_BLEND)desc.BlendMode.SrcBlendAlpha;
    _blend.RenderTarget[0].DestBlendAlpha = (D3D12_BLEND)desc.BlendMode.DestBlendAlpha;
    _blend.RenderTarget[0].BlendOpAlpha = (D3D12_BLEND_OP)desc.BlendMode.BlendOpAlpha;
    _blend.RenderTarget[0].RenderTargetWriteMask = (UINT8)desc.BlendMode.RenderTargetWriteMask;
    for (uint32 i = 1; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
        _blend.RenderTarget[i] = _blend.RenderTarget[0];

    // Set non-zero memory usage
    _memoryUsage = sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC);

    return GPUPipelineState::Init(desc);
}

#endif
