// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if XR_API_OPENXR

#include "Engine/Core/Collections/Array.h"
#include "Engine/Graphics/Enums.h"
#include "Engine/Graphics/PixelFormat.h"
#include "Engine/Graphics/Textures/GPUTextureDescription.h"
#include <ThirdParty/OpenXR/openxr/openxr.h>

class GPUContext;
class GPUTexture;

/// <summary>
/// The OpenXR graphics backend that binds the session to the graphics device.
/// </summary>
class OpenXRGraphics
{
public:
    virtual ~OpenXRGraphics()
    {
    }

public:
    /// <summary>
    /// Creates the graphics backends supported by the runtime and adds their extensions to the list.
    /// </summary>
    /// <param name="extensions">The extensions supported by the runtime.</param>
    /// <param name="enabledExtensions">The extensions to enable.</param>
    /// <returns>True if failed (no graphics backend is supported by the runtime), otherwise false.</returns>
    static bool Init(const Array<XrExtensionProperties>& extensions, Array<const char*>& enabledExtensions);

    /// <summary>
    /// Destroys the graphics backends.
    /// </summary>
    static void Dispose();

    /// <summary>
    /// Sets the graphics device creation hints if the device is not created yet, otherwise selects the backend for the device and checks if the device can be used by the runtime.
    /// </summary>
    /// <param name="instance">The OpenXR instance.</param>
    /// <param name="systemId">The head-mounted display system.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool Setup(XrInstance instance, XrSystemId systemId);

    /// <summary>
    /// Checks if the runtime can be connected with the graphics device that is already created (eg. the runtime was started after the engine).
    /// </summary>
    /// <returns>True if the runtime can be connected, otherwise false.</returns>
    static bool CanConnect();

    /// <summary>
    /// Gets the graphics backend selected for the graphics device.
    /// </summary>
    /// <returns>The graphics backend, or null if not selected yet.</returns>
    static OpenXRGraphics* Get();

public:
    /// <summary>
    /// Gets the name of the OpenXR extension required by this backend.
    /// </summary>
    /// <returns>The extension name.</returns>
    virtual const char* GetExtensionName() const = 0;

    /// <summary>
    /// Checks if the backend can be used with the graphics device renderer.
    /// </summary>
    /// <param name="rendererType">The renderer type.</param>
    /// <returns>True if supported, otherwise false.</returns>
    virtual bool IsSupported(RendererType rendererType) const = 0;

    /// <summary>
    /// Checks if the graphics device that is already created can be used by the runtime that gets connected later.
    /// </summary>
    /// <returns>True if the graphics device can be used, otherwise false.</returns>
    virtual bool CanUseDevice() const
    {
        return true;
    }

    /// <summary>
    /// Gets the graphics requirements of the runtime and sets the graphics device creation hints (called before the device is created).
    /// </summary>
    /// <param name="instance">The OpenXR instance.</param>
    /// <param name="systemId">The head-mounted display system.</param>
    /// <returns>True if failed, otherwise false.</returns>
    virtual bool SetupDeviceHints(XrInstance instance, XrSystemId systemId) = 0;

    /// <summary>
    /// Gets the graphics requirements of the runtime and checks if the graphics device can be used by the runtime.
    /// </summary>
    /// <param name="instance">The OpenXR instance.</param>
    /// <param name="systemId">The head-mounted display system.</param>
    /// <returns>True if failed, otherwise false.</returns>
    virtual bool SetupDevice(XrInstance instance, XrSystemId systemId) = 0;

    /// <summary>
    /// Gets the graphics binding structure for the session creation.
    /// </summary>
    /// <returns>The graphics binding.</returns>
    virtual const void* GetGraphicsBinding() const = 0;

    /// <summary>
    /// Converts the pixel format into the native format used by the swapchains.
    /// </summary>
    /// <param name="format">The pixel format.</param>
    /// <returns>The native format.</returns>
    virtual int64 ToNativeFormat(PixelFormat format) const = 0;

    /// <summary>
    /// Gets the swapchain images and creates the textures for them.
    /// </summary>
    /// <param name="swapchain">The swapchain.</param>
    /// <param name="format">The swapchain format.</param>
    /// <param name="width">The swapchain width.</param>
    /// <param name="height">The swapchain height.</param>
    /// <param name="flags">The texture flags.</param>
    /// <param name="images">The output textures.</param>
    /// <returns>True if failed, otherwise false.</returns>
    virtual bool InitSwapchainImages(XrSwapchain swapchain, PixelFormat format, int32 width, int32 height, GPUTextureFlags flags, Array<GPUTexture*>& images) = 0;

