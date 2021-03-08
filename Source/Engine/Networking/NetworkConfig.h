// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

API_STRUCT() struct FLAXENGINE_API NetworkConfig
{
DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkConfig);
public:
    INetworkDriver* NetworkDriver = nullptr;

public:
    uint16 ConnectionsLimit = 32;
    uint16 Port = 7777;
    uint16 MessageSize = 1500; // MTU
    uint16 MessagePoolSize = 2048; // (RX and TX)

    // TODO: End point for server/client
};
