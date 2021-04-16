// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.
// 
// TODO: Check defines so we can disable ENet

#include "ENetDriver.h"

#include "Engine/Networking/NetworkConfig.h"
#include "Engine/Networking/NetworkChannelType.h"
#include "Engine/Networking/NetworkEvent.h"
#include "Engine/Networking/NetworkManager.h"

#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"

#define ENET_IMPLEMENTATION
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <enet/enet.h>
#undef _WINSOCK_DEPRECATED_NO_WARNINGS
#undef SendMessage

ENetPacketFlag ChannelTypeToPacketFlag(const NetworkChannelType channel)
{
    int flag = 0; // Maybe use ENET_PACKET_FLAG_NO_ALLOCATE?

    // Add reliable flag when it is "reliable" channel
    if(channel > NetworkChannelType::UnreliableOrdered)
        flag |= ENET_PACKET_FLAG_RELIABLE;

    // Use unsequenced flag when the flag is unreliable. We have to sequence all other packets.
    if(channel == NetworkChannelType::Unreliable)
        flag |= ENET_PACKET_FLAG_UNSEQUENCED;

    // Note that all reliable channels are exactly the same. TODO: How to handle unordered reliable packets...?
    
    return static_cast<ENetPacketFlag>(flag);
}

void SendPacketToPeer(ENetPeer* peer, const NetworkChannelType channelType, const NetworkMessage& message)
{
    // Covert our channel type to the internal ENet packet flags
    const ENetPacketFlag flag = ChannelTypeToPacketFlag(channelType);

    // This will copy the data into the packet when ENET_PACKET_FLAG_NO_ALLOCATE is not set.
    // Tho, we cannot use it, because we're releasing the message right after the send - and the packet might not
    // be sent, yet. To avoid data corruption, we're just using the copy method. We might fix that later, but I'll take
    // the smaller risk.
    ENetPacket* packet = enet_packet_create(message.Buffer, message.Length, flag);

    // And send it!
    enet_peer_send (peer, 0, packet);

    // TODO: To reduce latency, we can use `enet_host_flush` to flush all packets. Maybe some API, like NetworkManager::FlushQueues()?
}

void ENetDriver::Initialize(const NetworkConfig& config)
{
    _config = config;
    _peerMap = Dictionary<NetworkConnection, void*>();

    if (enet_initialize () != 0) {
        LOG(Error, "Failed to initialize ENet driver!");
    }

    LOG(Info, "Initialized ENet driver!");
}

void ENetDriver::Dispose()
{
    if(_peer)
        enet_peer_disconnect_now((ENetPeer*)_peer, 0);
    enet_host_destroy((ENetHost*)_host);
    
    enet_deinitialize();
    
    _peerMap.Clear();
    _peerMap = {};
    
    _peer = nullptr;
    _host = nullptr;
}

bool ENetDriver::Listen()
{
    ENetAddress address = {0};
    address.host = ENET_HOST_ANY; // TODO
    address.port = _config.Port;
    
    // Create ENet host
    _host = enet_host_create(&address, _config.ConnectionsLimit, 1, 0, 0);
    if(_host == nullptr)
    {
        LOG(Error, "Failed to initialize ENet host!");
        return false;
    }
    
    LOG(Info, "Created ENet server!");
    return true;
}

bool ENetDriver::Connect()
{
    LOG(Info, "Connecting using ENet...");

    ENetAddress address = {0};
    address.port = _config.Port;
    enet_address_set_host(&address, "127.0.0.1"); // TODO

    // Create ENet host
    _host = enet_host_create(nullptr, 1, 1, 0, 0);
    if(_host == nullptr)
    {
        LOG(Error, "Failed to initialize ENet host!");
        return false;
    }
    
    // Create ENet peer/connect to the server
    _peer = enet_host_connect((ENetHost*)_host, &address, 1, 0);
    if(_peer == nullptr)
    {
        LOG(Error, "Failed to create ENet host!");
        enet_host_destroy((ENetHost*)_host);
        return false;
    }

    return true;
}

void ENetDriver::Disconnect()
{
    ASSERT(_peer != nullptr);
    
    if(_peer)
    {
        enet_peer_disconnect_now((ENetPeer*)_peer, 0);
        _peer = nullptr;
        
        LOG(Info, "Disconnected");
    }
}

void ENetDriver::Disconnect(const NetworkConnection& connection)
{
    // TODO: kick given connection (?)
}

bool ENetDriver::PopEvent(NetworkEvent* eventPtr)
{
    ENetEvent event;
    const int result = enet_host_service((ENetHost*)_host, &event, 0);

    if(result < 0)
        LOG(Error, "Failed to check ENet events!");
    
    if(result > 0)
    {
        // Copy sender data
        eventPtr->Sender = NetworkConnection(enet_peer_get_id(event.peer));
        
        switch(event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            eventPtr->EventType = NetworkEventType::Connected;

            if(IsServer())
                _peerMap.Add(eventPtr->Sender, event.peer);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            eventPtr->EventType = NetworkEventType::Disconnected;
            
            if(IsServer() && _peerMap.ContainsKey(eventPtr->Sender))
                _peerMap.Remove(eventPtr->Sender);
            break;
        case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
            eventPtr->EventType = NetworkEventType::Timeout;
            
            if(IsServer() && _peerMap.ContainsKey(eventPtr->Sender))
                _peerMap.Remove(eventPtr->Sender);
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            eventPtr->EventType = NetworkEventType::Message;

            // Acquire message and copy message data
            eventPtr->Message = NetworkManager::CreateMessage(eventPtr->HostId);
            eventPtr->Message.Length = event.packet->dataLength;
            Memory::CopyItems(eventPtr->Message.Buffer, event.packet->data, event.packet->dataLength);
            break;
            
        default: break;
        }
        return true; // Event
    }

    return false; // No events
}

void ENetDriver::SendMessage(const NetworkChannelType channelType, const NetworkMessage& message)
{
    SendPacketToPeer((ENetPeer*)_peer, channelType, message);
}

void ENetDriver::SendMessage(const NetworkChannelType channelType, const NetworkMessage& message, Array<NetworkConnection, HeapAllocation> targets)
{
    for(NetworkConnection connection&& : targets)
    {
        ASSERT(_peerMap.ContainsKey(connection));
        
        ENetPeer* peer = (ENetPeer*)_peerMap.TryGet(connection);
        ASSERT(peer != nullptr);
        
        SendPacketToPeer(peer, channelType, message);
    }
}
