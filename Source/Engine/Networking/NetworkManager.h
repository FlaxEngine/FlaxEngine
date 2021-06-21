// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"
#include "Types.h"

/// <summary>
/// Low-level network service. Provides network peer management functionality.
/// </summary>
API_CLASS(Namespace="FlaxEngine.Networking", Static) class FLAXENGINE_API NetworkManager
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(NetworkManager);
public:
    /// <summary>
    /// Creates new peer using given configuration.
    /// </summary>
    /// <param name="config">The configuration to create and setup new peer.</param>
    /// <returns>The peer.</returns>
    /// <remarks>Peer should be destroyed using <see cref="ShutdownPeer"/> once it is no longer in use.</remarks>
    API_FUNCTION()
    static NetworkPeer* CreatePeer(const NetworkConfig& config);

    /// <summary>
    /// Shutdowns and destroys given peer.
    /// </summary>
    /// <param name="peer">The peer to destroy.</param>
    API_FUNCTION()
    static void ShutdownPeer(NetworkPeer* peer);
    
};
