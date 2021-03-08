// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Win32Network.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"
#include <WinSock2.h>
#include <WS2ipdef.h>
#include <WS2tcpip.h>

#define SOCKOPT(OPTENUM, OPTLEVEL, OPTNAME) case OPTENUM: *level = OPTLEVEL; *name = OPTNAME; break;

static_assert(sizeof NetworkSocket::Data >= sizeof SOCKET, "NetworkSocket::Data is not big enough to contains SOCKET !");
static_assert(sizeof NetworkEndPoint::Data >= sizeof sockaddr_in6, "NetworkEndPoint::Data is not big enough to contains sockaddr_in6 !");
static_assert(SOCKGROUP_ITEMSIZE >= sizeof(pollfd), "SOCKGROUP_ITEMSIZE macro is not big enough to contains pollfd !");

// @formatter:off
static const IN6_ADDR v4MappedPrefix = { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                  0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 } };
// @formatter:on

/*
 * Todo :
 *  Return precise errors so user can understand what's happening ( disconnected, ect ... )
 * Known issues :
 *  Even if dualstacking is enabled it's not possible to bind an Ipv4mappedIPv6 endpoint. windows limitation
 */

static String GetErrorMessage(int error)
{
    wchar_t* s = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, error,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   reinterpret_cast<LPWSTR>(&s), 0, nullptr);
    String str(s);
    LocalFree(s);
    return str;
}

static String GetLastErrorMessage()
{
    return GetErrorMessage(WSAGetLastError());
}

static int GetAddrSize(const sockaddr& addr)
{
    return addr.sa_family == AF_INET6 ? sizeof sockaddr_in6 : sizeof sockaddr_in;
}

static int GetAddrSizeFromEP(NetworkEndPoint& endPoint)
{
    return endPoint.IPVersion == NetworkIPVersion::IPv6 ? sizeof sockaddr_in6 : sizeof sockaddr_in;
}

static NetworkIPVersion GetIPVersionFromAddr(const sockaddr& addr)
{
    return addr.sa_family == AF_INET6 ? NetworkIPVersion::IPv6 : NetworkIPVersion::IPv4;;
}

static bool CreateEndPointFromAddr(sockaddr* addr, NetworkEndPoint& endPoint)
{
    uint32 size = GetAddrSize(*addr);
    uint16 port;
    void* paddr;
    if (addr->sa_family == AF_INET6)
    {
        paddr = &((sockaddr_in6*)addr)->sin6_addr;
        port = ntohs(((sockaddr_in6*)addr)->sin6_port);
    }
    else if (addr->sa_family == AF_INET)
    {
        paddr = &((sockaddr_in*)addr)->sin_addr;
        port = ntohs(((sockaddr_in*)addr)->sin_port);
    }
    else
    {
        LOG(Error, "Unable to create endpoint, sockaddr must be INET or INET6! Family : {0}", addr->sa_family);
        return true;
    }

    char ip[INET6_ADDRSTRLEN];
    if (inet_ntop(addr->sa_family, paddr, ip, INET6_ADDRSTRLEN) == nullptr)
    {
        LOG(Error, "Unable to extract address from sockaddr! Error : {0}", GetLastErrorMessage());
        return true;
    }
    char strPort[6];
    _itoa(port, strPort, 10);
    endPoint.IPVersion = GetIPVersionFromAddr(*addr);
    memcpy(endPoint.Data, addr, size);
    return false;
}

static void PrintAddrFromInfo(addrinfoW& info)
{
    addrinfoW* curr;
    for (curr = &info; curr != nullptr; curr = curr->ai_next)
    {
        void* addr;
        if (curr->ai_family == AF_INET)
        {
            sockaddr_in* ipv4 = (struct sockaddr_in*)curr->ai_addr;
            addr = &(ipv4->sin_addr);
        }
        else
        {
            sockaddr_in6* ipv6 = (struct sockaddr_in6*)curr->ai_addr;
            addr = &(ipv6->sin6_addr);
        }

        char str[INET6_ADDRSTRLEN];
        inet_ntop(curr->ai_family, addr, str, INET6_ADDRSTRLEN);
        LOG(Info, "ADDR INFO family : {0} socktype : {1}, proto : {2} address : {3}", curr->ai_family, curr->ai_socktype, curr->ai_protocol, StringAnsi(str).ToString());
    }
}

