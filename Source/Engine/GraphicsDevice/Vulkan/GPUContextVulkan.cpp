// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN

#include "GPUContextVulkan.h"
#include "CmdBufferVulkan.h"
#include "RenderToolsVulkan.h"
#include "Engine/Core/Math/Color.h"
#include "GPUBufferVulkan.h"
#include "GPUShaderVulkan.h"
#include "GPUPipelineStateVulkan.h"
#include "Engine/Profiler/RenderStats.h"
#include "GPUShaderProgramVulkan.h"
#include "GPUTextureVulkan.h"
#include "Engine/Core/Math/Rectangle.h"

// Ensure to match the indirect commands arguments layout
static_assert(sizeof(GPUDispatchIndirectArgs) == sizeof(VkDispatchIndirectCommand), "Wrong size of GPUDrawIndirectArgs.");
static_assert(OFFSET_OF(GPUDispatchIndirectArgs, ThreadGroupCountX) == OFFSET_OF(VkDispatchIndirectCommand, x), "Wrong offset for GPUDrawIndirectArgs::ThreadGroupCountX");
static_assert(OFFSET_OF(GPUDispatchIndirectArgs, ThreadGroupCountY) == OFFSET_OF(VkDispatchIndirectCommand, y),"Wrong offset for GPUDrawIndirectArgs::ThreadGroupCountY");
static_assert(OFFSET_OF(GPUDispatchIndirectArgs, ThreadGroupCountZ) == OFFSET_OF(VkDispatchIndirectCommand, z), "Wrong offset for GPUDrawIndirectArgs::ThreadGroupCountZ");
//
static_assert(sizeof(GPUDrawIndirectArgs) == sizeof(VkDrawIndirectCommand), "Wrong size of GPUDrawIndirectArgs.");
static_assert(OFFSET_OF(GPUDrawIndirectArgs, VerticesCount) == OFFSET_OF(VkDrawIndirectCommand, vertexCount), "Wrong offset for GPUDrawIndirectArgs::VerticesCount");
static_assert(OFFSET_OF(GPUDrawIndirectArgs, InstanceCount) == OFFSET_OF(VkDrawIndirectCommand, instanceCount),"Wrong offset for GPUDrawIndirectArgs::InstanceCount");
static_assert(OFFSET_OF(GPUDrawIndirectArgs, StartVertex) == OFFSET_OF(VkDrawIndirectCommand, firstVertex), "Wrong offset for GPUDrawIndirectArgs::StartVertex");
static_assert(OFFSET_OF(GPUDrawIndirectArgs, StartInstance) == OFFSET_OF(VkDrawIndirectCommand, firstInstance), "Wrong offset for GPUDrawIndirectArgs::StartInstance");
//
static_assert(sizeof(GPUDrawIndexedIndirectArgs) == sizeof(VkDrawIndexedIndirectCommand), "Wrong size of GPUDrawIndexedIndirectArgs.");
static_assert(OFFSET_OF(GPUDrawIndexedIndirectArgs, IndicesCount) == OFFSET_OF(VkDrawIndexedIndirectCommand, indexCount), "Wrong offset for GPUDrawIndexedIndirectArgs::IndicesCount");
static_assert(OFFSET_OF(GPUDrawIndexedIndirectArgs, InstanceCount) == OFFSET_OF(VkDrawIndexedIndirectCommand, instanceCount),"Wrong offset for GPUDrawIndexedIndirectArgs::InstanceCount");
static_assert(OFFSET_OF(GPUDrawIndexedIndirectArgs, StartIndex) == OFFSET_OF(VkDrawIndexedIndirectCommand, firstIndex), "Wrong offset for GPUDrawIndexedIndirectArgs::StartIndex");
static_assert(OFFSET_OF(GPUDrawIndexedIndirectArgs, StartVertex) == OFFSET_OF(VkDrawIndexedIndirectCommand, vertexOffset), "Wrong offset for GPUDrawIndexedIndirectArgs::StartVertex");
static_assert(OFFSET_OF(GPUDrawIndexedIndirectArgs, StartInstance) == OFFSET_OF(VkDrawIndexedIndirectCommand, firstInstance), "Wrong offset for GPUDrawIndexedIndirectArgs::StartInstance");

