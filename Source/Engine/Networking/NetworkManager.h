// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"
#include "Types.h"

API_CLASS(Namespace="FlaxEngine.Networking", Static) class FLAXENGINE_API NetworkManager
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(NetworkManager);
public:
    
    API_FUNCTION() static int Initialize(const NetworkConfig& config);
    API_FUNCTION() static void Shutdown(int hostId);
    
    API_FUNCTION() static bool Listen(int hostId);
    API_FUNCTION() static bool Connect(int hostId);
    API_FUNCTION() static void Disconnect(int hostId);
    API_FUNCTION() static void Disconnect(int hostId, const NetworkConnection& connection);

    API_FUNCTION() static bool PopEvent(int hostId, API_PARAM(out) NetworkEvent& eventPtr);
    
    API_FUNCTION() static NetworkMessage CreateMessage(int hostId);
    API_FUNCTION() static void RecycleMessage(int hostId, const NetworkMessage& message);

    API_FUNCTION() static NetworkMessage BeginSendMessage(int hostId);
    API_FUNCTION() static void AbortSendMessage(int hostId, const NetworkMessage& message);
    API_FUNCTION() static bool EndSendMessage(int hostId, NetworkChannelType channelType, const NetworkMessage& message);
    API_FUNCTION() static bool EndSendMessage(int hostId, NetworkChannelType channelType, const NetworkMessage& message, Array<NetworkConnection, HeapAllocation> targets);

    // TODO: Stats API
    // TODO: Simulation API
    // TODO: Optimize 'targets' in EndSendMessage
    
};
