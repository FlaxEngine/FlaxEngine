// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Types/String.h"
API_INJECT_CPP_CODE("#include \"Engine/Platform/Network.h\"");

#define SOCKGROUP_MAXCOUNT 64
#define SOCKGROUP_ITEMSIZE 16

enum class FLAXENGINE_API NetworkProtocolType
{
    /// <summary>Not specified.</summary>
    Undefined,
    /// <summary>User Datagram Protocol.</summary>
    Udp,
    /// <summary>Transmission Control Protocol.</summary>
    Tcp
};

enum class FLAXENGINE_API NetworkIPVersion
{
    /// <summary>Not specified.</summary>
    Undefined,
    /// <summary>Internet Protocol version 4.</summary>
    IPv4,
    /// <summary>Internet Protocol version 6.</summary>
    IPv6
};

struct FLAXENGINE_API NetworkSocket
{
    NetworkProtocolType Protocol = NetworkProtocolType::Undefined;
    NetworkIPVersion IPVersion = NetworkIPVersion::Undefined;
    byte Data[8] = {};
};

struct FLAXENGINE_API NetworkEndPoint
{
    NetworkIPVersion IPVersion = NetworkIPVersion::Undefined;
    String Address;
    String Port;
    byte Data[28] = {};
};

enum class FLAXENGINE_API NetworkSocketOption
{
    /// <summary>Enables debugging info recording.</summary>
    Debug,
    /// <summary>Allows local address reusing.</summary>
    ReuseAddr,
    /// <summary>Keeps connections alive.</summary>
    KeepAlive,
    /// <summary>Indicates that outgoing data should be sent on whatever interface the socket is bound to and not a routed on some other interface.</summary>
    DontRoute,
    /// <summary>Allows for sending broadcast data.</summary>
    Broadcast,
    /// <summary>Uses the local loopback address when sending data from this socket.</summary>
    UseLoopback,
    /// <summary>Lingers on close if data present.</summary>
    Linger,
    /// <summary>Allows out-of-bound data to be returned in-line with regular data.</summary>
    OOBInline,
    /// <summary>Socket send data buffer size.</summary>
    SendBuffer,
    /// <summary>Socket receive data buffer size.</summary>
    RecvBuffer,
    /// <summary>The timeout in milliseconds for blocking send calls.</summary>
    SendTimeout,
    /// <summary>The timeout in milliseconds for blocking receive calls.</summary>
    RecvTimeout,
    /// <summary>The last socket error code.</summary>
    Error,
    /// <summary>Enables the Nagle algorithm for TCP sockets.</summary>
    NoDelay,
    /// <summary>Enables IPv6 communication only for TCP socket.</summary>
    IPv6Only,
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
    uint32 Count = 0;
    byte Data[SOCKGROUP_MAXCOUNT * SOCKGROUP_ITEMSIZE] = {};
};

class FLAXENGINE_API NetworkBase
{
public:
    /// <summary>
    /// Creates a new native socket.
    /// </summary>
    /// <param name="socket">The socket struct to fill in.</param>
    /// <param name="proto">The protocol.</param>
    /// <param name="ipv">The ip version.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    static bool CreateSocket(NetworkSocket& socket, NetworkProtocolType proto, NetworkIPVersion ipv);

    /// <summary>
    /// Closes native socket.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    static bool DestroySocket(NetworkSocket& socket);

    /// <summary>
    /// Sets the specified socket option.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="option">The option.</param>
    /// <param name="value">The value.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    static bool SetSocketOption(NetworkSocket& socket, NetworkSocketOption option, bool value);

    /// <summary>
    /// Sets the specified socket option.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="option">The option.</param>
    /// <param name="value">The value.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    static bool SetSocketOption(NetworkSocket& socket, NetworkSocketOption option, int32 value);

    /// <summary>
    /// Gets the specified socket option.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="option">The option.</param>
    /// <param name="value">The returned value.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    static bool GetSocketOption(NetworkSocket& socket, NetworkSocketOption option, bool* value);

    /// <summary>
    /// Gets the specified socket option.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="option">The option.</param>
    /// <param name="value">The returned value.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    static bool GetSocketOption(NetworkSocket& socket, NetworkSocketOption option, int32* value);

