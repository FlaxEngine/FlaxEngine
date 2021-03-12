// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "NetworkHost.h"

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

    // Setup messages
    CreateMessageBuffers();
    MessagePool.Clear();

    // Warmup message pool
    for (uint32 messageId = Config.MessagePoolSize; messageId > 0; messageId --)
        MessagePool.Push(messageId);

    // Setup network driver
    NetworkDriver = New<ENetDriver>();
    NetworkDriver->Initialize(Config);

    LOG(Info, "NetworkManager initialized using driver = {0}", static_cast<int>(Config.NetworkDriverType));
}

void NetworkHost::Shutdown()
{
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
