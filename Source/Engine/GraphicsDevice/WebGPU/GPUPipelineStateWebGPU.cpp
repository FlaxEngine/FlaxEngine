// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_WEBGPU

#include "GPUPipelineStateWebGPU.h"
#include "GPUVertexLayoutWebGPU.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerMemory.h"

WGPUCompareFunction ToCompareFunction(ComparisonFunc value)
{
    switch (value)
    {
    case ComparisonFunc::Never:
        return WGPUCompareFunction_Never;
    case ComparisonFunc::Less:
        return WGPUCompareFunction_Less;
    case ComparisonFunc::Equal:
        return WGPUCompareFunction_Equal;
    case ComparisonFunc::LessEqual:
        return WGPUCompareFunction_LessEqual;
    case ComparisonFunc::Greater:
        return WGPUCompareFunction_Greater;
    case ComparisonFunc::NotEqual:
        return WGPUCompareFunction_NotEqual;
    case ComparisonFunc::GreaterEqual:
        return WGPUCompareFunction_GreaterEqual;
    case ComparisonFunc::Always:
        return WGPUCompareFunction_Always;
    default:
        return WGPUCompareFunction_Undefined;
    }
}

WGPUStencilOperation ToStencilOperation(StencilOperation value)
{
    switch (value)
    {
    case StencilOperation::Keep:
        return WGPUStencilOperation_Keep;
    case StencilOperation::Zero:
        return WGPUStencilOperation_Zero;
    case StencilOperation::Replace:
        return WGPUStencilOperation_Replace;
    case StencilOperation::IncrementSaturated:
        return WGPUStencilOperation_IncrementClamp;
    case StencilOperation::DecrementSaturated:
        return WGPUStencilOperation_DecrementClamp;
    case StencilOperation::Invert:
        return WGPUStencilOperation_Invert;
    case StencilOperation::Increment:
        return WGPUStencilOperation_IncrementWrap;
    case StencilOperation::Decrement:
        return WGPUStencilOperation_DecrementWrap;
    default:
        return WGPUStencilOperation_Undefined;
    }
}

WGPUBlendFactor ToBlendFactor(BlendingMode::Blend value)
{
    switch (value)
    {
    case BlendingMode::Blend::Zero:
        return WGPUBlendFactor_Zero;
    case BlendingMode::Blend::One:
        return WGPUBlendFactor_One;
    case BlendingMode::Blend::SrcColor:
        return WGPUBlendFactor_Src;
    case BlendingMode::Blend::InvSrcColor:
        return WGPUBlendFactor_OneMinusSrc;
    case BlendingMode::Blend::SrcAlpha:
        return WGPUBlendFactor_SrcAlpha;
    case BlendingMode::Blend::InvSrcAlpha:
        return WGPUBlendFactor_OneMinusSrcAlpha;
    case BlendingMode::Blend::DestAlpha:
        return WGPUBlendFactor_DstAlpha;
    case BlendingMode::Blend::InvDestAlpha:
        return WGPUBlendFactor_OneMinusDstAlpha;
    case BlendingMode::Blend::DestColor:
        return WGPUBlendFactor_Dst;
    case BlendingMode::Blend::InvDestColor:
        return WGPUBlendFactor_OneMinusDst;
    case BlendingMode::Blend::SrcAlphaSat:
        return WGPUBlendFactor_SrcAlphaSaturated;
    case BlendingMode::Blend::BlendFactor:
        return WGPUBlendFactor_Constant;
    case BlendingMode::Blend::BlendInvFactor:
        return WGPUBlendFactor_OneMinusConstant;
    case BlendingMode::Blend::Src1Color:
        return WGPUBlendFactor_Src1;
    case BlendingMode::Blend::InvSrc1Color:
        return WGPUBlendFactor_OneMinusSrc1;
    case BlendingMode::Blend::Src1Alpha:
        return WGPUBlendFactor_Src1Alpha;
    case BlendingMode::Blend::InvSrc1Alpha:
        return WGPUBlendFactor_OneMinusSrc1Alpha;
    default:
        return WGPUBlendFactor_Undefined;
    }
}

