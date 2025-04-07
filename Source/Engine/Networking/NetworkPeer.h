// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "NetworkConfig.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Scripting/ScriptingObjectReference.h"

/// <summary>
/// Low-level network peer class. Provides server-client communication functions, message processing and sending.
/// </summary>
API_CLASS(sealed, NoSpawn, Namespace="FlaxEngine.Networking") class FLAXENGINE_API NetworkPeer final : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(NetworkPeer, ScriptingObject);

    // List with all active peers.
    API_FIELD(ReadOnly) static Array<NetworkPeer*> Peers;

public:
    int HostId = -1;
    NetworkConfig Config;

    uint8* MessageBuffer = nullptr;
    Array<uint32, HeapAllocation> MessagePool;

public:
    /// <summary>
    /// Low-level network transport driver used by this peer.
    /// </summary>
    API_FIELD() INetworkDriver* NetworkDriver = nullptr;

public:
    /// <summary>
    /// Starts listening for incoming connections.
    /// Once this is called, this peer becomes a server.
    /// </summary>
    /// <returns>True when succeeded.</returns>
    API_FUNCTION() bool Listen();

    /// <summary>
    /// Starts connection handshake with the end point specified in the <seealso cref="NetworkConfig"/> structure.
    /// Once this is called, this peer becomes a client.
    /// </summary>
    /// <returns>True when succeeded.</returns>
    API_FUNCTION() bool Connect();

    /// <summary>
    /// Disconnects from the server.
    /// </summary>
    /// <remarks>Can be used only by the client!</remarks>
    API_FUNCTION() void Disconnect();

    /// <summary>
    /// Disconnects given connection from the server.
    /// </summary>
    /// <remarks>Can be used only by the server!</remarks>
    API_FUNCTION() void Disconnect(const NetworkConnection& connection);

    /// <summary>
    /// Tries to pop an network event from the queue.
    /// </summary>
    /// <param name="eventRef">The reference to event structure.</param>
    /// <returns>True when succeeded and the event can be processed.</returns>
    /// <remarks>If this returns message event, make sure to recycle the message using <see cref="RecycleMessage"/> function after processing it!</remarks>
    API_FUNCTION() bool PopEvent(API_PARAM(Out) NetworkEvent& eventRef);

    /// <summary>
    /// Acquires new message from the pool.
    /// Cannot acquire more messages than the limit specified in the <seealso cref="NetworkConfig"/> structure.
    /// </summary>
    /// <returns>The acquired message.</returns>
    /// <remarks>Make sure to recycle the message to this peer once it is no longer needed!</remarks>
    API_FUNCTION() NetworkMessage CreateMessage();

    /// <summary>
    /// Returns given message to the pool.
    /// </summary>
    /// <remarks>Make sure that this message belongs to the peer and has not been recycled already (debug build checks for this)!</remarks>
    API_FUNCTION() void RecycleMessage(const NetworkMessage& message);

    /// <summary>
    /// Acquires new message from the pool and setups it for sending.
    /// </summary>
    /// <returns>The acquired message.</returns>
    API_FUNCTION() NetworkMessage BeginSendMessage();

    /// <summary>
    /// Aborts given message send. This effectively deinitializes the message and returns it to the pool. 
    /// </summary>
    /// <param name="message">The message.</param>
    API_FUNCTION() void AbortSendMessage(const NetworkMessage& message);

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
    API_FUNCTION() bool EndSendMessage(NetworkChannelType channelType, const NetworkMessage& message);

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
    API_FUNCTION() bool EndSendMessage(NetworkChannelType channelType, const NetworkMessage& message, const NetworkConnection& target);

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
    API_FUNCTION() bool EndSendMessage(NetworkChannelType channelType, const NetworkMessage& message, const Array<NetworkConnection, HeapAllocation>& targets);

    /// <summary>
    /// Creates new peer using given configuration.
    /// </summary>
    /// <param name="config">The configuration to create and setup new peer.</param>
    /// <returns>The peer.</returns>
    /// <remarks>Peer should be destroyed using <see cref="ShutdownPeer"/> once it is no longer in use. Returns null if failed to create a peer (eg. config is invalid).</remarks>
    API_FUNCTION() static NetworkPeer* CreatePeer(const NetworkConfig& config);

    /// <summary>
    /// Shutdowns and destroys given peer.
    /// </summary>
    /// <param name="peer">The peer to destroy.</param>
    API_FUNCTION() static void ShutdownPeer(NetworkPeer* peer);

public:
    bool IsValid() const
    {
        return NetworkDriver != nullptr && HostId >= 0;
    }

    uint8* GetMessageBuffer(const uint32 messageId) const
    {
        // Calculate and return the buffer slice using previously calculated slice.
        return MessageBuffer + Config.MessageSize * messageId;
    }

public:
    FORCE_INLINE bool operator==(const NetworkPeer& other) const
    {
        return HostId == other.HostId;
    }

    FORCE_INLINE bool operator!=(const NetworkPeer& other) const
    {
        return HostId != other.HostId;
    }

private:
    bool Initialize(const NetworkConfig& config);
    void Shutdown();
    void CreateMessageBuffers();
    void DisposeMessageBuffers();
};