static void TranslateSockOptToNative(NetworkSocketOption option, int32* level, int32* name)
{
    switch (option)
    {
    SOCKOPT(NetworkSocketOption::Debug, SOL_SOCKET, SO_DEBUG)
    SOCKOPT(NetworkSocketOption::ReuseAddr, SOL_SOCKET, SO_REUSEADDR)
    SOCKOPT(NetworkSocketOption::KeepAlive, SOL_SOCKET, SO_KEEPALIVE)
    SOCKOPT(NetworkSocketOption::DontRoute, SOL_SOCKET, SO_DONTROUTE)
    SOCKOPT(NetworkSocketOption::Broadcast, SOL_SOCKET, SO_BROADCAST)
    SOCKOPT(NetworkSocketOption::UseLoopback, SOL_SOCKET, SO_USELOOPBACK)
    SOCKOPT(NetworkSocketOption::Linger, SOL_SOCKET, SO_LINGER)
    SOCKOPT(NetworkSocketOption::OOBInline, SOL_SOCKET, SO_OOBINLINE)
    SOCKOPT(NetworkSocketOption::SendBuffer, SOL_SOCKET, SO_SNDBUF)
    SOCKOPT(NetworkSocketOption::RecvBuffer, SOL_SOCKET, SO_RCVBUF)
    SOCKOPT(NetworkSocketOption::SendTimeout, SOL_SOCKET, SO_SNDTIMEO)
    SOCKOPT(NetworkSocketOption::RecvTimeout, SOL_SOCKET, SO_RCVTIMEO)
    SOCKOPT(NetworkSocketOption::Error, SOL_SOCKET, SO_ERROR)
    SOCKOPT(NetworkSocketOption::NoDelay, IPPROTO_TCP, TCP_NODELAY)
    SOCKOPT(NetworkSocketOption::IPv6Only, IPPROTO_IPV6, IPV6_V6ONLY)
    SOCKOPT(NetworkSocketOption::Mtu, IPPROTO_IP, IP_MTU)
    SOCKOPT(NetworkSocketOption::Type, SOL_SOCKET, SO_TYPE)
    }
}

bool Win32Network::CreateSocket(NetworkSocket& socket, NetworkProtocol proto, NetworkIPVersion ipv)
{
    socket.Protocol = proto;
    socket.IPVersion = ipv;
    const uint8 family = socket.IPVersion == NetworkIPVersion::IPv6 ? AF_INET6 : AF_INET;
    const uint8 stype = socket.Protocol == NetworkProtocol::Tcp ? SOCK_STREAM : SOCK_DGRAM;
    const uint8 prot = socket.Protocol == NetworkProtocol::Tcp ? IPPROTO_TCP : IPPROTO_UDP;
    SOCKET sock;

    if ((sock = ::socket(family, stype, prot)) == INVALID_SOCKET)
    {
        LOG(Error, "Can't create native socket! Error : {0}", GetLastErrorMessage());
        return true;
    }
    memcpy(socket.Data, &sock, sizeof sock);
    unsigned long value = 1;
    if (ioctlsocket(sock, FIONBIO, &value) == SOCKET_ERROR)
    {
        LOG(Error, "Can't set socket to NON-BLOCKING type! Error : {0}", GetLastErrorMessage());
        return true; // Support using blocking socket , need to test it
    }
    return false;
}

bool Win32Network::DestroySocket(NetworkSocket& socket)
{
    const SOCKET sock = *(SOCKET*)socket.Data;
    if (sock != INVALID_SOCKET)
    {
        closesocket(sock);
        return false;
    }
    LOG(Warning, "Unable to delete socket INVALID_SOCKET! Socket : {0}", sock);
    return true;
}

