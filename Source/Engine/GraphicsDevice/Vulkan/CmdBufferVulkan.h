// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "GPUDeviceVulkan.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Collections/Array.h"

#if GRAPHICS_API_VULKAN

class GPUDeviceVulkan;
class CmdBufferPoolVulkan;
class QueueVulkan;
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
class DescriptorPoolSetContainerVulkan;
#endif

/// <summary>
/// Implementation of the command buffer for the Vulkan backend.
/// </summary>
class CmdBufferVulkan
{
    friend QueueVulkan;

public:

    enum class State
    {
        ReadyForBegin,
        IsInsideBegin,
        IsInsideRenderPass,
        HasEnded,
        Submitted,
    };

private:

    GPUDeviceVulkan* _device;
    VkCommandBuffer CommandBufferHandle;
    State _state;

    Array<VkPipelineStageFlags> WaitFlags;
    Array<SemaphoreVulkan*> WaitSemaphores;
    Array<SemaphoreVulkan*> SubmittedWaitSemaphores;

    void MarkSemaphoresAsSubmitted()
    {
        WaitFlags.Clear();

        // Move to pending delete list
        SubmittedWaitSemaphores = WaitSemaphores;
        WaitSemaphores.Clear();
    }

    FenceVulkan* _fence;
#if GPU_ALLOW_PROFILE_EVENTS
    int32 _eventsBegin = 0;
#endif

    // Last value passed after the fence got signaled
    volatile uint64 FenceSignaledCounter;

    // Last value when we submitted the cmd buffer; useful to track down if something waiting for the fence has actually been submitted
    volatile uint64 SubmittedFenceCounter;

    CmdBufferPoolVulkan* CommandBufferPool;

public:

    CmdBufferVulkan(GPUDeviceVulkan* device, CmdBufferPoolVulkan* pool);

    ~CmdBufferVulkan();

public:

    CmdBufferPoolVulkan* GetOwner()
    {
        return CommandBufferPool;
    }

    State GetState()
    {
        return _state;
    }

    FenceVulkan* GetFence()
    {
        return _fence;
    }

    inline bool IsInsideRenderPass() const
    {
        return _state == State::IsInsideRenderPass;
    }

    inline bool IsOutsideRenderPass() const
    {
        return _state == State::IsInsideBegin;
    }

    inline bool HasBegun() const
    {
        return _state == State::IsInsideBegin || _state == State::IsInsideRenderPass;
    }

    inline bool HasEnded() const
    {
        return _state == State::HasEnded;
    }

    inline bool IsSubmitted() const
    {
        return _state == State::Submitted;
    }

    inline VkCommandBuffer GetHandle() const
    {
        return CommandBufferHandle;
    }

    inline volatile uint64 GetFenceSignaledCounter() const
    {
        return FenceSignaledCounter;
    }

    inline volatile uint64 GetSubmittedFenceCounter() const
    {
        return SubmittedFenceCounter;
    }

public:

    void AddWaitSemaphore(VkPipelineStageFlags waitFlags, SemaphoreVulkan* waitSemaphore);

    void Begin();
    void End();

    void BeginRenderPass(RenderPassVulkan* renderPass, FramebufferVulkan* framebuffer, uint32 clearValueCount, VkClearValue* clearValues);
    void EndRenderPass();

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
    DescriptorPoolSetContainerVulkan* CurrentDescriptorPoolSetContainer = nullptr;

    void AcquirePoolSet();
#endif

#if GPU_ALLOW_PROFILE_EVENTS
    void BeginEvent(const Char* name);
    void EndEvent();
#endif

    void RefreshFenceStatus();
};

class CmdBufferPoolVulkan
{
private:

    GPUDeviceVulkan* Device;
    VkCommandPool Handle;

    CmdBufferVulkan* Create();

    Array<CmdBufferVulkan*> CmdBuffers;

    void Create(uint32 queueFamilyIndex);
    friend class CmdBufferManagerVulkan;

public:

    CmdBufferPoolVulkan(GPUDeviceVulkan* device);

    ~CmdBufferPoolVulkan();

public:

    inline VkCommandPool GetHandle() const
    {
        ASSERT(Handle != VK_NULL_HANDLE);
        return Handle;
    }

    void RefreshFenceStatus(CmdBufferVulkan* skipCmdBuffer = nullptr);
};

class CmdBufferManagerVulkan
{
private:

    GPUDeviceVulkan* Device;
    CmdBufferPoolVulkan Pool;
    QueueVulkan* Queue;
    CmdBufferVulkan* ActiveCmdBuffer;
    Array<GPUTimerQueryVulkan*> QueriesInProgress;

public:

    CmdBufferManagerVulkan(GPUDeviceVulkan* device, GPUContextVulkan* context);

public:

    inline VkCommandPool GetHandle() const
    {
        return Pool.GetHandle();
    }

    inline CmdBufferVulkan* GetActiveCmdBuffer() const
    {
        return ActiveCmdBuffer;
    }

    inline bool HasPendingActiveCmdBuffer() const
    {
        return ActiveCmdBuffer != nullptr;
    }

    inline bool HasQueriesInProgress() const
    {
        return QueriesInProgress.Count() != 0;
    }

    void SubmitActiveCmdBuffer(SemaphoreVulkan* signalSemaphore = nullptr);

    void WaitForCmdBuffer(CmdBufferVulkan* cmdBuffer, float timeInSecondsToWait = 1.0f);

    // Update the fences of all cmd buffers except SkipCmdBuffer
    void RefreshFenceStatus(CmdBufferVulkan* skipCmdBuffer = nullptr)
    {
        Pool.RefreshFenceStatus(skipCmdBuffer);
    }

    void PrepareForNewActiveCommandBuffer();

    inline CmdBufferVulkan* GetCmdBuffer()
    {
        if (!ActiveCmdBuffer)
            PrepareForNewActiveCommandBuffer();

        return ActiveCmdBuffer;
    }

    void OnQueryBegin(GPUTimerQueryVulkan* query);
    void OnQueryEnd(GPUTimerQueryVulkan* query);
};

#endif
