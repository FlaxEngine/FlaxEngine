// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Compiler.h"
#include "Engine/Core/Config.h"

/// <summary>
/// Interface for objects for network replication to receive additional events.
/// </summary>
API_INTERFACE(Namespace="FlaxEngine.Networking") class FLAXENGINE_API INetworkObject
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(INetworkObject);
public:
    /// <summary>
    /// Event called when network object gets spawned.
    /// </summary>
    API_FUNCTION() virtual void OnNetworkSpawn() {};

    /// <summary>
    /// Event called when network object gets despawned.
    /// </summary>
    API_FUNCTION() virtual void OnNetworkDespawn() {};

    /// <summary>
    /// Event called before network object gets replicated (before reading data).
    /// </summary>
    API_FUNCTION() virtual void OnNetworkSerialize() {};

    /// <summary>
    /// Event called when network object gets replicated (after reading data).
    /// </summary>
    API_FUNCTION() virtual void OnNetworkDeserialize() {};

    /// <summary>
    /// Event called when network object gets synced (called only once upon initial sync).
    /// </summary>
    API_FUNCTION() virtual void OnNetworkSync() {};
};
