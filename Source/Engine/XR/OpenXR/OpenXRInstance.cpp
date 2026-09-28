// Copyright (c) Wojciech Figat. All rights reserved.

#if XR_API_OPENXR

#include "OpenXRInstance.h"
#include "OpenXRInput.h"
#include "OpenXRGraphics.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Platform/StringUtils.h"
#include "FlaxEngine.Gen.h"
#include <ThirdParty/OpenXR/openxr/openxr_reflection.h>

XrInstance OpenXRInstance::Instance = XR_NULL_HANDLE;
XrSystemId OpenXRInstance::SystemId = XR_NULL_SYSTEM_ID;
String OpenXRInstance::RuntimeName;
String OpenXRInstance::RuntimeVersion;
String OpenXRInstance::SystemName;
bool OpenXRInstance::SupportsDepthLayer = false;
bool OpenXRInstance::SupportsDisplayRefreshRate = false;
bool OpenXRInstance::SupportsFrameController = false;

// Not included in the OpenXR SDK headers yet
#define OPENXR_VALVE_FRAME_CONTROLLER_INTERACTION_EXTENSION_NAME "XR_VALVE_frame_controller_interaction"

namespace
{
    bool SystemNotFoundLogged = false;
}

bool OpenXRInstance::Init()
{
    // Check if runtime supports the graphics API
    uint32 extensionsCount = 0;
    XrResult result = xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionsCount, nullptr);
    if (XR_FAILED(result))
    {
        LOG(Info, "XR unavailable ({0})", GetResultString(result));
        return true;
    }
    Array<XrExtensionProperties> extensions;
    extensions.Resize(extensionsCount);
    for (auto& extension : extensions)
        extension = { XR_TYPE_EXTENSION_PROPERTIES };
    xrEnumerateInstanceExtensionProperties(nullptr, extensionsCount, &extensionsCount, extensions.Get());
    Array<const char*> enabledExtensions;
    if (OpenXRGraphics::Init(extensions, enabledExtensions))
    {
        LOG(Info, "XR unavailable (runtime does not support any of the graphics APIs)");
        OpenXRGraphics::Dispose();
        return true;
    }

    // Use optional extensions if supported by the runtime
    SupportsDepthLayer = HasExtension(extensions, XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME);
    if (SupportsDepthLayer)
        enabledExtensions.Add(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME);
    SupportsDisplayRefreshRate = HasExtension(extensions, XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME);
    if (SupportsDisplayRefreshRate)
        enabledExtensions.Add(XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME);
    SupportsFrameController = HasExtension(extensions, OPENXR_VALVE_FRAME_CONTROLLER_INTERACTION_EXTENSION_NAME);
    if (SupportsFrameController)
        enabledExtensions.Add(OPENXR_VALVE_FRAME_CONTROLLER_INTERACTION_EXTENSION_NAME);

    // Engine registration
    XrInstanceCreateInfo createInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
    StringUtils::Copy(createInfo.applicationInfo.applicationName, *Globals::ProductName, XR_MAX_APPLICATION_NAME_SIZE - 1);
    createInfo.applicationInfo.applicationVersion = 1;
    StringUtils::Copy(createInfo.applicationInfo.engineName, TEXT("Flax"), XR_MAX_ENGINE_NAME_SIZE - 1);
    createInfo.applicationInfo.engineVersion = FLAXENGINE_VERSION_BUILD;
    createInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;
    createInfo.enabledExtensionCount = (uint32)enabledExtensions.Count();
    createInfo.enabledExtensionNames = enabledExtensions.Get();

    // Create OpenXR instance (fails if there is no active runtime)
    result = xrCreateInstance(&createInfo, &Instance);
    if (XR_FAILED(result))
    {
        LOG(Info, "XR unavailable ({0})", GetResultString(result));
        Instance = XR_NULL_HANDLE;
        return true;
    }

    // Get runtime info
    XrInstanceProperties instanceProperties = { XR_TYPE_INSTANCE_PROPERTIES };
    result = xrGetInstanceProperties(Instance, &instanceProperties);
    if (XR_SUCCEEDED(result))
    {
        RuntimeName = String(instanceProperties.runtimeName);
        RuntimeVersion = String::Format(TEXT("{0}.{1}.{2}"), XR_VERSION_MAJOR(instanceProperties.runtimeVersion), XR_VERSION_MINOR(instanceProperties.runtimeVersion), XR_VERSION_PATCH(instanceProperties.runtimeVersion));
    }
    LOG(Info, "OpenXR runtime: {0} {1}", RuntimeName, RuntimeVersion);

    return InitSystem();
}

bool OpenXRInstance::InitSystem()
{
    // Get head-mounted display system (fails if the headset is not connected)
    XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrResult result = xrGetSystem(Instance, &systemInfo, &SystemId);
    if (XR_FAILED(result))
    {
        if (!SystemNotFoundLogged)
        {
            SystemNotFoundLogged = true;
            LOG(Info, "XR unavailable, head-mounted display not found ({0})", GetResultString(result));
        }
        SystemId = XR_NULL_SYSTEM_ID;
        return true;
    }
    SystemNotFoundLogged = false;

    // Get system info
    XrSystemProperties systemProperties = { XR_TYPE_SYSTEM_PROPERTIES };
    result = xrGetSystemProperties(Instance, SystemId, &systemProperties);
    if (XR_FAILED(result))
    {
        LOG(Warning, "Failed to get OpenXR system properties ({0})", GetResultString(result));
        SystemId = XR_NULL_SYSTEM_ID;
        return true;
    }
    SystemName = String(systemProperties.systemName);
    LOG(Info, "OpenXR system: '{0}', VendorId: 0x{1:x}", SystemName, systemProperties.vendorId);
    LOG(Info, "	Max Swapchain Size: {0}x{1}, Max Layers: {2}, Orientation Tracking: {3}, Position Tracking: {4}", systemProperties.graphicsProperties.maxSwapchainImageWidth, systemProperties.graphicsProperties.maxSwapchainImageHeight, systemProperties.graphicsProperties.maxLayerCount, !!systemProperties.trackingProperties.orientationTracking, !!systemProperties.trackingProperties.positionTracking);

    // Setup graphics device for the headset
    if (OpenXRGraphics::Setup(Instance, SystemId))
    {
        Dispose();
        return true;
    }

    return false;
}

void OpenXRInstance::Dispose()
{
    OpenXRInput::Dispose();
    OpenXRGraphics::Dispose();
    if (Instance != XR_NULL_HANDLE)
    {
        xrDestroyInstance(Instance);
        Instance = XR_NULL_HANDLE;
    }
    SystemId = XR_NULL_SYSTEM_ID;
    RuntimeName.Clear();
    RuntimeVersion.Clear();
    SystemName.Clear();
    SupportsDepthLayer = false;
    SupportsDisplayRefreshRate = false;
    SupportsFrameController = false;
}

bool OpenXRInstance::HasExtension(const Array<XrExtensionProperties>& extensions, const char* name)
{
    for (const auto& extension : extensions)
    {
        if (StringUtils::Compare(extension.extensionName, name) == 0)
            return true;
    }
    return false;
}

bool OpenXRInstance::IsReady()
{
    return Instance != XR_NULL_HANDLE && SystemId != XR_NULL_SYSTEM_ID;
}

#define XRERR(x, value) case x: return TEXT(#x);

const Char* OpenXRInstance::GetResultString(XrResult result)
{
    switch (result)
    {
    XR_LIST_ENUM_XrResult(XRERR)
    default:
        return TEXT("XR_UNKNOWN_RESULT");
    }
}

#undef XRERR

#endif
