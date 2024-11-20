// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN

#include "GPUPipelineStateVulkan.h"
#include "RenderToolsVulkan.h"
#include "DescriptorSetVulkan.h"
#include "GPUShaderProgramVulkan.h"
#include "Engine/Core/Log.h"
#include "Engine/Profiler/ProfilerCPU.h"

static VkStencilOp ToVulkanStencilOp(const StencilOperation value)
{
    switch (value)
    {
    case StencilOperation::Keep:
        return VK_STENCIL_OP_KEEP;
    case StencilOperation::Zero:
        return VK_STENCIL_OP_ZERO;
    case StencilOperation::Replace:
        return VK_STENCIL_OP_REPLACE;
    case StencilOperation::IncrementSaturated:
        return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
    case StencilOperation::DecrementSaturated:
        return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
    case StencilOperation::Invert:
        return VK_STENCIL_OP_INVERT;
    case StencilOperation::Increment:
        return VK_STENCIL_OP_INCREMENT_AND_WRAP;
    case StencilOperation::Decrement:
        return VK_STENCIL_OP_DECREMENT_AND_WRAP;
    default:
        return VK_STENCIL_OP_KEEP;
    }
}

static VkBlendFactor ToVulkanBlendFactor(const BlendingMode::Blend value)
{
    switch (value)
    {
    case BlendingMode::Blend::Zero:
        return VK_BLEND_FACTOR_ZERO;
    case BlendingMode::Blend::One:
        return VK_BLEND_FACTOR_ONE;
    case BlendingMode::Blend::SrcColor:
        return VK_BLEND_FACTOR_SRC_COLOR;
    case BlendingMode::Blend::InvSrcColor:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case BlendingMode::Blend::SrcAlpha:
        return VK_BLEND_FACTOR_SRC_ALPHA;
    case BlendingMode::Blend::InvSrcAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case BlendingMode::Blend::DestAlpha:
        return VK_BLEND_FACTOR_DST_ALPHA;
    case BlendingMode::Blend::InvDestAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    case BlendingMode::Blend::DestColor:
        return VK_BLEND_FACTOR_DST_COLOR;
    case BlendingMode::Blend::InvDestColor:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    case BlendingMode::Blend::SrcAlphaSat:
        return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
    case BlendingMode::Blend::BlendFactor:
        return VK_BLEND_FACTOR_CONSTANT_COLOR;
    case BlendingMode::Blend::BlendInvFactor:
        return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
    case BlendingMode::Blend::Src1Color:
        return VK_BLEND_FACTOR_SRC1_COLOR;
    case BlendingMode::Blend::InvSrc1Color:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
    case BlendingMode::Blend::Src1Alpha:
        return VK_BLEND_FACTOR_SRC1_ALPHA;
    case BlendingMode::Blend::InvSrc1Alpha:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
    default:
        return VK_BLEND_FACTOR_ZERO;
    }
}

GPUShaderProgramCSVulkan::~GPUShaderProgramCSVulkan()
{
    if (_pipelineState)
        Delete(_pipelineState);
}

