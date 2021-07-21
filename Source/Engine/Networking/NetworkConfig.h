// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Network.h"

/// <summary>
/// Network driver implementations enum.
/// </summary>
API_ENUM(Namespace="FlaxEngine.Networking") enum class NetworkDriverType
{
    /// <summary>
    /// Invalid network driver implementation.
    /// </summary>
    Undefined = 0,

    /// <summary>
    /// ENet library based network driver implementation.
    /// </summary>
    ENet
};

/// <summary>
/// Low-level network configuration structure. Provides settings for the network driver and all internal components.
/// </summary>
API_STRUCT(Namespace="FlaxEngine.Networking") struct FLAXENGINE_API NetworkConfig
{
DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkConfig);
public:
    /// <summary>
    /// The network driver that will be used to create the peer.
    /// To allow two peers to connect, they must use the same host.
    /// </summary>
    API_FIELD()
    NetworkDriverType NetworkDriverType = NetworkDriverType::ENet;
    // TODO: Expose INetworkDriver as a ref not enum, when C++/C# interfaces are done.

public:
    /// <summary>
    /// The upper limit on how many peers can join when we're listening.
    /// </summary>
    API_FIELD()
    uint16 ConnectionsLimit = 32;

    /// <summary>
    /// Address used to connect to or listen at.
    /// </summary>
    /// <remarks>Set it to "any" when you want to listen at all available addresses.</remarks>
    /// <remarks>Only IPv4 is supported.</remarks>
    API_FIELD()
    String Address = String("127.0.0.1");

    /// <summary>
    /// The port to connect to or listen at.
    /// </summary>
    API_FIELD()
    uint16 Port = 7777;
    
    /// <summary>
    /// The size of a message buffer in bytes.
    /// Should be lower than the MTU (maximal transmission unit) - typically 1500 bytes.
    /// </summary>
    API_FIELD()
    uint16 MessageSize = 1500;
    
    /// <summary>
    /// The amount of pooled messages that can be used at once (receiving and sending!).
    /// </summary>
    /// <remarks>
    /// Creating more messages than this limit will result in a crash!
    /// This should be tweaked manually to fit the needs (adjusting this value will increase/decrease memory usage)!
    /// </remarks>
    API_FIELD()
    uint16 MessagePoolSize = 2048;
};
