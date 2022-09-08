// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "NetworkConnection.h"
#include "NetworkConnectionState.h"
#include "Engine/Scripting/ScriptingObject.h"

/// <summary>
/// High-level network client object (local or connected to the server).
/// </summary>
API_CLASS(sealed, NoSpawn, Namespace = "FlaxEngine.Networking") class FLAXENGINE_API NetworkClient final : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(NetworkClient);
    friend class NetworkManager;
    explicit NetworkClient(NetworkConnection connection);

public:
    /// <summary>
    /// Identifier of the client (connection id from local peer).
    /// </summary>
    API_FIELD(ReadOnly) NetworkConnection Connection;

    /// <summary>
    /// Client connection state.
    /// </summary>
    API_FIELD(ReadOnly) NetworkConnectionState State;

public:
    String ToString() const override
    {
        return String::Format(TEXT("NetworkClient Id={0}"), Connection.ConnectionId);
    }
};
