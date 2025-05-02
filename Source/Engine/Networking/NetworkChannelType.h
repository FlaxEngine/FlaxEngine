// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"

/// <summary>
/// The low-level network channel type for message sending.
/// </summary>
API_ENUM(Namespace="FlaxEngine.Networking") enum class NetworkChannelType
{
    /// <summary>
    /// Invalid channel type.
    /// </summary>
    None = 0,

    /// <summary>
    /// Unreliable channel type. Messages can be lost or arrive out-of-order.
    /// </summary>
    Unreliable,

    /// <summary>
    /// Unreliable-ordered channel type. Messages can be lost but always arrive in order.
    /// </summary>
    UnreliableOrdered,

    /// <summary>
    /// Reliable channel type. Messages won't be lost but may arrive out-of-order.
    /// </summary>
    Reliable,

    /// <summary>
    /// Reliable-ordered channel type. Messages won't be lost and always arrive in order.
    /// </summary>
    ReliableOrdered,
};
