// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Types/String.h"
API_INJECT_CPP_CODE("#include \"Engine/Platform/Network.h\"");
#define SOCKGROUP_MAXCOUNT 64
#define SOCKGROUP_ITEMSIZE 16

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

enum FLAXENGINE_API NetworkSocketOption
{
    Debug,
    ReuseAddr,
    KeepAlive,
    DontRoute,
    Broadcast,
    UseLoopback,
    Linger,
    OOBInline,
    SendBuffer,
    RecvBuffer,
    SendTimeout,
    RecvTimeout,
    Error,
    NoDelay,
    IPv6Only
};

struct FLAXENGINE_API NetworkSocketState
{
    bool Error = false;
    bool Invalid = false;
    bool Disconnected = false;
    bool Readable = false;
    bool Writeable = false;
};

struct FLAXENGINE_API NetworkSocketGroup
{
    uint32 Count;
    byte Data[SOCKGROUP_MAXCOUNT * 16] = {};
};

class FLAXENGINE_API NetworkBase
{
    public:
    /// <summary>
    /// Initialize the network module.
    /// </summary>
    /// <returns>Return true on error. Otherwise false.</returns>
    static bool Init();

    /// <summary>
    /// Deinitialize the network module.
    /// </summary>
    static void Exit();

    /// <summary>
    /// Create a new native socket.
    /// </summary>
    /// <param name="socket">The socket struct to fill in.</param>
    /// <param name="proto">The protocol.</param>
    /// <param name="ipv">The ip version.</param>
    /// <returns>Return true on error. Otherwise false.</returns>
    static bool CreateSocket(NetworkSocket& socket, NetworkProtocolType proto, NetworkIPVersion ipv);

    /// <summary>
    /// Close native socket.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <returns>Return true on error. Otherwise false.</returns>
    static bool DestroySocket(NetworkSocket& socket);

    /// <summary>
    /// Set the specified socket option.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="option">The option.</param>
    /// <param name="value">The value.</param>
    /// <returns>Return true on error. Otherwise false.</returns>
    static bool SetSocketOption(NetworkSocket& socket, NetworkSocketOption option, bool value);

    /// <summary>
    /// Set the specified socket option.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="option">The option.</param>
    /// <param name="value">The value.</param>
    /// <returns>Return true on error. Otherwise false.</returns>
    static bool SetSocketOption(NetworkSocket& socket, NetworkSocketOption option, int32 value);

    /// <summary>
    /// Get the specified socket option.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="option">The option.</param>
    /// <param name="value">The returned value.</param>
    /// <returns>Return true on error. Otherwise false.</returns>
    static bool GetSocketOption(NetworkSocket& socket, NetworkSocketOption option, bool* value);

    /// <summary>
    /// Get the specified socket option.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="option">The option.</param>
    /// <param name="value">The returned value.</param>
    /// <returns>Return true on error. Otherwise false.</returns>
    static bool GetSocketOption(NetworkSocket& socket, NetworkSocketOption option, int32* value);

    /// <summary>
    /// Connect a socket to the specified end point.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="endPoint">The end point.</param>
    /// <returns>Return true on error. Otherwise false.</returns>
    static bool ConnectSocket(NetworkSocket& socket, NetworkEndPoint& endPoint);

    /// <summary>
    /// Bind a socket to the specified end point.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="endPoint">The end point.</param>
    /// <returns>Return true on error. Otherwise false.</returns>
    static bool BindSocket(NetworkSocket& socket, NetworkEndPoint& endPoint);

    /// <summary>
    /// Listen for incoming connection.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="queueSize">Pending connection queue size.</param>
    /// <returns>Return true on error. Otherwise false.</returns>
    static bool Listen(NetworkSocket& socket, uint16 queueSize);

    /// <summary>
    /// Accept a pending connection.
    /// </summary>
    /// <param name="serverSock">The socket.</param>
    /// <param name="newSock">The newly connected socket.</param>
    /// <param name="newEndPoint">The end point of the new socket.</param>
    /// <returns>Return true on error. Otherwise false.</returns>
    static bool Accept(NetworkSocket& serverSock, NetworkSocket& newSock, NetworkEndPoint& newEndPoint);
    static bool IsReadable(NetworkSocket& socket);
    static bool IsWriteable(NetworkSocket& socket);
    static int32 Poll(NetworkSocketGroup& group);
    static bool GetSocketState(NetworkSocketGroup& group, uint32 index, NetworkSocketState& state);
    static int32 AddSocketToGroup(NetworkSocketGroup& group, NetworkSocket& socket);
    static void ClearGroup(NetworkSocketGroup& group);
    static int32 WriteSocket(NetworkSocket socket, byte* data, uint32 length, NetworkEndPoint* endPoint = nullptr);
    static int32 ReadSocket(NetworkSocket socket, byte* buffer, uint32 bufferSize, NetworkEndPoint* endPoint = nullptr);
    static bool CreateEndPoint(String* address, String* port, NetworkIPVersion ipv, NetworkEndPoint& endPoint, bool bindable = false);
    static NetworkEndPoint RemapEndPointToIPv6(NetworkEndPoint& endPoint);
};

