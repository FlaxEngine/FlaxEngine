// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

enum class NetworkMessageIDs : uint8
{
    None = 0,
    Handshake,
    HandshakeReply,
    ReplicatedObject,

    MAX,
};

class NetworkInternal
{
public:
    static void NetworkReplicatorClear();
    static void NetworkReplicatorUpdate();
    static void OnNetworkMessageReplicatedObject(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer);
};
