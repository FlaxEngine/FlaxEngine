// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

enum class NetworkChannelType;
enum class NetworkEventType;
enum class NetworkConnectionState;

class INetworkDriver;
class INetworkSerializable;
class NetworkPeer;
class NetworkClient;
class NetworkStream;
class NetworkReplicationHierarchy;

struct NetworkEvent;
struct NetworkConnection;
struct NetworkMessage;
struct NetworkConfig;
struct NetworkDriverStats;
struct NetworkRpcParams;
