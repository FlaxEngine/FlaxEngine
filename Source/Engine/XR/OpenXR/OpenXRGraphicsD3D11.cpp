// Copyright (c) Wojciech Figat. All rights reserved.

#if XR_API_OPENXR && GRAPHICS_API_DIRECTX11

#include "OpenXRGraphicsD3D11.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/GraphicsDevice/DirectX/GPUAdapterDX.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"
#define XR_USE_GRAPHICS_API_D3D11
#include <ThirdParty/OpenXR/openxr/openxr_platform.h>

namespace
{
    XrGraphicsBindingD3D11KHR GraphicsBinding = { XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };
}

const char* OpenXRGraphicsD3D11::GetExtensionName() const
{
    return XR_KHR_D3D11_ENABLE_EXTENSION_NAME;
}

bool OpenXRGraphicsD3D11::IsSupported(RendererType rendererType) const
{
    return rendererType == RendererType::DirectX11;
}

bool OpenXRGraphicsD3D11::SetupDeviceHints(XrInstance instance, XrSystemId systemId)
{
    // Use the adapter connected to the headset when creating the graphics device
    XrGraphicsRequirementsD3D11KHR requirements;
    if (GetGraphicsRequirements(instance, systemId, "xrGetD3D11GraphicsRequirementsKHR", XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR, requirements))
        return true;
    GPUDevice::CreationHints.RequiredAdapterLuid = GPUAdapterDX::PackLuid(requirements.adapterLuid);
    return false;
}

bool OpenXRGraphicsD3D11::SetupDevice(XrInstance instance, XrSystemId systemId)
{
    XrGraphicsRequirementsD3D11KHR requirements;
    if (GetGraphicsRequirements(instance, systemId, "xrGetD3D11GraphicsRequirementsKHR", XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR, requirements))
        return true;
    const uint64 adapterLuid = GPUAdapterDX::PackLuid(requirements.adapterLuid);

    // Check if the graphics device can be used by the runtime
    const auto device = GPUDevice::Instance;
    const auto d3dDevice = (ID3D11Device*)device->GetNativePtr();
    if (((GPUAdapterDX*)device->GetAdapter())->GetLuid() != adapterLuid)
    {
        LOG(Warning, "XR unavailable, the graphics device does not use the adapter connected to the headset (LUID: 0x{0:x})", adapterLuid);
        return true;
    }
    if (d3dDevice->GetFeatureLevel() < requirements.minFeatureLevel)
    {
        LOG(Warning, "XR unavailable, the graphics device feature level is not supported by the runtime");
        return true;
    }
    GraphicsBinding.device = d3dDevice;

    return false;
}

const void* OpenXRGraphicsD3D11::GetGraphicsBinding() const
{
    return &GraphicsBinding;
}

int64 OpenXRGraphicsD3D11::ToNativeFormat(PixelFormat format) const
{
    return (int64)RenderToolsDX::ToDxgiFormat(format);
}

bool OpenXRGraphicsD3D11::InitSwapchainImages(XrSwapchain swapchain, PixelFormat format, int32 width, int32 height, GPUTextureFlags flags, Array<GPUTexture*>& images)
{
    // Get swapchain textures
    Array<XrSwapchainImageD3D11KHR> swapchainImages;
    if (GetSwapchainImages(swapchain, XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR, swapchainImages))
        return true;

    // Render to the sRGB swapchain through the non-sRGB view if the texture is typeless (the engine output is already gamma-encoded)
    D3D11_TEXTURE2D_DESC textureDesc;
    swapchainImages[0].texture->GetDesc(&textureDesc);
    format = GetSwapchainViewFormat(format, (int64)textureDesc.Format);

    // Create textures
    const auto desc = GPUTextureDescription::New2D(width, height, format, flags);
    for (const auto& swapchainImage : swapchainImages)
    {
        if (CreateSwapchainTexture(desc, swapchainImage.texture, images))
            return true;
    }

    return false;
}

#endif
