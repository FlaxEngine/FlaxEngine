// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Compiler.h"
#include "Engine/Core/Config.h"

/// <summary>
/// Interface for objects for network replication to receive additional events.
/// </summary>
API_INTERFACE(Namespace = "FlaxEngine.Networking") class FLAXENGINE_API INetworkObject
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(INetworkObject);
public:
    /// <summary>
    /// Event called when network objects gets spawned.
    /// </summary>
    API_FUNCTION() virtual void OnNetworkSpawn() = 0;

    /// <summary>
    /// Event called when network objects gets despawned.
    /// </summary>
    API_FUNCTION() virtual void OnNetworkDespawn() = 0;

    /// <summary>
    /// Event called before network object gets replicated (before reading data).
    /// </summary>
    API_FUNCTION() virtual void OnNetworkSerialize() = 0;

    /// <summary>
    /// Event called when network objects gets replicated (after reading data).
    /// </summary>
    API_FUNCTION() virtual void OnNetworkDeserialize() = 0;
};
