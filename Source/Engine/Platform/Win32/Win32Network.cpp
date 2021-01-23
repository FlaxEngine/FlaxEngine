// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Win32Network.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"
#include <WinSock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>

static_assert(sizeof NetworkSocket::Data >= sizeof SOCKET, "NetworkSocket::Data is not big enough to contains SOCKET !");
static_assert(sizeof NetworkEndPoint::Data >= sizeof sockaddr_in6, "NetworkEndPoint::Data is not big enough to contains sockaddr_in6 !");

static const IN6_ADDR v4MappedPrefix = {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                  0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 }};
static WSAData _wsaData;

/*
 * TODO: Create func to retrieve WSA error string
 * Known issues :
 *  Sometimes getaddrinfo fails without reason, re-trying right after works
 *  Even if dualstacking is enabled it's not possible to bind an Ipv4mappedIPv6 endpoint. windows limitation
 */

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

//TODO: can be simplified by casting addr to the right struct and ntohl the port, so we can get rid of getnameinfo
// getnameinfo return a name ( like JimiPC ), not the ip !
static bool CreateEndPointFromAddr(sockaddr* addr, NetworkEndPoint& endPoint)
{
    uint16 size = GetAddrSize(*addr);
    char name[100];
    char service[20];
    if (getnameinfo(addr, size, name, sizeof name, service, sizeof service, 0) != 0)
    {
        LOG(Error, "Unable to extract info from sockaddr ! Error : {0}", WSAGetLastError());
        return true;
    }
    void* paddr;
    if (addr->sa_family == AF_INET6)
        paddr = &((sockaddr_in6*)addr)->sin6_addr;
    else if (addr->sa_family == AF_INET)
        paddr = &((sockaddr_in*)addr)->sin_addr;

    char ip[INET6_ADDRSTRLEN];
    if (inet_ntop(addr->sa_family, paddr, ip, INET6_ADDRSTRLEN) == nullptr)
    {
        LOG(Error, "Unable to extract address from sockaddr ! Error : {0}", WSAGetLastError());
        return true;
    }
    endPoint.Address = String(ip);
    endPoint.Port = String(service);
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
        if (curr->ai_family == AF_INET) { 
            sockaddr_in *ipv4 = (struct sockaddr_in *)curr->ai_addr;
            addr = &(ipv4->sin_addr);
        } else {
            sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)curr->ai_addr;
            addr = &(ipv6->sin6_addr);
        }
        
        char str[INET6_ADDRSTRLEN];
        inet_ntop(curr->ai_family, addr, str, INET6_ADDRSTRLEN);
        LOG(Info, "ADDR INFO  family : {0} socktype : {1}, proto : {2} address : {3}", curr->ai_family, curr->ai_socktype, curr->ai_protocol, StringAnsi(str).ToString().Get());
    }
}

bool Win32Network::Init()
{
    if (WSAStartup(MAKEWORD(2,0), &_wsaData) != 0)
        return true;
    return false;
}

void Win32Network::Exit()
{
    WSACleanup();
}

bool Win32Network::CreateSocket(NetworkSocket& netsock, NetworkSocketCreateSettings& settings)
{
    netsock.Protocol = settings.Protocol;
    netsock.IPVersion = settings.IPVersion;
    const uint8 family = settings.IPVersion == NetworkIPVersion::IPv6 ? AF_INET6 : AF_INET;
    const uint8 stype = settings.Protocol == NetworkProtocolType::Tcp ? SOCK_STREAM : SOCK_DGRAM;
    const uint8 proto = settings.Protocol == NetworkProtocolType::Tcp ? IPPROTO_TCP : IPPROTO_UDP;
    SOCKET sock;
    
    if ((sock = socket(family, stype, proto)) == INVALID_SOCKET)
    {
        LOG(Error, "Can't create native socket ! Error : {0}", WSAGetLastError());
        return true;
    }
    memcpy(netsock.Data, &sock, sizeof sock);
    DWORD dw = 0;
    if (family == AF_INET6 && setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&dw, sizeof dw) == SOCKET_ERROR)
    {
        LOG(Warning, "System does not support dual stacking socket ! Error : {0}", WSAGetLastError());
    }
    unsigned long value = 1;
    if (settings.ReuseAddress && setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof value) == SOCKET_ERROR)
    {
        LOG(Warning, "Can't set socket option to SO_REUSEADDR ! Error : {0}", WSAGetLastError());
    }

    if (settings.Broadcast && settings.Protocol == NetworkProtocolType::Udp)
    {
        if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&value, sizeof value) == SOCKET_ERROR)
        {
            LOG(Warning, "Can't set socket option to SO_BROADCAST ! Error : {0}", WSAGetLastError());
        }
    }
    else if (settings.Broadcast)
        LOG(Warning, "Can't set socket option to SO_BROADCAST ! The socket must use UDP protocol. Error : {0}", WSAGetLastError());    
    
    if (ioctlsocket(sock, FIONBIO, &value) == SOCKET_ERROR)
    {
        LOG(Error, "Can't set socket to NON-BLOCKING type ! Error : {0}", WSAGetLastError());
        return true; // Support using blocking socket , need to test it
    }
    //DEBUG
    LOG(Info, "New socket created : {0}", sock); //TODO: DEBUG
    return false;
}

