// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ENetDriver.h"

#include "Engine/Core/Collections/Array.h"

void ENetDriver::Initialize(const NetworkConfig& config)
{
}

void ENetDriver::Dispose()
{
}

bool ENetDriver::Listen()
{
    return false;
}

void ENetDriver::Connect()
{
}

void ENetDriver::Disconnect()
{
}

void ENetDriver::Disconnect(const NetworkConnection& connection)
{
}

bool ENetDriver::PopEvent(NetworkEvent* eventPtr)
{
    return true; // No events
}

void ENetDriver::SendMessage(NetworkChannelType channelType, const NetworkMessage& message, Array<NetworkConnection, HeapAllocation> targets)
{
}
