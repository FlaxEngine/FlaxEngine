// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "NetworkMessage.h"
#include "Engine/Scripting/ScriptingType.h"

API_ENUM(Namespace="FlaxEngine.Networking") enum class NetworkEventType
{
    Undefined = 0,

    Connected,
    Disconnected,
    Message
};

API_STRUCT(Namespace="FlaxEngine.Networking") struct FLAXENGINE_API NetworkEvent
{
DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkEvent);
public:
    API_FIELD();
    NetworkEventType EventType;
    
    API_FIELD();
    NetworkMessage Message;

    API_FIELD();
    NetworkConnection Sender;
};
