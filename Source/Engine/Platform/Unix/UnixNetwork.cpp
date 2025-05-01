// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_UNIX && !PLATFORM_PS4 && !PLATFORM_PS5

#include "UnixNetwork.h"
#include "Engine/Core/Log.h"
#include "Engine/Utilities/StringConverter.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <cerrno>

struct UnixSocketData
{
    int sockfd;
};

static_assert(sizeof(NetworkSocket::Data) >= sizeof(UnixSocketData), "NetworkSocket::Data is not big enough to contains UnixSocketData !");
static_assert(sizeof(NetworkEndPoint::Data) >= sizeof(sockaddr_in6), "NetworkEndPoint::Data is not big enough to contains sockaddr_in6 !");

static int GetAddrSize(const sockaddr& addr)
{
    return addr.sa_family == AF_INET6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in);
}

static int GetAddrSizeFromEP(NetworkEndPoint& endPoint)
{
    return endPoint.IPVersion == NetworkIPVersion::IPv6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in);
}

static void TranslateSockOptToNative(NetworkSocketOption option, int32* level, int32* name)
{
    switch (option)
    {
#define SOCKOPT(OPTENUM, OPTLEVEL, OPTNAME) case OPTENUM: *level = OPTLEVEL; *name = OPTNAME; break;
    SOCKOPT(NetworkSocketOption::Debug, SOL_SOCKET, SO_DEBUG)
    SOCKOPT(NetworkSocketOption::ReuseAddr, SOL_SOCKET, SO_REUSEADDR)
    SOCKOPT(NetworkSocketOption::KeepAlive, SOL_SOCKET, SO_KEEPALIVE)
    SOCKOPT(NetworkSocketOption::DontRoute, SOL_SOCKET, SO_DONTROUTE)
    SOCKOPT(NetworkSocketOption::Broadcast, SOL_SOCKET, SO_BROADCAST)
#ifdef SO_USELOOPBACK
    SOCKOPT(NetworkSocketOption::UseLoopback, SOL_SOCKET, SO_USELOOPBACK)
#endif
    SOCKOPT(NetworkSocketOption::Linger, SOL_SOCKET, SO_LINGER)
    SOCKOPT(NetworkSocketOption::OOBInline, SOL_SOCKET, SO_OOBINLINE)
    SOCKOPT(NetworkSocketOption::SendBuffer, SOL_SOCKET, SO_SNDBUF)
    SOCKOPT(NetworkSocketOption::RecvBuffer, SOL_SOCKET, SO_RCVBUF)
    SOCKOPT(NetworkSocketOption::SendTimeout, SOL_SOCKET, SO_SNDTIMEO)
    SOCKOPT(NetworkSocketOption::RecvTimeout, SOL_SOCKET, SO_RCVTIMEO)
    SOCKOPT(NetworkSocketOption::Error, SOL_SOCKET, SO_ERROR)
#ifdef TCP_NODELAY
    SOCKOPT(NetworkSocketOption::NoDelay, IPPROTO_TCP, TCP_NODELAY)
#endif
    SOCKOPT(NetworkSocketOption::IPv6Only, IPPROTO_IPV6, IPV6_V6ONLY)
#ifdef IP_MTU
    SOCKOPT(NetworkSocketOption::Mtu, IPPROTO_IP, IP_MTU)
#endif
    SOCKOPT(NetworkSocketOption::Type, SOL_SOCKET, SO_TYPE)
#undef SOCKOPT
    default:
        *level = 0;
        *name = 0;
        break;
    }
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
        LOG(Error, "Unable to extract address from sockaddr!");
        LOG_UNIX_LAST_ERROR;
        return true;
    }
    char strPort[6];
#if __APPLE__
    snprintf(strPort, sizeof(strPort), "%d", port);
#else
    sprintf(strPort, "%d", port);
#endif
    endPoint.IPVersion = addr->sa_family == AF_INET6 ? NetworkIPVersion::IPv6 : NetworkIPVersion::IPv4;
    memcpy(endPoint.Data, addr, size);
    return false;
}

