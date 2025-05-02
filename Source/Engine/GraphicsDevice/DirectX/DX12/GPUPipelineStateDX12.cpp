// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "GPUPipelineStateDX12.h"
#include "GPUShaderProgramDX12.h"
#include "GPUVertexLayoutDX12.h"
#include "GPUTextureDX12.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"
#include "Engine/Graphics/PixelFormatExtensions.h"

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
    PROFILE_CPU_NAMED("Create Pipeline State");

    // Update description to match the pipeline
    _desc.NumRenderTargets = key.RTsCount;
    for (int32 i = 0; i < GPU_MAX_RT_BINDED; i++)
        _desc.RTVFormats[i] = RenderToolsDX::ToDxgiFormat(key.RTVsFormats[i]);
    _desc.SampleDesc.Count = static_cast<UINT>(key.MSAA);
    _desc.SampleDesc.Quality = key.MSAA == MSAALevel::None ? 0 : GPUDeviceDX12::GetMaxMSAAQuality((int32)key.MSAA);
    _desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    _desc.DSVFormat = RenderToolsDX::ToDxgiFormat(PixelFormatExtensions::FindDepthStencilFormat(key.DepthFormat));
    if (!vertexLayout)
        vertexLayout = VertexBufferLayout; // Fallback to shader-specified layout (if using old APIs)
    if (vertexLayout)
    {
        int32 missingSlotOverride = GPU_MAX_VB_BINDED; // Use additional slot with empty VB
        if (VertexInputLayout)
            vertexLayout = (GPUVertexLayoutDX12*)GPUVertexLayout::Merge(vertexLayout, VertexInputLayout, false, true, missingSlotOverride);
        _desc.InputLayout.pInputElementDescs = vertexLayout->InputElements;
        _desc.InputLayout.NumElements = vertexLayout->InputElementsCount;
    }
    else
    {
        _desc.InputLayout.pInputElementDescs = nullptr;
        _desc.InputLayout.NumElements = 0;
    }

    // Create object
    const HRESULT result = _device->GetDevice()->CreateGraphicsPipelineState(&_desc, IID_PPV_ARGS(&state));
    LOG_DIRECTX_RESULT(result);
    if (FAILED(result))
        return nullptr;
#if GPU_ENABLE_RESOURCE_NAMING && BUILD_DEBUG
    Array<char, InlinedAllocation<200>> name;
    if (DebugDesc.VS)
    {
        name.Add(*DebugDesc.VS->GetName(), DebugDesc.VS->GetName().Length());
        name.Add('+');
    }
#if GPU_ALLOW_TESSELLATION_SHADERS
    if (DebugDesc.HS)
    {
        name.Add(*DebugDesc.HS->GetName(), DebugDesc.HS->GetName().Length());
        name.Add('+');
    }
    if (DebugDesc.DS)
    {
        name.Add(*DebugDesc.DS->GetName(), DebugDesc.DS->GetName().Length());
        name.Add('+');
    }
#endif
#if GPU_ALLOW_GEOMETRY_SHADERS
    if (DebugDesc.GS)
    {
        name.Add(*DebugDesc.GS->GetName(), DebugDesc.GS->GetName().Length());
        name.Add('+');
    }
