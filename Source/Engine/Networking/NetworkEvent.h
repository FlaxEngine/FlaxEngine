// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "NetworkConnection.h"
#include "NetworkMessage.h"
#include "Engine/Scripting/ScriptingType.h"

/// <summary>
/// Network event type enum contains all possible events that can be returned by PopEvent function.
/// </summary>
API_ENUM(Namespace="FlaxEngine.Networking") enum class NetworkEventType
{
    /// <summary>
    /// Invalid network event type.
    /// </summary>
    Undefined = 0,

    /// <summary>
    /// Event "connected" - client connected to our server or we've connected to the server.
    /// </summary>
    Connected,

    /// <summary>
    /// Event "disconnected" - client disconnected from our server or we've been kicked from the server.
    /// </summary>
    Disconnected,

    /// <summary>
    /// Event "disconnected" - client got a timeout from our server or we've list the connection to the server.
    /// </summary>
    Timeout,

    /// <summary>
    /// Event "message" - message received from some client or the server.
    /// </summary>
    Message
};

/// <summary>
/// Network event structure that wraps all data needed to identify and process it.
/// </summary>
API_STRUCT(Namespace="FlaxEngine.Networking") struct FLAXENGINE_API NetworkEvent
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkEvent);

public:
    /// <summary>
    /// The type of the received event.
    /// </summary>
    API_FIELD() NetworkEventType EventType;

    /// <summary>
    /// The message when this event is an "message" event - not valid in any other cases.
    /// If this is an message-event, make sure to return the message using RecycleMessage function of the peer after processing it!
    /// </summary>
    API_FIELD() NetworkMessage Message;

    /// <summary>
    /// The connected of the client that has sent message, connected, disconnected or got a timeout.
    /// </summary>
    /// <remarks>Only valid when event has been received on server-peer.</remarks>
    API_FIELD() NetworkConnection Sender;
};

template<>
struct TIsPODType<NetworkEvent>
{
    enum { Value = true };
};
