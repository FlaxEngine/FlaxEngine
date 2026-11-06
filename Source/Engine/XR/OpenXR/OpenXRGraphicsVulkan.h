// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if XR_API_OPENXR && GRAPHICS_API_VULKAN

#include "OpenXRGraphics.h"

/// <summary>
/// The OpenXR graphics backend for Vulkan (XR_KHR_vulkan_enable2).
/// </summary>
class OpenXRGraphicsVulkan : public OpenXRGraphics
{
public:
    // [OpenXRGraphics]
    const char* GetExtensionName() const override;
    bool IsSupported(RendererType rendererType) const override;
    bool CanUseDevice() const override;
    bool SetupDeviceHints(XrInstance instance, XrSystemId systemId) override;
    bool SetupDevice(XrInstance instance, XrSystemId systemId) override;
    const void* GetGraphicsBinding() const override;
    int64 ToNativeFormat(PixelFormat format) const override;
    bool InitSwapchainImages(XrSwapchain swapchain, PixelFormat format, int32 width, int32 height, GPUTextureFlags flags, Array<GPUTexture*>& images) override;
    PixelFormat SelectDepthSwapchainFormat(const Array<int64>& formats) const override;
    void TransitionImage(GPUContext* context, GPUTexture* image) const override;
    void Submit() const override;
};

#endif
