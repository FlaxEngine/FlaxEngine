// Copyright (c) Wojciech Figat. All rights reserved.

#if XR_API_OPENXR && GRAPHICS_API_VULKAN

#include "OpenXRGraphicsVulkan.h"
#include "OpenXRInstance.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/Config.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/GraphicsDevice/Vulkan/GPUDeviceVulkan.h"
#include "Engine/GraphicsDevice/Vulkan/GPUAdapterVulkan.h"
#include "Engine/GraphicsDevice/Vulkan/GPUContextVulkan.h"
#include "Engine/GraphicsDevice/Vulkan/GPUTextureVulkan.h"
#include "Engine/GraphicsDevice/Vulkan/CmdBufferVulkan.h"
#include "Engine/GraphicsDevice/Vulkan/QueueVulkan.h"
#include "Engine/GraphicsDevice/Vulkan/RenderToolsVulkan.h"
#define XR_USE_GRAPHICS_API_VULKAN
#include <ThirdParty/OpenXR/openxr/openxr_platform.h>

namespace
{
    XrGraphicsBindingVulkan2KHR GraphicsBinding = { XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR };
    bool IsDeviceCreatedByRuntime = false;

    bool GetGraphicsRequirements(XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsVulkan2KHR& requirements)
    {
        // Get the Vulkan version required by the runtime
        PFN_xrGetVulkanGraphicsRequirements2KHR getGraphicsRequirements = nullptr;
        XrResult result = xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsRequirements2KHR", (PFN_xrVoidFunction*)&getGraphicsRequirements);
        requirements = { XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR };
        if (XR_SUCCEEDED(result))
            result = getGraphicsRequirements(instance, systemId, &requirements);
        if (XR_FAILED(result))
        {
            LOG(Warning, "Failed to get OpenXR graphics requirements ({0})", OpenXRInstance::GetResultString(result));
            return true;
        }
        return false;
    }

    VkResult CreateVulkanInstance(const VkInstanceCreateInfo* createInfo, VkInstance* vulkanInstance)
    {
        // Create the instance by the runtime so it can add the extensions it needs
        PFN_xrCreateVulkanInstanceKHR createInstance = nullptr;
        XrResult result = XR_ERROR_INSTANCE_LOST;
        if (OpenXRInstance::IsReady())
            result = xrGetInstanceProcAddr(OpenXRInstance::Instance, "xrCreateVulkanInstanceKHR", (PFN_xrVoidFunction*)&createInstance);
        VkResult vulkanResult = VK_SUCCESS;
        if (XR_SUCCEEDED(result))
        {
            XrVulkanInstanceCreateInfoKHR info = { XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR };
            info.systemId = OpenXRInstance::SystemId;
            info.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
            info.vulkanCreateInfo = createInfo;
            result = createInstance(OpenXRInstance::Instance, &info, vulkanInstance, &vulkanResult);
        }
        if (XR_SUCCEEDED(result))
            return vulkanResult;

        // Create the instance without the runtime (XR will be unavailable)
        LOG(Warning, "Failed to create Vulkan instance by OpenXR runtime ({0})", OpenXRInstance::GetResultString(result));
        return vkCreateInstance(createInfo, nullptr, vulkanInstance);
    }

    VkPhysicalDevice GetVulkanPhysicalDevice(VkInstance vulkanInstance)
    {
        // Use the physical device connected to the headset
        PFN_xrGetVulkanGraphicsDevice2KHR getGraphicsDevice = nullptr;
        XrResult result = XR_ERROR_INSTANCE_LOST;
        if (OpenXRInstance::IsReady())
            result = xrGetInstanceProcAddr(OpenXRInstance::Instance, "xrGetVulkanGraphicsDevice2KHR", (PFN_xrVoidFunction*)&getGraphicsDevice);
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        if (XR_SUCCEEDED(result))
        {
            XrVulkanGraphicsDeviceGetInfoKHR info = { XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR };
            info.systemId = OpenXRInstance::SystemId;
            info.vulkanInstance = vulkanInstance;
            result = getGraphicsDevice(OpenXRInstance::Instance, &info, &physicalDevice);
        }
        if (XR_FAILED(result))
            LOG(Warning, "Failed to get Vulkan physical device from OpenXR runtime ({0})", OpenXRInstance::GetResultString(result));
        return physicalDevice;
    }

