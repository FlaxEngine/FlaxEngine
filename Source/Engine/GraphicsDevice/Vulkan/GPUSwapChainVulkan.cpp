// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN

#include "GPUSwapChainVulkan.h"
#include "RenderToolsVulkan.h"
#include "QueueVulkan.h"
#include "GPUAdapterVulkan.h"
#include "GPUContextVulkan.h"
#include "CmdBufferVulkan.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Scripting/Enums.h"
#include "Engine/Profiler/ProfilerCPU.h"

void BackBufferVulkan::Setup(GPUSwapChainVulkan* window, VkImage backbuffer, PixelFormat format, VkExtent3D extent)
{
    // Cache handle and set default initial state for the backbuffers
    initResource(VK_IMAGE_LAYOUT_UNDEFINED);

    Device = window->GetDevice();
    Handle.Init(window->GetDevice(), this, backbuffer, 1, format, MSAALevel::None, extent, VK_IMAGE_VIEW_TYPE_2D);
    RenderingDoneSemaphore = New<SemaphoreVulkan>(Device);
    ImageAcquiredSemaphore = New<SemaphoreVulkan>(Device);
}

void BackBufferVulkan::Release()
{
    Handle.Release();
    Delete(RenderingDoneSemaphore);
    Delete(ImageAcquiredSemaphore);
}

GPUSwapChainVulkan::GPUSwapChainVulkan(GPUDeviceVulkan* device, Window* window)
    : GPUResourceVulkan(device, StringView::Empty)
    , _surface(VK_NULL_HANDLE)
    , _swapChain(VK_NULL_HANDLE)
    , _currentImageIndex(-1)
    , _semaphoreIndex(0)
    , _acquiredImageIndex(-1)
    , _acquiredSemaphore(nullptr)
{
    _window = window;
}

void GPUSwapChainVulkan::ReleaseBackBuffer()
{
    for (int32 i = 0; i < _backBuffers.Count(); i++)
    {
        _backBuffers[i].Release();
    }
    _backBuffers.Clear();
}

void GPUSwapChainVulkan::OnReleaseGPU()
{
    GPUDeviceLock lock(_device);

    _device->WaitForGPU();

    ReleaseBackBuffer();

    // Release data
    _currentImageIndex = -1;
    _semaphoreIndex = 0;
    _acquiredImageIndex = -1;
    _acquiredSemaphore = nullptr;
    if (_swapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(_device->Device, _swapChain, nullptr);
        _swapChain = VK_NULL_HANDLE;
    }
    if (_surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(GPUDeviceVulkan::Instance, _surface, nullptr);
        _surface = VK_NULL_HANDLE;
    }
    _width = _height = 0;
}

bool GPUSwapChainVulkan::IsFullscreen()
{
#if PLATFORM_ANDROID || PLATFORM_SWITCH || PLATFORM_IOS
    // Not supported
    return true;
#else
    return false;
#endif
}

void GPUSwapChainVulkan::SetFullscreen(bool isFullscreen)
{
#if PLATFORM_ANDROID || PLATFORM_SWITCH || PLATFORM_IOS
    // Not supported
#else
    if (!_surface)
        return;
    // TODO: support fullscreen mode on Vulkan
    MISSING_CODE("support fullscreen mode on Vulkan");
#endif
}

