// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

enum class NetworkMessageIDs : uint8
{
    None = 0,
    Handshake,
    HandshakeReply,
    ObjectReplicate,
    ObjectSpawn,
    ObjectDespawn,
    ObjectRole,

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
    static void OnNetworkMessageObjectSpawn(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer);
    static void OnNetworkMessageObjectDespawn(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer);
    static void OnNetworkMessageObjectRole(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer);
};
