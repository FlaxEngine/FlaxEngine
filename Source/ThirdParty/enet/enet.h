/**
 * include/enet.h - a Single-Header auto-generated variant of enet.h library.
 *
 * Usage:
 * #define ENET_IMPLEMENTATION exactly in ONE source file right BEFORE including the library, like:
 *
 * #define ENET_IMPLEMENTATION
 * #include <enet.h>
 *
 * License:
 * The MIT License (MIT)
 *
 * Copyright (c) 2002-2016 Lee Salzman
 * Copyright (c) 2017-2021 Vladyslav Hrytsenko, Dominik Madarász
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#ifndef ENET_INCLUDE_H
#define ENET_INCLUDE_H

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define ENET_VERSION_MAJOR 2
#define ENET_VERSION_MINOR 3
#define ENET_VERSION_PATCH 6
#define ENET_VERSION_CREATE(major, minor, patch) (((major)<<16) | ((minor)<<8) | (patch))
#define ENET_VERSION_GET_MAJOR(version) (((version)>>16)&0xFF)
#define ENET_VERSION_GET_MINOR(version) (((version)>>8)&0xFF)
#define ENET_VERSION_GET_PATCH(version) ((version)&0xFF)
#define ENET_VERSION ENET_VERSION_CREATE(ENET_VERSION_MAJOR, ENET_VERSION_MINOR, ENET_VERSION_PATCH)

#define ENET_TIME_OVERFLOW 86400000
#define ENET_TIME_LESS(a, b) ((a) - (b) >= ENET_TIME_OVERFLOW)
#define ENET_TIME_GREATER(a, b) ((b) - (a) >= ENET_TIME_OVERFLOW)
#define ENET_TIME_LESS_EQUAL(a, b) (! ENET_TIME_GREATER (a, b))
#define ENET_TIME_GREATER_EQUAL(a, b) (! ENET_TIME_LESS (a, b))
#define ENET_TIME_DIFFERENCE(a, b) ((a) - (b) >= ENET_TIME_OVERFLOW ? (b) - (a) : (a) - (b))

// =======================================================================//
// !
// ! System differences
// !
// =======================================================================//

#if defined(_WIN32)
    #if defined(_MSC_VER) && defined(ENET_IMPLEMENTATION)
        #pragma warning (disable: 4267) // size_t to int conversion
        #pragma warning (disable: 4244) // 64bit to 32bit int
        #pragma warning (disable: 4018) // signed/unsigned mismatch
        #pragma warning (disable: 4146) // unary minus operator applied to unsigned type
    #endif

    #ifndef ENET_NO_PRAGMA_LINK
    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(lib, "winmm.lib")
    #endif

    #if _MSC_VER >= 1910
    /* It looks like there were changes as of Visual Studio 2017 and there are no 32/64 bit
       versions of _InterlockedExchange[operation], only InterlockedExchange[operation]
       (without leading underscore), so we have to distinguish between compiler versions */
    #define NOT_UNDERSCORED_INTERLOCKED_EXCHANGE
    #endif

    #ifdef __GNUC__
    #if (_WIN32_WINNT < 0x0600)
    #undef _WIN32_WINNT
    #define _WIN32_WINNT 0x0600
    #endif
    #endif

    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <mmsystem.h>

    #include <intrin.h>

    #if defined(_WIN32) && defined(_MSC_VER)
    #if _MSC_VER < 1900
    typedef struct timespec {
        long tv_sec;
        long tv_nsec;
    };
    #endif
    #define CLOCK_MONOTONIC 0
    #endif

    typedef SOCKET ENetSocket;
    #define ENET_SOCKET_NULL INVALID_SOCKET

    #define ENET_HOST_TO_NET_16(value) (htons(value))
    #define ENET_HOST_TO_NET_32(value) (htonl(value))

    #define ENET_NET_TO_HOST_16(value) (ntohs(value))
    #define ENET_NET_TO_HOST_32(value) (ntohl(value))

    typedef struct {
        size_t dataLength;
        void * data;
    } ENetBuffer;

    #define ENET_CALLBACK __cdecl

    #ifdef ENET_DLL
    #ifdef ENET_IMPLEMENTATION
    #define ENET_API __declspec( dllexport )
    #else
    #define ENET_API __declspec( dllimport )
    #endif // ENET_IMPLEMENTATION
    #else
    #define ENET_API extern
    #endif // ENET_DLL

    typedef fd_set ENetSocketSet;

    #define ENET_SOCKETSET_EMPTY(sockset)          FD_ZERO(&(sockset))
    #define ENET_SOCKETSET_ADD(sockset, socket)    FD_SET(socket, &(sockset))
    #define ENET_SOCKETSET_REMOVE(sockset, socket) FD_CLR(socket, &(sockset))
    #define ENET_SOCKETSET_CHECK(sockset, socket)  FD_ISSET(socket, &(sockset))
#else
    #include <sys/types.h>
    #include <sys/ioctl.h>
    #include <sys/time.h>
    #include <sys/socket.h>
    #include <poll.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <string.h>
    #include <errno.h>
    #include <fcntl.h>

    #ifdef __APPLE__
    #include <mach/clock.h>
    #include <mach/mach.h>
    #include <Availability.h>
    #endif

    #ifndef MSG_NOSIGNAL
    #define MSG_NOSIGNAL 0
    #endif

    #ifdef MSG_MAXIOVLEN
    #define ENET_BUFFER_MAXIMUM MSG_MAXIOVLEN
    #endif

    typedef int ENetSocket;

    #define ENET_SOCKET_NULL -1

    #define ENET_HOST_TO_NET_16(value) (htons(value)) /**< macro that converts host to net byte-order of a 16-bit value */
    #define ENET_HOST_TO_NET_32(value) (htonl(value)) /**< macro that converts host to net byte-order of a 32-bit value */

    #define ENET_NET_TO_HOST_16(value) (ntohs(value)) /**< macro that converts net to host byte-order of a 16-bit value */
    #define ENET_NET_TO_HOST_32(value) (ntohl(value)) /**< macro that converts net to host byte-order of a 32-bit value */

    typedef struct {
        void * data;
        size_t dataLength;
    } ENetBuffer;

    #define ENET_CALLBACK
    #define ENET_API extern

    typedef fd_set ENetSocketSet;

    #define ENET_SOCKETSET_EMPTY(sockset)          FD_ZERO(&(sockset))
    #define ENET_SOCKETSET_ADD(sockset, socket)    FD_SET(socket, &(sockset))
    #define ENET_SOCKETSET_REMOVE(sockset, socket) FD_CLR(socket, &(sockset))
    #define ENET_SOCKETSET_CHECK(sockset, socket)  FD_ISSET(socket, &(sockset))
#endif

#ifdef __GNUC__
#define ENET_DEPRECATED(func) func __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define ENET_DEPRECATED(func) __declspec(deprecated) func
#else
#pragma message("WARNING: Please ENET_DEPRECATED for this compiler")
#define ENET_DEPRECATED(func) func
#endif

#ifndef ENET_BUFFER_MAXIMUM
#define ENET_BUFFER_MAXIMUM (1 + 2 * ENET_PROTOCOL_MAXIMUM_PACKET_COMMANDS)
#endif

#define ENET_UNUSED(x) (void)x;

#define ENET_MAX(x, y) ((x) > (y) ? (x) : (y))
#define ENET_MIN(x, y) ((x) < (y) ? (x) : (y))

