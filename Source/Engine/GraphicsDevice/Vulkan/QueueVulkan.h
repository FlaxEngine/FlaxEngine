// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "IncludeVulkanHeaders.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Platform/CriticalSection.h"

#if GRAPHICS_API_VULKAN

class GPUDeviceVulkan;
class CmdBufferVulkan;

/// <summary>
/// Implementation of the command buffer queue for the Vulkan backend.
/// </summary>
class QueueVulkan
{
private:
    VkQueue _queue;
    uint32 _familyIndex;
    uint32 _queueIndex;
    GPUDeviceVulkan* _device;
    CriticalSection _locker;
    CmdBufferVulkan* _lastSubmittedCmdBuffer;
    uint64 _lastSubmittedCmdBufferFenceCounter;

public:
    QueueVulkan(GPUDeviceVulkan* device, uint32 familyIndex);

    inline uint32 GetFamilyIndex() const
    {
        return _familyIndex;
    }

    void Submit(CmdBufferVulkan* cmdBuffer, uint32 signalSemaphoresCount = 0, const VkSemaphore* signalSemaphores = nullptr);

    inline void Submit(CmdBufferVulkan* cmdBuffer, VkSemaphore signalSemaphore)
    {
        Submit(cmdBuffer, 1, &signalSemaphore);
    }

    inline VkQueue GetHandle() const
    {
        return _queue;
    }

    void GetLastSubmittedInfo(CmdBufferVulkan*& cmdBuffer, uint64& fenceCounter) const;

private:
    void UpdateLastSubmittedCommandBuffer(CmdBufferVulkan* cmdBuffer);
};

#endif
