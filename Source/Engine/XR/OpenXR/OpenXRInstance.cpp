// Copyright (c) Wojciech Figat. All rights reserved.

#if XR_API_OPENXR

#include "OpenXRInstance.h"
#include "OpenXRGraphicsD3D11.h"
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
    bool hasGraphicsExtension = false;
    for (const auto& extension : extensions)
    {
        if (StringUtils::Compare(extension.extensionName, OpenXRGraphicsD3D11::ExtensionName) == 0)
        {
            hasGraphicsExtension = true;
            break;
        }
    }
    if (!hasGraphicsExtension)
    {
        LOG(Info, "XR unavailable (missing {0} extension)", String(OpenXRGraphicsD3D11::ExtensionName));
        return true;
    }

    // Engine registration
    const char* enabledExtensions[] = { OpenXRGraphicsD3D11::ExtensionName };
    XrInstanceCreateInfo createInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
    StringUtils::Copy(createInfo.applicationInfo.applicationName, *Globals::ProductName, XR_MAX_APPLICATION_NAME_SIZE - 1);
    createInfo.applicationInfo.applicationVersion = 1;
    StringUtils::Copy(createInfo.applicationInfo.engineName, TEXT("Flax"), XR_MAX_ENGINE_NAME_SIZE - 1);
    createInfo.applicationInfo.engineVersion = FLAXENGINE_VERSION_BUILD;
    createInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;
    createInfo.enabledExtensionCount = ARRAY_COUNT(enabledExtensions);
    createInfo.enabledExtensionNames = enabledExtensions;

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
    if (OpenXRGraphicsD3D11::SetupDevice(Instance, SystemId))
    {
        Dispose();
        return true;
    }

    return false;
}

void OpenXRInstance::Dispose()
{
    if (Instance != XR_NULL_HANDLE)
    {
        xrDestroyInstance(Instance);
        Instance = XR_NULL_HANDLE;
    }
    SystemId = XR_NULL_SYSTEM_ID;
    RuntimeName.Clear();
    RuntimeVersion.Clear();
    SystemName.Clear();
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
