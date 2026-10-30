// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if XR_API_OPENXR && GRAPHICS_API_DIRECTX11

#include "OpenXRGraphics.h"

/// <summary>
/// The OpenXR graphics backend for DirectX 11 (XR_KHR_D3D11_enable).
/// </summary>
class OpenXRGraphicsD3D11 : public OpenXRGraphics
{
public:
    // [OpenXRGraphics]
    const char* GetExtensionName() const override;
    bool IsSupported(RendererType rendererType) const override;
    bool SetupDeviceHints(XrInstance instance, XrSystemId systemId) override;
    bool SetupDevice(XrInstance instance, XrSystemId systemId) override;
    const void* GetGraphicsBinding() const override;
    int64 ToNativeFormat(PixelFormat format) const override;
    bool InitSwapchainImages(XrSwapchain swapchain, PixelFormat format, int32 width, int32 height, GPUTextureFlags flags, Array<GPUTexture*>& images) override;
};

#endif
