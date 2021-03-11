// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.
// 
// TODO: Check defines so we can disable ENet

#include "ENetDriver.h"

#include "Engine/Networking/NetworkConfig.h"

#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"

#define ENET_IMPLEMENTATION
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <enet/enet.h>

#include "Engine/Networking/NetworkEvent.h"
#include "Engine/Networking/NetworkManager.h"
#undef _WINSOCK_DEPRECATED_NO_WARNINGS
#undef SendMessage

namespace
{
    bool Initialized = false;
    
    NetworkConfig Config;

    ENetHost* Server = nullptr;
    ENetHost* Client = nullptr;
    ENetPeer* ClientPeer = nullptr;
}

void ENetDriver::Initialize(const NetworkConfig& config)
{
    Config = config;

    if (enet_initialize () != 0) {
        LOG(Error, "Failed to initialize ENet driver!");
    }

    LOG(Info, "Initialized ENet driver!");
    Initialized = true;
}

void ENetDriver::Dispose()
{
    ASSERT(Initialized);
    
    if (Server != nullptr)
    {
        enet_host_destroy(Server);
        Server = nullptr;
    }
    
    if (Client != nullptr)
    {
        enet_peer_disconnect_now(ClientPeer, 0);
        enet_host_destroy(Client);
        Client = nullptr;
    }

    enet_deinitialize();
    Initialized = false;
}

bool ENetDriver::Listen()
{
    ASSERT(Client == nullptr);
    ASSERT(Server == nullptr);

    ENetAddress address = {0};
    address.host = ENET_HOST_ANY; // TODO
    address.port = Config.Port;

    Server = enet_host_create(&address, Config.ConnectionsLimit, 0, 0, 0);

    if(Server == nullptr)
    {
        LOG(Error, "Failed to initialize ENet driver!");
        return true;
    }
    
    LOG(Info, "Created ENet server!");
    return false;
}

void ENetDriver::Connect()
{
    ASSERT(Server == nullptr);
    ASSERT(Client == nullptr);
    ASSERT(ClientPeer == nullptr);
    
    LOG(Info, "Connecting using ENet...");

    ENetAddress address = {0};
    address.port = Config.Port;
    enet_address_set_host(&address, "127.0.0.1"); // TODO

    Client = enet_host_create(nullptr, 1, 0, 0, 0);
    ClientPeer = enet_host_connect(Client, &address, 0, 0);

    if(ClientPeer == nullptr)
    {
        LOG(Error, "Failed to create ENet host!");
        enet_host_destroy(Client);
        Client = nullptr;
    }
}

void ENetDriver::Disconnect()
{
    ASSERT(Client != nullptr);
    ASSERT(ClientPeer != nullptr);
    
    enet_peer_disconnect_now(ClientPeer, 0);
    enet_host_destroy(Client);
    
    Client = nullptr;
    ClientPeer = nullptr;
}

void ENetDriver::Disconnect(const NetworkConnection& connection)
{
    // TODO: kick given connection (?)
}

bool ENetDriver::PopEvent(NetworkEvent* eventPtr)
{
    ASSERT(Server != nullptr || Client != nullptr);
    
    ENetHost* host = Server != nullptr ? Server : Client; // TODO: Get host by id
    
    ENetEvent event;
    const int result = enet_host_service(host, &event, 0);

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
        case ENET_EVENT_TYPE_RECEIVE:
            eventPtr->EventType = NetworkEventType::Message;

            // Acquire message and copy message data
            eventPtr->Message = NetworkManager::CreateMessage();
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
