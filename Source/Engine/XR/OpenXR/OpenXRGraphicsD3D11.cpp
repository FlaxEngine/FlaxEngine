// Copyright (c) Wojciech Figat. All rights reserved.

#if XR_API_OPENXR && GRAPHICS_API_DIRECTX11

#include "OpenXRGraphicsD3D11.h"
#include "OpenXRInstance.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"
#define XR_USE_GRAPHICS_API_D3D11
#include <ThirdParty/OpenXR/openxr/openxr_platform.h>

namespace
{
    XrGraphicsBindingD3D11KHR GraphicsBinding = { XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };

    uint64 GetLuid(const LUID& luid)
    {
        return ((uint64)(uint32)luid.HighPart << 32) | (uint64)luid.LowPart;
    }

    bool GetGraphicsRequirements(XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsD3D11KHR& requirements)
    {
        // Get the adapter and the feature level required by the runtime
        PFN_xrGetD3D11GraphicsRequirementsKHR getGraphicsRequirements = nullptr;
        XrResult result = xrGetInstanceProcAddr(instance, "xrGetD3D11GraphicsRequirementsKHR", (PFN_xrVoidFunction*)&getGraphicsRequirements);
        requirements = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
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
    if (GetGraphicsRequirements(instance, systemId, requirements))
        return true;
    GPUDevice::CreationHints.RequiredAdapterLuid = GetLuid(requirements.adapterLuid);
    return false;
}

bool OpenXRGraphicsD3D11::SetupDevice(XrInstance instance, XrSystemId systemId)
{
    XrGraphicsRequirementsD3D11KHR requirements;
    if (GetGraphicsRequirements(instance, systemId, requirements))
        return true;
    const uint64 adapterLuid = GetLuid(requirements.adapterLuid);

    // Check if the graphics device can be used by the runtime
    auto d3dDevice = (ID3D11Device*)GPUDevice::Instance->GetNativePtr();
    ComPtr<IDXGIDevice> dxgiDevice;
    ComPtr<IDXGIAdapter> dxgiAdapter;
    DXGI_ADAPTER_DESC adapterDesc;
    if (FAILED(d3dDevice->QueryInterface(IID_PPV_ARGS(&dxgiDevice))) || FAILED(dxgiDevice->GetAdapter(&dxgiAdapter)) || FAILED(dxgiAdapter->GetDesc(&adapterDesc)))
    {
        LOG(Warning, "Failed to get the DirectX adapter of the graphics device");
        return true;
    }
    if (GetLuid(adapterDesc.AdapterLuid) != adapterLuid)
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
    uint32 imagesCount = 0;
    XrResult result = xrEnumerateSwapchainImages(swapchain, 0, &imagesCount, nullptr);
    Array<XrSwapchainImageD3D11KHR> swapchainImages;
    if (XR_SUCCEEDED(result))
    {
        swapchainImages.Resize(imagesCount);
        for (auto& swapchainImage : swapchainImages)
            swapchainImage = { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR };
        result = xrEnumerateSwapchainImages(swapchain, imagesCount, &imagesCount, (XrSwapchainImageBaseHeader*)swapchainImages.Get());
    }
    if (XR_FAILED(result) || imagesCount == 0)
    {
        LOG(Warning, "Failed to get OpenXR swapchain images ({0})", OpenXRInstance::GetResultString(result));
        return true;
    }

    // Render to the sRGB swapchain through the non-sRGB view if the texture is typeless (the engine output is already gamma-encoded)
    D3D11_TEXTURE2D_DESC textureDesc;
    swapchainImages[0].texture->GetDesc(&textureDesc);
    if (PixelFormatExtensions::IsSRGB(format))
    {
        if (textureDesc.Format == RenderToolsDX::ToDxgiFormat(PixelFormatExtensions::MakeTypeless(format)))
            format = PixelFormatExtensions::ToNonsRGB(format);
        else
            LOG(Warning, "OpenXR swapchain texture is not typeless, the headset image will be too bright");
    }

    // Create textures
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