WGPUBlendComponent ToBlendComponent(BlendingMode::Operation blendOp, BlendingMode::Blend srcBlend, BlendingMode::Blend dstBlend)
{
    WGPUBlendComponent result;
    switch (blendOp)
    {
    case BlendingMode::Operation::Add:
        result.operation = WGPUBlendOperation_Add;
        break;
    case BlendingMode::Operation::Subtract:
        result.operation = WGPUBlendOperation_Subtract;
        break;
    case BlendingMode::Operation::RevSubtract:
        result.operation = WGPUBlendOperation_ReverseSubtract;
        break;
    case BlendingMode::Operation::Min:
        result.operation = WGPUBlendOperation_Min;
        break;
    case BlendingMode::Operation::Max:
        result.operation = WGPUBlendOperation_Max;
        break;
    default:
        result.operation = WGPUBlendOperation_Undefined;
        break;
    }
    result.srcFactor = ToBlendFactor(srcBlend);
    result.dstFactor = ToBlendFactor(dstBlend);
    return result;
}

void GPUPipelineStateWebGPU::OnReleaseGPU()
{
    VS = nullptr;
    PS = nullptr;
    for (auto& e : _pipelines)
        wgpuRenderPipelineRelease(e.Value);
    _pipelines.Clear();
}

uint32 GetHash(const GPUPipelineStateWebGPU::Key& key)
{
    static_assert(sizeof(GPUPipelineStateWebGPU::Key) == sizeof(uint64) * 4, "Invalid PSO key size.");
    uint32 hash = GetHash(key.Packed[0]);
    CombineHash(hash, GetHash(key.Packed[1]));
    return hash;
}

WGPURenderPipeline GPUPipelineStateWebGPU::GetPipeline(const Key& key)
{
    WGPURenderPipeline pipeline;
    if (_pipelines.TryGet(key, pipeline))
        return pipeline;
    PROFILE_CPU();
    PROFILE_MEM(GraphicsCommands);
#if GPU_ENABLE_RESOURCE_NAMING
    ZoneText(_debugName.Get(), _debugName.Count() - 1);
#endif

    // Build final pipeline description
    _depthStencilDesc.format = (WGPUTextureFormat)key.DepthStencilFormat;
    PipelineDesc.multisample.count = key.MultiSampleCount;
    _fragmentDesc.targetCount = key.RenderTargetCount;
    for (int32 i = 0; i < _fragmentDesc.targetCount; i++)
        _colorTargets[i].format = (WGPUTextureFormat)key.RenderTargetFormats[i];
    WGPUVertexBufferLayout buffers[GPU_MAX_VB_BINDED];
    for (int32 i = 0; i < PipelineDesc.vertex.bufferCount; i++)
        buffers[i] = *key.VertexBuffers[i];
    PipelineDesc.vertex.buffers = buffers;

    // Create object
    pipeline = wgpuDeviceCreateRenderPipeline(_device->Device, &PipelineDesc);
    if (!pipeline)
    {
#if GPU_ENABLE_RESOURCE_NAMING
        LOG(Error, "wgpuDeviceCreateRenderPipeline failed for {}", String(_debugName.Get(), _debugName.Count() - 1));
#endif
        return nullptr;
    }

    // Cache it
    _pipelines.Add(key, pipeline);

    return pipeline;
}

bool GPUPipelineStateWebGPU::IsValid() const
{
    return _memoryUsage != 0;
}

