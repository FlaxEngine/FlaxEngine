// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if XR_API_OPENXR

#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Color.h"
#include <ThirdParty/OpenXR/openxr/openxr.h>

/// <summary>
/// The OpenXR graphics backend for DirectX 11 (XR_KHR_D3D11_enable).
/// </summary>
class OpenXRGraphicsD3D11
{
public:
    /// <summary>
    /// The name of the OpenXR extension required by this backend.
    /// </summary>
    static const char* ExtensionName;

public:
    /// <summary>
    /// Gets the graphics requirements of the runtime. Sets the graphics device creation hints if the device is not created yet, otherwise checks if the device can be used by the runtime.
    /// </summary>
    /// <param name="instance">The OpenXR instance.</param>
    /// <param name="systemId">The head-mounted display system.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool SetupDevice(XrInstance instance, XrSystemId systemId);

    /// <summary>
    /// Gets the graphics binding structure for the session creation.
    /// </summary>
    /// <returns>The graphics binding.</returns>
    static const void* GetGraphicsBinding();

    /// <summary>
    /// Selects the swapchain format from the formats supported by the runtime.
    /// </summary>
    /// <param name="formats">The formats supported by the runtime.</param>
    /// <returns>The selected format.</returns>
    static int64 SelectSwapchainFormat(const Array<int64>& formats);

    /// <summary>
    /// Gets the swapchain images and creates the render target views for them.
    /// </summary>
    /// <param name="swapchain">The swapchain.</param>
    /// <param name="format">The swapchain format.</param>
    /// <param name="images">The output render target views.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool InitSwapchainImages(XrSwapchain swapchain, int64 format, Array<void*>& images);

    /// <summary>
    /// Releases the render target views of the swapchain images.
    /// </summary>
    /// <param name="images">The render target views.</param>
    static void ReleaseSwapchainImages(Array<void*>& images);

    /// <summary>
    /// Clears the swapchain image with a color.
    /// </summary>
    /// <param name="image">The render target view.</param>
    /// <param name="color">The clear color.</param>
    static void ClearSwapchainImage(void* image, const Color& color);
};

#endif
