// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Compiler.h"
#include "Engine/Core/Config.h"

/// <summary>
/// The network transport driver statistics container. Contains information about INetworkDriver usage and performance.
/// </summary>
API_STRUCT(Namespace="FlaxEngine.Networking") struct FLAXENGINE_API NetworkDriverStats
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkDriverStats);

    /// <summary>
    /// The mean round trip time (RTT), in milliseconds, between sending a reliable packet and receiving its acknowledgement. Also known as ping time.
    /// </summary>
    API_FIELD() float RTT = 0.0f;
};

template<>
struct TIsPODType<NetworkDriverStats>
{
    enum { Value = true };
};
