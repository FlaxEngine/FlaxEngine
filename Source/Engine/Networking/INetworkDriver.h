// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

API_INTERFACE() class FLAXENGINE_API INetworkDriver
{
public:
    virtual void Initialize(const NetworkConfig& config) = 0;
    virtual void Dispose() = 0;
    
    virtual bool Listen() = 0;
    virtual void Connect() = 0;
    virtual void Disconnect() = 0;
    virtual void Disconnect(const NetworkConnection& connection) = 0;

    virtual bool PopEvent(NetworkEvent* event);
    
    virtual void SendMessage(NetworkChannelType channelType, const NetworkMessage& message, Array<NetworkConnection, HeapAllocation> targets) = 0;

    // TODO: Stats API
    // TODO: Simulation API
    
};