bool Win32Network::SetSocketOption(NetworkSocket& socket, NetworkSocketOption option, bool value)
{
    const int32 v = value;
    return SetSocketOption(socket, option, v);
}

bool Win32Network::SetSocketOption(NetworkSocket& socket, NetworkSocketOption option, int32 value)
{
    int32 optlvl = 0;
    int32 optnme = 0;

    TranslateSockOptToNative(option, &optlvl, &optnme);

    if (setsockopt(*(SOCKET*)socket.Data, optlvl, optnme, (char*)&value, sizeof value) == SOCKET_ERROR)
    {
        LOG(Warning, "Unable to set socket option ! Socket : {0} Error : {1}", *(SOCKET*)socket.Data, GetLastErrorMessage());
        return true;
    }
    return false;
}

bool Win32Network::GetSocketOption(NetworkSocket& socket, NetworkSocketOption option, bool* value)
{
    int32 v;
    const bool status = GetSocketOption(socket, option, &v);
    *value = v == 1 ? true : false;
    return status;
}

bool Win32Network::GetSocketOption(NetworkSocket& socket, NetworkSocketOption option, int32* value)
{
    int32 optlvl = 0;
    int32 optnme = 0;

    TranslateSockOptToNative(option, &optlvl, &optnme);

    int32 size;
    if (getsockopt(*(SOCKET*)socket.Data, optlvl, optnme, (char*)value, &size) == SOCKET_ERROR)
    {
        LOG(Warning, "Unable to get socket option ! Socket : {0} Error : {1}", *(SOCKET*)socket.Data, GetLastErrorMessage());
        return true;
    }
    return false;
}

bool Win32Network::ConnectSocket(NetworkSocket& socket, NetworkEndPoint& endPoint)
{
    const uint16 size = GetAddrSizeFromEP(endPoint);
    if (connect(*(SOCKET*)socket.Data, (const sockaddr*)endPoint.Data, size) == SOCKET_ERROR)
    {
        int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK)
            return false;
        LOG(Error, "Unable to connect socket to address! Socket : {0} Error : {1}", *(SOCKET*)socket.Data, GetErrorMessage(error));
        return true;
    }
    return false;
}

bool Win32Network::BindSocket(NetworkSocket& socket, NetworkEndPoint& endPoint)
{
    if (socket.IPVersion != endPoint.IPVersion)
    {
        LOG(Error, "Can't bind socket to end point, Socket.IPVersion != EndPoint.IPVersion! Socket : {0}", *(SOCKET*)socket.Data);
        return true;
    }

    const uint16 size = endPoint.IPVersion == NetworkIPVersion::IPv6 ? sizeof sockaddr_in6 : sizeof sockaddr_in;
    if (bind(*(SOCKET*)socket.Data, (const sockaddr*)endPoint.Data, size) == SOCKET_ERROR)
    {
        LOG(Error, "Unable to bind socket! Socket : {0} Error : {1}", *(SOCKET*)socket.Data, GetLastErrorMessage());
        return true;
    }
    return false;
}

bool Win32Network::Listen(NetworkSocket& socket, uint16 queueSize)
{
    if (listen(*(SOCKET*)socket.Data, (int32)queueSize) == SOCKET_ERROR)
    {
        LOG(Error, "Unable to listen ! Socket : {0} Error : {1}", *(SOCKET*)socket.Data, GetLastErrorMessage());
        return true;
    }
    return false;
}

