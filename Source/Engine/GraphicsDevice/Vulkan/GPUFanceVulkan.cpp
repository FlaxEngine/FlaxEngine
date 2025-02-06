// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX11

#include "GPUFanceVulkan.h"
#include "GPUContextVulkan.h"
#include "QueueVulkan.h"

GPUFenceVulkan::GPUFenceVulkan(GPUDeviceVulkan* device, const StringView& name) : 
    GPUFence(), _device(device)
{
    // Create Vulkan fence
    VkDevice vkDevice = _device->Device;

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = 0;  // No special flags

    // Create the fence object
    if (vkCreateFence(vkDevice, &fenceCreateInfo, nullptr, &fence) != VK_SUCCESS) {
        fence = VK_NULL_HANDLE;
        return;
    }
}
void GPUFenceVulkan::Signal()
{
    if (fence == VK_NULL_HANDLE)
        return;

    // Signal the fence after submitting commands
    VkQueue commandQueue = _device->MainContext->GetQueue()->GetHandle();

    // Increment the fence value
    fenceValue++;

    // Submit the fence signal
    // Note: Actual submission code of command buffers is omitted
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.signalSemaphoreCount = 0;

    // Assuming the fence is signaled after the command buffer is submitted
    vkQueueSubmit(commandQueue, 0, nullptr, fence);

    SignalCalled = true;
}
void GPUFenceVulkan::Wait()
{
    // Only wait if Signal() has been called
    if (!SignalCalled || fence == VK_NULL_HANDLE)
        return;

    // Wait until the GPU has finished processing the fence signal
    VkDevice vkDevice = _device->Device;

    while (true) {
        VkResult result = vkWaitForFences(vkDevice, 1, &fence, VK_TRUE, UINT64_MAX);
        if (result == VK_SUCCESS) {
            break;  // Fence has been signaled
        }
        Platform::Sleep(1);  // Poll until the fence is signaled
    }
}

GPUFenceVulkan::~GPUFenceVulkan()
{
    if (fence != VK_NULL_HANDLE) {
        VkDevice vkDevice = _device->Device;
        vkDestroyFence(vkDevice, fence, nullptr);
    }
}

#endif