#define ENET_IPV6           1
static const struct in6_addr enet_v4_anyaddr   = {{{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 }}};
static const struct in6_addr enet_v4_noaddr    = {{{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }}};
static const struct in6_addr enet_v4_localhost = {{{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x01 }}};
static const struct in6_addr enet_v6_anyaddr   = {{{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}};
static const struct in6_addr enet_v6_noaddr    = {{{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }}};
static const struct in6_addr enet_v6_localhost = {{{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }}};
#define ENET_HOST_ANY       in6addr_any
#define ENET_HOST_BROADCAST 0xFFFFFFFFU
#define ENET_PORT_ANY       0

#ifdef __cplusplus
extern "C" {
#endif

// =======================================================================//
// !
// ! Basic stuff
// !
// =======================================================================//

    typedef uint8_t   enet_uint8;   /**< unsigned 8-bit type  */
    typedef uint16_t enet_uint16;   /**< unsigned 16-bit type */
    typedef uint32_t enet_uint32;   /**< unsigned 32-bit type */
    typedef uint64_t enet_uint64;   /**< unsigned 64-bit type */

    typedef enet_uint32 ENetVersion;
    typedef struct _ENetPacket ENetPacket;

    typedef struct _ENetCallbacks {
        void *(ENET_CALLBACK *malloc) (size_t size);
        void (ENET_CALLBACK *free) (void *memory);
        void (ENET_CALLBACK *no_memory) (void);

        ENetPacket *(ENET_CALLBACK *packet_create)        (const void *data, size_t dataLength, enet_uint32 flags);
        void        (ENET_CALLBACK *packet_destroy)       (ENetPacket *packet);
    } ENetCallbacks;

    extern void *enet_malloc(size_t);
    extern void enet_free(void *);
    extern ENetPacket* enet_packet_create(const void*,size_t,enet_uint32);
    extern ENetPacket* enet_packet_copy(ENetPacket*);
    extern void enet_packet_destroy(ENetPacket*);

// =======================================================================//
// !
// ! List
// !
// =======================================================================//

    typedef struct _ENetListNode {
        struct _ENetListNode *next;
        struct _ENetListNode *previous;
    } ENetListNode;

    typedef ENetListNode *ENetListIterator;

    typedef struct _ENetList {
        ENetListNode sentinel;
    } ENetList;

    extern ENetListIterator enet_list_insert(ENetListIterator, void *);
    extern ENetListIterator enet_list_move(ENetListIterator, void *, void *);

    extern void *enet_list_remove(ENetListIterator);
    extern void enet_list_clear(ENetList *);
    extern size_t enet_list_size(ENetList *);

    #define enet_list_begin(list) ((list)->sentinel.next)
    #define enet_list_end(list) (&(list)->sentinel)
    #define enet_list_empty(list) (enet_list_begin(list) == enet_list_end(list))
    #define enet_list_next(iterator) ((iterator)->next)
    #define enet_list_previous(iterator) ((iterator)->previous)
    #define enet_list_front(list) ((void *)(list)->sentinel.next)
    #define enet_list_back(list) ((void *)(list)->sentinel.previous)


// =======================================================================//
// !
// ! Protocol
// !
// =======================================================================//

    enum {
        ENET_PROTOCOL_MINIMUM_MTU             = 576,
        ENET_PROTOCOL_MAXIMUM_MTU             = 4096,
        ENET_PROTOCOL_MAXIMUM_PACKET_COMMANDS = 32,
        ENET_PROTOCOL_MINIMUM_WINDOW_SIZE     = 4096,
        ENET_PROTOCOL_MAXIMUM_WINDOW_SIZE     = 65536,
        ENET_PROTOCOL_MINIMUM_CHANNEL_COUNT   = 1,
        ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT   = 255,
        ENET_PROTOCOL_MAXIMUM_PEER_ID         = 0xFFF,
        ENET_PROTOCOL_MAXIMUM_FRAGMENT_COUNT  = 1024 * 1024
    };

    typedef enum _ENetProtocolCommand {
        ENET_PROTOCOL_COMMAND_NONE                     = 0,
        ENET_PROTOCOL_COMMAND_ACKNOWLEDGE              = 1,
        ENET_PROTOCOL_COMMAND_CONNECT                  = 2,
        ENET_PROTOCOL_COMMAND_VERIFY_CONNECT           = 3,
        ENET_PROTOCOL_COMMAND_DISCONNECT               = 4,
        ENET_PROTOCOL_COMMAND_PING                     = 5,
        ENET_PROTOCOL_COMMAND_SEND_RELIABLE            = 6,
        ENET_PROTOCOL_COMMAND_SEND_UNRELIABLE          = 7,
        ENET_PROTOCOL_COMMAND_SEND_FRAGMENT            = 8,
        ENET_PROTOCOL_COMMAND_SEND_UNSEQUENCED         = 9,
        ENET_PROTOCOL_COMMAND_BANDWIDTH_LIMIT          = 10,
        ENET_PROTOCOL_COMMAND_THROTTLE_CONFIGURE       = 11,
        ENET_PROTOCOL_COMMAND_SEND_UNRELIABLE_FRAGMENT = 12,
        ENET_PROTOCOL_COMMAND_COUNT                    = 13,

        ENET_PROTOCOL_COMMAND_MASK                     = 0x0F
    } ENetProtocolCommand;

    typedef enum _ENetProtocolFlag {
        ENET_PROTOCOL_COMMAND_FLAG_ACKNOWLEDGE = (1 << 7),
        ENET_PROTOCOL_COMMAND_FLAG_UNSEQUENCED = (1 << 6),

        ENET_PROTOCOL_HEADER_FLAG_COMPRESSED   = (1 << 14),
        ENET_PROTOCOL_HEADER_FLAG_SENT_TIME    = (1 << 15),
        ENET_PROTOCOL_HEADER_FLAG_MASK         = ENET_PROTOCOL_HEADER_FLAG_COMPRESSED | ENET_PROTOCOL_HEADER_FLAG_SENT_TIME,

        ENET_PROTOCOL_HEADER_SESSION_MASK      = (3 << 12),
        ENET_PROTOCOL_HEADER_SESSION_SHIFT     = 12
    } ENetProtocolFlag;

    #ifdef _MSC_VER
    #pragma pack(push, 1)
    #define ENET_PACKED
    #elif defined(__GNUC__) || defined(__clang__)
    #define ENET_PACKED __attribute__ ((packed))
    #else
    #define ENET_PACKED
    #endif

    typedef struct _ENetProtocolHeader {
        enet_uint16 peerID;
        enet_uint16 sentTime;
    } ENET_PACKED ENetProtocolHeader;

    typedef struct _ENetProtocolCommandHeader {
        enet_uint8  command;
        enet_uint8  channelID;
        enet_uint16 reliableSequenceNumber;
    } ENET_PACKED ENetProtocolCommandHeader;

    typedef struct _ENetProtocolAcknowledge {
        ENetProtocolCommandHeader header;
        enet_uint16               receivedReliableSequenceNumber;
        enet_uint16               receivedSentTime;
    } ENET_PACKED ENetProtocolAcknowledge;

    typedef struct _ENetProtocolConnect {
        ENetProtocolCommandHeader header;
        enet_uint16               outgoingPeerID;
        enet_uint8                incomingSessionID;
        enet_uint8                outgoingSessionID;
        enet_uint32               mtu;
        enet_uint32               windowSize;
        enet_uint32               channelCount;
        enet_uint32               incomingBandwidth;
        enet_uint32               outgoingBandwidth;
        enet_uint32               packetThrottleInterval;
        enet_uint32               packetThrottleAcceleration;
        enet_uint32               packetThrottleDeceleration;
        enet_uint32               connectID;
        enet_uint32               data;
    } ENET_PACKED ENetProtocolConnect;

    typedef struct _ENetProtocolVerifyConnect {
        ENetProtocolCommandHeader header;
        enet_uint16               outgoingPeerID;
        enet_uint8                incomingSessionID;
        enet_uint8                outgoingSessionID;
        enet_uint32               mtu;
        enet_uint32               windowSize;
        enet_uint32               channelCount;
        enet_uint32               incomingBandwidth;
        enet_uint32               outgoingBandwidth;
        enet_uint32               packetThrottleInterval;
        enet_uint32               packetThrottleAcceleration;
        enet_uint32               packetThrottleDeceleration;
        enet_uint32               connectID;
    } ENET_PACKED ENetProtocolVerifyConnect;

    typedef struct _ENetProtocolBandwidthLimit {
        ENetProtocolCommandHeader header;
        enet_uint32               incomingBandwidth;
        enet_uint32               outgoingBandwidth;
    } ENET_PACKED ENetProtocolBandwidthLimit;

    typedef struct _ENetProtocolThrottleConfigure {
        ENetProtocolCommandHeader header;
        enet_uint32               packetThrottleInterval;
        enet_uint32               packetThrottleAcceleration;
        enet_uint32               packetThrottleDeceleration;
    } ENET_PACKED ENetProtocolThrottleConfigure;

    typedef struct _ENetProtocolDisconnect {
        ENetProtocolCommandHeader header;
        enet_uint32               data;
    } ENET_PACKED ENetProtocolDisconnect;

    typedef struct _ENetProtocolPing {
        ENetProtocolCommandHeader header;
    } ENET_PACKED ENetProtocolPing;

    typedef struct _ENetProtocolSendReliable {
        ENetProtocolCommandHeader header;
        enet_uint16               dataLength;
    } ENET_PACKED ENetProtocolSendReliable;

    typedef struct _ENetProtocolSendUnreliable {
        ENetProtocolCommandHeader header;
        enet_uint16               unreliableSequenceNumber;
        enet_uint16               dataLength;
    } ENET_PACKED ENetProtocolSendUnreliable;

    typedef struct _ENetProtocolSendUnsequenced {
        ENetProtocolCommandHeader header;
        enet_uint16               unsequencedGroup;
        enet_uint16               dataLength;
    } ENET_PACKED ENetProtocolSendUnsequenced;

    typedef struct _ENetProtocolSendFragment {
        ENetProtocolCommandHeader header;
        enet_uint16               startSequenceNumber;
        enet_uint16               dataLength;
        enet_uint32               fragmentCount;
        enet_uint32               fragmentNumber;
        enet_uint32               totalLength;
        enet_uint32               fragmentOffset;
    } ENET_PACKED ENetProtocolSendFragment;

    typedef union _ENetProtocol {
        ENetProtocolCommandHeader     header;
        ENetProtocolAcknowledge       acknowledge;
        ENetProtocolConnect           connect;
        ENetProtocolVerifyConnect     verifyConnect;
        ENetProtocolDisconnect        disconnect;
        ENetProtocolPing              ping;
        ENetProtocolSendReliable      sendReliable;
        ENetProtocolSendUnreliable    sendUnreliable;
        ENetProtocolSendUnsequenced   sendUnsequenced;
        ENetProtocolSendFragment      sendFragment;
        ENetProtocolBandwidthLimit    bandwidthLimit;
        ENetProtocolThrottleConfigure throttleConfigure;
    } ENET_PACKED ENetProtocol;

    #ifdef _MSC_VER
    #pragma pack(pop)
    #endif

// =======================================================================//
// !
// ! General ENet structs/enums
// !
// =======================================================================//

    typedef enum _ENetSocketType {
        ENET_SOCKET_TYPE_STREAM   = 1,
        ENET_SOCKET_TYPE_DATAGRAM = 2
    } ENetSocketType;

    typedef enum _ENetSocketWait {
        ENET_SOCKET_WAIT_NONE      = 0,
        ENET_SOCKET_WAIT_SEND      = (1 << 0),
        ENET_SOCKET_WAIT_RECEIVE   = (1 << 1),
        ENET_SOCKET_WAIT_INTERRUPT = (1 << 2)
    } ENetSocketWait;

    typedef enum _ENetSocketOption {
        ENET_SOCKOPT_NONBLOCK  = 1,
        ENET_SOCKOPT_BROADCAST = 2,
        ENET_SOCKOPT_RCVBUF    = 3,
        ENET_SOCKOPT_SNDBUF    = 4,
        ENET_SOCKOPT_REUSEADDR = 5,
        ENET_SOCKOPT_RCVTIMEO  = 6,
        ENET_SOCKOPT_SNDTIMEO  = 7,
        ENET_SOCKOPT_ERROR     = 8,
        ENET_SOCKOPT_NODELAY   = 9,
        ENET_SOCKOPT_IPV6_V6ONLY = 10,
    } ENetSocketOption;

    typedef enum _ENetSocketShutdown {
        ENET_SOCKET_SHUTDOWN_READ       = 0,
        ENET_SOCKET_SHUTDOWN_WRITE      = 1,
        ENET_SOCKET_SHUTDOWN_READ_WRITE = 2
    } ENetSocketShutdown;

    /**
     * Portable internet address structure.
     *
     * The host must be specified in network byte-order, and the port must be in host
     * byte-order. The constant ENET_HOST_ANY may be used to specify the default
     * server host. The constant ENET_HOST_BROADCAST may be used to specify the
     * broadcast address (255.255.255.255).  This makes sense for enet_host_connect,
     * but not for enet_host_create.  Once a server responds to a broadcast, the
     * address is updated from ENET_HOST_BROADCAST to the server's actual IP address.
     */
    typedef struct _ENetAddress {
        struct in6_addr host;
        enet_uint16 port;
        enet_uint16 sin6_scope_id;
    } ENetAddress;

    #define in6_equal(in6_addr_a, in6_addr_b) (memcmp(&in6_addr_a, &in6_addr_b, sizeof(struct in6_addr)) == 0)

    /**
     * Packet flag bit constants.
     *
     * The host must be specified in network byte-order, and the port must be in
     * host byte-order. The constant ENET_HOST_ANY may be used to specify the
     * default server host.
     *
     * @sa ENetPacket
     */
    typedef enum _ENetPacketFlag {
        ENET_PACKET_FLAG_RELIABLE            = (1 << 0), /** packet must be received by the target peer and resend attempts should be made until the packet is delivered */
        ENET_PACKET_FLAG_UNSEQUENCED         = (1 << 1), /** packet will not be sequenced with other packets not supported for reliable packets */
        ENET_PACKET_FLAG_NO_ALLOCATE         = (1 << 2), /** packet will not allocate data, and user must supply it instead */
        ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT = (1 << 3), /** packet will be fragmented using unreliable (instead of reliable) sends if it exceeds the MTU */
        ENET_PACKET_FLAG_SENT                = (1 << 8), /** whether the packet has been sent from all queues it has been entered into */
    } ENetPacketFlag;

    typedef void (ENET_CALLBACK *ENetPacketFreeCallback)(void *);

    /**
     * ENet packet structure.
     *
     * An ENet data packet that may be sent to or received from a peer. The shown
     * fields should only be read and never modified. The data field contains the
     * allocated data for the packet. The dataLength fields specifies the length
     * of the allocated data.  The flags field is either 0 (specifying no flags),
     * or a bitwise-or of any combination of the following flags:
     *
     *    ENET_PACKET_FLAG_RELIABLE - packet must be received by the target peer and resend attempts should be made until the packet is delivered
     *    ENET_PACKET_FLAG_UNSEQUENCED - packet will not be sequenced with other packets (not supported for reliable packets)
     *    ENET_PACKET_FLAG_NO_ALLOCATE - packet will not allocate data, and user must supply it instead
     *    ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT - packet will be fragmented using unreliable (instead of reliable) sends if it exceeds the MTU
     *    ENET_PACKET_FLAG_SENT - whether the packet has been sent from all queues it has been entered into
     * @sa ENetPacketFlag
     */
    typedef struct _ENetPacket {
        size_t                 referenceCount; /**< internal use only */
        enet_uint32            flags;          /**< bitwise-or of ENetPacketFlag constants */
        enet_uint8 *           data;           /**< allocated data for packet */
        size_t                 dataLength;     /**< length of data */
        ENetPacketFreeCallback freeCallback;   /**< function to be called when the packet is no longer in use */
        void *                 userData;       /**< application private data, may be freely modified */
    } ENetPacket;

    typedef struct _ENetAcknowledgement {
        ENetListNode acknowledgementList;
        enet_uint32  sentTime;
        ENetProtocol command;
    } ENetAcknowledgement;

    typedef struct _ENetOutgoingCommand {
        ENetListNode outgoingCommandList;
        enet_uint16  reliableSequenceNumber;
        enet_uint16  unreliableSequenceNumber;
        enet_uint32  sentTime;
        enet_uint32  roundTripTimeout;
        enet_uint32  roundTripTimeoutLimit;
        enet_uint32  fragmentOffset;
        enet_uint16  fragmentLength;
        enet_uint16  sendAttempts;
        ENetProtocol command;
        ENetPacket * packet;
    } ENetOutgoingCommand;

    typedef struct _ENetIncomingCommand {
        ENetListNode incomingCommandList;
        enet_uint16  reliableSequenceNumber;
        enet_uint16  unreliableSequenceNumber;
        ENetProtocol command;
        enet_uint32  fragmentCount;
        enet_uint32  fragmentsRemaining;
        enet_uint32 *fragments;
        ENetPacket * packet;
    } ENetIncomingCommand;

    typedef enum _ENetPeerState {
        ENET_PEER_STATE_DISCONNECTED             = 0,
        ENET_PEER_STATE_CONNECTING               = 1,
        ENET_PEER_STATE_ACKNOWLEDGING_CONNECT    = 2,
        ENET_PEER_STATE_CONNECTION_PENDING       = 3,
        ENET_PEER_STATE_CONNECTION_SUCCEEDED     = 4,
        ENET_PEER_STATE_CONNECTED                = 5,
        ENET_PEER_STATE_DISCONNECT_LATER         = 6,
        ENET_PEER_STATE_DISCONNECTING            = 7,
        ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT = 8,
        ENET_PEER_STATE_ZOMBIE                   = 9
    } ENetPeerState;

    enum {
        ENET_HOST_RECEIVE_BUFFER_SIZE          = 256 * 1024,
        ENET_HOST_SEND_BUFFER_SIZE             = 256 * 1024,
        ENET_HOST_BANDWIDTH_THROTTLE_INTERVAL  = 1000,
        ENET_HOST_DEFAULT_MTU                  = 1400,
        ENET_HOST_DEFAULT_MAXIMUM_PACKET_SIZE  = 32 * 1024 * 1024,
        ENET_HOST_DEFAULT_MAXIMUM_WAITING_DATA = 32 * 1024 * 1024,

        ENET_PEER_DEFAULT_ROUND_TRIP_TIME      = 500,
        ENET_PEER_DEFAULT_PACKET_THROTTLE      = 32,
        ENET_PEER_PACKET_THROTTLE_SCALE        = 32,
        ENET_PEER_PACKET_THROTTLE_COUNTER      = 7,
        ENET_PEER_PACKET_THROTTLE_ACCELERATION = 2,
        ENET_PEER_PACKET_THROTTLE_DECELERATION = 2,
        ENET_PEER_PACKET_THROTTLE_INTERVAL     = 5000,
        ENET_PEER_PACKET_LOSS_SCALE            = (1 << 16),
        ENET_PEER_PACKET_LOSS_INTERVAL         = 10000,
        ENET_PEER_WINDOW_SIZE_SCALE            = 64 * 1024,
        ENET_PEER_TIMEOUT_LIMIT                = 32,
        ENET_PEER_TIMEOUT_MINIMUM              = 5000,
        ENET_PEER_TIMEOUT_MAXIMUM              = 30000,
        ENET_PEER_PING_INTERVAL                = 500,
        ENET_PEER_UNSEQUENCED_WINDOWS          = 64,
        ENET_PEER_UNSEQUENCED_WINDOW_SIZE      = 1024,
        ENET_PEER_FREE_UNSEQUENCED_WINDOWS     = 32,
        ENET_PEER_RELIABLE_WINDOWS             = 16,
        ENET_PEER_RELIABLE_WINDOW_SIZE         = 0x1000,
        ENET_PEER_FREE_RELIABLE_WINDOWS        = 8
    };

    typedef struct _ENetChannel {
        enet_uint16 outgoingReliableSequenceNumber;
        enet_uint16 outgoingUnreliableSequenceNumber;
        enet_uint16 usedReliableWindows;
        enet_uint16 reliableWindows[ENET_PEER_RELIABLE_WINDOWS];
        enet_uint16 incomingReliableSequenceNumber;
        enet_uint16 incomingUnreliableSequenceNumber;
        ENetList    incomingReliableCommands;
        ENetList    incomingUnreliableCommands;
    } ENetChannel;

    /**
     * An ENet peer which data packets may be sent or received from.
     *
     * No fields should be modified unless otherwise specified.
     */
    typedef struct _ENetPeer {
        ENetListNode      dispatchList;
        struct _ENetHost *host;
        enet_uint16       outgoingPeerID;
        enet_uint16       incomingPeerID;
        enet_uint32       connectID;
        enet_uint8        outgoingSessionID;
        enet_uint8        incomingSessionID;
        ENetAddress       address; /**< Internet address of the peer */
        void *            data;    /**< Application private data, may be freely modified */
        ENetPeerState     state;
        ENetChannel *     channels;
        size_t            channelCount;      /**< Number of channels allocated for communication with peer */
        enet_uint32       incomingBandwidth; /**< Downstream bandwidth of the client in bytes/second */
        enet_uint32       outgoingBandwidth; /**< Upstream bandwidth of the client in bytes/second */
        enet_uint32       incomingBandwidthThrottleEpoch;
        enet_uint32       outgoingBandwidthThrottleEpoch;
        enet_uint32       incomingDataTotal;
        enet_uint64       totalDataReceived;
        enet_uint32       outgoingDataTotal;
        enet_uint64       totalDataSent;
        enet_uint32       lastSendTime;
        enet_uint32       lastReceiveTime;
        enet_uint32       nextTimeout;
        enet_uint32       earliestTimeout;
        enet_uint32       packetLossEpoch;
        enet_uint32       packetsSent;
        enet_uint64       totalPacketsSent; /**< total number of packets sent during a session */
        enet_uint32       packetsLost;
        enet_uint32       totalPacketsLost;     /**< total number of packets lost during a session */
        enet_uint32       packetLoss; /**< mean packet loss of reliable packets as a ratio with respect to the constant ENET_PEER_PACKET_LOSS_SCALE */
        enet_uint32       packetLossVariance;
        enet_uint32       packetThrottle;
        enet_uint32       packetThrottleLimit;
        enet_uint32       packetThrottleCounter;
        enet_uint32       packetThrottleEpoch;
        enet_uint32       packetThrottleAcceleration;
        enet_uint32       packetThrottleDeceleration;
        enet_uint32       packetThrottleInterval;
        enet_uint32       pingInterval;
        enet_uint32       timeoutLimit;
        enet_uint32       timeoutMinimum;
        enet_uint32       timeoutMaximum;
        enet_uint32       lastRoundTripTime;
        enet_uint32       lowestRoundTripTime;
        enet_uint32       lastRoundTripTimeVariance;
        enet_uint32       highestRoundTripTimeVariance;
        enet_uint32       roundTripTime; /**< mean round trip time (RTT), in milliseconds, between sending a reliable packet and receiving its acknowledgement */
        enet_uint32       roundTripTimeVariance;
        enet_uint32       mtu;
        enet_uint32       windowSize;
        enet_uint32       reliableDataInTransit;
        enet_uint16       outgoingReliableSequenceNumber;
        ENetList          acknowledgements;
        ENetList          sentReliableCommands;
        ENetList          sentUnreliableCommands;
        ENetList          outgoingReliableCommands;
        ENetList          outgoingUnreliableCommands;
        ENetList          dispatchedCommands;
        int               needsDispatch;
        enet_uint16       incomingUnsequencedGroup;
        enet_uint16       outgoingUnsequencedGroup;
        enet_uint32       unsequencedWindow[ENET_PEER_UNSEQUENCED_WINDOW_SIZE / 32];
        enet_uint32       eventData;
        size_t            totalWaitingData;
    } ENetPeer;

    /** An ENet packet compressor for compressing UDP packets before socket sends or receives. */
    typedef struct _ENetCompressor {
        /** Context data for the compressor. Must be non-NULL. */
        void *context;

        /** Compresses from inBuffers[0:inBufferCount-1], containing inLimit bytes, to outData, outputting at most outLimit bytes. Should return 0 on failure. */
        size_t(ENET_CALLBACK * compress) (void *context, const ENetBuffer * inBuffers, size_t inBufferCount, size_t inLimit, enet_uint8 * outData, size_t outLimit);

        /** Decompresses from inData, containing inLimit bytes, to outData, outputting at most outLimit bytes. Should return 0 on failure. */
        size_t(ENET_CALLBACK * decompress) (void *context, const enet_uint8 * inData, size_t inLimit, enet_uint8 * outData, size_t outLimit);

        /** Destroys the context when compression is disabled or the host is destroyed. May be NULL. */
        void (ENET_CALLBACK * destroy)(void *context);
    } ENetCompressor;

    /** Callback that computes the checksum of the data held in buffers[0:bufferCount-1] */
    typedef enet_uint32 (ENET_CALLBACK * ENetChecksumCallback)(const ENetBuffer *buffers, size_t bufferCount);

    /** Callback for intercepting received raw UDP packets. Should return 1 to intercept, 0 to ignore, or -1 to propagate an error. */
    typedef int (ENET_CALLBACK * ENetInterceptCallback)(struct _ENetHost *host, void *event);

    /** An ENet host for communicating with peers.
     *
     * No fields should be modified unless otherwise stated.
     *
     *  @sa enet_host_create()
     *  @sa enet_host_destroy()
     *  @sa enet_host_connect()
     *  @sa enet_host_service()
     *  @sa enet_host_flush()
     *  @sa enet_host_broadcast()
     *  @sa enet_host_compress()
     *  @sa enet_host_channel_limit()
     *  @sa enet_host_bandwidth_limit()
     *  @sa enet_host_bandwidth_throttle()
     */
    typedef struct _ENetHost {
        ENetSocket            socket;
        ENetAddress           address;           /**< Internet address of the host */
        enet_uint32           incomingBandwidth; /**< downstream bandwidth of the host */
        enet_uint32           outgoingBandwidth; /**< upstream bandwidth of the host */
        enet_uint32           bandwidthThrottleEpoch;
        enet_uint32           mtu;
        enet_uint32           randomSeed;
        int                   recalculateBandwidthLimits;
        ENetPeer *            peers;        /**< array of peers allocated for this host */
        size_t                peerCount;    /**< number of peers allocated for this host */
        size_t                channelLimit; /**< maximum number of channels allowed for connected peers */
        enet_uint32           serviceTime;
        ENetList              dispatchQueue;
        int                   continueSending;
        size_t                packetSize;
        enet_uint16           headerFlags;
        ENetProtocol          commands[ENET_PROTOCOL_MAXIMUM_PACKET_COMMANDS];
        size_t                commandCount;
        ENetBuffer            buffers[ENET_BUFFER_MAXIMUM];
        size_t                bufferCount;
        ENetChecksumCallback  checksum; /**< callback the user can set to enable packet checksums for this host */
        ENetCompressor        compressor;
        enet_uint8            packetData[2][ENET_PROTOCOL_MAXIMUM_MTU];
        ENetAddress           receivedAddress;
        enet_uint8 *          receivedData;
        size_t                receivedDataLength;
        enet_uint32           totalSentData;        /**< total data sent, user should reset to 0 as needed to prevent overflow */
        enet_uint32           totalSentPackets;     /**< total UDP packets sent, user should reset to 0 as needed to prevent overflow */
        enet_uint32           totalReceivedData;    /**< total data received, user should reset to 0 as needed to prevent overflow */
        enet_uint32           totalReceivedPackets; /**< total UDP packets received, user should reset to 0 as needed to prevent overflow */
        ENetInterceptCallback intercept;            /**< callback the user can set to intercept received raw UDP packets */
        size_t                connectedPeers;
        size_t                bandwidthLimitedPeers;
        size_t                duplicatePeers;     /**< optional number of allowed peers from duplicate IPs, defaults to ENET_PROTOCOL_MAXIMUM_PEER_ID */
        size_t                maximumPacketSize;  /**< the maximum allowable packet size that may be sent or received on a peer */
        size_t                maximumWaitingData; /**< the maximum aggregate amount of buffer space a peer may use waiting for packets to be delivered */
    } ENetHost;

    /**
     * An ENet event type, as specified in @ref ENetEvent.
     */
    typedef enum _ENetEventType {
        /** no event occurred within the specified time limit */
        ENET_EVENT_TYPE_NONE       = 0,

        /** a connection request initiated by enet_host_connect has completed.
         * The peer field contains the peer which successfully connected.
         */
        ENET_EVENT_TYPE_CONNECT    = 1,

        /** a peer has disconnected.  This event is generated on a successful
         * completion of a disconnect initiated by enet_peer_disconnect, if
         * a peer has timed out.  The peer field contains the peer
         * which disconnected. The data field contains user supplied data
         * describing the disconnection, or 0, if none is available.
         */
        ENET_EVENT_TYPE_DISCONNECT = 2,

        /** a packet has been received from a peer.  The peer field specifies the
         * peer which sent the packet.  The channelID field specifies the channel
         * number upon which the packet was received.  The packet field contains
         * the packet that was received; this packet must be destroyed with
         * enet_packet_destroy after use.
         */
        ENET_EVENT_TYPE_RECEIVE    = 3,

        /** a peer is disconnected because the host didn't receive the acknowledgment
         * packet within certain maximum time out. The reason could be because of bad
         * network connection or  host crashed.
         */
        ENET_EVENT_TYPE_DISCONNECT_TIMEOUT = 4,
    } ENetEventType;

    /**
     * An ENet event as returned by enet_host_service().
     *
     * @sa enet_host_service
     */
    typedef struct _ENetEvent {
        ENetEventType type;      /**< type of the event */
        ENetPeer *    peer;      /**< peer that generated a connect, disconnect or receive event */
        enet_uint8    channelID; /**< channel on the peer that generated the event, if appropriate */
        enet_uint32   data;      /**< data associated with the event, if appropriate */
        ENetPacket *  packet;    /**< packet associated with the event, if appropriate */
    } ENetEvent;

// =======================================================================//
// !
// ! Public API
// !
// =======================================================================//

    /**
     * Initializes ENet globally.  Must be called prior to using any functions in ENet.
     * @returns 0 on success, < 0 on failure
     */
    ENET_API int enet_initialize(void);

    /**
     * Initializes ENet globally and supplies user-overridden callbacks. Must be called prior to using any functions in ENet. Do not use enet_initialize() if you use this variant. Make sure the ENetCallbacks structure is zeroed out so that any additional callbacks added in future versions will be properly ignored.
     *
     * @param version the constant ENET_VERSION should be supplied so ENet knows which version of ENetCallbacks struct to use
     * @param inits user-overridden callbacks where any NULL callbacks will use ENet's defaults
     * @returns 0 on success, < 0 on failure
     */
    ENET_API int enet_initialize_with_callbacks(ENetVersion version, const ENetCallbacks * inits);

    /**
     * Shuts down ENet globally.  Should be called when a program that has initialized ENet exits.
     */
    ENET_API void enet_deinitialize(void);

    /**
     * Gives the linked version of the ENet library.
     * @returns the version number
     */
    ENET_API ENetVersion enet_linked_version(void);

    /** Returns the monotonic time in milliseconds. Its initial value is unspecified unless otherwise set. */
    ENET_API enet_uint32 enet_time_get(void);

    /** ENet socket functions */
    ENET_API ENetSocket enet_socket_create(ENetSocketType);
    ENET_API int        enet_socket_bind(ENetSocket, const ENetAddress *);
    ENET_API int        enet_socket_get_address(ENetSocket, ENetAddress *);
    ENET_API int        enet_socket_listen(ENetSocket, int);
    ENET_API ENetSocket enet_socket_accept(ENetSocket, ENetAddress *);
    ENET_API int        enet_socket_connect(ENetSocket, const ENetAddress *);
    ENET_API int        enet_socket_send(ENetSocket, const ENetAddress *, const ENetBuffer *, size_t);
    ENET_API int        enet_socket_receive(ENetSocket, ENetAddress *, ENetBuffer *, size_t);
    ENET_API int        enet_socket_wait(ENetSocket, enet_uint32 *, enet_uint64);
    ENET_API int        enet_socket_set_option(ENetSocket, ENetSocketOption, int);
    ENET_API int        enet_socket_get_option(ENetSocket, ENetSocketOption, int *);
    ENET_API int        enet_socket_shutdown(ENetSocket, ENetSocketShutdown);
    ENET_API void       enet_socket_destroy(ENetSocket);
    ENET_API int        enet_socketset_select(ENetSocket, ENetSocketSet *, ENetSocketSet *, enet_uint32);

    /** Attempts to parse the printable form of the IP address in the parameter hostName
        and sets the host field in the address parameter if successful.
        @param address destination to store the parsed IP address
        @param hostName IP address to parse
        @retval 0 on success
        @retval < 0 on failure
        @returns the address of the given hostName in address on success
    */
    ENET_API int enet_address_set_host_ip_old(ENetAddress * address, const char * hostName);

    /** Attempts to resolve the host named by the parameter hostName and sets
        the host field in the address parameter if successful.
        @param address destination to store resolved address
        @param hostName host name to lookup
        @retval 0 on success
        @retval < 0 on failure
        @returns the address of the given hostName in address on success
    */
    ENET_API int enet_address_set_host_old(ENetAddress * address, const char * hostName);

    /** Gives the printable form of the IP address specified in the address parameter.
        @param address    address printed
        @param hostName   destination for name, must not be NULL
        @param nameLength maximum length of hostName.
        @returns the null-terminated name of the host in hostName on success
        @retval 0 on success
        @retval < 0 on failure
    */
    ENET_API int enet_address_get_host_ip_old(const ENetAddress * address, char * hostName, size_t nameLength);

    /** Attempts to do a reverse lookup of the host field in the address parameter.
        @param address    address used for reverse lookup
        @param hostName   destination for name, must not be NULL
        @param nameLength maximum length of hostName.
        @returns the null-terminated name of the host in hostName on success
        @retval 0 on success
        @retval < 0 on failure
    */
    ENET_API int enet_address_get_host_old(const ENetAddress * address, char * hostName, size_t nameLength);

    ENET_API int enet_address_set_host_ip_new(ENetAddress * address, const char * hostName);
    ENET_API int enet_address_set_host_new(ENetAddress * address, const char * hostName);
    ENET_API int enet_address_get_host_ip_new(const ENetAddress * address, char * hostName, size_t nameLength);
    ENET_API int enet_address_get_host_new(const ENetAddress * address, char * hostName, size_t nameLength);

#ifdef ENET_FEATURE_ADDRESS_MAPPING
#define enet_address_set_host_ip enet_address_set_host_ip_new
#define enet_address_set_host    enet_address_set_host_new
#define enet_address_get_host_ip enet_address_get_host_ip_new
#define enet_address_get_host    enet_address_get_host_new
#else
#define enet_address_set_host_ip enet_address_set_host_ip_old
#define enet_address_set_host    enet_address_set_host_old
#define enet_address_get_host_ip enet_address_get_host_ip_old
#define enet_address_get_host    enet_address_get_host_old
#endif

    ENET_API enet_uint32 enet_host_get_peers_count(ENetHost *);
    ENET_API enet_uint32 enet_host_get_packets_sent(ENetHost *);
    ENET_API enet_uint32 enet_host_get_packets_received(ENetHost *);
    ENET_API enet_uint32 enet_host_get_bytes_sent(ENetHost *);
    ENET_API enet_uint32 enet_host_get_bytes_received(ENetHost *);
    ENET_API enet_uint32 enet_host_get_received_data(ENetHost *, enet_uint8** data);
    ENET_API enet_uint32 enet_host_get_mtu(ENetHost *);

    ENET_API enet_uint32 enet_peer_get_id(ENetPeer *);
    ENET_API enet_uint32 enet_peer_get_ip(ENetPeer *, char * ip, size_t ipLength);
    ENET_API enet_uint16 enet_peer_get_port(ENetPeer *);
    ENET_API enet_uint32 enet_peer_get_rtt(ENetPeer *);
    ENET_API enet_uint64 enet_peer_get_packets_sent(ENetPeer *);
    ENET_API enet_uint32 enet_peer_get_packets_lost(ENetPeer *);
    ENET_API enet_uint64 enet_peer_get_bytes_sent(ENetPeer *);
    ENET_API enet_uint64 enet_peer_get_bytes_received(ENetPeer *);

    ENET_API ENetPeerState enet_peer_get_state(ENetPeer *);

    ENET_API void * enet_peer_get_data(ENetPeer *);
    ENET_API void   enet_peer_set_data(ENetPeer *, const void *);

    ENET_API void *      enet_packet_get_data(ENetPacket *);
    ENET_API enet_uint32 enet_packet_get_length(ENetPacket *);
    ENET_API void        enet_packet_set_free_callback(ENetPacket *, void *);

    ENET_API ENetPacket * enet_packet_create_offset(const void *, size_t, size_t, enet_uint32);
    ENET_API enet_uint32  enet_crc32(const ENetBuffer *, size_t);

    ENET_API ENetHost * enet_host_create(const ENetAddress *, size_t, size_t, enet_uint32, enet_uint32);
    ENET_API void       enet_host_destroy(ENetHost *);
    ENET_API ENetPeer * enet_host_connect(ENetHost *, const ENetAddress *, size_t, enet_uint32);
    ENET_API int        enet_host_check_events(ENetHost *, ENetEvent *);
    ENET_API int        enet_host_service(ENetHost *, ENetEvent *, enet_uint32);    
    ENET_API int        enet_host_send_raw(ENetHost *, const ENetAddress *, enet_uint8 *, size_t);
    ENET_API int        enet_host_send_raw_ex(ENetHost *host, const ENetAddress* address, enet_uint8* data, size_t skipBytes, size_t bytesToSend);
    ENET_API void       enet_host_set_intercept(ENetHost *, const ENetInterceptCallback);
    ENET_API void       enet_host_flush(ENetHost *);
    ENET_API void       enet_host_broadcast(ENetHost *, enet_uint8, ENetPacket *);    
    ENET_API void       enet_host_compress(ENetHost *, const ENetCompressor *);
    ENET_API void       enet_host_channel_limit(ENetHost *, size_t);
    ENET_API void       enet_host_bandwidth_limit(ENetHost *, enet_uint32, enet_uint32);
    extern   void       enet_host_bandwidth_throttle(ENetHost *);
    extern  enet_uint64 enet_host_random_seed(void);

    ENET_API int                 enet_peer_send(ENetPeer *, enet_uint8, ENetPacket *);
    ENET_API ENetPacket *        enet_peer_receive(ENetPeer *, enet_uint8 * channelID);
    ENET_API void                enet_peer_ping(ENetPeer *);
    ENET_API void                enet_peer_ping_interval(ENetPeer *, enet_uint32);
    ENET_API void                enet_peer_timeout(ENetPeer *, enet_uint32, enet_uint32, enet_uint32);
    ENET_API void                enet_peer_reset(ENetPeer *);
    ENET_API void                enet_peer_disconnect(ENetPeer *, enet_uint32);
    ENET_API void                enet_peer_disconnect_now(ENetPeer *, enet_uint32);
    ENET_API void                enet_peer_disconnect_later(ENetPeer *, enet_uint32);
    ENET_API void                enet_peer_throttle_configure(ENetPeer *, enet_uint32, enet_uint32, enet_uint32);
    extern int                   enet_peer_throttle(ENetPeer *, enet_uint32);
    extern void                  enet_peer_reset_queues(ENetPeer *);
    extern void                  enet_peer_setup_outgoing_command(ENetPeer *, ENetOutgoingCommand *);
    extern ENetOutgoingCommand * enet_peer_queue_outgoing_command(ENetPeer *, const ENetProtocol *, ENetPacket *, enet_uint32, enet_uint16);
    extern ENetIncomingCommand * enet_peer_queue_incoming_command(ENetPeer *, const ENetProtocol *, const void *, size_t, enet_uint32, enet_uint32);
    extern ENetAcknowledgement * enet_peer_queue_acknowledgement(ENetPeer *, const ENetProtocol *, enet_uint16);
    extern void                  enet_peer_dispatch_incoming_unreliable_commands(ENetPeer *, ENetChannel *);
    extern void                  enet_peer_dispatch_incoming_reliable_commands(ENetPeer *, ENetChannel *);
    extern void                  enet_peer_on_connect(ENetPeer *);
    extern void                  enet_peer_on_disconnect(ENetPeer *);

    extern size_t enet_protocol_command_size (enet_uint8);

#ifdef __cplusplus
}
#endif

#if defined(ENET_IMPLEMENTATION) && !defined(ENET_IMPLEMENTATION_DONE)
#define ENET_IMPLEMENTATION_DONE 1

#ifdef __cplusplus
extern "C" {
#endif

// =======================================================================//
// !
// ! Atomics
// !
// =======================================================================//

#if defined(_MSC_VER)

    #define ENET_AT_CASSERT_PRED(predicate) sizeof(char[2 * !!(predicate)-1])
    #define ENET_IS_SUPPORTED_ATOMIC(size) ENET_AT_CASSERT_PRED(size == 1 || size == 2 || size == 4 || size == 8)
    #define ENET_ATOMIC_SIZEOF(variable) (ENET_IS_SUPPORTED_ATOMIC(sizeof(*(variable))), sizeof(*(variable)))

    __inline int64_t enet_at_atomic_read(char *ptr, size_t size)
    {
        switch (size) {
            case 1:
                return _InterlockedExchangeAdd8((volatile char *)ptr, 0);
            case 2:
                return _InterlockedExchangeAdd16((volatile SHORT *)ptr, 0);
            case 4:
    #ifdef NOT_UNDERSCORED_INTERLOCKED_EXCHANGE
                return InterlockedExchangeAdd((volatile LONG *)ptr, 0);
    #else
                return _InterlockedExchangeAdd((volatile LONG *)ptr, 0);
    #endif
            case 8:
    #ifdef NOT_UNDERSCORED_INTERLOCKED_EXCHANGE
                return InterlockedExchangeAdd64((volatile LONGLONG *)ptr, 0);
    #else
                return _InterlockedExchangeAdd64((volatile LONGLONG *)ptr, 0);
    #endif
            default:
                return 0xbad13bad; /* never reached */
        }
    }

    __inline int64_t enet_at_atomic_write(char *ptr, int64_t value, size_t size)
    {
        switch (size) {
            case 1:
                return _InterlockedExchange8((volatile char *)ptr, (char)value);
            case 2:
                return _InterlockedExchange16((volatile SHORT *)ptr, (SHORT)value);
            case 4:
    #ifdef NOT_UNDERSCORED_INTERLOCKED_EXCHANGE
                return InterlockedExchange((volatile LONG *)ptr, (LONG)value);
    #else
                return _InterlockedExchange((volatile LONG *)ptr, (LONG)value);
    #endif
            case 8:
    #ifdef NOT_UNDERSCORED_INTERLOCKED_EXCHANGE
                return InterlockedExchange64((volatile LONGLONG *)ptr, (LONGLONG)value);
    #else
                return _InterlockedExchange64((volatile LONGLONG *)ptr, (LONGLONG)value);
    #endif
            default:
                return 0xbad13bad; /* never reached */
        }
    }

    __inline int64_t enet_at_atomic_cas(char *ptr, int64_t new_val, int64_t old_val, size_t size)
    {
        switch (size) {
            case 1:
                return _InterlockedCompareExchange8((volatile char *)ptr, (char)new_val, (char)old_val);
            case 2:
                return _InterlockedCompareExchange16((volatile SHORT *)ptr, (SHORT)new_val,
                                                     (SHORT)old_val);
            case 4:
    #ifdef NOT_UNDERSCORED_INTERLOCKED_EXCHANGE
                return InterlockedCompareExchange((volatile LONG *)ptr, (LONG)new_val, (LONG)old_val);
    #else
                return _InterlockedCompareExchange((volatile LONG *)ptr, (LONG)new_val, (LONG)old_val);
    #endif
            case 8:
    #ifdef NOT_UNDERSCORED_INTERLOCKED_EXCHANGE
                return InterlockedCompareExchange64((volatile LONGLONG *)ptr, (LONGLONG)new_val,
                                                    (LONGLONG)old_val);
    #else
                return _InterlockedCompareExchange64((volatile LONGLONG *)ptr, (LONGLONG)new_val,
                                                     (LONGLONG)old_val);
    #endif
            default:
                return 0xbad13bad; /* never reached */
        }
    }

    __inline int64_t enet_at_atomic_inc(char *ptr, int64_t delta, size_t data_size)
    {
        switch (data_size) {
            case 1:
                return _InterlockedExchangeAdd8((volatile char *)ptr, (char)delta);
            case 2:
                return _InterlockedExchangeAdd16((volatile SHORT *)ptr, (SHORT)delta);
            case 4:
    #ifdef NOT_UNDERSCORED_INTERLOCKED_EXCHANGE
                return InterlockedExchangeAdd((volatile LONG *)ptr, (LONG)delta);
    #else
                return _InterlockedExchangeAdd((volatile LONG *)ptr, (LONG)delta);
    #endif
            case 8:
    #ifdef NOT_UNDERSCORED_INTERLOCKED_EXCHANGE
                return InterlockedExchangeAdd64((volatile LONGLONG *)ptr, (LONGLONG)delta);
    #else
                return _InterlockedExchangeAdd64((volatile LONGLONG *)ptr, (LONGLONG)delta);
    #endif
            default:
                return 0xbad13bad; /* never reached */
        }
    }

    #define ENET_ATOMIC_READ(variable) enet_at_atomic_read((char *)(variable), ENET_ATOMIC_SIZEOF(variable))
    #define ENET_ATOMIC_WRITE(variable, new_val)                                                            \
        enet_at_atomic_write((char *)(variable), (int64_t)(new_val), ENET_ATOMIC_SIZEOF(variable))
    #define ENET_ATOMIC_CAS(variable, old_value, new_val)                                                   \
        enet_at_atomic_cas((char *)(variable), (int64_t)(new_val), (int64_t)(old_value),                    \
                      ENET_ATOMIC_SIZEOF(variable))
    #define ENET_ATOMIC_INC(variable) enet_at_atomic_inc((char *)(variable), 1, ENET_ATOMIC_SIZEOF(variable))
    #define ENET_ATOMIC_DEC(variable) enet_at_atomic_inc((char *)(variable), -1, ENET_ATOMIC_SIZEOF(variable))
    #define ENET_ATOMIC_INC_BY(variable, delta)                                                             \
        enet_at_atomic_inc((char *)(variable), (delta), ENET_ATOMIC_SIZEOF(variable))
    #define ENET_ATOMIC_DEC_BY(variable, delta)                                                             \
        enet_at_atomic_inc((char *)(variable), -(delta), ENET_ATOMIC_SIZEOF(variable))

#elif defined(__GNUC__) || defined(__clang__)

    #if defined(__clang__) || (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
    #define AT_HAVE_ATOMICS
    #endif

    /* We want to use __atomic built-ins if possible because the __sync primitives are
       deprecated, because the __atomic build-ins allow us to use ENET_ATOMIC_WRITE on
       uninitialized memory without running into undefined behavior, and because the
       __atomic versions generate more efficient code since we don't need to rely on
       CAS when we don't actually want it.

       Note that we use acquire-release memory order (like mutexes do). We could use
       sequentially consistent memory order but that has lower performance and is
       almost always unneeded. */
    #ifdef AT_HAVE_ATOMICS
        #define ENET_ATOMIC_READ(ptr) __atomic_load_n((ptr), __ATOMIC_ACQUIRE)
        #define ENET_ATOMIC_WRITE(ptr, value) __atomic_store_n((ptr), (value), __ATOMIC_RELEASE)

        #ifndef typeof
        #define typeof __typeof__
        #endif

        /* clang_analyzer doesn't know that CAS writes to memory so it complains about
           potentially lost data. Replace the code with the equivalent non-sync code. */
        #ifdef __clang_analyzer__

        #define ENET_ATOMIC_CAS(ptr, old_value, new_value)                                                      \
            ({                                                                                             \
                typeof(*(ptr)) ENET_ATOMIC_CAS_old_actual_ = (*(ptr));                                          \
                if (ATOMIC_CAS_old_actual_ == (old_value)) {                                               \
                    *(ptr) = new_value;                                                                    \
                }                                                                                          \
                ENET_ATOMIC_CAS_old_actual_;                                                                    \
            })

        #else

        /* Could use __auto_type instead of typeof but that shouldn't work in C++.
           The ({ }) syntax is a GCC extension called statement expression. It lets
           us return a value out of the macro.

           TODO We should return bool here instead of the old value to avoid the ABA
           problem. */
        #define ENET_ATOMIC_CAS(ptr, old_value, new_value)                                                      \
            ({                                                                                             \
                typeof(*(ptr)) ENET_ATOMIC_CAS_expected_ = (old_value);                                         \
                __atomic_compare_exchange_n((ptr), &ENET_ATOMIC_CAS_expected_, (new_value), false,              \
                                            __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);                           \
                ENET_ATOMIC_CAS_expected_;                                                                      \
            })

        #endif /* __clang_analyzer__ */

        #define ENET_ATOMIC_INC(ptr) __atomic_fetch_add((ptr), 1, __ATOMIC_ACQ_REL)
        #define ENET_ATOMIC_DEC(ptr) __atomic_fetch_sub((ptr), 1, __ATOMIC_ACQ_REL)
        #define ENET_ATOMIC_INC_BY(ptr, delta) __atomic_fetch_add((ptr), (delta), __ATOMIC_ACQ_REL)
        #define ENET_ATOMIC_DEC_BY(ptr, delta) __atomic_fetch_sub((ptr), (delta), __ATOMIC_ACQ_REL)

        #else

        #define ENET_ATOMIC_READ(variable) __sync_fetch_and_add(variable, 0)
        #define ENET_ATOMIC_WRITE(variable, new_val)                                                            \
            (void) __sync_val_compare_and_swap((variable), *(variable), (new_val))
        #define ENET_ATOMIC_CAS(variable, old_value, new_val)                                                   \
            __sync_val_compare_and_swap((variable), (old_value), (new_val))
        #define ENET_ATOMIC_INC(variable) __sync_fetch_and_add((variable), 1)
        #define ENET_ATOMIC_DEC(variable) __sync_fetch_and_sub((variable), 1)
        #define ENET_ATOMIC_INC_BY(variable, delta) __sync_fetch_and_add((variable), (delta), 1)
        #define ENET_ATOMIC_DEC_BY(variable, delta) __sync_fetch_and_sub((variable), (delta), 1)

    #endif /* AT_HAVE_ATOMICS */

    #undef AT_HAVE_ATOMICS

#endif /* defined(_MSC_VER) */

// =======================================================================//
// !
// ! Callbacks
// !
// =======================================================================//

    ENetCallbacks callbacks = { malloc, free, abort, enet_packet_create, enet_packet_destroy };

    int enet_initialize_with_callbacks(ENetVersion version, const ENetCallbacks *inits) {
        if (version < ENET_VERSION_CREATE(1, 3, 0)) {
            return -1;
        }

        if (inits->malloc != NULL || inits->free != NULL) {
            if (inits->malloc == NULL || inits->free == NULL) {
                return -1;
            }

            callbacks.malloc = inits->malloc;
            callbacks.free   = inits->free;
        }

        if (inits->no_memory != NULL) {
            callbacks.no_memory = inits->no_memory;
        }

        if (inits->packet_create != NULL || inits->packet_destroy != NULL) {
            if (inits->packet_create == NULL || inits->packet_destroy == NULL) {
                return -1;
            }

            callbacks.packet_create = inits->packet_create;
            callbacks.packet_destroy = inits->packet_destroy;
        }

        return enet_initialize();
    }

    ENetVersion enet_linked_version(void) {
        return ENET_VERSION;
    }

    void * enet_malloc(size_t size) {
        void *memory = callbacks.malloc(size);

        if (memory == NULL) {
            callbacks.no_memory();
        }

        return memory;
    }

    void enet_free(void *memory) {
        callbacks.free(memory);
    }

// =======================================================================//
// !
// ! List
// !
// =======================================================================//

    void enet_list_clear(ENetList *list) {
        list->sentinel.next     = &list->sentinel;
        list->sentinel.previous = &list->sentinel;
    }

    ENetListIterator enet_list_insert(ENetListIterator position, void *data) {
        ENetListIterator result = (ENetListIterator)data;

        result->previous = position->previous;
        result->next     = position;

        result->previous->next = result;
        position->previous     = result;

        return result;
    }

    void *enet_list_remove(ENetListIterator position) {
        position->previous->next = position->next;
        position->next->previous = position->previous;

        return position;
    }

    ENetListIterator enet_list_move(ENetListIterator position, void *dataFirst, void *dataLast) {
        ENetListIterator first = (ENetListIterator)dataFirst;
        ENetListIterator last  = (ENetListIterator)dataLast;

        first->previous->next = last->next;
        last->next->previous  = first->previous;

        first->previous = position->previous;
        last->next      = position;

        first->previous->next = first;
        position->previous    = last;

        return first;
    }

    size_t enet_list_size(ENetList *list) {
        size_t size = 0;
        ENetListIterator position;

        for (position = enet_list_begin(list); position != enet_list_end(list); position = enet_list_next(position)) {
            ++size;
        }

        return size;
    }

// =======================================================================//
// !
// ! Packet
// !
// =======================================================================//

    /**
     * Creates a packet that may be sent to a peer.
     * @param data         initial contents of the packet's data; the packet's data will remain uninitialized if data is NULL.
     * @param dataLength   size of the data allocated for this packet
     * @param flags        flags for this packet as described for the ENetPacket structure.
     * @returns the packet on success, NULL on failure
     */
    ENetPacket *enet_packet_create(const void *data, size_t dataLength, enet_uint32 flags) {
        ENetPacket *packet;
        if (flags & ENET_PACKET_FLAG_NO_ALLOCATE) {
            packet = (ENetPacket *)enet_malloc(sizeof (ENetPacket));
            if (packet == NULL) {
                return NULL;
            }

            packet->data = (enet_uint8 *)data;
        }
        else {
            packet = (ENetPacket *)enet_malloc(sizeof (ENetPacket) + dataLength);
            if (packet == NULL) {
                return NULL;
            }

            packet->data = (enet_uint8 *)packet + sizeof(ENetPacket);

            if (data != NULL) {
                memcpy(packet->data, data, dataLength);
            }
        }

        packet->referenceCount = 0;
        packet->flags        = flags;
        packet->dataLength   = dataLength;
        packet->freeCallback = NULL;
        packet->userData     = NULL;

        return packet;
    }

    ENetPacket *enet_packet_create_offset(const void *data, size_t dataLength, size_t dataOffset, enet_uint32 flags) {
        ENetPacket *packet;
        if (flags & ENET_PACKET_FLAG_NO_ALLOCATE) {
            packet = (ENetPacket *)enet_malloc(sizeof (ENetPacket));
            if (packet == NULL) {
                return NULL;
            }

            packet->data = (enet_uint8 *)data;
        }
        else {
            packet = (ENetPacket *)enet_malloc(sizeof (ENetPacket) + dataLength + dataOffset);
            if (packet == NULL) {
                return NULL;
            }

            packet->data = (enet_uint8 *)packet + sizeof(ENetPacket);

            if (data != NULL) {
                memcpy(packet->data + dataOffset, data, dataLength);
            }
        }

        packet->referenceCount = 0;
        packet->flags        = flags;
        packet->dataLength   = dataLength + dataOffset;
        packet->freeCallback = NULL;
        packet->userData     = NULL;

        return packet;
    }

    ENetPacket *enet_packet_copy(ENetPacket *packet) {
        return enet_packet_create(packet->data, packet->dataLength, packet->flags);
    }

    /**
     * Destroys the packet and deallocates its data.
     * @param packet packet to be destroyed
     */
    void enet_packet_destroy(ENetPacket *packet) {
        if (packet == NULL) {
            return;
        }

        if (packet->freeCallback != NULL) {
            (*packet->freeCallback)((void *)packet);
        }

        enet_free(packet);
    }

    static int initializedCRC32 = 0;
    static enet_uint32 crcTable[256];

    static enet_uint32 reflect_crc(int val, int bits) {
        int result = 0, bit;

        for (bit = 0; bit < bits; bit++) {
            if (val & 1) { result |= 1 << (bits - 1 - bit); }
            val >>= 1;
        }

        return result;
    }

    static void initialize_crc32(void) {
        int byte;

        for (byte = 0; byte < 256; ++byte) {
            enet_uint32 crc = reflect_crc(byte, 8) << 24;
            int offset;

            for (offset = 0; offset < 8; ++offset) {
                if (crc & 0x80000000) {
                    crc = (crc << 1) ^ 0x04c11db7;
                } else {
                    crc <<= 1;
                }
            }

            crcTable[byte] = reflect_crc(crc, 32);
        }

        initializedCRC32 = 1;
    }

    enet_uint32 enet_crc32(const ENetBuffer *buffers, size_t bufferCount) {
        enet_uint32 crc = 0xFFFFFFFF;

        if (!initializedCRC32) { initialize_crc32(); }

        while (bufferCount-- > 0) {
            const enet_uint8 *data = (const enet_uint8 *)buffers->data;
            const enet_uint8 *dataEnd = &data[buffers->dataLength];

            while (data < dataEnd) {
                crc = (crc >> 8) ^ crcTable[(crc & 0xFF) ^ *data++];
            }

            ++buffers;
        }

        return ENET_HOST_TO_NET_32(~crc);
    }

// =======================================================================//
// !
// ! Protocol
// !
// =======================================================================//

    static size_t commandSizes[ENET_PROTOCOL_COMMAND_COUNT] = {
        0,
        sizeof(ENetProtocolAcknowledge),
        sizeof(ENetProtocolConnect),
        sizeof(ENetProtocolVerifyConnect),
        sizeof(ENetProtocolDisconnect),
        sizeof(ENetProtocolPing),
        sizeof(ENetProtocolSendReliable),
        sizeof(ENetProtocolSendUnreliable),
        sizeof(ENetProtocolSendFragment),
        sizeof(ENetProtocolSendUnsequenced),
        sizeof(ENetProtocolBandwidthLimit),
        sizeof(ENetProtocolThrottleConfigure),
        sizeof(ENetProtocolSendFragment)
    };

    size_t enet_protocol_command_size(enet_uint8 commandNumber) {
        return commandSizes[commandNumber & ENET_PROTOCOL_COMMAND_MASK];
    }

    static void enet_protocol_change_state(ENetHost *host, ENetPeer *peer, ENetPeerState state) {
        ENET_UNUSED(host)

        if (state == ENET_PEER_STATE_CONNECTED || state == ENET_PEER_STATE_DISCONNECT_LATER) {
            enet_peer_on_connect(peer);
        } else {
            enet_peer_on_disconnect(peer);
        }

        peer->state = state;
    }

    static void enet_protocol_dispatch_state(ENetHost *host, ENetPeer *peer, ENetPeerState state) {
        enet_protocol_change_state(host, peer, state);

        if (!peer->needsDispatch) {
            enet_list_insert(enet_list_end(&host->dispatchQueue), &peer->dispatchList);
            peer->needsDispatch = 1;
        }
    }

    static int enet_protocol_dispatch_incoming_commands(ENetHost *host, ENetEvent *event) {
        while (!enet_list_empty(&host->dispatchQueue)) {
            ENetPeer *peer = (ENetPeer *) enet_list_remove(enet_list_begin(&host->dispatchQueue));
            peer->needsDispatch = 0;

            switch (peer->state) {
                case ENET_PEER_STATE_CONNECTION_PENDING:
                case ENET_PEER_STATE_CONNECTION_SUCCEEDED:
                    enet_protocol_change_state(host, peer, ENET_PEER_STATE_CONNECTED);

                    event->type = ENET_EVENT_TYPE_CONNECT;
                    event->peer = peer;
                    event->data = peer->eventData;

                    return 1;

                case ENET_PEER_STATE_ZOMBIE:
                    host->recalculateBandwidthLimits = 1;

                    event->type = ENET_EVENT_TYPE_DISCONNECT;
                    event->peer = peer;
                    event->data = peer->eventData;

                    enet_peer_reset(peer);

                    return 1;

                case ENET_PEER_STATE_CONNECTED:
                    if (enet_list_empty(&peer->dispatchedCommands)) {
                        continue;
                    }

                    event->packet = enet_peer_receive(peer, &event->channelID);
                    if (event->packet == NULL) {
                        continue;
                    }

                    event->type = ENET_EVENT_TYPE_RECEIVE;
                    event->peer = peer;

                    if (!enet_list_empty(&peer->dispatchedCommands)) {
                        peer->needsDispatch = 1;
                        enet_list_insert(enet_list_end(&host->dispatchQueue), &peer->dispatchList);
                    }

                    return 1;

                default:
                    break;
            }
        }

        return 0;
    } /* enet_protocol_dispatch_incoming_commands */

    static void enet_protocol_notify_connect(ENetHost *host, ENetPeer *peer, ENetEvent *event) {
        host->recalculateBandwidthLimits = 1;

        if (event != NULL) {
            enet_protocol_change_state(host, peer, ENET_PEER_STATE_CONNECTED);

            peer->totalDataSent     = 0;
            peer->totalDataReceived = 0;
            peer->totalPacketsSent  = 0;
            peer->totalPacketsLost  = 0;

            event->type = ENET_EVENT_TYPE_CONNECT;
            event->peer = peer;
            event->data = peer->eventData;
        } else {
            enet_protocol_dispatch_state(host, peer, peer->state == ENET_PEER_STATE_CONNECTING ? ENET_PEER_STATE_CONNECTION_SUCCEEDED : ENET_PEER_STATE_CONNECTION_PENDING);
        }
    }

    static void enet_protocol_notify_disconnect(ENetHost *host, ENetPeer *peer, ENetEvent *event) {
        if (peer->state >= ENET_PEER_STATE_CONNECTION_PENDING) {
            host->recalculateBandwidthLimits = 1;
        }

        if (peer->state != ENET_PEER_STATE_CONNECTING && peer->state < ENET_PEER_STATE_CONNECTION_SUCCEEDED) {
            enet_peer_reset(peer);
        } else if (event != NULL) {
            event->type = ENET_EVENT_TYPE_DISCONNECT;
            event->peer = peer;
            event->data = 0;

            enet_peer_reset(peer);
        } else {
            peer->eventData = 0;
            enet_protocol_dispatch_state(host, peer, ENET_PEER_STATE_ZOMBIE);
        }
    }

    static void enet_protocol_notify_disconnect_timeout (ENetHost * host, ENetPeer * peer, ENetEvent * event) {
        if (peer->state >= ENET_PEER_STATE_CONNECTION_PENDING) {
           host->recalculateBandwidthLimits = 1;
        }

        if (peer->state != ENET_PEER_STATE_CONNECTING && peer->state < ENET_PEER_STATE_CONNECTION_SUCCEEDED) {
            enet_peer_reset (peer);
        }
        else if (event != NULL) {
            event->type = ENET_EVENT_TYPE_DISCONNECT_TIMEOUT;
            event->peer = peer;
            event->data = 0;

            enet_peer_reset(peer);
        }
        else {
            peer->eventData = 0;
            enet_protocol_dispatch_state(host, peer, ENET_PEER_STATE_ZOMBIE);
        }
    }

    static void enet_protocol_remove_sent_unreliable_commands(ENetPeer *peer) {
        ENetOutgoingCommand *outgoingCommand;

        while (!enet_list_empty(&peer->sentUnreliableCommands)) {
            outgoingCommand = (ENetOutgoingCommand *) enet_list_front(&peer->sentUnreliableCommands);
            enet_list_remove(&outgoingCommand->outgoingCommandList);

            if (outgoingCommand->packet != NULL) {
                --outgoingCommand->packet->referenceCount;

                if (outgoingCommand->packet->referenceCount == 0) {
                    outgoingCommand->packet->flags |= ENET_PACKET_FLAG_SENT;
                    callbacks.packet_destroy(outgoingCommand->packet);
                }
            }

            enet_free(outgoingCommand);
        }
    }

    static ENetProtocolCommand enet_protocol_remove_sent_reliable_command(ENetPeer *peer, enet_uint16 reliableSequenceNumber, enet_uint8 channelID) {
        ENetOutgoingCommand *outgoingCommand = NULL;
        ENetListIterator currentCommand;
        ENetProtocolCommand commandNumber;
        int wasSent = 1;

        for (currentCommand = enet_list_begin(&peer->sentReliableCommands);
            currentCommand != enet_list_end(&peer->sentReliableCommands);
            currentCommand = enet_list_next(currentCommand)
        ) {
            outgoingCommand = (ENetOutgoingCommand *) currentCommand;

            if (outgoingCommand->reliableSequenceNumber == reliableSequenceNumber && outgoingCommand->command.header.channelID == channelID) {
                break;
            }
        }

        if (currentCommand == enet_list_end(&peer->sentReliableCommands)) {
            for (currentCommand = enet_list_begin(&peer->outgoingReliableCommands);
                currentCommand != enet_list_end(&peer->outgoingReliableCommands);
                currentCommand = enet_list_next(currentCommand)
            ) {
                outgoingCommand = (ENetOutgoingCommand *) currentCommand;

                if (outgoingCommand->sendAttempts < 1) { return ENET_PROTOCOL_COMMAND_NONE; }
                if (outgoingCommand->reliableSequenceNumber == reliableSequenceNumber && outgoingCommand->command.header.channelID == channelID) {
                    break;
                }
            }

            if (currentCommand == enet_list_end(&peer->outgoingReliableCommands)) {
                return ENET_PROTOCOL_COMMAND_NONE;
            }

            wasSent = 0;
        }

        if (outgoingCommand == NULL) {
            return ENET_PROTOCOL_COMMAND_NONE;
        }

        if (channelID < peer->channelCount) {
            ENetChannel *channel       = &peer->channels[channelID];
            enet_uint16 reliableWindow = reliableSequenceNumber / ENET_PEER_RELIABLE_WINDOW_SIZE;
            if (channel->reliableWindows[reliableWindow] > 0) {
                --channel->reliableWindows[reliableWindow];
                if (!channel->reliableWindows[reliableWindow]) {
                    channel->usedReliableWindows &= ~(1 << reliableWindow);
                }
            }
        }

        commandNumber = (ENetProtocolCommand) (outgoingCommand->command.header.command & ENET_PROTOCOL_COMMAND_MASK);
        enet_list_remove(&outgoingCommand->outgoingCommandList);

        if (outgoingCommand->packet != NULL) {
            if (wasSent) {
                peer->reliableDataInTransit -= outgoingCommand->fragmentLength;
            }

            --outgoingCommand->packet->referenceCount;

            if (outgoingCommand->packet->referenceCount == 0) {
                outgoingCommand->packet->flags |= ENET_PACKET_FLAG_SENT;
                callbacks.packet_destroy(outgoingCommand->packet);
            }
        }

        enet_free(outgoingCommand);

        if (enet_list_empty(&peer->sentReliableCommands)) {
            return commandNumber;
        }

        outgoingCommand = (ENetOutgoingCommand *) enet_list_front(&peer->sentReliableCommands);
        peer->nextTimeout = outgoingCommand->sentTime + outgoingCommand->roundTripTimeout;

        return commandNumber;
    } /* enet_protocol_remove_sent_reliable_command */

    static ENetPeer * enet_protocol_handle_connect(ENetHost *host, ENetProtocolHeader *header, ENetProtocol *command) {
        ENET_UNUSED(header)

        enet_uint8 incomingSessionID, outgoingSessionID;
        enet_uint32 mtu, windowSize;
        ENetChannel *channel;
        size_t channelCount, duplicatePeers = 0;
        ENetPeer *currentPeer, *peer = NULL;
        ENetProtocol verifyCommand;

        channelCount = ENET_NET_TO_HOST_32(command->connect.channelCount);

        if (channelCount < ENET_PROTOCOL_MINIMUM_CHANNEL_COUNT || channelCount > ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT) {
            return NULL;
        }

        for (currentPeer = host->peers; currentPeer < &host->peers[host->peerCount]; ++currentPeer) {
            if (currentPeer->state == ENET_PEER_STATE_DISCONNECTED) {
                if (peer == NULL) {
                    peer = currentPeer;
                }
            } else if (currentPeer->state != ENET_PEER_STATE_CONNECTING && in6_equal(currentPeer->address.host, host->receivedAddress.host)) {
                if (currentPeer->address.port == host->receivedAddress.port && currentPeer->connectID == command->connect.connectID) {
                    return NULL;
                }

                ++duplicatePeers;
            }
        }

        if (peer == NULL || duplicatePeers >= host->duplicatePeers) {
            return NULL;
        }

        if (channelCount > host->channelLimit) {
            channelCount = host->channelLimit;
        }
        peer->channels = (ENetChannel *) enet_malloc(channelCount * sizeof(ENetChannel));
        if (peer->channels == NULL) {
            return NULL;
        }
        peer->channelCount               = channelCount;
        peer->state                      = ENET_PEER_STATE_ACKNOWLEDGING_CONNECT;
        peer->connectID                  = command->connect.connectID;
        peer->address                    = host->receivedAddress;
        peer->outgoingPeerID             = ENET_NET_TO_HOST_16(command->connect.outgoingPeerID);
        peer->incomingBandwidth          = ENET_NET_TO_HOST_32(command->connect.incomingBandwidth);
        peer->outgoingBandwidth          = ENET_NET_TO_HOST_32(command->connect.outgoingBandwidth);
        peer->packetThrottleInterval     = ENET_NET_TO_HOST_32(command->connect.packetThrottleInterval);
        peer->packetThrottleAcceleration = ENET_NET_TO_HOST_32(command->connect.packetThrottleAcceleration);
        peer->packetThrottleDeceleration = ENET_NET_TO_HOST_32(command->connect.packetThrottleDeceleration);
        peer->eventData                  = ENET_NET_TO_HOST_32(command->connect.data);

        incomingSessionID = command->connect.incomingSessionID == 0xFF ? peer->outgoingSessionID : command->connect.incomingSessionID;
        incomingSessionID = (incomingSessionID + 1) & (ENET_PROTOCOL_HEADER_SESSION_MASK >> ENET_PROTOCOL_HEADER_SESSION_SHIFT);
        if (incomingSessionID == peer->outgoingSessionID) {
            incomingSessionID = (incomingSessionID + 1)
              & (ENET_PROTOCOL_HEADER_SESSION_MASK >> ENET_PROTOCOL_HEADER_SESSION_SHIFT);
        }
        peer->outgoingSessionID = incomingSessionID;

        outgoingSessionID = command->connect.outgoingSessionID == 0xFF ? peer->incomingSessionID : command->connect.outgoingSessionID;
        outgoingSessionID = (outgoingSessionID + 1) & (ENET_PROTOCOL_HEADER_SESSION_MASK >> ENET_PROTOCOL_HEADER_SESSION_SHIFT);
        if (outgoingSessionID == peer->incomingSessionID) {
            outgoingSessionID = (outgoingSessionID + 1)
              & (ENET_PROTOCOL_HEADER_SESSION_MASK >> ENET_PROTOCOL_HEADER_SESSION_SHIFT);
        }
        peer->incomingSessionID = outgoingSessionID;

        for (channel = peer->channels; channel < &peer->channels[channelCount]; ++channel) {
            channel->outgoingReliableSequenceNumber   = 0;
            channel->outgoingUnreliableSequenceNumber = 0;
            channel->incomingReliableSequenceNumber   = 0;
            channel->incomingUnreliableSequenceNumber = 0;

            enet_list_clear(&channel->incomingReliableCommands);
            enet_list_clear(&channel->incomingUnreliableCommands);

            channel->usedReliableWindows = 0;
            memset(channel->reliableWindows, 0, sizeof(channel->reliableWindows));
        }

        mtu = ENET_NET_TO_HOST_32(command->connect.mtu);

        if (mtu < ENET_PROTOCOL_MINIMUM_MTU) {
            mtu = ENET_PROTOCOL_MINIMUM_MTU;
        } else if (mtu > ENET_PROTOCOL_MAXIMUM_MTU) {
            mtu = ENET_PROTOCOL_MAXIMUM_MTU;
        }

        peer->mtu = mtu;

        if (host->outgoingBandwidth == 0 && peer->incomingBandwidth == 0) {
            peer->windowSize = ENET_PROTOCOL_MAXIMUM_WINDOW_SIZE;
        } else if (host->outgoingBandwidth == 0 || peer->incomingBandwidth == 0) {
            peer->windowSize = (ENET_MAX(host->outgoingBandwidth, peer->incomingBandwidth) / ENET_PEER_WINDOW_SIZE_SCALE) * ENET_PROTOCOL_MINIMUM_WINDOW_SIZE;
        } else {
            peer->windowSize = (ENET_MIN(host->outgoingBandwidth, peer->incomingBandwidth) / ENET_PEER_WINDOW_SIZE_SCALE) * ENET_PROTOCOL_MINIMUM_WINDOW_SIZE;
        }

        if (peer->windowSize < ENET_PROTOCOL_MINIMUM_WINDOW_SIZE) {
            peer->windowSize = ENET_PROTOCOL_MINIMUM_WINDOW_SIZE;
        } else if (peer->windowSize > ENET_PROTOCOL_MAXIMUM_WINDOW_SIZE) {
            peer->windowSize = ENET_PROTOCOL_MAXIMUM_WINDOW_SIZE;
        }

        if (host->incomingBandwidth == 0) {
            windowSize = ENET_PROTOCOL_MAXIMUM_WINDOW_SIZE;
        } else {
            windowSize = (host->incomingBandwidth / ENET_PEER_WINDOW_SIZE_SCALE) * ENET_PROTOCOL_MINIMUM_WINDOW_SIZE;
        }

        if (windowSize > ENET_NET_TO_HOST_32(command->connect.windowSize)) {
            windowSize = ENET_NET_TO_HOST_32(command->connect.windowSize);
        }

        if (windowSize < ENET_PROTOCOL_MINIMUM_WINDOW_SIZE) {
            windowSize = ENET_PROTOCOL_MINIMUM_WINDOW_SIZE;
        } else if (windowSize > ENET_PROTOCOL_MAXIMUM_WINDOW_SIZE) {
            windowSize = ENET_PROTOCOL_MAXIMUM_WINDOW_SIZE;
        }

        verifyCommand.header.command                            = ENET_PROTOCOL_COMMAND_VERIFY_CONNECT | ENET_PROTOCOL_COMMAND_FLAG_ACKNOWLEDGE;
        verifyCommand.header.channelID                          = 0xFF;
        verifyCommand.verifyConnect.outgoingPeerID              = ENET_HOST_TO_NET_16(peer->incomingPeerID);
        verifyCommand.verifyConnect.incomingSessionID           = incomingSessionID;
        verifyCommand.verifyConnect.outgoingSessionID           = outgoingSessionID;
        verifyCommand.verifyConnect.mtu                         = ENET_HOST_TO_NET_32(peer->mtu);
        verifyCommand.verifyConnect.windowSize                  = ENET_HOST_TO_NET_32(windowSize);
        verifyCommand.verifyConnect.channelCount                = ENET_HOST_TO_NET_32(channelCount);
        verifyCommand.verifyConnect.incomingBandwidth           = ENET_HOST_TO_NET_32(host->incomingBandwidth);
        verifyCommand.verifyConnect.outgoingBandwidth           = ENET_HOST_TO_NET_32(host->outgoingBandwidth);
        verifyCommand.verifyConnect.packetThrottleInterval      = ENET_HOST_TO_NET_32(peer->packetThrottleInterval);
        verifyCommand.verifyConnect.packetThrottleAcceleration  = ENET_HOST_TO_NET_32(peer->packetThrottleAcceleration);
        verifyCommand.verifyConnect.packetThrottleDeceleration  = ENET_HOST_TO_NET_32(peer->packetThrottleDeceleration);
        verifyCommand.verifyConnect.connectID                   = peer->connectID;

        enet_peer_queue_outgoing_command(peer, &verifyCommand, NULL, 0, 0);
        return peer;
    } /* enet_protocol_handle_connect */

    static int enet_protocol_handle_send_reliable(ENetHost *host, ENetPeer *peer, const ENetProtocol *command, enet_uint8 **currentData) {
        size_t dataLength;

        if (command->header.channelID >= peer->channelCount || (peer->state != ENET_PEER_STATE_CONNECTED && peer->state != ENET_PEER_STATE_DISCONNECT_LATER)) {
            return -1;
        }

        dataLength    = ENET_NET_TO_HOST_16(command->sendReliable.dataLength);
        *currentData += dataLength;

        if (dataLength > host->maximumPacketSize || *currentData < host->receivedData || *currentData > &host->receivedData[host->receivedDataLength]) {
            return -1;
        }

        if (enet_peer_queue_incoming_command(peer, command, (const enet_uint8 *) command + sizeof(ENetProtocolSendReliable), dataLength, ENET_PACKET_FLAG_RELIABLE, 0) == NULL) {
            return -1;
        }

        return 0;
    }

    static int enet_protocol_handle_send_unsequenced(ENetHost *host, ENetPeer *peer, const ENetProtocol *command, enet_uint8 **currentData) {
        enet_uint32 unsequencedGroup, index;
        size_t dataLength;

        if (command->header.channelID >= peer->channelCount || (peer->state != ENET_PEER_STATE_CONNECTED && peer->state != ENET_PEER_STATE_DISCONNECT_LATER)) {
            return -1;
        }

        dataLength    = ENET_NET_TO_HOST_16(command->sendUnsequenced.dataLength);
        *currentData += dataLength;
        if (dataLength > host->maximumPacketSize || *currentData < host->receivedData || *currentData > &host->receivedData[host->receivedDataLength]) {
            return -1;
        }

        unsequencedGroup = ENET_NET_TO_HOST_16(command->sendUnsequenced.unsequencedGroup);
        index = unsequencedGroup % ENET_PEER_UNSEQUENCED_WINDOW_SIZE;

        if (unsequencedGroup < peer->incomingUnsequencedGroup) {
            unsequencedGroup += 0x10000;
        }

        if (unsequencedGroup >= (enet_uint32) peer->incomingUnsequencedGroup + ENET_PEER_FREE_UNSEQUENCED_WINDOWS * ENET_PEER_UNSEQUENCED_WINDOW_SIZE) {
            return 0;
        }

        unsequencedGroup &= 0xFFFF;

        if (unsequencedGroup - index != peer->incomingUnsequencedGroup) {
            peer->incomingUnsequencedGroup = unsequencedGroup - index;
            memset(peer->unsequencedWindow, 0, sizeof(peer->unsequencedWindow));
        } else if (peer->unsequencedWindow[index / 32] & (1 << (index % 32))) {
            return 0;
        }

        if (enet_peer_queue_incoming_command(peer, command, (const enet_uint8 *) command + sizeof(ENetProtocolSendUnsequenced), dataLength, ENET_PACKET_FLAG_UNSEQUENCED,0) == NULL) {
            return -1;
        }

        peer->unsequencedWindow[index / 32] |= 1 << (index % 32);

        return 0;
    } /* enet_protocol_handle_send_unsequenced */

    static int enet_protocol_handle_send_unreliable(ENetHost *host, ENetPeer *peer, const ENetProtocol *command,
      enet_uint8 **currentData) {
        size_t dataLength;

        if (command->header.channelID >= peer->channelCount ||
          (peer->state != ENET_PEER_STATE_CONNECTED && peer->state != ENET_PEER_STATE_DISCONNECT_LATER))
        {
            return -1;
        }

        dataLength    = ENET_NET_TO_HOST_16(command->sendUnreliable.dataLength);
        *currentData += dataLength;
        if (dataLength > host->maximumPacketSize || *currentData < host->receivedData || *currentData > &host->receivedData[host->receivedDataLength]) {
            return -1;
        }

        if (enet_peer_queue_incoming_command(peer, command, (const enet_uint8 *) command + sizeof(ENetProtocolSendUnreliable), dataLength, 0, 0) == NULL) {
            return -1;
        }

        return 0;
    }

    static int enet_protocol_handle_send_fragment(ENetHost *host, ENetPeer *peer, const ENetProtocol *command, enet_uint8 **currentData) {
        enet_uint32 fragmentNumber, fragmentCount, fragmentOffset, fragmentLength, startSequenceNumber, totalLength;
        ENetChannel *channel;
        enet_uint16 startWindow, currentWindow;
        ENetListIterator currentCommand;
        ENetIncomingCommand *startCommand = NULL;

        if (command->header.channelID >= peer->channelCount || (peer->state != ENET_PEER_STATE_CONNECTED && peer->state != ENET_PEER_STATE_DISCONNECT_LATER)) {
            return -1;
        }

        fragmentLength = ENET_NET_TO_HOST_16(command->sendFragment.dataLength);
        *currentData  += fragmentLength;
        if (fragmentLength > host->maximumPacketSize || *currentData < host->receivedData || *currentData > &host->receivedData[host->receivedDataLength]) {
            return -1;
        }

        channel = &peer->channels[command->header.channelID];
        startSequenceNumber = ENET_NET_TO_HOST_16(command->sendFragment.startSequenceNumber);
        startWindow         = startSequenceNumber / ENET_PEER_RELIABLE_WINDOW_SIZE;
        currentWindow       = channel->incomingReliableSequenceNumber / ENET_PEER_RELIABLE_WINDOW_SIZE;

        if (startSequenceNumber < channel->incomingReliableSequenceNumber) {
            startWindow += ENET_PEER_RELIABLE_WINDOWS;
        }

        if (startWindow < currentWindow || startWindow >= currentWindow + ENET_PEER_FREE_RELIABLE_WINDOWS - 1) {
            return 0;
        }

        fragmentNumber = ENET_NET_TO_HOST_32(command->sendFragment.fragmentNumber);
        fragmentCount  = ENET_NET_TO_HOST_32(command->sendFragment.fragmentCount);
        fragmentOffset = ENET_NET_TO_HOST_32(command->sendFragment.fragmentOffset);
        totalLength    = ENET_NET_TO_HOST_32(command->sendFragment.totalLength);

        if (fragmentCount > ENET_PROTOCOL_MAXIMUM_FRAGMENT_COUNT ||
            fragmentNumber >= fragmentCount ||
            totalLength > host->maximumPacketSize ||
            fragmentOffset >= totalLength ||
            fragmentLength > totalLength - fragmentOffset
        ) {
            return -1;
        }

        for (currentCommand = enet_list_previous(enet_list_end(&channel->incomingReliableCommands));
            currentCommand != enet_list_end(&channel->incomingReliableCommands);
            currentCommand = enet_list_previous(currentCommand)
        ) {
            ENetIncomingCommand *incomingCommand = (ENetIncomingCommand *) currentCommand;

            if (startSequenceNumber >= channel->incomingReliableSequenceNumber) {
                if (incomingCommand->reliableSequenceNumber < channel->incomingReliableSequenceNumber) {
                    continue;
                }
            } else if (incomingCommand->reliableSequenceNumber >= channel->incomingReliableSequenceNumber) {
                break;
            }

            if (incomingCommand->reliableSequenceNumber <= startSequenceNumber) {
                if (incomingCommand->reliableSequenceNumber < startSequenceNumber) {
                    break;
                }

                if ((incomingCommand->command.header.command & ENET_PROTOCOL_COMMAND_MASK) !=
                    ENET_PROTOCOL_COMMAND_SEND_FRAGMENT ||
                    totalLength != incomingCommand->packet->dataLength ||
                    fragmentCount != incomingCommand->fragmentCount
                ) {
                    return -1;
                }

                startCommand = incomingCommand;
                break;
            }
        }

        if (startCommand == NULL) {
            ENetProtocol hostCommand = *command;
            hostCommand.header.reliableSequenceNumber = startSequenceNumber;
            startCommand = enet_peer_queue_incoming_command(peer, &hostCommand, NULL, totalLength, ENET_PACKET_FLAG_RELIABLE, fragmentCount);
            if (startCommand == NULL) {
                return -1;
            }
        }

        if ((startCommand->fragments[fragmentNumber / 32] & (1 << (fragmentNumber % 32))) == 0) {
            --startCommand->fragmentsRemaining;
            startCommand->fragments[fragmentNumber / 32] |= (1 << (fragmentNumber % 32));

            if (fragmentOffset + fragmentLength > startCommand->packet->dataLength) {
                fragmentLength = startCommand->packet->dataLength - fragmentOffset;
            }

            memcpy(startCommand->packet->data + fragmentOffset, (enet_uint8 *) command + sizeof(ENetProtocolSendFragment), fragmentLength);

            if (startCommand->fragmentsRemaining <= 0) {
                enet_peer_dispatch_incoming_reliable_commands(peer, channel);
            }
        }

        return 0;
    } /* enet_protocol_handle_send_fragment */

    static int enet_protocol_handle_send_unreliable_fragment(ENetHost *host, ENetPeer *peer, const ENetProtocol *command, enet_uint8 **currentData) {
        enet_uint32 fragmentNumber, fragmentCount, fragmentOffset, fragmentLength, reliableSequenceNumber, startSequenceNumber, totalLength;
        enet_uint16 reliableWindow, currentWindow;
        ENetChannel *channel;
        ENetListIterator currentCommand;
        ENetIncomingCommand *startCommand = NULL;

        if (command->header.channelID >= peer->channelCount || (peer->state != ENET_PEER_STATE_CONNECTED && peer->state != ENET_PEER_STATE_DISCONNECT_LATER)) {
            return -1;
        }

        fragmentLength = ENET_NET_TO_HOST_16(command->sendFragment.dataLength);
        *currentData  += fragmentLength;
        if (fragmentLength > host->maximumPacketSize || *currentData < host->receivedData || *currentData > &host->receivedData[host->receivedDataLength]) {
            return -1;
        }

        channel = &peer->channels[command->header.channelID];
        reliableSequenceNumber = command->header.reliableSequenceNumber;
        startSequenceNumber    = ENET_NET_TO_HOST_16(command->sendFragment.startSequenceNumber);

        reliableWindow = reliableSequenceNumber / ENET_PEER_RELIABLE_WINDOW_SIZE;
        currentWindow  = channel->incomingReliableSequenceNumber / ENET_PEER_RELIABLE_WINDOW_SIZE;

        if (reliableSequenceNumber < channel->incomingReliableSequenceNumber) {
            reliableWindow += ENET_PEER_RELIABLE_WINDOWS;
        }

        if (reliableWindow < currentWindow || reliableWindow >= currentWindow + ENET_PEER_FREE_RELIABLE_WINDOWS - 1) {
            return 0;
        }

        if (reliableSequenceNumber == channel->incomingReliableSequenceNumber && startSequenceNumber <= channel->incomingUnreliableSequenceNumber) {
            return 0;
        }

        fragmentNumber = ENET_NET_TO_HOST_32(command->sendFragment.fragmentNumber);
        fragmentCount  = ENET_NET_TO_HOST_32(command->sendFragment.fragmentCount);
        fragmentOffset = ENET_NET_TO_HOST_32(command->sendFragment.fragmentOffset);
        totalLength    = ENET_NET_TO_HOST_32(command->sendFragment.totalLength);

        if (fragmentCount > ENET_PROTOCOL_MAXIMUM_FRAGMENT_COUNT ||
            fragmentNumber >= fragmentCount ||
            totalLength > host->maximumPacketSize ||
            fragmentOffset >= totalLength ||
            fragmentLength > totalLength - fragmentOffset
        ) {
            return -1;
        }

        for (currentCommand = enet_list_previous(enet_list_end(&channel->incomingUnreliableCommands));
            currentCommand != enet_list_end(&channel->incomingUnreliableCommands);
            currentCommand = enet_list_previous(currentCommand)
        ) {
            ENetIncomingCommand *incomingCommand = (ENetIncomingCommand *) currentCommand;

            if (reliableSequenceNumber >= channel->incomingReliableSequenceNumber) {
                if (incomingCommand->reliableSequenceNumber < channel->incomingReliableSequenceNumber) {
                    continue;
                }
            } else if (incomingCommand->reliableSequenceNumber >= channel->incomingReliableSequenceNumber) {
                break;
            }

            if (incomingCommand->reliableSequenceNumber < reliableSequenceNumber) {
                break;
            }

            if (incomingCommand->reliableSequenceNumber > reliableSequenceNumber) {
                continue;
            }

            if (incomingCommand->unreliableSequenceNumber <= startSequenceNumber) {
                if (incomingCommand->unreliableSequenceNumber < startSequenceNumber) {
                    break;
                }

                if ((incomingCommand->command.header.command & ENET_PROTOCOL_COMMAND_MASK) !=
                    ENET_PROTOCOL_COMMAND_SEND_UNRELIABLE_FRAGMENT ||
                    totalLength != incomingCommand->packet->dataLength ||
                    fragmentCount != incomingCommand->fragmentCount
                ) {
                    return -1;
                }

                startCommand = incomingCommand;
                break;
            }
        }

        if (startCommand == NULL) {
            startCommand = enet_peer_queue_incoming_command(peer, command, NULL, totalLength,
                ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT, fragmentCount);
            if (startCommand == NULL) {
                return -1;
            }
        }

        if ((startCommand->fragments[fragmentNumber / 32] & (1 << (fragmentNumber % 32))) == 0) {
            --startCommand->fragmentsRemaining;
            startCommand->fragments[fragmentNumber / 32] |= (1 << (fragmentNumber % 32));

            if (fragmentOffset + fragmentLength > startCommand->packet->dataLength) {
                fragmentLength = startCommand->packet->dataLength - fragmentOffset;
            }

            memcpy(startCommand->packet->data + fragmentOffset, (enet_uint8 *) command + sizeof(ENetProtocolSendFragment), fragmentLength);

            if (startCommand->fragmentsRemaining <= 0) {
                enet_peer_dispatch_incoming_unreliable_commands(peer, channel);
            }
        }

        return 0;
    } /* enet_protocol_handle_send_unreliable_fragment */

    static int enet_protocol_handle_ping(ENetHost *host, ENetPeer *peer, const ENetProtocol *command) {
        ENET_UNUSED(host)
        ENET_UNUSED(command)

        if (peer->state != ENET_PEER_STATE_CONNECTED && peer->state != ENET_PEER_STATE_DISCONNECT_LATER) {
            return -1;
        }

        return 0;
    }

    static int enet_protocol_handle_bandwidth_limit(ENetHost *host, ENetPeer *peer, const ENetProtocol *command) {
        if (peer->state != ENET_PEER_STATE_CONNECTED && peer->state != ENET_PEER_STATE_DISCONNECT_LATER) {
            return -1;
        }
        if (peer->incomingBandwidth != 0) {
            --host->bandwidthLimitedPeers;
        }

        peer->incomingBandwidth = ENET_NET_TO_HOST_32(command->bandwidthLimit.incomingBandwidth);
        if (peer->incomingBandwidth != 0) {
            ++host->bandwidthLimitedPeers;
        }

        peer->outgoingBandwidth = ENET_NET_TO_HOST_32(command->bandwidthLimit.outgoingBandwidth);

        if (peer->incomingBandwidth == 0 && host->outgoingBandwidth == 0) {
            peer->windowSize = ENET_PROTOCOL_MAXIMUM_WINDOW_SIZE;
        } else if (peer->incomingBandwidth == 0 || host->outgoingBandwidth == 0) {
            peer->windowSize = (ENET_MAX(peer->incomingBandwidth, host->outgoingBandwidth)
              / ENET_PEER_WINDOW_SIZE_SCALE) * ENET_PROTOCOL_MINIMUM_WINDOW_SIZE;
        } else {
            peer->windowSize = (ENET_MIN(peer->incomingBandwidth, host->outgoingBandwidth)
              / ENET_PEER_WINDOW_SIZE_SCALE) * ENET_PROTOCOL_MINIMUM_WINDOW_SIZE;
        }

        if (peer->windowSize < ENET_PROTOCOL_MINIMUM_WINDOW_SIZE) {
            peer->windowSize = ENET_PROTOCOL_MINIMUM_WINDOW_SIZE;
        } else if (peer->windowSize > ENET_PROTOCOL_MAXIMUM_WINDOW_SIZE) {
            peer->windowSize = ENET_PROTOCOL_MAXIMUM_WINDOW_SIZE;
        }

        return 0;
    } /* enet_protocol_handle_bandwidth_limit */

    static int enet_protocol_handle_throttle_configure(ENetHost *host, ENetPeer *peer, const ENetProtocol *command) {
        ENET_UNUSED(host)

        if (peer->state != ENET_PEER_STATE_CONNECTED && peer->state != ENET_PEER_STATE_DISCONNECT_LATER) {
            return -1;
        }

        peer->packetThrottleInterval     = ENET_NET_TO_HOST_32(command->throttleConfigure.packetThrottleInterval);
        peer->packetThrottleAcceleration = ENET_NET_TO_HOST_32(command->throttleConfigure.packetThrottleAcceleration);
        peer->packetThrottleDeceleration = ENET_NET_TO_HOST_32(command->throttleConfigure.packetThrottleDeceleration);

        return 0;
    }

    static int enet_protocol_handle_disconnect(ENetHost *host, ENetPeer *peer, const ENetProtocol *command) {
        if (peer->state == ENET_PEER_STATE_DISCONNECTED || peer->state == ENET_PEER_STATE_ZOMBIE ||
            peer->state == ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT
        ) {
            return 0;
        }

        enet_peer_reset_queues(peer);

        if (peer->state == ENET_PEER_STATE_CONNECTION_SUCCEEDED || peer->state == ENET_PEER_STATE_DISCONNECTING || peer->state == ENET_PEER_STATE_CONNECTING) {
            enet_protocol_dispatch_state(host, peer, ENET_PEER_STATE_ZOMBIE);
        }
        else if (peer->state != ENET_PEER_STATE_CONNECTED && peer->state != ENET_PEER_STATE_DISCONNECT_LATER) {
            if (peer->state == ENET_PEER_STATE_CONNECTION_PENDING) { host->recalculateBandwidthLimits = 1; }
            enet_peer_reset(peer);
        }
        else if (command->header.command & ENET_PROTOCOL_COMMAND_FLAG_ACKNOWLEDGE) {
            enet_protocol_change_state(host, peer, ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT);
        }
        else {
            enet_protocol_dispatch_state(host, peer, ENET_PEER_STATE_ZOMBIE);
        }

        if (peer->state != ENET_PEER_STATE_DISCONNECTED) {
            peer->eventData = ENET_NET_TO_HOST_32(command->disconnect.data);
        }

        return 0;
    }

    static int enet_protocol_handle_acknowledge(ENetHost *host, ENetEvent *event, ENetPeer *peer, const ENetProtocol *command) {
        enet_uint32 roundTripTime, receivedSentTime, receivedReliableSequenceNumber;
        ENetProtocolCommand commandNumber;

        if (peer->state == ENET_PEER_STATE_DISCONNECTED || peer->state == ENET_PEER_STATE_ZOMBIE) {
            return 0;
        }

        receivedSentTime  = ENET_NET_TO_HOST_16(command->acknowledge.receivedSentTime);
        receivedSentTime |= host->serviceTime & 0xFFFF0000;
        if ((receivedSentTime & 0x8000) > (host->serviceTime & 0x8000)) {
            receivedSentTime -= 0x10000;
        }

        if (ENET_TIME_LESS(host->serviceTime, receivedSentTime)) {
            return 0;
        }

        peer->lastReceiveTime = host->serviceTime;
        peer->earliestTimeout = 0;
        roundTripTime = ENET_TIME_DIFFERENCE(host->serviceTime, receivedSentTime);

        enet_peer_throttle(peer, roundTripTime);
        peer->roundTripTimeVariance -= peer->roundTripTimeVariance / 4;

        if (roundTripTime >= peer->roundTripTime) {
            peer->roundTripTime         += (roundTripTime - peer->roundTripTime) / 8;
            peer->roundTripTimeVariance += (roundTripTime - peer->roundTripTime) / 4;
        } else {
            peer->roundTripTime         -= (peer->roundTripTime - roundTripTime) / 8;
            peer->roundTripTimeVariance += (peer->roundTripTime - roundTripTime) / 4;
        }

        if (peer->roundTripTime < peer->lowestRoundTripTime) {
            peer->lowestRoundTripTime = peer->roundTripTime;
        }

        if (peer->roundTripTimeVariance > peer->highestRoundTripTimeVariance) {
            peer->highestRoundTripTimeVariance = peer->roundTripTimeVariance;
        }

        if (peer->packetThrottleEpoch == 0 ||
            ENET_TIME_DIFFERENCE(host->serviceTime, peer->packetThrottleEpoch) >= peer->packetThrottleInterval
        ) {
            peer->lastRoundTripTime            = peer->lowestRoundTripTime;
            peer->lastRoundTripTimeVariance    = peer->highestRoundTripTimeVariance;
            peer->lowestRoundTripTime          = peer->roundTripTime;
            peer->highestRoundTripTimeVariance = peer->roundTripTimeVariance;
            peer->packetThrottleEpoch          = host->serviceTime;
        }

        receivedReliableSequenceNumber = ENET_NET_TO_HOST_16(command->acknowledge.receivedReliableSequenceNumber);
        commandNumber = enet_protocol_remove_sent_reliable_command(peer, receivedReliableSequenceNumber, command->header.channelID);

        switch (peer->state) {
            case ENET_PEER_STATE_ACKNOWLEDGING_CONNECT:
                if (commandNumber != ENET_PROTOCOL_COMMAND_VERIFY_CONNECT) {
                    return -1;
                }

                enet_protocol_notify_connect(host, peer, event);
                break;

            case ENET_PEER_STATE_DISCONNECTING:
                if (commandNumber != ENET_PROTOCOL_COMMAND_DISCONNECT) {
                    return -1;
                }

                enet_protocol_notify_disconnect(host, peer, event);
                break;

            case ENET_PEER_STATE_DISCONNECT_LATER:
                if (enet_list_empty(&peer->outgoingReliableCommands) &&
                  enet_list_empty(&peer->outgoingUnreliableCommands) &&
                  enet_list_empty(&peer->sentReliableCommands))
                {
                    enet_peer_disconnect(peer, peer->eventData);
                }
                break;

            default:
                break;
        }

        return 0;
    } /* enet_protocol_handle_acknowledge */

    static int enet_protocol_handle_verify_connect(ENetHost *host, ENetEvent *event, ENetPeer *peer, const ENetProtocol *command) {
        enet_uint32 mtu, windowSize;
        size_t channelCount;

        if (peer->state != ENET_PEER_STATE_CONNECTING) {
            return 0;
        }

        channelCount = ENET_NET_TO_HOST_32(command->verifyConnect.channelCount);

        if (channelCount < ENET_PROTOCOL_MINIMUM_CHANNEL_COUNT || channelCount > ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT ||
            ENET_NET_TO_HOST_32(command->verifyConnect.packetThrottleInterval) != peer->packetThrottleInterval ||
            ENET_NET_TO_HOST_32(command->verifyConnect.packetThrottleAcceleration) != peer->packetThrottleAcceleration ||
            ENET_NET_TO_HOST_32(command->verifyConnect.packetThrottleDeceleration) != peer->packetThrottleDeceleration ||
            command->verifyConnect.connectID != peer->connectID
        ) {
            peer->eventData = 0;
            enet_protocol_dispatch_state(host, peer, ENET_PEER_STATE_ZOMBIE);
            return -1;
        }

        enet_protocol_remove_sent_reliable_command(peer, 1, 0xFF);

        if (channelCount < peer->channelCount) {
            peer->channelCount = channelCount;
        }

        peer->outgoingPeerID    = ENET_NET_TO_HOST_16(command->verifyConnect.outgoingPeerID);
        peer->incomingSessionID = command->verifyConnect.incomingSessionID;
        peer->outgoingSessionID = command->verifyConnect.outgoingSessionID;

        mtu = ENET_NET_TO_HOST_32(command->verifyConnect.mtu);

        if (mtu < ENET_PROTOCOL_MINIMUM_MTU) {
            mtu = ENET_PROTOCOL_MINIMUM_MTU;
        } else if (mtu > ENET_PROTOCOL_MAXIMUM_MTU) {
            mtu = ENET_PROTOCOL_MAXIMUM_MTU;
        }

        if (mtu < peer->mtu) {
            peer->mtu = mtu;
        }

        windowSize = ENET_NET_TO_HOST_32(command->verifyConnect.windowSize);
        if (windowSize < ENET_PROTOCOL_MINIMUM_WINDOW_SIZE) {
            windowSize = ENET_PROTOCOL_MINIMUM_WINDOW_SIZE;
        }

        if (windowSize > ENET_PROTOCOL_MAXIMUM_WINDOW_SIZE) {
            windowSize = ENET_PROTOCOL_MAXIMUM_WINDOW_SIZE;
        }

        if (windowSize < peer->windowSize) {
            peer->windowSize = windowSize;
        }

        peer->incomingBandwidth = ENET_NET_TO_HOST_32(command->verifyConnect.incomingBandwidth);
        peer->outgoingBandwidth = ENET_NET_TO_HOST_32(command->verifyConnect.outgoingBandwidth);

        enet_protocol_notify_connect(host, peer, event);
        return 0;
    } /* enet_protocol_handle_verify_connect */

    static int enet_protocol_handle_incoming_commands(ENetHost *host, ENetEvent *event) {
        ENetProtocolHeader *header;
        ENetProtocol *command;
        ENetPeer *peer;
        enet_uint8 *currentData;
        size_t headerSize;
        enet_uint16 peerID, flags;
        enet_uint8 sessionID;

        if (host->receivedDataLength < (size_t) &((ENetProtocolHeader *) 0)->sentTime) {
            return 0;
        }

        header = (ENetProtocolHeader *) host->receivedData;

        peerID    = ENET_NET_TO_HOST_16(header->peerID);
        sessionID = (peerID & ENET_PROTOCOL_HEADER_SESSION_MASK) >> ENET_PROTOCOL_HEADER_SESSION_SHIFT;
        flags     = peerID & ENET_PROTOCOL_HEADER_FLAG_MASK;
        peerID   &= ~(ENET_PROTOCOL_HEADER_FLAG_MASK | ENET_PROTOCOL_HEADER_SESSION_MASK);

        headerSize = (flags & ENET_PROTOCOL_HEADER_FLAG_SENT_TIME ? sizeof(ENetProtocolHeader) : (size_t) &((ENetProtocolHeader *) 0)->sentTime);
        if (host->checksum != NULL) {
            headerSize += sizeof(enet_uint32);
        }

        if (peerID == ENET_PROTOCOL_MAXIMUM_PEER_ID) {
            peer = NULL;
        } else if (peerID >= host->peerCount) {
            return 0;
        } else {
            peer = &host->peers[peerID];

            if (peer->state == ENET_PEER_STATE_DISCONNECTED ||
                peer->state == ENET_PEER_STATE_ZOMBIE ||
                ((!in6_equal(host->receivedAddress.host , peer->address.host) ||
                host->receivedAddress.port != peer->address.port) &&
                1 /* no broadcast in ipv6  !in6_equal(peer->address.host , ENET_HOST_BROADCAST)*/) ||
                (peer->outgoingPeerID < ENET_PROTOCOL_MAXIMUM_PEER_ID &&
                sessionID != peer->incomingSessionID)
            ) {
                return 0;
            }
        }

        if (flags & ENET_PROTOCOL_HEADER_FLAG_COMPRESSED) {
            size_t originalSize;
            if (host->compressor.context == NULL || host->compressor.decompress == NULL) {
                return 0;
            }

            originalSize = host->compressor.decompress(host->compressor.context,
                host->receivedData + headerSize,
                host->receivedDataLength - headerSize,
                host->packetData[1] + headerSize,
                sizeof(host->packetData[1]) - headerSize
            );

            if (originalSize <= 0 || originalSize > sizeof(host->packetData[1]) - headerSize) {
                return 0;
            }

            memcpy(host->packetData[1], header, headerSize);
            host->receivedData       = host->packetData[1];
            host->receivedDataLength = headerSize + originalSize;
        }

        if (host->checksum != NULL) {
            enet_uint32 *checksum = (enet_uint32 *) &host->receivedData[headerSize - sizeof(enet_uint32)];
            enet_uint32 desiredChecksum = *checksum;
            ENetBuffer buffer;

            *checksum = peer != NULL ? peer->connectID : 0;

            buffer.data       = host->receivedData;
            buffer.dataLength = host->receivedDataLength;

            if (host->checksum(&buffer, 1) != desiredChecksum) {
                return 0;
            }
        }

        if (peer != NULL) {
            peer->address.host       = host->receivedAddress.host;
            peer->address.port       = host->receivedAddress.port;
            peer->incomingDataTotal += host->receivedDataLength;
            peer->totalDataReceived += host->receivedDataLength;
        }

        currentData = host->receivedData + headerSize;

        while (currentData < &host->receivedData[host->receivedDataLength]) {
            enet_uint8 commandNumber;
            size_t commandSize;

            command = (ENetProtocol *) currentData;

            if (currentData + sizeof(ENetProtocolCommandHeader) > &host->receivedData[host->receivedDataLength]) {
                break;
            }

            commandNumber = command->header.command & ENET_PROTOCOL_COMMAND_MASK;
            if (commandNumber >= ENET_PROTOCOL_COMMAND_COUNT) {
                break;
            }

            commandSize = commandSizes[commandNumber];
            if (commandSize == 0 || currentData + commandSize > &host->receivedData[host->receivedDataLength]) {
                break;
            }

            currentData += commandSize;

            if (peer == NULL && (commandNumber != ENET_PROTOCOL_COMMAND_CONNECT || currentData < &host->receivedData[host->receivedDataLength])) {
                break;
            }

            command->header.reliableSequenceNumber = ENET_NET_TO_HOST_16(command->header.reliableSequenceNumber);

            switch (commandNumber) {
                case ENET_PROTOCOL_COMMAND_ACKNOWLEDGE:
                    if (enet_protocol_handle_acknowledge(host, event, peer, command)) {
                        goto commandError;
                    }
                    break;

                case ENET_PROTOCOL_COMMAND_CONNECT:
                    if (peer != NULL) {
                        goto commandError;
                    }
                    peer = enet_protocol_handle_connect(host, header, command);
                    if (peer == NULL) {
                        goto commandError;
                    }
                    break;

                case ENET_PROTOCOL_COMMAND_VERIFY_CONNECT:
                    if (enet_protocol_handle_verify_connect(host, event, peer, command)) {
                        goto commandError;
                    }
                    break;

                case ENET_PROTOCOL_COMMAND_DISCONNECT:
                    if (enet_protocol_handle_disconnect(host, peer, command)) {
                        goto commandError;
                    }
                    break;

                case ENET_PROTOCOL_COMMAND_PING:
                    if (enet_protocol_handle_ping(host, peer, command)) {
                        goto commandError;
                    }
                    break;

                case ENET_PROTOCOL_COMMAND_SEND_RELIABLE:
                    if (enet_protocol_handle_send_reliable(host, peer, command, &currentData)) {
                        goto commandError;
                    }
                    break;

                case ENET_PROTOCOL_COMMAND_SEND_UNRELIABLE:
                    if (enet_protocol_handle_send_unreliable(host, peer, command, &currentData)) {
                        goto commandError;
                    }
                    break;

                case ENET_PROTOCOL_COMMAND_SEND_UNSEQUENCED:
                    if (enet_protocol_handle_send_unsequenced(host, peer, command, &currentData)) {
                        goto commandError;
                    }
                    break;

                case ENET_PROTOCOL_COMMAND_SEND_FRAGMENT:
                    if (enet_protocol_handle_send_fragment(host, peer, command, &currentData)) {
                        goto commandError;
                    }
                    break;

                case ENET_PROTOCOL_COMMAND_BANDWIDTH_LIMIT:
                    if (enet_protocol_handle_bandwidth_limit(host, peer, command)) {
                        goto commandError;
                    }
                    break;

                case ENET_PROTOCOL_COMMAND_THROTTLE_CONFIGURE:
                    if (enet_protocol_handle_throttle_configure(host, peer, command)) {
                        goto commandError;
                    }
                    break;

                case ENET_PROTOCOL_COMMAND_SEND_UNRELIABLE_FRAGMENT:
                    if (enet_protocol_handle_send_unreliable_fragment(host, peer, command, &currentData)) {
                        goto commandError;
                    }
                    break;

                default:
                    goto commandError;
            }

            assert(peer);
            if ((command->header.command & ENET_PROTOCOL_COMMAND_FLAG_ACKNOWLEDGE) != 0) {
                enet_uint16 sentTime;

                if (!(flags & ENET_PROTOCOL_HEADER_FLAG_SENT_TIME)) {
                    break;
                }

                sentTime = ENET_NET_TO_HOST_16(header->sentTime);

                switch (peer->state) {
                    case ENET_PEER_STATE_DISCONNECTING:
                    case ENET_PEER_STATE_ACKNOWLEDGING_CONNECT:
                    case ENET_PEER_STATE_DISCONNECTED:
                    case ENET_PEER_STATE_ZOMBIE:
                        break;

                    case ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT:
                        if ((command->header.command & ENET_PROTOCOL_COMMAND_MASK) == ENET_PROTOCOL_COMMAND_DISCONNECT) {
                            enet_peer_queue_acknowledgement(peer, command, sentTime);
                        }
                        break;

                    default:
                        enet_peer_queue_acknowledgement(peer, command, sentTime);
                        break;
                }
            }
        }

    commandError:
        if (event != NULL && event->type != ENET_EVENT_TYPE_NONE) {
            return 1;
        }

        return 0;
    } /* enet_protocol_handle_incoming_commands */

    static int enet_protocol_receive_incoming_commands(ENetHost *host, ENetEvent *event) {
        int packets;

        for (packets = 0; packets < 256; ++packets) {
            int receivedLength;
            ENetBuffer buffer;

            buffer.data       = host->packetData[0];
            // buffer.dataLength = sizeof (host->packetData[0]);
            buffer.dataLength = host->mtu;

            receivedLength    = enet_socket_receive(host->socket, &host->receivedAddress, &buffer, 1);

            if (receivedLength == -2)
                continue;

            if (receivedLength < 0) {
                return -1;
            }

            if (receivedLength == 0) {
                return 0;
            }

            host->receivedData       = host->packetData[0];
            host->receivedDataLength = receivedLength;

            host->totalReceivedData += receivedLength;
            host->totalReceivedPackets++;

            if (host->intercept != NULL) {
                switch (host->intercept(host, (void *)event)) {
                    case 1:
                        if (event != NULL && event->type != ENET_EVENT_TYPE_NONE) {
                            return 1;
                        }

                        continue;

                    case -1:
                        return -1;

                    default:
                        break;
                }
            }

            switch (enet_protocol_handle_incoming_commands(host, event)) {
                case 1:
                    return 1;

                case -1:
                    return -1;

                default:
                    break;
            }
        }

        return -1;
    } /* enet_protocol_receive_incoming_commands */

    static void enet_protocol_send_acknowledgements(ENetHost *host, ENetPeer *peer) {
        ENetProtocol *command = &host->commands[host->commandCount];
        ENetBuffer *buffer    = &host->buffers[host->bufferCount];
        ENetAcknowledgement *acknowledgement;
        ENetListIterator currentAcknowledgement;
        enet_uint16 reliableSequenceNumber;

        currentAcknowledgement = enet_list_begin(&peer->acknowledgements);

        while (currentAcknowledgement != enet_list_end(&peer->acknowledgements)) {
            if (command >= &host->commands[sizeof(host->commands) / sizeof(ENetProtocol)] ||
                buffer >= &host->buffers[sizeof(host->buffers) / sizeof(ENetBuffer)] ||
                peer->mtu - host->packetSize < sizeof(ENetProtocolAcknowledge)
            ) {
                host->continueSending = 1;
                break;
            }

            acknowledgement = (ENetAcknowledgement *) currentAcknowledgement;
            currentAcknowledgement = enet_list_next(currentAcknowledgement);

            buffer->data       = command;
            buffer->dataLength = sizeof(ENetProtocolAcknowledge);
            host->packetSize += buffer->dataLength;

            reliableSequenceNumber = ENET_HOST_TO_NET_16(acknowledgement->command.header.reliableSequenceNumber);

            command->header.command   = ENET_PROTOCOL_COMMAND_ACKNOWLEDGE;
            command->header.channelID = acknowledgement->command.header.channelID;
            command->header.reliableSequenceNumber = reliableSequenceNumber;
            command->acknowledge.receivedReliableSequenceNumber = reliableSequenceNumber;
            command->acknowledge.receivedSentTime = ENET_HOST_TO_NET_16(acknowledgement->sentTime);

            if ((acknowledgement->command.header.command & ENET_PROTOCOL_COMMAND_MASK) == ENET_PROTOCOL_COMMAND_DISCONNECT) {
                enet_protocol_dispatch_state(host, peer, ENET_PEER_STATE_ZOMBIE);
            }

            enet_list_remove(&acknowledgement->acknowledgementList);
            enet_free(acknowledgement);

            ++command;
            ++buffer;
        }

        host->commandCount = command - host->commands;
        host->bufferCount  = buffer - host->buffers;
    } /* enet_protocol_send_acknowledgements */

    static void enet_protocol_send_unreliable_outgoing_commands(ENetHost *host, ENetPeer *peer) {
        ENetProtocol *command = &host->commands[host->commandCount];
        ENetBuffer *buffer    = &host->buffers[host->bufferCount];
        ENetOutgoingCommand *outgoingCommand;
        ENetListIterator currentCommand;

        currentCommand = enet_list_begin(&peer->outgoingUnreliableCommands);
        while (currentCommand != enet_list_end(&peer->outgoingUnreliableCommands)) {
            size_t commandSize;

            outgoingCommand = (ENetOutgoingCommand *) currentCommand;
            commandSize     = commandSizes[outgoingCommand->command.header.command & ENET_PROTOCOL_COMMAND_MASK];

            if (command >= &host->commands[sizeof(host->commands) / sizeof(ENetProtocol)] ||
                buffer + 1 >= &host->buffers[sizeof(host->buffers) / sizeof(ENetBuffer)] ||
                peer->mtu - host->packetSize < commandSize ||
                (outgoingCommand->packet != NULL &&
                peer->mtu - host->packetSize < commandSize + outgoingCommand->fragmentLength)
            ) {
                host->continueSending = 1;
                break;
            }

            currentCommand = enet_list_next(currentCommand);

            if (outgoingCommand->packet != NULL && outgoingCommand->fragmentOffset == 0) {
                peer->packetThrottleCounter += ENET_PEER_PACKET_THROTTLE_COUNTER;
                peer->packetThrottleCounter %= ENET_PEER_PACKET_THROTTLE_SCALE;

                if (peer->packetThrottleCounter > peer->packetThrottle) {
                    enet_uint16 reliableSequenceNumber = outgoingCommand->reliableSequenceNumber;
                    enet_uint16 unreliableSequenceNumber = outgoingCommand->unreliableSequenceNumber;
                    for (;;) {
                        --outgoingCommand->packet->referenceCount;

                        if (outgoingCommand->packet->referenceCount == 0) {
                            callbacks.packet_destroy(outgoingCommand->packet);
                        }

                        enet_list_remove(&outgoingCommand->outgoingCommandList);
                        enet_free(outgoingCommand);

                        if (currentCommand == enet_list_end(&peer->outgoingUnreliableCommands)) {
                            break;
                        }

                        outgoingCommand = (ENetOutgoingCommand *) currentCommand;
                        if (outgoingCommand->reliableSequenceNumber != reliableSequenceNumber || outgoingCommand->unreliableSequenceNumber != unreliableSequenceNumber) {
                            break;
                        }

                        currentCommand = enet_list_next(currentCommand);
                    }

                    continue;
                }
            }

            buffer->data       = command;
            buffer->dataLength = commandSize;
            host->packetSize += buffer->dataLength;
            *command = outgoingCommand->command;
            enet_list_remove(&outgoingCommand->outgoingCommandList);

            if (outgoingCommand->packet != NULL) {
                ++buffer;

                buffer->data       = outgoingCommand->packet->data + outgoingCommand->fragmentOffset;
                buffer->dataLength = outgoingCommand->fragmentLength;

                host->packetSize += buffer->dataLength;

                enet_list_insert(enet_list_end(&peer->sentUnreliableCommands), outgoingCommand);
            } else {
                enet_free(outgoingCommand);
            }

            ++command;
            ++buffer;
        }

        host->commandCount = command - host->commands;
        host->bufferCount  = buffer - host->buffers;

        if (peer->state == ENET_PEER_STATE_DISCONNECT_LATER &&
          enet_list_empty(&peer->outgoingReliableCommands) &&
          enet_list_empty(&peer->outgoingUnreliableCommands) &&
          enet_list_empty(&peer->sentReliableCommands))
        {
            enet_peer_disconnect(peer, peer->eventData);
        }
    } /* enet_protocol_send_unreliable_outgoing_commands */

    static int enet_protocol_check_timeouts(ENetHost *host, ENetPeer *peer, ENetEvent *event) {
        ENetOutgoingCommand *outgoingCommand;
        ENetListIterator currentCommand, insertPosition;

        currentCommand = enet_list_begin(&peer->sentReliableCommands);
        insertPosition = enet_list_begin(&peer->outgoingReliableCommands);

        while (currentCommand != enet_list_end(&peer->sentReliableCommands)) {
            outgoingCommand = (ENetOutgoingCommand *) currentCommand;

            currentCommand = enet_list_next(currentCommand);

            if (ENET_TIME_DIFFERENCE(host->serviceTime, outgoingCommand->sentTime) < outgoingCommand->roundTripTimeout) {
                continue;
            }

            if (peer->earliestTimeout == 0 || ENET_TIME_LESS(outgoingCommand->sentTime, peer->earliestTimeout)) {
                peer->earliestTimeout = outgoingCommand->sentTime;
            }

            if (peer->earliestTimeout != 0 &&
                (ENET_TIME_DIFFERENCE(host->serviceTime, peer->earliestTimeout) >= peer->timeoutMaximum ||
                (outgoingCommand->roundTripTimeout >= outgoingCommand->roundTripTimeoutLimit &&
                ENET_TIME_DIFFERENCE(host->serviceTime, peer->earliestTimeout) >= peer->timeoutMinimum))
            ) {
                enet_protocol_notify_disconnect_timeout(host, peer, event);
                return 1;
            }

            if (outgoingCommand->packet != NULL) {
                peer->reliableDataInTransit -= outgoingCommand->fragmentLength;
            }

            ++peer->packetsLost;
            ++peer->totalPacketsLost;

            /* Replaced exponential backoff time with something more linear */
            /* Source: http://lists.cubik.org/pipermail/enet-discuss/2014-May/002308.html */
            outgoingCommand->roundTripTimeout = peer->roundTripTime + 4 * peer->roundTripTimeVariance;
            outgoingCommand->roundTripTimeoutLimit = peer->timeoutLimit * outgoingCommand->roundTripTimeout;

            enet_list_insert(insertPosition, enet_list_remove(&outgoingCommand->outgoingCommandList));

            if (currentCommand == enet_list_begin(&peer->sentReliableCommands) && !enet_list_empty(&peer->sentReliableCommands)) {
                outgoingCommand = (ENetOutgoingCommand *) currentCommand;
                peer->nextTimeout = outgoingCommand->sentTime + outgoingCommand->roundTripTimeout;
            }
        }

        return 0;
    } /* enet_protocol_check_timeouts */

    static int enet_protocol_send_reliable_outgoing_commands(ENetHost *host, ENetPeer *peer) {
        ENetProtocol *command = &host->commands[host->commandCount];
        ENetBuffer *buffer    = &host->buffers[host->bufferCount];
        ENetOutgoingCommand *outgoingCommand;
        ENetListIterator currentCommand;
        ENetChannel *channel;
        enet_uint16 reliableWindow;
        size_t commandSize;
        int windowExceeded = 0, windowWrap = 0, canPing = 1;

        currentCommand = enet_list_begin(&peer->outgoingReliableCommands);

        while (currentCommand != enet_list_end(&peer->outgoingReliableCommands)) {
            outgoingCommand = (ENetOutgoingCommand *) currentCommand;

            channel = outgoingCommand->command.header.channelID < peer->channelCount ? &peer->channels[outgoingCommand->command.header.channelID] : NULL;
            reliableWindow = outgoingCommand->reliableSequenceNumber / ENET_PEER_RELIABLE_WINDOW_SIZE;
            if (channel != NULL) {
                if (!windowWrap &&
                    outgoingCommand->sendAttempts < 1 &&
                    !(outgoingCommand->reliableSequenceNumber % ENET_PEER_RELIABLE_WINDOW_SIZE) &&
                    (channel->reliableWindows[(reliableWindow + ENET_PEER_RELIABLE_WINDOWS - 1)
                    % ENET_PEER_RELIABLE_WINDOWS] >= ENET_PEER_RELIABLE_WINDOW_SIZE ||
                    channel->usedReliableWindows & ((((1 << ENET_PEER_FREE_RELIABLE_WINDOWS) - 1) << reliableWindow)
                    | (((1 << ENET_PEER_FREE_RELIABLE_WINDOWS) - 1) >> (ENET_PEER_RELIABLE_WINDOWS - reliableWindow))))
                ) {
                    windowWrap = 1;
                }

                if (windowWrap) {
                    currentCommand = enet_list_next(currentCommand);
                    continue;
                }
            }

            if (outgoingCommand->packet != NULL) {
                if (!windowExceeded) {
                    enet_uint32 windowSize = (peer->packetThrottle * peer->windowSize) / ENET_PEER_PACKET_THROTTLE_SCALE;

                    if (peer->reliableDataInTransit + outgoingCommand->fragmentLength > ENET_MAX(windowSize, peer->mtu)) {
                        windowExceeded = 1;
                    }
                }
                if (windowExceeded) {
                    currentCommand = enet_list_next(currentCommand);

                    continue;
                }
            }

            canPing = 0;

            commandSize = commandSizes[outgoingCommand->command.header.command & ENET_PROTOCOL_COMMAND_MASK];
            if (command >= &host->commands[sizeof(host->commands) / sizeof(ENetProtocol)] ||
                buffer + 1 >= &host->buffers[sizeof(host->buffers) / sizeof(ENetBuffer)] ||
                peer->mtu - host->packetSize < commandSize ||
                (outgoingCommand->packet != NULL &&
                (enet_uint16) (peer->mtu - host->packetSize) < (enet_uint16) (commandSize + outgoingCommand->fragmentLength))
            ) {
                host->continueSending = 1;
                break;
            }

            currentCommand = enet_list_next(currentCommand);

            if (channel != NULL && outgoingCommand->sendAttempts < 1) {
                channel->usedReliableWindows |= 1 << reliableWindow;
                ++channel->reliableWindows[reliableWindow];
            }

            ++outgoingCommand->sendAttempts;

            if (outgoingCommand->roundTripTimeout == 0) {
                outgoingCommand->roundTripTimeout      = peer->roundTripTime + 4 * peer->roundTripTimeVariance;
                outgoingCommand->roundTripTimeoutLimit = peer->timeoutLimit * outgoingCommand->roundTripTimeout;
            }

            if (enet_list_empty(&peer->sentReliableCommands)) {
                peer->nextTimeout = host->serviceTime + outgoingCommand->roundTripTimeout;
            }

            enet_list_insert(enet_list_end(&peer->sentReliableCommands), enet_list_remove(&outgoingCommand->outgoingCommandList));

            outgoingCommand->sentTime = host->serviceTime;

            buffer->data       = command;
            buffer->dataLength = commandSize;

            host->packetSize  += buffer->dataLength;
            host->headerFlags |= ENET_PROTOCOL_HEADER_FLAG_SENT_TIME;

            *command = outgoingCommand->command;

            if (outgoingCommand->packet != NULL) {
                ++buffer;
                buffer->data       = outgoingCommand->packet->data + outgoingCommand->fragmentOffset;
                buffer->dataLength = outgoingCommand->fragmentLength;
                host->packetSize += outgoingCommand->fragmentLength;
                peer->reliableDataInTransit += outgoingCommand->fragmentLength;
            }

            ++peer->packetsSent;
            ++peer->totalPacketsSent;

            ++command;
            ++buffer;
        }

        host->commandCount = command - host->commands;
        host->bufferCount  = buffer - host->buffers;

        return canPing;
    } /* enet_protocol_send_reliable_outgoing_commands */

    static int enet_protocol_send_outgoing_commands(ENetHost *host, ENetEvent *event, int checkForTimeouts) {
        enet_uint8 headerData[sizeof(ENetProtocolHeader) + sizeof(enet_uint32)];
        ENetProtocolHeader *header = (ENetProtocolHeader *) headerData;
        ENetPeer *currentPeer;
        int sentLength;
        size_t shouldCompress = 0;
        host->continueSending = 1;

        while (host->continueSending)
            for (host->continueSending = 0, currentPeer = host->peers; currentPeer < &host->peers[host->peerCount]; ++currentPeer) {
                if (currentPeer->state == ENET_PEER_STATE_DISCONNECTED || currentPeer->state == ENET_PEER_STATE_ZOMBIE) {
                    continue;
                }

                host->headerFlags  = 0;
                host->commandCount = 0;
                host->bufferCount  = 1;
                host->packetSize   = sizeof(ENetProtocolHeader);

                if (!enet_list_empty(&currentPeer->acknowledgements)) {
                    enet_protocol_send_acknowledgements(host, currentPeer);
                }

                if (checkForTimeouts != 0 &&
                    !enet_list_empty(&currentPeer->sentReliableCommands) &&
                    ENET_TIME_GREATER_EQUAL(host->serviceTime, currentPeer->nextTimeout) &&
                    enet_protocol_check_timeouts(host, currentPeer, event) == 1
                ) {
                    if (event != NULL && event->type != ENET_EVENT_TYPE_NONE) {
                        return 1;
                    } else {
                        continue;
                    }
                }

                if ((enet_list_empty(&currentPeer->outgoingReliableCommands) ||
                    enet_protocol_send_reliable_outgoing_commands(host, currentPeer)) &&
                    enet_list_empty(&currentPeer->sentReliableCommands) &&
                    ENET_TIME_DIFFERENCE(host->serviceTime, currentPeer->lastReceiveTime) >= currentPeer->pingInterval &&
                    currentPeer->mtu - host->packetSize >= sizeof(ENetProtocolPing)
                ) {
                    enet_peer_ping(currentPeer);
                    enet_protocol_send_reliable_outgoing_commands(host, currentPeer);
                }

                if (!enet_list_empty(&currentPeer->outgoingUnreliableCommands)) {
                    enet_protocol_send_unreliable_outgoing_commands(host, currentPeer);
                }

                if (host->commandCount == 0) {
                    continue;
                }

                if (currentPeer->packetLossEpoch == 0) {
                    currentPeer->packetLossEpoch = host->serviceTime;
                } else if (ENET_TIME_DIFFERENCE(host->serviceTime, currentPeer->packetLossEpoch) >= ENET_PEER_PACKET_LOSS_INTERVAL && currentPeer->packetsSent > 0) {
                    enet_uint32 packetLoss = currentPeer->packetsLost * ENET_PEER_PACKET_LOSS_SCALE / currentPeer->packetsSent;

                    #ifdef ENET_DEBUG
                    printf(
                        "peer %u: %f%%+-%f%% packet loss, %u+-%u ms round trip time, %f%% throttle, %u/%u outgoing, %u/%u incoming\n", currentPeer->incomingPeerID,
                        currentPeer->packetLoss / (float) ENET_PEER_PACKET_LOSS_SCALE,
                        currentPeer->packetLossVariance / (float) ENET_PEER_PACKET_LOSS_SCALE, currentPeer->roundTripTime, currentPeer->roundTripTimeVariance,
                        currentPeer->packetThrottle / (float) ENET_PEER_PACKET_THROTTLE_SCALE,
                        enet_list_size(&currentPeer->outgoingReliableCommands),
                        enet_list_size(&currentPeer->outgoingUnreliableCommands),
                        currentPeer->channels != NULL ? enet_list_size( &currentPeer->channels->incomingReliableCommands) : 0,
                        currentPeer->channels != NULL ? enet_list_size(&currentPeer->channels->incomingUnreliableCommands) : 0
                    );
                    #endif

                    currentPeer->packetLossVariance -= currentPeer->packetLossVariance / 4;

                    if (packetLoss >= currentPeer->packetLoss) {
                        currentPeer->packetLoss         += (packetLoss - currentPeer->packetLoss) / 8;
                        currentPeer->packetLossVariance += (packetLoss - currentPeer->packetLoss) / 4;
                    } else {
                        currentPeer->packetLoss         -= (currentPeer->packetLoss - packetLoss) / 8;
                        currentPeer->packetLossVariance += (currentPeer->packetLoss - packetLoss) / 4;
                    }

                    currentPeer->packetLossEpoch = host->serviceTime;
                    currentPeer->packetsSent     = 0;
                    currentPeer->packetsLost     = 0;
                }

                host->buffers[0].data = headerData;
                if (host->headerFlags & ENET_PROTOCOL_HEADER_FLAG_SENT_TIME) {
                    header->sentTime = ENET_HOST_TO_NET_16(host->serviceTime & 0xFFFF);
                    host->buffers[0].dataLength = sizeof(ENetProtocolHeader);
                } else {
                    host->buffers[0].dataLength = (size_t) &((ENetProtocolHeader *) 0)->sentTime;
                }

                shouldCompress = 0;
                if (host->compressor.context != NULL && host->compressor.compress != NULL) {
                    size_t originalSize = host->packetSize - sizeof(ENetProtocolHeader),
                      compressedSize    = host->compressor.compress(host->compressor.context, &host->buffers[1], host->bufferCount - 1, originalSize, host->packetData[1], originalSize);
                    if (compressedSize > 0 && compressedSize < originalSize) {
                        host->headerFlags |= ENET_PROTOCOL_HEADER_FLAG_COMPRESSED;
                        shouldCompress     = compressedSize;
                        #ifdef ENET_DEBUG_COMPRESS
                        printf("peer %u: compressed %u->%u (%u%%)\n", currentPeer->incomingPeerID, originalSize, compressedSize, (compressedSize * 100) / originalSize);
                        #endif
                    }
                }

                if (currentPeer->outgoingPeerID < ENET_PROTOCOL_MAXIMUM_PEER_ID) {
                    host->headerFlags |= currentPeer->outgoingSessionID << ENET_PROTOCOL_HEADER_SESSION_SHIFT;
                }
                header->peerID = ENET_HOST_TO_NET_16(currentPeer->outgoingPeerID | host->headerFlags);
                if (host->checksum != NULL) {
                    enet_uint32 *checksum = (enet_uint32 *) &headerData[host->buffers[0].dataLength];
                    *checksum = currentPeer->outgoingPeerID < ENET_PROTOCOL_MAXIMUM_PEER_ID ? currentPeer->connectID : 0;
                    host->buffers[0].dataLength += sizeof(enet_uint32);
                    *checksum = host->checksum(host->buffers, host->bufferCount);
                }

                if (shouldCompress > 0) {
                    host->buffers[1].data       = host->packetData[1];
                    host->buffers[1].dataLength = shouldCompress;
                    host->bufferCount = 2;
                }

                currentPeer->lastSendTime = host->serviceTime;
                sentLength = enet_socket_send(host->socket, &currentPeer->address, host->buffers, host->bufferCount);
                enet_protocol_remove_sent_unreliable_commands(currentPeer);

                if (sentLength < 0) {
                    // The local 'headerData' array (to which 'data' is assigned) goes out
                    // of scope on return from this function, so ensure we no longer point to it.
                    host->buffers[0].data = NULL;
                    return -1;
                }

                host->totalSentData += sentLength;
                currentPeer->totalDataSent += sentLength;
                host->totalSentPackets++;
            }

        // The local 'headerData' array (to which 'data' is assigned) goes out
        // of scope on return from this function, so ensure we no longer point to it.
        host->buffers[0].data = NULL;

        return 0;
    } /* enet_protocol_send_outgoing_commands */

    /** Sends any queued packets on the host specified to its designated peers.
     *
     *  @param host   host to flush
     *  @remarks this function need only be used in circumstances where one wishes to send queued packets earlier than in a call to enet_host_service().
     *  @ingroup host
     */
    void enet_host_flush(ENetHost *host) {
        host->serviceTime = enet_time_get();
        enet_protocol_send_outgoing_commands(host, NULL, 0);
    }

    /** Checks for any queued events on the host and dispatches one if available.
     *
     *  @param host    host to check for events
     *  @param event   an event structure where event details will be placed if available
     *  @retval > 0 if an event was dispatched
     *  @retval 0 if no events are available
     *  @retval < 0 on failure
     *  @ingroup host
     */
    int enet_host_check_events(ENetHost *host, ENetEvent *event) {
        if (event == NULL) { return -1; }

        event->type   = ENET_EVENT_TYPE_NONE;
        event->peer   = NULL;
        event->packet = NULL;

        return enet_protocol_dispatch_incoming_commands(host, event);
    }

    /** Waits for events on the host specified and shuttles packets between
     *  the host and its peers.
     *
     *  @param host    host to service
     *  @param event   an event structure where event details will be placed if one occurs
     *                 if event == NULL then no events will be delivered
     *  @param timeout number of milliseconds that ENet should wait for events
     *  @retval > 0 if an event occurred within the specified time limit
     *  @retval 0 if no event occurred
     *  @retval < 0 on failure
     *  @remarks enet_host_service should be called fairly regularly for adequate performance
     *  @ingroup host
     */
    int enet_host_service(ENetHost *host, ENetEvent *event, enet_uint32 timeout) {
        enet_uint32 waitCondition;

        if (event != NULL) {
            event->type   = ENET_EVENT_TYPE_NONE;
            event->peer   = NULL;
            event->packet = NULL;

            switch (enet_protocol_dispatch_incoming_commands(host, event)) {
                case 1:
                    return 1;

                case -1:
                    #ifdef ENET_DEBUG
                    perror("Error dispatching incoming packets");
                    #endif

                    return -1;

                default:
                    break;
            }
        }

        host->serviceTime = enet_time_get();
        timeout += host->serviceTime;

        do {
            if (ENET_TIME_DIFFERENCE(host->serviceTime, host->bandwidthThrottleEpoch) >= ENET_HOST_BANDWIDTH_THROTTLE_INTERVAL) {
                enet_host_bandwidth_throttle(host);
            }

            switch (enet_protocol_send_outgoing_commands(host, event, 1)) {
                case 1:
                    return 1;

                case -1:
                    #ifdef ENET_DEBUG
                    perror("Error sending outgoing packets");
                    #endif

                    return -1;

                default:
                    break;
            }

            switch (enet_protocol_receive_incoming_commands(host, event)) {
                case 1:
                    return 1;

                case -1:
                    #ifdef ENET_DEBUG
                    perror("Error receiving incoming packets");
                    #endif

                    return -1;

                default:
                    break;
            }

            switch (enet_protocol_send_outgoing_commands(host, event, 1)) {
                case 1:
                    return 1;

                case -1:
                    #ifdef ENET_DEBUG
                    perror("Error sending outgoing packets");
                    #endif

                    return -1;

                default:
                    break;
            }

            if (event != NULL) {
                switch (enet_protocol_dispatch_incoming_commands(host, event)) {
                    case 1:
                        return 1;

                    case -1:
                        #ifdef ENET_DEBUG
                        perror("Error dispatching incoming packets");
                        #endif

                        return -1;

                    default:
                        break;
                }
            }

            if (ENET_TIME_GREATER_EQUAL(host->serviceTime, timeout)) {
                return 0;
            }

            do {
                host->serviceTime = enet_time_get();

                if (ENET_TIME_GREATER_EQUAL(host->serviceTime, timeout)) {
                    return 0;
                }

                waitCondition = ENET_SOCKET_WAIT_RECEIVE | ENET_SOCKET_WAIT_INTERRUPT;
                if (enet_socket_wait(host->socket, &waitCondition, ENET_TIME_DIFFERENCE(timeout, host->serviceTime)) != 0) {
                    return -1;
                }
            } while (waitCondition & ENET_SOCKET_WAIT_INTERRUPT);

            host->serviceTime = enet_time_get();
        } while (waitCondition & ENET_SOCKET_WAIT_RECEIVE);

        return 0;
    } /* enet_host_service */


// =======================================================================//
// !
// ! Peer
// !
// =======================================================================//

    /** Configures throttle parameter for a peer.
     *
     *  Unreliable packets are dropped by ENet in response to the varying conditions
     *  of the Internet connection to the peer.  The throttle represents a probability
     *  that an unreliable packet should not be dropped and thus sent by ENet to the peer.
     *  The lowest mean round trip time from the sending of a reliable packet to the
     *  receipt of its acknowledgement is measured over an amount of time specified by
     *  the interval parameter in milliseconds.  If a measured round trip time happens to
     *  be significantly less than the mean round trip time measured over the interval,
     *  then the throttle probability is increased to allow more traffic by an amount
     *  specified in the acceleration parameter, which is a ratio to the ENET_PEER_PACKET_THROTTLE_SCALE
     *  constant.  If a measured round trip time happens to be significantly greater than
     *  the mean round trip time measured over the interval, then the throttle probability
     *  is decreased to limit traffic by an amount specified in the deceleration parameter, which
     *  is a ratio to the ENET_PEER_PACKET_THROTTLE_SCALE constant.  When the throttle has
     *  a value of ENET_PEER_PACKET_THROTTLE_SCALE, no unreliable packets are dropped by
     *  ENet, and so 100% of all unreliable packets will be sent.  When the throttle has a
     *  value of 0, all unreliable packets are dropped by ENet, and so 0% of all unreliable
     *  packets will be sent.  Intermediate values for the throttle represent intermediate
     *  probabilities between 0% and 100% of unreliable packets being sent.  The bandwidth
     *  limits of the local and foreign hosts are taken into account to determine a
     *  sensible limit for the throttle probability above which it should not raise even in
     *  the best of conditions.
     *
     *  @param peer peer to configure
     *  @param interval interval, in milliseconds, over which to measure lowest mean RTT; the default value is ENET_PEER_PACKET_THROTTLE_INTERVAL.
     *  @param acceleration rate at which to increase the throttle probability as mean RTT declines
     *  @param deceleration rate at which to decrease the throttle probability as mean RTT increases
     */
    void enet_peer_throttle_configure(ENetPeer *peer, enet_uint32 interval, enet_uint32 acceleration, enet_uint32 deceleration) {
        ENetProtocol command;

        peer->packetThrottleInterval     = interval;
        peer->packetThrottleAcceleration = acceleration;
        peer->packetThrottleDeceleration = deceleration;

        command.header.command   = ENET_PROTOCOL_COMMAND_THROTTLE_CONFIGURE | ENET_PROTOCOL_COMMAND_FLAG_ACKNOWLEDGE;
        command.header.channelID = 0xFF;

        command.throttleConfigure.packetThrottleInterval     = ENET_HOST_TO_NET_32(interval);
        command.throttleConfigure.packetThrottleAcceleration = ENET_HOST_TO_NET_32(acceleration);
        command.throttleConfigure.packetThrottleDeceleration = ENET_HOST_TO_NET_32(deceleration);

        enet_peer_queue_outgoing_command(peer, &command, NULL, 0, 0);
    }

    int enet_peer_throttle(ENetPeer *peer, enet_uint32 rtt) {
        if (peer->lastRoundTripTime <= peer->lastRoundTripTimeVariance) {
            peer->packetThrottle = peer->packetThrottleLimit;
        }
        else if (rtt < peer->lastRoundTripTime) {
            peer->packetThrottle += peer->packetThrottleAcceleration;

            if (peer->packetThrottle > peer->packetThrottleLimit) {
                peer->packetThrottle = peer->packetThrottleLimit;
            }

            return 1;
        }
        else if (rtt > peer->lastRoundTripTime + 2 * peer->lastRoundTripTimeVariance) {
            if (peer->packetThrottle > peer->packetThrottleDeceleration) {
                peer->packetThrottle -= peer->packetThrottleDeceleration;
            } else {
                peer->packetThrottle = 0;
            }

            return -1;
        }

        return 0;
    }

    /* Extended functionality for easier binding in other programming languages */
    enet_uint32 enet_host_get_peers_count(ENetHost *host) {
        return host->connectedPeers;
    }

    enet_uint32 enet_host_get_packets_sent(ENetHost *host) {
        return host->totalSentPackets;
    }

    enet_uint32 enet_host_get_packets_received(ENetHost *host) {
        return host->totalReceivedPackets;
    }

    enet_uint32 enet_host_get_bytes_sent(ENetHost *host) {
        return host->totalSentData;
    }

    enet_uint32 enet_host_get_bytes_received(ENetHost *host) {
        return host->totalReceivedData;
    }

    /** Gets received data buffer. Returns buffer length.
     *  @param host host to access recevie buffer
     *  @param data ouput parameter for recevied data
     *  @retval buffer length
     */
    enet_uint32 enet_host_get_received_data(ENetHost *host, /*out*/ enet_uint8** data) {
        *data = host->receivedData;
        return host->receivedDataLength;
    }

    enet_uint32 enet_host_get_mtu(ENetHost *host) {
        return host->mtu;
    }

    enet_uint32 enet_peer_get_id(ENetPeer *peer) {
        return peer->connectID;
    }

    enet_uint32 enet_peer_get_ip(ENetPeer *peer, char *ip, size_t ipLength) {
        return enet_address_get_host_ip(&peer->address, ip, ipLength);
    }

    enet_uint16 enet_peer_get_port(ENetPeer *peer) {
        return peer->address.port;
    }

    ENetPeerState enet_peer_get_state(ENetPeer *peer) {
        return peer->state;
    }

    enet_uint32 enet_peer_get_rtt(ENetPeer *peer) {
        return peer->roundTripTime;
    }

    enet_uint64 enet_peer_get_packets_sent(ENetPeer *peer) {
        return peer->totalPacketsSent;
    }

    enet_uint32 enet_peer_get_packets_lost(ENetPeer *peer) {
        return peer->totalPacketsLost;
    }

    enet_uint64 enet_peer_get_bytes_sent(ENetPeer *peer) {
        return peer->totalDataSent;
    }

    enet_uint64 enet_peer_get_bytes_received(ENetPeer *peer) {
        return peer->totalDataReceived;
    }

    void * enet_peer_get_data(ENetPeer *peer) {
        return (void *) peer->data;
    }

    void enet_peer_set_data(ENetPeer *peer, const void *data) {
        peer->data = (enet_uint32 *) data;
    }

    void * enet_packet_get_data(ENetPacket *packet) {
        return (void *) packet->data;
    }

    enet_uint32 enet_packet_get_length(ENetPacket *packet) {
        return packet->dataLength;
    }

    void enet_packet_set_free_callback(ENetPacket *packet, void *callback) {
        packet->freeCallback = (ENetPacketFreeCallback)callback;
    }

    /** Queues a packet to be sent.
     *  @param peer destination for the packet
     *  @param channelID channel on which to send
     *  @param packet packet to send
     *  @retval 0 on success
     *  @retval < 0 on failure
     */
    int enet_peer_send(ENetPeer *peer, enet_uint8 channelID, ENetPacket *packet) {
        ENetChannel *channel = &peer->channels[channelID];
        ENetProtocol command;
        size_t fragmentLength;

        if (peer->state != ENET_PEER_STATE_CONNECTED || channelID >= peer->channelCount || packet->dataLength > peer->host->maximumPacketSize) {
            return -1;
        }

        fragmentLength = peer->mtu - sizeof(ENetProtocolHeader) - sizeof(ENetProtocolSendFragment);
        if (peer->host->checksum != NULL) {
            fragmentLength -= sizeof(enet_uint32);
        }

        if (packet->dataLength > fragmentLength) {
            enet_uint32 fragmentCount = (packet->dataLength + fragmentLength - 1) / fragmentLength, fragmentNumber, fragmentOffset;
            enet_uint8 commandNumber;
            enet_uint16 startSequenceNumber;
            ENetList fragments;
            ENetOutgoingCommand *fragment;

            if (fragmentCount > ENET_PROTOCOL_MAXIMUM_FRAGMENT_COUNT) {
                return -1;
            }

            if ((packet->flags & (ENET_PACKET_FLAG_RELIABLE | ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT)) ==
                ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT &&
                channel->outgoingUnreliableSequenceNumber < 0xFFFF)
            {
                commandNumber       = ENET_PROTOCOL_COMMAND_SEND_UNRELIABLE_FRAGMENT;
                startSequenceNumber = ENET_HOST_TO_NET_16(channel->outgoingUnreliableSequenceNumber + 1);
            } else {
                commandNumber       = ENET_PROTOCOL_COMMAND_SEND_FRAGMENT | ENET_PROTOCOL_COMMAND_FLAG_ACKNOWLEDGE;
                startSequenceNumber = ENET_HOST_TO_NET_16(channel->outgoingReliableSequenceNumber + 1);
            }

            enet_list_clear(&fragments);

            for (fragmentNumber = 0, fragmentOffset = 0; fragmentOffset < packet->dataLength; ++fragmentNumber, fragmentOffset += fragmentLength) {
                if (packet->dataLength - fragmentOffset < fragmentLength) {
                    fragmentLength = packet->dataLength - fragmentOffset;
                }

                fragment = (ENetOutgoingCommand *) enet_malloc(sizeof(ENetOutgoingCommand));

                if (fragment == NULL) {
                    while (!enet_list_empty(&fragments)) {
                        fragment = (ENetOutgoingCommand *) enet_list_remove(enet_list_begin(&fragments));

                        enet_free(fragment);
                    }

                    return -1;
                }

                fragment->fragmentOffset           = fragmentOffset;
                fragment->fragmentLength           = fragmentLength;
                fragment->packet                   = packet;
                fragment->command.header.command   = commandNumber;
                fragment->command.header.channelID = channelID;

                fragment->command.sendFragment.startSequenceNumber = startSequenceNumber;

                fragment->command.sendFragment.dataLength     = ENET_HOST_TO_NET_16(fragmentLength);
                fragment->command.sendFragment.fragmentCount  = ENET_HOST_TO_NET_32(fragmentCount);
                fragment->command.sendFragment.fragmentNumber = ENET_HOST_TO_NET_32(fragmentNumber);
                fragment->command.sendFragment.totalLength    = ENET_HOST_TO_NET_32(packet->dataLength);
                fragment->command.sendFragment.fragmentOffset = ENET_NET_TO_HOST_32(fragmentOffset);

                enet_list_insert(enet_list_end(&fragments), fragment);
            }

            packet->referenceCount += fragmentNumber;

            while (!enet_list_empty(&fragments)) {
                fragment = (ENetOutgoingCommand *) enet_list_remove(enet_list_begin(&fragments));
                enet_peer_setup_outgoing_command(peer, fragment);
            }

            return 0;
        }

        command.header.channelID = channelID;

        if ((packet->flags & (ENET_PACKET_FLAG_RELIABLE | ENET_PACKET_FLAG_UNSEQUENCED)) == ENET_PACKET_FLAG_UNSEQUENCED) {
            command.header.command = ENET_PROTOCOL_COMMAND_SEND_UNSEQUENCED | ENET_PROTOCOL_COMMAND_FLAG_UNSEQUENCED;
            command.sendUnsequenced.dataLength = ENET_HOST_TO_NET_16(packet->dataLength);
        }
        else if (packet->flags & ENET_PACKET_FLAG_RELIABLE || channel->outgoingUnreliableSequenceNumber >= 0xFFFF) {
            command.header.command = ENET_PROTOCOL_COMMAND_SEND_RELIABLE | ENET_PROTOCOL_COMMAND_FLAG_ACKNOWLEDGE;
            command.sendReliable.dataLength = ENET_HOST_TO_NET_16(packet->dataLength);
        }
        else {
            command.header.command = ENET_PROTOCOL_COMMAND_SEND_UNRELIABLE;
            command.sendUnreliable.dataLength = ENET_HOST_TO_NET_16(packet->dataLength);
        }

        if (enet_peer_queue_outgoing_command(peer, &command, packet, 0, packet->dataLength) == NULL) {
            return -1;
        }

        return 0;
    } // enet_peer_send

    /** Attempts to dequeue any incoming queued packet.
     *  @param peer peer to dequeue packets from
     *  @param channelID holds the channel ID of the channel the packet was received on success
     *  @returns a pointer to the packet, or NULL if there are no available incoming queued packets
     */
    ENetPacket * enet_peer_receive(ENetPeer *peer, enet_uint8 *channelID) {
        ENetIncomingCommand *incomingCommand;
        ENetPacket *packet;

        if (enet_list_empty(&peer->dispatchedCommands)) {
            return NULL;
        }

        incomingCommand = (ENetIncomingCommand *) enet_list_remove(enet_list_begin(&peer->dispatchedCommands));

        if (channelID != NULL) {
            *channelID = incomingCommand->command.header.channelID;
        }

        packet = incomingCommand->packet;
        --packet->referenceCount;

        if (incomingCommand->fragments != NULL) {
            enet_free(incomingCommand->fragments);
        }

        enet_free(incomingCommand);
        peer->totalWaitingData -= packet->dataLength;

        return packet;
    }

    static void enet_peer_reset_outgoing_commands(ENetList *queue) {
        ENetOutgoingCommand *outgoingCommand;

        while (!enet_list_empty(queue)) {
            outgoingCommand = (ENetOutgoingCommand *) enet_list_remove(enet_list_begin(queue));

            if (outgoingCommand->packet != NULL) {
                --outgoingCommand->packet->referenceCount;

                if (outgoingCommand->packet->referenceCount == 0) {
                    callbacks.packet_destroy(outgoingCommand->packet);
                }
            }

            enet_free(outgoingCommand);
        }
    }

    static void enet_peer_remove_incoming_commands(ENetList *queue, ENetListIterator startCommand, ENetListIterator endCommand) {
        ENET_UNUSED(queue)

        ENetListIterator currentCommand;

        for (currentCommand = startCommand; currentCommand != endCommand;) {
            ENetIncomingCommand *incomingCommand = (ENetIncomingCommand *) currentCommand;

            currentCommand = enet_list_next(currentCommand);
            enet_list_remove(&incomingCommand->incomingCommandList);

            if (incomingCommand->packet != NULL) {
                --incomingCommand->packet->referenceCount;

                if (incomingCommand->packet->referenceCount == 0) {
                    callbacks.packet_destroy(incomingCommand->packet);
                }
            }

            if (incomingCommand->fragments != NULL) {
                enet_free(incomingCommand->fragments);
            }

            enet_free(incomingCommand);
        }
    }

    static void enet_peer_reset_incoming_commands(ENetList *queue) {
        enet_peer_remove_incoming_commands(queue, enet_list_begin(queue), enet_list_end(queue));
    }

    void enet_peer_reset_queues(ENetPeer *peer) {
        ENetChannel *channel;

        if (peer->needsDispatch) {
            enet_list_remove(&peer->dispatchList);
            peer->needsDispatch = 0;
        }

        while (!enet_list_empty(&peer->acknowledgements)) {
            enet_free(enet_list_remove(enet_list_begin(&peer->acknowledgements)));
        }

        enet_peer_reset_outgoing_commands(&peer->sentReliableCommands);
        enet_peer_reset_outgoing_commands(&peer->sentUnreliableCommands);
        enet_peer_reset_outgoing_commands(&peer->outgoingReliableCommands);
        enet_peer_reset_outgoing_commands(&peer->outgoingUnreliableCommands);
        enet_peer_reset_incoming_commands(&peer->dispatchedCommands);

        if (peer->channels != NULL && peer->channelCount > 0) {
            for (channel = peer->channels; channel < &peer->channels[peer->channelCount]; ++channel) {
                enet_peer_reset_incoming_commands(&channel->incomingReliableCommands);
                enet_peer_reset_incoming_commands(&channel->incomingUnreliableCommands);
            }

            enet_free(peer->channels);
        }

        peer->channels     = NULL;
        peer->channelCount = 0;
    }

    void enet_peer_on_connect(ENetPeer *peer) {
        if (peer->state != ENET_PEER_STATE_CONNECTED && peer->state != ENET_PEER_STATE_DISCONNECT_LATER) {
            if (peer->incomingBandwidth != 0) {
                ++peer->host->bandwidthLimitedPeers;
            }

            ++peer->host->connectedPeers;
        }
    }

    void enet_peer_on_disconnect(ENetPeer *peer) {
        if (peer->state == ENET_PEER_STATE_CONNECTED || peer->state == ENET_PEER_STATE_DISCONNECT_LATER) {
            if (peer->incomingBandwidth != 0) {
                --peer->host->bandwidthLimitedPeers;
            }

            --peer->host->connectedPeers;
        }
    }

    /** Forcefully disconnects a peer.
     *  @param peer peer to forcefully disconnect
     *  @remarks The foreign host represented by the peer is not notified of the disconnection and will timeout
     *  on its connection to the local host.
     */
    void enet_peer_reset(ENetPeer *peer) {
        enet_peer_on_disconnect(peer);

        // We don't want to reset connectID here, otherwise, we can't get it in the Disconnect event
        // peer->connectID                     = 0;
        peer->outgoingPeerID                = ENET_PROTOCOL_MAXIMUM_PEER_ID;
        peer->state                         = ENET_PEER_STATE_DISCONNECTED;
        peer->incomingBandwidth             = 0;
        peer->outgoingBandwidth             = 0;
        peer->incomingBandwidthThrottleEpoch = 0;
        peer->outgoingBandwidthThrottleEpoch = 0;
        peer->incomingDataTotal             = 0;
        peer->totalDataReceived             = 0;
        peer->outgoingDataTotal             = 0;
        peer->totalDataSent                 = 0;
        peer->lastSendTime                  = 0;
        peer->lastReceiveTime               = 0;
        peer->nextTimeout                   = 0;
        peer->earliestTimeout               = 0;
        peer->packetLossEpoch               = 0;
        peer->packetsSent                   = 0;
        peer->totalPacketsSent              = 0;
        peer->packetsLost                   = 0;
        peer->totalPacketsLost              = 0;
        peer->packetLoss                    = 0;
        peer->packetLossVariance            = 0;
        peer->packetThrottle                = ENET_PEER_DEFAULT_PACKET_THROTTLE;
        peer->packetThrottleLimit           = ENET_PEER_PACKET_THROTTLE_SCALE;
        peer->packetThrottleCounter         = 0;
        peer->packetThrottleEpoch           = 0;
        peer->packetThrottleAcceleration    = ENET_PEER_PACKET_THROTTLE_ACCELERATION;
        peer->packetThrottleDeceleration    = ENET_PEER_PACKET_THROTTLE_DECELERATION;
        peer->packetThrottleInterval        = ENET_PEER_PACKET_THROTTLE_INTERVAL;
        peer->pingInterval                  = ENET_PEER_PING_INTERVAL;
        peer->timeoutLimit                  = ENET_PEER_TIMEOUT_LIMIT;
        peer->timeoutMinimum                = ENET_PEER_TIMEOUT_MINIMUM;
        peer->timeoutMaximum                = ENET_PEER_TIMEOUT_MAXIMUM;
        peer->lastRoundTripTime             = ENET_PEER_DEFAULT_ROUND_TRIP_TIME;
        peer->lowestRoundTripTime           = ENET_PEER_DEFAULT_ROUND_TRIP_TIME;
        peer->lastRoundTripTimeVariance     = 0;
        peer->highestRoundTripTimeVariance  = 0;
        peer->roundTripTime                 = ENET_PEER_DEFAULT_ROUND_TRIP_TIME;
        peer->roundTripTimeVariance         = 0;
        peer->mtu                           = peer->host->mtu;
        peer->reliableDataInTransit         = 0;
        peer->outgoingReliableSequenceNumber = 0;
        peer->windowSize                    = ENET_PROTOCOL_MAXIMUM_WINDOW_SIZE;
        peer->incomingUnsequencedGroup      = 0;
        peer->outgoingUnsequencedGroup      = 0;
        peer->eventData                     = 0;
        peer->totalWaitingData              = 0;

        memset(peer->unsequencedWindow, 0, sizeof(peer->unsequencedWindow));
        enet_peer_reset_queues(peer);
    }

    /** Sends a ping request to a peer.
     *  @param peer destination for the ping request
     *  @remarks ping requests factor into the mean round trip time as designated by the
     *  roundTripTime field in the ENetPeer structure.  ENet automatically pings all connected
     *  peers at regular intervals, however, this function may be called to ensure more
     *  frequent ping requests.
     */
    void enet_peer_ping(ENetPeer *peer) {
        ENetProtocol command;

        if (peer->state != ENET_PEER_STATE_CONNECTED) {
            return;
        }

        command.header.command   = ENET_PROTOCOL_COMMAND_PING | ENET_PROTOCOL_COMMAND_FLAG_ACKNOWLEDGE;
        command.header.channelID = 0xFF;

        enet_peer_queue_outgoing_command(peer, &command, NULL, 0, 0);
    }

    /** Sets the interval at which pings will be sent to a peer.
     *
     *  Pings are used both to monitor the liveness of the connection and also to dynamically
     *  adjust the throttle during periods of low traffic so that the throttle has reasonable
     *  responsiveness during traffic spikes.
     *
     *  @param peer the peer to adjust
     *  @param pingInterval the interval at which to send pings; defaults to ENET_PEER_PING_INTERVAL if 0
     */
    void enet_peer_ping_interval(ENetPeer *peer, enet_uint32 pingInterval) {
        peer->pingInterval = pingInterval ? pingInterval : (enet_uint32)ENET_PEER_PING_INTERVAL;
    }

    /** Sets the timeout parameters for a peer.
     *
     *  The timeout parameter control how and when a peer will timeout from a failure to acknowledge
     *  reliable traffic. Timeout values use an exponential backoff mechanism, where if a reliable
     *  packet is not acknowledge within some multiple of the average RTT plus a variance tolerance,
     *  the timeout will be doubled until it reaches a set limit. If the timeout is thus at this
     *  limit and reliable packets have been sent but not acknowledged within a certain minimum time
     *  period, the peer will be disconnected. Alternatively, if reliable packets have been sent
     *  but not acknowledged for a certain maximum time period, the peer will be disconnected regardless
     *  of the current timeout limit value.
     *
     *  @param peer the peer to adjust
     *  @param timeoutLimit the timeout limit; defaults to ENET_PEER_TIMEOUT_LIMIT if 0
     *  @param timeoutMinimum the timeout minimum; defaults to ENET_PEER_TIMEOUT_MINIMUM if 0
     *  @param timeoutMaximum the timeout maximum; defaults to ENET_PEER_TIMEOUT_MAXIMUM if 0
     */

    void enet_peer_timeout(ENetPeer *peer, enet_uint32 timeoutLimit, enet_uint32 timeoutMinimum, enet_uint32 timeoutMaximum) {
        peer->timeoutLimit   = timeoutLimit ? timeoutLimit : (enet_uint32)ENET_PEER_TIMEOUT_LIMIT;
        peer->timeoutMinimum = timeoutMinimum ? timeoutMinimum : (enet_uint32)ENET_PEER_TIMEOUT_MINIMUM;
        peer->timeoutMaximum = timeoutMaximum ? timeoutMaximum : (enet_uint32)ENET_PEER_TIMEOUT_MAXIMUM;
    }

    /** Force an immediate disconnection from a peer.
     *  @param peer peer to disconnect
     *  @param data data describing the disconnection
     *  @remarks No ENET_EVENT_DISCONNECT event will be generated. The foreign peer is not
     *  guaranteed to receive the disconnect notification, and is reset immediately upon
     *  return from this function.
     */
    void enet_peer_disconnect_now(ENetPeer *peer, enet_uint32 data) {
        ENetProtocol command;

        if (peer->state == ENET_PEER_STATE_DISCONNECTED) {
            return;
        }

        if (peer->state != ENET_PEER_STATE_ZOMBIE && peer->state != ENET_PEER_STATE_DISCONNECTING) {
            enet_peer_reset_queues(peer);

            command.header.command   = ENET_PROTOCOL_COMMAND_DISCONNECT | ENET_PROTOCOL_COMMAND_FLAG_UNSEQUENCED;
            command.header.channelID = 0xFF;
            command.disconnect.data  = ENET_HOST_TO_NET_32(data);

            enet_peer_queue_outgoing_command(peer, &command, NULL, 0, 0);
            enet_host_flush(peer->host);
        }

        enet_peer_reset(peer);
    }

    /** Request a disconnection from a peer.
     *  @param peer peer to request a disconnection
     *  @param data data describing the disconnection
     *  @remarks An ENET_EVENT_DISCONNECT event will be generated by enet_host_service()
     *  once the disconnection is complete.
     */
    void enet_peer_disconnect(ENetPeer *peer, enet_uint32 data) {
        ENetProtocol command;

        if (peer->state == ENET_PEER_STATE_DISCONNECTING ||
            peer->state == ENET_PEER_STATE_DISCONNECTED ||
            peer->state == ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT ||
            peer->state == ENET_PEER_STATE_ZOMBIE
        ) {
            return;
        }

        enet_peer_reset_queues(peer);

        command.header.command   = ENET_PROTOCOL_COMMAND_DISCONNECT;
        command.header.channelID = 0xFF;
        command.disconnect.data  = ENET_HOST_TO_NET_32(data);

        if (peer->state == ENET_PEER_STATE_CONNECTED || peer->state == ENET_PEER_STATE_DISCONNECT_LATER) {
            command.header.command |= ENET_PROTOCOL_COMMAND_FLAG_ACKNOWLEDGE;
        } else {
            command.header.command |= ENET_PROTOCOL_COMMAND_FLAG_UNSEQUENCED;
        }

        enet_peer_queue_outgoing_command(peer, &command, NULL, 0, 0);

        if (peer->state == ENET_PEER_STATE_CONNECTED || peer->state == ENET_PEER_STATE_DISCONNECT_LATER) {
            enet_peer_on_disconnect(peer);

            peer->state = ENET_PEER_STATE_DISCONNECTING;
        } else {
            enet_host_flush(peer->host);
            enet_peer_reset(peer);
        }
    }

    /** Request a disconnection from a peer, but only after all queued outgoing packets are sent.
     *  @param peer peer to request a disconnection
     *  @param data data describing the disconnection
     *  @remarks An ENET_EVENT_DISCONNECT event will be generated by enet_host_service()
     *  once the disconnection is complete.
     */
    void enet_peer_disconnect_later(ENetPeer *peer, enet_uint32 data) {
        if ((peer->state == ENET_PEER_STATE_CONNECTED || peer->state == ENET_PEER_STATE_DISCONNECT_LATER) &&
            !(enet_list_empty(&peer->outgoingReliableCommands) &&
            enet_list_empty(&peer->outgoingUnreliableCommands) &&
            enet_list_empty(&peer->sentReliableCommands))
        ) {
            peer->state     = ENET_PEER_STATE_DISCONNECT_LATER;
            peer->eventData = data;
        } else {
            enet_peer_disconnect(peer, data);
        }
    }

    ENetAcknowledgement *enet_peer_queue_acknowledgement(ENetPeer *peer, const ENetProtocol *command, enet_uint16 sentTime) {
        ENetAcknowledgement *acknowledgement;

        if (command->header.channelID < peer->channelCount) {
            ENetChannel *channel       = &peer->channels[command->header.channelID];
            enet_uint16 reliableWindow = command->header.reliableSequenceNumber / ENET_PEER_RELIABLE_WINDOW_SIZE;
            enet_uint16 currentWindow  = channel->incomingReliableSequenceNumber / ENET_PEER_RELIABLE_WINDOW_SIZE;

            if (command->header.reliableSequenceNumber < channel->incomingReliableSequenceNumber) {
                reliableWindow += ENET_PEER_RELIABLE_WINDOWS;
            }

            if (reliableWindow >= currentWindow + ENET_PEER_FREE_RELIABLE_WINDOWS - 1 && reliableWindow <= currentWindow + ENET_PEER_FREE_RELIABLE_WINDOWS) {
                return NULL;
            }
        }

        acknowledgement = (ENetAcknowledgement *) enet_malloc(sizeof(ENetAcknowledgement));
        if (acknowledgement == NULL) {
            return NULL;
        }

        peer->outgoingDataTotal += sizeof(ENetProtocolAcknowledge);

        acknowledgement->sentTime = sentTime;
        acknowledgement->command  = *command;

        enet_list_insert(enet_list_end(&peer->acknowledgements), acknowledgement);
        return acknowledgement;
    }

    void enet_peer_setup_outgoing_command(ENetPeer *peer, ENetOutgoingCommand *outgoingCommand) {
        ENetChannel *channel = &peer->channels[outgoingCommand->command.header.channelID];
        peer->outgoingDataTotal += enet_protocol_command_size(outgoingCommand->command.header.command) + outgoingCommand->fragmentLength;

        if (outgoingCommand->command.header.channelID == 0xFF) {
            ++peer->outgoingReliableSequenceNumber;

            outgoingCommand->reliableSequenceNumber   = peer->outgoingReliableSequenceNumber;
            outgoingCommand->unreliableSequenceNumber = 0;
        }
        else if (outgoingCommand->command.header.command & ENET_PROTOCOL_COMMAND_FLAG_ACKNOWLEDGE) {
            ++channel->outgoingReliableSequenceNumber;
            channel->outgoingUnreliableSequenceNumber = 0;

            outgoingCommand->reliableSequenceNumber   = channel->outgoingReliableSequenceNumber;
            outgoingCommand->unreliableSequenceNumber = 0;
        }
        else if (outgoingCommand->command.header.command & ENET_PROTOCOL_COMMAND_FLAG_UNSEQUENCED) {
            ++peer->outgoingUnsequencedGroup;

            outgoingCommand->reliableSequenceNumber   = 0;
            outgoingCommand->unreliableSequenceNumber = 0;
        }
        else {
            if (outgoingCommand->fragmentOffset == 0) {
                ++channel->outgoingUnreliableSequenceNumber;
            }

            outgoingCommand->reliableSequenceNumber   = channel->outgoingReliableSequenceNumber;
            outgoingCommand->unreliableSequenceNumber = channel->outgoingUnreliableSequenceNumber;
        }

        outgoingCommand->sendAttempts          = 0;
        outgoingCommand->sentTime              = 0;
        outgoingCommand->roundTripTimeout      = 0;
        outgoingCommand->roundTripTimeoutLimit = 0;
        outgoingCommand->command.header.reliableSequenceNumber = ENET_HOST_TO_NET_16(outgoingCommand->reliableSequenceNumber);

        switch (outgoingCommand->command.header.command & ENET_PROTOCOL_COMMAND_MASK) {
            case ENET_PROTOCOL_COMMAND_SEND_UNRELIABLE:
                outgoingCommand->command.sendUnreliable.unreliableSequenceNumber = ENET_HOST_TO_NET_16(outgoingCommand->unreliableSequenceNumber);
                break;

            case ENET_PROTOCOL_COMMAND_SEND_UNSEQUENCED:
                outgoingCommand->command.sendUnsequenced.unsequencedGroup = ENET_HOST_TO_NET_16(peer->outgoingUnsequencedGroup);
                break;

            default:
                break;
        }

        if (outgoingCommand->command.header.command & ENET_PROTOCOL_COMMAND_FLAG_ACKNOWLEDGE) {
            enet_list_insert(enet_list_end(&peer->outgoingReliableCommands), outgoingCommand);
        } else {
            enet_list_insert(enet_list_end(&peer->outgoingUnreliableCommands), outgoingCommand);
        }
    }

    ENetOutgoingCommand * enet_peer_queue_outgoing_command(ENetPeer *peer, const ENetProtocol *command, ENetPacket *packet, enet_uint32 offset, enet_uint16 length) {
        ENetOutgoingCommand *outgoingCommand = (ENetOutgoingCommand *) enet_malloc(sizeof(ENetOutgoingCommand));

        if (outgoingCommand == NULL) {
            return NULL;
        }

        outgoingCommand->command        = *command;
        outgoingCommand->fragmentOffset = offset;
        outgoingCommand->fragmentLength = length;
        outgoingCommand->packet         = packet;
        if (packet != NULL) {
            ++packet->referenceCount;
        }

        enet_peer_setup_outgoing_command(peer, outgoingCommand);
        return outgoingCommand;
    }

    void enet_peer_dispatch_incoming_unreliable_commands(ENetPeer *peer, ENetChannel *channel) {
        ENetListIterator droppedCommand, startCommand, currentCommand;

        for (droppedCommand = startCommand = currentCommand = enet_list_begin(&channel->incomingUnreliableCommands);
            currentCommand != enet_list_end(&channel->incomingUnreliableCommands);
            currentCommand = enet_list_next(currentCommand)
        ) {
            ENetIncomingCommand *incomingCommand = (ENetIncomingCommand *) currentCommand;

            if ((incomingCommand->command.header.command & ENET_PROTOCOL_COMMAND_MASK) == ENET_PROTOCOL_COMMAND_SEND_UNSEQUENCED) {
                continue;
            }

            if (incomingCommand->reliableSequenceNumber == channel->incomingReliableSequenceNumber) {
                if (incomingCommand->fragmentsRemaining <= 0) {
                    channel->incomingUnreliableSequenceNumber = incomingCommand->unreliableSequenceNumber;
                    continue;
                }

                if (startCommand != currentCommand) {
                    enet_list_move(enet_list_end(&peer->dispatchedCommands), startCommand, enet_list_previous(currentCommand));

                    if (!peer->needsDispatch) {
                        enet_list_insert(enet_list_end(&peer->host->dispatchQueue), &peer->dispatchList);
                        peer->needsDispatch = 1;
                    }

                    droppedCommand = currentCommand;
                } else if (droppedCommand != currentCommand) {
                    droppedCommand = enet_list_previous(currentCommand);
                }
            } else {
                enet_uint16 reliableWindow = incomingCommand->reliableSequenceNumber / ENET_PEER_RELIABLE_WINDOW_SIZE;
                enet_uint16 currentWindow  = channel->incomingReliableSequenceNumber / ENET_PEER_RELIABLE_WINDOW_SIZE;

                if (incomingCommand->reliableSequenceNumber < channel->incomingReliableSequenceNumber) {
                    reliableWindow += ENET_PEER_RELIABLE_WINDOWS;
                }

                if (reliableWindow >= currentWindow && reliableWindow < currentWindow + ENET_PEER_FREE_RELIABLE_WINDOWS - 1) {
                    break;
                }

                droppedCommand = enet_list_next(currentCommand);

                if (startCommand != currentCommand) {
                    enet_list_move(enet_list_end(&peer->dispatchedCommands), startCommand, enet_list_previous(currentCommand));

                    if (!peer->needsDispatch) {
                        enet_list_insert(enet_list_end(&peer->host->dispatchQueue), &peer->dispatchList);
                        peer->needsDispatch = 1;
                    }
                }
            }

            startCommand = enet_list_next(currentCommand);
        }

        if (startCommand != currentCommand) {
            enet_list_move(enet_list_end(&peer->dispatchedCommands), startCommand, enet_list_previous(currentCommand));

            if (!peer->needsDispatch) {
                enet_list_insert(enet_list_end(&peer->host->dispatchQueue), &peer->dispatchList);
                peer->needsDispatch = 1;
            }

            droppedCommand = currentCommand;
        }

        enet_peer_remove_incoming_commands(&channel->incomingUnreliableCommands,enet_list_begin(&channel->incomingUnreliableCommands), droppedCommand);
    }

    void enet_peer_dispatch_incoming_reliable_commands(ENetPeer *peer, ENetChannel *channel) {
        ENetListIterator currentCommand;

        for (currentCommand = enet_list_begin(&channel->incomingReliableCommands);
            currentCommand != enet_list_end(&channel->incomingReliableCommands);
            currentCommand = enet_list_next(currentCommand)
        ) {
            ENetIncomingCommand *incomingCommand = (ENetIncomingCommand *) currentCommand;

            if (incomingCommand->fragmentsRemaining > 0 || incomingCommand->reliableSequenceNumber != (enet_uint16) (channel->incomingReliableSequenceNumber + 1)) {
                break;
            }

            channel->incomingReliableSequenceNumber = incomingCommand->reliableSequenceNumber;

            if (incomingCommand->fragmentCount > 0) {
                channel->incomingReliableSequenceNumber += incomingCommand->fragmentCount - 1;
            }
        }

        if (currentCommand == enet_list_begin(&channel->incomingReliableCommands)) {
            return;
        }

        channel->incomingUnreliableSequenceNumber = 0;
        enet_list_move(enet_list_end(&peer->dispatchedCommands), enet_list_begin(&channel->incomingReliableCommands), enet_list_previous(currentCommand));

        if (!peer->needsDispatch) {
            enet_list_insert(enet_list_end(&peer->host->dispatchQueue), &peer->dispatchList);
            peer->needsDispatch = 1;
        }

        if (!enet_list_empty(&channel->incomingUnreliableCommands)) {
            enet_peer_dispatch_incoming_unreliable_commands(peer, channel);
        }
    }

    ENetIncomingCommand * enet_peer_queue_incoming_command(ENetPeer *peer, const ENetProtocol *command, const void *data, size_t dataLength, enet_uint32 flags, enet_uint32 fragmentCount) {
        static ENetIncomingCommand dummyCommand;

        ENetChannel *channel = &peer->channels[command->header.channelID];
        enet_uint32 unreliableSequenceNumber = 0, reliableSequenceNumber = 0;
        enet_uint16 reliableWindow, currentWindow;
        ENetIncomingCommand *incomingCommand;
        ENetListIterator currentCommand;
        ENetPacket *packet = NULL;

        if (peer->state == ENET_PEER_STATE_DISCONNECT_LATER) {
            goto discardCommand;
        }

        if ((command->header.command & ENET_PROTOCOL_COMMAND_MASK) != ENET_PROTOCOL_COMMAND_SEND_UNSEQUENCED) {
            reliableSequenceNumber = command->header.reliableSequenceNumber;
            reliableWindow         = reliableSequenceNumber / ENET_PEER_RELIABLE_WINDOW_SIZE;
            currentWindow = channel->incomingReliableSequenceNumber / ENET_PEER_RELIABLE_WINDOW_SIZE;

            if (reliableSequenceNumber < channel->incomingReliableSequenceNumber) {
                reliableWindow += ENET_PEER_RELIABLE_WINDOWS;
            }

            if (reliableWindow < currentWindow || reliableWindow >= currentWindow + ENET_PEER_FREE_RELIABLE_WINDOWS - 1) {
                goto discardCommand;
            }
        }

        switch (command->header.command & ENET_PROTOCOL_COMMAND_MASK) {
            case ENET_PROTOCOL_COMMAND_SEND_FRAGMENT:
            case ENET_PROTOCOL_COMMAND_SEND_RELIABLE:
                if (reliableSequenceNumber == channel->incomingReliableSequenceNumber) {
                    goto discardCommand;
                }

                for (currentCommand = enet_list_previous(enet_list_end(&channel->incomingReliableCommands));
                    currentCommand != enet_list_end(&channel->incomingReliableCommands);
                    currentCommand = enet_list_previous(currentCommand)
                ) {
                    incomingCommand = (ENetIncomingCommand *) currentCommand;

                    if (reliableSequenceNumber >= channel->incomingReliableSequenceNumber) {
                        if (incomingCommand->reliableSequenceNumber < channel->incomingReliableSequenceNumber) {
                            continue;
                        }
                    } else if (incomingCommand->reliableSequenceNumber >= channel->incomingReliableSequenceNumber) {
                        break;
                    }

                    if (incomingCommand->reliableSequenceNumber <= reliableSequenceNumber) {
                        if (incomingCommand->reliableSequenceNumber < reliableSequenceNumber) {
                            break;
                        }

                        goto discardCommand;
                    }
                }
                break;

            case ENET_PROTOCOL_COMMAND_SEND_UNRELIABLE:
            case ENET_PROTOCOL_COMMAND_SEND_UNRELIABLE_FRAGMENT:
                unreliableSequenceNumber = ENET_NET_TO_HOST_16(command->sendUnreliable.unreliableSequenceNumber);

                if (reliableSequenceNumber == channel->incomingReliableSequenceNumber && unreliableSequenceNumber <= channel->incomingUnreliableSequenceNumber) {
                    goto discardCommand;
                }

                for (currentCommand = enet_list_previous(enet_list_end(&channel->incomingUnreliableCommands));
                    currentCommand != enet_list_end(&channel->incomingUnreliableCommands);
                    currentCommand = enet_list_previous(currentCommand)
                ) {
                    incomingCommand = (ENetIncomingCommand *) currentCommand;

                    if ((command->header.command & ENET_PROTOCOL_COMMAND_MASK) == ENET_PROTOCOL_COMMAND_SEND_UNSEQUENCED) {
                        continue;
                    }

                    if (reliableSequenceNumber >= channel->incomingReliableSequenceNumber) {
                        if (incomingCommand->reliableSequenceNumber < channel->incomingReliableSequenceNumber) {
                            continue;
                        }
                    } else if (incomingCommand->reliableSequenceNumber >= channel->incomingReliableSequenceNumber) {
                        break;
                    }

                    if (incomingCommand->reliableSequenceNumber < reliableSequenceNumber) {
                        break;
                    }

                    if (incomingCommand->reliableSequenceNumber > reliableSequenceNumber) {
                        continue;
                    }

                    if (incomingCommand->unreliableSequenceNumber <= unreliableSequenceNumber) {
                        if (incomingCommand->unreliableSequenceNumber < unreliableSequenceNumber) {
                            break;
                        }

                        goto discardCommand;
                    }
                }
                break;

            case ENET_PROTOCOL_COMMAND_SEND_UNSEQUENCED:
                currentCommand = enet_list_end(&channel->incomingUnreliableCommands);
                break;

            default:
                goto discardCommand;
        }

        if (peer->totalWaitingData >= peer->host->maximumWaitingData) {
            goto notifyError;
        }

        packet = callbacks.packet_create(data, dataLength, flags);
        if (packet == NULL) {
            goto notifyError;
        }

        incomingCommand = (ENetIncomingCommand *) enet_malloc(sizeof(ENetIncomingCommand));
        if (incomingCommand == NULL) {
            goto notifyError;
        }

        incomingCommand->reliableSequenceNumber     = command->header.reliableSequenceNumber;
        incomingCommand->unreliableSequenceNumber   = unreliableSequenceNumber & 0xFFFF;
        incomingCommand->command                    = *command;
        incomingCommand->fragmentCount              = fragmentCount;
        incomingCommand->fragmentsRemaining         = fragmentCount;
        incomingCommand->packet                     = packet;
        incomingCommand->fragments                  = NULL;

        if (fragmentCount > 0) {
            if (fragmentCount <= ENET_PROTOCOL_MAXIMUM_FRAGMENT_COUNT) {
                incomingCommand->fragments = (enet_uint32 *) enet_malloc((fragmentCount + 31) / 32 * sizeof(enet_uint32));
            }

            if (incomingCommand->fragments == NULL) {
                enet_free(incomingCommand);

                goto notifyError;
            }

            memset(incomingCommand->fragments, 0, (fragmentCount + 31) / 32 * sizeof(enet_uint32));
        }

        assert(packet != NULL);
        ++packet->referenceCount;
        peer->totalWaitingData += packet->dataLength;

        enet_list_insert(enet_list_next(currentCommand), incomingCommand);

        switch (command->header.command & ENET_PROTOCOL_COMMAND_MASK) {
            case ENET_PROTOCOL_COMMAND_SEND_FRAGMENT:
            case ENET_PROTOCOL_COMMAND_SEND_RELIABLE:
                enet_peer_dispatch_incoming_reliable_commands(peer, channel);
                break;

            default:
                enet_peer_dispatch_incoming_unreliable_commands(peer, channel);
                break;
        }

        return incomingCommand;

    discardCommand:
        if (fragmentCount > 0) {
            goto notifyError;
        }

        if (packet != NULL  && packet->referenceCount == 0) {
            callbacks.packet_destroy(packet);
        }

        return &dummyCommand;

    notifyError:
        if (packet != NULL && packet->referenceCount == 0) {
            callbacks.packet_destroy(packet);
        }

        return NULL;
    } /* enet_peer_queue_incoming_command */

// =======================================================================//
// !
// ! Host
// !
// =======================================================================//

    /** Creates a host for communicating to peers.
     *
     *  @param address   the address at which other peers may connect to this host.  If NULL, then no peers may connect to the host.
     *  @param peerCount the maximum number of peers that should be allocated for the host.
     *  @param channelLimit the maximum number of channels allowed; if 0, then this is equivalent to ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT
     *  @param incomingBandwidth downstream bandwidth of the host in bytes/second; if 0, ENet will assume unlimited bandwidth.
     *  @param outgoingBandwidth upstream bandwidth of the host in bytes/second; if 0, ENet will assume unlimited bandwidth.
     *
     *  @returns the host on success and NULL on failure
     *
     *  @remarks ENet will strategically drop packets on specific sides of a connection between hosts
     *  to ensure the host's bandwidth is not overwhelmed.  The bandwidth parameters also determine
     *  the window size of a connection which limits the amount of reliable packets that may be in transit
     *  at any given time.
     */
    ENetHost * enet_host_create(const ENetAddress *address, size_t peerCount, size_t channelLimit, enet_uint32 incomingBandwidth, enet_uint32 outgoingBandwidth) {
        ENetHost *host;
        ENetPeer *currentPeer;

        if (peerCount > ENET_PROTOCOL_MAXIMUM_PEER_ID) {
            return NULL;
        }

        host = (ENetHost *) enet_malloc(sizeof(ENetHost));
        if (host == NULL) { return NULL; }
        memset(host, 0, sizeof(ENetHost));

        host->peers = (ENetPeer *) enet_malloc(peerCount * sizeof(ENetPeer));
        if (host->peers == NULL) {
            enet_free(host);
            return NULL;
        }

        memset(host->peers, 0, peerCount * sizeof(ENetPeer));

        host->socket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
        if (host->socket != ENET_SOCKET_NULL) {
            enet_socket_set_option (host->socket, ENET_SOCKOPT_IPV6_V6ONLY, 0);
        }

        if (host->socket == ENET_SOCKET_NULL || (address != NULL && enet_socket_bind(host->socket, address) < 0)) {
            if (host->socket != ENET_SOCKET_NULL) {
                enet_socket_destroy(host->socket);
            }

            enet_free(host->peers);
            enet_free(host);

            return NULL;
        }

        enet_socket_set_option(host->socket, ENET_SOCKOPT_NONBLOCK, 1);
        enet_socket_set_option(host->socket, ENET_SOCKOPT_BROADCAST, 1);
        enet_socket_set_option(host->socket, ENET_SOCKOPT_RCVBUF, ENET_HOST_RECEIVE_BUFFER_SIZE);
        enet_socket_set_option(host->socket, ENET_SOCKOPT_SNDBUF, ENET_HOST_SEND_BUFFER_SIZE);
        enet_socket_set_option(host->socket, ENET_SOCKOPT_IPV6_V6ONLY, 0);

        if (address != NULL && enet_socket_get_address(host->socket, &host->address) < 0) {
            host->address = *address;
        }

        if (!channelLimit || channelLimit > ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT) {
            channelLimit = ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT;
        }

        host->randomSeed                    = (enet_uint32) ((uintptr_t) host % UINT32_MAX);
        host->randomSeed                    += enet_host_random_seed();
        host->randomSeed                    = (host->randomSeed << 16) | (host->randomSeed >> 16);
        host->channelLimit                  = channelLimit;
        host->incomingBandwidth             = incomingBandwidth;
        host->outgoingBandwidth             = outgoingBandwidth;
        host->bandwidthThrottleEpoch        = 0;
        host->recalculateBandwidthLimits    = 0;
        host->mtu                           = ENET_HOST_DEFAULT_MTU;
        host->peerCount                     = peerCount;
        host->commandCount                  = 0;
        host->bufferCount                   = 0;
        host->checksum                      = NULL;
        host->receivedAddress.host          = ENET_HOST_ANY;
        host->receivedAddress.port          = 0;
        host->receivedData                  = NULL;
        host->receivedDataLength            = 0;
        host->totalSentData                 = 0;
        host->totalSentPackets              = 0;
        host->totalReceivedData             = 0;
        host->totalReceivedPackets          = 0;
        host->connectedPeers                = 0;
        host->bandwidthLimitedPeers         = 0;
        host->duplicatePeers                = ENET_PROTOCOL_MAXIMUM_PEER_ID;
        host->maximumPacketSize             = ENET_HOST_DEFAULT_MAXIMUM_PACKET_SIZE;
        host->maximumWaitingData            = ENET_HOST_DEFAULT_MAXIMUM_WAITING_DATA;
        host->compressor.context            = NULL;
        host->compressor.compress           = NULL;
        host->compressor.decompress         = NULL;
        host->compressor.destroy            = NULL;
        host->intercept                     = NULL;

        enet_list_clear(&host->dispatchQueue);

        for (currentPeer = host->peers; currentPeer < &host->peers[host->peerCount]; ++currentPeer) {
            currentPeer->host = host;
            currentPeer->incomingPeerID    = currentPeer - host->peers;
            currentPeer->outgoingSessionID = currentPeer->incomingSessionID = 0xFF;
            currentPeer->data = NULL;

            enet_list_clear(&currentPeer->acknowledgements);
            enet_list_clear(&currentPeer->sentReliableCommands);
            enet_list_clear(&currentPeer->sentUnreliableCommands);
            enet_list_clear(&currentPeer->outgoingReliableCommands);
            enet_list_clear(&currentPeer->outgoingUnreliableCommands);
            enet_list_clear(&currentPeer->dispatchedCommands);

            enet_peer_reset(currentPeer);
        }

        return host;
    } /* enet_host_create */

    /** Destroys the host and all resources associated with it.
     *  @param host pointer to the host to destroy
     */
    void enet_host_destroy(ENetHost *host) {
        ENetPeer *currentPeer;

        if (host == NULL) {
            return;
        }

        enet_socket_destroy(host->socket);

        for (currentPeer = host->peers; currentPeer < &host->peers[host->peerCount]; ++currentPeer) {
            enet_peer_reset(currentPeer);
        }

        if (host->compressor.context != NULL && host->compressor.destroy) {
            (*host->compressor.destroy)(host->compressor.context);
        }

        enet_free(host->peers);
        enet_free(host);
    }

    /** Initiates a connection to a foreign host.
     *  @param host host seeking the connection
     *  @param address destination for the connection
     *  @param channelCount number of channels to allocate
     *  @param data user data supplied to the receiving host
     *  @returns a peer representing the foreign host on success, NULL on failure
     *  @remarks The peer returned will have not completed the connection until enet_host_service()
     *  notifies of an ENET_EVENT_TYPE_CONNECT event for the peer.
     */
    ENetPeer * enet_host_connect(ENetHost *host, const ENetAddress *address, size_t channelCount, enet_uint32 data) {
        ENetPeer *currentPeer;
        ENetChannel *channel;
        ENetProtocol command;

        if (channelCount < ENET_PROTOCOL_MINIMUM_CHANNEL_COUNT) {
            channelCount = ENET_PROTOCOL_MINIMUM_CHANNEL_COUNT;
        } else if (channelCount > ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT) {
            channelCount = ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT;
        }

        for (currentPeer = host->peers; currentPeer < &host->peers[host->peerCount]; ++currentPeer) {
            if (currentPeer->state == ENET_PEER_STATE_DISCONNECTED) {
                break;
            }
        }

        if (currentPeer >= &host->peers[host->peerCount]) {
            return NULL;
        }

        currentPeer->channels = (ENetChannel *) enet_malloc(channelCount * sizeof(ENetChannel));
        if (currentPeer->channels == NULL) {
            return NULL;
        }

        currentPeer->channelCount = channelCount;
        currentPeer->state        = ENET_PEER_STATE_CONNECTING;
        currentPeer->address      = *address;
        currentPeer->connectID    = ++host->randomSeed;

        if (host->outgoingBandwidth == 0) {
            currentPeer->windowSize = ENET_PROTOCOL_MAXIMUM_WINDOW_SIZE;
        } else {
            currentPeer->windowSize = (host->outgoingBandwidth / ENET_PEER_WINDOW_SIZE_SCALE) * ENET_PROTOCOL_MINIMUM_WINDOW_SIZE;
        }

        if (currentPeer->windowSize < ENET_PROTOCOL_MINIMUM_WINDOW_SIZE) {
            currentPeer->windowSize = ENET_PROTOCOL_MINIMUM_WINDOW_SIZE;
        } else if (currentPeer->windowSize > ENET_PROTOCOL_MAXIMUM_WINDOW_SIZE) {
            currentPeer->windowSize = ENET_PROTOCOL_MAXIMUM_WINDOW_SIZE;
        }

        for (channel = currentPeer->channels; channel < &currentPeer->channels[channelCount]; ++channel) {
            channel->outgoingReliableSequenceNumber   = 0;
            channel->outgoingUnreliableSequenceNumber = 0;
            channel->incomingReliableSequenceNumber   = 0;
            channel->incomingUnreliableSequenceNumber = 0;

            enet_list_clear(&channel->incomingReliableCommands);
            enet_list_clear(&channel->incomingUnreliableCommands);

            channel->usedReliableWindows = 0;
            memset(channel->reliableWindows, 0, sizeof(channel->reliableWindows));
        }

        command.header.command                     = ENET_PROTOCOL_COMMAND_CONNECT | ENET_PROTOCOL_COMMAND_FLAG_ACKNOWLEDGE;
        command.header.channelID                   = 0xFF;
        command.connect.outgoingPeerID             = ENET_HOST_TO_NET_16(currentPeer->incomingPeerID);
        command.connect.incomingSessionID          = currentPeer->incomingSessionID;
        command.connect.outgoingSessionID          = currentPeer->outgoingSessionID;
        command.connect.mtu                        = ENET_HOST_TO_NET_32(currentPeer->mtu);
        command.connect.windowSize                 = ENET_HOST_TO_NET_32(currentPeer->windowSize);
        command.connect.channelCount               = ENET_HOST_TO_NET_32(channelCount);
        command.connect.incomingBandwidth          = ENET_HOST_TO_NET_32(host->incomingBandwidth);
        command.connect.outgoingBandwidth          = ENET_HOST_TO_NET_32(host->outgoingBandwidth);
        command.connect.packetThrottleInterval     = ENET_HOST_TO_NET_32(currentPeer->packetThrottleInterval);
        command.connect.packetThrottleAcceleration = ENET_HOST_TO_NET_32(currentPeer->packetThrottleAcceleration);
        command.connect.packetThrottleDeceleration = ENET_HOST_TO_NET_32(currentPeer->packetThrottleDeceleration);
        command.connect.connectID                  = currentPeer->connectID;
        command.connect.data                       = ENET_HOST_TO_NET_32(data);

        enet_peer_queue_outgoing_command(currentPeer, &command, NULL, 0, 0);

        return currentPeer;
    } /* enet_host_connect */

    /** Queues a packet to be sent to all peers associated with the host.
     *  @param host host on which to broadcast the packet
     *  @param channelID channel on which to broadcast
     *  @param packet packet to broadcast
     */
    void enet_host_broadcast(ENetHost *host, enet_uint8 channelID, ENetPacket *packet) {
        ENetPeer *currentPeer;

        for (currentPeer = host->peers; currentPeer < &host->peers[host->peerCount]; ++currentPeer) {
            if (currentPeer->state != ENET_PEER_STATE_CONNECTED) {
                continue;
            }

            enet_peer_send(currentPeer, channelID, packet);
        }

        if (packet->referenceCount == 0) {
            callbacks.packet_destroy(packet);
        }
    }

    /** Sends raw data to specified address. Useful when you want to send unconnected data using host's socket.         
     *  @param host host sending data
     *  @param address destination address
     *  @param data data pointer
     *  @param dataLength length of data to send
     *  @retval >=0 bytes sent
     *  @retval <0 error
     *  @sa enet_socket_send
     */
    int enet_host_send_raw(ENetHost *host, const ENetAddress* address, enet_uint8* data, size_t dataLength) {
        ENetBuffer buffer;
        buffer.data = data;
        buffer.dataLength = dataLength;
        return enet_socket_send(host->socket, address, &buffer, 1);
    }

    /** Sends raw data to specified address with extended arguments. Allows to send only part of data, handy for other programming languages.
     *  I.e. if you have data =- { 0, 1, 2, 3 } and call function as enet_host_send_raw_ex(data, 1, 2) then it will skip 1 byte and send 2 bytes { 1, 2 }.
     *  @param host host sending data
     *  @param address destination address
     *  @param data data pointer
     *  @param skipBytes number of bytes to skip from start of data
     *  @param bytesToSend number of bytes to send
     *  @retval >=0 bytes sent
     *  @retval <0 error
     *  @sa enet_socket_send
     */
    int enet_host_send_raw_ex(ENetHost *host, const ENetAddress* address, enet_uint8* data, size_t skipBytes, size_t bytesToSend) {
        ENetBuffer buffer;
        buffer.data = data + skipBytes;
        buffer.dataLength = bytesToSend;
        return enet_socket_send(host->socket, address, &buffer, 1);
    }

    /** Sets intercept callback for the host.
     *  @param host host to set a callback
     *  @param callback intercept callback
     */
    void enet_host_set_intercept(ENetHost *host, const ENetInterceptCallback callback) {
        host->intercept = callback;
    }

    /** Sets the packet compressor the host should use to compress and decompress packets.
     *  @param host host to enable or disable compression for
     *  @param compressor callbacks for for the packet compressor; if NULL, then compression is disabled
     */
    void enet_host_compress(ENetHost *host, const ENetCompressor *compressor) {
        if (host->compressor.context != NULL && host->compressor.destroy) {
            (*host->compressor.destroy)(host->compressor.context);
        }

        if (compressor) {
            host->compressor = *compressor;
        } else {
            host->compressor.context = NULL;
        }
    }

    /** Limits the maximum allowed channels of future incoming connections.
     *  @param host host to limit
     *  @param channelLimit the maximum number of channels allowed; if 0, then this is equivalent to ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT
     */
    void enet_host_channel_limit(ENetHost *host, size_t channelLimit) {
        if (!channelLimit || channelLimit > ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT) {
            channelLimit = ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT;
        }
        host->channelLimit = channelLimit;
    }

    /** Adjusts the bandwidth limits of a host.
     *  @param host host to adjust
     *  @param incomingBandwidth new incoming bandwidth
     *  @param outgoingBandwidth new outgoing bandwidth
     *  @remarks the incoming and outgoing bandwidth parameters are identical in function to those
     *  specified in enet_host_create().
     */
    void enet_host_bandwidth_limit(ENetHost *host, enet_uint32 incomingBandwidth, enet_uint32 outgoingBandwidth) {
        host->incomingBandwidth = incomingBandwidth;
        host->outgoingBandwidth = outgoingBandwidth;
        host->recalculateBandwidthLimits = 1;
    }

    void enet_host_bandwidth_throttle(ENetHost *host) {
        enet_uint32 timeCurrent       = enet_time_get();
        enet_uint32 elapsedTime       = timeCurrent - host->bandwidthThrottleEpoch;
        enet_uint32 peersRemaining    = (enet_uint32) host->connectedPeers;
        enet_uint32 dataTotal         = ~0;
        enet_uint32 bandwidth         = ~0;
        enet_uint32 throttle          = 0;
        enet_uint32 bandwidthLimit    = 0;

        int needsAdjustment = host->bandwidthLimitedPeers > 0 ? 1 : 0;
        ENetPeer *peer;
        ENetProtocol command;

        if (elapsedTime < ENET_HOST_BANDWIDTH_THROTTLE_INTERVAL) {
            return;
        }

        if (host->outgoingBandwidth == 0 && host->incomingBandwidth == 0) {
            return;
        }

        host->bandwidthThrottleEpoch = timeCurrent;

        if (peersRemaining == 0) {
            return;
        }

        if (host->outgoingBandwidth != 0) {
            dataTotal = 0;
            bandwidth = (host->outgoingBandwidth * elapsedTime) / 1000;

            for (peer = host->peers; peer < &host->peers[host->peerCount]; ++peer) {
                if (peer->state != ENET_PEER_STATE_CONNECTED && peer->state != ENET_PEER_STATE_DISCONNECT_LATER) {
                    continue;
                }

                dataTotal += peer->outgoingDataTotal;
            }
        }

        while (peersRemaining > 0 && needsAdjustment != 0) {
            needsAdjustment = 0;

            if (dataTotal <= bandwidth) {
                throttle = ENET_PEER_PACKET_THROTTLE_SCALE;
            } else {
                throttle = (bandwidth * ENET_PEER_PACKET_THROTTLE_SCALE) / dataTotal;
            }

            for (peer = host->peers; peer < &host->peers[host->peerCount]; ++peer) {
                enet_uint32 peerBandwidth;

                if ((peer->state != ENET_PEER_STATE_CONNECTED && peer->state != ENET_PEER_STATE_DISCONNECT_LATER) ||
                    peer->incomingBandwidth == 0 ||
                    peer->outgoingBandwidthThrottleEpoch == timeCurrent
                ) {
                    continue;
                }

                peerBandwidth = (peer->incomingBandwidth * elapsedTime) / 1000;
                if ((throttle * peer->outgoingDataTotal) / ENET_PEER_PACKET_THROTTLE_SCALE <= peerBandwidth) {
                    continue;
                }

                peer->packetThrottleLimit = (peerBandwidth * ENET_PEER_PACKET_THROTTLE_SCALE) / peer->outgoingDataTotal;

                if (peer->packetThrottleLimit == 0) {
                    peer->packetThrottleLimit = 1;
                }

                if (peer->packetThrottle > peer->packetThrottleLimit) {
                    peer->packetThrottle = peer->packetThrottleLimit;
                }

                peer->outgoingBandwidthThrottleEpoch = timeCurrent;

                peer->incomingDataTotal = 0;
                peer->outgoingDataTotal = 0;

                needsAdjustment = 1;
                --peersRemaining;
                bandwidth -= peerBandwidth;
                dataTotal -= peerBandwidth;
            }
        }

        if (peersRemaining > 0) {
            if (dataTotal <= bandwidth) {
                throttle = ENET_PEER_PACKET_THROTTLE_SCALE;
            } else {
                throttle = (bandwidth * ENET_PEER_PACKET_THROTTLE_SCALE) / dataTotal;
            }

            for (peer = host->peers;
              peer < &host->peers[host->peerCount];
              ++peer)
            {
                if ((peer->state != ENET_PEER_STATE_CONNECTED && peer->state != ENET_PEER_STATE_DISCONNECT_LATER) || peer->outgoingBandwidthThrottleEpoch == timeCurrent) {
                    continue;
                }

                peer->packetThrottleLimit = throttle;

                if (peer->packetThrottle > peer->packetThrottleLimit) {
                    peer->packetThrottle = peer->packetThrottleLimit;
                }

                peer->incomingDataTotal = 0;
                peer->outgoingDataTotal = 0;
            }
        }

        if (host->recalculateBandwidthLimits) {
            host->recalculateBandwidthLimits = 0;

            peersRemaining  = (enet_uint32) host->connectedPeers;
            bandwidth       = host->incomingBandwidth;
            needsAdjustment = 1;

            if (bandwidth == 0) {
                bandwidthLimit = 0;
            } else {
                while (peersRemaining > 0 && needsAdjustment != 0) {
                    needsAdjustment = 0;
                    bandwidthLimit  = bandwidth / peersRemaining;

                    for (peer = host->peers; peer < &host->peers[host->peerCount]; ++peer) {
                        if ((peer->state != ENET_PEER_STATE_CONNECTED && peer->state != ENET_PEER_STATE_DISCONNECT_LATER) ||
                            peer->incomingBandwidthThrottleEpoch == timeCurrent
                        ) {
                            continue;
                        }

                        if (peer->outgoingBandwidth > 0 && peer->outgoingBandwidth >= bandwidthLimit) {
                            continue;
                        }

                        peer->incomingBandwidthThrottleEpoch = timeCurrent;

                        needsAdjustment = 1;
                        --peersRemaining;
                        bandwidth -= peer->outgoingBandwidth;
                    }
                }
            }

            for (peer = host->peers; peer < &host->peers[host->peerCount]; ++peer) {
                if (peer->state != ENET_PEER_STATE_CONNECTED && peer->state != ENET_PEER_STATE_DISCONNECT_LATER) {
                    continue;
                }

                command.header.command   = ENET_PROTOCOL_COMMAND_BANDWIDTH_LIMIT | ENET_PROTOCOL_COMMAND_FLAG_ACKNOWLEDGE;
                command.header.channelID = 0xFF;
                command.bandwidthLimit.outgoingBandwidth = ENET_HOST_TO_NET_32(host->outgoingBandwidth);

                if (peer->incomingBandwidthThrottleEpoch == timeCurrent) {
                    command.bandwidthLimit.incomingBandwidth = ENET_HOST_TO_NET_32(peer->outgoingBandwidth);
                } else {
                    command.bandwidthLimit.incomingBandwidth = ENET_HOST_TO_NET_32(bandwidthLimit);
                }

                enet_peer_queue_outgoing_command(peer, &command, NULL, 0, 0);
            }
        }
    } /* enet_host_bandwidth_throttle */

// =======================================================================//
// !
// ! Time
// !
// =======================================================================//

    #ifdef _WIN32
        static LARGE_INTEGER getFILETIMEoffset() {
            SYSTEMTIME s;
            FILETIME f;
            LARGE_INTEGER t;

            s.wYear = 1970;
            s.wMonth = 1;
            s.wDay = 1;
            s.wHour = 0;
            s.wMinute = 0;
            s.wSecond = 0;
            s.wMilliseconds = 0;
            SystemTimeToFileTime(&s, &f);
            t.QuadPart = f.dwHighDateTime;
            t.QuadPart <<= 32;
            t.QuadPart |= f.dwLowDateTime;
            return (t);
        }
        int clock_gettime(int X, struct timespec *tv) {
            (void)X;
            LARGE_INTEGER t;
            FILETIME f;
            double microseconds;
            static LARGE_INTEGER offset;
            static double frequencyToMicroseconds;
            static int initialized = 0;
            static BOOL usePerformanceCounter = 0;

            if (!initialized) {
                LARGE_INTEGER performanceFrequency;
                initialized = 1;
                usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
                if (usePerformanceCounter) {
                    QueryPerformanceCounter(&offset);
                    frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
                } else {
                    offset = getFILETIMEoffset();
                    frequencyToMicroseconds = 10.;
                }
            }
            if (usePerformanceCounter) {
                QueryPerformanceCounter(&t);
            } else {
                GetSystemTimeAsFileTime(&f);
                t.QuadPart = f.dwHighDateTime;
                t.QuadPart <<= 32;
                t.QuadPart |= f.dwLowDateTime;
            }

            t.QuadPart -= offset.QuadPart;
            microseconds = (double)t.QuadPart / frequencyToMicroseconds;
            t.QuadPart = (LONGLONG)microseconds;
            tv->tv_sec = (long)(t.QuadPart / 1000000);
            tv->tv_nsec = t.QuadPart % 1000000 * 1000;
            return (0);
        }
    #elif __APPLE__ && __MAC_OS_X_VERSION_MIN_REQUIRED < 101200
        #define CLOCK_MONOTONIC 0

        int clock_gettime(int X, struct timespec *ts) {
            clock_serv_t cclock;
            mach_timespec_t mts;

            host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
            clock_get_time(cclock, &mts);
            mach_port_deallocate(mach_task_self(), cclock);

            ts->tv_sec = mts.tv_sec;
            ts->tv_nsec = mts.tv_nsec;

            return 0;
        }
    #endif

    enet_uint32 enet_time_get() {
        // TODO enet uses 32 bit timestamps. We should modify it to use
        // 64 bit timestamps, but this is not trivial since we'd end up
        // changing half the structs in enet. For now, retain 32 bits, but
        // use an offset so we don't run out of bits. Basically, the first
        // call of enet_time_get() will always return 1, and follow-up calls
        // indicate elapsed time since the first call.
        //
        // Note that we don't want to return 0 from the first call, in case
        // some part of enet uses 0 as a special value (meaning time not set
        // for example).
        static uint64_t start_time_ns = 0;

        struct timespec ts;
    #if defined(CLOCK_MONOTONIC_RAW)
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    #else
        clock_gettime(CLOCK_MONOTONIC, &ts);
    #endif

        static const uint64_t ns_in_s = 1000 * 1000 * 1000;
        static const uint64_t ns_in_ms = 1000 * 1000;
        uint64_t current_time_ns = ts.tv_nsec + (uint64_t)ts.tv_sec * ns_in_s;

        // Most of the time we just want to atomically read the start time. We
        // could just use a single CAS instruction instead of this if, but it
        // would be slower in the average case.
        //
        // Note that statics are auto-initialized to zero, and starting a thread
        // implies a memory barrier. So we know that whatever thread calls this,
        // it correctly sees the start_time_ns as 0 initially.
        uint64_t offset_ns = ENET_ATOMIC_READ(&start_time_ns);
        if (offset_ns == 0) {
            // We still need to CAS, since two different threads can get here
            // at the same time.
            //
            // We assume that current_time_ns is > 1ms.
            //
            // Set the value of the start_time_ns, such that the first timestamp
            // is at 1ms. This ensures 0 remains a special value.
            uint64_t want_value = current_time_ns - 1 * ns_in_ms;
            uint64_t old_value = ENET_ATOMIC_CAS(&start_time_ns, 0, want_value);
            offset_ns = old_value == 0 ? want_value : old_value;
        }

        uint64_t result_in_ns = current_time_ns - offset_ns;
        return (enet_uint32)(result_in_ns / ns_in_ms);
    }

    void enet_inaddr_map4to6(struct in_addr in, struct in6_addr *out)
    {
        if (in.s_addr == 0x00000000) { /* 0.0.0.0 */
            *out = enet_v6_anyaddr;
        } else if (in.s_addr == 0xFFFFFFFF) { /* 255.255.255.255 */
            *out = enet_v6_noaddr;
        } else {
            *out = enet_v4_anyaddr;
            out->s6_addr[10] = 0xFF;
            out->s6_addr[11] = 0xFF;
            out->s6_addr[12] = ((uint8_t *)&in.s_addr)[0];
            out->s6_addr[13] = ((uint8_t *)&in.s_addr)[1];
            out->s6_addr[14] = ((uint8_t *)&in.s_addr)[2];
            out->s6_addr[15] = ((uint8_t *)&in.s_addr)[3];
        }
    }
    void enet_inaddr_map6to4(const struct in6_addr *in, struct in_addr *out)
    {
        memset(out, 0, sizeof(struct in_addr));
        ((uint8_t *)&out->s_addr)[0] = in->s6_addr[12];
        ((uint8_t *)&out->s_addr)[1] = in->s6_addr[13];
        ((uint8_t *)&out->s_addr)[2] = in->s6_addr[14];
        ((uint8_t *)&out->s_addr)[3] = in->s6_addr[15];
    }

    int enet_in6addr_lookup_host(const char *name, bool nodns, ENetAddress *out) {
        struct addrinfo hints, *resultList = NULL, *result = NULL;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;

        if (nodns)
        {
            hints.ai_flags = AI_NUMERICHOST; /* prevent actual DNS lookups! */
        }

        if (getaddrinfo(name, NULL, &hints, &resultList) != 0) {
            freeaddrinfo(resultList);
            return -1;
        }

        for (result = resultList; result != NULL; result = result->ai_next) {
            if (result->ai_addr != NULL) {
                if (result->ai_family == AF_INET || (result->ai_family == AF_UNSPEC && result->ai_addrlen == sizeof(struct sockaddr_in))) {
                    enet_inaddr_map4to6(((struct sockaddr_in*)result->ai_addr)->sin_addr, &out->host);
                    out->sin6_scope_id = 0;
                    freeaddrinfo(resultList);
                    return 0;

                } else if (result->ai_family == AF_INET6 || (result->ai_family == AF_UNSPEC && result->ai_addrlen == sizeof(struct sockaddr_in6))) {
                    memcpy(&out->host, &((struct sockaddr_in6*)result->ai_addr)->sin6_addr, sizeof(struct in6_addr));
                    out->sin6_scope_id = (enet_uint16) ((struct sockaddr_in6*)result->ai_addr)->sin6_scope_id;
                    freeaddrinfo(resultList);
                    return 0;
                }
            }
        }
        freeaddrinfo(resultList);
        return -1;
    }

    int enet_address_set_host_ip_new(ENetAddress *address, const char *name) {
        return enet_in6addr_lookup_host(name, true, address);
    }

    int enet_address_set_host_new(ENetAddress *address, const char *name) {
        return enet_in6addr_lookup_host(name, false, address);
    }

    int enet_address_get_host_ip_new(const ENetAddress *address, char *name, size_t nameLength) {
        if (IN6_IS_ADDR_V4MAPPED(&address->host)) {
            struct in_addr buf;
            enet_inaddr_map6to4(&address->host, &buf);

            if (inet_ntop(AF_INET, &buf, name, nameLength) == NULL) {
                return -1;
            }
        }
        else {
            if (inet_ntop(AF_INET6, (void*)&address->host, name, nameLength) == NULL) {
                return -1;
            }
        }

        return 0;
    } /* enet_address_get_host_ip_new */

    int enet_address_get_host_new(const ENetAddress *address, char *name, size_t nameLength) {
        struct sockaddr_in6 sin;
        memset(&sin, 0, sizeof(struct sockaddr_in6));

        int err;


        sin.sin6_family = AF_INET6;
        sin.sin6_port = ENET_HOST_TO_NET_16 (address->port);
        sin.sin6_addr = address->host;
        sin.sin6_scope_id = address->sin6_scope_id;

        err = getnameinfo((struct sockaddr *) &sin, sizeof(sin), name, nameLength, NULL, 0, NI_NAMEREQD);
        if (!err) {
            if (name != NULL && nameLength > 0 && !memchr(name, '\0', nameLength)) {
                return -1;
            }
            return 0;
        }
        if (err != EAI_NONAME) {
            return -1;
        }

        return enet_address_get_host_ip_new(address, name, nameLength);
    } /* enet_address_get_host_new */

// =======================================================================//
// !
// ! Platform Specific (Unix)
// !
// =======================================================================//

    #ifndef _WIN32

        #if defined(__MINGW32__) && defined(ENET_MINGW_COMPAT)
        // inet_ntop/inet_pton for MinGW from http://mingw-users.1079350.n2.nabble.com/IPv6-getaddrinfo-amp-inet-ntop-td5891996.html
        const char *inet_ntop(int af, const void *src, char *dst, socklen_t cnt) {
            if (af == AF_INET) {
                struct sockaddr_in in;
                memset(&in, 0, sizeof(in));
                in.sin_family = AF_INET;
                memcpy(&in.sin_addr, src, sizeof(struct in_addr));
                getnameinfo((struct sockaddr *)&in, sizeof(struct sockaddr_in), dst, cnt, NULL, 0, NI_NUMERICHOST);
                return dst;
            }
            else if (af == AF_INET6) {
                struct sockaddr_in6 in;
                memset(&in, 0, sizeof(in));
                in.sin6_family = AF_INET6;
                memcpy(&in.sin6_addr, src, sizeof(struct in_addr6));
                getnameinfo((struct sockaddr *)&in, sizeof(struct sockaddr_in6), dst, cnt, NULL, 0, NI_NUMERICHOST);
                return dst;
            }

            return NULL;
        }

        #define NS_INADDRSZ  4
        #define NS_IN6ADDRSZ 16
        #define NS_INT16SZ   2

        int inet_pton4(const char *src, char *dst) {
            uint8_t tmp[NS_INADDRSZ], *tp;

            int saw_digit = 0;
            int octets = 0;
            *(tp = tmp) = 0;

            int ch;
            while ((ch = *src++) != '\0')
            {
                if (ch >= '0' && ch <= '9')
                {
                    uint32_t n = *tp * 10 + (ch - '0');

                    if (saw_digit && *tp == 0)
                        return 0;

                    if (n > 255)
                        return 0;

                    *tp = n;
                    if (!saw_digit)
                    {
                        if (++octets > 4)
                            return 0;
                        saw_digit = 1;
                    }
                }
                else if (ch == '.' && saw_digit)
                {
                    if (octets == 4)
                        return 0;
                    *++tp = 0;
                    saw_digit = 0;
                }
                else
                    return 0;
            }
            if (octets < 4)
                return 0;

            memcpy(dst, tmp, NS_INADDRSZ);

            return 1;
        }

        int inet_pton6(const char *src, char *dst) {
            static const char xdigits[] = "0123456789abcdef";
            uint8_t tmp[NS_IN6ADDRSZ];

            uint8_t *tp = (uint8_t*) memset(tmp, '\0', NS_IN6ADDRSZ);
            uint8_t *endp = tp + NS_IN6ADDRSZ;
            uint8_t *colonp = NULL;

            /* Leading :: requires some special handling. */
            if (*src == ':')
            {
                if (*++src != ':')
                    return 0;
            }

            const char *curtok = src;
            int saw_xdigit = 0;
            uint32_t val = 0;
            int ch;
            while ((ch = tolower(*src++)) != '\0')
            {
                const char *pch = strchr(xdigits, ch);
                if (pch != NULL)
                {
                    val <<= 4;
                    val |= (pch - xdigits);
                    if (val > 0xffff)
                        return 0;
                    saw_xdigit = 1;
                    continue;
                }
                if (ch == ':')
                {
                    curtok = src;
                    if (!saw_xdigit)
                    {
                        if (colonp)
                            return 0;
                        colonp = tp;
                        continue;
                    }
                    else if (*src == '\0')
                    {
                        return 0;
                    }
                    if (tp + NS_INT16SZ > endp)
                        return 0;
                    *tp++ = (uint8_t) (val >> 8) & 0xff;
                    *tp++ = (uint8_t) val & 0xff;
                    saw_xdigit = 0;
                    val = 0;
                    continue;
                }
                if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) &&
                        inet_pton4(curtok, (char*) tp) > 0)
                {
                    tp += NS_INADDRSZ;
                    saw_xdigit = 0;
                    break; /* '\0' was seen by inet_pton4(). */
                }
                return 0;
            }
            if (saw_xdigit)
            {
                if (tp + NS_INT16SZ > endp)
                    return 0;
                *tp++ = (uint8_t) (val >> 8) & 0xff;
                *tp++ = (uint8_t) val & 0xff;
            }
            if (colonp != NULL)
            {
                /*
                 * Since some memmove()'s erroneously fail to handle
                 * overlapping regions, we'll do the shift by hand.
                 */
                const int n = tp - colonp;

                if (tp == endp)
                    return 0;

                for (int i = 1; i <= n; i++)
                {
                    endp[-i] = colonp[n - i];
                    colonp[n - i] = 0;
                }
                tp = endp;
            }
            if (tp != endp)
                return 0;

            memcpy(dst, tmp, NS_IN6ADDRSZ);

            return 1;
        }


        int inet_pton(int af, const char *src, struct in6_addr *dst) {
            switch (af)
            {
            case AF_INET:
                return inet_pton4(src, (char *)dst);
            case AF_INET6:
                return inet_pton6(src, (char *)dst);
            default:
                return -1;
            }
        }
    #endif // __MINGW__

    int enet_initialize(void) {
        return 0;
    }

    void enet_deinitialize(void) {}

    enet_uint64 enet_host_random_seed(void) {
        return (enet_uint64) time(NULL);
    }

    int enet_address_set_host_ip_old(ENetAddress *address, const char *name) {
        if (!inet_pton(AF_INET6, name, &address->host)) {
            return -1;
        }

        return 0;
    }

    int enet_address_set_host_old(ENetAddress *address, const char *name) {
        struct addrinfo hints, *resultList = NULL, *result = NULL;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;

        if (getaddrinfo(name, NULL, &hints, &resultList) != 0) {
            return -1;
        }

        for (result = resultList; result != NULL; result = result->ai_next) {
            if (result->ai_addr != NULL && result->ai_addrlen >= sizeof(struct sockaddr_in)) {
                if (result->ai_family == AF_INET) {
                    struct sockaddr_in * sin = (struct sockaddr_in *) result->ai_addr;

                    ((uint32_t *)&address->host.s6_addr)[0] = 0;
                    ((uint32_t *)&address->host.s6_addr)[1] = 0;
                    ((uint32_t *)&address->host.s6_addr)[2] = htonl(0xffff);
                    ((uint32_t *)&address->host.s6_addr)[3] = sin->sin_addr.s_addr;

                    freeaddrinfo(resultList);
                    return 0;
                }
                else if(result->ai_family == AF_INET6) {
                    struct sockaddr_in6 * sin = (struct sockaddr_in6 *)result->ai_addr;

                    address->host = sin->sin6_addr;
                    address->sin6_scope_id = sin->sin6_scope_id;

                    freeaddrinfo(resultList);
                    return 0;
                }
            }
        }
        freeaddrinfo(resultList);

        return enet_address_set_host_ip(address, name);
    } /* enet_address_set_host_old */

    int enet_address_get_host_ip_old(const ENetAddress *address, char *name, size_t nameLength) {
        if (inet_ntop(AF_INET6, &address->host, name, nameLength) == NULL) {
            return -1;
        }

        return 0;
    }

    int enet_address_get_host_old(const ENetAddress *address, char *name, size_t nameLength) {
        struct sockaddr_in6 sin;
        int err;
        memset(&sin, 0, sizeof(struct sockaddr_in6));
        sin.sin6_family = AF_INET6;
        sin.sin6_port = ENET_HOST_TO_NET_16 (address->port);
        sin.sin6_addr = address->host;
        sin.sin6_scope_id = address->sin6_scope_id;
        err = getnameinfo((struct sockaddr *) &sin, sizeof(sin), name, nameLength, NULL, 0, NI_NAMEREQD);
        if (!err) {
            if (name != NULL && nameLength > 0 && !memchr(name, '\0', nameLength)) {
                return -1;
            }
            return 0;
        }
        if (err != EAI_NONAME) {
            return -1;
        }
        return enet_address_get_host_ip(address, name, nameLength);
    } /* enet_address_get_host_old */

    int enet_socket_bind(ENetSocket socket, const ENetAddress *address) {
        struct sockaddr_in6 sin;
        memset(&sin, 0, sizeof(struct sockaddr_in6));
        sin.sin6_family = AF_INET6;

        if (address != NULL) {
            sin.sin6_port       = ENET_HOST_TO_NET_16(address->port);
            sin.sin6_addr       = address->host;
            sin.sin6_scope_id   = address->sin6_scope_id;
        } else {
            sin.sin6_port       = 0;
            sin.sin6_addr       = ENET_HOST_ANY;
            sin.sin6_scope_id   = 0;
        }

        return bind(socket, (struct sockaddr *)&sin, sizeof(struct sockaddr_in6));
    }

    int enet_socket_get_address(ENetSocket socket, ENetAddress *address) {
        struct sockaddr_in6 sin;
        socklen_t sinLength = sizeof(struct sockaddr_in6);

        if (getsockname(socket, (struct sockaddr *) &sin, &sinLength) == -1) {
            return -1;
        }

        address->host           = sin.sin6_addr;
        address->port           = ENET_NET_TO_HOST_16(sin.sin6_port);
        address->sin6_scope_id  = sin.sin6_scope_id;

        return 0;
    }

    int enet_socket_listen(ENetSocket socket, int backlog) {
        return listen(socket, backlog < 0 ? SOMAXCONN : backlog);
    }

    ENetSocket enet_socket_create(ENetSocketType type) {
        return socket(PF_INET6, (int)type == ENET_SOCKET_TYPE_DATAGRAM ? SOCK_DGRAM : SOCK_STREAM, 0);
    }

    int enet_socket_set_option(ENetSocket socket, ENetSocketOption option, int value) {
        int result = -1;

        switch (option) {
            case ENET_SOCKOPT_NONBLOCK:
                result = fcntl(socket, F_SETFL, (value ? O_NONBLOCK : 0) | (fcntl(socket, F_GETFL) & ~O_NONBLOCK));
                break;

            case ENET_SOCKOPT_BROADCAST:
                result = setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (char *)&value, sizeof(int));
                break;

            case ENET_SOCKOPT_REUSEADDR:
                result = setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char *)&value, sizeof(int));
                break;

            case ENET_SOCKOPT_RCVBUF:
                result = setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (char *)&value, sizeof(int));
                break;

            case ENET_SOCKOPT_SNDBUF:
                result = setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char *)&value, sizeof(int));
                break;

            case ENET_SOCKOPT_RCVTIMEO: {
                struct timeval timeVal;
                timeVal.tv_sec  = value / 1000;
                timeVal.tv_usec = (value % 1000) * 1000;
                result = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeVal, sizeof(struct timeval));
                break;
            }

            case ENET_SOCKOPT_SNDTIMEO: {
                struct timeval timeVal;
                timeVal.tv_sec  = value / 1000;
                timeVal.tv_usec = (value % 1000) * 1000;
                result = setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeVal, sizeof(struct timeval));
                break;
            }

            case ENET_SOCKOPT_NODELAY:
                result = setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char *)&value, sizeof(int));
                break;

            case ENET_SOCKOPT_IPV6_V6ONLY:
                result = setsockopt(socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&value, sizeof(int));
                break;

            default:
                break;
        }
        return result == -1 ? -1 : 0;
    } /* enet_socket_set_option */

    int enet_socket_get_option(ENetSocket socket, ENetSocketOption option, int *value) {
        int result = -1;
        socklen_t len;

        switch (option) {
            case ENET_SOCKOPT_ERROR:
                len    = sizeof(int);
                result = getsockopt(socket, SOL_SOCKET, SO_ERROR, value, &len);
                break;

            default:
                break;
        }
        return result == -1 ? -1 : 0;
    }

    int enet_socket_connect(ENetSocket socket, const ENetAddress *address) {
        struct sockaddr_in6 sin;
        int result;

        memset(&sin, 0, sizeof(struct sockaddr_in6));

        sin.sin6_family     = AF_INET6;
        sin.sin6_port       = ENET_HOST_TO_NET_16(address->port);
        sin.sin6_addr       = address->host;
        sin.sin6_scope_id   = address->sin6_scope_id;

        result = connect(socket, (struct sockaddr *)&sin, sizeof(struct sockaddr_in6));
        if (result == -1 && errno == EINPROGRESS) {
            return 0;
        }

        return result;
    }

    ENetSocket enet_socket_accept(ENetSocket socket, ENetAddress *address) {
        int result;
        struct sockaddr_in6 sin;
        socklen_t sinLength = sizeof(struct sockaddr_in6);

        result = accept(socket,address != NULL ? (struct sockaddr *) &sin : NULL, address != NULL ? &sinLength : NULL);

        if (result == -1) {
            return ENET_SOCKET_NULL;
        }

        if (address != NULL) {
            address->host = sin.sin6_addr;
            address->port = ENET_NET_TO_HOST_16 (sin.sin6_port);
            address->sin6_scope_id = sin.sin6_scope_id;
        }

        return result;
    }

    int enet_socket_shutdown(ENetSocket socket, ENetSocketShutdown how) {
        return shutdown(socket, (int) how);
    }

    void enet_socket_destroy(ENetSocket socket) {
        if (socket != -1) {
            close(socket);
        }
    }

    int enet_socket_send(ENetSocket socket, const ENetAddress *address, const ENetBuffer *buffers, size_t bufferCount) {
        struct msghdr msgHdr;
        struct sockaddr_in6 sin;
        int sentLength;

        memset(&msgHdr, 0, sizeof(struct msghdr));

        if (address != NULL) {
            memset(&sin, 0, sizeof(struct sockaddr_in6));

            sin.sin6_family     = AF_INET6;
            sin.sin6_port       = ENET_HOST_TO_NET_16(address->port);
            sin.sin6_addr       = address->host;
            sin.sin6_scope_id   = address->sin6_scope_id;

            msgHdr.msg_name    = &sin;
            msgHdr.msg_namelen = sizeof(struct sockaddr_in6);
        }

        msgHdr.msg_iov    = (struct iovec *) buffers;
        msgHdr.msg_iovlen = bufferCount;

        sentLength = sendmsg(socket, &msgHdr, MSG_NOSIGNAL);

        if (sentLength == -1) {
            if (errno == EWOULDBLOCK) {
                return 0;
            }

            return -1;
        }

        return sentLength;
    } /* enet_socket_send */

    int enet_socket_receive(ENetSocket socket, ENetAddress *address, ENetBuffer *buffers, size_t bufferCount) {
        struct msghdr msgHdr;
        struct sockaddr_in6 sin;
        int recvLength;

        memset(&msgHdr, 0, sizeof(struct msghdr));

        if (address != NULL) {
            msgHdr.msg_name    = &sin;
            msgHdr.msg_namelen = sizeof(struct sockaddr_in6);
        }

        msgHdr.msg_iov    = (struct iovec *) buffers;
        msgHdr.msg_iovlen = bufferCount;

        recvLength = recvmsg(socket, &msgHdr, MSG_NOSIGNAL);

        if (recvLength == -1) {
            if (errno == EWOULDBLOCK) {
                return 0;
            }

            return -1;
        }

        if (msgHdr.msg_flags & MSG_TRUNC) {
            return -1;
        }

        if (address != NULL) {
            address->host           = sin.sin6_addr;
            address->port           = ENET_NET_TO_HOST_16(sin.sin6_port);
            address->sin6_scope_id  = sin.sin6_scope_id;
        }

        return recvLength;
    } /* enet_socket_receive */

    int enet_socketset_select(ENetSocket maxSocket, ENetSocketSet *readSet, ENetSocketSet *writeSet, enet_uint32 timeout) {
        struct timeval timeVal;

        timeVal.tv_sec  = timeout / 1000;
        timeVal.tv_usec = (timeout % 1000) * 1000;

        return select(maxSocket + 1, readSet, writeSet, NULL, &timeVal);
    }

    int enet_socket_wait(ENetSocket socket, enet_uint32 *condition, enet_uint64 timeout) {
        struct pollfd pollSocket;
        int pollCount;

        pollSocket.fd     = socket;
        pollSocket.events = 0;

        if (*condition & ENET_SOCKET_WAIT_SEND) {
            pollSocket.events |= POLLOUT;
        }

        if (*condition & ENET_SOCKET_WAIT_RECEIVE) {
            pollSocket.events |= POLLIN;
        }

        pollCount = poll(&pollSocket, 1, timeout);

        if (pollCount < 0) {
            if (errno == EINTR && *condition & ENET_SOCKET_WAIT_INTERRUPT) {
                *condition = ENET_SOCKET_WAIT_INTERRUPT;

                return 0;
            }

            return -1;
        }

        *condition = ENET_SOCKET_WAIT_NONE;

        if (pollCount == 0) {
            return 0;
        }

        if (pollSocket.revents & POLLOUT) {
            *condition |= ENET_SOCKET_WAIT_SEND;
        }

        if (pollSocket.revents & POLLIN) {
            *condition |= ENET_SOCKET_WAIT_RECEIVE;
        }

        return 0;
    } /* enet_socket_wait */

    #endif // !_WIN32