bool Win32Network::Accept(NetworkSocket& serverSock, NetworkSocket& newSock, NetworkEndPoint& newEndPoint)
{
    if (serverSock.Protocol != NetworkProtocol::Tcp)
    {
        LOG(Warning, "Can't accept connection on UDP socket! Socket : {0}", *(SOCKET*)serverSock.Data);
        return true;
    }
    SOCKET sock;
    sockaddr_in6 addr;
    int32 size = sizeof sockaddr_in6;
    if ((sock = accept(*(SOCKET*)serverSock.Data, (sockaddr*)&addr, &size)) == INVALID_SOCKET)
    {
        int32 error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK)
            return false;
        LOG(Warning, "Unable to accept incoming connection! Socket : {0} Error : {1}", *(SOCKET*)serverSock.Data, GetErrorMessage(error));
        return true;
    }
    memcpy(newSock.Data, &sock, sizeof sock);
    memcpy(newEndPoint.Data, &addr, size);
    newSock.Protocol = serverSock.Protocol;
    newSock.IPVersion = serverSock.IPVersion;
    if (CreateEndPointFromAddr((sockaddr*)&addr, newEndPoint))
        return true;
    return false;
}

bool Win32Network::IsReadable(NetworkSocket& socket)
{
    pollfd entry;
    entry.fd = *(SOCKET*)socket.Data;
    entry.events = POLLRDNORM;
    if (WSAPoll(&entry, 1, 0) == SOCKET_ERROR)
    {
        int32 error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK)
            return false;
        LOG(Error, "Unable to poll socket! Socket : {0} Error : {1}", *(SOCKET*)socket.Data, GetErrorMessage(error));
        return false;
    }
    if (entry.revents & POLLRDNORM)
        return true;
    return false;
}

bool Win32Network::IsWriteable(NetworkSocket& socket)
{
    pollfd entry;
    entry.fd = *(SOCKET*)socket.Data;
    entry.events = POLLWRNORM;
    if (WSAPoll(&entry, 1, 0) == SOCKET_ERROR)
    {
        int32 error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK)
            return false;
        LOG(Error, "Unable to poll socket! Socket : {0} Error : {1}", *(SOCKET*)socket.Data, GetErrorMessage(error));
        return false;
    }
    if (entry.revents & POLLWRNORM)
        return true;
    return false;
}

bool Win32Network::CreateSocketGroup(uint32 capacity, NetworkSocketGroup& group)
{
    if (!(group.Data = (byte*)Platform::Allocate(capacity * SOCKGROUP_ITEMSIZE, 16)))
    {
        LOG(Error, "Unable to malloc NetworkSocketGroup::Data ! Size : {0}", capacity * SOCKGROUP_ITEMSIZE);
        return true;
    }
    group.Capacity = capacity;
    for (int i = 0; i < (int)group.Capacity; i++)
        ((pollfd*)&group.Data[i * SOCKGROUP_ITEMSIZE])->fd = -1;

    return false;
}

bool Win32Network::DestroySocketGroup(NetworkSocketGroup& group)
{
    if (!group.Data)
        return true;
    Platform::Free(group.Data);
    return false;
}

int32 Win32Network::Poll(NetworkSocketGroup& group)
{
    int32 pollret = WSAPoll((pollfd*)group.Data, group.Count, 0);
    if (pollret == SOCKET_ERROR)
        LOG(Error, "Unable to poll socket group! Error : {0}", GetLastErrorMessage());
    return pollret;
}

bool Win32Network::GetSocketState(NetworkSocketGroup& group, uint32 index, NetworkSocketState& state)
{
    if (index >= group.Capacity)
        return true;
    pollfd* pollptr = (pollfd*)&group.Data[index * SOCKGROUP_ITEMSIZE];
    memset(&state, 0, sizeof state);
    if (pollptr->revents & POLLERR)
        state.Error = true;
    if (pollptr->revents & POLLHUP)
        state.Disconnected = true;
    if (pollptr->revents & POLLNVAL)
        state.Invalid = true;
    if (pollptr->revents & POLLRDNORM)
        state.Readable = true;
    if (pollptr->revents & POLLWRNORM)
        state.Writeable = true;
    return false;
}

