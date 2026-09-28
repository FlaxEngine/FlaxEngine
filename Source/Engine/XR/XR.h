// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/StringView.h"
#include "Engine/Scripting/ScriptingType.h"

/// <summary>
/// The XR service used for virtual reality head-mounted displays.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API XR
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(XR);

public:
    /// <summary>
    /// Gets a value indicating whether an XR runtime is running and a head-mounted display is connected.
    /// </summary>
    /// <returns>True if XR is available, otherwise false.</returns>
    API_PROPERTY() static bool IsAvailable();

    /// <summary>
    /// Gets the name of the XR runtime (eg. SteamVR/OpenXR). Empty if no runtime was found.
    /// </summary>
    /// <returns>The runtime name.</returns>
    API_PROPERTY() static StringView GetRuntimeName();

    /// <summary>
    /// Gets the version of the XR runtime (eg. 2.7.4). Empty if no runtime was found.
    /// </summary>
    /// <returns>The runtime version.</returns>
    API_PROPERTY() static StringView GetRuntimeVersion();

    /// <summary>
    /// Gets the name of the head-mounted display system reported by the XR runtime. Empty if no head-mounted display was found.
    /// </summary>
    /// <returns>The system name.</returns>
    API_PROPERTY() static StringView GetSystemName();
};
