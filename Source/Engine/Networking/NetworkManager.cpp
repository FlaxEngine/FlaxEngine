// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "NetworkManager.h"

#include "NetworkMessage.h"
#include "NetworkConfig.h"
#include "NetworkConnection.h"
#include "INetworkDriver.h"
#include "NetworkEvent.h"
#include "NetworkHost.h"

#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Math.h"

namespace
{
    Array<NetworkHost, HeapAllocation> Hosts;
}

int NetworkManager::Initialize(const NetworkConfig& config)
{
    // Validate the address for listen/connect
    NetworkEndPoint endPoint = {};
    const bool isValidEndPoint = NetworkBase::CreateEndPoint(config.Address, String("7777"), NetworkIPVersion::IPv4, endPoint, false);
    ASSERT(config.Address == String("any") || isValidEndPoint);
    
    // Alloc new host
    const int hostId = Hosts.Count();
    Hosts.Add(NetworkHost());
    NetworkHost& host = Hosts.Last();
    host.HostId = hostId;

    // Initialize the host
    host.Initialize(config);
    
    return hostId;
}

void NetworkManager::Shutdown(const int hostId)
{
    ASSERT(Hosts[hostId].IsValid());
    NetworkHost& host = Hosts[hostId];
    host.Shutdown();
    Hosts.Remove(host);
}

bool NetworkManager::Listen(const int hostId)
{
    ASSERT(Hosts[hostId].IsValid());
    NetworkHost& host = Hosts[hostId];
    
    LOG(Info, "NetworkManager starting to listen on address = {0}:{1}", host.Config.Address, host.Config.Port);
    
    return host.NetworkDriver->Listen();
}

bool NetworkManager::Connect(const int hostId)
{
    ASSERT(Hosts[hostId].IsValid());
    NetworkHost& host = Hosts[hostId];
    LOG(Info, "Connecting to {0}:{1}...", host.Config.Address, host.Config.Port);
    
    return host.NetworkDriver->Connect();
}

void NetworkManager::Disconnect(const int hostId)
{
    ASSERT(Hosts[hostId].IsValid());
    NetworkHost& host = Hosts[hostId];
    LOG(Info, "Disconnecting...");
    
    host.NetworkDriver->Disconnect();
}

void NetworkManager::Disconnect(const int hostId, const NetworkConnection& connection)
{
    ASSERT(Hosts[hostId].IsValid());
    NetworkHost& host = Hosts[hostId];
    LOG(Info, "Disconnecting connection with id = {0}...", connection.ConnectionId);
    
    host.NetworkDriver->Disconnect(connection);
}

bool NetworkManager::PopEvent(const int hostId, NetworkEvent& eventPtr)
{
    ASSERT(Hosts[hostId].IsValid());
    NetworkHost& host = Hosts[hostId];

    // Set host id of the event
    eventPtr.HostId = hostId;
    
    return host.NetworkDriver->PopEvent(&eventPtr);
}

NetworkMessage NetworkManager::CreateMessage(const int hostId)
{
    ASSERT(Hosts[hostId].IsValid());
    NetworkHost& host = Hosts[hostId];
    ASSERT(host.MessagePool.Count() > 0);

    const uint32 messageId = host.MessagePool.Pop();
    uint8* messageBuffer = host.GetMessageBuffer(messageId);

    return NetworkMessage(messageBuffer, messageId, host.Config.MessageSize, 0, 0);
}

void NetworkManager::RecycleMessage(const int hostId, const NetworkMessage& message)
{
    ASSERT(Hosts[hostId].IsValid());
    NetworkHost& host = Hosts[hostId];
    ASSERT(message.IsValid());
#ifdef BUILD_DEBUG
    ASSERT(host.MessagePool.Contains(message.MessageId) == false);
#endif
    
    // Return the message id
    host.MessagePool.Push(message.MessageId);
}

NetworkMessage NetworkManager::BeginSendMessage(const int hostId)
{
    ASSERT(Hosts[hostId].IsValid());
    return CreateMessage(hostId);
}

void NetworkManager::AbortSendMessage(const int hostId, const NetworkMessage& message)
{
    ASSERT(Hosts[hostId].IsValid());
    ASSERT(message.IsValid());
    RecycleMessage(hostId, message);
}

bool NetworkManager::EndSendMessage(const int hostId, const NetworkChannelType channelType, const NetworkMessage& message)
{
    ASSERT(Hosts[hostId].IsValid());
    NetworkHost& host = Hosts[hostId];
    ASSERT(message.IsValid());
    
    host.NetworkDriver->SendMessage(channelType, message);

    RecycleMessage(hostId, message);
    return false;
}

bool NetworkManager::EndSendMessage(const int hostId, const NetworkChannelType channelType, const NetworkMessage& message, const NetworkConnection& target)
{
    ASSERT(Hosts[hostId].IsValid());
    NetworkHost& host = Hosts[hostId];
    ASSERT(message.IsValid());
    
    host.NetworkDriver->SendMessage(channelType, message, target);

    RecycleMessage(hostId, message);
    return false;
}

bool NetworkManager::EndSendMessage(const int hostId, const NetworkChannelType channelType, const NetworkMessage& message, const Array<NetworkConnection> targets)
{
    ASSERT(Hosts[hostId].IsValid());
    NetworkHost& host = Hosts[hostId];
    ASSERT(message.IsValid());
    
    host.NetworkDriver->SendMessage(channelType, message, targets);

    RecycleMessage(hostId, message);
    return false;
}
