// Copyright (c) Wojciech Figat. All rights reserved.

#if XR_API_OPENXR && GRAPHICS_API_DIRECTX12

#include "OpenXRGraphicsD3D12.h"
#include "OpenXRInstance.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/GraphicsDevice/DirectX/GPUAdapterDX.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"
#include "Engine/GraphicsDevice/DirectX/DX12/GPUDeviceDX12.h"
#define XR_USE_GRAPHICS_API_D3D12
#include <ThirdParty/OpenXR/openxr/openxr_platform.h>

namespace
{
    XrGraphicsBindingD3D12KHR GraphicsBinding = { XR_TYPE_GRAPHICS_BINDING_D3D12_KHR };

    uint64 GetLuid(const LUID& luid)
    {
        return ((uint64)(uint32)luid.HighPart << 32) | (uint64)luid.LowPart;
    }

    bool GetGraphicsRequirements(XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsD3D12KHR& requirements)
    {
        // Get the adapter and the feature level required by the runtime
        PFN_xrGetD3D12GraphicsRequirementsKHR getGraphicsRequirements = nullptr;
        XrResult result = xrGetInstanceProcAddr(instance, "xrGetD3D12GraphicsRequirementsKHR", (PFN_xrVoidFunction*)&getGraphicsRequirements);
        requirements = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR };
        if (XR_SUCCEEDED(result))
            result = getGraphicsRequirements(instance, systemId, &requirements);
        if (XR_FAILED(result))
        {
            LOG(Warning, "Failed to get OpenXR graphics requirements ({0})", OpenXRInstance::GetResultString(result));
            return true;
        }
        return false;
    }
}

const char* OpenXRGraphicsD3D12::GetExtensionName() const
{
    return XR_KHR_D3D12_ENABLE_EXTENSION_NAME;
}

bool OpenXRGraphicsD3D12::IsSupported(RendererType rendererType) const
{
    return rendererType == RendererType::DirectX12;
}

bool OpenXRGraphicsD3D12::SetupDeviceHints(XrInstance instance, XrSystemId systemId)
{
    // Use the adapter connected to the headset when creating the graphics device
    XrGraphicsRequirementsD3D12KHR requirements;
    if (GetGraphicsRequirements(instance, systemId, requirements))
        return true;
    GPUDevice::CreationHints.RequiredAdapterLuid = GetLuid(requirements.adapterLuid);
    return false;
}

bool OpenXRGraphicsD3D12::SetupDevice(XrInstance instance, XrSystemId systemId)
{
    XrGraphicsRequirementsD3D12KHR requirements;
    if (GetGraphicsRequirements(instance, systemId, requirements))
        return true;
    const uint64 adapterLuid = GetLuid(requirements.adapterLuid);

    // Check if the graphics device can be used by the runtime
    auto device = (GPUDeviceDX12*)GPUDevice::Instance;
    if (GetLuid(device->GetDevice()->GetAdapterLuid()) != adapterLuid)
    {
        LOG(Warning, "XR unavailable, the graphics device does not use the adapter connected to the headset (LUID: 0x{0:x})", adapterLuid);
        return true;
    }
    if (((GPUAdapterDX*)device->GetAdapter())->MaxFeatureLevel < requirements.minFeatureLevel)
    {
        LOG(Warning, "XR unavailable, the graphics device feature level is not supported by the runtime");
        return true;
    }
    GraphicsBinding.device = device->GetDevice();
    GraphicsBinding.queue = device->GetCommandQueueDX12();

    return false;
}

const void* OpenXRGraphicsD3D12::GetGraphicsBinding() const
{
    return &GraphicsBinding;
}

int64 OpenXRGraphicsD3D12::ToNativeFormat(PixelFormat format) const
{
    return (int64)RenderToolsDX::ToDxgiFormat(format);
}

bool OpenXRGraphicsD3D12::InitSwapchainImages(XrSwapchain swapchain, PixelFormat format, int32 width, int32 height, GPUTextureFlags flags, Array<GPUTexture*>& images)
{
    // Get swapchain textures
    uint32 imagesCount = 0;
    XrResult result = xrEnumerateSwapchainImages(swapchain, 0, &imagesCount, nullptr);
    Array<XrSwapchainImageD3D12KHR> swapchainImages;
    if (XR_SUCCEEDED(result))
    {
        swapchainImages.Resize(imagesCount);
        for (auto& swapchainImage : swapchainImages)
            swapchainImage = { XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR };
        result = xrEnumerateSwapchainImages(swapchain, imagesCount, &imagesCount, (XrSwapchainImageBaseHeader*)swapchainImages.Get());
    }
    if (XR_FAILED(result) || imagesCount == 0)
    {
        LOG(Warning, "Failed to get OpenXR swapchain images ({0})", OpenXRInstance::GetResultString(result));
        return true;
    }

    // Render to the sRGB swapchain through the non-sRGB view if the texture is typeless (the engine output is already gamma-encoded)
    const D3D12_RESOURCE_DESC resourceDesc = swapchainImages[0].texture->GetDesc();
    if (PixelFormatExtensions::IsSRGB(format))
    {
        if (resourceDesc.Format == RenderToolsDX::ToDxgiFormat(PixelFormatExtensions::MakeTypeless(format)))
            format = PixelFormatExtensions::ToNonsRGB(format);
        else
            LOG(Warning, "OpenXR swapchain texture is not typeless, the headset image will be too bright");
    }

    // Create textures (images are in the render target state, or the depth write state for depth images)
    const auto desc = GPUTextureDescription::New2D(width, height, format, flags);
    for (const auto& swapchainImage : swapchainImages)
    {
        auto texture = GPUDevice::Instance->CreateTexture(TEXT("OpenXR.Swapchain"));
        images.Add(texture);
        if (texture->InitFromNative(desc, swapchainImage.texture))
            return true;
    }

    return false;
}

#endif
