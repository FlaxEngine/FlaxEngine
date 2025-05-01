// Copyright (c) Wojciech Figat. All rights reserved.

#include "NetworkLagDriver.h"
#include "ENetDriver.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/Time.h"
#include "Engine/Networking/NetworkStats.h"

NetworkLagDriver::NetworkLagDriver(const SpawnParams& params)
    : ScriptingObject(params)
{
}

NetworkLagDriver::~NetworkLagDriver()
{
    SetDriver(nullptr);
}

void NetworkLagDriver::SetDriver(INetworkDriver* value)
{
    if (_driver == value)
        return;

    // Cleanup created proxy driver object
    if (auto* driver = FromInterface(_driver, INetworkDriver::TypeInitializer))
        Delete(driver);

    _driver = value;
}

String NetworkLagDriver::DriverName()
{
    if (!_driver)
        return String::Empty;
    return _driver->DriverName();
}

bool NetworkLagDriver::Initialize(NetworkPeer* host, const NetworkConfig& config)
{
    if (!_driver)
    {
        // Use ENet as default
        _driver = New<ENetDriver>();
    }
    if (!_driver)
    {
        LOG(Error, "Missing Driver for Network Lag simulation.");
        return true;
    }
    if (_driver->Initialize(host, config))
        return true;
    Engine::Update.Bind<NetworkLagDriver, &NetworkLagDriver::OnUpdate>(this);
    return false;
}

void NetworkLagDriver::Dispose()
{
    if (!_driver)
        return;
    Engine::Update.Unbind<NetworkLagDriver, &NetworkLagDriver::OnUpdate>(this);
    _driver->Dispose();
    _messages.Clear();
    _events.Clear();
}

bool NetworkLagDriver::Listen()
{
    if (!_driver)
        return false;
    return _driver->Listen();
}

bool NetworkLagDriver::Connect()
{
    if (!_driver)
        return false;
    return _driver->Connect();
}

void NetworkLagDriver::Disconnect()
{
    if (!_driver)
        return;
    return _driver->Disconnect();
}

void NetworkLagDriver::Disconnect(const NetworkConnection& connection)
{
    if (!_driver)
        return;
    _driver->Disconnect(connection);
}

bool NetworkLagDriver::PopEvent(NetworkEvent& eventPtr)
{
    if (!_driver)
        return false;

    // Try to pop lagged event from the queue
    for (int32 i = 0; i < _events.Count(); i++)
    {
        auto& e = _events[i];
        if (e.Lag > 0.0)
            continue;

        eventPtr = e.Event;
        _events.RemoveAtKeepOrder(i);
        return true;
    }

    // Consume any incoming events
    while (_driver->PopEvent(eventPtr))
    {
        if (Lag <= 0.0)
            return true;

        auto& e = _events.AddOne();
        e.Lag = (double)Lag;
        e.Event = eventPtr;
    }
    return false;
}

void NetworkLagDriver::SendMessage(const NetworkChannelType channelType, const NetworkMessage& message)
{
    if (Lag <= 0.0)
    {
        _driver->SendMessage(channelType, message);
        return;
    }

    auto& msg = _messages.AddOne();
    msg.Lag = (double)Lag;
    msg.ChannelType = channelType;
    msg.Type = 0;
    msg.MessageData.Set(message.Buffer, message.Length);
    msg.MessageLength = message.Length;
}

void NetworkLagDriver::SendMessage(NetworkChannelType channelType, const NetworkMessage& message, NetworkConnection target)
{
    if (Lag <= 0.0)
    {
        _driver->SendMessage(channelType, message, target);
        return;
    }

    auto& msg = _messages.AddOne();
    msg.Lag = (double)Lag;
    msg.ChannelType = channelType;
    msg.Type = 1;
    msg.Target = target;
    msg.MessageData.Set(message.Buffer, message.Length);
    msg.MessageLength = message.Length;
}

void NetworkLagDriver::SendMessage(const NetworkChannelType channelType, const NetworkMessage& message, const Array<NetworkConnection, HeapAllocation>& targets)
{
    if (Lag <= 0.0)
    {
        _driver->SendMessage(channelType, message, targets);
        return;
    }

    auto& msg = _messages.AddOne();
    msg.Lag = (double)Lag;
    msg.ChannelType = channelType;
    msg.Type = 2;
    msg.Targets = targets;
    msg.MessageData.Set(message.Buffer, message.Length);
    msg.MessageLength = message.Length;
}

NetworkDriverStats NetworkLagDriver::GetStats()
{
    if (!_driver)
        return NetworkDriverStats();
    return _driver->GetStats();
}

NetworkDriverStats NetworkLagDriver::GetStats(NetworkConnection target)
{
    if (!_driver)
        return NetworkDriverStats();
    return _driver->GetStats(target);
}

void NetworkLagDriver::OnUpdate()
{
    if (!_driver)
        return;

    // Update all pending messages and events
    const double deltaTime = Time::Update.UnscaledDeltaTime.GetTotalMilliseconds();
    for (int32 i = 0; i < _messages.Count(); i++)
    {
        auto& msg = _messages[i];
        msg.Lag -= deltaTime;
        if (msg.Lag > 0.0)
            continue;

        // Use this helper message as a container to send the stored data and length to the ENet driver
        NetworkMessage message;
        message.Buffer = msg.MessageData.Get();
        message.Length = msg.MessageLength;

        switch (msg.Type)
        {
        case 0:
            _driver->SendMessage(msg.ChannelType, message);
            break;
        case 1:
            _driver->SendMessage(msg.ChannelType, message, msg.Target);
            break;
        case 2:
            _driver->SendMessage(msg.ChannelType, message, msg.Targets);
            break;
        }
        _messages.RemoveAt(i);
    }
    for (int32 i = 0; i < _events.Count(); i++)
    {
        auto& e = _events[i];
        e.Lag -= deltaTime;
    }
}
