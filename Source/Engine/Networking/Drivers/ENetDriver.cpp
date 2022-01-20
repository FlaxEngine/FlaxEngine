// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

// TODO: Check defines so we can disable ENet

#include "ENetDriver.h"
#include "Engine/Networking/NetworkConfig.h"
#include "Engine/Networking/NetworkChannelType.h"
#include "Engine/Networking/NetworkEvent.h"
#include "Engine/Networking/NetworkPeer.h"
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
    if (channel > NetworkChannelType::UnreliableOrdered)
        flag |= ENET_PACKET_FLAG_RELIABLE;

    // Use unsequenced flag when the flag is unreliable. We have to sequence all other packets.
    if (channel == NetworkChannelType::Unreliable)
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
    enet_peer_send(peer, 0, packet);

    // TODO: To reduce latency, we can use `enet_host_flush` to flush all packets. Maybe some API, like NetworkManager::FlushQueues()?
}

ENetDriver::ENetDriver(const SpawnParams& params)
    : ScriptingObject(params)
{
}

bool ENetDriver::Initialize(NetworkPeer* host, const NetworkConfig& config)
{
    _networkHost = host;
    _config = config;
    _peerMap = Dictionary<uint32, void*>();

    if (enet_initialize() != 0)
    {
        LOG(Error, "Failed to initialize ENet driver!");
        return true;
    }

    LOG(Info, "Initialized ENet driver");
    return false;
}

void ENetDriver::Dispose()
{
    if (_peer)
        enet_peer_disconnect_now((ENetPeer*)_peer, 0);
    enet_host_destroy((ENetHost*)_host);

    enet_deinitialize();

    _peerMap.Clear();
    _peerMap = {};

    _peer = nullptr;
    _host = nullptr;

    LOG(Info, "ENet driver stopped!");
}

bool ENetDriver::Listen()
{
    ENetAddress address = { 0 };
    address.port = _config.Port;
    address.host = ENET_HOST_ANY;

    // Set host address if needed
    if (_config.Address != String("any"))
        enet_address_set_host(&address, _config.Address.ToStringAnsi().GetText());

    // Create ENet host
    _host = enet_host_create(&address, _config.ConnectionsLimit, 1, 0, 0);
    if (_host == nullptr)
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

    ENetAddress address = { 0 };
    address.port = _config.Port;
    enet_address_set_host(&address, _config.Address.ToStringAnsi().GetText());

    // Create ENet host
    _host = enet_host_create(nullptr, 1, 1, 0, 0);
    if (_host == nullptr)
    {
        LOG(Error, "Failed to initialize ENet host!");
        return false;
    }

    // Create ENet peer/connect to the server
    _peer = enet_host_connect((ENetHost*)_host, &address, 1, 0);
    if (_peer == nullptr)
    {
        LOG(Error, "Failed to create ENet host!");
        enet_host_destroy((ENetHost*)_host);
        return false;
    }

    return true;
}

void ENetDriver::Disconnect()
{
    if (_peer)
    {
        enet_peer_disconnect_now((ENetPeer*)_peer, 0);
        _peer = nullptr;

        LOG(Info, "Disconnected");
    }
}

void ENetDriver::Disconnect(const NetworkConnection& connection)
{
    const int connectionId = connection.ConnectionId;

    void* peer = nullptr;
    if (_peerMap.TryGet(connectionId, peer))
    {
        enet_peer_disconnect_now((ENetPeer*)peer, 0);
        _peerMap.Remove(connectionId);
    }
    else
    {
        LOG(Error, "Failed to kick connection({0}). ENetPeer not found!", connection.ConnectionId);
    }
}

bool ENetDriver::PopEvent(NetworkEvent* eventPtr)
{
    ENetEvent event;
    const int result = enet_host_service((ENetHost*)_host, &event, 0);

    if (result < 0)
        LOG(Error, "Failed to check ENet events!");

    if (result > 0)
    {
        // Copy sender data
        const uint32 connectionId = enet_peer_get_id(event.peer);
        eventPtr->Sender = NetworkConnection();
        eventPtr->Sender.ConnectionId = connectionId;

        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            eventPtr->EventType = NetworkEventType::Connected;

            if (IsServer())
                _peerMap.Add(connectionId, event.peer);
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            eventPtr->EventType = NetworkEventType::Disconnected;

            if (IsServer())
                _peerMap.Remove(connectionId);
            break;

        case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
            eventPtr->EventType = NetworkEventType::Timeout;

            if (IsServer())
                _peerMap.Remove(connectionId);
            break;

        case ENET_EVENT_TYPE_RECEIVE:
            eventPtr->EventType = NetworkEventType::Message;

            // Acquire message and copy message data
            eventPtr->Message = _networkHost->CreateMessage();
            eventPtr->Message.Length = event.packet->dataLength;
            Memory::CopyItems(eventPtr->Message.Buffer, event.packet->data, event.packet->dataLength);
            break;

        default:
            break;
        }
        return true; // Event
    }

    return false; // No events
}

void ENetDriver::SendMessage(const NetworkChannelType channelType, const NetworkMessage& message)
{
    ASSERT(IsServer() == false);

    SendPacketToPeer((ENetPeer*)_peer, channelType, message);
}

void ENetDriver::SendMessage(NetworkChannelType channelType, const NetworkMessage& message, NetworkConnection target)
{
    ASSERT(IsServer());

    ENetPeer* peer = *(ENetPeer**)_peerMap.TryGet(target.ConnectionId);
    ASSERT(peer != nullptr);
    ASSERT(peer->state == ENET_PEER_STATE_CONNECTED);

    SendPacketToPeer(peer, channelType, message);
}

void ENetDriver::SendMessage(const NetworkChannelType channelType, const NetworkMessage& message, const Array<NetworkConnection, HeapAllocation>& targets)
{
    ASSERT(IsServer());

    for (NetworkConnection target : targets)
    {
        ENetPeer* peer = *(ENetPeer**)_peerMap.TryGet(target.ConnectionId);
        ASSERT(peer != nullptr);
        ASSERT(peer->state == ENET_PEER_STATE_CONNECTED);

        SendPacketToPeer(peer, channelType, message);
    }
}
