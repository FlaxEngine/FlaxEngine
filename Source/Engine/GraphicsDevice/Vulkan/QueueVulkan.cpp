// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN

#include "QueueVulkan.h"
#include "GPUDeviceVulkan.h"
#include "CmdBufferVulkan.h"
#include "RenderToolsVulkan.h"

QueueVulkan::QueueVulkan(GPUDeviceVulkan* device, uint32 familyIndex)
    : _queue(VK_NULL_HANDLE)
    , _familyIndex(familyIndex)
    , _queueIndex(0)
    , _device(device)
    , _lastSubmittedCmdBuffer(nullptr)
    , _lastSubmittedCmdBufferFenceCounter(0)
{
    vkGetDeviceQueue(device->Device, familyIndex, 0, &_queue);
}

void QueueVulkan::Submit(CmdBufferVulkan* cmdBuffer, uint32 signalSemaphoresCount, const VkSemaphore* signalSemaphores)
{
    ASSERT(cmdBuffer->HasEnded());
    auto fence = cmdBuffer->GetFence();
    ASSERT(!fence->IsSignaled);

    const VkCommandBuffer cmdBuffers[] = { cmdBuffer->GetHandle() };

    VkSubmitInfo submitInfo;
    RenderToolsVulkan::ZeroStruct(submitInfo, VK_STRUCTURE_TYPE_SUBMIT_INFO);
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = cmdBuffers;
    submitInfo.signalSemaphoreCount = signalSemaphoresCount;
    submitInfo.pSignalSemaphores = signalSemaphores;

    Array<VkSemaphore, InlinedAllocation<8>> waitSemaphores;
    if (cmdBuffer->_waitSemaphores.HasItems())
    {
        waitSemaphores.EnsureCapacity((uint32)cmdBuffer->_waitSemaphores.Count());
        for (auto semaphore : cmdBuffer->_waitSemaphores)
        {
            waitSemaphores.Add(semaphore->GetHandle());
        }
        submitInfo.waitSemaphoreCount = (uint32)cmdBuffer->_waitSemaphores.Count();
        submitInfo.pWaitSemaphores = waitSemaphores.Get();
        submitInfo.pWaitDstStageMask = cmdBuffer->_waitFlags.Get();
    }

    VALIDATE_VULKAN_RESULT(vkQueueSubmit(_queue, 1, &submitInfo, fence->Handle));

    // Mark semaphores as submitted
    cmdBuffer->_state = CmdBufferVulkan::State::Submitted;
    cmdBuffer->_waitFlags.Clear();
    cmdBuffer->_submittedWaitSemaphores = cmdBuffer->_waitSemaphores;
    cmdBuffer->_waitSemaphores.Clear();
    cmdBuffer->_submittedFenceCounter = cmdBuffer->_fenceSignaledCounter;

#if 0
	// Wait for the GPU to be idle on every submit (useful for tracking GPU hangs)
	const bool WaitForIdleOnSubmit = false;
	if (WaitForIdleOnSubmit)
	{
		// Use 200ms timeout
		bool success = _device->FenceManager.WaitForFence(fence, 200 * 1000 * 1000);
		ASSERT(success);
		ASSERT(_device->FenceManager.IsFenceSignaled(fence));
		cmdBuffer->GetOwner()->RefreshFenceStatus();
	}
#endif

    UpdateLastSubmittedCommandBuffer(cmdBuffer);

    cmdBuffer->GetOwner()->RefreshFenceStatus(cmdBuffer);
}

void QueueVulkan::GetLastSubmittedInfo(CmdBufferVulkan*& cmdBuffer, uint64& fenceCounter) const
{
    _locker.Lock();
    cmdBuffer = _lastSubmittedCmdBuffer;
    fenceCounter = _lastSubmittedCmdBufferFenceCounter;
    _locker.Unlock();
}

void QueueVulkan::UpdateLastSubmittedCommandBuffer(CmdBufferVulkan* cmdBuffer)
{
    _locker.Lock();
    _lastSubmittedCmdBuffer = cmdBuffer;
    _lastSubmittedCmdBufferFenceCounter = cmdBuffer->GetFenceSignaledCounter();
    _locker.Unlock();
}

#endif
