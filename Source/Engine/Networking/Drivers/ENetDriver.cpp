// Copyright (c) Wojciech Figat. All rights reserved.

// TODO: Check defines so we can disable ENet

#include "ENetDriver.h"
#include "Engine/Networking/NetworkConfig.h"
#include "Engine/Networking/NetworkChannelType.h"
#include "Engine/Networking/NetworkEvent.h"
#include "Engine/Networking/NetworkPeer.h"
#include "Engine/Networking/NetworkStats.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Profiler/ProfilerCPU.h"
#define ENET_IMPLEMENTATION
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <enet/enet.h>
#undef _WINSOCK_DEPRECATED_NO_WARNINGS
#undef SendMessage

ENetPacketFlag ChannelTypeToPacketFlag(const NetworkChannelType channel)
{
    int flag = 0; // Maybe use ENET_PACKET_FLAG_NO_ALLOCATE?

    // Add reliable flag when it is "reliable" channel
    if (channel == NetworkChannelType::Reliable || channel == NetworkChannelType::ReliableOrdered)
        flag |= ENET_PACKET_FLAG_RELIABLE;

    // Use unsequenced flag when the flag is unreliable. We have to sequence all other packets.
    // Note: ENet has no "unreliable ordered" channel type, so we have to use reliable for that.
    if (channel == NetworkChannelType::Unreliable)
        flag |= ENET_PACKET_FLAG_UNSEQUENCED;

    return static_cast<ENetPacketFlag>(flag);
}

// Unreliable - 0
// UnreliableOrdered - 1
// Reliable/ReliableOrdered - 2
#define ENET_CHANNEL_COUNT 3

enet_uint8 ChannelTypeToChannelIndex(const NetworkChannelType channel)
{
    switch (channel)
    {
    case NetworkChannelType::Unreliable:
        return 0;
    case NetworkChannelType::UnreliableOrdered:
        return 1;
    case NetworkChannelType::Reliable:
    case NetworkChannelType::ReliableOrdered:
        return 2;
    default:
        return 0;
    }
}

void SendPacketToPeer(ENetPeer* peer, const NetworkChannelType channelType, const NetworkMessage& message)
{
    // Covert our channel type to the internal ENet packet flags
    const ENetPacketFlag flag = ChannelTypeToPacketFlag(channelType);

    // This will copy the data into the packet when ENET_PACKET_FLAG_NO_ALLOCATE is not set.
    // Tho, we cannot use it, because we're releasing the message right after the send - and the packet might not
    // be sent, yet. To avoid data corruption, we're just using the copy method. We might fix that later, but I'll take
    // the smaller risk.
    ENetPacket* packet = enet_packet_create(message.Buffer, message.Position, flag);

    // And send it!
    enet_peer_send(peer, ChannelTypeToChannelIndex(channelType), packet);

    // TODO: To reduce latency, we can use `enet_host_flush` to flush all packets. Maybe some API, like NetworkManager::FlushQueues()?
}

ENetDriver::ENetDriver(const SpawnParams& params)
    : ScriptingObject(params)
{
}

bool ENetDriver::Initialize(NetworkPeer* host, const NetworkConfig& config)
{
    PROFILE_CPU();
    _networkHost = host;
    _config = config;
    _peerMap.Clear();

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
    PROFILE_CPU();
    if (_peer)
        enet_peer_disconnect_now(_peer, 0);
    enet_host_destroy(_host);

    enet_deinitialize();

    _peerMap.Clear();

    _peer = nullptr;
    _host = nullptr;

    LOG(Info, "ENet driver stopped!");
}

bool ENetDriver::Listen()
{
    PROFILE_CPU();
    ENetAddress address = { 0 };
    address.port = _config.Port;
    address.host = ENET_HOST_ANY;

    // Set host address if needed
    if (_config.Address != TEXT("any"))
        enet_address_set_host(&address, _config.Address.ToStringAnsi().GetText());

    // Create ENet host
    _host = enet_host_create(&address, _config.ConnectionsLimit, ENET_CHANNEL_COUNT, 0, 0);
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
    PROFILE_CPU();
    LOG(Info, "Connecting using ENet...");

    ENetAddress address = { 0 };
    address.port = _config.Port;
    enet_address_set_host(&address, _config.Address.ToStringAnsi().GetText());

    // Create ENet host
    _host = enet_host_create(nullptr, 1, ENET_CHANNEL_COUNT, 0, 0);
    if (_host == nullptr)
    {
        LOG(Error, "Failed to initialize ENet host!");
        return false;
    }

    // Create ENet peer/connect to the server
    _peer = enet_host_connect(_host, &address, ENET_CHANNEL_COUNT, 0);
    if (_peer == nullptr)
    {
        LOG(Error, "Failed to create ENet host!");
        enet_host_destroy(_host);
        return false;
    }

    return true;
}