bool GPUPipelineStateWebGPU::Init(const Description& desc)
{
    if (IsValid())
        OnReleaseGPU();

    // Cache shaders
    VS = (GPUShaderProgramVSWebGPU*)desc.VS;
    PS = (GPUShaderProgramPSWebGPU*)desc.PS;

    // Initialize description (without dynamic state from context such as render targets, vertex buffers, etc.)
    PipelineDesc = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
#if GPU_ENABLE_RESOURCE_NAMING
    GetDebugName(_debugName);
    PipelineDesc.label = { _debugName.Get(), (size_t)_debugName.Count() - 1 };
#endif
    PipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    PipelineDesc.primitive.frontFace = WGPUFrontFace_CW;
    switch (desc.CullMode)
    {
    case CullMode::Normal:
        PipelineDesc.primitive.cullMode = WGPUCullMode_Back;
        break;
    case CullMode::Inverted:
        PipelineDesc.primitive.cullMode = WGPUCullMode_Front;
        break;
    case CullMode::TwoSided:
        PipelineDesc.primitive.cullMode = WGPUCullMode_None;
        break;
    }
    PipelineDesc.primitive.unclippedDepth = !desc.DepthClipEnable && _device->Limits.HasDepthClip;
    if (desc.DepthEnable || desc.StencilEnable)
    {
        PipelineDesc.depthStencil = &_depthStencilDesc;
        _depthStencilDesc = WGPU_DEPTH_STENCIL_STATE_INIT;
        _depthStencilDesc.depthWriteEnabled = desc.DepthEnable && desc.DepthWriteEnable ? WGPUOptionalBool_True : WGPUOptionalBool_False;
        _depthStencilDesc.depthCompare = ToCompareFunction(desc.DepthFunc);
        if (desc.StencilEnable)
        {
            _depthStencilDesc.stencilFront.compare = ToCompareFunction(desc.StencilFunc);
            _depthStencilDesc.stencilFront.failOp = ToStencilOperation(desc.StencilFailOp);
            _depthStencilDesc.stencilFront.depthFailOp = ToStencilOperation(desc.StencilDepthFailOp);
            _depthStencilDesc.stencilFront.passOp = ToStencilOperation(desc.StencilPassOp);
            _depthStencilDesc.stencilBack = _depthStencilDesc.stencilFront;
            _depthStencilDesc.stencilReadMask = desc.StencilReadMask;
            _depthStencilDesc.stencilWriteMask = desc.StencilWriteMask;
        }
    }
    PipelineDesc.multisample.alphaToCoverageEnabled = desc.BlendMode.AlphaToCoverageEnable;
    PipelineDesc.fragment = &_fragmentDesc;
    _fragmentDesc = WGPU_FRAGMENT_STATE_INIT;
    _fragmentDesc.targets = _colorTargets;
    _blendState = WGPU_BLEND_STATE_INIT;
    _blendState.color = ToBlendComponent(desc.BlendMode.BlendOp, desc.BlendMode.SrcBlend, desc.BlendMode.DestBlend);
    _blendState.alpha = ToBlendComponent(desc.BlendMode.BlendOpAlpha, desc.BlendMode.SrcBlendAlpha, desc.BlendMode.DestBlendAlpha);
    WGPUColorWriteMask writeMask = WGPUColorWriteMask_All;
    if (desc.BlendMode.RenderTargetWriteMask != BlendingMode::ColorWrite::All)
    {
        writeMask = 0;
        if (EnumHasAllFlags(desc.BlendMode.RenderTargetWriteMask, BlendingMode::ColorWrite::Red))
            writeMask |= WGPUColorWriteMask_Red;
        if (EnumHasAllFlags(desc.BlendMode.RenderTargetWriteMask, BlendingMode::ColorWrite::Green))
            writeMask |= WGPUColorWriteMask_Green;
        if (EnumHasAllFlags(desc.BlendMode.RenderTargetWriteMask, BlendingMode::ColorWrite::Blue))
            writeMask |= WGPUColorWriteMask_Blue;
        if (EnumHasAllFlags(desc.BlendMode.RenderTargetWriteMask, BlendingMode::ColorWrite::Alpha))
            writeMask |= WGPUColorWriteMask_Alpha;
    }
    for (auto& e : _colorTargets)
    {
        e = WGPU_COLOR_TARGET_STATE_INIT;
        e.blend = &_blendState;
        e.writeMask = writeMask;
    }

    // TODO: set resources binding into PipelineDesc.layout
    // TODO: set vertex shader into PipelineDesc.vertex
    // TODO: set pixel shader into PipelineDesc.fragment

    _memoryUsage = 1;
    return GPUPipelineState::Init(desc);
}

#endif