GPUTextureView* GPUSwapChainVulkan::GetBackBufferView()
{
    if (_acquiredImageIndex == -1)
    {
        PROFILE_CPU();
        if (TryPresent(DoAcquireImageIndex) < 0)
        {
            LOG(Fatal, "Swapchain acquire image index failed!");
        }
        ASSERT(_acquiredImageIndex != -1);

        auto context = _device->MainContext;
        const auto backBuffer = &_backBuffers[_acquiredImageIndex].Handle;

        auto cmdBufferManager = context->GetCmdBufferManager();
        auto cmdBuffer = cmdBufferManager->GetCmdBuffer();

        // Transition to render target (typical usage in most cases when calling backbuffer getter)
        context->AddImageBarrier(backBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        context->FlushBarriers();

        // Submit here so we can add a dependency with the acquired semaphore
        cmdBuffer->AddWaitSemaphore(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, _acquiredSemaphore);
        cmdBufferManager->SubmitActiveCmdBuffer();
        cmdBufferManager->PrepareForNewActiveCommandBuffer();
        ASSERT(cmdBufferManager->HasPendingActiveCmdBuffer() && cmdBufferManager->GetActiveCmdBuffer()->GetState() == CmdBufferVulkan::State::IsInsideBegin);
    }
    return &_backBuffers[_acquiredImageIndex].Handle;
}

bool GPUSwapChainVulkan::Resize(int32 width, int32 height)
{
    // Check if size won't change
    if (width == _width && height == _height)
        return false;

    // Wait for GPU to flush commands
    _device->WaitForGPU();

    return CreateSwapChain(width, height);
}

void GPUSwapChainVulkan::CopyBackbuffer(GPUContext* context, GPUTexture* dst)
{
    const auto contextVulkan = (GPUContextVulkan*)context;

    const auto dstVulkan = (GPUTextureVulkan*)dst;
    const auto backBuffer = (GPUTextureViewVulkan*)GetBackBufferView();

    contextVulkan->AddImageBarrier(dstVulkan, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    contextVulkan->AddImageBarrier(backBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    contextVulkan->FlushBarriers();

    ASSERT(dstVulkan->MipLevels() == 1 && dstVulkan->ArraySize() == 1 && dstVulkan->Format() == _format);
    VkImageCopy region;
    region.extent = { (uint32_t)dstVulkan->Width(), (uint32_t)dstVulkan->Height(), 1 };
    region.srcOffset = { 0, 0, 0 };
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount = 1;
    region.srcSubresource.mipLevel = 0;
    region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.dstOffset = { 0, 0, 0 };
    region.dstSubresource.baseArrayLayer = 0;
    region.dstSubresource.layerCount = 1;
    region.dstSubresource.mipLevel = 0;
    region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vkCmdCopyImage(contextVulkan->GetCmdBufferManager()->GetCmdBuffer()->GetHandle(), backBuffer->Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstVulkan->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

bool GPUSwapChainVulkan::CreateSwapChain(int32 width, int32 height)
{
    // Skip if window handle is missing (eg. Android window is not yet visible)
    auto windowHandle = _window->GetNativePtr();
    if (!windowHandle)
        return false;
    PROFILE_CPU();
    GPUDeviceLock lock(_device);
    const auto device = _device->Device;

    // Check if surface has been created before
    if (_surface != VK_NULL_HANDLE)
    {
        // Release previous data
        ReleaseGPU();

        // Flush removed resources
        _device->DeferredDeletionQueue.ReleaseResources(true);
    }
    ASSERT(_surface == VK_NULL_HANDLE);
    ASSERT_LOW_LAYER(_backBuffers.Count() == 0);

    // Create platform-dependent surface
    VulkanPlatform::CreateSurface(windowHandle, GPUDeviceVulkan::Instance, &_surface);
    if (_surface == VK_NULL_HANDLE)
    {
        LOG(Warning, "Failed to create Vulkan surface.");
        return true;
    }
    _memoryUsage = 1;

    const auto& gpu = _device->Adapter->Gpu;

    // Pick a format for backbuffer
    PixelFormat resultFormat = GPU_BACK_BUFFER_PIXEL_FORMAT;
    VkSurfaceFormatKHR result;
    Platform::MemoryClear(&result, sizeof(result));
    {
        uint32 surfaceFormatsCount;
        VALIDATE_VULKAN_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, _surface, &surfaceFormatsCount, nullptr));
        Array<VkSurfaceFormatKHR, InlinedAllocation<16>> surfaceFormats;
        surfaceFormats.AddZeroed(surfaceFormatsCount);
        VALIDATE_VULKAN_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, _surface, &surfaceFormatsCount, surfaceFormats.Get()));

        if (resultFormat != PixelFormat::Unknown)
        {
            bool found = false;
            if (EnumHasAllFlags(_device->FeaturesPerFormat[(int32)resultFormat].Support, FormatSupport::RenderTarget))
            {
                const VkFormat requested = RenderToolsVulkan::ToVulkanFormat(resultFormat);
                for (int32 i = 0; i < surfaceFormats.Count(); i++)
                {
                    if (surfaceFormats[i].format == requested)
                    {
                        result = surfaceFormats[i];
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    LOG(Warning, "Requested pixel format {0} not supported by this swapchain. Falling back to supported swapchain formats...", ScriptingEnum::ToString(resultFormat));
                    resultFormat = PixelFormat::Unknown;
                }
            }
            else
            {
                LOG(Warning, "Requested pixel format {0} is not supported by this Vulkan implementation", ScriptingEnum::ToString(resultFormat));
                resultFormat = PixelFormat::Unknown;
            }
        }

        if (resultFormat == PixelFormat::Unknown)
        {
            for (int32 i = 0; i < surfaceFormats.Count(); i++)
            {
                // Reverse lookup
                ASSERT(surfaceFormats[i].format != VK_FORMAT_UNDEFINED);
                for (int32 pixelFormat = 0; pixelFormat < static_cast<int32>(PixelFormat::MAX); pixelFormat++)
                {
                    if (surfaceFormats[i].format == RenderToolsVulkan::ToVulkanFormat(static_cast<PixelFormat>(pixelFormat)))
                    {
                        resultFormat = static_cast<PixelFormat>(pixelFormat);
                        result = surfaceFormats[i];
                        LOG(Info, "No swapchain format requested, picking up format {} (vk={})", ScriptingEnum::ToString(resultFormat), (int32)result.format);
                        break;
                    }
                }
                if (resultFormat != PixelFormat::Unknown)
                    break;
            }
        }

        if (resultFormat == PixelFormat::Unknown)
        {
            LOG(Warning, "Can't find a proper pixel format for the swapchain, trying to pick up the first available");
            const VkFormat format = RenderToolsVulkan::ToVulkanFormat(resultFormat);
            bool supported = false;
            for (int32 i = 0; i < surfaceFormats.Count(); i++)
            {
                if (surfaceFormats[i].format == format)
                {
                    supported = true;
                    result = surfaceFormats[i];
                    break;
                }
            }
            ASSERT(supported);
            String msg;
            for (int32 index = 0; index < surfaceFormats.Count(); index++)
            {
                msg += index == 0 ? TEXT("(") : TEXT(", ");
                msg += StringUtils::ToString((int32)surfaceFormats[index].format);
            }
            if (surfaceFormats.HasItems())
                msg += TEXT(")");
            LOG(Error, "Unable to find a pixel format for the swapchain; swapchain returned {0} Vulkan formats {1}", surfaceFormats.Count(), *msg);
        }
    }
    result.format = RenderToolsVulkan::ToVulkanFormat(resultFormat);
    _format = resultFormat;

    // Prepare present queue
    _device->SetupPresentQueue(_surface);

    // Calculate the swap chain present mode
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    {
        uint32 presentModesCount = 0;
        VALIDATE_VULKAN_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, _surface, &presentModesCount, nullptr));
        Array<VkPresentModeKHR, InlinedAllocation<4>> presentModes;
        presentModes.Resize(presentModesCount);
        VALIDATE_VULKAN_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, _surface, &presentModesCount, presentModes.Get()));
        bool foundPresentModeMailbox = false;
        bool foundPresentModeImmediate = false;
        bool foundPresentModeFifo = false;
        for (size_t i = 0; i < presentModesCount; i++)
        {
            switch (presentModes[(int32)i])
            {
            case VK_PRESENT_MODE_MAILBOX_KHR:
                foundPresentModeMailbox = true;
                break;
            case VK_PRESENT_MODE_IMMEDIATE_KHR:
                foundPresentModeImmediate = true;
                break;
            case VK_PRESENT_MODE_FIFO_KHR:
                foundPresentModeFifo = true;
                break;
            }
        }
        if (foundPresentModeMailbox)
        {
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
        else if (foundPresentModeImmediate)
        {
            presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
        else if (foundPresentModeFifo)
        {
            presentMode = VK_PRESENT_MODE_FIFO_KHR;
        }
        else
        {
            LOG(Warning, "Couldn't find desired Vulkan present mode! Using {0}", static_cast<int32>(presentModes[0]));
            presentMode = presentModes[0];
        }
    }

    // Check the surface properties and formats
    VkSurfaceCapabilitiesKHR surfProperties;
    VALIDATE_VULKAN_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, _surface, &surfProperties));
    width = Math::Clamp<int32>(width, surfProperties.minImageExtent.width, surfProperties.maxImageExtent.width);
    height = Math::Clamp<int32>(height, surfProperties.minImageExtent.height, surfProperties.maxImageExtent.height);
    if (width <= 0 || height <= 0)
    {
        LOG(Error, "Vulkan swapchain dimensions are invalid {}x{} (minImageExtent={}x{}, maxImageExtent={}x{})", width, height, surfProperties.minImageExtent.width, surfProperties.minImageExtent.height, surfProperties.maxImageExtent.width, surfProperties.maxImageExtent.height);
        return true;
    }
    ASSERT(surfProperties.minImageCount <= VULKAN_BACK_BUFFERS_COUNT_MAX);
    VkSwapchainCreateInfoKHR swapChainInfo;
    RenderToolsVulkan::ZeroStruct(swapChainInfo, VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
    swapChainInfo.surface = _surface;
    swapChainInfo.minImageCount = surfProperties.maxImageCount > 0 // A value of 0 means that there is no limit on the number of image
                                      ? Math::Min<uint32_t>(VULKAN_BACK_BUFFERS_COUNT, surfProperties.maxImageCount)
                                      : VULKAN_BACK_BUFFERS_COUNT;
    swapChainInfo.minImageCount = Math::Max<uint32_t>(swapChainInfo.minImageCount, surfProperties.minImageCount);
    swapChainInfo.minImageCount = Math::Min<uint32_t>(swapChainInfo.minImageCount, VULKAN_BACK_BUFFERS_COUNT_MAX);
    swapChainInfo.imageFormat = result.format;
    swapChainInfo.imageColorSpace = result.colorSpace;
    swapChainInfo.imageExtent.width = width;
    swapChainInfo.imageExtent.height = height;
    swapChainInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
#if GPU_USE_WINDOW_SRV
    swapChainInfo.imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
#endif
    swapChainInfo.preTransform = surfProperties.currentTransform;
    if (surfProperties.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        swapChainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapChainInfo.imageArrayLayers = 1;
    swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainInfo.presentMode = presentMode;
    swapChainInfo.clipped = VK_TRUE;
    swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    if (surfProperties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
        swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    // Create swap chain
    VkBool32 supportsPresent;
    VALIDATE_VULKAN_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, _device->PresentQueue->GetFamilyIndex(), _surface, &supportsPresent));
    ASSERT(supportsPresent);
    VALIDATE_VULKAN_RESULT(vkCreateSwapchainKHR(device, &swapChainInfo, nullptr, &_swapChain));

    // Cache data
    _width = width;
    _height = height;

    // Setup back buffers
    {
        uint32 imagesCount;
        VALIDATE_VULKAN_RESULT(vkGetSwapchainImagesKHR(device, _swapChain, &imagesCount, nullptr));
        if (imagesCount < 1 || imagesCount > VULKAN_BACK_BUFFERS_COUNT_MAX)
        {
            LOG(Warning, "Vulkan swapchain got invalid amount of backbuffers {} instead of {} (min {})", imagesCount, VULKAN_BACK_BUFFERS_COUNT, swapChainInfo.minImageCount);
            return true;
        }
        VkImage images[VULKAN_BACK_BUFFERS_COUNT_MAX];
        VALIDATE_VULKAN_RESULT(vkGetSwapchainImagesKHR(device, _swapChain, &imagesCount, images));

        _backBuffers.Resize(imagesCount);
        VkExtent3D extent;
        extent.width = _width;
        extent.height = _height;
        extent.depth = 1;
        for (uint32 i = 0; i < imagesCount; i++)
        {
            _backBuffers.Get()[i].Setup(this, images[i], _format, extent);
        }
    }

    // Estimate memory usage
    _memoryUsage = 1024 + RenderTools::CalculateTextureMemoryUsage(_format, _width, _height, 1) * _backBuffers.Count();

    return false;
}