    /// <summary>
    /// Selects the depth swapchain format from the formats supported by the runtime. The format matches the engine depth buffer so it can be copied.
    /// </summary>
    /// <param name="formats">The native formats supported by the runtime.</param>
    /// <returns>The selected format, or unknown if the runtime doesn't support it.</returns>
    virtual PixelFormat SelectDepthSwapchainFormat(const Array<int64>& formats) const;

    /// <summary>
    /// Transitions the swapchain image into the state required by the runtime when the image is released (render target or depth write).
    /// </summary>
    /// <param name="context">The GPU context.</param>
    /// <param name="image">The swapchain image.</param>
    virtual void TransitionImage(GPUContext* context, GPUTexture* image) const;

    /// <summary>
    /// Submits the recorded rendering commands to the graphics queue used by the runtime (called before the swapchain images are released).
    /// </summary>
    virtual void Submit() const
    {
    }

public:
    /// <summary>
    /// Selects the swapchain format from the formats supported by the runtime.
    /// </summary>
    /// <param name="formats">The native formats supported by the runtime.</param>
    /// <returns>The selected format, or unknown if the runtime doesn't support any of the formats used by the engine.</returns>
    PixelFormat SelectSwapchainFormat(const Array<int64>& formats) const;

protected:
    /// <summary>
    /// Gets the graphics requirements of the runtime for the graphics API of the backend.
    /// </summary>
    /// <param name="instance">The OpenXR instance.</param>
    /// <param name="systemId">The head-mounted display system.</param>
    /// <param name="functionName">The name of the extension function that gets the requirements.</param>
    /// <param name="type">The type of the requirements structure.</param>
    /// <param name="requirements">The result requirements.</param>
    /// <returns>True if failed, otherwise false.</returns>
    template<typename T>
    static bool GetGraphicsRequirements(XrInstance instance, XrSystemId systemId, const char* functionName, XrStructureType type, T& requirements)
    {
        XrResult (XRAPI_PTR* getGraphicsRequirements)(XrInstance, XrSystemId, T*) = nullptr;
        XrResult result = xrGetInstanceProcAddr(instance, functionName, (PFN_xrVoidFunction*)&getGraphicsRequirements);
        requirements = { type };
        if (XR_SUCCEEDED(result))
            result = getGraphicsRequirements(instance, systemId, &requirements);
        return CheckGraphicsRequirements(result);
    }

    /// <summary>
    /// Gets the native images of the swapchain.
    /// </summary>
    /// <param name="swapchain">The swapchain.</param>
    /// <param name="type">The type of the swapchain image structure.</param>
    /// <param name="swapchainImages">The result images.</param>
    /// <returns>True if failed, otherwise false.</returns>
    template<typename T>
    static bool GetSwapchainImages(XrSwapchain swapchain, XrStructureType type, Array<T>& swapchainImages)
    {
        uint32 imagesCount = 0;
        XrResult result = xrEnumerateSwapchainImages(swapchain, 0, &imagesCount, nullptr);
        if (XR_SUCCEEDED(result))
        {
            swapchainImages.Resize(imagesCount);
            for (auto& swapchainImage : swapchainImages)
                swapchainImage = { type };
            result = xrEnumerateSwapchainImages(swapchain, imagesCount, &imagesCount, (XrSwapchainImageBaseHeader*)swapchainImages.Get());
        }
        return CheckSwapchainImages(result, imagesCount);
    }

    /// <summary>
    /// Creates the texture for the native image of the swapchain and adds it to the list.
    /// </summary>
    /// <param name="desc">The texture description.</param>
    /// <param name="nativeImage">The native image of the swapchain.</param>
    /// <param name="images">The output textures.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool CreateSwapchainTexture(const GPUTextureDescription& desc, void* nativeImage, Array<GPUTexture*>& images);

    /// <summary>
    /// Gets the format of the texture views used to render to the swapchain images. The engine output is already gamma-encoded so the sRGB swapchain is rendered through the non-sRGB views, which requires the typeless images.
    /// </summary>
    /// <param name="format">The swapchain format.</param>
    /// <param name="imageFormat">The native format of the swapchain images.</param>
    /// <returns>The format of the views.</returns>
    PixelFormat GetSwapchainViewFormat(PixelFormat format, int64 imageFormat) const;

private:
    static bool CheckGraphicsRequirements(XrResult result);
    static bool CheckSwapchainImages(XrResult result, uint32 imagesCount);
};

#endif
