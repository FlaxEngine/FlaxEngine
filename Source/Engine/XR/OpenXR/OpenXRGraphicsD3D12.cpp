// Copyright (c) Wojciech Figat. All rights reserved.

#if XR_API_OPENXR && GRAPHICS_API_DIRECTX12

#include "OpenXRGraphicsD3D12.h"
#include "Engine/Core/Log.h"
#include "Engine/GraphicsDevice/DirectX/GPUAdapterDX.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"
#include "Engine/GraphicsDevice/DirectX/DX12/GPUDeviceDX12.h"
#define XR_USE_GRAPHICS_API_D3D12
#include <ThirdParty/OpenXR/openxr/openxr_platform.h>

namespace
{
    XrGraphicsBindingD3D12KHR GraphicsBinding = { XR_TYPE_GRAPHICS_BINDING_D3D12_KHR };
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
    if (GetGraphicsRequirements(instance, systemId, "xrGetD3D12GraphicsRequirementsKHR", XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR, requirements))
        return true;
    GPUDevice::CreationHints.RequiredAdapterLuid = GPUAdapterDX::PackLuid(requirements.adapterLuid);
    return false;
}

bool OpenXRGraphicsD3D12::SetupDevice(XrInstance instance, XrSystemId systemId)
{
    XrGraphicsRequirementsD3D12KHR requirements;
    if (GetGraphicsRequirements(instance, systemId, "xrGetD3D12GraphicsRequirementsKHR", XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR, requirements))
        return true;
    const uint64 adapterLuid = GPUAdapterDX::PackLuid(requirements.adapterLuid);

    // Check if the graphics device can be used by the runtime
    const auto device = (GPUDeviceDX12*)GPUDevice::Instance;
    const auto adapter = (GPUAdapterDX*)device->GetAdapter();
    if (adapter->GetLuid() != adapterLuid)
    {
        LOG(Warning, "XR unavailable, the graphics device does not use the adapter connected to the headset (LUID: 0x{0:x})", adapterLuid);
        return true;
    }
    if (adapter->MaxFeatureLevel < requirements.minFeatureLevel)
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
    Array<XrSwapchainImageD3D12KHR> swapchainImages;
    if (GetSwapchainImages(swapchain, XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR, swapchainImages))
        return true;

    // Render to the sRGB swapchain through the non-sRGB view if the texture is typeless (the engine output is already gamma-encoded)
    format = GetSwapchainViewFormat(format, (int64)swapchainImages[0].texture->GetDesc().Format);

    // Create textures (images are in the render target state, or the depth write state for depth images)
    const auto desc = GPUTextureDescription::New2D(width, height, format, flags);
    for (const auto& swapchainImage : swapchainImages)
    {
        if (CreateSwapchainTexture(desc, swapchainImage.texture, images))
            return true;
    }

    return false;
}

#endif