GPUSwapChainVulkan::Status GPUSwapChainVulkan::Present(QueueVulkan* presentQueue, SemaphoreVulkan* backBufferRenderingDoneSemaphore)
{
    if (_currentImageIndex == -1)
        return Status::Ok;

    VkPresentInfoKHR presentInfo;
    RenderToolsVulkan::ZeroStruct(presentInfo, VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
    VkSemaphore semaphore;
    if (backBufferRenderingDoneSemaphore)
    {
        presentInfo.waitSemaphoreCount = 1;
        semaphore = backBufferRenderingDoneSemaphore->GetHandle();
        presentInfo.pWaitSemaphores = &semaphore;
    }
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapChain;
    presentInfo.pImageIndices = (uint32*)&_currentImageIndex;

    const VkResult presentResult = vkQueuePresentKHR(presentQueue->GetHandle(), &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        return Status::Outdated;
    }
    if (presentResult == VK_ERROR_SURFACE_LOST_KHR)
    {
        return Status::LostSurface;
    }
#if GPU_ENABLE_ASSERTION
    if (presentResult != VK_SUCCESS && presentResult != VK_SUBOPTIMAL_KHR)
    {
        VALIDATE_VULKAN_RESULT(presentResult);
    }
#endif

    return Status::Ok;
}

int32 GPUSwapChainVulkan::DoAcquireImageIndex(GPUSwapChainVulkan* viewport, void* customData)
{
    return viewport->_acquiredImageIndex = viewport->AcquireNextImage(viewport->_acquiredSemaphore);
}

int32 GPUSwapChainVulkan::DoPresent(GPUSwapChainVulkan* viewport, void* customData)
{
    return (int32)viewport->Present((QueueVulkan*)customData, viewport->_backBuffers[viewport->_acquiredImageIndex].RenderingDoneSemaphore);
}

int32 GPUSwapChainVulkan::TryPresent(Function<int32(GPUSwapChainVulkan*, void*)> job, void* customData, bool skipOnOutOfDate)
{
    int32 attemptsPending = 4;
    int32 status = job(this, customData);
    while (status < 0 && attemptsPending > 0)
    {
        if (status == (int32)Status::Outdated)
        {
            //LOG(Warning, "Swapchain is out of date");
            if (skipOnOutOfDate)
                return status;
        }
        else if (status == (int32)Status::LostSurface)
        {
            LOG(Warning, "Swapchain surface lost");
        }
        else
        {
            CRASH;
        }

        // Recreate swapchain
        ASSERT(_swapChain != VK_NULL_HANDLE);
        int32 width = _width, height = _height;
        ReleaseGPU();
        CreateSwapChain(width, height);

        // Flush commands
        _device->GetMainContext()->Flush();
        _device->WaitForGPU();

        status = job(this, customData);
        attemptsPending--;
    }
    return status;
}

int32 GPUSwapChainVulkan::AcquireNextImage(SemaphoreVulkan*& outSemaphore)
{
    PROFILE_CPU();
    ASSERT(_swapChain && _backBuffers.HasItems());

    uint32 imageIndex = _currentImageIndex;
    const int32 prevSemaphoreIndex = _semaphoreIndex;
    _semaphoreIndex = (_semaphoreIndex + 1) % _backBuffers.Count();
    const auto semaphore = _backBuffers[_semaphoreIndex].ImageAcquiredSemaphore;

    const VkResult result = vkAcquireNextImageKHR(
        _device->Device,
        _swapChain,
        UINT64_MAX,
        semaphore->GetHandle(),
        VK_NULL_HANDLE,
        &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        _semaphoreIndex = prevSemaphoreIndex;
        return (int32)Status::Outdated;
    }
    if (result == VK_ERROR_SURFACE_LOST_KHR)
    {
        _semaphoreIndex = prevSemaphoreIndex;
        return (int32)Status::LostSurface;
    }
    outSemaphore = semaphore;
    if (result == VK_ERROR_VALIDATION_FAILED_EXT)
    {
        LOG(Fatal, "vkAcquireNextImageKHR failed with validation error");
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
#if GPU_ENABLE_ASSERTION
        RenderToolsVulkan::LogVkResult(result, __FILE__, __LINE__);
#endif
    }
    _currentImageIndex = (int32)imageIndex;

    return _currentImageIndex;
}

void GPUSwapChainVulkan::Present(bool vsync)
{
    // Skip if there was no rendering to the backbuffer
    if (_acquiredImageIndex == -1)
        return;
    PROFILE_CPU();

    // Ensure that backbuffer has been acquired before presenting it to the window
    const auto backBuffer = (GPUTextureViewVulkan*)GetBackBufferView();

    // Ensure to keep backbuffer in a proper layout
    auto context = _device->MainContext;
    context->AddImageBarrier(backBuffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    context->FlushBarriers();

    context->GetCmdBufferManager()->SubmitActiveCmdBuffer(_backBuffers[_acquiredImageIndex].RenderingDoneSemaphore);

    // Present the back buffer to the viewport window
    const auto result = TryPresent(DoPresent, _device->PresentQueue, true);
    if (result == (int32)Status::Outdated)
    {
        // Failed to present, window can be minimized or doesn't want to swap the buffers so just ignore the present
        if (_window->IsMinimized())
        {
            return;
        }

        // Rebuild swapchain for the next present
        int32 width = _width, height = _height;
        ReleaseGPU();
        CreateSwapChain(width, height);
        _device->GetMainContext()->Flush();
        _device->WaitForGPU();
        return;
    }
    if (result < 0)
    {
        LOG(Fatal, "Swapchain present failed!");
    }

    // Release the back buffer
    _acquiredImageIndex = -1;

    // Base
    GPUSwapChain::Present(vsync);
}

#endif
