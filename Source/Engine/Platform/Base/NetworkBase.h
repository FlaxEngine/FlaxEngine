// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Types/String.h"

API_INJECT_CODE(cpp, "#include \"Engine/Platform/Network.h\"");

/// <summary>
/// Network connection protocol type.
/// </summary>
API_ENUM() enum class NetworkProtocol
{
    /// <summary>Not specified.</summary>
    Undefined,
    /// <summary>User Datagram Protocol.</summary>
    Udp,
    /// <summary>Transmission Control Protocol.</summary>
    Tcp,
};

/// <summary>
/// IP version type.
/// </summary>
API_ENUM() enum class NetworkIPVersion
{
    /// <summary>Not specified.</summary>
    Undefined,
    /// <summary>Internet Protocol version 4.</summary>
    IPv4,
    /// <summary>Internet Protocol version 6.</summary>
    IPv6,
};

/// <summary>
/// Network socket.
/// </summary>
API_STRUCT() struct FLAXENGINE_API NetworkSocket
{
DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkSocketGroup)

    /// <summary>Socket protocol type.</summary>
    API_FIELD() NetworkProtocol Protocol = NetworkProtocol::Undefined;
    /// <summary>Socket address IP version.</summary>
    API_FIELD() NetworkIPVersion IPVersion = NetworkIPVersion::Undefined;
    API_FIELD(Private, NoArray) byte Data[8] = {};
};

/// <summary>
/// Network end-point.
/// </summary>
API_STRUCT() struct FLAXENGINE_API NetworkEndPoint
{
DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkSocketGroup)

    /// <summary>End-point IP version.</summary>
    API_FIELD(ReadOnly) NetworkIPVersion IPVersion = NetworkIPVersion::Undefined;
    API_FIELD(Private, NoArray) byte Data[28] = {};
};

/// <summary>
/// Network socket options.
/// </summary>
API_ENUM() enum class NetworkSocketOption
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
    /// <summary>Enables IPv6/Ipv4 dual-stacking, UDP/TCP.</summary>
    IPv6Only,
    /// <summary>Retrieve the current path MTU, the socket must be connected UDP/TCP.</summary>
    Mtu,
    /// <summary>Socket type, DGRAM, STREAM ..</summary>
    Type,
};

/// <summary>
/// Network socket state.
/// </summary>
API_ENUM() enum class NetworkSocketState
{
    /// <summary>Nothing.</summary>
    None = 0,
    /// <summary>Socket error.</summary>
    Error = 1 << 0,
    /// <summary>Invalid request.</summary>
    Invalid = 1 << 1,
    /// <summary>Socket disconnected.</summary>
    Disconnected = 1 << 2,
    /// <summary>Socket is readable.</summary>
    Readable = 1 << 3,
    /// <summary>Socket is writable.</summary>
    Writeable = 1 << 4,
};

DECLARE_ENUM_OPERATORS(NetworkSocketState);

/// <summary>
/// Network sockets group.
/// </summary>
API_STRUCT() struct FLAXENGINE_API NetworkSocketGroup
{
DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkSocketGroup)

    /// <summary>Group size.</summary>
    API_FIELD(ReadOnly) uint32 Count = 0;
    /// <summary>Group capacity.</summary>
    API_FIELD(ReadOnly) uint32 Capacity = 0;
    API_FIELD(Private) byte* Data = nullptr;
};

/// <summary>
/// Low-level networking implementation interface with Berkeley sockets.
/// </summary>
API_CLASS(Static, Name="Network", Tag="NativeInvokeUseName")
class FLAXENGINE_API NetworkBase
{
public:
    static struct FLAXENGINE_API ScriptingTypeInitializer TypeInitializer;

    /// <summary>
    /// Creates a new native socket.
    /// </summary>
    /// <param name="socket">The socket struct to fill in.</param>
    /// <param name="proto">The protocol.</param>
    /// <param name="ipv">The ip version.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    API_FUNCTION() static bool CreateSocket(API_PARAM(Ref) NetworkSocket& socket, NetworkProtocol proto, NetworkIPVersion ipv);

