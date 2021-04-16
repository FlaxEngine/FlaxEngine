// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"

API_INTERFACE(Namespace="FlaxEngine.Networking") class FLAXENGINE_API INetworkDriver
{
DECLARE_SCRIPTING_TYPE_MINIMAL(INetworkDriver);
public:
    virtual void Initialize(const NetworkConfig& config) = 0;
    virtual void Dispose() = 0;
    
    virtual bool Listen() = 0;
    virtual bool Connect() = 0;
    virtual void Disconnect() = 0;
    virtual void Disconnect(const NetworkConnection& connection) = 0;

    virtual bool PopEvent(NetworkEvent* eventPtr) = 0;
    
    virtual void SendMessage(NetworkChannelType channelType, const NetworkMessage& message) = 0;
    virtual void SendMessage(NetworkChannelType channelType, const NetworkMessage& message, NetworkConnection target) = 0;
    virtual void SendMessage(NetworkChannelType channelType, const NetworkMessage& message, Array<NetworkConnection, HeapAllocation> targets) = 0;

    // TODO: Stats API
    // TODO: Simulation API
    
};
