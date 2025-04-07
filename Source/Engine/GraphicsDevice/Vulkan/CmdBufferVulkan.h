// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "GPUDeviceVulkan.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Collections/Array.h"

#if GRAPHICS_API_VULKAN

class GPUDeviceVulkan;
class CmdBufferPoolVulkan;
class QueueVulkan;
class DescriptorPoolSetContainerVulkan;

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
    VkCommandBuffer _commandBuffer;
    State _state;

    Array<VkPipelineStageFlags> _waitFlags;
    Array<SemaphoreVulkan*> _waitSemaphores;
    Array<SemaphoreVulkan*> _submittedWaitSemaphores;

    FenceVulkan* _fence;
#if GPU_ALLOW_PROFILE_EVENTS
    int32 _eventsBegin = 0;
#endif

    // The latest value when command buffer was submitted.
    volatile uint64 _submittedFenceCounter;

    // The latest value passed after the fence was signaled.
    volatile uint64 _fenceSignaledCounter;

    CmdBufferPoolVulkan* _commandBufferPool;

    DescriptorPoolSetContainerVulkan* _descriptorPoolSetContainer = nullptr;

public:
    CmdBufferVulkan(GPUDeviceVulkan* device, CmdBufferPoolVulkan* pool);
    ~CmdBufferVulkan();

public:
    CmdBufferPoolVulkan* GetOwner() const
    {
        return _commandBufferPool;
    }

    State GetState() const
    {
        return _state;
    }

    FenceVulkan* GetFence() const
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
        return _commandBuffer;
    }

    inline volatile uint64 GetFenceSignaledCounter() const
    {
        return _fenceSignaledCounter;
    }

    inline volatile uint64 GetSubmittedFenceCounter() const
    {
        return _submittedFenceCounter;
    }

public:
    void AddWaitSemaphore(VkPipelineStageFlags waitFlags, SemaphoreVulkan* waitSemaphore);

    void Begin();
    void End();

    void BeginRenderPass(RenderPassVulkan* renderPass, FramebufferVulkan* framebuffer, uint32 clearValueCount, VkClearValue* clearValues);
    void EndRenderPass();

    FORCE_INLINE DescriptorPoolSetContainerVulkan* GetDescriptorPoolSet() const
    {
        return _descriptorPoolSetContainer;
    }

#if GPU_ALLOW_PROFILE_EVENTS
    void BeginEvent(const Char* name);
    void EndEvent();
#endif

    void RefreshFenceStatus();
};

class CmdBufferPoolVulkan
{
    friend class CmdBufferManagerVulkan;
private:
    GPUDeviceVulkan* _device;
    VkCommandPool _handle;
    Array<CmdBufferVulkan*> _cmdBuffers;

    CmdBufferVulkan* Create();

    void Create(uint32 queueFamilyIndex);

public:
    CmdBufferPoolVulkan(GPUDeviceVulkan* device);
    ~CmdBufferPoolVulkan();

public:
    inline VkCommandPool GetHandle() const
    {
        return _handle;
    }

    void RefreshFenceStatus(const CmdBufferVulkan* skipCmdBuffer = nullptr);
};

class CmdBufferManagerVulkan
{
private:
    GPUDeviceVulkan* _device;
    CmdBufferPoolVulkan _pool;
    QueueVulkan* _queue;
    CmdBufferVulkan* _activeCmdBuffer;
    Array<GPUTimerQueryVulkan*> _queriesInProgress;

public:
    CmdBufferManagerVulkan(GPUDeviceVulkan* device, GPUContextVulkan* context);

public:
    FORCE_INLINE VkCommandPool GetHandle() const
    {
        return _pool.GetHandle();
    }

    FORCE_INLINE CmdBufferVulkan* GetActiveCmdBuffer() const
    {
        return _activeCmdBuffer;
    }

    FORCE_INLINE bool HasPendingActiveCmdBuffer() const
    {
        return _activeCmdBuffer != nullptr;
    }

    FORCE_INLINE bool HasQueriesInProgress() const
    {
        return _queriesInProgress.Count() != 0;
    }

    FORCE_INLINE CmdBufferVulkan* GetCmdBuffer()
    {
        if (!_activeCmdBuffer)
            PrepareForNewActiveCommandBuffer();
        return _activeCmdBuffer;
    }

public:
    void SubmitActiveCmdBuffer(SemaphoreVulkan* signalSemaphore = nullptr);
    void WaitForCmdBuffer(CmdBufferVulkan* cmdBuffer, float timeInSecondsToWait = 1.0f);
    void RefreshFenceStatus(CmdBufferVulkan* skipCmdBuffer = nullptr)
    {
        _pool.RefreshFenceStatus(skipCmdBuffer);
    }
    void PrepareForNewActiveCommandBuffer();

    void OnQueryBegin(GPUTimerQueryVulkan* query);
    void OnQueryEnd(GPUTimerQueryVulkan* query);
};

#endif
