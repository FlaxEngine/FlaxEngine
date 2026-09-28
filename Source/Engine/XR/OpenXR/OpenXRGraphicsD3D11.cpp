// Copyright (c) Wojciech Figat. All rights reserved.

#if XR_API_OPENXR

#include "OpenXRGraphicsD3D11.h"
#include "OpenXRInstance.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/GraphicsDevice/DirectX/IncludeDirectXHeaders.h"
#define XR_USE_GRAPHICS_API_D3D11
#include <ThirdParty/OpenXR/openxr/openxr_platform.h>

const char* OpenXRGraphicsD3D11::ExtensionName = XR_KHR_D3D11_ENABLE_EXTENSION_NAME;

namespace
{
    XrGraphicsBindingD3D11KHR GraphicsBinding = { XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };

    uint64 GetLuid(const LUID& luid)
    {
        return ((uint64)(uint32)luid.HighPart << 32) | (uint64)luid.LowPart;
    }
}

bool OpenXRGraphicsD3D11::SetupDevice(XrInstance instance, XrSystemId systemId)
{
    // Get the adapter and the feature level required by the runtime
    PFN_xrGetD3D11GraphicsRequirementsKHR getGraphicsRequirements = nullptr;
    XrResult result = xrGetInstanceProcAddr(instance, "xrGetD3D11GraphicsRequirementsKHR", (PFN_xrVoidFunction*)&getGraphicsRequirements);
    XrGraphicsRequirementsD3D11KHR requirements = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
    if (XR_SUCCEEDED(result))
        result = getGraphicsRequirements(instance, systemId, &requirements);
    if (XR_FAILED(result))
    {
        LOG(Warning, "Failed to get OpenXR graphics requirements ({0})", OpenXRInstance::GetResultString(result));
        return true;
    }
    const uint64 adapterLuid = GetLuid(requirements.adapterLuid);

    // Use the adapter connected to the headset when creating the graphics device
    const auto device = GPUDevice::Instance;
    if (device == nullptr)
    {
        GPUDevice::CreationHints.RequiredAdapterLuid = adapterLuid;
        return false;
    }

    // Check if the graphics device can be used by the runtime
    if (device->GetRendererType() != RendererType::DirectX11)
    {
        LOG(Info, "XR unavailable (renderer {0} is not supported)", ::ToString(device->GetRendererType()));
        return true;
    }
    auto d3dDevice = (ID3D11Device*)device->GetNativePtr();
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

const void* OpenXRGraphicsD3D11::GetGraphicsBinding()
{
    return &GraphicsBinding;
}

int64 OpenXRGraphicsD3D11::SelectSwapchainFormat(const Array<int64>& formats)
{
    // Prefer 8-bit sRGB formats
    const DXGI_FORMAT preferredFormats[] =
    {
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_B8G8R8A8_UNORM,
    };
    for (const DXGI_FORMAT format : preferredFormats)
    {
        if (formats.Contains((int64)format))
            return (int64)format;
    }
    return formats.HasItems() ? formats[0] : (int64)DXGI_FORMAT_UNKNOWN;
}

bool OpenXRGraphicsD3D11::InitSwapchainImages(XrSwapchain swapchain, int64 format, Array<void*>& images)
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
    if (XR_FAILED(result))
    {
        LOG(Warning, "Failed to get OpenXR swapchain images ({0})", OpenXRInstance::GetResultString(result));
        return true;
    }

    // Create render target views
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = (DXGI_FORMAT)format;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    for (const auto& swapchainImage : swapchainImages)
    {
        ID3D11RenderTargetView* rtv = nullptr;
        if (FAILED(GraphicsBinding.device->CreateRenderTargetView(swapchainImage.texture, &rtvDesc, &rtv)))
        {
            LOG(Warning, "Failed to create render target view for OpenXR swapchain image");
            ReleaseSwapchainImages(images);
            return true;
        }
        images.Add(rtv);
    }

    return false;
}

void OpenXRGraphicsD3D11::ReleaseSwapchainImages(Array<void*>& images)
{
    for (void* image : images)
        ((ID3D11RenderTargetView*)image)->Release();
    images.Clear();
}

void OpenXRGraphicsD3D11::ClearSwapchainImage(void* image, const Color& color)
{
    ID3D11DeviceContext* context = nullptr;
    GraphicsBinding.device->GetImmediateContext(&context);
    context->ClearRenderTargetView((ID3D11RenderTargetView*)image, color.Raw);
    context->Release();
}

#endif