    /// <summary>
    /// Closes native socket.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    API_FUNCTION() static bool DestroySocket(API_PARAM(Ref) NetworkSocket& socket);

    /// <summary>
    /// Sets the specified socket option.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="option">The option.</param>
    /// <param name="value">The value.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    API_FUNCTION() static bool SetSocketOption(API_PARAM(Ref) NetworkSocket& socket, NetworkSocketOption option, int32 value);

    /// <summary>
    /// Gets the specified socket option.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="option">The option.</param>
    /// <param name="value">The returned value.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    API_FUNCTION() static bool GetSocketOption(API_PARAM(Ref) NetworkSocket& socket, NetworkSocketOption option, API_PARAM(Out) int32& value);

    /// <summary>
    /// Connects a socket to the specified end point.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="endPoint">The end point.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    API_FUNCTION() static bool ConnectSocket(API_PARAM(Ref) NetworkSocket& socket, API_PARAM(Ref) NetworkEndPoint& endPoint);

    /// <summary>
    /// Binds a socket to the specified end point.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="endPoint">The end point.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    API_FUNCTION() static bool BindSocket(API_PARAM(Ref) NetworkSocket& socket, API_PARAM(Ref) NetworkEndPoint& endPoint);

    /// <summary>
    /// Listens for incoming connection.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="queueSize">Pending connection queue size.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    API_FUNCTION() static bool Listen(API_PARAM(Ref) NetworkSocket& socket, uint16 queueSize);

    /// <summary>
    /// Accepts a pending connection.
    /// </summary>
    /// <param name="serverSocket">The socket.</param>
    /// <param name="newSocket">The newly connected socket.</param>
    /// <param name="newEndPoint">The end point of the new socket.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    API_FUNCTION() static bool Accept(API_PARAM(Ref) NetworkSocket& serverSocket, API_PARAM(Ref) NetworkSocket& newSocket, API_PARAM(Out) NetworkEndPoint& newEndPoint);

    /// <summary>
    /// Checks for socket readability.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <returns>Returns true when data is available. Otherwise false.</returns>
    API_FUNCTION() static bool IsReadable(API_PARAM(Ref) NetworkSocket& socket);

    /// <summary>
    /// Checks for socket writeability.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <returns>Returns true when data can be written. Otherwise false.</returns>
    API_FUNCTION() static bool IsWritable(API_PARAM(Ref) NetworkSocket& socket);

    /// <summary>
    /// Creates a socket group. It allocate memory based on the desired capacity.
    /// </summary>
    /// <param name="capacity">The group capacity (fixed).</param>
    /// <param name="group">The group.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    API_FUNCTION() static bool CreateSocketGroup(uint32 capacity, API_PARAM(Out) NetworkSocketGroup& group);

    /// <summary>
    /// Destroy the socket group, and free the allocated memory. 
    /// </summary>
    /// <param name="group">The group.</param>
    /// <returns>Returns true if the group is already destroyed, otherwise false.</returns>
    API_FUNCTION() static bool DestroySocketGroup(API_PARAM(Ref) NetworkSocketGroup& group);

    /// <summary>
    /// Updates sockets states.
    /// </summary>
    /// <param name="group">The sockets group.</param>
    /// <returns>Returns -1 on error, The number of elements where states are nonzero, otherwise 0.</returns>
    API_FUNCTION() static int32 Poll(API_PARAM(Ref) NetworkSocketGroup& group);

    /// <summary>
    /// Retrieves socket state.
    /// </summary>
    /// <param name="group">The group.</param>
    /// <param name="index">The socket index in group.</param>
    /// <param name="state">The returned state.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    API_FUNCTION() static bool GetSocketState(API_PARAM(Ref) NetworkSocketGroup& group, uint32 index, API_PARAM(Out) NetworkSocketState& state);

