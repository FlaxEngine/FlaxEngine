// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#if COMPILE_WITH_PROFILER
#include "Engine/Core/Collections/Dictionary.h"
#endif

enum class NetworkMessageIDs : uint8
{
    None = 0,
    Handshake,
    HandshakeReply,
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
