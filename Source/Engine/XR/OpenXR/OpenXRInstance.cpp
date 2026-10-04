// Copyright (c) Wojciech Figat. All rights reserved.

#if XR_API_OPENXR

#include "OpenXRInstance.h"
#include "OpenXRInput.h"
#include "OpenXRGraphics.h"
#include "Engine/XR/XRSettings.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Platform/StringUtils.h"
#include "Engine/Threading/Task.h"
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
bool OpenXRInstance::SupportsHandTracking = false;
bool OpenXRInstance::SupportsHandJointsMotionRange = false;
bool OpenXRInstance::SupportsHandTrackingDataSource = false;

// Not included in the OpenXR SDK headers yet
#define OPENXR_VALVE_FRAME_CONTROLLER_INTERACTION_EXTENSION_NAME "XR_VALVE_frame_controller_interaction"

namespace
{
    bool SystemNotFoundLogged = false;
    Array<const char*> EnabledExtensions;
    Task* InitTask = nullptr;
    XrInstance InitTaskInstance = XR_NULL_HANDLE;
    XrResult InitTaskResult = XR_SUCCESS;

    XrResult CreateInstance(XrInstance& instance)
    {
        // Engine registration
        XrInstanceCreateInfo createInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
        StringUtils::Copy(createInfo.applicationInfo.applicationName, *Globals::ProductName, XR_MAX_APPLICATION_NAME_SIZE - 1);
        createInfo.applicationInfo.applicationVersion = 1;
        StringUtils::Copy(createInfo.applicationInfo.engineName, TEXT("Flax"), XR_MAX_ENGINE_NAME_SIZE - 1);
        createInfo.applicationInfo.engineVersion = FLAXENGINE_VERSION_BUILD;
        createInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;
        createInfo.enabledExtensionCount = (uint32)EnabledExtensions.Count();
        createInfo.enabledExtensionNames = EnabledExtensions.Get();

        // Create OpenXR instance (fails if there is no active runtime, can take a few seconds if the runtime has to be started)
        instance = XR_NULL_HANDLE;
        const XrResult result = xrCreateInstance(&createInfo, &instance);
        if (XR_FAILED(result))
            instance = XR_NULL_HANDLE;
        return result;
    }

    void CreateInstanceTask()
    {
        InitTaskResult = CreateInstance(InitTaskInstance);
    }
}

bool OpenXRInstance::Init()
{
    if (InitExtensions())
        return true;
    XrInstance instance;
    const XrResult result = CreateInstance(instance);
    return OnInstanceCreated(result, instance);
}

void OpenXRInstance::InitAsync()
{
    // Skip if the instance is already created or pending
    if (Instance != XR_NULL_HANDLE || InitTask != nullptr)
        return;
    if (InitExtensions())
        return;

    // Create the instance in the background (the runtime can block for a few seconds when it's starting)
    InitTask = Task::StartNew(CreateInstanceTask);
    if (InitTask == nullptr)
        Dispose();
}

bool OpenXRInstance::IsInitPending()
{
    return InitTask != nullptr;
}

void OpenXRInstance::Update()
{
    // Finish the instance creation on the main thread
    if (InitTask != nullptr && InitTask->IsEnded())
    {
        InitTask = nullptr;
        OnInstanceCreated(InitTaskResult, InitTaskInstance);
        InitTaskInstance = XR_NULL_HANDLE;
    }
}

bool OpenXRInstance::InitExtensions()
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
    EnabledExtensions.Clear();
    if (OpenXRGraphics::Init(extensions, EnabledExtensions))
    {
        LOG(Info, "XR unavailable (runtime does not support any of the graphics APIs)");
        OpenXRGraphics::Dispose();
        return true;
    }

    // Use optional extensions if supported by the runtime
    SupportsDepthLayer = HasExtension(extensions, XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME);
    if (SupportsDepthLayer)
        EnabledExtensions.Add(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME);
    SupportsDisplayRefreshRate = HasExtension(extensions, XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME);
    if (SupportsDisplayRefreshRate)
        EnabledExtensions.Add(XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME);
    SupportsFrameController = HasExtension(extensions, OPENXR_VALVE_FRAME_CONTROLLER_INTERACTION_EXTENSION_NAME);
    if (SupportsFrameController)
        EnabledExtensions.Add(OPENXR_VALVE_FRAME_CONTROLLER_INTERACTION_EXTENSION_NAME);
    if (XRSettings::Get()->HandTracking)
    {
        SupportsHandTracking = HasExtension(extensions, XR_EXT_HAND_TRACKING_EXTENSION_NAME);
        if (SupportsHandTracking)
        {
            EnabledExtensions.Add(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
            SupportsHandJointsMotionRange = HasExtension(extensions, XR_EXT_HAND_JOINTS_MOTION_RANGE_EXTENSION_NAME);
            if (SupportsHandJointsMotionRange)
                EnabledExtensions.Add(XR_EXT_HAND_JOINTS_MOTION_RANGE_EXTENSION_NAME);
            SupportsHandTrackingDataSource = HasExtension(extensions, XR_EXT_HAND_TRACKING_DATA_SOURCE_EXTENSION_NAME);
            if (SupportsHandTrackingDataSource)
                EnabledExtensions.Add(XR_EXT_HAND_TRACKING_DATA_SOURCE_EXTENSION_NAME);
        }
    }

    return false;
}

bool OpenXRInstance::OnInstanceCreated(XrResult result, XrInstance instance)
{
    if (XR_FAILED(result))
    {
        LOG(Info, "XR unavailable ({0})", GetResultString(result));
        Dispose();
        return true;
    }
    Instance = instance;

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
    if (InitTask != nullptr)
    {
        // Wait for the pending instance creation
        InitTask->Wait();
        InitTask = nullptr;
        if (InitTaskInstance != XR_NULL_HANDLE)
        {
            xrDestroyInstance(InitTaskInstance);
            InitTaskInstance = XR_NULL_HANDLE;
        }
    }
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
    SupportsHandTracking = false;
    SupportsHandJointsMotionRange = false;
    SupportsHandTrackingDataSource = false;
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
