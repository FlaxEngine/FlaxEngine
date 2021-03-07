// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"
#include "Types.h"

API_CLASS(Static) class FLAXENGINE_API NetworkManager
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(NetworkManager);
public:
    
    API_FUNCTION() static bool Initialize(const NetworkConfig& config);
    API_FUNCTION() static void Shutdown();
    
    API_FUNCTION() static bool Listen();
    API_FUNCTION() static void Connect();
    API_FUNCTION() static void Disconnect();
    API_FUNCTION() static void Disconnect(const NetworkConnection& connection);

    API_FUNCTION() static bool PopEvent(NetworkEvent* event);

    API_FUNCTION() static NetworkMessage BeginSendMessage();
    API_FUNCTION() static void AbortSendMessage(const NetworkMessage& message);
    API_FUNCTION() static bool EndSendMessage(NetworkChannelType channelType, const NetworkMessage& message, Array<NetworkConnection, HeapAllocation> targets);

    // TODO: Stats API
    // TODO: Simulation API
    
private:

    static NetworkMessage CreateMessage();
    static void RecycleMessage(const NetworkMessage& message);
    
    static void CreateMessageBuffers();
    static void DisposeMessageBuffers();
    static uint8* GetMessageBuffer(uint32 messageId);
};
