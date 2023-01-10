// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Scripting/ScriptingType.h"

/// <summary>
/// Basic interface for the low-level network transport/driver.
/// </summary>
API_INTERFACE(Namespace="FlaxEngine.Networking") class FLAXENGINE_API INetworkDriver
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(INetworkDriver);
public:
    /// <summary>
    /// Finalizes an instance of the <see cref="INetworkDriver"/> class.
    /// </summary>
    virtual ~INetworkDriver() = default;

    /// <summary>
    /// Return name of this network driver implementation.
    /// </summary>
    API_FUNCTION() virtual String DriverName()
    {
        return String("Unknown");
    }

    /// <summary>
    /// Initializes the instance of this network driver using given configuration.
    /// </summary>
    /// <param name="host">The peer that this driver has been assigned to.</param>
    /// <param name="config">The network config to use to configure this driver.</param>
    /// <returns>True if failed to initialize network driver, false otherwise.</returns>
    API_FUNCTION() virtual bool Initialize(NetworkPeer* host, const NetworkConfig& config) = 0;

    /// <summary>
    /// Disposes this driver making it no longer usable.
    /// Reserved for resource deallocation etc.
    /// </summary>
    API_FUNCTION() virtual void Dispose() = 0;

    /// <summary>
    /// Starts listening for incoming connections.
    /// Once this is called, this driver becomes a server.
    /// </summary>
    /// <returns>True when succeeded.</returns>
    API_FUNCTION() virtual bool Listen() = 0;

    /// <summary>
    /// Starts connection handshake with the end point specified in the <seealso cref="NetworkConfig"/> structure.
    /// Once this is called, this driver becomes a client.
    /// </summary>
    /// <returns>True when succeeded.</returns>
    API_FUNCTION() virtual bool Connect() = 0;

    /// <summary>
    /// Disconnects from the server.
    /// </summary>
    /// <remarks>Can be used only by the client!</remarks>
    API_FUNCTION() virtual void Disconnect() = 0;

    /// <summary>
    /// Disconnects given connection from the server.
    /// </summary>
    /// <remarks>Can be used only by the server!</remarks>
    API_FUNCTION() virtual void Disconnect(const NetworkConnection& connection) = 0;

    /// <summary>
    /// Tries to pop an network event from the queue.
    /// </summary>
    /// <param name="eventPtr">The pointer to event structure.</param>
    /// <returns>True when succeeded and the event can be processed.</returns>
    API_FUNCTION() virtual bool PopEvent(NetworkEvent* eventPtr) = 0;

    /// <summary>
    /// Sends given message over specified channel to the server.
    /// </summary>
    /// <param name="channelType">The channel to send the message over.</param>
    /// <param name="message">The message.</param>
    /// <remarks>Can be used only by the client!</remarks>
    /// <remarks>
    /// Do not recycle the message after calling this.
    /// This function automatically recycles the message.
    /// </remarks>
    API_FUNCTION() virtual void SendMessage(NetworkChannelType channelType, const NetworkMessage& message) = 0;

    /// <summary>
    /// Sends given message over specified channel to the given client connection (target).
    /// </summary>
    /// <param name="channelType">The channel to send the message over.</param>
    /// <param name="message">The message.</param>
    /// <param name="target">The client connection to send the message to.</param>
    /// <remarks>Can be used only by the server!</remarks>
    /// <remarks>
    /// Do not recycle the message after calling this.
    /// This function automatically recycles the message.
    /// </remarks>
    API_FUNCTION() virtual void SendMessage(NetworkChannelType channelType, const NetworkMessage& message, NetworkConnection target) = 0;

    /// <summary>
    /// Sends given message over specified channel to the given client connection (target).
    /// </summary>
    /// <param name="channelType">The channel to send the message over.</param>
    /// <param name="message">The message.</param>
    /// <param name="targets">The connections list to send the message to.</param>
    /// <remarks>Can be used only by the server!</remarks>
    /// <remarks>
    /// Do not recycle the message after calling this.
    /// This function automatically recycles the message.
    /// </remarks>
    API_FUNCTION() virtual void SendMessage(NetworkChannelType channelType, const NetworkMessage& message, const Array<NetworkConnection, HeapAllocation>& targets) = 0;

    // TODO: Stats API
    // TODO: Simulation API
};
