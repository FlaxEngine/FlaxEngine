// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "NetworkConnection.h"
#include "NetworkConnectionState.h"
#include "Engine/Scripting/ScriptingObject.h"

/// <summary>
/// High-level network client object (local or connected to the server).
/// </summary>
API_CLASS(sealed, NoSpawn, Namespace="FlaxEngine.Networking") class FLAXENGINE_API NetworkClient final : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(NetworkClient);
    friend class NetworkManager;
    explicit NetworkClient(uint32 id, NetworkConnection connection);

public:
    /// <summary>
    /// Unique client identifier.
    /// </summary>
    API_FIELD(ReadOnly) uint32 ClientId;

    /// <summary>
    /// Local peer connection.
    /// </summary>
    API_FIELD(ReadOnly) NetworkConnection Connection;

    /// <summary>
    /// Client connection state.
    /// </summary>
    API_FIELD(ReadOnly) NetworkConnectionState State;

public:
    String ToString() const override
    {
        return String::Format(TEXT("NetworkClient Id={0}, ConnectionId={1}"), ClientId, Connection.ConnectionId);
    }
};