bool Win32Network::DestroySocket(NetworkSocket& socket)
{
    const SOCKET sock = *(SOCKET*)socket.Data;
    if (sock != INVALID_SOCKET)
    {
        closesocket(sock);
        //DEBUG
        LOG(Info, "Deleting socket : {0}", sock); //TODO: DEBUG
        return false;
    }
    LOG(Warning, "Unable to delete socket INVALID_SOCKET ! Socket : {0}", sock);
    return true;
}


bool Win32Network::ConnectSocket(NetworkSocket& socket, NetworkEndPoint& endPoint)
{
    const uint16 size = GetAddrSizeFromEP(endPoint);
    if (connect(*(SOCKET*)socket.Data, (const sockaddr*)endPoint.Data, size) == SOCKET_ERROR)
    {
        LOG(Error, "Unable to connect socket to address ! Socket : {0} Address : {1} Port : {2}", *(SOCKET*)socket.Data, endPoint.Address.Get(), endPoint.Port.Get());
        return true;
    }
    return false;
}

bool Win32Network::BindSocket(NetworkSocket& socket, NetworkEndPoint& endPoint)
{
    if (socket.IPVersion != endPoint.IPVersion)
    {
        LOG(Error, "Can't bind socket to end point, Socket.IPVersion != EndPoint.IPVersion ! Socket : {0}", *(SOCKET*)socket.Data);
        return true;
    }

    const uint16 size = endPoint.IPVersion == NetworkIPVersion::IPv6 ? sizeof sockaddr_in6 : sizeof sockaddr_in;
    const sockaddr* addr = (sockaddr*)endPoint.Data;
    LOG(Info, "BIND : EndPoint family : {0}", addr->sa_family);
    if (bind(*(SOCKET*)socket.Data, (const sockaddr*)endPoint.Data, size) == SOCKET_ERROR)
    {
        LOG(Error, "Unable to bind socket ! Socket : {0} Address : {1} Port : {2} Error : {3}", *(SOCKET*)socket.Data, endPoint.Address.Get(), endPoint.Port.Get(), WSAGetLastError());
        return true;
    }
    //DEBUG
    LOG(Info, "Binding socket ! Socket : {0} Address : {1} Port : {2}", *(SOCKET*)socket.Data, endPoint.Address.Get(), endPoint.Port.Get());
    return false;
}

bool Win32Network::Accept(NetworkSocket& serverSock, NetworkSocket& newSock, NetworkEndPoint& newEndPoint)
{
    if (serverSock.Protocol != NetworkProtocolType::Tcp)
    {
        LOG(Warning, "Can't accept connection on UDP socket ! Socket : {0}", *(SOCKET*)serverSock.Data);
        return true;
    }
    SOCKET sock;
    sockaddr addr;
    if ((sock = accept(*(SOCKET*)serverSock.Data, &addr, nullptr)) == INVALID_SOCKET)
    {
        LOG(Warning, "Unable to accept incoming connection ! Socket : {0} Error : {1}", *(SOCKET*)serverSock.Data, WSAGetLastError());
        return true;
    }
    memcpy(newSock.Data, &sock, sizeof sock);
    memcpy(newEndPoint.Data, &addr, GetAddrSize(addr));

    newSock.Protocol = serverSock.Protocol;
    newSock.IPVersion = serverSock.IPVersion;
    if (CreateEndPointFromAddr(&addr, newEndPoint))
        return true;
    return false;
}

bool Win32Network::IsReadable(NetworkSocket& socket, uint64* size)
{
    unsigned long value;
    if (ioctlsocket(*(SOCKET*)socket.Data, FIONREAD, &value) != 0)
    {
        LOG(Error, "Unable to query socket for readability ! Socket : {0} Error : {1}", *(SOCKET*)socket.Data, WSAGetLastError());
        return true;
    }
    *size = value;
    return false;
}

