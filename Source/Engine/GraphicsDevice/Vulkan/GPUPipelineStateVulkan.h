// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUPipelineState.h"
#include "GPUDeviceVulkan.h"
#include "DescriptorSetVulkan.h"
#include "CmdBufferVulkan.h"

#if GRAPHICS_API_VULKAN

class PipelineLayoutVulkan;

class ComputePipelineStateVulkan
{
private:
    GPUDeviceVulkan* _device;
    VkPipeline _handle;
    PipelineLayoutVulkan* _layout;

public:
    ComputePipelineStateVulkan(GPUDeviceVulkan* device, VkPipeline pipeline, PipelineLayoutVulkan* layout);
    ~ComputePipelineStateVulkan();

public:
    /// <summary>
    /// The cached shader descriptor infos for compute shader.
    /// </summary>
    const SpirvShaderDescriptorInfo* DescriptorInfo;

    DescriptorSetWriteContainerVulkan DSWriteContainer;
    DescriptorSetWriterVulkan DSWriter;

    const DescriptorSetLayoutVulkan* DescriptorSetsLayout = nullptr;
    TypedDescriptorPoolSetVulkan* CurrentTypedDescriptorPoolSet = nullptr;
    Array<VkDescriptorSet> DescriptorSetHandles;

    inline bool AcquirePoolSet(CmdBufferVulkan* cmdBuffer)
    {
        // Pipeline state has no current descriptor pools set or set owner is not current - acquire a new pool set
        DescriptorPoolSetContainerVulkan* cmdBufferPoolSet = cmdBuffer->GetDescriptorPoolSet();
        if (CurrentTypedDescriptorPoolSet == nullptr || CurrentTypedDescriptorPoolSet->GetOwner() != cmdBufferPoolSet)
        {
            if (CurrentTypedDescriptorPoolSet)
                CurrentTypedDescriptorPoolSet->GetOwner()->Refs--;
            CurrentTypedDescriptorPoolSet = cmdBufferPoolSet->AcquireTypedPoolSet(*DescriptorSetsLayout);
            CurrentTypedDescriptorPoolSet->GetOwner()->Refs++;
            return true;
        }
        return false;
    }

    inline bool AllocateDescriptorSets()
    {
        return CurrentTypedDescriptorPoolSet->AllocateDescriptorSets(*DescriptorSetsLayout, DescriptorSetHandles.Get());
    }

    Array<uint32> DynamicOffsets;

public:
    void Bind(CmdBufferVulkan* cmdBuffer)
    {
        vkCmdBindDescriptorSets(
            cmdBuffer->GetHandle(),
            VK_PIPELINE_BIND_POINT_COMPUTE,
            GetLayout()->Handle,
            0,
            DescriptorSetHandles.Count(),
            DescriptorSetHandles.Get(),
            DynamicOffsets.Count(),
            DynamicOffsets.Get());
    }

public:
    VkPipeline GetHandle() const
    {
        return _handle;
    }

    PipelineLayoutVulkan* GetLayout() const
    {
        return _layout;
    }
};

/// <summary>
/// Graphics pipeline state object for Vulkan backend.
/// </summary>
class GPUPipelineStateVulkan : public GPUResourceVulkan<GPUPipelineState>
{
private:
    Dictionary<RenderPassVulkan*, VkPipeline> _pipelines;
    VkGraphicsPipelineCreateInfo _desc;
    VkPipelineShaderStageCreateInfo _shaderStages[ShaderStage_Count - 1];
    VkPipelineInputAssemblyStateCreateInfo _descInputAssembly;
#if GPU_ALLOW_TESSELLATION_SHADERS
    VkPipelineTessellationStateCreateInfo _descTessellation;
#endif
    VkPipelineViewportStateCreateInfo _descViewport;
    VkPipelineDynamicStateCreateInfo _descDynamic;
    VkDynamicState _dynamicStates[4];
    VkPipelineMultisampleStateCreateInfo _descMultisample;
    VkPipelineDepthStencilStateCreateInfo _descDepthStencil;
    VkPipelineRasterizationStateCreateInfo _descRasterization;
    VkPipelineColorBlendStateCreateInfo _descColorBlend;
    VkPipelineColorBlendAttachmentState _descColorBlendAttachments[GPU_MAX_RT_BINDED];
    PipelineLayoutVulkan* _layout;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUPipelineStateVulkan"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    GPUPipelineStateVulkan(GPUDeviceVulkan* device);

public:
    /// <summary>
    /// The bitmask of stages that exist in this pipeline.
    /// </summary>
    uint32 UsedStagesMask;

