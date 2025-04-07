// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#if COMPILE_WITH_PROFILER
#include "Engine/Core/Collections/Dictionary.h"
#endif

// Internal version number of networking implementation. Updated once engine changes serialization or connection rules.
#define NETWORK_PROTOCOL_VERSION 4

// Enables encoding object ids and typenames via uint32 keys rather than full data send.
#define USE_NETWORK_KEYS 1

// Cached replication messages if contents didn't change
#define USE_NETWORK_REPLICATOR_CACHE 1

enum class NetworkMessageIDs : uint8
{
    None = 0,
    Handshake,
    HandshakeReply,
    Key,
    ObjectReplicate,
    ObjectReplicatePart,
    ObjectSpawn,
    ObjectSpawnPart,
    ObjectDespawn,
    ObjectRole,
    ObjectRpc,

    MAX,
};

class NetworkInternal
{
public:
    static void NetworkReplicatorClientConnected(NetworkClient* client);
    static void NetworkReplicatorClientDisconnected(NetworkClient* client);
    static void NetworkReplicatorClear();
    static void NetworkReplicatorPreUpdate();
    static void NetworkReplicatorUpdate();
    static void OnNetworkMessageObjectReplicate(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer);
    static void OnNetworkMessageObjectReplicatePart(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer);
    static void OnNetworkMessageObjectSpawn(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer);
    static void OnNetworkMessageObjectSpawnPart(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer);
    static void OnNetworkMessageObjectDespawn(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer);
    static void OnNetworkMessageObjectRole(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer);
    static void OnNetworkMessageObjectRpc(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer);

#if COMPILE_WITH_PROFILER

    struct ProfilerEvent
    {
        uint16 Count = 0;
        uint16 DataSize = 0;
        uint16 MessageSize = 0;
        uint16 Receivers = 0;
    };

    /// <summary>
    /// Enables network usage profiling tools. Captures network objects replication and RPCs send statistics.
    /// </summary>
    static bool EnableProfiling;

    static Dictionary<Pair<ScriptingTypeHandle, StringAnsiView>, ProfilerEvent> ProfilerEvents;
#endif
};
