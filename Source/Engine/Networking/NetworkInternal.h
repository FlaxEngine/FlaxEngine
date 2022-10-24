// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

enum class NetworkMessageIDs : uint8
{
    None = 0,
    Handshake,
    HandshakeReply,
    ReplicatedObject,
    SpawnObject,

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
    static void OnNetworkMessageReplicatedObject(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer);
    static void OnNetworkMessageSpawnObject(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer);
};
