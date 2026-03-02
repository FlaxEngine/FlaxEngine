// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_WEBGPU

#include "GPUPipelineStateWebGPU.h"
#include "GPUTextureWebGPU.h"
#include "GPUVertexLayoutWebGPU.h"
#include "RenderToolsWebGPU.h"
#include "Engine/Core/Log.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerMemory.h"
#include "Engine/Graphics/PixelFormatExtensions.h"

#define WEBGPU_LOG_PSO 0

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
    for (auto& e : BindGroupLayouts)
    {
        if (e)
        {
            wgpuBindGroupLayoutRelease(e);
            e = nullptr;
        }
    }
    if (PipelineDesc.layout)
    {
        wgpuPipelineLayoutRelease(PipelineDesc.layout);
        PipelineDesc.layout = nullptr;
    }
    Platform::MemoryClear(&BindGroupDescriptors, sizeof(BindGroupDescriptors));
}

uint32 GetHash(const GPUPipelineStateWebGPU::Key& key)
{
    static_assert(sizeof(GPUPipelineStateWebGPU::Key) == sizeof(uint64) * 2, "Invalid PSO key size.");
    uint32 hash = GetHash(key.Packed[0]);
    CombineHash(hash, GetHash(key.Packed[1]));
    return hash;
}

WGPURenderPipeline GPUPipelineStateWebGPU::GetPipeline(const Key& key, GPUResourceView* shaderResources[GPU_MAX_SR_BINDED])
{
    WGPURenderPipeline pipeline;
    if (_pipelines.TryGet(key, pipeline))
        return pipeline;
    PROFILE_CPU();
    PROFILE_MEM(GraphicsCommands);
#if GPU_ENABLE_RESOURCE_NAMING
    ZoneText(_debugName.Get(), _debugName.Count() - 1);
#endif
#if WEBGPU_LOG_PSO
    LOG(Info, "[WebGPU] GetPipeline: '{}'", String(_debugName.Get(), _debugName.Count() - 1));
#endif

    // Lazy-init layout (cannot do it during Init as texture samplers that access eg. depth need to explicitly use UnfilterableFloat)
    if (!PipelineDesc.layout)
        InitLayout(shaderResources);

    // Build final pipeline description
    _depthStencilDesc.format = (WGPUTextureFormat)key.DepthStencilFormat;
    PipelineDesc.depthStencil = key.DepthStencilFormat ? &_depthStencilDesc : nullptr; // Unbind depth stencil state when no debug buffer is bound
    PipelineDesc.multisample.count = key.MultiSampleCount;
    if (PS)
    {
        _fragmentDesc.targetCount = key.RenderTargetCount;
        for (int32 i = 0; i < _fragmentDesc.targetCount; i++)
            _colorTargets[i].format = (WGPUTextureFormat)key.RenderTargetFormats[i];
    }
    WGPUVertexBufferLayout buffers[GPU_MAX_VB_BINDED];
    if (key.VertexLayout)
    {
        // Combine input layout of Vertex Buffers with the destination layout used by the Vertex Shader
        GPUVertexLayoutWebGPU* mergedVertexLayout = key.VertexLayout;
        if (!mergedVertexLayout)
            mergedVertexLayout = (GPUVertexLayoutWebGPU*)VS->Layout; // Fallback to shader-specified layout (if using old APIs)
        if (VS->InputLayout)
            mergedVertexLayout = (GPUVertexLayoutWebGPU*)GPUVertexLayout::Merge(mergedVertexLayout, VS->InputLayout, false, true, -1, true);

        // Build attributes list
        WGPUVertexAttribute attributes[GPU_MAX_VS_ELEMENTS];
        PipelineDesc.vertex.bufferCount = 0;
        PipelineDesc.vertex.buffers = buffers;
        int32 attributeIndex = 0;
        auto& elements = mergedVertexLayout->GetElements();
        for (int32 bufferIndex = 0; bufferIndex < GPU_MAX_VB_BINDED; bufferIndex++)
        {
            auto& buffer = buffers[bufferIndex];
            buffer.nextInChain = nullptr;
            buffer.stepMode = WGPUVertexStepMode_Vertex;
            buffer.arrayStride = 0;
            buffer.attributeCount = 0;
            buffer.attributes = attributes + attributeIndex;
            for (int32 i = 0; i < elements.Count(); i++)
            {
                const VertexElement& element = elements[i];
                if (element.Slot != bufferIndex)
                    continue;
                WGPUVertexAttribute& dst = attributes[attributeIndex++];
                buffer.attributeCount++;
                dst.nextInChain = nullptr;
                dst.format = RenderToolsWebGPU::ToVertexFormat(element.Format);
                dst.offset = element.Offset;
                dst.shaderLocation = i; // Elements are sorted to match Input Layout order of Vertex Shader as provided by GPUVertexLayout::Merge
                if (element.PerInstance)
                    buffer.stepMode = WGPUVertexStepMode_Instance;
                buffer.arrayStride = Math::Max<uint64>(buffer.arrayStride, element.Offset + PixelFormatExtensions::SizeInBytes(element.Format));
                PipelineDesc.vertex.bufferCount = Math::Max<size_t>(PipelineDesc.vertex.bufferCount, bufferIndex + 1);
            }
        }
    }
    else
    {
        // No vertex input
        PipelineDesc.vertex.bufferCount = 0;
        PipelineDesc.vertex.buffers = nullptr;
    }

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

void GPUPipelineStateWebGPU::InitLayout(GPUResourceView* shaderResources[GPU_MAX_SR_BINDED])
{
#if WEBGPU_LOG_PSO
    // Debug log for PSOs with specific name
    const bool log = true;// StringAnsiView(_debugName.Get(), _debugName.Count() - 1).Contains("PS_HalfDepth");
#endif

    // Count the biggest bind group entries (for all shaders) to allocate reused memory
    int32 maxEntriesCount = 0;
    for (int32 groupIndex = 0; groupIndex < ARRAY_COUNT(BindGroupDescriptors); groupIndex++)
    {
        auto descriptors = BindGroupDescriptors[groupIndex];
        if (descriptors && maxEntriesCount < descriptors->DescriptorTypesCount)
            maxEntriesCount = (int32)descriptors->DescriptorTypesCount;
    }
    Array<WGPUBindGroupLayoutEntry, InlinedAllocation<8>> entries;
    entries.Resize(maxEntriesCount);

    // Setup bind groups
    WGPUBindGroupLayoutEntry* entriesPtr = entries.Get();
    for (int32 groupIndex = 0; groupIndex < ARRAY_COUNT(BindGroupDescriptors); groupIndex++)
    {
        auto descriptors = BindGroupDescriptors[groupIndex];
        if (!descriptors || descriptors->DescriptorTypesCount == 0)
            continue;

        int32 entriesCount = descriptors->DescriptorTypesCount;
        Platform::MemoryClear(entries.Get(), sizeof(WGPUBindGroupLayoutEntry) * entriesCount);
        auto visibility = groupIndex == 0 ? WGPUShaderStage_Vertex : WGPUShaderStage_Fragment;
#if WEBGPU_LOG_PSO
        if (log)
            LOG(Info, " > group {} - {}", groupIndex, groupIndex == 0 ? TEXT("Vertex") : TEXT("Fragment"));
        const Char* samplerType = TEXT("?");
#endif
        for (int32 index = 0; index < entriesCount; index++)
        {
            auto& descriptor = descriptors->DescriptorTypes[index];
            auto& entry = entriesPtr[index];
            entry.binding = descriptor.Binding;
            entry.bindingArraySize = descriptor.Count;
            entry.visibility = visibility;
            switch (descriptor.DescriptorType)
            {
            case VK_DESCRIPTOR_TYPE_SAMPLER:
                entry.sampler.type = WGPUSamplerBindingType_Undefined;
                if (descriptor.Slot == 4 || descriptor.Slot == 5) // Hack for ShadowSampler and ShadowSamplerLinear (this could get binded samplers table just like for shaderResources)
                    entry.sampler.type = WGPUSamplerBindingType_Comparison;
#if WEBGPU_LOG_PSO
                switch (entry.sampler.type)
                {
                case WGPUSamplerBindingType_BindingNotUsed:
                    samplerType = TEXT("BindingNotUsed");
                    break;
                case WGPUSamplerBindingType_Undefined:
                    samplerType = TEXT("Undefined");
                    break;
                case WGPUSamplerBindingType_Filtering:
                    samplerType = TEXT("Filtering");
                    break;
                case WGPUSamplerBindingType_NonFiltering:
                    samplerType = TEXT("NonFiltering");
                    break;
                case WGPUSamplerBindingType_Comparison:
                    samplerType = TEXT("Comparison");
                    break;
                }
                if (log)
                    LOG(Info, "   > [{}] sampler ({})", entry.binding, samplerType);
#endif
                break;
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                entry.texture.sampleType = WGPUTextureSampleType_Undefined;
                if (shaderResources[descriptor.Slot])
                {
                    // Hack to use the sample type directly from the view which allows to fix incorrect Depth Buffer reading that allows only manual Load when UnfilterableFloat is used (see SAMPLE_RT_DEPTH)
                    auto ptr = (GPUResourceViewPtrWebGPU*)shaderResources[descriptor.Slot]->GetNativePtr();
                    if (ptr && ptr->TextureView)
                        entry.texture.sampleType = ptr->TextureView->SampleType;
                }
#if WEBGPU_LOG_PSO
                if (log)
                {
                    switch (entry.texture.sampleType)
                    {
                    case WGPUTextureSampleType_BindingNotUsed:
                        samplerType = TEXT("BindingNotUsed");
                        break;
                    case WGPUTextureSampleType_Undefined:
                        samplerType = TEXT("Undefined");
                        break;
                    case WGPUTextureSampleType_Float:
                        samplerType = TEXT("Float");
                        break;
                    case WGPUTextureSampleType_UnfilterableFloat:
                        samplerType = TEXT("UnfilterableFloat");
                        break;
                    case WGPUTextureSampleType_Depth:
                        samplerType = TEXT("Depth");
                        break;
                    case WGPUTextureSampleType_Sint:
                        samplerType = TEXT("Sint");
                        break;
                    case WGPUTextureSampleType_Uint:
                        samplerType = TEXT("Uint");
                        break;
                    }
                    switch (descriptor.ResourceType)
                    {
                    case SpirvShaderResourceType::Texture1D:
                        LOG(Info, "   > [{}] texture 1D ({})", entry.binding, samplerType);
                        break;
                    case SpirvShaderResourceType::Texture2D:
                        LOG(Info, "   > [{}] texture 2D ({})", entry.binding, samplerType);
                        break;
                    case SpirvShaderResourceType::Texture3D:
                        LOG(Info, "   > [{}] texture 3D ({})", entry.binding, samplerType);
                        break;
                    case SpirvShaderResourceType::TextureCube:
                        LOG(Info, "   > [{}] texture Cube ({})", entry.binding, samplerType);
                        break;
                    case SpirvShaderResourceType::Texture2DArray:
                        LOG(Info, "   > [{}] texture 2D array ({})", entry.binding, samplerType);
                        break;
                    }
                }
#endif
                switch (descriptor.ResourceType)
                {
                case SpirvShaderResourceType::Texture1D:
                    entry.texture.viewDimension = WGPUTextureViewDimension_1D;
                    break;
                case SpirvShaderResourceType::Texture2D:
                    entry.texture.viewDimension = WGPUTextureViewDimension_2D;
                    break;
                case SpirvShaderResourceType::Texture3D:
                    entry.texture.viewDimension = WGPUTextureViewDimension_3D;
                    break;
                case SpirvShaderResourceType::TextureCube:
                    entry.texture.viewDimension = WGPUTextureViewDimension_Cube;
                    break;
                case SpirvShaderResourceType::Texture1DArray:
                    CRASH; // Not supported TODO: add error at compile time (in ShaderCompilerWebGPU::Write)
                    break;
                case SpirvShaderResourceType::Texture2DArray:
                    entry.texture.viewDimension = WGPUTextureViewDimension_2DArray;
                    break;
                }
                break;
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                entry.buffer.hasDynamicOffset = true;
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                if (descriptor.BindingType == SpirvShaderResourceBindingType::SRV)
                    entry.buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
                else
                    entry.buffer.type = WGPUBufferBindingType_Storage;
#if WEBGPU_LOG_PSO
                if (log)
                    LOG(Info, "   > [{}] storage buffer (read-only = {}, dynamic = {})", entry.binding, entry.buffer.type == WGPUBufferBindingType_ReadOnlyStorage, entry.buffer.hasDynamicOffset);
#endif
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                entry.buffer.hasDynamicOffset = true;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                entry.buffer.type = WGPUBufferBindingType_Uniform;
#if WEBGPU_LOG_PSO
                if (log)
                    LOG(Info, "   > [{}] uniform buffer (dynamic = {})", entry.binding, entry.buffer.hasDynamicOffset);
#endif
                break;
            default:
#if GPU_ENABLE_DIAGNOSTICS
                LOG(Fatal, "Unknown descriptor type: {} used as {} in '{}'", (uint32)descriptor.DescriptorType, (uint32)descriptor.BindingType, String(_debugName.Get(), _debugName.Count() - 1));
#else
                CRASH;
#endif
                return;
            }
        }

        // Create a bind group layout
        WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
        bindGroupLayoutDesc.entryCount = entriesCount;
        bindGroupLayoutDesc.entries = entriesPtr;
        BindGroupLayouts[groupIndex] = wgpuDeviceCreateBindGroupLayout(_device->Device, &bindGroupLayoutDesc);
    }

    // Create the pipeline layout
    WGPUPipelineLayoutDescriptor layoutDesc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
#if GPU_ENABLE_RESOURCE_NAMING
    layoutDesc.label = PipelineDesc.label;
#endif
    layoutDesc.bindGroupLayoutCount = GPUBindGroupsWebGPU::GraphicsMax;
    layoutDesc.bindGroupLayouts = BindGroupLayouts;
    PipelineDesc.layout = wgpuDeviceCreatePipelineLayout(_device->Device, &layoutDesc);
    if (!PipelineDesc.layout)
    {
        LOG(Error, "wgpuDeviceCreatePipelineLayout failed");
        return;
    }
}

bool GPUPipelineStateWebGPU::IsValid() const
{
    return _memoryUsage != 0;
}

bool GPUPipelineStateWebGPU::Init(const Description& desc)
{
    if (IsValid())
        OnReleaseGPU();

    // Initialize description (without dynamic state from context such as render targets, vertex buffers, etc.)
    PipelineDesc = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
#if GPU_ENABLE_RESOURCE_NAMING
    DebugDesc = desc;
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
    if (desc.PS)
    {
        PipelineDesc.fragment = &_fragmentDesc;
        _fragmentDesc = WGPU_FRAGMENT_STATE_INIT;
        _fragmentDesc.targets = _colorTargets;
        Platform::MemoryClear(&_colorTargets, sizeof(_colorTargets));
        if (desc.BlendMode.BlendEnable)
        {
            _blendState = WGPU_BLEND_STATE_INIT;
            _blendState.color = ToBlendComponent(desc.BlendMode.BlendOp, desc.BlendMode.SrcBlend, desc.BlendMode.DestBlend);
            _blendState.alpha = ToBlendComponent(desc.BlendMode.BlendOpAlpha, desc.BlendMode.SrcBlendAlpha, desc.BlendMode.DestBlendAlpha);
            for (auto& e : _colorTargets)
                e.blend = &_blendState;
        }
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
        uint16 outputsCount = desc.PS->GetBindings().OutputsCount;
        for (uint16 rtIndex = 0; rtIndex < outputsCount; rtIndex++)
            _colorTargets[rtIndex].writeMask = writeMask;
    }

    // Cache shaders
    VS = (GPUShaderProgramVSWebGPU*)desc.VS;
    BindGroupDescriptors[GPUBindGroupsWebGPU::Vertex] = &VS->DescriptorInfo;
    PipelineDesc.vertex.module = VS->ShaderModule;
    PS = (GPUShaderProgramPSWebGPU*)desc.PS;
    if (PS)
    {
        BindGroupDescriptors[GPUBindGroupsWebGPU::Pixel] = &PS->DescriptorInfo;
        _fragmentDesc.module = PS->ShaderModule;
    }

    _memoryUsage = 1;
    return GPUPipelineState::Init(desc);
}

#endif
