// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN

#include "CmdBufferVulkan.h"
#include "RenderToolsVulkan.h"
#include "QueueVulkan.h"
#include "GPUContextVulkan.h"
#if VULKAN_USE_QUERIES
#include "GPUTimerQueryVulkan.h"
#endif
#include "DescriptorSetVulkan.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Profiler/ProfilerCPU.h"

void CmdBufferVulkan::AddWaitSemaphore(VkPipelineStageFlags waitFlags, SemaphoreVulkan* waitSemaphore)
{
    _waitFlags.Add(waitFlags);
    ASSERT(!_waitSemaphores.Contains(waitSemaphore));
    _waitSemaphores.Add(waitSemaphore);
}

void CmdBufferVulkan::Begin()
{
    PROFILE_CPU();
    ASSERT(_state == State::ReadyForBegin);

    VkCommandBufferBeginInfo beginInfo;
    RenderToolsVulkan::ZeroStruct(beginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VALIDATE_VULKAN_RESULT(vkBeginCommandBuffer(_commandBuffer, &beginInfo));

    // Acquire a descriptor pool set on
    if (_descriptorPoolSetContainer == nullptr)
    {
        _descriptorPoolSetContainer = _device->DescriptorPoolsManager->AcquirePoolSetContainer();
    }

    _state = State::IsInsideBegin;

#if GPU_ALLOW_PROFILE_EVENTS
    // Reset events counter
    _eventsBegin = 0;
#endif
}

void CmdBufferVulkan::End()
{
    PROFILE_CPU();
    ASSERT(IsOutsideRenderPass());

#if GPU_ALLOW_PROFILE_EVENTS && VK_EXT_debug_utils
    // End remaining events
    while (_eventsBegin--)
        vkCmdEndDebugUtilsLabelEXT(GetHandle());
#endif

    VALIDATE_VULKAN_RESULT(vkEndCommandBuffer(GetHandle()));
    _state = State::HasEnded;
}

void CmdBufferVulkan::BeginRenderPass(RenderPassVulkan* renderPass, FramebufferVulkan* framebuffer, uint32 clearValueCount, VkClearValue* clearValues)
{
    ASSERT(IsOutsideRenderPass());
    VkRenderPassBeginInfo info;
    RenderToolsVulkan::ZeroStruct(info, VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
    info.renderPass = renderPass->Handle;
    info.framebuffer = framebuffer->Handle;
    info.renderArea.offset.x = 0;
    info.renderArea.offset.y = 0;
    info.renderArea.extent.width = framebuffer->Extent.width;
    info.renderArea.extent.height = framebuffer->Extent.height;
    info.clearValueCount = clearValueCount;
    info.pClearValues = clearValues;
    vkCmdBeginRenderPass(_commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    _state = State::IsInsideRenderPass;
}

void CmdBufferVulkan::EndRenderPass()
{
    ASSERT(IsInsideRenderPass());
    vkCmdEndRenderPass(_commandBuffer);
    _state = State::IsInsideBegin;
}

#if GPU_ALLOW_PROFILE_EVENTS

void CmdBufferVulkan::BeginEvent(const Char* name)
{
#if VK_EXT_debug_utils
    if (!vkCmdBeginDebugUtilsLabelEXT)
        return;

    _eventsBegin++;

    // Convert to ANSI
    char buffer[101];
    int32 i = 0;
    while (i < 100 && name[i])
    {
        buffer[i] = (char)name[i];
        i++;
    }
    buffer[i] = 0;

    VkDebugUtilsLabelEXT label;
    RenderToolsVulkan::ZeroStruct(label, VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT);
    label.pLabelName = buffer;
    vkCmdBeginDebugUtilsLabelEXT(GetHandle(), &label);
#endif
}

void CmdBufferVulkan::EndEvent()
{
#if VK_EXT_debug_utils
    if (_eventsBegin == 0 || !vkCmdEndDebugUtilsLabelEXT)
        return;
    _eventsBegin--;

    vkCmdEndDebugUtilsLabelEXT(GetHandle());
#endif
}

#endif

void CmdBufferVulkan::RefreshFenceStatus()
{
    if (_state == State::Submitted)
    {
        PROFILE_CPU();
        if (_device->FenceManager.IsFenceSignaled(_fence))
        {
            _state = State::ReadyForBegin;
            _submittedWaitSemaphores.Clear();

            vkResetCommandBuffer(_commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
            _device->FenceManager.ResetFence(_fence);
            _fenceSignaledCounter++;

            if (_descriptorPoolSetContainer)
            {
                _descriptorPoolSetContainer->LastFrameUsed = Engine::FrameCount;
                _descriptorPoolSetContainer = nullptr;
            }
        }
    }
    else
    {
        ASSERT(!_fence->IsSignaled);
    }
}

CmdBufferVulkan::CmdBufferVulkan(GPUDeviceVulkan* device, CmdBufferPoolVulkan* pool)
    : _device(device)
    , _commandBuffer(VK_NULL_HANDLE)
    , _state(State::ReadyForBegin)
    , _fence(nullptr)
    , _submittedFenceCounter(0)
    , _fenceSignaledCounter(0)
    , _commandBufferPool(pool)
{
    VkCommandBufferAllocateInfo createCmdBufInfo;
    RenderToolsVulkan::ZeroStruct(createCmdBufInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
    createCmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    createCmdBufInfo.commandBufferCount = 1;
    createCmdBufInfo.commandPool = _commandBufferPool->GetHandle();
    VALIDATE_VULKAN_RESULT(vkAllocateCommandBuffers(_device->Device, &createCmdBufInfo, &_commandBuffer));
    _fence = _device->FenceManager.AllocateFence();
}

CmdBufferVulkan::~CmdBufferVulkan()
{
    auto& fenceManager = _device->FenceManager;
    if (_state == State::Submitted)
    {
        // Wait 60ms
        const uint64 waitForCmdBufferInNanoSeconds = 60 * 1000 * 1000LL;
        fenceManager.WaitAndReleaseFence(_fence, waitForCmdBufferInNanoSeconds);
    }
    else
    {
        // Just free the fence, command buffer was not submitted
        fenceManager.ReleaseFence(_fence);
    }

    vkFreeCommandBuffers(_device->Device, _commandBufferPool->GetHandle(), 1, &_commandBuffer);
}

CmdBufferVulkan* CmdBufferPoolVulkan::Create()
{
    const auto cmdBuffer = New<CmdBufferVulkan>(_device, this);
    _cmdBuffers.Add(cmdBuffer);
    return cmdBuffer;
}

void CmdBufferPoolVulkan::Create(uint32 queueFamilyIndex)
{
    VkCommandPoolCreateInfo poolInfo;
    RenderToolsVulkan::ZeroStruct(poolInfo, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    // TODO: use VK_COMMAND_POOL_CREATE_TRANSIENT_BIT?
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VALIDATE_VULKAN_RESULT(vkCreateCommandPool(_device->Device, &poolInfo, nullptr, &_handle));
}

CmdBufferPoolVulkan::CmdBufferPoolVulkan(GPUDeviceVulkan* device)
    : _device(device)
    , _handle(VK_NULL_HANDLE)
{
}

CmdBufferPoolVulkan::~CmdBufferPoolVulkan()
{
    for (int32 i = 0; i < _cmdBuffers.Count(); i++)
        Delete(_cmdBuffers[i]);
    vkDestroyCommandPool(_device->Device, _handle, nullptr);
}

void CmdBufferPoolVulkan::RefreshFenceStatus(const CmdBufferVulkan* skipCmdBuffer)
{
    for (int32 i = 0; i < _cmdBuffers.Count(); i++)
    {
        auto cmdBuffer = _cmdBuffers[i];
        if (cmdBuffer != skipCmdBuffer)
        {
            cmdBuffer->RefreshFenceStatus();
        }
    }
}

CmdBufferManagerVulkan::CmdBufferManagerVulkan(GPUDeviceVulkan* device, GPUContextVulkan* context)
    : _device(device)
    , _pool(device)
    , _queue(context->GetQueue())
    , _activeCmdBuffer(nullptr)
{
    _pool.Create(_queue->GetFamilyIndex());
}

void CmdBufferManagerVulkan::SubmitActiveCmdBuffer(SemaphoreVulkan* signalSemaphore)
{
    PROFILE_CPU();
    ASSERT(_activeCmdBuffer);
    if (!_activeCmdBuffer->IsSubmitted() && _activeCmdBuffer->HasBegun())
    {
        if (_activeCmdBuffer->IsInsideRenderPass())
            _activeCmdBuffer->EndRenderPass();

#if VULKAN_USE_QUERIES
        // Pause all active queries
        for (int32 i = 0; i < _queriesInProgress.Count(); i++)
        {
            _queriesInProgress.Get()[i]->Interrupt(_activeCmdBuffer);
        }
#endif

        _activeCmdBuffer->End();

        if (signalSemaphore)
        {
            _queue->Submit(_activeCmdBuffer, signalSemaphore->GetHandle());
        }
        else
        {
            _queue->Submit(_activeCmdBuffer);
        }
    }
    _activeCmdBuffer = nullptr;
}

void CmdBufferManagerVulkan::WaitForCmdBuffer(CmdBufferVulkan* cmdBuffer, float timeInSecondsToWait)
{
    PROFILE_CPU();
    ASSERT(cmdBuffer->IsSubmitted());
    const bool failed = _device->FenceManager.WaitForFence(cmdBuffer->GetFence(), (uint64)(timeInSecondsToWait * 1e9));
    ASSERT(!failed);
    cmdBuffer->RefreshFenceStatus();
}

void CmdBufferManagerVulkan::PrepareForNewActiveCommandBuffer()
{
    PROFILE_CPU();
    ASSERT_LOW_LAYER(_activeCmdBuffer == nullptr)
    for (int32 i = 0; i < _pool._cmdBuffers.Count(); i++)
    {
        auto cmdBuffer = _pool._cmdBuffers.Get()[i];
        cmdBuffer->RefreshFenceStatus();
        if (cmdBuffer->GetState() == CmdBufferVulkan::State::ReadyForBegin)
        {
            _activeCmdBuffer = cmdBuffer;
            break;
        }
        else
        {
            ASSERT(cmdBuffer->GetState() == CmdBufferVulkan::State::Submitted);
        }
    }

    if (_activeCmdBuffer == nullptr)
    {
        // Always begin fresh command buffer for rendering
        _activeCmdBuffer = _pool.Create();
    }

    _activeCmdBuffer->Begin();

#if VULKAN_USE_QUERIES
    // Resume any paused queries with the new command buffer
    for (int32 i = 0; i < _queriesInProgress.Count(); i++)
    {
        _queriesInProgress.Get()[i]->Resume(_activeCmdBuffer);
    }
#endif
}

void CmdBufferManagerVulkan::OnQueryBegin(GPUTimerQueryVulkan* query)
{
#if VULKAN_USE_QUERIES
    _queriesInProgress.Add(query);
#endif
}

void CmdBufferManagerVulkan::OnQueryEnd(GPUTimerQueryVulkan* query)
{
#if VULKAN_USE_QUERIES
    _queriesInProgress.Remove(query);
#endif
}

#endif