void ENetDriver::Disconnect()
{
    PROFILE_CPU();
    if (_peer)
    {
        enet_peer_disconnect_now(_peer, 0);
        _peer = nullptr;
        LOG(Info, "Disconnected");
    }
}

void ENetDriver::Disconnect(const NetworkConnection& connection)
{
    PROFILE_CPU();
    const int connectionId = connection.ConnectionId;
    ENetPeer* peer;
    if (_peerMap.TryGet(connectionId, peer))
    {
        enet_peer_disconnect_now(peer, 0);
        _peerMap.Remove(connectionId);
    }
    else
    {
        LOG(Error, "Failed to kick connection({0}). ENetPeer not found!", connection.ConnectionId);
    }
}

bool ENetDriver::PopEvent(NetworkEvent& eventPtr)
{
    PROFILE_CPU();
    ASSERT(_host);
    ENetEvent event;
    const int result = enet_host_service(_host, &event, 0);
    if (result < 0)
        LOG(Error, "Failed to check ENet events!");
    if (result > 0)
    {
        // Copy sender data
        const uint32 connectionId = enet_peer_get_id(event.peer);
        eventPtr.Sender.ConnectionId = connectionId;

        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            eventPtr.EventType = NetworkEventType::Connected;
            if (IsServer())
                _peerMap.Add(connectionId, event.peer);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            eventPtr.EventType = NetworkEventType::Disconnected;
            if (IsServer())
                _peerMap.Remove(connectionId);
            break;
        case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
            eventPtr.EventType = NetworkEventType::Timeout;
            if (IsServer())
                _peerMap.Remove(connectionId);
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            eventPtr.EventType = NetworkEventType::Message;
            eventPtr.Message = _networkHost->CreateMessage();
            eventPtr.Message.BufferSize = event.packet->dataLength;
            Platform::MemoryCopy(eventPtr.Message.Buffer, event.packet->data, event.packet->dataLength);

            // Clean up the packet after we're done using it
            enet_packet_destroy(event.packet);
            break;
        default:
            break;
        }

        // Got event
        return true;
    }

    // No events
    return false;
}

void ENetDriver::SendMessage(const NetworkChannelType channelType, const NetworkMessage& message)
{
    PROFILE_CPU();
    ASSERT(!IsServer());
    SendPacketToPeer(_peer, channelType, message);
}

void ENetDriver::SendMessage(NetworkChannelType channelType, const NetworkMessage& message, NetworkConnection target)
{
    PROFILE_CPU();
    ASSERT(IsServer());
    ENetPeer* peer;
    if (_peerMap.TryGet(target.ConnectionId, peer) && peer && peer->state == ENET_PEER_STATE_CONNECTED)
    {
        SendPacketToPeer(peer, channelType, message);
    }
}

void ENetDriver::SendMessage(const NetworkChannelType channelType, const NetworkMessage& message, const Array<NetworkConnection, HeapAllocation>& targets)
{
    PROFILE_CPU();
    ASSERT(IsServer());
    ENetPeer* peer;
    for (NetworkConnection target : targets)
    {
        if (_peerMap.TryGet(target.ConnectionId, peer) && peer && peer->state == ENET_PEER_STATE_CONNECTED)
        {
            SendPacketToPeer(peer, channelType, message);
        }
    }
}

NetworkDriverStats ENetDriver::GetStats()
{
    NetworkDriverStats stats;
    if (_host)
    {
        // Get host stats
        stats.TotalDataSent = _host->totalSentData;
        stats.TotalDataReceived = _host->totalReceivedData;
        int32 peers = 0;
        for (ENetPeer* peer = _host->peers; peer < &_host->peers[_host->peerCount]; peer++)
        {
            if (peer->state != ENET_PEER_STATE_CONNECTED)
                continue;
            stats.RTT += (float)peer->roundTripTime;
            peers++;
        }
        if (peers > 0)
        {
            stats.RTT /= (float)peers;
        }
    }
    else
        stats = GetStats({});
    return stats;
}

NetworkDriverStats ENetDriver::GetStats(NetworkConnection target)
{
    NetworkDriverStats stats;
    ENetPeer* peer = _peer;
    if (!peer)
        _peerMap.TryGet(target.ConnectionId, peer);
    if (!peer && _host && _host->peerCount > 0)
        peer = _host->peers; // Get the first peer
    if (peer)
    {
        // Get peer stats
        stats.RTT = (float)peer->roundTripTime;
        stats.TotalDataSent = peer->totalDataSent;
        stats.TotalDataReceived = peer->totalDataReceived;
    }
    return stats;
}
