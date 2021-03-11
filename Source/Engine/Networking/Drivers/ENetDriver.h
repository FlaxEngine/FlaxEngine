// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Networking/Types.h"
#include "Engine/Networking/INetworkDriver.h"

#include "Engine/Networking/NetworkConnection.h"
#include "Engine/Scripting/ScriptingType.h"

API_CLASS(Namespace="FlaxEngine.Networking", Sealed) class FLAXENGINE_API ENetDriver : public INetworkDriver
{
DECLARE_SCRIPTING_TYPE_MINIMAL(ENetDriver);
public:
    void Initialize(const NetworkConfig& config) override;
    void Dispose() override;
    
    bool Listen() override;
    void Connect() override;
    void Disconnect() override;
    void Disconnect(const NetworkConnection& connection) override;

    bool PopEvent(NetworkEvent* eventPtr) override;
    
    void SendMessage(NetworkChannelType channelType, const NetworkMessage& message, Array<NetworkConnection, HeapAllocation> targets) override;
};

