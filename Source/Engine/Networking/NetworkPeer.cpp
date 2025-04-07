// Copyright (c) Wojciech Figat. All rights reserved.

#include "NetworkPeer.h"
#include "NetworkEvent.h"
#include "Drivers/ENetDriver.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Platform/CPUInfo.h"
#include "Engine/Profiler/ProfilerCPU.h"

Array<NetworkPeer*> NetworkPeer::Peers;

namespace
{
    uint32 LastHostId = 0;
}

bool NetworkPeer::Initialize(const NetworkConfig& config)
{
    if (NetworkDriver)
        return true;

    Config = config;
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    if (Config.NetworkDriver == nullptr && Config.NetworkDriverType == NetworkDriverType::ENet)
        Config.NetworkDriver = New<ENetDriver>();
    PRAGMA_ENABLE_DEPRECATION_WARNINGS

    if (Config.NetworkDriver == nullptr)
    {
        LOG(Error, "Missing NetworkDriver");
        return true;
    }
    if (Config.ConnectionsLimit <= 0)
    {
        LOG(Error, "Invalid ConnectionsLimit");
        return true;
    }
    if (Config.MessageSize <= 32) // TODO: Adjust this, not sure what the lowest limit should be.
    {
        LOG(Error, "Invalid MessageSize");
        return true;
    }
    if (Config.MessagePoolSize <= 128)
    {
        LOG(Error, "Invalid MessagePoolSize");
        return true;
    }
    NetworkDriver = ToInterface<INetworkDriver>(Config.NetworkDriver);
    if (!NetworkDriver)
    {
        LOG(Error, "NetworkDriver doesn't implement INetworkDriver interface");
        return true;
    }

    // TODO: Dynamic message pool allocation
    // Setup messages
    CreateMessageBuffers();
    MessagePool.Clear();

    // Warmup message pool
    for (uint32 messageId = Config.MessagePoolSize; messageId > 0; messageId --)
        MessagePool.Push(messageId);

    // Setup network driver
    if (NetworkDriver->Initialize(this, Config))
    {
        LOG(Error, "Failed to initialize NetworkDriver");
        return true;
    }

    LOG(Info, "NetworkPeer initialized using driver = {0}", NetworkDriver->DriverName());
    return false;
}

void NetworkPeer::Shutdown()
{
    NetworkDriver->Dispose();
    Delete(Config.NetworkDriver);
    DisposeMessageBuffers();

    Config.NetworkDriver = nullptr;
    NetworkDriver = nullptr;
}

void NetworkPeer::CreateMessageBuffers()
{
    ASSERT(MessageBuffer == nullptr);

    const uint32 pageSize = Platform::GetCPUInfo().PageSize;

    // Calculate total size in bytes
    const uint64 totalSize = static_cast<uint64>(Config.MessagePoolSize) * Config.MessageSize;

    // Calculate the amount of pages that we need
    const uint32 numPages = totalSize > pageSize ? Math::CeilToInt(totalSize / static_cast<float>(pageSize)) : 1;

    MessageBuffer = static_cast<uint8*>(Platform::AllocatePages(numPages, pageSize));
    Platform::MemorySet(MessageBuffer, 0, numPages * pageSize);
}

void NetworkPeer::DisposeMessageBuffers()
{
    ASSERT(MessageBuffer != nullptr);

    Platform::FreePages(MessageBuffer);
    MessageBuffer = nullptr;
}

bool NetworkPeer::Listen()
{
    LOG(Info, "Starting to listen on address = {0}:{1}", Config.Address, Config.Port);
    return NetworkDriver->Listen();
}

bool NetworkPeer::Connect()
{
    LOG(Info, "Connecting to {0}:{1}...", Config.Address, Config.Port);
    return NetworkDriver->Connect();
}

void NetworkPeer::Disconnect()
{
    LOG(Info, "Disconnecting...");
    NetworkDriver->Disconnect();
}

void NetworkPeer::Disconnect(const NetworkConnection& connection)
{
    LOG(Info, "Disconnecting connection with id = {0}...", connection.ConnectionId);
    NetworkDriver->Disconnect(connection);
}

bool NetworkPeer::PopEvent(NetworkEvent& eventRef)
{
    PROFILE_CPU();
    return NetworkDriver->PopEvent(eventRef);
}

NetworkMessage NetworkPeer::CreateMessage()
{
    const uint32 messageId = MessagePool.Pop();
    uint8* messageBuffer = GetMessageBuffer(messageId);
    return NetworkMessage(messageBuffer, messageId, Config.MessageSize, 0, 0);
}

void NetworkPeer::RecycleMessage(const NetworkMessage& message)
{
    ASSERT(message.IsValid());
#ifdef BUILD_DEBUG
    ASSERT(MessagePool.Contains(message.MessageId) == false);
#endif

    // Return the message id
    MessagePool.Push(message.MessageId);
}

NetworkMessage NetworkPeer::BeginSendMessage()
{
    return CreateMessage();
}

void NetworkPeer::AbortSendMessage(const NetworkMessage& message)
{
    ASSERT(message.IsValid());
    RecycleMessage(message);
}

bool NetworkPeer::EndSendMessage(const NetworkChannelType channelType, const NetworkMessage& message)
{
    ASSERT(message.IsValid());

    NetworkDriver->SendMessage(channelType, message);

    RecycleMessage(message);
    return false;
}

bool NetworkPeer::EndSendMessage(const NetworkChannelType channelType, const NetworkMessage& message, const NetworkConnection& target)
{
    ASSERT(message.IsValid());

    NetworkDriver->SendMessage(channelType, message, target);

    RecycleMessage(message);
    return false;
}

bool NetworkPeer::EndSendMessage(const NetworkChannelType channelType, const NetworkMessage& message, const Array<NetworkConnection>& targets)
{
    ASSERT(message.IsValid());

    NetworkDriver->SendMessage(channelType, message, targets);

    RecycleMessage(message);
    return false;
}

NetworkPeer* NetworkPeer::CreatePeer(const NetworkConfig& config)
{
    // Validate the address for listen/connect
    if (config.Address != TEXT("any"))
    {
        NetworkEndPoint endPoint;
        if (Network::CreateEndPoint(config.Address, String::Empty, NetworkIPVersion::IPv4, endPoint, false))
        {
            LOG(Error, "Invalid end point.");
            return nullptr;
        }
    }

    // Alloc new host
    NetworkPeer* host = New<NetworkPeer>();
    host->HostId = LastHostId++;

    // Initialize the host
    if (host->Initialize(config))
    {
        Delete(host);
        return nullptr;
    }

    Peers.Add(host);
    return host;
}

void NetworkPeer::ShutdownPeer(NetworkPeer* peer)
{
    if (!peer)
        return;
    CHECK(peer->IsValid());
    peer->Shutdown();
    peer->HostId = -1;
    Peers.Remove(peer);

    Delete(peer);
}
