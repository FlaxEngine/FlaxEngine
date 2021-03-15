// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.
// 
// TODO: Check defines so we can disable ENet

#include "ENetDriver.h"

#include "Engine/Networking/NetworkConfig.h"
#include "Engine/Networking/NetworkEvent.h"
#include "Engine/Networking/NetworkManager.h"

#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"

#define ENET_IMPLEMENTATION
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <enet/enet.h>
#undef _WINSOCK_DEPRECATED_NO_WARNINGS
#undef SendMessage

void ENetDriver::Initialize(const NetworkConfig& config)
{
    _config = config;

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
        switch(event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            eventPtr->EventType = NetworkEventType::Connected;
            LOG(Info, "Connected"); // TODO
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            eventPtr->EventType = NetworkEventType::Disconnected;
            LOG(Info, "Disconnected"); // TODO
            break;
        case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
            eventPtr->EventType = NetworkEventType::Disconnected;
            LOG(Info, "Disconnected (timeout)"); // TODO
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            eventPtr->EventType = NetworkEventType::Message;

            // Acquire message and copy message data
            eventPtr->Message = NetworkManager::CreateMessage(eventPtr->HostId);
            eventPtr->Message.Length = event.packet->dataLength;
            Memory::CopyItems(eventPtr->Message.Buffer, event.packet->data, event.packet->dataLength);

            // TODO: Copy sender info
            break;
            
        default: break;
        }
        return true; // Event
    }

    return false; // No events
}

void ENetDriver::SendMessage(NetworkChannelType channelType, const NetworkMessage& message, Array<NetworkConnection, HeapAllocation> targets)
{
    // TODO: Send messages
}
