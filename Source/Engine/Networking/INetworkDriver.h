// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

API_INTERFACE() class FLAXENGINE_API INetworkDriver
{
public:
    virtual void Initialize(const NetworkConfig& config) = 0;
    virtual void Dispose() = 0;
    
    virtual void Listen() = 0;
    virtual void SendMessage(const NetworkMessage* message, void* targets) = 0; // TODO
};
