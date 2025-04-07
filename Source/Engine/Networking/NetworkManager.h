// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "NetworkConnection.h"
#include "Types.h"
#include "NetworkConnectionState.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingType.h"

/// <summary>
/// The high-level network manage modes.
/// </summary>
API_ENUM(Namespace="FlaxEngine.Networking") enum class NetworkManagerMode
{
    // Disabled.
    Offline = 0,
    // Server-only without a client (host a game but not participate).
    Server,
    // Client-only (connected to Server or Host).
    Client,
    // Both server and client (other clients can connect).
    Host,
};

/// <summary>
/// The high-level network client connection data. Can be used to accept/deny new connection.
/// </summary>
API_STRUCT(Namespace="FlaxEngine.Networking", NoDefault) struct NetworkClientConnectionData
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkClientConnectionData);
    // The incoming client.
    API_FIELD() NetworkClient* Client;
    // 0 if accept new connection, error code otherwise. Send back to the client.
    API_FIELD() int32 Result;
    // Client platform type (can be different that current one when using cross-play).
    API_FIELD() PlatformType Platform;
    // Client platform architecture (can be different that current one when using cross-play).
    API_FIELD() ArchitectureType Architecture;
    // Custom data to send from client to server as part of connection initialization and verification. Can contain game build info, platform data or certificate needed upon joining the game.
    API_FIELD() Array<byte> PayloadData;
};

/// <summary>
/// High-level networking manager for multiplayer games.
/// </summary>
API_CLASS(static, Namespace="FlaxEngine.Networking", Attributes="DebugCommand") class FLAXENGINE_API NetworkManager
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkManager);
public:
    /// <summary>
    /// The target amount of the network logic updates per second (frequency of replication, events sending and network ticking). Use 0 to run every game update.
    /// </summary>
    API_FIELD() static float NetworkFPS;

    /// <summary>
    /// Current network peer (low-level).
    /// </summary>
    API_FIELD(ReadOnly) static NetworkPeer* Peer;

    /// <summary>
    /// Current manager mode.
    /// </summary>
    API_FIELD(ReadOnly) static NetworkManagerMode Mode;

    /// <summary>
    /// Current network connection state.
    /// </summary>
    API_FIELD(ReadOnly) static NetworkConnectionState State;

    /// <summary>
    /// Current network system frame number (incremented every tick). Can be used for frames counting in networking and replication.
    /// </summary>
    API_FIELD(ReadOnly) static uint32 Frame;

    /// <summary>
    /// Server client identifier. Constant value of 0.
    /// </summary>
    API_FIELD(ReadOnly) static constexpr uint32 ServerClientId = 0;

    /// <summary>
    /// Local client identifier. Valid even on server that doesn't have LocalClient.
    /// </summary>
    API_FIELD(ReadOnly) static uint32 LocalClientId;

    /// <summary>
    /// Local client, valid only when Network Manager is running in client or host mode (server doesn't have a client).
    /// </summary>
    API_FIELD(ReadOnly) static NetworkClient* LocalClient;

    /// <summary>
    /// List of all clients: connecting, connected and disconnected. Empty on clients.
    /// </summary>
    API_FIELD(ReadOnly) static Array<NetworkClient*, HeapAllocation> Clients;

public:
    /// <summary>
    /// Event called when network manager state gets changed (eg. client connected to the server, or server shutdown started).
    /// </summary>
    API_EVENT() static Action StateChanged;

    /// <summary>
    /// Event called when new client is connecting. Can be used to accept/deny connection. Called on client to fill PayloadData send and called on server/host to validate incoming client connection (eg. with PayloadData validation).
    /// </summary>
    API_EVENT() static Delegate<NetworkClientConnectionData&> ClientConnecting;

    /// <summary>
    /// Event called after new client successfully connected.
    /// </summary>
    API_EVENT() static Delegate<NetworkClient*> ClientConnected;

    /// <summary>
    /// Event called after new client successfully disconnected.
    /// </summary>
    API_EVENT() static Delegate<NetworkClient*> ClientDisconnected;

public:
    // Returns true if network is a client.
    API_PROPERTY() FORCE_INLINE static bool IsClient()
    {
        return Mode == NetworkManagerMode::Client;
    }

    // Returns true if network is a server.
    API_PROPERTY() FORCE_INLINE static bool IsServer()
    {
        return Mode == NetworkManagerMode::Server;
    }

    // Returns true if network is a host (both client and server).
    API_PROPERTY() FORCE_INLINE static bool IsHost()
    {
        return Mode == NetworkManagerMode::Host;
    }

    // Returns true if network is connected and online.
    API_PROPERTY() FORCE_INLINE static bool IsConnected()
    {
        return State == NetworkConnectionState::Connected;
    }

    // Returns true if network is online or disconnected.
    API_PROPERTY() FORCE_INLINE static bool IsOffline()
    {
        return State == NetworkConnectionState::Offline || State == NetworkConnectionState::Disconnected;
    }

    /// <summary>
    /// Gets the network client for a given connection. Returns null if failed to find it.
    /// </summary>
    /// <param name="connection">Network connection identifier.</param>
    /// <returns>Found client or null.</returns>
    API_FUNCTION() static NetworkClient* GetClient(API_PARAM(Ref) const NetworkConnection& connection);

    /// <summary>
    /// Gets the network client with a given identifier. Returns null if failed to find it.
    /// </summary>
    /// <param name="clientId">Network client identifier (synchronized on all peers).</param>
    /// <returns>Found client or null.</returns>
    API_FUNCTION() static NetworkClient* GetClient(uint32 clientId);

public:
    /// <summary>
    /// Starts the network in server mode. Returns true if failed (eg. invalid config).
    /// </summary>
    API_FUNCTION() static bool StartServer();

    /// <summary>
    /// Starts the network in client mode. Returns true if failed (eg. invalid config).
    /// </summary>
    API_FUNCTION() static bool StartClient();

    /// <summary>
    /// Starts the network in host mode. Returns true if failed (eg. invalid config).
    /// </summary>
    API_FUNCTION() static bool StartHost();

    /// <summary>
    /// Stops the network.
    /// </summary>
    API_FUNCTION() static void Stop();
};
