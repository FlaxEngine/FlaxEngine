// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"

API_ENUM(Namespace="FlaxEngine.Networking") enum class NetworkEventType
{
    Undefined = 0,

    ConnectionRequest,
    Connected,
    Disconnected,
    Message,
    Error
};

API_STRUCT(Namespace="FlaxEngine.Networking") struct FLAXENGINE_API NetworkEvent
{
DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkEvent);
public:
    API_FIELD();
    NetworkEventType EventType;
    
    // TODO
};
