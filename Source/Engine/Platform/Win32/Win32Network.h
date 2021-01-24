// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WIN32

#include "Engine/Platform/Base/NetworkBase.h"

class FLAXENGINE_API Win32Network : public NetworkBase
{
    friend NetworkEndPoint;
    friend NetworkSocket;

public:
    // [NetworkBase]
    static bool Init();
    static void Exit();
    static bool CreateSocket(NetworkSocket& socket, NetworkSocketCreateSettings& settings);
    static bool DestroySocket(NetworkSocket& socket);
    static bool ConnectSocket(NetworkSocket& socket, NetworkEndPoint& endPoint);
    static bool BindSocket(NetworkSocket& socket, NetworkEndPoint& endPoint);
    static bool Listen(NetworkSocket& socket, uint16 queueSize);
    static bool Accept(NetworkSocket& serverSock, NetworkSocket& newSock, NetworkEndPoint& newEndPoint);
    static bool IsReadable(NetworkSocket& socket, uint64* size);
    static int32 WriteSocket(NetworkSocket socket, byte* data, uint32 length, NetworkEndPoint* endPoint = nullptr);
    static int32 ReadSocket(NetworkSocket socket, byte* buffer, uint32 bufferSize, NetworkEndPoint* endPoint = nullptr);
    static bool CreateEndPoint(String* address, String* port, NetworkIPVersion ipv, NetworkEndPoint& endPoint, bool bindable = false);
    static NetworkEndPoint RemapEndPointToIPv6(NetworkEndPoint endPoint);
};

#endif
