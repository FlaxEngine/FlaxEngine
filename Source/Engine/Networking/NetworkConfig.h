// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

API_STRUCT(Namespace="FlaxEngine.Networking") struct FLAXENGINE_API NetworkConfig
{
DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkConfig);
public:
    API_FIELD()
    INetworkDriver* NetworkDriver = nullptr; // TODO: Pass by ref not pointer (?)

public:
    API_FIELD()
    uint16 ConnectionsLimit = 32;
    
    API_FIELD()
    uint16 Port = 7777;
    
    API_FIELD()
    uint16 MessageSize = 1500; // MTU
    
    API_FIELD()
    uint16 MessagePoolSize = 2048; // (RX and TX)

    // TODO: End point for server/client
};
