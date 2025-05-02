// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WIN32

#include "Engine/Platform/Base/NetworkBase.h"

class FLAXENGINE_API Win32Network : public NetworkBase
{
public:

    // [NetworkBase]
    static bool CreateSocket(NetworkSocket& socket, NetworkProtocol proto, NetworkIPVersion ipv);
    static bool DestroySocket(NetworkSocket& socket);
    static bool SetSocketOption(NetworkSocket& socket, NetworkSocketOption option, int32 value);
    static bool GetSocketOption(NetworkSocket& socket, NetworkSocketOption option, int32& value);
    static bool ConnectSocket(NetworkSocket& socket, NetworkEndPoint& endPoint);
    static bool BindSocket(NetworkSocket& socket, NetworkEndPoint& endPoint);
    static bool Listen(NetworkSocket& socket, uint16 queueSize);
    static bool Accept(NetworkSocket& serverSocket, NetworkSocket& newSocket, NetworkEndPoint& newEndPoint);
    static bool IsReadable(NetworkSocket& socket);
    static bool IsWritable(NetworkSocket& socket);
    static bool CreateSocketGroup(uint32 capacity, NetworkSocketGroup& group);
    static bool DestroySocketGroup(NetworkSocketGroup& group);
    static int32 Poll(NetworkSocketGroup& group);
    static bool GetSocketState(NetworkSocketGroup& group, uint32 index, NetworkSocketState& state);
    static int32 AddSocketToGroup(NetworkSocketGroup& group, NetworkSocket& socket);
    static bool GetSocketFromGroup(NetworkSocketGroup& group, uint32 index, NetworkSocket& socket);
    static void RemoveSocketFromGroup(NetworkSocketGroup& group, uint32 index);
    static bool RemoveSocketFromGroup(NetworkSocketGroup& group, NetworkSocket& socket);
    static void ClearGroup(NetworkSocketGroup& group);
    static int32 WriteSocket(NetworkSocket socket, byte* data, uint32 length, NetworkEndPoint* endPoint = nullptr);
    static int32 ReadSocket(NetworkSocket socket, byte* buffer, uint32 bufferSize, NetworkEndPoint* endPoint = nullptr);
    static bool CreateEndPoint(const String& address, const String& port, NetworkIPVersion ipv, NetworkEndPoint& endPoint, bool bindable = true);
    static NetworkEndPoint RemapEndPointToIPv6(NetworkEndPoint& endPoint);
};

#endif
