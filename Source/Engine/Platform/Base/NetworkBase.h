// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Types/String.h"
API_INJECT_CPP_CODE("#include \"Engine/Platform/Network.h\"");

enum class FLAXENGINE_API NetworkProtocolType
{
    Undefined,
    Udp,
    Tcp
};

enum class FLAXENGINE_API NetworkIPVersion
{
    Undefined,
    IPv4,
    IPv6
};

struct FLAXENGINE_API NetworkSocketCreateSettings
{
    NetworkProtocolType Protocol = NetworkProtocolType::Udp;
    NetworkIPVersion IPVersion = NetworkIPVersion::IPv4;
    bool DualStacking = true;
    bool ReuseAddress = true;
    bool Broadcast = false;
};

struct FLAXENGINE_API NetworkSocket
{
    NetworkProtocolType Protocol = NetworkProtocolType::Undefined;
    NetworkIPVersion IPVersion = NetworkIPVersion::Undefined;
    byte Data[8] = {}; // sizeof SOCKET = unsigned __int64
};

struct FLAXENGINE_API NetworkEndPoint
{
    NetworkIPVersion IPVersion = NetworkIPVersion::Undefined;
    String Address;
    String Port;
    byte Data[28] = {}; // sizeof sockaddr_in6 , biggest sockaddr that we will use
};

class FLAXENGINE_API NetworkBase
{
    public:
    static bool Init();
    static void Exit();
    static bool CreateSocket(NetworkSocket& socket, NetworkSocketCreateSettings& settings);
    static bool DestroySocket(NetworkSocket& socket);
    static bool ConnectSocket(NetworkSocket& socket, NetworkEndPoint& endPoint);
    static bool BindSocket(NetworkSocket& socket, NetworkEndPoint& endPoint);
    static bool Accept(NetworkSocket& serverSock, NetworkSocket& newSock, NetworkEndPoint& newEndPoint);
    static bool IsReadable(NetworkSocket& socket, uint64* size);
    static int32 WriteSocket(NetworkSocket socket, byte* data, uint32 length, NetworkEndPoint* endPoint = nullptr);
    static int32 ReadSocket(NetworkSocket socket, byte* buffer, uint32 bufferSize, NetworkEndPoint* endPoint = nullptr);
    static bool CreateEndPoint(String* address, String* port, NetworkIPVersion ipv, NetworkEndPoint& endPoint);
    static NetworkEndPoint RemapEndPointToIPv6(NetworkEndPoint& endPoint);
};

