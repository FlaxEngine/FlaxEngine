// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "NetworkManager.h"

#include "NetworkMessage.h"
#include "NetworkConfig.h"
#include "NetworkPeer.h"

#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"

namespace
{
    Array<NetworkPeer*, HeapAllocation> Peers;
    uint32_t LastHostId = 0;
}

NetworkPeer* NetworkManager::CreatePeer(const NetworkConfig& config)
{
    // Validate the address for listen/connect
    NetworkEndPoint endPoint = {};
    const bool isValidEndPoint = NetworkBase::CreateEndPoint(config.Address, String("7777"), NetworkIPVersion::IPv4, endPoint, false);
    ASSERT(config.Address == String("any") || isValidEndPoint);
    
    // Alloc new host
    Peers.Add(New<NetworkPeer>());
    NetworkPeer* host = Peers.Last();
    host->HostId = LastHostId++;

    // Initialize the host
    host->Initialize(config);
    
    return host;
}

void NetworkManager::ShutdownPeer(NetworkPeer* peer)
{
    ASSERT(peer->IsValid());
    peer->Shutdown();
    peer->HostId = -1;
    Peers.Remove(peer);
    
    Delete(peer);
}
