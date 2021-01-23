// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "NetworkBase.h"

bool NetworkBase::Init()
{
    return false;
}

void NetworkBase::Exit()
{
}

bool NetworkBase::CreateSocket(NetworkSocket& socket, NetworkSocketCreateSettings& settings)
{
    return false;
}

bool NetworkBase::DestroySocket(NetworkSocket& socket)
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

bool NetworkBase::Accept(NetworkSocket& serverSock, NetworkSocket& newSock, NetworkEndPoint& newEndPoint)
{
    return false;
}

bool NetworkBase::IsReadable(NetworkSocket& socket, uint64* size)
{
    return false;
}

uint32 NetworkBase::WriteSocket(NetworkSocket socket, byte* data, uint32 length, NetworkEndPoint* endPoint)
{
    return 0;
}

uint32 NetworkBase::ReadSocket(NetworkSocket socket, byte* buffer, uint32 bufferSize, NetworkEndPoint* endPoint)
{
    return 0;
}

bool NetworkBase::CreateEndPoint(String* address, String* port, NetworkIPVersion ipv, NetworkEndPoint& endPoint)
{
    return false;
}

NetworkEndPoint NetworkBase::RemapEndPointToIPv6(NetworkEndPoint& endPoint)
{
    return NetworkEndPoint();
}

