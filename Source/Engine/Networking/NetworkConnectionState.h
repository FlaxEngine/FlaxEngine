// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"

/// <summary>
/// The high-level network connection state.
/// </summary>
API_ENUM(Namespace="FlaxEngine.Networking") enum class NetworkConnectionState
{
    /// <summary>
    /// Not connected.
    /// </summary>
    Offline = 0,

    /// <summary>
    /// Connection process was started but not yet finished.
    /// </summary>
    Connecting,

    /// <summary>
    /// Connection has been made.
    /// </summary>
    Connected,

    /// <summary>
    /// Disconnection process was started but not yet finished.
    /// </summary>
    Disconnecting,

    /// <summary>
    /// Connection ended.
    /// </summary>
    Disconnected,
};