bool UnixNetwork::CreateSocket(NetworkSocket& socket, NetworkProtocol proto, NetworkIPVersion ipv)
{
    socket.Protocol = proto;
    socket.IPVersion = ipv;
    const int domain = socket.IPVersion == NetworkIPVersion::IPv6 ? AF_INET6 : AF_INET;
    const int type = socket.Protocol == NetworkProtocol::Tcp ? SOCK_STREAM : SOCK_DGRAM;
    const int protocol = socket.Protocol == NetworkProtocol::Tcp ? IPPROTO_TCP : IPPROTO_UDP;
    auto& sock = *(UnixSocketData*)&socket.Data;
    sock.sockfd = ::socket(domain, type, protocol);
    if (sock.sockfd < 0)
    {
        LOG(Error, "Can't create native socket");
        LOG_UNIX_LAST_ERROR;
        return true;
    }
    return false;
}

bool UnixNetwork::DestroySocket(NetworkSocket& socket)
{
    auto& sock = *(UnixSocketData*)&socket.Data;
    ::close(sock.sockfd);
    return false;
}

bool UnixNetwork::SetSocketOption(NetworkSocket& socket, NetworkSocketOption option, int32 value)
{
    int32 optlvl = 0;
    int32 optnme = 0;
    TranslateSockOptToNative(option, &optlvl, &optnme);
    auto& sock = *(UnixSocketData*)&socket.Data;
    if (setsockopt(sock.sockfd, optlvl, optnme, (char*)&value, sizeof(value)) == -1)
    {
        LOG(Warning, "Unable to set socket option ! Socket : {0}", sock.sockfd);
        LOG_UNIX_LAST_ERROR;
        return true;
    }
    return false;
}

bool UnixNetwork::GetSocketOption(NetworkSocket& socket, NetworkSocketOption option, int32& value)
{
    int32 optlvl = 0;
    int32 optnme = 0;
    TranslateSockOptToNative(option, &optlvl, &optnme);
    socklen_t size;
    auto& sock = *(UnixSocketData*)&socket.Data;
    if (getsockopt(sock.sockfd, optlvl, optnme, (char*)&value, &size) == -1)
    {
        LOG(Warning, "Unable to get socket option ! Socket : {0}", sock.sockfd);
        LOG_UNIX_LAST_ERROR;
        return true;
    }
    return false;
}

bool UnixNetwork::ConnectSocket(NetworkSocket& socket, NetworkEndPoint& endPoint)
{
    const int size = GetAddrSizeFromEP(endPoint);
    auto& sock = *(UnixSocketData*)&socket.Data;
    if (connect(sock.sockfd, (const sockaddr*)endPoint.Data, size) == -1)
    {
        LOG(Error, "Unable to connect socket to address! Socket : {0}", sock.sockfd);
        LOG_UNIX_LAST_ERROR;
        return true;
    }
    return false;
}

bool UnixNetwork::BindSocket(NetworkSocket& socket, NetworkEndPoint& endPoint)
{
    auto& sock = *(UnixSocketData*)&socket.Data;
    if (socket.IPVersion != endPoint.IPVersion)
    {
        LOG(Error, "Can't bind socket to end point, Socket.IPVersion != EndPoint.IPVersion! Socket : {0}", sock.sockfd);
        return true;
    }
    const int size = GetAddrSizeFromEP(endPoint);
    if (bind(sock.sockfd, (const sockaddr*)endPoint.Data, size) == -1)
    {
        LOG(Error, "Unable to bind socket! Socket : {0}", sock.sockfd);
        LOG_UNIX_LAST_ERROR;
        return true;
    }
    return false;
}

bool UnixNetwork::Listen(NetworkSocket& socket, uint16 queueSize)
{
    auto& sock = *(UnixSocketData*)&socket.Data;
    if (listen(sock.sockfd, (int32)queueSize) == -1)
    {
        LOG(Error, "Unable to listen ! Socket : {0}", sock.sockfd);
        return true;
    }
    return false;
}

bool UnixNetwork::Accept(NetworkSocket& serverSocket, NetworkSocket& newSocket, NetworkEndPoint& newEndPoint)
{
    auto& serverSock = *(UnixSocketData*)&serverSocket.Data;
    if (serverSocket.Protocol != NetworkProtocol::Tcp)
    {
        LOG(Warning, "Can't accept connection on UDP socket! Socket : {0}", serverSock.sockfd);
        return true;
    }
    sockaddr_in6 addr;
    socklen_t size = sizeof(sockaddr_in6);
    int sock = accept(serverSock.sockfd, (sockaddr*)&addr, &size);
    if (sock < 0)
    {
        LOG(Warning, "Unable to accept incoming connection! Socket : {0}", serverSock.sockfd);
        LOG_UNIX_LAST_ERROR;
        return true;
    }
    auto& newSock = *(UnixSocketData*)&newSocket.Data;
    newSock.sockfd = sock;
    memcpy(newEndPoint.Data, &addr, size);
    newSocket.Protocol = serverSocket.Protocol;
    newSocket.IPVersion = serverSocket.IPVersion;
    if (CreateEndPointFromAddr((sockaddr*)&addr, newEndPoint))
        return true;
    return false;
}

