// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN

#include "GPUPipelineStateVulkan.h"
#include "RenderToolsVulkan.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "DescriptorSetVulkan.h"
#include "GPUShaderProgramVulkan.h"

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
    desc.layout = layout->GetHandle();
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
    _pipelineState->DescriptorSetsLayout = &layout->GetDescriptorSetLayout();
    _pipelineState->DescriptorSetHandles.AddZeroed(_pipelineState->DescriptorSetsLayout->GetHandles().Count());
    uint32 dynamicOffsetsCount = 0;
    if (DescriptorInfo.DescriptorTypesCount != 0)
    {
        _pipelineState->DSWriteContainer.DescriptorWrites.AddZeroed(DescriptorInfo.DescriptorTypesCount);
        _pipelineState->DSWriteContainer.DescriptorImageInfo.AddZeroed(DescriptorInfo.ImageInfosCount);
        _pipelineState->DSWriteContainer.DescriptorBufferInfo.AddZeroed(DescriptorInfo.BufferInfosCount);

        ASSERT(DescriptorInfo.DescriptorTypesCount < 255);
        _pipelineState->DSWriteContainer.BindingToDynamicOffset.AddDefault(DescriptorInfo.DescriptorTypesCount);
        _pipelineState->DSWriteContainer.BindingToDynamicOffset.SetAll(255);

        VkWriteDescriptorSet* currentDescriptorWrite = _pipelineState->DSWriteContainer.DescriptorWrites.Get();
        VkDescriptorImageInfo* currentImageInfo = _pipelineState->DSWriteContainer.DescriptorImageInfo.Get();
        VkDescriptorBufferInfo* currentBufferInfo = _pipelineState->DSWriteContainer.DescriptorBufferInfo.Get();
        uint8* currentBindingToDynamicOffsetMap = _pipelineState->DSWriteContainer.BindingToDynamicOffset.Get();

        dynamicOffsetsCount = _pipelineState->DSWriter.SetupDescriptorWrites(DescriptorInfo, currentDescriptorWrite, currentImageInfo, currentBufferInfo, currentBindingToDynamicOffsetMap);
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
    CurrentTypedDescriptorPoolSet = nullptr;
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
	{ \
		descriptorSetLayoutInfo.AddBindingsForStage(bit, DescriptorSet::set, DescriptorInfoPerStage[DescriptorSet::set]); \
	}
    INIT_SHADER_STAGE(Vertex, VK_SHADER_STAGE_VERTEX_BIT);
    INIT_SHADER_STAGE(Hull, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
    INIT_SHADER_STAGE(Domain, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
    INIT_SHADER_STAGE(Geometry, VK_SHADER_STAGE_GEOMETRY_BIT);
    INIT_SHADER_STAGE(Pixel, VK_SHADER_STAGE_FRAGMENT_BIT);
#undef INIT_SHADER_STAGE

    _layout = _device->GetOrCreateLayout(descriptorSetLayoutInfo);
    ASSERT(_layout);
    DescriptorSetsLayout = &_layout->GetDescriptorSetLayout();
    DescriptorSetHandles.AddZeroed(DescriptorSetsLayout->GetHandles().Count());

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
    _desc.renderPass = renderPass->GetHandle();

    // Check if has missing layout
    if (_desc.layout == VK_NULL_HANDLE)
    {
        _desc.layout = GetLayout()->GetHandle();
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
    CurrentTypedDescriptorPoolSet = nullptr;
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
    INIT_SHADER_STAGE(HS, Hull, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
    INIT_SHADER_STAGE(DS, Domain, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
    INIT_SHADER_STAGE(GS, Geometry, VK_SHADER_STAGE_GEOMETRY_BIT);
    INIT_SHADER_STAGE(PS, Pixel, VK_SHADER_STAGE_FRAGMENT_BIT);
#undef INIT_SHADER_STAGE
    _desc.pStages = _shaderStages;

    // Input Assembly
    RenderToolsVulkan::ZeroStruct(_descInputAssembly, VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);;
    switch (desc.PrimitiveTopologyType)
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
    if (desc.HS)
        _descInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
    _desc.pInputAssemblyState = &_descInputAssembly;

    // Tessellation
    if (desc.HS)
    {
        RenderToolsVulkan::ZeroStruct(_descTessellation, VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO);
        _descTessellation.patchControlPoints = desc.HS->GetControlPointsCount();
        _desc.pTessellationState = &_descTessellation;
    }

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
    //_dynamicStates[_descDynamic.dynamicStateCount++] = VK_DYNAMIC_STATE_STENCIL_REFERENCE;
    static_assert(ARRAY_COUNT(_dynamicStates) <= 3, "Invalid dynamic states array.");
    _desc.pDynamicState = &_descDynamic;

    // Multisample
    RenderToolsVulkan::ZeroStruct(_descMultisample, VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
    _descMultisample.minSampleShading = 1.0f;
    _descMultisample.alphaToCoverageEnable = desc.BlendMode.AlphaToCoverageEnable;
    _desc.pMultisampleState = &_descMultisample;

    // Depth Stencil
    RenderToolsVulkan::ZeroStruct(_descDepthStencil, VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO);
    _descDepthStencil.depthTestEnable = desc.DepthTestEnable;
    _descDepthStencil.depthWriteEnable = desc.DepthWriteEnable;
    _descDepthStencil.depthCompareOp = RenderToolsVulkan::ToVulkanCompareOp(desc.DepthFunc);
    _desc.pDepthStencilState = &_descDepthStencil;

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
    _descRasterization.depthClampEnable = !desc.DepthClipEnable;
    _descRasterization.lineWidth = 1.0f;
    _desc.pRasterizationState = &_descRasterization;

    // Color Blend State
    BlendEnable = desc.BlendMode.BlendEnable;
    RenderToolsVulkan::ZeroStruct(_descColorBlend, VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
    {
        auto& blend = _descColorBlendAttachments[0];
        blend.blendEnable = desc.BlendMode.BlendEnable;
        blend.srcColorBlendFactor = RenderToolsVulkan::ToVulkanBlendFactor(desc.BlendMode.SrcBlend);
        blend.dstColorBlendFactor = RenderToolsVulkan::ToVulkanBlendFactor(desc.BlendMode.DestBlend);
        blend.colorBlendOp = RenderToolsVulkan::ToVulkanBlendOp(desc.BlendMode.BlendOp);
        blend.srcAlphaBlendFactor = RenderToolsVulkan::ToVulkanBlendFactor(desc.BlendMode.SrcBlendAlpha);
        blend.dstAlphaBlendFactor = RenderToolsVulkan::ToVulkanBlendFactor(desc.BlendMode.DestBlendAlpha);
        blend.alphaBlendOp = RenderToolsVulkan::ToVulkanBlendOp(desc.BlendMode.BlendOpAlpha);
        blend.colorWriteMask = (VkColorComponentFlags)desc.BlendMode.RenderTargetWriteMask;
    }
    for (int32 i = 1; i < GPU_MAX_RT_BINDED; i++)
        _descColorBlendAttachments[i] = _descColorBlendAttachments[i - 1];
    _descColorBlend.pAttachments = _descColorBlendAttachments;
    _descColorBlend.blendConstants[0] = 0.0f;
    _descColorBlend.blendConstants[1] = 0.0f;
    _descColorBlend.blendConstants[2] = 0.0f;
    _descColorBlend.blendConstants[3] = 0.0f;
    _desc.pColorBlendState = &_descColorBlend;

    ASSERT(DSWriteContainer.DescriptorWrites.IsEmpty());
    for (int32 stage = 0; stage < DescriptorSet::GraphicsStagesCount; stage++)
    {
        const auto descriptor = DescriptorInfoPerStage[stage];
        if (descriptor == nullptr || descriptor->DescriptorTypesCount == 0)
            continue;

        DSWriteContainer.DescriptorWrites.AddZeroed(descriptor->DescriptorTypesCount);
        DSWriteContainer.DescriptorImageInfo.AddZeroed(descriptor->ImageInfosCount);
        DSWriteContainer.DescriptorBufferInfo.AddZeroed(descriptor->BufferInfosCount);

        ASSERT(descriptor->DescriptorTypesCount < 255);
        DSWriteContainer.BindingToDynamicOffset.AddDefault(descriptor->DescriptorTypesCount);
        DSWriteContainer.BindingToDynamicOffset.SetAll(255);
    }

    VkWriteDescriptorSet* currentDescriptorWrite = DSWriteContainer.DescriptorWrites.Get();
    VkDescriptorImageInfo* currentImageInfo = DSWriteContainer.DescriptorImageInfo.Get();
    VkDescriptorBufferInfo* currentBufferInfo = DSWriteContainer.DescriptorBufferInfo.Get();
    byte* currentBindingToDynamicOffsetMap = DSWriteContainer.BindingToDynamicOffset.Get();
    uint32 dynamicOffsetsStart[DescriptorSet::GraphicsStagesCount];
    uint32 dynamicOffsetsCount = 0;
    for (int32 stage = 0; stage < DescriptorSet::GraphicsStagesCount; stage++)
    {
        dynamicOffsetsStart[stage] = dynamicOffsetsCount;

        const auto descriptor = DescriptorInfoPerStage[stage];
        if (descriptor == nullptr || descriptor->DescriptorTypesCount == 0)
            continue;

        const uint32 numDynamicOffsets = DSWriter[stage].SetupDescriptorWrites(*descriptor, currentDescriptorWrite, currentImageInfo, currentBufferInfo, currentBindingToDynamicOffsetMap);
        dynamicOffsetsCount += numDynamicOffsets;

        currentDescriptorWrite += descriptor->DescriptorTypesCount;
        currentImageInfo += descriptor->ImageInfosCount;
        currentBufferInfo += descriptor->BufferInfosCount;
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