    /// <summary>
    /// Connects a socket to the specified end point.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="endPoint">The end point.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    static bool ConnectSocket(NetworkSocket& socket, NetworkEndPoint& endPoint);

    /// <summary>
    /// Binds a socket to the specified end point.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="endPoint">The end point.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    static bool BindSocket(NetworkSocket& socket, NetworkEndPoint& endPoint);

    /// <summary>
    /// Listens for incoming connection.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="queueSize">Pending connection queue size.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    static bool Listen(NetworkSocket& socket, uint16 queueSize);

    /// <summary>
    /// Accepts a pending connection.
    /// </summary>
    /// <param name="serverSock">The socket.</param>
    /// <param name="newSock">The newly connected socket.</param>
    /// <param name="newEndPoint">The end point of the new socket.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    static bool Accept(NetworkSocket& serverSock, NetworkSocket& newSock, NetworkEndPoint& newEndPoint);

    /// <summary>
    /// Checks for socket readability.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <returns>Returns true when data is available. Otherwise false.</returns>
    static bool IsReadable(NetworkSocket& socket);

    /// <summary>
    /// Checks for socket writeability.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <returns>Returns true when data can be written. Otherwise false.</returns>
    static bool IsWriteable(NetworkSocket& socket);

    /// <summary>
    /// Updates sockets states.
    /// </summary>
    /// <param name="group">The sockets group.</param>
    /// <returns>Returns -1 on error, The number of elements where states are nonzero, otherwise 0.</returns>
    static int32 Poll(NetworkSocketGroup& group);

    /// <summary>
    /// Retrieves socket state.
    /// </summary>
    /// <param name="group">The group.</param>
    /// <param name="index">The socket index in group.</param>
    /// <param name="state">The returned state.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    static bool GetSocketState(NetworkSocketGroup& group, uint32 index, NetworkSocketState& state);

    /// <summary>
    /// Adds a socket to a group.
    /// </summary>
    /// <param name="group">The group.</param>
    /// <param name="socket">The socket.</param>
    /// <returns>Returns the socket index in group or -1 on error.</returns>
    static int32 AddSocketToGroup(NetworkSocketGroup& group, NetworkSocket& socket);

    /// <summary>
    /// Clears the socket group.
    /// </summary>
    /// <param name="group">The group.</param>
    static void ClearGroup(NetworkSocketGroup& group);

    /// <summary>
    /// Writes data to the socket.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="data">The data to write.</param>
    /// <param name="length">The length of data.</param>
    /// <param name="endPoint">If protocol is UDP , the destination end point. Otherwise nullptr.</param>
    /// <returns>Returns -1 on error, otherwise bytes written.</returns>
    static int32 WriteSocket(NetworkSocket socket, byte* data, uint32 length, NetworkEndPoint* endPoint = nullptr);

    /// <summary>
    /// Reads data on the socket.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="buffer">The buffer.</param>
    /// <param name="bufferSize">Size of the buffer.</param>
    /// <param name="endPoint">If UDP, the end point from where data is coming. Otherwise nullptr.</param>
    /// <returns>Returns -1 on error, otherwise bytes read.</returns>
    static int32 ReadSocket(NetworkSocket socket, byte* buffer, uint32 bufferSize, NetworkEndPoint* endPoint = nullptr);

    /// <summary>
    /// Creates an end point.
    /// </summary>
    /// <param name="address">The address (hostname, IPv4 or IPv6).</param>
    /// <param name="port">The port.</param>
    /// <param name="ipv">The ip version.</param>
    /// <param name="endPoint">The created end point.</param>
    /// <param name="bindable">True if the end point will be connected or binded.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    static bool CreateEndPoint(String* address, String* port, NetworkIPVersion ipv, NetworkEndPoint& endPoint, bool bindable = false);

    /// <summary>
    /// Remaps an ipv4 end point to an ipv6 one.
    /// </summary>
    /// <param name="endPoint">The ipv4 end point.</param>
    /// <returns>The ipv6 end point.</returns>
    static NetworkEndPoint RemapEndPointToIPv6(NetworkEndPoint& endPoint);
};
