// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if XR_API_OPENXR

#include "Engine/Core/Collections/Array.h"
#include "Engine/Graphics/Enums.h"
#include "Engine/Graphics/PixelFormat.h"
#include "Engine/Graphics/Textures/GPUTextureDescription.h"
#include <ThirdParty/OpenXR/openxr/openxr.h>

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

public:
    /// <summary>
    /// Selects the swapchain format from the formats supported by the runtime.
    /// </summary>
    /// <param name="formats">The native formats supported by the runtime.</param>
    /// <returns>The selected format, or unknown if the runtime doesn't support any of the formats used by the engine.</returns>
    PixelFormat SelectSwapchainFormat(const Array<int64>& formats) const;

    /// <summary>
    /// Selects the depth swapchain format from the formats supported by the runtime. The format matches the engine depth buffer so it can be copied.
    /// </summary>
    /// <param name="formats">The native formats supported by the runtime.</param>
    /// <returns>The selected format, or unknown if the runtime doesn't support it.</returns>
    PixelFormat SelectDepthSwapchainFormat(const Array<int64>& formats) const;
};

#endif