    uint32 BlendEnable : 1;
    uint32 DepthReadEnable : 1;
    uint32 DepthWriteEnable : 1;
    uint32 StencilReadEnable : 1;
    uint32 StencilWriteEnable : 1;

    /// <summary>
    /// The bitmask of stages that have descriptors.
    /// </summary>
    uint32 HasDescriptorsPerStageMask;

    /// <summary>
    /// The cached shader bindings per stage.
    /// </summary>
    const ShaderBindings* ShaderBindingsPerStage[DescriptorSet::GraphicsStagesCount];

    /// <summary>
    /// The cached shader descriptor infos per stage.
    /// </summary>
    const SpirvShaderDescriptorInfo* DescriptorInfoPerStage[DescriptorSet::GraphicsStagesCount];

    const VkPipelineVertexInputStateCreateInfo* GetVertexInputState() const
    {
        return _desc.pVertexInputState;
    }

    DescriptorSetWriteContainerVulkan DSWriteContainer;
    DescriptorSetWriterVulkan DSWriter[DescriptorSet::GraphicsStagesCount];

    const DescriptorSetLayoutVulkan* DescriptorSetsLayout = nullptr;
    TypedDescriptorPoolSetVulkan* CurrentTypedDescriptorPoolSet = nullptr;
    Array<VkDescriptorSet> DescriptorSetHandles;

    Array<uint32> DynamicOffsets;

public:
    inline bool AcquirePoolSet(CmdBufferVulkan* cmdBuffer)
    {
        // Lazy init
        if (!DescriptorSetsLayout)
            GetLayout();

        // Pipeline state has no current descriptor pools set or set owner is not current - acquire a new pool set
        DescriptorPoolSetContainerVulkan* cmdBufferPoolSet = cmdBuffer->GetDescriptorPoolSet();
        if (CurrentTypedDescriptorPoolSet == nullptr || CurrentTypedDescriptorPoolSet->GetOwner() != cmdBufferPoolSet)
        {
            if (CurrentTypedDescriptorPoolSet)
                CurrentTypedDescriptorPoolSet->GetOwner()->Refs--;
            CurrentTypedDescriptorPoolSet = cmdBufferPoolSet->AcquireTypedPoolSet(*DescriptorSetsLayout);
            CurrentTypedDescriptorPoolSet->GetOwner()->Refs++;
            return true;
        }
        return false;
    }

    /// <summary>
    /// Gets the Vulkan pipeline layout for this pipeline state.
    /// </summary>
    /// <returns>The layout.</returns>
    PipelineLayoutVulkan* GetLayout();

    /// <summary>
    /// Gets the Vulkan graphics pipeline object for the given rendering state. Uses depth buffer and render targets formats and multi-sample levels to setup a proper PSO. Uses caching.
    /// </summary>
    /// <param name="renderPass">The render pass.</param>
    /// <returns>Vulkan graphics pipeline object.</returns>
    VkPipeline GetState(RenderPassVulkan* renderPass);

public:
    // [GPUPipelineState]
    bool IsValid() const final override;
    bool Init(const Description& desc) final override;

protected:
    // [GPUResourceVulkan]
    void OnReleaseGPU() override;
};

#endif