    /// <summary>
    /// Adds a socket to a group.
    /// </summary>
    /// <param name="group">The group.</param>
    /// <param name="socket">The socket.</param>
    /// <returns>Returns the socket index in group or -1 on error.</returns>
    API_FUNCTION() static int32 AddSocketToGroup(API_PARAM(Ref) NetworkSocketGroup& group, API_PARAM(Ref) NetworkSocket& socket);

    /// <summary>
    /// Gets a socket by index. Some data like socket IPVersion might be undefined.
    /// </summary>
    /// <param name="group">The group.</param>
    /// <param name="index">The index.</param>
    /// <param name="socket">The returned socket.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    API_FUNCTION() static bool GetSocketFromGroup(API_PARAM(Ref) NetworkSocketGroup& group, uint32 index, API_PARAM(Out) NetworkSocket& socket);

    /// <summary>
    /// Removes the socket at the specified index.
    /// </summary>
    /// <param name="group">The group.</param>
    /// <param name="index">The index.</param>
    API_FUNCTION() static void RemoveSocketFromGroup(API_PARAM(Ref) NetworkSocketGroup& group, uint32 index);

    /// <summary>
    /// Removes the socket if present.
    /// </summary>
    /// <param name="group">The group.</param>
    /// <param name="socket">The socket.</param>
    /// <returns>Returns true if the socket is not found, otherwise false.</returns>
    API_FUNCTION() static bool RemoveSocketFromGroup(API_PARAM(Ref) NetworkSocketGroup& group, API_PARAM(Ref) NetworkSocket& socket);

    /// <summary>
    /// Clears the socket group.
    /// </summary>
    /// <param name="group">The group.</param>
    API_FUNCTION() static void ClearGroup(API_PARAM(Ref) NetworkSocketGroup& group);

    /// <summary>
    /// Writes data to the socket.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="data">The data to write.</param>
    /// <param name="length">The length of data.</param>
    /// <param name="endPoint">If protocol is UDP, the destination end point. Otherwise nullptr.</param>
    /// <returns>Returns -1 on error, otherwise bytes written.</returns>
    API_FUNCTION() static int32 WriteSocket(NetworkSocket socket, byte* data, uint32 length, NetworkEndPoint* endPoint = nullptr);

    /// <summary>
    /// Reads data on the socket.
    /// </summary>
    /// <param name="socket">The socket.</param>
    /// <param name="buffer">The buffer.</param>
    /// <param name="bufferSize">Size of the buffer.</param>
    /// <param name="endPoint">If UDP, the end point from where data is coming. Otherwise nullptr.</param>
    /// <returns>Returns -1 on error, otherwise bytes read.</returns>
    API_FUNCTION() static int32 ReadSocket(NetworkSocket socket, byte* buffer, uint32 bufferSize, NetworkEndPoint* endPoint = nullptr);

    /// <summary>
    /// Creates an end point.
    /// </summary>
    /// <param name="address">The network address.</param>
    /// <param name="port">The network port.</param>
    /// <param name="ipv">The ip version.</param>
    /// <param name="endPoint">The created end point.</param>
    /// <param name="bindable">True if the end point will be connected or binded.</param>
    /// <returns>Returns true on error, otherwise false.</returns>
    API_FUNCTION() static bool CreateEndPoint(const String& address, const String& port, NetworkIPVersion ipv, API_PARAM(Out) NetworkEndPoint& endPoint, bool bindable = true);

    /// <summary>
    /// Remaps an ipv4 end point to an ipv6 one.
    /// </summary>
    /// <param name="endPoint">The ipv4 end point.</param>
    /// <returns>The ipv6 end point.</returns>
    API_FUNCTION() static NetworkEndPoint RemapEndPointToIPv6(API_PARAM(Ref) NetworkEndPoint& endPoint);
};