int32 Win32Network::WriteSocket(NetworkSocket socket, byte* data, uint32 length, NetworkEndPoint* endPoint)
{
    if (socket.IPVersion != endPoint->IPVersion)
    {
        LOG(Error, "Unable to send data, Socket.IPVersion != EndPoint.IPVersion ! Socket : {0}", *(SOCKET*)socket.Data);
        return 0;
    }
    uint32 size;
    if (endPoint == nullptr && socket.Protocol == NetworkProtocolType::Tcp)
    {
        if ((size = send(*(SOCKET*)socket.Data, (const char*)data, length, 0)) == SOCKET_ERROR)
        {
            LOG(Error, "Unable to send data ! Socket : {0} Data Length : {1}", *(SOCKET*)socket.Data, length);
            return -1;
        }
    }
    else if (endPoint != nullptr && socket.Protocol == NetworkProtocolType::Udp)
    {
        if ((size = sendto(*(SOCKET*)socket.Data, (const char*)data, length, 0, (const sockaddr*)endPoint->Data, GetAddrSizeFromEP(*endPoint))) == SOCKET_ERROR)
        {
            LOG(Error, "Unable to send data ! Socket : {0} Address : {1} Port : {2} Data Length : {3}", *(SOCKET*)socket.Data, endPoint->Address, endPoint->Port, length);
            return -1;
        }
    }
    else
    {
        //TODO: better explanation
        LOG(Error, "Unable to send data ! Socket : {0} Data Length : {1}", *(SOCKET*)socket.Data, length);
        return -1;
    }
    return size;
}

/*
 * TODO: handle size == 0 when there is a shutdown
 */
int32 Win32Network::ReadSocket(NetworkSocket socket, byte* buffer, uint32 bufferSize, NetworkEndPoint* endPoint)
{
    uint32 size;
    if (endPoint == nullptr) // TCP
    {
        if ((size = recv(*(SOCKET*)socket.Data, (char*) buffer, bufferSize, 0)) == SOCKET_ERROR)
        {
            LOG(Error, "Unable to read data ! Socket : {0} Buffer Size : {1}", *(SOCKET*)socket.Data, bufferSize);
            return -1;
        }
    }
    else // UDP
    {
        int32 addrsize;
        sockaddr_in6 addr;
        if ((size = recvfrom(*(SOCKET*)socket.Data, (char*) buffer, bufferSize, 0, (sockaddr*)&addr, &addrsize)) == SOCKET_ERROR)
        {
            LOG(Error, "Unable to read data ! Socket : {0} Buffer Size : {1}", *(SOCKET*)socket.Data, bufferSize);
            return -1;
        }
        if (CreateEndPointFromAddr((sockaddr*)&addr, *endPoint))
            return true;
    }
    return size;
}

// if address is null, it's ADDR_ANY
bool Win32Network::CreateEndPoint(String* address, String* port, NetworkIPVersion ipv, NetworkEndPoint& endPoint)
{
    int status;
    addrinfoW hints;
    addrinfoW *info;
    //DEBUG
    LOG(Info, "Searching available adresses for {0} : {1}", address == nullptr ? String("nullptr").Get() : address->Get(),
        port == nullptr ? String("nullptr").Get() : port->Get());
    memset(&hints, 0, sizeof hints);
    hints.ai_family = ipv == NetworkIPVersion::IPv6 ? AF_INET6 : ipv == NetworkIPVersion::IPv4 ? AF_INET : AF_UNSPEC;
    hints.ai_flags |= AI_ADDRCONFIG;
    hints.ai_flags |= AI_V4MAPPED;
    if (paddr == nullptr)
    {
        hints.ai_flags = AI_PASSIVE;

    // consider using NUMERICHOST/NUMERICSERV if address is a valid Ipv4 or IPv6 so we can skip some look up ( potentially slow when resolving host names )
    // can *paddr works  ?
    // paddr = nullptr don't work with this func
    if ((status = GetAddrInfoW(address == nullptr ? nullptr : address->Get(), port->Get(), &hints, &info)) != 0)
    {
        LOG(Error, "Unable to query info for address : {0}   Error : {1} !", address != nullptr ? address->Get() : String("ANY").Get(), gai_strerror(status)); //TODO: address can be NULL
        return true;
    }

    if (info == nullptr)
    {
        LOG(Error, "Unable to resolve address ! Address : {0}", address != nullptr ? address->Get() : String("ANY").Get());//TODO: address can be NULL
        return true;
    }

    //DEBUG
    PrintAddrFromInfo(*info);
    
    // We are taking the first addr in the linked list
    if (CreateEndPointFromAddr(info->ai_addr, endPoint))
    {
        FreeAddrInfoW(info);
        return true;
    }
    FreeAddrInfoW(info);

    //DEBUG
    LOG(Info, "Address found : {0} : {1}", endPoint.Address, endPoint.Port);
    return false;
}

NetworkEndPoint Win32Network::RemapEndPointToIPv6(NetworkEndPoint endPoint)
{
    if (endPoint.IPVersion == NetworkIPVersion::IPv6)
    {
        LOG(Warning, "Unable to remap EndPoint, already in IPv6 format !");
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
    pv6.Port = endPoint.Port;
    
    return pv6;
}