    VkResult CreateVulkanDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* createInfo, VkDevice* vulkanDevice)
    {
        // Create the device by the runtime so it can add the extensions it needs
        PFN_xrCreateVulkanDeviceKHR createDevice = nullptr;
        XrResult result = XR_ERROR_INSTANCE_LOST;
        if (OpenXRInstance::IsReady())
            result = xrGetInstanceProcAddr(OpenXRInstance::Instance, "xrCreateVulkanDeviceKHR", (PFN_xrVoidFunction*)&createDevice);
        VkResult vulkanResult = VK_SUCCESS;
        if (XR_SUCCEEDED(result))
        {
            XrVulkanDeviceCreateInfoKHR info = { XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR };
            info.systemId = OpenXRInstance::SystemId;
            info.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
            info.vulkanPhysicalDevice = physicalDevice;
            info.vulkanCreateInfo = createInfo;
            result = createDevice(OpenXRInstance::Instance, &info, vulkanDevice, &vulkanResult);
        }
        if (XR_SUCCEEDED(result))
        {
            IsDeviceCreatedByRuntime = vulkanResult == VK_SUCCESS;
            return vulkanResult;
        }

        // Create the device without the runtime (XR will be unavailable)
        LOG(Warning, "Failed to create Vulkan device by OpenXR runtime ({0})", OpenXRInstance::GetResultString(result));
        return vkCreateDevice(physicalDevice, createInfo, nullptr, vulkanDevice);
    }
}

const char* OpenXRGraphicsVulkan::GetExtensionName() const
{
    return XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME;
}

bool OpenXRGraphicsVulkan::IsSupported(RendererType rendererType) const
{
    return rendererType == RendererType::Vulkan;
}

bool OpenXRGraphicsVulkan::SetupDeviceHints(XrInstance instance, XrSystemId systemId)
{
    // Create the graphics device by the runtime (requirements have to be queried before)
    XrGraphicsRequirementsVulkan2KHR requirements;
    if (GetGraphicsRequirements(instance, systemId, requirements))
        return true;
    GPUDeviceVulkan::CreateInstanceOverride.Bind(CreateVulkanInstance);
    GPUDeviceVulkan::GetPhysicalDeviceOverride.Bind(GetVulkanPhysicalDevice);
    GPUDeviceVulkan::CreateDeviceOverride.Bind(CreateVulkanDevice);
    return false;
}

bool OpenXRGraphicsVulkan::SetupDevice(XrInstance instance, XrSystemId systemId)
{
    XrGraphicsRequirementsVulkan2KHR requirements;
    if (GetGraphicsRequirements(instance, systemId, requirements))
        return true;

    // Check if the graphics device can be used by the runtime
    auto device = (GPUDeviceVulkan*)GPUDevice::Instance;
    if (!IsDeviceCreatedByRuntime || GetVulkanPhysicalDevice(GPUDeviceVulkan::Instance) != device->Adapter->Gpu)
    {
        LOG(Warning, "XR unavailable, the graphics device was not created for the headset");
        return true;
    }
    if (VK_MAKE_VERSION(XR_VERSION_MAJOR(requirements.minApiVersionSupported), XR_VERSION_MINOR(requirements.minApiVersionSupported), 0) > device->Adapter->GpuProps.apiVersion)
    {
        LOG(Warning, "XR unavailable, the graphics device Vulkan version is not supported by the runtime");
        return true;
    }
    GraphicsBinding.instance = GPUDeviceVulkan::Instance;
    GraphicsBinding.physicalDevice = device->Adapter->Gpu;
    GraphicsBinding.device = device->Device;
    GraphicsBinding.queueFamilyIndex = device->GraphicsQueue->GetFamilyIndex();
    GraphicsBinding.queueIndex = 0;

    return false;
}

