// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if XR_API_OPENXR && GRAPHICS_API_DIRECTX12

#include "OpenXRGraphics.h"

/// <summary>
/// The OpenXR graphics backend for DirectX 12 (XR_KHR_D3D12_enable).
/// </summary>
class OpenXRGraphicsD3D12 : public OpenXRGraphics
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
