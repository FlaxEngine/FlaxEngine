// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "NetworkBase.h"

bool NetworkBase::Init()
{
    return false;
}

void NetworkBase::Exit()
{
}

bool NetworkBase::CreateSocket(NetworkSocket& socket, NetworkProtocolType proto, NetworkIPVersion ipv)
{
    return false;
}

bool NetworkBase::DestroySocket(NetworkSocket& socket)
{
    return false;
}

bool NetworkBase::SetSocketOption(NetworkSocket& socket, NetworkSocketOption option, bool value)
{
    return false;
}

bool NetworkBase::SetSocketOption(NetworkSocket& socket, NetworkSocketOption option, int32 value)
{
    return false;
}

bool NetworkBase::GetSocketOption(NetworkSocket& socket, NetworkSocketOption option, bool* value)
{
    return false;
}

bool NetworkBase::GetSocketOption(NetworkSocket& socket, NetworkSocketOption option, int32* value)
{
    return false;
}

bool NetworkBase::ConnectSocket(NetworkSocket& socket, NetworkEndPoint& endPoint)
{
    return false;
}

bool NetworkBase::BindSocket(NetworkSocket& socket, NetworkEndPoint& endPoint)
{
    return false;
}

bool NetworkBase::Listen(NetworkSocket& socket, uint16 queueSize)
{
    return false;
}

bool NetworkBase::Accept(NetworkSocket& serverSock, NetworkSocket& newSock, NetworkEndPoint& newEndPoint)
{
    return false;
}

bool NetworkBase::IsReadable(NetworkSocket& socket)
{
    return false;
}

bool NetworkBase::IsWriteable(NetworkSocket& socket)
{
    return true;
}

int32 NetworkBase::WriteSocket(NetworkSocket socket, byte* data, uint32 length, NetworkEndPoint* endPoint)
{
    return 0;
}

int32 NetworkBase::ReadSocket(NetworkSocket socket, byte* buffer, uint32 bufferSize, NetworkEndPoint* endPoint)
{
    return 0;
}

bool NetworkBase::CreateEndPoint(String* address, String* port, NetworkIPVersion ipv, NetworkEndPoint& endPoint, bool bindable)
{
    return false;
}

NetworkEndPoint NetworkBase::RemapEndPointToIPv6(NetworkEndPoint& endPoint)
{
    return NetworkEndPoint();
}