void PipelineBarrierVulkan::AddImageBarrier(VkImage image, const VkImageSubresourceRange& range, VkImageLayout srcLayout, VkImageLayout dstLayout, GPUTextureViewVulkan* handle)
{
#if VK_ENABLE_BARRIERS_DEBUG
	ImageBarriersDebug.Add(handle);
#endif
    VkImageMemoryBarrier& imageBarrier = ImageBarriers.AddOne();
    RenderToolsVulkan::ZeroStruct(imageBarrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
    imageBarrier.image = image;
    imageBarrier.subresourceRange = range;
    imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    RenderToolsVulkan::SetImageBarrierInfo(srcLayout, dstLayout, imageBarrier, SourceStage, DestStage);

#if VK_ENABLE_BARRIERS_DEBUG
	LOG(Warning, "Image Barrier: 0x{0:x}, {1} -> {2} for baseMipLevel: {3}, baseArrayLayer: {4}, levelCount: {5}, layerCount: {6} ({7})",
		(int)image,
		srcLayout,
		dstLayout,
		range.baseMipLevel,
		range.baseArrayLayer,
		range.levelCount,
		range.layerCount,

		handle && handle->Owner->AsGPUResource() ? handle->Owner->AsGPUResource()->ToString() : String::Empty
		);
#endif
}

void PipelineBarrierVulkan::AddBufferBarrier(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, VkAccessFlags srcAccess, VkAccessFlags dstAccess)
{
    VkBufferMemoryBarrier& bufferBarrier = BufferBarriers.AddOne();
    RenderToolsVulkan::ZeroStruct(bufferBarrier, VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER);
    bufferBarrier.buffer = buffer;
    bufferBarrier.offset = offset;
    bufferBarrier.size = size;
    bufferBarrier.srcAccessMask = srcAccess;
    bufferBarrier.dstAccessMask = dstAccess;
    bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    RenderToolsVulkan::SetBufferBarrierInfo(srcAccess, dstAccess, SourceStage, DestStage);
}

void PipelineBarrierVulkan::Execute(CmdBufferVulkan* cmdBuffer)
{
    ASSERT(cmdBuffer->IsOutsideRenderPass());

    vkCmdPipelineBarrier(cmdBuffer->GetHandle(), SourceStage, DestStage, 0, 0, nullptr, BufferBarriers.Count(), BufferBarriers.Get(), ImageBarriers.Count(), ImageBarriers.Get());

    Reset();
}

GPUContextVulkan::GPUContextVulkan(GPUDeviceVulkan* device, QueueVulkan* queue)
    : GPUContext(device)
    , _device(device)
    , _queue(queue)
    , _cmdBufferManager(New<CmdBufferManagerVulkan>(device, this))
{
    // Setup descriptor handles tables lookup cache
    _handles[(int32)SpirvShaderResourceBindingType::INVALID] = nullptr;
    _handles[(int32)SpirvShaderResourceBindingType::CB] = _cbHandles;
    _handles[(int32)SpirvShaderResourceBindingType::SAMPLER] = nullptr;
    _handles[(int32)SpirvShaderResourceBindingType::SRV] = _srHandles;
    _handles[(int32)SpirvShaderResourceBindingType::UAV] = _uaHandles;
}

GPUContextVulkan::~GPUContextVulkan()
{
    for (int32 i = 0; i < _descriptorPools.Count(); i++)
    {
        _descriptorPools[i].ClearDelete();
    }
    _descriptorPools.Clear();
    Delete(_cmdBufferManager);
}

void GPUContextVulkan::AddImageBarrier(VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout, VkImageSubresourceRange& subresourceRange, GPUTextureViewVulkan* handle)
{
#if VK_ENABLE_BARRIERS_BATCHING
    // Auto-flush on overflow
    if (_barriers.IsFull())
    {
        const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
        if (cmdBuffer->IsInsideRenderPass())
            EndRenderPass();
        _barriers.Execute(cmdBuffer);
    }
#endif

    // Insert barrier
    _barriers.AddImageBarrier(image, subresourceRange, srcLayout, dstLayout, handle);

#if !VK_ENABLE_BARRIERS_BATCHING
	// Auto-flush without batching
	const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
	if (cmdBuffer->IsInsideRenderPass())
		EndRenderPass();
	_barriers.Execute(cmdBuffer);
#endif
}

void GPUContextVulkan::AddImageBarrier(GPUTextureViewVulkan* handle, VkImageLayout dstLayout)
{
    auto& state = handle->Owner->State;
    const auto subresourceIndex = handle->SubresourceIndex;
    if (subresourceIndex == -1)
    {
        const int32 mipLevels = state.GetSubresourcesCount() / handle->Owner->ArraySlices;
        if (state.AreAllSubresourcesSame())
        {
            // Transition entire resource at once
            const VkImageLayout srcLayout = state.GetSubresourceState(0);
            VkImageSubresourceRange range;
            range.aspectMask = handle->Info.subresourceRange.aspectMask;
            range.baseMipLevel = 0;
            range.levelCount = mipLevels;
            range.baseArrayLayer = 0;
            range.layerCount = handle->Owner->ArraySlices;
            AddImageBarrier(handle->Image, srcLayout, dstLayout, range, handle);
        }
        else
        {
            // Slow path to transition each subresource
            for (int32 i = 0; i < state.GetSubresourcesCount(); i++)
            {
                const VkImageLayout srcLayout = state.GetSubresourceState(i);

                if (srcLayout != dstLayout)
                {
                    VkImageSubresourceRange range;
                    range.aspectMask = handle->Info.subresourceRange.aspectMask;
                    range.baseMipLevel = i % mipLevels;
                    range.levelCount = 1;
                    range.baseArrayLayer = i / mipLevels;
                    range.layerCount = 1;
                    AddImageBarrier(handle->Image, srcLayout, dstLayout, range, handle);
                    state.SetSubresourceState(i, dstLayout);
                }
            }
            ASSERT(state.CheckResourceState(dstLayout));
        }

        state.SetResourceState(dstLayout);
    }
    else
    {
        const VkImageLayout srcLayout = state.GetSubresourceState(subresourceIndex);

        if (srcLayout != dstLayout)
        {
            // Transition a single subresource
            AddImageBarrier(handle->Image, srcLayout, dstLayout, handle->Info.subresourceRange, handle);
            state.SetSubresourceState(subresourceIndex, dstLayout);
        }
    }
}

void GPUContextVulkan::AddImageBarrier(GPUTextureVulkan* texture, int32 mipSlice, int32 arraySlice, VkImageLayout dstLayout)
{
    // Skip if no need to perform image layout transition
    const auto subresourceIndex = RenderTools::CalcSubresourceIndex(mipSlice, arraySlice, texture->MipLevels());
    auto& state = texture->State;
    const auto srcLayout = state.GetSubresourceState(subresourceIndex);
    if (srcLayout == dstLayout)
        return;

    VkImageSubresourceRange range;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = mipSlice;
    range.levelCount = 1;
    range.baseArrayLayer = arraySlice;
    range.layerCount = 1;
    AddImageBarrier(texture->GetHandle(), srcLayout, dstLayout, range, nullptr);
    state.SetSubresourceState(subresourceIndex, dstLayout);
}

void GPUContextVulkan::AddImageBarrier(GPUTextureVulkan* texture, VkImageLayout dstLayout)
{
    // Check for fast path to transition the entire resource at once
    auto& state = texture->State;
    if (state.AreAllSubresourcesSame())
    {
        // Skip if no need to perform image layout transition
        const auto srcLayout = state.GetSubresourceState(0);
        if (srcLayout == dstLayout)
            return;

        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = texture->MipLevels();
        range.baseArrayLayer = 0;
        range.layerCount = texture->ArraySize();
        AddImageBarrier(texture->GetHandle(), srcLayout, dstLayout, range, nullptr);
        state.SetResourceState(dstLayout);
    }
    else
    {
        // Slow path to transition each subresource
        for (int32 arraySlice = 0; arraySlice < texture->ArraySize(); arraySlice++)
        {
            for (int32 mipSlice = 0; mipSlice < texture->MipLevels(); mipSlice++)
            {
                AddImageBarrier(texture, mipSlice, arraySlice, dstLayout);
            }
        }
    }
}

void GPUContextVulkan::AddBufferBarrier(GPUBufferVulkan* buffer, VkAccessFlags dstAccess)
{
    // Skip if no need to perform buffer memory transition
    if ((buffer->Access & dstAccess) == dstAccess)
        return;

#if VK_ENABLE_BARRIERS_BATCHING
    // Auto-flush on overflow
    if (_barriers.IsFull())
    {
        const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
        if (cmdBuffer->IsInsideRenderPass())
            EndRenderPass();
        _barriers.Execute(cmdBuffer);
    }
#endif

    // Insert barrier
    _barriers.AddBufferBarrier(buffer->GetHandle(), 0, buffer->GetSize(), buffer->Access, dstAccess);
    buffer->Access = dstAccess;

#if !VK_ENABLE_BARRIERS_BATCHING
	// Auto-flush without batching
	const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
	if (cmdBuffer->IsInsideRenderPass())
		EndRenderPass();
	_barriers.Execute(cmdBuffer);
#endif
}

void GPUContextVulkan::FlushBarriers()
{
#if VK_ENABLE_BARRIERS_BATCHING
    if (_barriers.HasBarrier())
    {
        const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
        if (cmdBuffer->IsInsideRenderPass())
            EndRenderPass();
        _barriers.Execute(cmdBuffer);
    }
#endif
}

DescriptorPoolVulkan* GPUContextVulkan::AllocateDescriptorSets(const VkDescriptorSetAllocateInfo& descriptorSetAllocateInfo, const DescriptorSetLayoutVulkan& layout, VkDescriptorSet* outSets)
{
    VkResult result = VK_ERROR_OUT_OF_DEVICE_MEMORY;
    VkDescriptorSetAllocateInfo allocateInfo = descriptorSetAllocateInfo;
    DescriptorPoolVulkan* pool = nullptr;

    const uint32 hash = VULKAN_HASH_POOLS_WITH_TYPES_USAGE_ID ? layout.GetTypesUsageID() : GetHash(layout);
    DescriptorPoolArray* typedDescriptorPools = _descriptorPools.TryGet(hash);

    if (typedDescriptorPools != nullptr)
    {
        pool = typedDescriptorPools->HasItems() ? typedDescriptorPools->Last() : nullptr;

        if (pool && pool->CanAllocate(layout))
        {
            allocateInfo.descriptorPool = pool->GetHandle();
            result = vkAllocateDescriptorSets(_device->Device, &allocateInfo, outSets);
        }
    }
    else
    {
        typedDescriptorPools = &_descriptorPools.Add(hash, DescriptorPoolArray())->Value;
    }

    if (result < VK_SUCCESS)
    {
        if (pool && pool->IsEmpty())
        {
            LOG_VULKAN_RESULT(result);
        }
        else
        {
            // Spec says any negative value could be due to fragmentation, so create a new Pool. If it fails here then we really are out of memory!
            pool = New<DescriptorPoolVulkan>(_device, layout);
            typedDescriptorPools->Add(pool);
            allocateInfo.descriptorPool = pool->GetHandle();
            VALIDATE_VULKAN_RESULT(vkAllocateDescriptorSets(_device->Device, &allocateInfo, outSets));
        }
    }

    return pool;
}

void GPUContextVulkan::BeginRenderPass()
{
    auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();

    // Build render targets layout descriptor and framebuffer key
    FramebufferVulkan::Key framebufferKey;
    framebufferKey.AttachmentCount = _rtCount;
    RenderTargetLayoutVulkan layout;
    layout.RTsCount = _rtCount;
    layout.DepthFormat = _rtDepth ? _rtDepth->GetFormat() : PixelFormat::Unknown;
    for (int32 i = 0; i < GPU_MAX_RT_BINDED; i++)
    {
        auto handle = _rtHandles[i];
        if (handle)
        {
            layout.RTVsFormats[i] = handle->GetFormat();
            framebufferKey.Attachments[i] = handle->GetFramebufferView();

            AddImageBarrier(handle, handle->LayoutRTV);
        }
        else
        {
            layout.RTVsFormats[i] = PixelFormat::Unknown;
            framebufferKey.Attachments[i] = VK_NULL_HANDLE;
        }
    }
    if (_rtDepth)
    {
        auto handle = _rtDepth;
        layout.MSAA = handle->GetMSAA();
        layout.Extent = handle->Extent;
        layout.ReadDepth = true; // TODO: use proper depthStencilAccess flags
        layout.WriteDepth = handle->LayoutRTV == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // TODO: do it in a proper way
        framebufferKey.AttachmentCount++;
        framebufferKey.Attachments[_rtCount] = handle->GetFramebufferView();

        AddImageBarrier(handle, handle->LayoutRTV);
    }
    else if (_rtHandles[0])
    {
        layout.MSAA = _rtHandles[0]->GetMSAA();
        layout.Extent = _rtHandles[0]->Extent;
        layout.ReadDepth = false;
        layout.WriteDepth = false;
    }
    else
    {
        // No depth or render target binded?
        CRASH;
    }

    // Get or create objects
    auto renderPass = _device->GetOrCreateRenderPass(layout);
    framebufferKey.RenderPass = renderPass;
    uint32 layers = 1; // TODO: support rendering to many layers (eg. texture array)
    auto framebuffer = _device->GetOrCreateFramebuffer(framebufferKey, layout.Extent, layers);
    _renderPass = renderPass;

    FlushBarriers();

    // TODO: use clear values for render pass begin to improve performance
    cmdBuffer->BeginRenderPass(renderPass, framebuffer, 0, nullptr);
}

void GPUContextVulkan::EndRenderPass()
{
    auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
    cmdBuffer->EndRenderPass();
    _renderPass = nullptr;

    // Place a barrier between RenderPasses, so that color / depth outputs can be read in subsequent passes
    // TODO: remove it in future and use proper barriers without whole pipeline stalls
    vkCmdPipelineBarrier(cmdBuffer->GetHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);
}

void GPUContextVulkan::UpdateDescriptorSets(const SpirvShaderDescriptorInfo& descriptorInfo, DescriptorSetWriterVulkan& dsWriter, bool& needsWrite)
{
    for (uint32 i = 0; i < descriptorInfo.DescriptorTypesCount; i++)
    {
        const auto& descriptor = descriptorInfo.DescriptorTypes[i];
        const int32 descriptorIndex = descriptor.Binding;
        DescriptorOwnerResourceVulkan** handles = _handles[(int32)descriptor.BindingType];

        switch (descriptor.DescriptorType)
        {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        {
            // Sampler
            const VkSampler sampler = _device->HelperResources.GetStaticSampler((HelperResourcesVulkan::StaticSamplers)descriptor.Slot);
            needsWrite |= dsWriter.WriteSampler(descriptorIndex, sampler);
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        {
            // Shader Resource (Texture)
            auto handle = (GPUTextureViewVulkan*)handles[descriptor.Slot];
            if (!handle)
            {
                const auto dummy = _device->HelperResources.GetDummyTexture(descriptor.ResourceType);
                switch (descriptor.ResourceType)
                {
                case SpirvShaderResourceType::Texture1D:
                case SpirvShaderResourceType::Texture2D:
                    handle = static_cast<GPUTextureViewVulkan*>(dummy->View(0));
                    break;
                case SpirvShaderResourceType::Texture3D:
                    handle = static_cast<GPUTextureViewVulkan*>(dummy->ViewVolume());
                    break;
                case SpirvShaderResourceType::TextureCube:
                case SpirvShaderResourceType::Texture1DArray:
                case SpirvShaderResourceType::Texture2DArray:
                    handle = static_cast<GPUTextureViewVulkan*>(dummy->ViewArray());
                    break;
                }
            }
            VkImageView imageView;
            VkImageLayout layout;
            handle->DescriptorAsImage(this, imageView, layout);
            ASSERT(imageView != VK_NULL_HANDLE);
            needsWrite |= dsWriter.WriteImage(descriptorIndex, imageView, layout);
            break;
        }
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        {
            // Shader Resource (Buffer)
            auto sr = handles[descriptor.Slot];
            if (!sr)
            {
                const auto dummy = _device->HelperResources.GetDummyBuffer();
                sr = (DescriptorOwnerResourceVulkan*)dummy->View()->GetNativePtr();
            }
            const VkBufferView* bufferView;
            sr->DescriptorAsUniformTexelBuffer(this, bufferView);
            needsWrite |= dsWriter.WriteUniformTexelBuffer(descriptorIndex, bufferView);
            break;
        }
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        {
            // Unordered Access (Texture)
            auto ua = handles[descriptor.Slot];
            ASSERT(ua);
            VkImageView imageView;
            VkImageLayout layout;
            ua->DescriptorAsStorageImage(this, imageView, layout);
            needsWrite |= dsWriter.WriteStorageImage(descriptorIndex, imageView, layout);
            break;
        }
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        {
            // Unordered Access (Buffer)
            auto ua = handles[descriptor.Slot];
            if (!ua)
            {
                const auto dummy = _device->HelperResources.GetDummyBuffer();
                ua = (DescriptorOwnerResourceVulkan*)dummy->View()->GetNativePtr();
            }
            VkBuffer buffer;
            VkDeviceSize offset, range;
            ua->DescriptorAsStorageBuffer(this, buffer, offset, range);
            needsWrite |= dsWriter.WriteStorageBuffer(descriptorIndex, buffer, offset, range);
            break;
        }
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        {
            // Constant Buffer
            auto cb = handles[descriptor.Slot];
            ASSERT(cb);
            VkBuffer buffer;
            VkDeviceSize offset, range;
            uint32 dynamicOffset;
            cb->DescriptorAsDynamicUniformBuffer(this, buffer, offset, range, dynamicOffset);
            needsWrite |= dsWriter.WriteDynamicUniformBuffer(descriptorIndex, buffer, offset, range, dynamicOffset);
            break;
        }
        default:
            // Unknown or invalid descriptor type
        CRASH;
            break;
        }
    }
}

void GPUContextVulkan::UpdateDescriptorSets(GPUPipelineStateVulkan* pipelineState)
{
    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
    const auto pipelineLayout = pipelineState->GetLayout();
    ASSERT(pipelineLayout);
    bool needsWrite = false;

    // No current descriptor pools set - acquire one and reset
    const bool newDescriptorPool = pipelineState->AcquirePoolSet(cmdBuffer);
    needsWrite |= newDescriptorPool;

    // Update descriptors for every used shader stage
    uint32 remainingHasDescriptorsPerStageMask = pipelineState->HasDescriptorsPerStageMask;
    for (int32 stage = 0; stage < DescriptorSet::GraphicsStagesCount && remainingHasDescriptorsPerStageMask; stage++)
    {
        // Only process stages that exist in this pipeline and use descriptors
        if (remainingHasDescriptorsPerStageMask & 1)
        {
            UpdateDescriptorSets(*pipelineState->DescriptorInfoPerStage[stage], pipelineState->DSWriter[stage], needsWrite);
        }

        remainingHasDescriptorsPerStageMask >>= 1;
    }

    // Allocate sets if need to
    //if (needsWrite) // TODO: write on change only?
    {
        if (!pipelineState->AllocateDescriptorSets())
        {
            return;
        }
        uint32 remainingStagesMask = pipelineState->HasDescriptorsPerStageMask;
        uint32 stage = 0;
        while (remainingStagesMask)
        {
            if (remainingStagesMask & 1)
            {
                const VkDescriptorSet descriptorSet = pipelineState->DescriptorSetHandles[stage];
                pipelineState->DSWriter[stage].SetDescriptorSet(descriptorSet);
            }

            stage++;
            remainingStagesMask >>= 1;
        }

        vkUpdateDescriptorSets(_device->Device, pipelineState->DSWriteContainer.DescriptorWrites.Count(), pipelineState->DSWriteContainer.DescriptorWrites.Get(), 0, nullptr);
    }
}

void GPUContextVulkan::UpdateDescriptorSets(ComputePipelineStateVulkan* pipelineState)
{
    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
    const auto pipelineLayout = pipelineState->GetLayout();
    ASSERT(pipelineLayout);

    bool needsWrite = false;

    // No current descriptor pools set - acquire one and reset
    const bool newDescriptorPool = pipelineState->AcquirePoolSet(cmdBuffer);
    needsWrite |= newDescriptorPool;

    // Update descriptors
    UpdateDescriptorSets(*pipelineState->DescriptorInfo, pipelineState->DSWriter, needsWrite);

    // Allocate sets if need to
    //if (needsWrite) // TODO: write on change only?f
    {
        if (!pipelineState->AllocateDescriptorSets())
        {
            return;
        }
        const VkDescriptorSet descriptorSet = pipelineState->DescriptorSetHandles[DescriptorSet::Compute];
        pipelineState->DSWriter.SetDescriptorSet(descriptorSet);

        vkUpdateDescriptorSets(_device->Device, pipelineState->DSWriteContainer.DescriptorWrites.Count(), pipelineState->DSWriteContainer.DescriptorWrites.Get(), 0, nullptr);
    }
}

void GPUContextVulkan::BindPipeline()
{
    if (_psDirtyFlag && _currentState && (_rtDepth || _rtCount))
    {
        // Clear flag
        _psDirtyFlag = false;

        // Change state
        const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
        const auto pipeline = _currentState->GetState(_renderPass);
        vkCmdBindPipeline(cmdBuffer->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        RENDER_STAT_PS_STATE_CHANGE();
    }
}

void GPUContextVulkan::OnDrawCall()
{
    GPUPipelineStateVulkan* pipelineState = _currentState;
    ASSERT(pipelineState && pipelineState->IsValid());
    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
    const auto pipelineLayout = pipelineState->GetLayout();
    ASSERT(pipelineLayout);

    // End previous render pass if render targets layout was modified
    if (_rtDirtyFlag && cmdBuffer->IsInsideRenderPass())
        EndRenderPass();

    if (pipelineState->HasDescriptorsPerStageMask)
    {
        UpdateDescriptorSets(pipelineState);
    }

    // Start render pass if not during one
    if (cmdBuffer->IsOutsideRenderPass())
        BeginRenderPass();
    else if (_barriers.HasBarrier())
    {
        // TODO: implement better image/buffer barriers - and remove this renderPass split to flush barriers
        EndRenderPass();
        BeginRenderPass();
    }

    BindPipeline();

    //UpdateDynamicStates();

    // Bind descriptors sets to the graphics pipeline
    if (pipelineState->HasDescriptorsPerStageMask)
    {
        pipelineState->Bind(cmdBuffer);
    }

    // Clear flag
    _rtDirtyFlag = false;

#if VK_ENABLE_BARRIERS_DEBUG
	LOG(Warning, "Draw");
#endif
}

void GPUContextVulkan::FrameBegin()
{
    // Base
    GPUContext::FrameBegin();

    // Setup
    _psDirtyFlag = 0;
    _rtDirtyFlag = 0;
    _cbDirtyFlag = 0;
    _srDirtyFlag = 0;
    _uaDirtyFlag = 0;
    _rtCount = 0;
    _renderPass = nullptr;
    _currentState = nullptr;
    _rtDepth = nullptr;
    Platform::MemoryClear(_rtHandles, sizeof(_rtHandles));
    Platform::MemoryClear(_cbHandles, sizeof(_cbHandles));
    Platform::MemoryClear(_srHandles, sizeof(_srHandles));
    Platform::MemoryClear(_uaHandles, sizeof(_uaHandles));

#if VULKAN_RESET_QUERY_POOLS
    // Reset pending queries
    if (_device->QueriesToReset.HasItems())
    {
        const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
        for (auto query : _device->QueriesToReset)
            query->Reset(cmdBuffer);
        _device->QueriesToReset.Clear();
    }
#endif
}

void GPUContextVulkan::FrameEnd()
{
    const auto cmdBuffer = GetCmdBufferManager()->GetActiveCmdBuffer();
    if (cmdBuffer && cmdBuffer->IsInsideRenderPass())
    {
        EndRenderPass();
    }

    // Execute any queued layout transitions that weren't already handled by the render pass
    FlushBarriers();

    // Base
    GPUContext::FrameEnd();
}

#if GPU_ALLOW_PROFILE_EVENTS

void GPUContextVulkan::EventBegin(const Char* name)
{
    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
    cmdBuffer->BeginEvent(name);
}

void GPUContextVulkan::EventEnd()
{
    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
    cmdBuffer->EndEvent();
}

#endif

void* GPUContextVulkan::GetNativePtr() const
{
    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
    return (void*)cmdBuffer;
}

bool GPUContextVulkan::IsDepthBufferBinded()
{
    return _rtDepth != nullptr;
}

void GPUContextVulkan::Clear(GPUTextureView* rt, const Color& color)
{
    auto rtVulkan = static_cast<GPUTextureViewVulkan*>(rt);

    if (rtVulkan)
    {
        // TODO: detect if inside render pass and use ClearAttachments
        // TODO: delay clear for attachments before render pass to use render pass clear values for faster clearing

        const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
        if (cmdBuffer->IsInsideRenderPass())
            EndRenderPass();

        AddImageBarrier(rtVulkan, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        FlushBarriers();

        vkCmdClearColorImage(cmdBuffer->GetHandle(), rtVulkan->Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (const VkClearColorValue*)color.Raw, 1, &rtVulkan->Info.subresourceRange);
    }
}

void GPUContextVulkan::ClearDepth(GPUTextureView* depthBuffer, float depthValue)
{
    const auto rtVulkan = static_cast<GPUTextureViewVulkan*>(depthBuffer);

    if (rtVulkan)
    {
        // TODO: detect if inside render pass and use ClearAttachments
        // TODO: delay clear for attachments before render pass to use render pass clear values for faster clearing

        const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
        if (cmdBuffer->IsInsideRenderPass())
            EndRenderPass();

        AddImageBarrier(rtVulkan, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        FlushBarriers();

        VkClearDepthStencilValue clear;
        clear.depth = depthValue;
        clear.stencil = 0;
        vkCmdClearDepthStencilImage(cmdBuffer->GetHandle(), rtVulkan->Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &rtVulkan->Info.subresourceRange);
    }
}

void GPUContextVulkan::ClearUA(GPUBuffer* buf, const Vector4& value)
{
    const auto bufVulkan = static_cast<GPUBufferVulkan*>(buf);

    if (bufVulkan)
    {
        const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
        if (cmdBuffer->IsInsideRenderPass())
            EndRenderPass();

        uint32_t* data = (uint32_t*)&value;
        vkCmdFillBuffer(cmdBuffer->GetHandle(), bufVulkan->GetHandle(), 0, bufVulkan->GetSize(), *data);
    }
}

void GPUContextVulkan::ResetRenderTarget()
{
    if (_rtDepth != nullptr || _rtCount != 0 || _rtDepth)
    {
        _rtDirtyFlag = true;
        _psDirtyFlag = true;
        _rtCount = 0;
        _rtDepth = nullptr;
        Platform::MemoryClear(_rtHandles, sizeof(_rtHandles));

        const auto cmdBuffer = _cmdBufferManager->GetActiveCmdBuffer();
        if (cmdBuffer && cmdBuffer->IsInsideRenderPass())
            EndRenderPass();
    }
}

void GPUContextVulkan::SetRenderTarget(GPUTextureView* rt)
{
    const auto rtVulkan = static_cast<GPUTextureViewVulkan*>(rt);

    if (_rtDepth != nullptr || _rtCount != 1 || _rtHandles[0] != rtVulkan)
    {
        _rtDirtyFlag = true;
        _psDirtyFlag = true;
        _rtCount = 1;
        _rtDepth = nullptr;
        _rtHandles[0] = rtVulkan;
    }
}

void GPUContextVulkan::SetRenderTarget(GPUTextureView* depthBuffer, GPUTextureView* rt)
{
    const auto rtVulkan = static_cast<GPUTextureViewVulkan*>(rt);
    const auto depthBufferVulkan = static_cast<GPUTextureViewVulkan*>(depthBuffer);
    const auto rtCount = rtVulkan ? 1 : 0;

    if (_rtDepth != depthBufferVulkan || _rtCount != rtCount || _rtHandles[0] != rtVulkan)
    {
        _rtDirtyFlag = true;
        _psDirtyFlag = true;
        _rtCount = rtCount;
        _rtDepth = depthBufferVulkan;
        _rtHandles[0] = rtVulkan;
    }
}

void GPUContextVulkan::SetRenderTarget(GPUTextureView* depthBuffer, const Span<GPUTextureView*>& rts)
{
    ASSERT(Math::IsInRange(rts.Length(), 1, GPU_MAX_RT_BINDED));

    const auto depthBufferVulkan = static_cast<GPUTextureViewVulkan*>(depthBuffer);

    GPUTextureViewVulkan* rtvs[GPU_MAX_RT_BINDED];
    for (int32 i = 0; i < rts.Length(); i++)
    {
        rtvs[i] = static_cast<GPUTextureViewVulkan*>(rts[i]);
    }
    const int32 rtvsSize = sizeof(GPUTextureViewVulkan*) * rts.Length();

    if (_rtDepth != depthBufferVulkan || _rtCount != rts.Length() || Platform::MemoryCompare(_rtHandles, rtvs, rtvsSize) != 0)
    {
        _rtDirtyFlag = true;
        _psDirtyFlag = true;
        _rtCount = rts.Length();
        _rtDepth = depthBufferVulkan;
        Platform::MemoryCopy(_rtHandles, rtvs, rtvsSize);
    }
}

void GPUContextVulkan::SetRenderTarget(GPUTextureView* rt, GPUBuffer* uaOutput)
{
    // TODO: implement Draw Indirect and Pixel Shader write to UAV on Vulkan
    MISSING_CODE("GPUContextVulkan::SetRenderTarget with UA output");
}

void GPUContextVulkan::ResetSR()
{
    _srDirtyFlag = false;
    Platform::MemoryClear(_srHandles, sizeof(_srHandles));
}

void GPUContextVulkan::ResetUA()
{
    _uaDirtyFlag = false;
    Platform::MemoryClear(_uaHandles, sizeof(_uaHandles));
}

void GPUContextVulkan::ResetCB()
{
    _cbDirtyFlag = false;
    Platform::MemoryClear(_cbHandles, sizeof(_cbHandles));
}

void GPUContextVulkan::BindCB(int32 slot, GPUConstantBuffer* cb)
{
    ASSERT(slot >= 0 && slot < GPU_MAX_CB_BINDED);

    const auto cbVulkan = static_cast<GPUConstantBufferVulkan*>(cb);

    if (_cbHandles[slot] != cbVulkan)
    {
        _cbDirtyFlag = true;
        _cbHandles[slot] = cbVulkan;
    }
}

void GPUContextVulkan::BindSR(int32 slot, GPUResourceView* view)
{
    ASSERT(slot >= 0 && slot < GPU_MAX_SR_BINDED);

    const auto handle = view ? (DescriptorOwnerResourceVulkan*)view->GetNativePtr() : nullptr;

    if (_srHandles[slot] != handle)
    {
        _srDirtyFlag = true;
        _srHandles[slot] = handle;
    }
}

void GPUContextVulkan::BindUA(int32 slot, GPUResourceView* view)
{
    ASSERT(slot >= 0 && slot < GPU_MAX_UA_BINDED);

    const auto handle = view ? (DescriptorOwnerResourceVulkan*)view->GetNativePtr() : nullptr;

    if (_uaHandles[slot] != handle)
    {
        _uaDirtyFlag = true;
        _uaHandles[slot] = handle;
    }
}

void GPUContextVulkan::BindVB(const Span<GPUBuffer*>& vertexBuffers, const uint32* vertexBuffersOffsets)
{
    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
    VkBuffer buffers[GPU_MAX_VB_BINDED];
    VkDeviceSize offsets[GPU_MAX_VB_BINDED];
    for (int32 i = 0; i < vertexBuffers.Length(); i++)
    {
        auto vbVulkan = static_cast<GPUBufferVulkan*>(vertexBuffers[i]);
        if (!vbVulkan)
            vbVulkan = _device->HelperResources.GetDummyVertexBuffer();
        buffers[i] = vbVulkan->GetHandle();
        offsets[i] = vertexBuffersOffsets ? vertexBuffersOffsets[i] : 0;
    }
    vkCmdBindVertexBuffers(cmdBuffer->GetHandle(), 0, vertexBuffers.Length(), buffers, offsets);
}

void GPUContextVulkan::BindIB(GPUBuffer* indexBuffer)
{
    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
    const auto ibVulkan = static_cast<GPUBufferVulkan*>(indexBuffer)->GetHandle();
    vkCmdBindIndexBuffer(cmdBuffer->GetHandle(), ibVulkan, 0, indexBuffer->GetFormat() == PixelFormat::R32_UInt ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
}

void GPUContextVulkan::UpdateCB(GPUConstantBuffer* cb, const void* data)
{
    ASSERT(data && cb);
    auto cbVulkan = static_cast<GPUConstantBufferVulkan*>(cb);
    const uint32 size = cbVulkan->GetSize();
    if (size == 0)
        return;
    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();

    // Allocate bytes for the buffer
    const auto allocation = _device->UniformBufferUploader->Allocate(size, 0, cmdBuffer);

    // Copy data
    Platform::MemoryCopy(allocation.CPUAddress, data, allocation.Size);

    // Cache the allocation to update the descriptor
    cbVulkan->Allocation = allocation;

    // Mark CB slot as dirty if this CB is binded to the pipeline
    for (int32 i = 0; i < ARRAY_COUNT(_cbHandles); i++)
    {
        if (_cbHandles[i] == cbVulkan)
        {
            _cbDirtyFlag = true;
            break;
        }
    }
}

void GPUContextVulkan::Dispatch(GPUShaderProgramCS* shader, uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ)
{
    ASSERT(shader);
    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
    auto shaderVulkan = (GPUShaderProgramCSVulkan*)shader;

    // End any render pass
    if (cmdBuffer->IsInsideRenderPass())
        EndRenderPass();

    auto pipelineState = shaderVulkan->GetOrCreateState();

    UpdateDescriptorSets(pipelineState);

    FlushBarriers();

    // Bind pipeline
    vkCmdBindPipeline(cmdBuffer->GetHandle(), VK_PIPELINE_BIND_POINT_COMPUTE, pipelineState->GetHandle());
    RENDER_STAT_PS_STATE_CHANGE();

    // Bind descriptors sets to the compute pipeline
    pipelineState->Bind(cmdBuffer);

    // Dispatch
    vkCmdDispatch(cmdBuffer->GetHandle(), threadGroupCountX, threadGroupCountY, threadGroupCountZ);
    RENDER_STAT_DISPATCH_CALL();

#if VK_ENABLE_BARRIERS_DEBUG
	LOG(Warning, "Dispatch");
#endif
}

void GPUContextVulkan::DispatchIndirect(GPUShaderProgramCS* shader, GPUBuffer* bufferForArgs, uint32 offsetForArgs)
{
    ASSERT(shader && bufferForArgs);
    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
    auto shaderVulkan = (GPUShaderProgramCSVulkan*)shader;
    const auto bufferForArgsVulkan = (GPUBufferVulkan*)bufferForArgs;

    // End any render pass
    if (cmdBuffer->IsInsideRenderPass())
        EndRenderPass();

    auto pipelineState = shaderVulkan->GetOrCreateState();

    UpdateDescriptorSets(pipelineState);
    AddBufferBarrier(bufferForArgsVulkan, VK_ACCESS_INDIRECT_COMMAND_READ_BIT);

    FlushBarriers();

    // Bind pipeline
    vkCmdBindPipeline(cmdBuffer->GetHandle(), VK_PIPELINE_BIND_POINT_COMPUTE, pipelineState->GetHandle());
    RENDER_STAT_PS_STATE_CHANGE();

    // Bind descriptors sets to the compute pipeline
    pipelineState->Bind(cmdBuffer);

    // Dispatch
    vkCmdDispatchIndirect(cmdBuffer->GetHandle(), bufferForArgsVulkan->GetHandle(), offsetForArgs);
    RENDER_STAT_DISPATCH_CALL();

#if VK_ENABLE_BARRIERS_DEBUG
	LOG(Warning, "DispatchIndirect");
#endif
}

void GPUContextVulkan::ResolveMultisample(GPUTexture* sourceMultisampleTexture, GPUTexture* destTexture, int32 sourceSubResource, int32 destSubResource, PixelFormat format)
{
    ASSERT(sourceMultisampleTexture && sourceMultisampleTexture->IsMultiSample());
    ASSERT(destTexture && !destTexture->IsMultiSample());

    // TODO: use render pass to resolve attachments

    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
    if (cmdBuffer->IsInsideRenderPass())
        EndRenderPass();

    const auto dstResourceVulkan = dynamic_cast<GPUTextureVulkan*>(destTexture);
    const auto srcResourceVulkan = dynamic_cast<GPUTextureVulkan*>(sourceMultisampleTexture);

    const int32 srcMipIndex = sourceSubResource % dstResourceVulkan->MipLevels();
    const int32 srcArrayIndex = sourceSubResource / dstResourceVulkan->MipLevels();
    const int32 dstMipIndex = destSubResource % dstResourceVulkan->MipLevels();
    const int32 dstArrayIndex = destSubResource / dstResourceVulkan->MipLevels();

    int32 width, height, depth;
    sourceMultisampleTexture->GetMipSize(srcMipIndex, width, height, depth);

    AddImageBarrier(dstResourceVulkan, dstMipIndex, dstArrayIndex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    AddImageBarrier(srcResourceVulkan, srcMipIndex, srcArrayIndex, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    FlushBarriers();

    VkImageResolve region;
    region.srcSubresource.aspectMask = srcResourceVulkan->DefaultAspectMask;
    region.srcSubresource.mipLevel = srcMipIndex;
    region.srcSubresource.baseArrayLayer = srcArrayIndex;
    region.srcSubresource.layerCount = 1;
    region.srcOffset = { 0, 0, 0 };
    region.dstSubresource.aspectMask = dstResourceVulkan->DefaultAspectMask;
    region.dstSubresource.mipLevel = dstMipIndex;
    region.dstSubresource.baseArrayLayer = dstArrayIndex;
    region.dstSubresource.layerCount = 1;
    region.dstOffset = { 0, 0, 0 };
    region.extent = { (uint32_t)width, (uint32_t)height, (uint32_t)depth };

    vkCmdResolveImage(cmdBuffer->GetHandle(), srcResourceVulkan->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstResourceVulkan->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void GPUContextVulkan::DrawInstanced(uint32 verticesCount, uint32 instanceCount, int32 startInstance, int32 startVertex)
{
    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
    OnDrawCall();
    vkCmdDraw(cmdBuffer->GetHandle(), verticesCount, instanceCount, startVertex, startInstance);
    RENDER_STAT_DRAW_CALL(verticesCount * instanceCount, verticesCount * instanceCount / 3);
}

void GPUContextVulkan::DrawIndexedInstanced(uint32 indicesCount, uint32 instanceCount, int32 startInstance, int32 startVertex, int32 startIndex)
{
    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
    OnDrawCall();
    vkCmdDrawIndexed(cmdBuffer->GetHandle(), indicesCount, instanceCount, startIndex, startVertex, startInstance);
    RENDER_STAT_DRAW_CALL(0, indicesCount / 3 * instanceCount);
}

void GPUContextVulkan::DrawInstancedIndirect(GPUBuffer* bufferForArgs, uint32 offsetForArgs)
{
    // TODO: implement it
    MISSING_CODE("GPUContextVulkan::DrawInstancedIndirect");
}

void GPUContextVulkan::DrawIndexedInstancedIndirect(GPUBuffer* bufferForArgs, uint32 offsetForArgs)
{
    // TODO: implement it
    MISSING_CODE("GPUContextVulkan::DrawIndexedInstancedIndirect");
}

void GPUContextVulkan::SetViewport(const Viewport& viewport)
{
    vkCmdSetViewport(_cmdBufferManager->GetCmdBuffer()->GetHandle(), 0, 1, (VkViewport*)&viewport);
}

void GPUContextVulkan::SetScissor(const Rectangle& scissorRect)
{
    VkRect2D rect;
    rect.offset.x = (int32_t)scissorRect.Location.X;
    rect.offset.y = (int32_t)scissorRect.Location.Y;
    rect.extent.width = (uint32_t)scissorRect.Size.X;
    rect.extent.height = (uint32_t)scissorRect.Size.Y;
    vkCmdSetScissor(_cmdBufferManager->GetCmdBuffer()->GetHandle(), 0, 1, &rect);
}

GPUPipelineState* GPUContextVulkan::GetState() const
{
    return _currentState;
}

void GPUContextVulkan::SetState(GPUPipelineState* state)
{
    if (_currentState != state)
    {
        _currentState = static_cast<GPUPipelineStateVulkan*>(state);
        _psDirtyFlag = true;
    }
}

void GPUContextVulkan::ClearState()
{
    ResetRenderTarget();
    ResetSR();
    ResetUA();
    ResetCB();
    SetState(nullptr);

    FlushState();
}

void GPUContextVulkan::FlushState()
{
    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
    if (cmdBuffer->IsInsideRenderPass())
    {
        EndRenderPass();
    }

    FlushBarriers();
}

void GPUContextVulkan::Flush()
{
    // Flush reaming and buffered commands
    FlushState();
    _currentState = nullptr;

    // Execute commands
    _cmdBufferManager->SubmitActiveCmdBuffer();
    _cmdBufferManager->PrepareForNewActiveCommandBuffer();
    ASSERT(_cmdBufferManager->HasPendingActiveCmdBuffer() && _cmdBufferManager->GetActiveCmdBuffer()->GetState() == CmdBufferVulkan::State::IsInsideBegin);
}

void GPUContextVulkan::UpdateBuffer(GPUBuffer* buffer, const void* data, uint32 size, uint32 offset)
{
    ASSERT(data);
    ASSERT(buffer && buffer->GetSize() >= size);

    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
    if (cmdBuffer->IsInsideRenderPass())
        EndRenderPass();

    const auto bufferVulkan = static_cast<GPUBufferVulkan*>(buffer);

    // Use direct update for small buffers
    if (size <= 16 * 1024)
    {
        //AddBufferBarrier(bufferVulkan, VK_ACCESS_TRANSFER_WRITE_BIT);
        //FlushBarriers();

        size = Math::AlignUp<uint32>(size, 4);
        vkCmdUpdateBuffer(cmdBuffer->GetHandle(), bufferVulkan->GetHandle(), offset, size, data);
    }
    else
    {
        auto staging = _device->StagingManager.AcquireBuffer(size, GPUResourceUsage::StagingUpload);
        staging->SetData(data, size);

        VkBufferCopy region;
        region.size = size;
        region.srcOffset = 0;
        region.dstOffset = offset;
        vkCmdCopyBuffer(cmdBuffer->GetHandle(), ((GPUBufferVulkan*)staging)->GetHandle(), ((GPUBufferVulkan*)buffer)->GetHandle(), 1, &region);

        _device->StagingManager.ReleaseBuffer(cmdBuffer, staging);
    }
}

void GPUContextVulkan::CopyBuffer(GPUBuffer* dstBuffer, GPUBuffer* srcBuffer, uint32 size, uint32 dstOffset, uint32 srcOffset)
{
    ASSERT(dstBuffer && srcBuffer);
    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();

    // Ensure to end active render pass
    if (cmdBuffer->IsInsideRenderPass())
        EndRenderPass();

    auto dstBufferVulkan = static_cast<GPUBufferVulkan*>(dstBuffer);
    auto srcBufferVulkan = static_cast<GPUBufferVulkan*>(srcBuffer);

    // Transition resources
    AddBufferBarrier(dstBufferVulkan, VK_ACCESS_TRANSFER_WRITE_BIT);
    AddBufferBarrier(srcBufferVulkan, VK_ACCESS_TRANSFER_READ_BIT);
    FlushBarriers();

    VkBufferCopy bufferCopy;
    bufferCopy.srcOffset = srcOffset;
    bufferCopy.dstOffset = dstOffset;
    bufferCopy.size = size;
    vkCmdCopyBuffer(cmdBuffer->GetHandle(), srcBufferVulkan->GetHandle(), dstBufferVulkan->GetHandle(), 1, &bufferCopy);
}

void GPUContextVulkan::UpdateTexture(GPUTexture* texture, int32 arrayIndex, int32 mipIndex, const void* data, uint32 rowPitch, uint32 slicePitch)
{
    ASSERT(texture && texture->IsAllocated() && data);

    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
    if (cmdBuffer->IsInsideRenderPass())
        EndRenderPass();

    const auto textureVulkan = static_cast<GPUTextureVulkan*>(texture);

    AddImageBarrier(textureVulkan, mipIndex, arrayIndex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    FlushBarriers();

    auto buffer = _device->StagingManager.AcquireBuffer(slicePitch, GPUResourceUsage::StagingUpload);
    buffer->SetData(data, slicePitch);

    // Setup buffer copy region
    int32 mipWidth, mipHeight, mipDepth;
    texture->GetMipSize(mipIndex, mipWidth, mipHeight, mipDepth);
    VkBufferImageCopy bufferCopyRegion;
    Platform::MemoryClear(&bufferCopyRegion, sizeof(bufferCopyRegion));
    bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferCopyRegion.imageSubresource.mipLevel = mipIndex;
    bufferCopyRegion.imageSubresource.baseArrayLayer = arrayIndex;
    bufferCopyRegion.imageSubresource.layerCount = 1;
    bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(mipWidth);
    bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(mipHeight);
    bufferCopyRegion.imageExtent.depth = static_cast<uint32_t>(mipDepth);

    // Copy mip level from staging buffer
    vkCmdCopyBufferToImage(cmdBuffer->GetHandle(), ((GPUBufferVulkan*)buffer)->GetHandle(), textureVulkan->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);

    _device->StagingManager.ReleaseBuffer(cmdBuffer, buffer);
}

void GPUContextVulkan::CopyTexture(GPUTexture* dstResource, uint32 dstSubresource, uint32 dstX, uint32 dstY, uint32 dstZ, GPUTexture* srcResource, uint32 srcSubresource)
{
    ASSERT(dstResource && srcResource);
    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();

    // Ensure to end active render pass
    if (cmdBuffer->IsInsideRenderPass())
        EndRenderPass();

    const auto dstTextureVulkan = static_cast<GPUTextureVulkan*>(dstResource);
    const auto srcTextureVulkan = static_cast<GPUTextureVulkan*>(srcResource);

    // Transition resources
    AddImageBarrier(dstTextureVulkan, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    AddImageBarrier(srcTextureVulkan, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    FlushBarriers();

    // Prepare
    const int32 dstMipIndex = dstSubresource % dstTextureVulkan->MipLevels();
    const int32 dstArrayIndex = dstSubresource / dstTextureVulkan->MipLevels();
    const int32 srcMipIndex = srcSubresource % srcTextureVulkan->MipLevels();
    const int32 srcArrayIndex = srcSubresource / srcTextureVulkan->MipLevels();
    int32 mipWidth, mipHeight, mipDepth;
    srcTextureVulkan->GetMipSize(srcMipIndex, mipWidth, mipHeight, mipDepth);

    // Copy
    VkImageCopy region;
    Platform::MemoryClear(&region, sizeof(VkBufferImageCopy));
    region.extent.width = mipWidth;
    region.extent.height = mipHeight;
    region.extent.depth = mipDepth;
    region.dstOffset.x = dstX;
    region.dstOffset.y = dstY;
    region.dstOffset.z = dstZ;
    region.srcSubresource.baseArrayLayer = srcArrayIndex;
    region.srcSubresource.layerCount = 1;
    region.srcSubresource.mipLevel = srcMipIndex;
    region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.dstSubresource.baseArrayLayer = dstArrayIndex;
    region.dstSubresource.layerCount = 1;
    region.dstSubresource.mipLevel = dstMipIndex;
    region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vkCmdCopyImage(cmdBuffer->GetHandle(), srcTextureVulkan->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstTextureVulkan->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void GPUContextVulkan::ResetCounter(GPUBuffer* buffer)
{
    ASSERT(buffer);
    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();

    const auto bufferVulkan = static_cast<GPUBufferVulkan*>(buffer);
    const auto counter = bufferVulkan->Counter;
    ASSERT(counter != nullptr);

    AddBufferBarrier(counter, VK_ACCESS_TRANSFER_WRITE_BIT);
    FlushBarriers();

    uint32 value = 0;
    vkCmdUpdateBuffer(cmdBuffer->GetHandle(), counter->GetHandle(), 0, 4, &value);
}

void GPUContextVulkan::CopyCounter(GPUBuffer* dstBuffer, uint32 dstOffset, GPUBuffer* srcBuffer)
{
    ASSERT(dstBuffer && srcBuffer);
    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();

    const auto dstBufferVulkan = static_cast<GPUBufferVulkan*>(dstBuffer);
    const auto srcBufferVulkan = static_cast<GPUBufferVulkan*>(srcBuffer);
    const auto counter = srcBufferVulkan->Counter;
    ASSERT(counter != nullptr);

    AddBufferBarrier(dstBufferVulkan, VK_ACCESS_TRANSFER_WRITE_BIT);
    AddBufferBarrier(counter, VK_ACCESS_TRANSFER_READ_BIT);
    FlushBarriers();

    VkBufferCopy bufferCopy;
    bufferCopy.srcOffset = 0;
    bufferCopy.dstOffset = dstOffset;
    bufferCopy.size = 4;
    vkCmdCopyBuffer(cmdBuffer->GetHandle(), srcBufferVulkan->GetHandle(), dstBufferVulkan->GetHandle(), 1, &bufferCopy);
}

void GPUContextVulkan::CopyResource(GPUResource* dstResource, GPUResource* srcResource)
{
    ASSERT(dstResource && srcResource);
    const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();

    // Ensure to end active render pass
    if (cmdBuffer->IsInsideRenderPass())
        EndRenderPass();

    auto dstTextureVulkan = static_cast<GPUTextureVulkan*>(dstResource);
    auto srcTextureVulkan = static_cast<GPUTextureVulkan*>(srcResource);

    auto dstBufferVulkan = static_cast<GPUBufferVulkan*>(dstResource);
    auto srcBufferVulkan = static_cast<GPUBufferVulkan*>(srcResource);

    const auto srcType = srcResource->GetObjectType();
    const auto dstType = dstResource->GetObjectType();

    // Buffer -> Buffer
    if (srcType == GPUResource::ObjectType::Buffer && dstType == GPUResource::ObjectType::Buffer)
    {
        // Transition resources
        AddBufferBarrier(dstBufferVulkan, VK_ACCESS_TRANSFER_WRITE_BIT);
        AddBufferBarrier(srcBufferVulkan, VK_ACCESS_TRANSFER_READ_BIT);
        FlushBarriers();

        // Copy
        VkBufferCopy bufferCopy;
        bufferCopy.srcOffset = 0;
        bufferCopy.dstOffset = 0;
        bufferCopy.size = srcBufferVulkan->GetSize();
        ASSERT(bufferCopy.size == dstBufferVulkan->GetSize());
        vkCmdCopyBuffer(cmdBuffer->GetHandle(), srcBufferVulkan->GetHandle(), dstBufferVulkan->GetHandle(), 1, &bufferCopy);
    }
        // Texture -> Texture
    else if (srcType == GPUResource::ObjectType::Texture && dstType == GPUResource::ObjectType::Texture)
    {
        if (dstTextureVulkan->IsStaging())
        {
            // Staging Texture -> Staging Texture
            if (srcTextureVulkan->IsStaging())
            {
                ASSERT(dstTextureVulkan->StagingBuffer && srcTextureVulkan->StagingBuffer);
                CopyResource(dstTextureVulkan->StagingBuffer, srcTextureVulkan->StagingBuffer);
            }
                // Texture -> Staging Texture
            else
            {
                // Transition resources
                ASSERT(dstTextureVulkan->StagingBuffer);
                AddBufferBarrier(dstTextureVulkan->StagingBuffer, VK_ACCESS_TRANSFER_WRITE_BIT);
                AddImageBarrier(srcTextureVulkan, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
                FlushBarriers();

                // Copy
                int32 copyOffset = 0;
                const int32 arraySize = srcTextureVulkan->ArraySize();
                const int32 mipMaps = srcTextureVulkan->MipLevels();
                for (int32 arraySlice = 0; arraySlice < arraySize; arraySlice++)
                {
                    VkBufferImageCopy regions[GPU_MAX_TEXTURE_MIP_LEVELS];
                    uint32_t mipWidth = srcTextureVulkan->Width();
                    uint32_t mipHeight = srcTextureVulkan->Height();
                    uint32_t mipDepth = srcTextureVulkan->Depth();

                    for (int32 mipLevel = 0; mipLevel < mipMaps; mipLevel++)
                    {
                        VkBufferImageCopy& region = regions[mipLevel];
                        region.bufferOffset = copyOffset;
                        region.bufferRowLength = mipWidth;
                        region.bufferImageHeight = mipHeight;
                        region.imageOffset = { 0, 0, 0 };
                        region.imageExtent = { mipWidth, mipHeight, mipDepth };
                        region.imageSubresource.baseArrayLayer = arraySlice;
                        region.imageSubresource.layerCount = 1;
                        region.imageSubresource.mipLevel = mipLevel;
                        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

                        // TODO: pitch/slice alignment on Vulkan?
                        copyOffset += dstTextureVulkan->ComputeSubresourceSize(mipLevel, 1, 1);

                        if (mipWidth != 1)
                            mipWidth >>= 1;
                        if (mipHeight != 1)
                            mipHeight >>= 1;
                        if (mipDepth != 1)
                            mipDepth >>= 1;
                    }

                    vkCmdCopyImageToBuffer(cmdBuffer->GetHandle(), srcTextureVulkan->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstTextureVulkan->StagingBuffer->GetHandle(), mipMaps, regions);
                }
            }
        }
        else
        {
            // Transition resources
            AddImageBarrier(dstTextureVulkan, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            AddImageBarrier(srcTextureVulkan, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            FlushBarriers();

            // Copy
            const int32 arraySize = srcTextureVulkan->ArraySize();
            const int32 mipMaps = srcTextureVulkan->MipLevels();
            ASSERT(dstTextureVulkan->MipLevels() == mipMaps);
            VkImageCopy regions[GPU_MAX_TEXTURE_MIP_LEVELS];
            uint32_t mipWidth = srcTextureVulkan->Width();
            uint32_t mipHeight = srcTextureVulkan->Height();
            uint32_t mipDepth = srcTextureVulkan->Depth();
            for (int32 mipLevel = 0; mipLevel < mipMaps; mipLevel++)
            {
                VkImageCopy& region = regions[mipLevel];
                region.extent = { mipWidth, mipHeight, mipDepth };
                region.srcOffset = { 0, 0, 0 };
                region.srcSubresource.baseArrayLayer = 0;
                region.srcSubresource.layerCount = arraySize;
                region.srcSubresource.mipLevel = mipLevel;
                region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.dstOffset = { 0, 0, 0 };
                region.dstSubresource.baseArrayLayer = 0;
                region.dstSubresource.layerCount = arraySize;
                region.dstSubresource.mipLevel = mipLevel;
                region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

                if (mipWidth != 1)
                    mipWidth >>= 1;
                if (mipHeight != 1)
                    mipHeight >>= 1;
                if (mipDepth != 1)
                    mipDepth >>= 1;
            }
            vkCmdCopyImage(cmdBuffer->GetHandle(), srcTextureVulkan->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstTextureVulkan->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipMaps, regions);
        }
    }
    else
    {
        // TODO: implement it
        MISSING_CODE("GPUContextVulkan::CopyResource");
    }
}

void GPUContextVulkan::CopySubresource(GPUResource* dstResource, uint32 dstSubresource, GPUResource* srcResource, uint32 srcSubresource)
{
    // TODO: implement it
    MISSING_CODE("GPUContextVulkan::CopySubresource");
}

#endif