const void* OpenXRGraphicsVulkan::GetGraphicsBinding() const
{
    return &GraphicsBinding;
}

int64 OpenXRGraphicsVulkan::ToNativeFormat(PixelFormat format) const
{
    return (int64)RenderToolsVulkan::ToVulkanFormat(format);
}

bool OpenXRGraphicsVulkan::InitSwapchainImages(XrSwapchain swapchain, PixelFormat format, int32 width, int32 height, GPUTextureFlags flags, Array<GPUTexture*>& images)
{
    // Get swapchain images
    uint32 imagesCount = 0;
    XrResult result = xrEnumerateSwapchainImages(swapchain, 0, &imagesCount, nullptr);
    Array<XrSwapchainImageVulkan2KHR> swapchainImages;
    if (XR_SUCCEEDED(result))
    {
        swapchainImages.Resize(imagesCount);
        for (auto& swapchainImage : swapchainImages)
            swapchainImage = { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR };
        result = xrEnumerateSwapchainImages(swapchain, imagesCount, &imagesCount, (XrSwapchainImageBaseHeader*)swapchainImages.Get());
    }
    if (XR_FAILED(result) || imagesCount == 0)
    {
        LOG(Warning, "Failed to get OpenXR swapchain images ({0})", OpenXRInstance::GetResultString(result));
        return true;
    }

    // Render to the sRGB swapchain through the non-sRGB view (the engine output is already gamma-encoded, images are created with the mutable format)
    if (PixelFormatExtensions::IsSRGB(format))
        format = PixelFormatExtensions::ToNonsRGB(format);

    // Create textures (images are in the color attachment layout, or the depth stencil attachment layout for depth images)
    const auto desc = GPUTextureDescription::New2D(width, height, format, flags);
    for (const auto& swapchainImage : swapchainImages)
    {
        auto texture = GPUDevice::Instance->CreateTexture(TEXT("OpenXR.Swapchain"));
        images.Add(texture);
        if (texture->InitFromNative(desc, (void*)swapchainImage.image))
            return true;
    }

    return false;
}

PixelFormat OpenXRGraphicsVulkan::SelectDepthSwapchainFormat(const Array<int64>& formats) const
{
    // Use the depth buffer format supported by the device (eg. 24-bit depth is not supported on AMD)
    const auto device = (GPUDeviceVulkan*)GPUDevice::Instance;
    const PixelFormat format = device->GetClosestSupportedPixelFormat(PixelFormatExtensions::FindDepthStencilFormat(GPU_DEPTH_BUFFER_PIXEL_FORMAT), GPUTextureFlags::ShaderResource | GPUTextureFlags::DepthStencil, true);
    return formats.Contains(ToNativeFormat(format)) ? format : PixelFormat::Unknown;
}

void OpenXRGraphicsVulkan::TransitionImage(GPUContext* context, GPUTexture* image) const
{
    const auto contextVulkan = (GPUContextVulkan*)context;
    contextVulkan->AddImageBarrier((GPUTextureVulkan*)image, image->IsDepthStencil() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    contextVulkan->FlushBarriers();
}

void OpenXRGraphicsVulkan::Submit() const
{
    // Submit the frame commands if they were not submitted with the window present
    const auto device = (GPUDeviceVulkan*)GPUDevice::Instance;
    const auto cmdBufferManager = device->MainContext->GetCmdBufferManager();
    if (cmdBufferManager->HasPendingActiveCmdBuffer())
        cmdBufferManager->SubmitActiveCmdBuffer();
}

#endif