int32 Win32Network::AddSocketToGroup(NetworkSocketGroup& group, NetworkSocket& socket)
{
    if (group.Count >= group.Capacity)
        return -1;

    pollfd pollinfo;
    pollinfo.fd = *(SOCKET*)socket.Data;
    pollinfo.events = POLLRDNORM | POLLWRNORM;

    for (int i = 0; i < (int)group.Capacity; i++)
    {
        if (((pollfd*)&group.Data[i * SOCKGROUP_ITEMSIZE])->fd == -1)
        {
            *(pollfd*)&group.Data[i * SOCKGROUP_ITEMSIZE] = pollinfo;
            group.Count++;
            return i;
        }
    }
    return -1;
}

bool Win32Network::GetSocketFromGroup(NetworkSocketGroup& group, uint32 index, NetworkSocket* socket)
{
    if (index >= group.Capacity)
        return true;
    SOCKET s = ((pollfd*)&group.Data[index * SOCKGROUP_ITEMSIZE])->fd;
    memcpy(socket->Data, &s, sizeof s);
    int32 value;
    if (GetSocketOption(*socket, NetworkSocketOption::Type, &value))
        return true;
    if (value == SOCK_DGRAM)
        socket->Protocol = NetworkProtocol::Udp;
    else if (value == SOCK_STREAM)
        socket->Protocol = NetworkProtocol::Tcp;
    else
        socket->Protocol = NetworkProtocol::Undefined;
    return false;
}

void Win32Network::RemoveSocketFromGroup(NetworkSocketGroup& group, uint32 index)
{
    if (((pollfd*)&group.Data[index * SOCKGROUP_ITEMSIZE])->fd != -1)
    {
        ((pollfd*)&group.Data[index * SOCKGROUP_ITEMSIZE])->fd = -1;
        group.Count--;
    }
}

bool Win32Network::RemoveSocketFromGroup(NetworkSocketGroup& group, NetworkSocket& socket)
{
    for (int i = 0; i < (int)group.Capacity; i++)
    {
        if (((pollfd*)&group.Data[i * SOCKGROUP_ITEMSIZE])->fd == *(SOCKET*)&socket.Data)
        {
            ((pollfd*)&group.Data[i * SOCKGROUP_ITEMSIZE])->fd = -1;
            group.Count--;
            return false;
        }
    }
    return true;
}

void Win32Network::ClearGroup(NetworkSocketGroup& group)
{
    for (int i = 0; i < (int)group.Capacity; i++)
        ((pollfd*)&group.Data[i * SOCKGROUP_ITEMSIZE])->fd = -1;
    group.Count = 0;
}

int32 Win32Network::WriteSocket(NetworkSocket socket, byte* data, uint32 length, NetworkEndPoint* endPoint)
{
    if (endPoint != nullptr && socket.IPVersion != endPoint->IPVersion)
    {
        LOG(Error, "Unable to send data, Socket.IPVersion != EndPoint.IPVersion! Socket : {0}", *(SOCKET*)socket.Data);
        return -1;
    }
    uint32 size;
    if (endPoint == nullptr && socket.Protocol == NetworkProtocol::Tcp)
    {
        if ((size = send(*(SOCKET*)socket.Data, (const char*)data, length, 0)) == SOCKET_ERROR)
        {
            LOG(Error, "Unable to send data! Socket : {0} Data Length : {1} Error : {2}", *(SOCKET*)socket.Data, length, GetLastErrorMessage());
            return -1;
        }
    }
    else if (endPoint != nullptr && socket.Protocol == NetworkProtocol::Udp)
    {
        if ((size = sendto(*(SOCKET*)socket.Data, (const char*)data, length, 0, (const sockaddr*)endPoint->Data, GetAddrSizeFromEP(*endPoint))) == SOCKET_ERROR)
        {
            LOG(Error, "Unable to send data! Socket : {0} Data Length : {1} Error : {2}", *(SOCKET*)socket.Data, length, GetLastErrorMessage());
            return -1;
        }
    }
    else
    {
        // TODO: better explanation
        LOG(Error, "Unable to send data! Socket : {0} Data Length : {1}", *(SOCKET*)socket.Data, length);
        return -1;
    }
    return size;
}

