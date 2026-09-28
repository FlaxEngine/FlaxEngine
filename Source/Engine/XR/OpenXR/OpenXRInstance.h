// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if XR_API_OPENXR

#include "Engine/Core/Types/String.h"

/// <summary>
/// The OpenXR instance and the head-mounted display system used by the XR service.
/// </summary>
class OpenXRInstance
{
public:
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
    /// Destroys the OpenXR instance.
    /// </summary>
    static void Dispose();

    /// <summary>
    /// Checks if the OpenXR instance has been created and the head-mounted display system has been found.
    /// </summary>
    /// <returns>True if ready to use, otherwise false.</returns>
    static bool IsReady();
};

#endif