#endif
    if (DebugDesc.PS)
    {
        name.Add(*DebugDesc.PS->GetName(), DebugDesc.PS->GetName().Length());
        name.Add('+');
    }
    if (name.Count() != 0 && name[name.Count() - 1] == '+')
        name.RemoveLast();
    name.Add('\0');
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
    ASSERT(!IsValid());

    // Create description
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psDesc;
    Platform::MemoryClear(&psDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psDesc.pRootSignature = _device->GetRootSignature();

    // Shaders
    Platform::MemoryClear(&Header, sizeof(Header));
#define INIT_SHADER_STAGE(stage, type) \
    if (desc.stage) \
    { \
        psDesc.stage = { desc.stage->GetBufferHandle(), desc.stage->GetBufferSize() }; \
        auto shader = (type*)desc.stage; \
        auto srCount = Math::FloorLog2(shader->GetBindings().UsedSRsMask) + 1; \
        for (uint32 i = 0; i < srCount; i++) \
            if (shader->Header.SrDimensions[i]) \
                Header.SrDimensions[i] = shader->Header.SrDimensions[i]; \
        auto uaCount = Math::FloorLog2(shader->GetBindings().UsedUAsMask) + 1; \
        for (uint32 i = 0; i < uaCount; i++) \
            if (shader->Header.UaDimensions[i]) \
                Header.UaDimensions[i] = shader->Header.UaDimensions[i]; \
    }
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
    psDesc.PrimitiveTopologyType = primTypes1[(int32)desc.PrimitiveTopology];
    PrimitiveTopology = primTypes2[(int32)desc.PrimitiveTopology];
#if GPU_ALLOW_TESSELLATION_SHADERS
    if (desc.HS)
    {
        psDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        PrimitiveTopology = (D3D_PRIMITIVE_TOPOLOGY)((int32)D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + (desc.HS->GetControlPointsCount() - 1));
    }
#endif

    // Depth State
    psDesc.DepthStencilState.DepthEnable = !!desc.DepthEnable;
    psDesc.DepthStencilState.DepthWriteMask = desc.DepthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    psDesc.DepthStencilState.DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(desc.DepthFunc);
    psDesc.DepthStencilState.StencilEnable = !!desc.StencilEnable;
    psDesc.DepthStencilState.StencilReadMask = desc.StencilReadMask;
    psDesc.DepthStencilState.StencilWriteMask = desc.StencilWriteMask;
    psDesc.DepthStencilState.FrontFace.StencilFailOp = ToStencilOp(desc.StencilFailOp);
    psDesc.DepthStencilState.FrontFace.StencilDepthFailOp = ToStencilOp(desc.StencilDepthFailOp);
    psDesc.DepthStencilState.FrontFace.StencilPassOp = ToStencilOp(desc.StencilPassOp);
    psDesc.DepthStencilState.FrontFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(desc.StencilFunc);
    psDesc.DepthStencilState.BackFace = psDesc.DepthStencilState.FrontFace;

    // Rasterizer State
    psDesc.RasterizerState.FillMode = desc.Wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
    D3D12_CULL_MODE dxCullMode;
    switch (desc.CullMode)
    {
    case CullMode::Normal:
        dxCullMode = D3D12_CULL_MODE_BACK;
        break;
    case CullMode::Inverted:
        dxCullMode = D3D12_CULL_MODE_FRONT;
        break;
    case CullMode::TwoSided:
        dxCullMode = D3D12_CULL_MODE_NONE;
        break;
    }
    psDesc.RasterizerState.CullMode = dxCullMode;
    psDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    psDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    psDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    psDesc.RasterizerState.DepthClipEnable = !!desc.DepthClipEnable;
    psDesc.RasterizerState.MultisampleEnable = TRUE;
    psDesc.RasterizerState.AntialiasedLineEnable = !!desc.Wireframe;
    psDesc.RasterizerState.ForcedSampleCount = 0;
    psDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // Blend State
    psDesc.BlendState.AlphaToCoverageEnable = desc.BlendMode.AlphaToCoverageEnable ? TRUE : FALSE;
    psDesc.BlendState.IndependentBlendEnable = FALSE;
    psDesc.BlendState.RenderTarget[0].BlendEnable = desc.BlendMode.BlendEnable ? TRUE : FALSE;
    psDesc.BlendState.RenderTarget[0].SrcBlend = (D3D12_BLEND)desc.BlendMode.SrcBlend;
    psDesc.BlendState.RenderTarget[0].DestBlend = (D3D12_BLEND)desc.BlendMode.DestBlend;
    psDesc.BlendState.RenderTarget[0].BlendOp = (D3D12_BLEND_OP)desc.BlendMode.BlendOp;
    psDesc.BlendState.RenderTarget[0].SrcBlendAlpha = (D3D12_BLEND)desc.BlendMode.SrcBlendAlpha;
    psDesc.BlendState.RenderTarget[0].DestBlendAlpha = (D3D12_BLEND)desc.BlendMode.DestBlendAlpha;
    psDesc.BlendState.RenderTarget[0].BlendOpAlpha = (D3D12_BLEND_OP)desc.BlendMode.BlendOpAlpha;
    psDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = (UINT8)desc.BlendMode.RenderTargetWriteMask;
#if BUILD_DEBUG
    for (byte i = 1; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
        psDesc.BlendState.RenderTarget[i] = psDesc.BlendState.RenderTarget[0];
#endif

    // Cache description
    _desc = psDesc;

    // Set non-zero memory usage
    _memoryUsage = sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC);

    return GPUPipelineState::Init(desc);
}

#endif