int32 Win32Network::ReadSocket(NetworkSocket socket, byte* buffer, uint32 bufferSize, NetworkEndPoint* endPoint)
{
    uint32 size;
    if (endPoint == nullptr)
    {
        if ((size = recv(*(SOCKET*)socket.Data, (char*)buffer, bufferSize, 0)) == SOCKET_ERROR)
        {
            const int32 error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK)
                return 0;
            LOG(Error, "Unable to read data! Socket : {0} Buffer Size : {1} Error : {2}", *(SOCKET*)socket.Data, bufferSize, GetErrorMessage(error));
            return -1;
        }
    }
    else
    {
        int32 addrsize = sizeof sockaddr_in6;
        sockaddr_in6 addr;
        if ((size = recvfrom(*(SOCKET*)socket.Data, (char*)buffer, bufferSize, 0, (sockaddr*)&addr, &addrsize)) == SOCKET_ERROR)
        {
            LOG(Error, "Unable to read data! Socket : {0} Buffer Size : {1} Error : {2}", *(SOCKET*)socket.Data, bufferSize, GetLastErrorMessage());
            return -1;
        }
        if (CreateEndPointFromAddr((sockaddr*)&addr, *endPoint))
            return true;
    }
    return size;
}

bool Win32Network::CreateEndPoint(NetworkAddress& address, NetworkIPVersion ipv, NetworkEndPoint& endPoint, bool bindable)
{
    int status;
    addrinfoW hints;
    addrinfoW* info;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = ipv == NetworkIPVersion::IPv6 ? AF_INET6 : ipv == NetworkIPVersion::IPv4 ? AF_INET : AF_UNSPEC;
    hints.ai_flags |= AI_ADDRCONFIG;
    hints.ai_flags |= AI_V4MAPPED;
    if (bindable)
        hints.ai_flags = AI_PASSIVE;

    // consider using NUMERICHOST/NUMERICSERV if address is a valid Ipv4 or IPv6 so we can skip some look up ( potentially slow when resolving host names )
    if ((status = GetAddrInfoW(address.Address == String::Empty ? nullptr : address.Address.Get(), address.Port == String::Empty ? nullptr : address.Port.Get(), &hints, &info)) != 0)
    {
        LOG(Error, "Unable to query info for address : {0} Error : {1}", address.Address != String::Empty ? address.Address : String("ANY"), gai_strerror(status));
        return true;
    }

    if (info == nullptr)
    {
        LOG(Error, "Unable to resolve address! Address : {0}", address.Address != String::Empty ? address.Address : String("ANY"));
        return true;
    }

    if (CreateEndPointFromAddr(info->ai_addr, endPoint))
    {
        FreeAddrInfoW(info);
        return true;
    }
    FreeAddrInfoW(info);

    return false;
}

NetworkEndPoint Win32Network::RemapEndPointToIPv6(NetworkEndPoint endPoint)
{
    if (endPoint.IPVersion == NetworkIPVersion::IPv6)
    {
        LOG(Warning, "Unable to remap EndPoint, already in IPv6 format!");
        return endPoint;
    }

    NetworkEndPoint pv6;
    sockaddr_in* addr4 = (sockaddr_in*)endPoint.Data;
    sockaddr_in6* addr6 = (sockaddr_in6*)pv6.Data;
    const SCOPE_ID scope = SCOPEID_UNSPECIFIED_INIT;

    // Can be replaced by windows built-in macro IN6ADDR_SETV4MAPPED()
    memset(addr6, 0, sizeof sockaddr_in6);
    addr6->sin6_family = AF_INET6;
    addr6->sin6_scope_struct = scope;
    addr6->sin6_addr = v4MappedPrefix;
    addr6->sin6_port = addr4->sin_port;
    memcpy(&addr6->sin6_addr.u.Byte[12], &addr4->sin_addr, 4); // :::::FFFF:XXXX:XXXX    X=IPv4
    pv6.IPVersion = NetworkIPVersion::IPv6;

    return pv6;
}
