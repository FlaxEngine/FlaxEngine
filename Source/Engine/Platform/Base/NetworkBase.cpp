// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "NetworkBase.h"

bool NetworkBase::CreateSocket(NetworkSocket& socket, NetworkProtocolType proto, NetworkIPVersion ipv)
{
    return true;
}

bool NetworkBase::DestroySocket(NetworkSocket& socket)
{
    return true;
}

bool NetworkBase::SetSocketOption(NetworkSocket& socket, NetworkSocketOption option, bool value)
{
    return true;
}

bool NetworkBase::SetSocketOption(NetworkSocket& socket, NetworkSocketOption option, int32 value)
{
    return true;
}

bool NetworkBase::GetSocketOption(NetworkSocket& socket, NetworkSocketOption option, bool* value)
{
    return true;
}

bool NetworkBase::GetSocketOption(NetworkSocket& socket, NetworkSocketOption option, int32* value)
{
    return true;
}

bool NetworkBase::ConnectSocket(NetworkSocket& socket, NetworkEndPoint& endPoint)
{
    return true;
}

bool NetworkBase::BindSocket(NetworkSocket& socket, NetworkEndPoint& endPoint)
{
    return true;
}

bool NetworkBase::Listen(NetworkSocket& socket, uint16 queueSize)
{
    return true;
}

bool NetworkBase::Accept(NetworkSocket& serverSock, NetworkSocket& newSock, NetworkEndPoint& newEndPoint)
{
    return true;
}

bool NetworkBase::IsReadable(NetworkSocket& socket)
{
    return true;
}

bool NetworkBase::IsWriteable(NetworkSocket& socket)
{
    return true;
}

int32 NetworkBase::Poll(NetworkSocketGroup& group)
{
    return -1;
}

bool NetworkBase::GetSocketState(NetworkSocketGroup& group, uint32 index, NetworkSocketState& state)
{
    return true;
}

int32 NetworkBase::AddSocketToGroup(NetworkSocketGroup& group, NetworkSocket& socket)
{
    return -1;
}

void NetworkBase::ClearGroup(NetworkSocketGroup& group)
{
    group.Count = 0;
}

int32 NetworkBase::WriteSocket(NetworkSocket socket, byte* data, uint32 length, NetworkEndPoint* endPoint)
{
    return -1;
}

int32 NetworkBase::ReadSocket(NetworkSocket socket, byte* buffer, uint32 bufferSize, NetworkEndPoint* endPoint)
{
    return -1;
}

bool NetworkBase::CreateEndPoint(String* address, String* port, NetworkIPVersion ipv, NetworkEndPoint& endPoint, bool bindable)
{
    return true;
}

NetworkEndPoint NetworkBase::RemapEndPointToIPv6(NetworkEndPoint& endPoint)
{
    return NetworkEndPoint();
}

