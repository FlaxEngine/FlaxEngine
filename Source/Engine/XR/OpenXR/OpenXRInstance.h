// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if XR_API_OPENXR

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/Array.h"
#include <ThirdParty/OpenXR/openxr/openxr.h>

/// <summary>
/// The OpenXR instance and the head-mounted display system used by the XR service.
/// </summary>
class OpenXRInstance
{
public:
    /// <summary>
    /// The OpenXR instance. Null if the runtime is not available.
    /// </summary>
    static XrInstance Instance;

    /// <summary>
    /// The head-mounted display system. Null if the headset is not connected.
    /// </summary>
    static XrSystemId SystemId;

    /// <summary>
    /// The name of the OpenXR runtime.
    /// </summary>
    static String RuntimeName;

    /// <summary>
    /// The version of the OpenXR runtime.
    /// </summary>
    static String RuntimeVersion;

    /// <summary>
    /// The name of the head-mounted display system.
    /// </summary>
    static String SystemName;

    /// <summary>
    /// True if the runtime supports submitting the depth buffer with the projection layer (XR_KHR_composition_layer_depth).
    /// </summary>
    static bool SupportsDepthLayer;

    /// <summary>
    /// True if the runtime supports getting and changing the display refresh rate (XR_FB_display_refresh_rate).
    /// </summary>
    static bool SupportsDisplayRefreshRate;

    /// <summary>
    /// True if the runtime supports the interaction profile of the Steam Frame controllers (XR_VALVE_frame_controller_interaction).
    /// </summary>
    static bool SupportsFrameController;

    /// <summary>
    /// True if the runtime supports tracking the hand joints (XR_EXT_hand_tracking).
    /// </summary>
    static bool SupportsHandTracking;

    /// <summary>
    /// True if the runtime supports selecting the motion range of the hand joints (XR_EXT_hand_joints_motion_range).
    /// </summary>
    static bool SupportsHandJointsMotionRange;

    /// <summary>
    /// True if the runtime supports reporting the source of the hand joints data (XR_EXT_hand_tracking_data_source).
    /// </summary>
    static bool SupportsHandTrackingDataSource;

public:
    /// <summary>
    /// Creates the OpenXR instance and gets the head-mounted display system.
    /// </summary>
    /// <returns>True if failed, otherwise false.</returns>
    static bool Init();

    /// <summary>
    /// Creates the OpenXR instance in the background if it's not created yet (eg. the runtime was not running when the engine started). The instance becomes available after a later Update.
    /// </summary>
    static void InitAsync();

    /// <summary>
    /// Checks if the instance creation started by InitAsync is in progress.
    /// </summary>
    /// <returns>True if the instance is being created, otherwise false.</returns>
    static bool IsInitPending();

    /// <summary>
    /// Finishes the instance creation started by InitAsync once it's done. Called on the main thread.
    /// </summary>
    static void Update();

    /// <summary>
    /// Gets the head-mounted display system. Can be called again later if the headset was not connected.
    /// </summary>
    /// <returns>True if failed, otherwise false.</returns>
    static bool InitSystem();

    /// <summary>
    /// Destroys the OpenXR instance.
    /// </summary>
    static void Dispose();

    /// <summary>
    /// Checks if the extension is in the list of the extensions supported by the runtime.
    /// </summary>
    /// <param name="extensions">The extensions supported by the runtime.</param>
    /// <param name="name">The extension name.</param>
    /// <returns>True if supported, otherwise false.</returns>
    static bool HasExtension(const Array<XrExtensionProperties>& extensions, const char* name);

    /// <summary>
    /// Checks if the OpenXR instance has been created and the head-mounted display system has been found.
    /// </summary>
    /// <returns>True if ready to use, otherwise false.</returns>
    static bool IsReady();

    /// <summary>
    /// Gets the name of the OpenXR result code.
    /// </summary>
    /// <param name="result">The result code.</param>
    /// <returns>The result code name.</returns>
    static const Char* GetResultString(XrResult result);

private:
    static bool InitExtensions();
    static bool OnInstanceCreated(XrResult result, XrInstance instance);
};

#endif
