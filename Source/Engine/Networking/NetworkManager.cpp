// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "NetworkManager.h"

#include "NetworkMessage.h"
#include "NetworkConfig.h"
#include "INetworkDriver.h"

#include "Engine/Core/Core.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Platform/CPUInfo.h"

namespace
{
    NetworkConfig Config;
    INetworkDriver* NetworkDriver = nullptr;

    uint8* MessageBuffer = nullptr;
    Array<uint32, HeapAllocation> MessagePool;
}

bool NetworkManager::Initialize(const NetworkConfig& config)
{
    Config = config;
    
    ASSERT(NetworkDriver == nullptr);
    ASSERT(Config.NetworkDriver);
    ASSERT(Config.ConnectionsLimit > 0);
    ASSERT(Config.MessageSize > 32); // TODO: Adjust this, not sure what the lowest limit should be.
    ASSERT(Config.MessagePoolSize > 128);

    // Setup messages
    CreateMessageBuffers();
    MessagePool.Clear();
    MessagePool.Resize(Config.MessagePoolSize);

    // Warmup message pool
    for(uint32 messageId = Config.MessagePoolSize; messageId > 0; messageId --)
        MessagePool.Push(messageId);

    // Setup network driver
    NetworkDriver = Config.NetworkDriver;
    NetworkDriver->Initialize(Config);
    
    return false;
}

void NetworkManager::Shutdown()
{
    SAFE_DISPOSE(NetworkDriver);
    DisposeMessageBuffers();
}

bool NetworkManager::Listen()
{
    // TODO
    return false;
}

void NetworkManager::Connect()
{
    // TODO
}

void NetworkManager::Disconnect()
{
    // TODO
}

NetworkMessage NetworkManager::BeginSendMessage()
{
    return CreateMessage();
}

void NetworkManager::AbortSendMessage(const NetworkMessage& message)
{
    ASSERT(message.IsValid());
    RecycleMessage(message);
}

bool NetworkManager::EndSendMessage(NetworkChannelType channelType, const NetworkMessage& message, Array<NetworkConnection> targets)
{
    ASSERT(message.IsValid());
    
    // TODO: Send message

    RecycleMessage(message);
    return false;
}

NetworkMessage NetworkManager::CreateMessage()
{
    ASSERT(MessagePool.Count() > 0);

    const uint32 messageId = MessagePool.Pop();
    uint8* messageBuffer = GetMessageBuffer(messageId);

    return NetworkMessage(messageBuffer, messageId, Config.MessageSize, 0, 0);
}

void NetworkManager::RecycleMessage(const NetworkMessage& message)
{
    ASSERT(message.IsValid());
#ifdef BUILD_DEBUG
    ASSERT(MessagePool.Contains(message.MessageId) == false);
#endif
    
    // Return the message id
    MessagePool.Push(message.MessageId);
}

void NetworkManager::CreateMessageBuffers()
{
    ASSERT(MessageBuffer == nullptr);
    
    const uint32 pageSize = Platform::GetCPUInfo().PageSize;

    // Calculate total size in bytes
    const uint64 totalSize = static_cast<uint64>(Config.MessagePoolSize) * Config.MessageSize;

    // Calculate the amount of pages that we need
    const uint32 numPages = totalSize > pageSize ? Math::CeilToInt(totalSize / static_cast<double>(pageSize)) : 1;

    MessageBuffer = static_cast<uint8*>(Platform::AllocatePages(numPages, pageSize));
    Platform::MemorySet(MessageBuffer, 0, numPages * pageSize);
}

void NetworkManager::DisposeMessageBuffers()
{
    ASSERT(MessageBuffer != nullptr);
    
    Platform::FreePages(MessageBuffer);
    MessageBuffer = nullptr;
}

uint8* NetworkManager::GetMessageBuffer(const uint32 messageId)
{
    // Calculate and return the buffer slice using previously calculated slice.
    return MessageBuffer + Config.MessageSize * messageId;
}
