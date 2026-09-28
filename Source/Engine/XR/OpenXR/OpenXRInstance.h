// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if XR_API_OPENXR

#include "Engine/Core/Types/String.h"
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

public:
    /// <summary>
    /// Creates the OpenXR instance and gets the head-mounted display system.
    /// </summary>
    /// <returns>True if failed, otherwise false.</returns>
    static bool Init();

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
};

#endif