// =======================================================================//
// !
// ! Platform Specific (Win)
// !
// =======================================================================//

    #ifdef _WIN32

    int enet_initialize(void) {
        WORD versionRequested = MAKEWORD(1, 1);
        WSADATA wsaData;

        if (WSAStartup(versionRequested, &wsaData)) {
            return -1;
        }

        if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1) {
            WSACleanup();
            return -1;
        }

        timeBeginPeriod(1);
        return 0;
    }

    void enet_deinitialize(void) {
        timeEndPeriod(1);
        WSACleanup();
    }

    enet_uint64 enet_host_random_seed(void) {
        return (enet_uint64) timeGetTime();
    }

    int enet_address_set_host_ip_old(ENetAddress *address, const char *name) {
        enet_uint8 vals[4] = { 0, 0, 0, 0 };
        int i;

        for (i = 0; i < 4; ++i) {
            const char *next = name + 1;
            if (*name != '0') {
                long val = strtol(name, (char **) &next, 10);
                if (val < 0 || val > 255 || next == name || next - name > 3) {
                    return -1;
                }
                vals[i] = (enet_uint8) val;
            }

            if (*next != (i < 3 ? '.' : '\0')) {
                return -1;
            }
            name = next + 1;
        }

        memcpy(&address->host, vals, sizeof(enet_uint32));
        return 0;
    }

    int enet_address_set_host_old(ENetAddress *address, const char *name) {
        struct hostent *hostEntry = NULL;
        hostEntry = gethostbyname(name);

        if (hostEntry == NULL || hostEntry->h_addrtype != AF_INET) {
            if (!inet_pton(AF_INET6, name, &address->host)) {
                return -1;
            }

            return 0;
        }

        ((enet_uint32 *)&address->host.s6_addr)[0] = 0;
        ((enet_uint32 *)&address->host.s6_addr)[1] = 0;
        ((enet_uint32 *)&address->host.s6_addr)[2] = htonl(0xffff);
        ((enet_uint32 *)&address->host.s6_addr)[3] = *(enet_uint32 *)hostEntry->h_addr_list[0];

        return 0;
    }

    int enet_address_get_host_ip_old(const ENetAddress *address, char *name, size_t nameLength) {
        if (inet_ntop(AF_INET6, (PVOID)&address->host, name, nameLength) == NULL) {
            return -1;
        }

        return 0;
    }

    int enet_address_get_host_old(const ENetAddress *address, char *name, size_t nameLength) {
        struct in6_addr in;
        struct hostent *hostEntry = NULL;
        in = address->host;
        hostEntry = gethostbyaddr((char *)&in, sizeof(struct in6_addr), AF_INET6);
        if (hostEntry == NULL) {
            return enet_address_get_host_ip(address, name, nameLength);
        } else {
            size_t hostLen = strlen(hostEntry->h_name);
            if (hostLen >= nameLength) {
                return -1;
            }
            memcpy(name, hostEntry->h_name, hostLen + 1);
        }
        return 0;
    }

    int enet_socket_bind(ENetSocket socket, const ENetAddress *address) {
        struct sockaddr_in6 sin;
        memset(&sin, 0, sizeof(struct sockaddr_in6));
        sin.sin6_family = AF_INET6;

        if (address != NULL) {
            sin.sin6_port       = ENET_HOST_TO_NET_16 (address->port);
            sin.sin6_addr       = address->host;
            sin.sin6_scope_id   = address->sin6_scope_id;
        } else   {
            sin.sin6_port       = 0;
            sin.sin6_addr       = in6addr_any;
            sin.sin6_scope_id   = 0;
        }

        return bind(socket, (struct sockaddr *) &sin, sizeof(struct sockaddr_in6)) == SOCKET_ERROR ? -1 : 0;
    }

    int enet_socket_get_address(ENetSocket socket, ENetAddress *address) {
        struct sockaddr_in6 sin;
        int sinLength = sizeof(struct sockaddr_in6);

        if (getsockname(socket, (struct sockaddr *) &sin, &sinLength) == -1) {
            return -1;
        }

        address->host           = sin.sin6_addr;
        address->port           = ENET_NET_TO_HOST_16(sin.sin6_port);
        address->sin6_scope_id  = sin.sin6_scope_id;

        return 0;
    }

    int enet_socket_listen(ENetSocket socket, int backlog) {
        return listen(socket, backlog < 0 ? SOMAXCONN : backlog) == SOCKET_ERROR ? -1 : 0;
    }

    ENetSocket enet_socket_create(ENetSocketType type) {
        return socket(PF_INET6, type == ENET_SOCKET_TYPE_DATAGRAM ? SOCK_DGRAM : SOCK_STREAM, 0);
    }

    int enet_socket_set_option(ENetSocket socket, ENetSocketOption option, int value) {
        int result = SOCKET_ERROR;

        switch (option) {
            case ENET_SOCKOPT_NONBLOCK: {
                u_long nonBlocking = (u_long) value;
                result = ioctlsocket(socket, FIONBIO, &nonBlocking);
                break;
            }

            case ENET_SOCKOPT_BROADCAST:
                result = setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (char *)&value, sizeof(int));
                break;

            case ENET_SOCKOPT_REUSEADDR:
                result = setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char *)&value, sizeof(int));
                break;

            case ENET_SOCKOPT_RCVBUF:
                result = setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (char *)&value, sizeof(int));
                break;

            case ENET_SOCKOPT_SNDBUF:
                result = setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char *)&value, sizeof(int));
                break;

            case ENET_SOCKOPT_RCVTIMEO:
                result = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&value, sizeof(int));
                break;

            case ENET_SOCKOPT_SNDTIMEO:
                result = setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&value, sizeof(int));
                break;

            case ENET_SOCKOPT_NODELAY:
                result = setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char *)&value, sizeof(int));
                break;

            case ENET_SOCKOPT_IPV6_V6ONLY:
                result = setsockopt(socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&value, sizeof(int));
                break;

            default:
                break;
        }
        return result == SOCKET_ERROR ? -1 : 0;
    } /* enet_socket_set_option */

    int enet_socket_get_option(ENetSocket socket, ENetSocketOption option, int *value) {
        int result = SOCKET_ERROR, len;

        switch (option) {
            case ENET_SOCKOPT_ERROR:
                len    = sizeof(int);
                result = getsockopt(socket, SOL_SOCKET, SO_ERROR, (char *)value, &len);
                break;

            default:
                break;
        }
        return result == SOCKET_ERROR ? -1 : 0;
    }

    int enet_socket_connect(ENetSocket socket, const ENetAddress *address) {
        struct sockaddr_in6 sin;
        int result;

        memset(&sin, 0, sizeof(struct sockaddr_in6));

        sin.sin6_family     = AF_INET6;
        sin.sin6_port       = ENET_HOST_TO_NET_16(address->port);
        sin.sin6_addr       = address->host;
        sin.sin6_scope_id   = address->sin6_scope_id;

        result = connect(socket, (struct sockaddr *) &sin, sizeof(struct sockaddr_in6));
        if (result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
            return -1;
        }

        return 0;
    }

    ENetSocket enet_socket_accept(ENetSocket socket, ENetAddress *address) {
        SOCKET result;
        struct sockaddr_in6 sin;
        int sinLength = sizeof(struct sockaddr_in6);

        result = accept(socket, address != NULL ? (struct sockaddr *)&sin : NULL, address != NULL ? &sinLength : NULL);

        if (result == INVALID_SOCKET) {
            return ENET_SOCKET_NULL;
        }

        if (address != NULL) {
            address->host           = sin.sin6_addr;
            address->port           = ENET_NET_TO_HOST_16(sin.sin6_port);
            address->sin6_scope_id  = sin.sin6_scope_id;
        }

        return result;
    }

    int enet_socket_shutdown(ENetSocket socket, ENetSocketShutdown how) {
        return shutdown(socket, (int) how) == SOCKET_ERROR ? -1 : 0;
    }

    void enet_socket_destroy(ENetSocket socket) {
        if (socket != INVALID_SOCKET) {
            closesocket(socket);
        }
    }

    int enet_socket_send(ENetSocket socket, const ENetAddress *address, const ENetBuffer *buffers, size_t bufferCount) {
        struct sockaddr_in6 sin;
        DWORD sentLength;

        if (address != NULL) {
            memset(&sin, 0, sizeof(struct sockaddr_in6));

            sin.sin6_family     = AF_INET6;
            sin.sin6_port       = ENET_HOST_TO_NET_16(address->port);
            sin.sin6_addr       = address->host;
            sin.sin6_scope_id   = address->sin6_scope_id;
        }

        if (WSASendTo(socket,
            (LPWSABUF) buffers,
            (DWORD) bufferCount,
            &sentLength,
            0,
            address != NULL ? (struct sockaddr *) &sin : NULL,
            address != NULL ? sizeof(struct sockaddr_in6) : 0,
            NULL,
            NULL) == SOCKET_ERROR
        ) {
            return (WSAGetLastError() == WSAEWOULDBLOCK) ? 0 : -1;
        }

        return (int) sentLength;
    }

    int enet_socket_receive(ENetSocket socket, ENetAddress *address, ENetBuffer *buffers, size_t bufferCount) {
        INT sinLength = sizeof(struct sockaddr_in6);
        DWORD flags   = 0, recvLength;
        struct sockaddr_in6 sin;

        if (WSARecvFrom(socket,
            (LPWSABUF) buffers,
            (DWORD) bufferCount,
            &recvLength,
            &flags,
            address != NULL ? (struct sockaddr *) &sin : NULL,
            address != NULL ? &sinLength : NULL,
            NULL,
            NULL) == SOCKET_ERROR
        ) {
            switch (WSAGetLastError()) {
                case WSAEWOULDBLOCK:
                case WSAECONNRESET:
                    return 0;
            }

            return -1;
        }

        if (flags & MSG_PARTIAL) {
            return -1;
        }

        if (address != NULL) {
            address->host           = sin.sin6_addr;
            address->port           = ENET_NET_TO_HOST_16(sin.sin6_port);
            address->sin6_scope_id  = sin.sin6_scope_id;
        }

        return (int) recvLength;
    } /* enet_socket_receive */

    int enet_socketset_select(ENetSocket maxSocket, ENetSocketSet *readSet, ENetSocketSet *writeSet, enet_uint32 timeout) {
        struct timeval timeVal;

        timeVal.tv_sec  = timeout / 1000;
        timeVal.tv_usec = (timeout % 1000) * 1000;

        return select(maxSocket + 1, readSet, writeSet, NULL, &timeVal);
    }

    int enet_socket_wait(ENetSocket socket, enet_uint32 *condition, enet_uint64 timeout) {
        fd_set readSet, writeSet;
        struct timeval timeVal;
        int selectCount;

        timeVal.tv_sec  = timeout / 1000;
        timeVal.tv_usec = (timeout % 1000) * 1000;

        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);

        if (*condition & ENET_SOCKET_WAIT_SEND) {
            FD_SET(socket, &writeSet);
        }

        if (*condition & ENET_SOCKET_WAIT_RECEIVE) {
            FD_SET(socket, &readSet);
        }

        selectCount = select(socket + 1, &readSet, &writeSet, NULL, &timeVal);

        if (selectCount < 0) {
            return -1;
        }

        *condition = ENET_SOCKET_WAIT_NONE;

        if (selectCount == 0) {
            return 0;
        }

        if (FD_ISSET(socket, &writeSet)) {
            *condition |= ENET_SOCKET_WAIT_SEND;
        }

        if (FD_ISSET(socket, &readSet)) {
            *condition |= ENET_SOCKET_WAIT_RECEIVE;
        }

        return 0;
    } /* enet_socket_wait */

    #endif // _WIN32


#ifdef __cplusplus
}
#endif

#endif // ENET_IMPLEMENTATION
#endif // ENET_INCLUDE_H
