// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "NetworkManager.h"

#include "NetworkMessage.h"
#include "NetworkConfig.h"
#include "NetworkHost.h"

#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"

namespace
{
    Array<NetworkHost*, HeapAllocation> Hosts;
}

NetworkHost* NetworkManager::CreateHost(const NetworkConfig& config)
{
    // Validate the address for listen/connect
    NetworkEndPoint endPoint = {};
    const bool isValidEndPoint = NetworkBase::CreateEndPoint(config.Address, String("7777"), NetworkIPVersion::IPv4, endPoint, false);
    ASSERT(config.Address == String("any") || isValidEndPoint);
    
    // Alloc new host
    const int hostId = Hosts.Count(); // TODO: Maybe keep the host count under a limit? Maybe some drivers do not support this?
                                      // TODO: Reuse host ids
    Hosts.Add(New<NetworkHost>());
    NetworkHost* host = Hosts.Last();
    host->HostId = hostId;

    // Initialize the host
    host->Initialize(config);
    
    return host;
}

void NetworkManager::ShutdownHost(NetworkHost* host)
{
    ASSERT(host->IsValid());
    host->Shutdown();
    Hosts[host->HostId] = nullptr;
    host->HostId = -1;
    
    // Hosts.Remove(host); // Do not remove hosts, because we need to keep the array unmodified to make the id's work
}
