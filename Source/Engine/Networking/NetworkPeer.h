// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Types.h"
#include "NetworkConfig.h"

#include "Engine/Core/Collections/Array.h"

API_CLASS(sealed, NoSpawn, Namespace="FlaxEngine.Networking") class FLAXENGINE_API NetworkPeer final : public PersistentScriptingObject
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(NetworkHost);
    friend class NetworkManager;
public:
    int HostId = -1;
    NetworkConfig Config;
    INetworkDriver* NetworkDriver = nullptr;

    uint8* MessageBuffer = nullptr;
    Array<uint32, HeapAllocation> MessagePool;

public:
    NetworkPeer() : PersistentScriptingObject(SpawnParams(Guid::New(), TypeInitializer))
    {
    }
    
private:
    void Initialize(const NetworkConfig& config);
    void Shutdown();
    
private:
    void CreateMessageBuffers();
    void DisposeMessageBuffers();

public:
    API_FUNCTION()
    bool Listen();
    
    API_FUNCTION()
    bool Connect();
    
    API_FUNCTION()
    void Disconnect();
    
    API_FUNCTION()
    void Disconnect(const NetworkConnection& connection);
    
    API_FUNCTION()
    bool PopEvent(API_PARAM(out) NetworkEvent& eventRef);

    API_FUNCTION()
    NetworkMessage CreateMessage();
    
    API_FUNCTION()
    void RecycleMessage(const NetworkMessage& message);

    API_FUNCTION()
    NetworkMessage BeginSendMessage();
    
    API_FUNCTION()
    void AbortSendMessage(const NetworkMessage& message);
    
    API_FUNCTION()
    bool EndSendMessage(NetworkChannelType channelType, const NetworkMessage& message);
    
    API_FUNCTION()
    bool EndSendMessage(NetworkChannelType channelType, const NetworkMessage& message, const NetworkConnection& target);
    
    API_FUNCTION()
    bool EndSendMessage(NetworkChannelType channelType, const NetworkMessage& message, Array<NetworkConnection, HeapAllocation> targets);
    
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
};

