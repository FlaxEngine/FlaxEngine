// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "NetworkConfig.h"

#include "Engine/Core/Collections/Array.h"

struct FLAXENGINE_API NetworkHost
{
public:
    NetworkConfig Config;
    INetworkDriver* NetworkDriver = nullptr;

    uint8* MessageBuffer = nullptr;
    Array<uint32, HeapAllocation> MessagePool;

public:
    void Initialize(const NetworkConfig& config);
    void Shutdown();
    void CreateMessageBuffers();
    void DisposeMessageBuffers();
    
    bool IsValid() const
    {
        return NetworkDriver != nullptr;
    }

    uint8* GetMessageBuffer(const uint32 messageId) const
    {
        // Calculate and return the buffer slice using previously calculated slice.
        return MessageBuffer + Config.MessageSize * messageId;
    }
};

