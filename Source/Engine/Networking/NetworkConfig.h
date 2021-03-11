// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Network.h"

API_ENUM(Namespace="FlaxEngine.Networking") enum class NetworkTransportType
{
    Undefined = 0,
    
    ENet
};

API_STRUCT(Namespace="FlaxEngine.Networking") struct FLAXENGINE_API NetworkConfig
{
DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkConfig);
public:
    API_FIELD()
    NetworkTransportType NetworkDriverType = NetworkTransportType::ENet;
    // TODO: Expose INetworkDriver as a ref not enum, when C++/C# interfaces are done.

public:
    API_FIELD()
    uint16 ConnectionsLimit = 32;
    
    API_FIELD()
    uint16 Port = 7777;
    
    // API_FIELD()
    // NetworkEndPoint EndPoint = {}; // TODO: Use NetSockets C# API when done
    
    API_FIELD()
    uint16 MessageSize = 1500; // MTU
    
    API_FIELD()
    uint16 MessagePoolSize = 2048; // (RX and TX)
};