int32 UnixNetwork::WriteSocket(NetworkSocket socket, byte* data, uint32 length, NetworkEndPoint* endPoint)
{
    auto& sock = *(UnixSocketData*)&socket.Data;
    if (endPoint != nullptr && socket.IPVersion != endPoint->IPVersion)
    {
        LOG(Error, "Unable to send data, Socket.IPVersion != EndPoint.IPVersion! Socket : {0}", sock.sockfd);
        return -1;
    }
    uint32 size;
    if (endPoint == nullptr && socket.Protocol == NetworkProtocol::Tcp)
    {
        if ((size = send(sock.sockfd, (const char*)data, length, 0)) == -1)
        {
            LOG(Error, "Unable to send data! Socket : {0} Data Length : {1}", sock.sockfd, length);
            return -1;
        }
    }
    else if (endPoint != nullptr && socket.Protocol == NetworkProtocol::Udp)
    {
        if ((size = sendto(sock.sockfd, (const char*)data, length, 0, (const sockaddr*)endPoint->Data, GetAddrSizeFromEP(*endPoint))) == -1)
        {
            LOG(Error, "Unable to send data! Socket : {0} Data Length : {1}", sock.sockfd, length);
            return -1;
        }
    }
    else
    {
        // TODO: better explanation
        LOG(Error, "Unable to send data! Socket : {0} Data Length : {1}", sock.sockfd, length);
        return -1;
    }
    return size;
}

int32 UnixNetwork::ReadSocket(NetworkSocket socket, byte* buffer, uint32 bufferSize, NetworkEndPoint* endPoint)
{
    auto& sock = *(UnixSocketData*)&socket.Data;
    uint32 size;
    if (endPoint == nullptr)
    {
        if ((size = recv(sock.sockfd, (char*)buffer, bufferSize, 0)) == -1)
        {
            LOG(Error, "Unable to read data! Socket : {0} Buffer Size : {1}", sock.sockfd, bufferSize);
            LOG_UNIX_LAST_ERROR;
            return -1;
        }
    }
    else
    {
        socklen_t addrsize = sizeof(sockaddr_in6);
        sockaddr_in6 addr;
        if ((size = recvfrom(sock.sockfd, (void*)buffer, bufferSize, 0, (sockaddr*)&addr, &addrsize)) == -1)
        {
            LOG(Error, "Unable to read data! Socket : {0} Buffer Size : {1}", sock.sockfd, bufferSize);
            return -1;
        }
        if (CreateEndPointFromAddr((sockaddr*)&addr, *endPoint))
            return true;
    }
    return size;
}

bool UnixNetwork::CreateEndPoint(const String& address, const String& port, NetworkIPVersion ipv, NetworkEndPoint& endPoint, bool bindable)
{
    int status;
    addrinfo hints;
    addrinfo* info;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = ipv == NetworkIPVersion::IPv6 ? AF_INET6 : ipv == NetworkIPVersion::IPv4 ? AF_INET : AF_UNSPEC;
    hints.ai_flags |= AI_ADDRCONFIG;
    hints.ai_flags |= AI_V4MAPPED;
    if (bindable)
        hints.ai_flags = AI_PASSIVE;
    const StringAsANSI<60> addressAnsi(*address, address.Length());
    const StringAsANSI<10> portAnsi(*port, port.Length());
    if ((status = getaddrinfo(address.IsEmpty() ? nullptr : addressAnsi.Get(), port.IsEmpty() ? nullptr : portAnsi.Get(), &hints, &info)) != 0)
    {
        LOG(Error, "Unable to query info for address : {0}::{1} Error : {2}", address, port, String(gai_strerror(status)));
        return true;
    }
    if (info == nullptr)
    {
        LOG(Error, "Unable to resolve address! Address : {0}::{1}", address, port);
        return true;
    }
    if (CreateEndPointFromAddr(info->ai_addr, endPoint))
    {
        freeaddrinfo(info);
        return true;
    }
    freeaddrinfo(info);
    return false;
}

#endif
