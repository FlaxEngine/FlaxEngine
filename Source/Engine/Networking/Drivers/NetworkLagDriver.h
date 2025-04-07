// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Networking/INetworkDriver.h"
#include "Engine/Networking/NetworkMessage.h"
#include "Engine/Networking/NetworkConnection.h"
#include "Engine/Networking/NetworkEvent.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// Low-level network transport interface implementation that is proxy of another nested INetworkDriver implementation but with lag simulation feature.
/// </summary>
API_CLASS(Namespace="FlaxEngine.Networking", Sealed) class FLAXENGINE_API NetworkLagDriver : public ScriptingObject, public INetworkDriver
{
    DECLARE_SCRIPTING_TYPE(NetworkLagDriver);

private:
    struct LagMessage
    {
        double Lag;
        int32 Type;
        NetworkChannelType ChannelType;
        NetworkConnection Target;
        Array<NetworkConnection> Targets;
        Array<byte> MessageData; // TODO: use message buffers cache (of size config.MessageSize) to reduce dynamic memory allocations
        uint32 MessageLength;
    };

    struct LagEvent
    {
        double Lag;
        NetworkEvent Event;
    };

    INetworkDriver* _driver = nullptr;
    Array<LagMessage> _messages;
    Array<LagEvent> _events;

public:
    ~NetworkLagDriver();

    /// <summary>
    /// Network lag value in milliseconds. Adds a delay between sending and receiving messages (2 * Lag is TTL).
    /// </summary>
    API_FIELD() float Lag = 100.0f;

    /// <summary>
    /// Gets or sets the nested INetworkDriver to use as a proxy with lags.
    /// </summary>
    API_PROPERTY() INetworkDriver* GetDriver() const
    {
        return _driver;
    }

    /// <summary>
    /// Gets or sets the nested INetworkDriver to use as a proxy with lags.
    /// </summary>
    API_PROPERTY() void SetDriver(INetworkDriver* value);

public:
    // [INetworkDriver]
    String DriverName() override;
    bool Initialize(NetworkPeer* host, const NetworkConfig& config) override;
    void Dispose() override;
    bool Listen() override;
    bool Connect() override;
    void Disconnect() override;
    void Disconnect(const NetworkConnection& connection) override;
    bool PopEvent(NetworkEvent& eventPtr) override;
    void SendMessage(NetworkChannelType channelType, const NetworkMessage& message) override;
    void SendMessage(NetworkChannelType channelType, const NetworkMessage& message, NetworkConnection target) override;
    void SendMessage(NetworkChannelType channelType, const NetworkMessage& message, const Array<NetworkConnection, HeapAllocation>& targets) override;
    NetworkDriverStats GetStats() override;
    NetworkDriverStats GetStats(NetworkConnection target) override;

private:
    void OnUpdate();
};