ComputePipelineStateVulkan* GPUShaderProgramCSVulkan::GetOrCreateState()
{
    if (_pipelineState)
        return _pipelineState;

    // Create pipeline layout
    DescriptorSetLayoutInfoVulkan descriptorSetLayoutInfo;
    descriptorSetLayoutInfo.AddBindingsForStage(VK_SHADER_STAGE_COMPUTE_BIT, DescriptorSet::Compute, &DescriptorInfo);
    const auto layout = _device->GetOrCreateLayout(descriptorSetLayoutInfo);

    // Create pipeline description
    VkComputePipelineCreateInfo desc;
    RenderToolsVulkan::ZeroStruct(desc, VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO);
    desc.basePipelineIndex = -1;
    desc.layout = layout->Handle;
    RenderToolsVulkan::ZeroStruct(desc.stage, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
    auto& stage = desc.stage;
    RenderToolsVulkan::ZeroStruct(stage, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module = (VkShaderModule)GetBufferHandle();
    stage.pName = GetName().Get();

    // Create pipeline object
    VkPipeline pipeline;
    const VkResult result = vkCreateComputePipelines(_device->Device, _device->PipelineCache, 1, &desc, nullptr, &pipeline);
    LOG_VULKAN_RESULT(result);
    if (result != VK_SUCCESS)
        return nullptr;

    // Setup the state
    _pipelineState = New<ComputePipelineStateVulkan>(_device, pipeline, layout);
    _pipelineState->DescriptorInfo = &DescriptorInfo;
    _pipelineState->DescriptorSetsLayout = &layout->DescriptorSetLayout;
    _pipelineState->DescriptorSetHandles.AddZeroed(_pipelineState->DescriptorSetsLayout->Handles.Count());
    uint32 dynamicOffsetsCount = 0;
    if (DescriptorInfo.DescriptorTypesCount != 0)
    {
        // TODO: merge into a single allocation
        _pipelineState->DSWriteContainer.DescriptorWrites.AddZeroed(DescriptorInfo.DescriptorTypesCount);
        _pipelineState->DSWriteContainer.DescriptorImageInfo.AddZeroed(DescriptorInfo.ImageInfosCount);
        _pipelineState->DSWriteContainer.DescriptorBufferInfo.AddZeroed(DescriptorInfo.BufferInfosCount);
        _pipelineState->DSWriteContainer.DescriptorTexelBufferView.AddZeroed(DescriptorInfo.TexelBufferViewsCount);

        ASSERT(DescriptorInfo.DescriptorTypesCount < 255);
        _pipelineState->DSWriteContainer.BindingToDynamicOffset.AddDefault(DescriptorInfo.DescriptorTypesCount);
        _pipelineState->DSWriteContainer.BindingToDynamicOffset.SetAll(255);

        VkWriteDescriptorSet* currentDescriptorWrite = _pipelineState->DSWriteContainer.DescriptorWrites.Get();
        VkDescriptorImageInfo* currentImageInfo = _pipelineState->DSWriteContainer.DescriptorImageInfo.Get();
        VkDescriptorBufferInfo* currentBufferInfo = _pipelineState->DSWriteContainer.DescriptorBufferInfo.Get();
        VkBufferView* currentTexelBufferView = _pipelineState->DSWriteContainer.DescriptorTexelBufferView.Get();
        uint8* currentBindingToDynamicOffsetMap = _pipelineState->DSWriteContainer.BindingToDynamicOffset.Get();

        dynamicOffsetsCount = _pipelineState->DSWriter.SetupDescriptorWrites(DescriptorInfo, currentDescriptorWrite, currentImageInfo, currentBufferInfo, currentTexelBufferView, currentBindingToDynamicOffsetMap);
    }

    _pipelineState->DynamicOffsets.AddZeroed(dynamicOffsetsCount);
    _pipelineState->DSWriter.DynamicOffsets = _pipelineState->DynamicOffsets.Get();

    return _pipelineState;
}

ComputePipelineStateVulkan::ComputePipelineStateVulkan(GPUDeviceVulkan* device, VkPipeline pipeline, PipelineLayoutVulkan* layout)
    : _device(device)
    , _handle(pipeline)
    , _layout(layout)
{
}

ComputePipelineStateVulkan::~ComputePipelineStateVulkan()
{
    DSWriteContainer.Release();
    if (CurrentTypedDescriptorPoolSet)
    {
        CurrentTypedDescriptorPoolSet->GetOwner()->Refs--;
        CurrentTypedDescriptorPoolSet = nullptr;
    }
    DescriptorSetsLayout = nullptr;
    DescriptorSetHandles.Resize(0);
    DynamicOffsets.Resize(0);
    _device->DeferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::Pipeline, _handle);
    _layout = nullptr;
}

GPUPipelineStateVulkan::GPUPipelineStateVulkan(GPUDeviceVulkan* device)
    : GPUResourceVulkan<GPUPipelineState>(device, StringView::Empty)
    , _pipelines(16)
    , _layout(nullptr)
{
}

PipelineLayoutVulkan* GPUPipelineStateVulkan::GetLayout()
{
    if (_layout)
        return _layout;

    DescriptorSetLayoutInfoVulkan descriptorSetLayoutInfo;
#define INIT_SHADER_STAGE(set, bit) \
	if (DescriptorInfoPerStage[DescriptorSet::set]) \
		descriptorSetLayoutInfo.AddBindingsForStage(bit, DescriptorSet::set, DescriptorInfoPerStage[DescriptorSet::set])
    INIT_SHADER_STAGE(Vertex, VK_SHADER_STAGE_VERTEX_BIT);
#if GPU_ALLOW_TESSELLATION_SHADERS
    INIT_SHADER_STAGE(Hull, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
    INIT_SHADER_STAGE(Domain, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
#endif
#if GPU_ALLOW_GEOMETRY_SHADERS
    INIT_SHADER_STAGE(Geometry, VK_SHADER_STAGE_GEOMETRY_BIT);
#endif
    INIT_SHADER_STAGE(Pixel, VK_SHADER_STAGE_FRAGMENT_BIT);
#undef INIT_SHADER_STAGE
    _layout = _device->GetOrCreateLayout(descriptorSetLayoutInfo);
    ASSERT(_layout);
    DescriptorSetsLayout = &_layout->DescriptorSetLayout;
    DescriptorSetHandles.AddZeroed(DescriptorSetsLayout->Handles.Count());

    return _layout;
}

VkPipeline GPUPipelineStateVulkan::GetState(RenderPassVulkan* renderPass)
{
    ASSERT(renderPass);

    // Try reuse cached version
    VkPipeline pipeline = VK_NULL_HANDLE;
    if (_pipelines.TryGet(renderPass, pipeline))
    {
#if BUILD_DEBUG
        // Verify
        RenderPassVulkan* refKey = nullptr;
        _pipelines.KeyOf(pipeline, &refKey);
        ASSERT(refKey == renderPass);
#endif
        return pipeline;
    }

    PROFILE_CPU_NAMED("Create Pipeline");

    // Update description to match the pipeline
    _descColorBlend.attachmentCount = renderPass->Layout.RTsCount;
    _descMultisample.rasterizationSamples = (VkSampleCountFlagBits)renderPass->Layout.MSAA;
    _desc.renderPass = renderPass->Handle;

    // Check if has missing layout
    if (_desc.layout == VK_NULL_HANDLE)
    {
        _desc.layout = GetLayout()->Handle;
    }

    // Create object
    const VkResult result = vkCreateGraphicsPipelines(_device->Device, _device->PipelineCache, 1, &_desc, nullptr, &pipeline);
    LOG_VULKAN_RESULT(result);
    if (result != VK_SUCCESS)
    {
#if BUILD_DEBUG
        const StringAnsi vsName = DebugDesc.VS ? DebugDesc.VS->GetName() : StringAnsi::Empty;
        const StringAnsi psName = DebugDesc.PS ? DebugDesc.PS->GetName() : StringAnsi::Empty;
        LOG(Error, "vkCreateGraphicsPipelines failed for VS={0}, PS={1}", String(vsName), String(psName));
#endif
        return VK_NULL_HANDLE;
    }

    // Cache it
    _pipelines.Add(renderPass, pipeline);

    return pipeline;
}

void GPUPipelineStateVulkan::OnReleaseGPU()
{
    DSWriteContainer.Release();
    if (CurrentTypedDescriptorPoolSet)
    {
        CurrentTypedDescriptorPoolSet->GetOwner()->Refs--;
        CurrentTypedDescriptorPoolSet = nullptr;
    }
    DescriptorSetsLayout = nullptr;
    DescriptorSetHandles.Resize(0);
    DynamicOffsets.Resize(0);
    for (auto i = _pipelines.Begin(); i.IsNotEnd(); ++i)
    {
        _device->DeferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::Pipeline, i->Value);
    }
    _layout = nullptr;
    _pipelines.Clear();
}

bool GPUPipelineStateVulkan::IsValid() const
{
    return _memoryUsage != 0;
}

bool GPUPipelineStateVulkan::Init(const Description& desc)
{
    ASSERT(!IsValid());

    // Create description
    RenderToolsVulkan::ZeroStruct(_desc, VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);

    // Vertex Input
    _desc.pVertexInputState = (VkPipelineVertexInputStateCreateInfo*)desc.VS->GetInputLayout();

    // Stages
    UsedStagesMask = 0;
    HasDescriptorsPerStageMask = 0;
    Platform::MemoryClear(ShaderBindingsPerStage, sizeof(ShaderBindingsPerStage));
    Platform::MemoryClear(DescriptorInfoPerStage, sizeof(DescriptorInfoPerStage));
#define INIT_SHADER_STAGE(type, set, bit) \
	if(desc.type) \
	{ \
		int32 stageIndex = (int32)DescriptorSet::set; \
		UsedStagesMask |= (1 << stageIndex); \
		auto bindings = &desc.type->GetBindings(); \
		if (bindings->UsedCBsMask + bindings->UsedSRsMask + bindings->UsedUAsMask) \
			HasDescriptorsPerStageMask |= (1 << stageIndex); \
		ShaderBindingsPerStage[stageIndex] = bindings; \
		DescriptorInfoPerStage[stageIndex] = &((GPUShaderProgram##type##Vulkan*)desc.type)->DescriptorInfo; \
		auto& stage = _shaderStages[_desc.stageCount++]; \
		RenderToolsVulkan::ZeroStruct(stage, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO); \
		stage.stage = bit; \
		stage.module = (VkShaderModule)desc.type->GetBufferHandle(); \
		stage.pName = desc.type->GetName().Get(); \
	}
    INIT_SHADER_STAGE(VS, Vertex, VK_SHADER_STAGE_VERTEX_BIT);
#if GPU_ALLOW_TESSELLATION_SHADERS
    INIT_SHADER_STAGE(HS, Hull, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
    INIT_SHADER_STAGE(DS, Domain, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
#endif
#if GPU_ALLOW_GEOMETRY_SHADERS
    INIT_SHADER_STAGE(GS, Geometry, VK_SHADER_STAGE_GEOMETRY_BIT);
#endif
    INIT_SHADER_STAGE(PS, Pixel, VK_SHADER_STAGE_FRAGMENT_BIT);
#undef INIT_SHADER_STAGE
    _desc.pStages = _shaderStages;

    // Input Assembly
    RenderToolsVulkan::ZeroStruct(_descInputAssembly, VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);;
    switch (desc.PrimitiveTopology)
    {
    case PrimitiveTopologyType::Point:
        _descInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        break;
    case PrimitiveTopologyType::Line:
        _descInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        break;
    case PrimitiveTopologyType::Triangle:
        _descInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    }
#if GPU_ALLOW_TESSELLATION_SHADERS
    if (desc.HS)
        _descInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
#endif
    _desc.pInputAssemblyState = &_descInputAssembly;

#if GPU_ALLOW_TESSELLATION_SHADERS
    // Tessellation
    if (desc.HS)
    {
        RenderToolsVulkan::ZeroStruct(_descTessellation, VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO);
        _descTessellation.patchControlPoints = desc.HS->GetControlPointsCount();
        _desc.pTessellationState = &_descTessellation;
    }
#endif

    // Viewport
    RenderToolsVulkan::ZeroStruct(_descViewport, VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);
    _descViewport.viewportCount = 1;
    _descViewport.scissorCount = 1;
    _desc.pViewportState = &_descViewport;

    // Dynamic
    RenderToolsVulkan::ZeroStruct(_descDynamic, VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);
    _descDynamic.pDynamicStates = _dynamicStates;
    _dynamicStates[_descDynamic.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    _dynamicStates[_descDynamic.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
    _dynamicStates[_descDynamic.dynamicStateCount++] = VK_DYNAMIC_STATE_STENCIL_REFERENCE;
#define IsBlendUsingBlendFactor(blend) blend == BlendingMode::Blend::BlendFactor || blend == BlendingMode::Blend::BlendInvFactor
    if (desc.BlendMode.BlendEnable && (
        IsBlendUsingBlendFactor(desc.BlendMode.SrcBlend) || IsBlendUsingBlendFactor(desc.BlendMode.SrcBlendAlpha) ||
        IsBlendUsingBlendFactor(desc.BlendMode.DestBlend) || IsBlendUsingBlendFactor(desc.BlendMode.DestBlendAlpha)))
        _dynamicStates[_descDynamic.dynamicStateCount++] = VK_DYNAMIC_STATE_BLEND_CONSTANTS;
#undef IsBlendUsingBlendFactor
    static_assert(ARRAY_COUNT(_dynamicStates) <= 4, "Invalid dynamic states array.");
    _desc.pDynamicState = &_descDynamic;

    // Multisample
    RenderToolsVulkan::ZeroStruct(_descMultisample, VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
    _descMultisample.minSampleShading = 1.0f;
    _descMultisample.alphaToCoverageEnable = desc.BlendMode.AlphaToCoverageEnable;
    _desc.pMultisampleState = &_descMultisample;

    // Depth Stencil
    RenderToolsVulkan::ZeroStruct(_descDepthStencil, VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO);
    _descDepthStencil.depthTestEnable = desc.DepthEnable;
    _descDepthStencil.depthWriteEnable = desc.DepthWriteEnable;
    _descDepthStencil.depthCompareOp = RenderToolsVulkan::ToVulkanCompareOp(desc.DepthFunc);
    _descDepthStencil.stencilTestEnable = desc.StencilEnable;
    _descDepthStencil.front.compareMask = desc.StencilReadMask;
    _descDepthStencil.front.writeMask = desc.StencilWriteMask;
    _descDepthStencil.front.compareOp = RenderToolsVulkan::ToVulkanCompareOp(desc.StencilFunc);
    _descDepthStencil.front.failOp = ToVulkanStencilOp(desc.StencilFailOp);
    _descDepthStencil.front.depthFailOp = ToVulkanStencilOp(desc.StencilDepthFailOp);
    _descDepthStencil.front.passOp = ToVulkanStencilOp(desc.StencilPassOp);
    _descDepthStencil.back = _descDepthStencil.front;
    _desc.pDepthStencilState = &_descDepthStencil;
    DepthReadEnable = desc.DepthEnable && desc.DepthFunc != ComparisonFunc::Always;
    DepthWriteEnable = _descDepthStencil.depthWriteEnable;
    StencilReadEnable = desc.StencilEnable && desc.StencilReadMask != 0 && desc.StencilFunc != ComparisonFunc::Always;
    StencilWriteEnable = desc.StencilEnable && desc.StencilWriteMask != 0;

    // Rasterization
    RenderToolsVulkan::ZeroStruct(_descRasterization, VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
    _descRasterization.polygonMode = desc.Wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    switch (desc.CullMode)
    {
    case CullMode::Normal:
        _descRasterization.cullMode = VK_CULL_MODE_BACK_BIT;
        break;
    case CullMode::Inverted:
        _descRasterization.cullMode = VK_CULL_MODE_FRONT_BIT;
        break;
    case CullMode::TwoSided:
        _descRasterization.cullMode = VK_CULL_MODE_NONE;
        break;
    }
    _descRasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
    _descRasterization.depthClampEnable = !desc.DepthClipEnable && _device->Limits.HasDepthClip;
    _descRasterization.lineWidth = 1.0f;
    _desc.pRasterizationState = &_descRasterization;

    // Color Blend State
    BlendEnable = desc.BlendMode.BlendEnable;
    RenderToolsVulkan::ZeroStruct(_descColorBlend, VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
    {
        auto& blend = _descColorBlendAttachments[0];
        blend.blendEnable = desc.BlendMode.BlendEnable;
        blend.srcColorBlendFactor = ToVulkanBlendFactor(desc.BlendMode.SrcBlend);
        blend.dstColorBlendFactor = ToVulkanBlendFactor(desc.BlendMode.DestBlend);
        blend.colorBlendOp = RenderToolsVulkan::ToVulkanBlendOp(desc.BlendMode.BlendOp);
        blend.srcAlphaBlendFactor = ToVulkanBlendFactor(desc.BlendMode.SrcBlendAlpha);
        blend.dstAlphaBlendFactor = ToVulkanBlendFactor(desc.BlendMode.DestBlendAlpha);
        blend.alphaBlendOp = RenderToolsVulkan::ToVulkanBlendOp(desc.BlendMode.BlendOpAlpha);
        blend.colorWriteMask = (VkColorComponentFlags)desc.BlendMode.RenderTargetWriteMask;
    }
    for (int32 i = 1; i < GPU_MAX_RT_BINDED; i++)
        _descColorBlendAttachments[i] = _descColorBlendAttachments[i - 1];
    _descColorBlend.pAttachments = _descColorBlendAttachments;
    _descColorBlend.blendConstants[0] = 1.0f;
    _descColorBlend.blendConstants[1] = 1.0f;
    _descColorBlend.blendConstants[2] = 1.0f;
    _descColorBlend.blendConstants[3] = 1.0f;
    _desc.pColorBlendState = &_descColorBlend;

    ASSERT(DSWriteContainer.DescriptorWrites.IsEmpty());
    for (int32 stage = 0; stage < DescriptorSet::GraphicsStagesCount; stage++)
    {
        const auto descriptor = DescriptorInfoPerStage[stage];
        if (descriptor == nullptr || descriptor->DescriptorTypesCount == 0)
            continue;

        // TODO: merge into a single allocation for a whole PSO
        DSWriteContainer.DescriptorWrites.AddZeroed(descriptor->DescriptorTypesCount);
        DSWriteContainer.DescriptorImageInfo.AddZeroed(descriptor->ImageInfosCount);
        DSWriteContainer.DescriptorBufferInfo.AddZeroed(descriptor->BufferInfosCount);
        DSWriteContainer.DescriptorTexelBufferView.AddZeroed(descriptor->TexelBufferViewsCount);

        ASSERT(descriptor->DescriptorTypesCount < 255);
        DSWriteContainer.BindingToDynamicOffset.AddDefault(descriptor->DescriptorTypesCount);
        DSWriteContainer.BindingToDynamicOffset.SetAll(255);
    }

    VkWriteDescriptorSet* currentDescriptorWrite = DSWriteContainer.DescriptorWrites.Get();
    VkDescriptorImageInfo* currentImageInfo = DSWriteContainer.DescriptorImageInfo.Get();
    VkDescriptorBufferInfo* currentBufferInfo = DSWriteContainer.DescriptorBufferInfo.Get();
    VkBufferView* currentTexelBufferView = DSWriteContainer.DescriptorTexelBufferView.Get();
    byte* currentBindingToDynamicOffsetMap = DSWriteContainer.BindingToDynamicOffset.Get();
    uint32 dynamicOffsetsStart[DescriptorSet::GraphicsStagesCount];
    uint32 dynamicOffsetsCount = 0;
    for (int32 stage = 0; stage < DescriptorSet::GraphicsStagesCount; stage++)
    {
        dynamicOffsetsStart[stage] = dynamicOffsetsCount;

        const auto descriptor = DescriptorInfoPerStage[stage];
        if (descriptor == nullptr || descriptor->DescriptorTypesCount == 0)
            continue;

        const uint32 numDynamicOffsets = DSWriter[stage].SetupDescriptorWrites(*descriptor, currentDescriptorWrite, currentImageInfo, currentBufferInfo, currentTexelBufferView, currentBindingToDynamicOffsetMap);
        dynamicOffsetsCount += numDynamicOffsets;

        currentDescriptorWrite += descriptor->DescriptorTypesCount;
        currentImageInfo += descriptor->ImageInfosCount;
        currentBufferInfo += descriptor->BufferInfosCount;
        currentTexelBufferView += descriptor->TexelBufferViewsCount;
        currentBindingToDynamicOffsetMap += descriptor->DescriptorTypesCount;
    }

    DynamicOffsets.AddZeroed(dynamicOffsetsCount);
    for (int32 stage = 0; stage < DescriptorSet::GraphicsStagesCount; stage++)
    {
        DSWriter[stage].DynamicOffsets = dynamicOffsetsStart[stage] + DynamicOffsets.Get();
    }

    // Set non-zero memory usage
    _memoryUsage = sizeof(VkGraphicsPipelineCreateInfo);

    return GPUPipelineState::Init(desc);
}

#endif
