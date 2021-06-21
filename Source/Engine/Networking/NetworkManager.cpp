// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "NetworkManager.h"

#include "NetworkMessage.h"
#include "NetworkConfig.h"
#include "NetworkPeer.h"

#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"

namespace
{
    Array<NetworkPeer*, HeapAllocation> Hosts;
    uint32_t LastHostId = 0;
}

NetworkPeer* NetworkManager::CreateHost(const NetworkConfig& config)
{
    // Validate the address for listen/connect
    NetworkEndPoint endPoint = {};
    const bool isValidEndPoint = NetworkBase::CreateEndPoint(config.Address, String("7777"), NetworkIPVersion::IPv4, endPoint, false);
    ASSERT(config.Address == String("any") || isValidEndPoint);
    
    // Alloc new host
    Hosts.Add(New<NetworkPeer>());
    NetworkPeer* host = Hosts.Last();
    host->HostId = LastHostId++;

    // Initialize the host
    host->Initialize(config);
    
    return host;
}

void NetworkManager::ShutdownHost(NetworkPeer* host)
{
    ASSERT(host->IsValid());
    host->Shutdown();
    host->HostId = -1;
    Hosts.Remove(host);
    
    Delete(host);
}
