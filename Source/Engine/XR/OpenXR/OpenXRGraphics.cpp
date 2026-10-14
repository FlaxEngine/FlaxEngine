// Copyright (c) Wojciech Figat. All rights reserved.

#if XR_API_OPENXR

#include "OpenXRGraphics.h"
#include "OpenXRInstance.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/Config.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUResourceAccess.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#if GRAPHICS_API_DIRECTX11
#include "OpenXRGraphicsD3D11.h"
#endif
#if GRAPHICS_API_DIRECTX12
#include "OpenXRGraphicsD3D12.h"
#endif
#if GRAPHICS_API_VULKAN
#include "OpenXRGraphicsVulkan.h"
#endif

namespace
{
    Array<OpenXRGraphics*> Backends;
    OpenXRGraphics* Current = nullptr;

    void AddBackend(OpenXRGraphics* backend, const Array<XrExtensionProperties>& extensions, Array<const char*>& enabledExtensions)
    {
        if (OpenXRInstance::HasExtension(extensions, backend->GetExtensionName()))
        {
            Backends.Add(backend);
            enabledExtensions.Add(backend->GetExtensionName());
        }
        else
        {
            Delete(backend);
        }
    }
}

bool OpenXRGraphics::Init(const Array<XrExtensionProperties>& extensions, Array<const char*>& enabledExtensions)
{
    // Use the backends for the graphics APIs supported by the runtime (the graphics device is not created yet so it's not known which one will be used)
#if GRAPHICS_API_DIRECTX11
    AddBackend(New<OpenXRGraphicsD3D11>(), extensions, enabledExtensions);
#endif
#if GRAPHICS_API_DIRECTX12
    AddBackend(New<OpenXRGraphicsD3D12>(), extensions, enabledExtensions);
#endif
#if GRAPHICS_API_VULKAN
    AddBackend(New<OpenXRGraphicsVulkan>(), extensions, enabledExtensions);
#endif
    return Backends.IsEmpty();
}

void OpenXRGraphics::Dispose()
{
    Current = nullptr;
    Backends.ClearDelete();
}

bool OpenXRGraphics::Setup(XrInstance instance, XrSystemId systemId)
{
    // Set the hints of all backends before the graphics device is created (it's not known which one will be used)
    const auto device = GPUDevice::Instance;
    if (device == nullptr)
    {
        bool failed = true;
        for (OpenXRGraphics* backend : Backends)
        {
            if (!backend->SetupDeviceHints(instance, systemId))
                failed = false;
        }
        return failed;
    }

    // Select the backend for the graphics device
    Current = nullptr;
    for (OpenXRGraphics* backend : Backends)
    {
        if (backend->IsSupported(device->GetRendererType()))
        {
            Current = backend;
            break;
        }
    }
    if (Current == nullptr)
    {
        LOG(Info, "XR unavailable (renderer {0} is not supported)", ::ToString(device->GetRendererType()));
        return true;
    }
    return Current->SetupDevice(instance, systemId);
}

OpenXRGraphics* OpenXRGraphics::Get()
{
    return Current;
}

PixelFormat OpenXRGraphics::SelectSwapchainFormat(const Array<int64>& formats) const
{
    // Prefer 8-bit sRGB formats
    const PixelFormat preferredFormats[] =
    {
        PixelFormat::R8G8B8A8_UNorm_sRGB,
        PixelFormat::B8G8R8A8_UNorm_sRGB,
        PixelFormat::R8G8B8A8_UNorm,
        PixelFormat::B8G8R8A8_UNorm,
    };
    for (const PixelFormat format : preferredFormats)
    {
        if (formats.Contains(ToNativeFormat(format)))
            return format;
    }
    return PixelFormat::Unknown;
}

PixelFormat OpenXRGraphics::SelectDepthSwapchainFormat(const Array<int64>& formats) const
{
    const PixelFormat format = GPU_DEPTH_BUFFER_PIXEL_FORMAT;
    return formats.Contains(ToNativeFormat(format)) ? format : PixelFormat::Unknown;
}

void OpenXRGraphics::TransitionImage(GPUContext* context, GPUTexture* image) const
{
    context->Transition(image, image->IsDepthStencil() ? GPUResourceAccess::DepthWrite : GPUResourceAccess::RenderTarget);
}

bool OpenXRGraphics::CreateSwapchainTexture(const GPUTextureDescription& desc, void* nativeImage, Array<GPUTexture*>& images)
{
    auto texture = GPUDevice::Instance->CreateTexture(TEXT("OpenXR.Swapchain"));
    images.Add(texture);
    return texture->InitFromNative(desc, nativeImage);
}

PixelFormat OpenXRGraphics::GetSwapchainViewFormat(PixelFormat format, int64 imageFormat) const
{
    if (!PixelFormatExtensions::IsSRGB(format))
        return format;
    if (imageFormat == ToNativeFormat(PixelFormatExtensions::MakeTypeless(format)))
        return PixelFormatExtensions::ToNonsRGB(format);
    LOG(Warning, "OpenXR swapchain texture is not typeless, the headset image will be too bright");
    return format;
}

bool OpenXRGraphics::CheckGraphicsRequirements(XrResult result)
{
    if (XR_FAILED(result))
    {
        LOG(Warning, "Failed to get OpenXR graphics requirements ({0})", OpenXRInstance::GetResultString(result));
        return true;
    }
    return false;
}

bool OpenXRGraphics::CheckSwapchainImages(XrResult result, uint32 imagesCount)
{
    if (XR_FAILED(result) || imagesCount == 0)
    {
        LOG(Warning, "Failed to get OpenXR swapchain images ({0})", OpenXRInstance::GetResultString(result));
        return true;
    }
    return false;
}

#endif
