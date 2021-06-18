// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "NetworkHost.h"
#include "NetworkEvent.h"


#include "Drivers/ENetDriver.h"

#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Platform/CPUInfo.h"

void NetworkHost::Initialize(const NetworkConfig& config)
{
    Config = config;

    ASSERT(NetworkDriver == nullptr);
    ASSERT(Config.NetworkDriverType != NetworkTransportType::Undefined);
    ASSERT(Config.ConnectionsLimit > 0);
    ASSERT(Config.MessageSize > 32); // TODO: Adjust this, not sure what the lowest limit should be.
    ASSERT(Config.MessagePoolSize > 128);

    // TODO: Dynamic message pool allocation
    // Setup messages
    CreateMessageBuffers();
    MessagePool.Clear();

    // Warmup message pool
    for (uint32 messageId = Config.MessagePoolSize; messageId > 0; messageId --)
        MessagePool.Push(messageId);

    // Setup network driver
    NetworkDriver = New<ENetDriver>();
    NetworkDriver->Initialize(this, Config);

    LOG(Info, "NetworkManager initialized using driver = {0}", static_cast<int>(Config.NetworkDriverType));
}

void NetworkHost::Shutdown()
{
    NetworkDriver->Dispose();
    Delete(NetworkDriver);
    DisposeMessageBuffers();

    NetworkDriver = nullptr;
}

void NetworkHost::CreateMessageBuffers()
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

void NetworkHost::DisposeMessageBuffers()
{
    ASSERT(MessageBuffer != nullptr);
        
    Platform::FreePages(MessageBuffer);
    MessageBuffer = nullptr;
}

bool NetworkHost::Listen()
{
    LOG(Info, "NetworkManager starting to listen on address = {0}:{1}", Config.Address, Config.Port);
    return NetworkDriver->Listen();
}

bool NetworkHost::Connect()
{
    LOG(Info, "Connecting to {0}:{1}...", Config.Address, Config.Port);
    return NetworkDriver->Connect();
}

void NetworkHost::Disconnect()
{
    LOG(Info, "Disconnecting...");
    NetworkDriver->Disconnect();
}

void NetworkHost::Disconnect(const NetworkConnection& connection)
{
    LOG(Info, "Disconnecting connection with id = {0}...", connection.ConnectionId);
    NetworkDriver->Disconnect(connection);
}

bool NetworkHost::PopEvent(NetworkEvent& eventRef)
{
    // Set host id of the event
    eventRef.HostId = HostId;
    return NetworkDriver->PopEvent(&eventRef);
}

NetworkMessage NetworkHost::CreateMessage()
{
    const uint32 messageId = MessagePool.Pop();
    uint8* messageBuffer = GetMessageBuffer(messageId);

    return NetworkMessage(messageBuffer, messageId, Config.MessageSize, 0, 0);
}

void NetworkHost::RecycleMessage(const NetworkMessage& message)
{
    ASSERT(message.IsValid());
#ifdef BUILD_DEBUG
    ASSERT(MessagePool.Contains(message.MessageId) == false);
#endif
    
    // Return the message id
    MessagePool.Push(message.MessageId);
}

NetworkMessage NetworkHost::BeginSendMessage()
{
    return CreateMessage();
}

void NetworkHost::AbortSendMessage(const NetworkMessage& message)
{
    ASSERT(message.IsValid());
    RecycleMessage(message);
}

bool NetworkHost::EndSendMessage(const NetworkChannelType channelType, const NetworkMessage& message)
{
    ASSERT(message.IsValid());
    
    NetworkDriver->SendMessage(channelType, message);

    RecycleMessage(message);
    return false;
}

bool NetworkHost::EndSendMessage(const NetworkChannelType channelType, const NetworkMessage& message, const NetworkConnection& target)
{
    ASSERT(message.IsValid());
    
    NetworkDriver->SendMessage(channelType, message, target);

    RecycleMessage(message);
    return false;
}

bool NetworkHost::EndSendMessage(const NetworkChannelType channelType, const NetworkMessage& message, Array<NetworkConnection> targets)
{
    ASSERT(message.IsValid());
    
    NetworkDriver->SendMessage(channelType, message, targets);

    RecycleMessage(message);
    return false;
}
