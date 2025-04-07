// Copyright (c) Wojciech Figat. All rights reserved.

#include "NetworkManager.h"
#include "NetworkClient.h"
#include "NetworkPeer.h"
#include "NetworkEvent.h"
#include "NetworkChannelType.h"
#include "NetworkSettings.h"
#include "NetworkInternal.h"
#include "FlaxEngine.Gen.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Engine/Time.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/Scripting.h"

float NetworkManager::NetworkFPS = 60.0f;
NetworkPeer* NetworkManager::Peer = nullptr;
NetworkManagerMode NetworkManager::Mode = NetworkManagerMode::Offline;
NetworkConnectionState NetworkManager::State = NetworkConnectionState::Offline;
uint32 NetworkManager::Frame = 0;
uint32 NetworkManager::LocalClientId = 0;
NetworkClient* NetworkManager::LocalClient = nullptr;
Array<NetworkClient*> NetworkManager::Clients;
Action NetworkManager::StateChanged;
Delegate<NetworkClientConnectionData&> NetworkManager::ClientConnecting;
Delegate<NetworkClient*> NetworkManager::ClientConnected;
Delegate<NetworkClient*> NetworkManager::ClientDisconnected;

PACK_STRUCT(struct NetworkMessageKey
    {
    NetworkMessageIDs ID = NetworkMessageIDs::Key;
    byte Type;
    uint32 Index;
    });

struct NetworkKey
{
    enum Types
    {
        TypeNone = 0,
        TypeId = 1,
        TypeName = 2,
    } Type;

    union
    {
        Guid Id;
        StringAnsiView Name;
    };

    POD_COPYABLE(NetworkKey);

    NetworkKey()
    {
        Type = TypeNone;
    }

    NetworkKey(const Guid& id)
    {
        Type = TypeId;
        Id = id;
    }

    NetworkKey(const StringAnsiView& name)
    {
        Type = TypeName;
        Name = name;
    }
};

struct NetworkKeys
{
    CriticalSection Lock;
    Array<NetworkKey> Table;
    Dictionary<Guid, uint32> LookupId;
    Dictionary<StringAnsiView, uint32> LookupName;
    Dictionary<Guid, NetworkKey> PendingIds;
    Dictionary<StringAnsiView, NetworkKey> PendingNames;

    void SendPending();
    void SendAll(const NetworkConnection* target = nullptr);
    void Clear();

private:
    static void Send(const NetworkKey& key, uint32 index, const NetworkConnection* target = nullptr);
};

namespace
{
    uint32 GameProtocolVersion = 0;
    uint32 NextClientId = 0;
    double LastUpdateTime = 0;
    Array<NetworkConnection> ActiveConnections;
    NetworkKeys Keys;
}

PACK_STRUCT(struct NetworkMessageHandshake
    {
    NetworkMessageIDs ID = NetworkMessageIDs::Handshake;
    uint32 EngineBuild;
    uint32 EngineProtocolVersion;
    uint32 GameProtocolVersion;
    byte Platform;
    byte Architecture;
    uint16 PayloadDataSize;
    });

PACK_STRUCT(struct NetworkMessageHandshakeReply
    {
    NetworkMessageIDs ID = NetworkMessageIDs::HandshakeReply;
    uint32 ClientId;
    int32 Result;
    });

FORCE_INLINE StringAnsiView CloneAllocName(const StringAnsiView& name)
{
    StringAnsiView result;
    if (name.Get())
    {
        const int32 length = name.Length();
        char* str = (char*)Allocator::Allocate(length + 1);
        Platform::MemoryCopy(str, name.Get(), length);
        str[length] = 0;
        result = StringAnsiView(str, length);
    }
    return result;
}

FORCE_INLINE bool IsNetworkKeyValid(uint32 index)
{
    // TODO: use NetworkClientsMask to skip using network keys for clients that might not know it yet
    // TODO: if key has been added within a last couple of frames then don't use it yet as it needs to be propagated across the peers
    return true;
}

void NetworkMessage::WriteNetworkId(const Guid& id)
{
#if USE_NETWORK_KEYS
    ScopeLock lock(Keys.Lock);
    uint32 index = MAX_uint32;
    bool hasIndex = Keys.LookupId.TryGet(id, index);
    if (hasIndex)
        hasIndex &= IsNetworkKeyValid(index);
    WriteUInt32(index);
    if (!hasIndex)
    {
        // No key cached locally so send the full data
        WriteBytes((const uint8*)&id, sizeof(Guid));

        // Add to the pending list (ignore on clients as server will automatically create a key once it gets full data)
        if (NetworkManager::Mode != NetworkManagerMode::Client && 
            !Keys.PendingIds.ContainsKey(id))
        {
            Keys.PendingIds.Add(id, NetworkKey(id));
        }
    }
#else
    WriteBytes((const uint8*)&id, sizeof(Guid));
#endif
}

void NetworkMessage::ReadNetworkId(Guid& id)
{
#if USE_NETWORK_KEYS
    ScopeLock lock(Keys.Lock);
    uint32 index = ReadUInt32();
    if (index != MAX_uint32)
    {
        if (index < (uint32)Keys.Table.Count())
        {
            // Use cached key data
            const NetworkKey& k = Keys.Table.Get()[index];
            ASSERT(k.Type == NetworkKey::TypeId);
            id = k.Id;
        }
        else
        {
            // Incorrect data
            // TODO: should we check if message comes before new key arrival? should sender assume that key needs confirmation of receive?
            id = Guid::Empty;
        }
    }
    else
    {
        // Read full data
        ReadBytes((uint8*)&id, sizeof(Guid));

        // When server receives unknown data then turn this into key so connected client will receive it
        if (NetworkManager::Mode != NetworkManagerMode::Client && 
            !Keys.PendingIds.ContainsKey(id) && 
            !Keys.LookupId.ContainsKey(id))
        {
            Keys.PendingIds.Add(id, NetworkKey(id));
        }
    }
#else
    ReadBytes((uint8*)&id, sizeof(Guid));
#endif
}

void NetworkMessage::WriteNetworkName(const StringAnsiView& name)
{
#if USE_NETWORK_KEYS
    ScopeLock lock(Keys.Lock);
    uint32 index = MAX_uint32;
    bool hasIndex = Keys.LookupName.TryGet(name, index);
    if (hasIndex)
        hasIndex &= IsNetworkKeyValid(index);
    WriteUInt32(index);
    if (!hasIndex)
    {
        // No key cached locally so send the full data
        WriteStringAnsi(name);

        // Add to the pending list (ignore on clients as server will automatically create a key once it gets full data)
        if (NetworkManager::Mode != NetworkManagerMode::Client && 
            !Keys.PendingNames.ContainsKey(name))
        {
            StringAnsiView newName = CloneAllocName(name);
            Keys.PendingNames.Add(newName, NetworkKey(newName));
        }
    }
#else
    WriteStringAnsi(name);
#endif
}

void NetworkMessage::ReadNetworkName(StringAnsiView& name)
{
#if USE_NETWORK_KEYS
    ScopeLock lock(Keys.Lock);
    uint32 index = ReadUInt32();
    if (index != MAX_uint32)
    {
        if (index < (uint32)Keys.Table.Count())
        {
            // Use cached key data
            const NetworkKey& k = Keys.Table.Get()[index];
            ASSERT(k.Type == NetworkKey::TypeName);
            name = k.Name;
        }
        else
        {
            // Incorrect data
            // TODO: should we check if message comes before new key arrival? should sender assume that key needs confirmation of receive?
            name = StringAnsiView::Empty;
        }
    }
    else
    {
        // Read full data
        name = ReadStringAnsi();

        // When server receives unknown data then turn this into key so connected client will receive it
        if (NetworkManager::Mode != NetworkManagerMode::Client && 
            !Keys.PendingNames.ContainsKey(name) && 
            !Keys.LookupName.ContainsKey(name))
        {
            StringAnsiView newName = CloneAllocName(name);
            Keys.PendingNames.Add(newName, NetworkKey(newName));
        }
    }
#else
    name = ReadStringAnsi();
#endif
}

void OnNetworkMessageHandshake(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer)
{
    // Read client connection data
    NetworkMessageHandshake msgData;
    event.Message.ReadStructure(msgData);
    NetworkClientConnectionData connectionData;
    connectionData.Client = client;
    connectionData.Result = 0;
    connectionData.Platform = (PlatformType)msgData.Platform;
    connectionData.Architecture = (ArchitectureType)msgData.Architecture;
    connectionData.PayloadData.Resize(msgData.PayloadDataSize);
    event.Message.ReadBytes(connectionData.PayloadData.Get(), msgData.PayloadDataSize);
    if (msgData.EngineProtocolVersion != NETWORK_PROTOCOL_VERSION ||
        msgData.GameProtocolVersion != GameProtocolVersion)
    {
        connectionData.Result = 1; // Mismatching network protocol version
    }
    NetworkManager::ClientConnecting(connectionData); // Allow server to validate connection

    // Reply to the handshake message with a result
    NetworkMessageHandshakeReply replyData;
    replyData.Result = connectionData.Result;
    replyData.ClientId = client->ClientId;
    NetworkMessage msgReply = peer->BeginSendMessage();
    msgReply.WriteStructure(replyData);
    peer->EndSendMessage(NetworkChannelType::ReliableOrdered, msgReply, event.Sender);

    // Update client based on connection result
    if (connectionData.Result != 0)
    {
        LOG(Info, "Connection blocked with result {0} from client id={1}.", connectionData.Result, event.Sender.ConnectionId);
        client->State = NetworkConnectionState::Disconnecting;
        peer->Disconnect(event.Sender);
        client->State = NetworkConnectionState::Disconnected;
    }
    else
    {
        client->State = NetworkConnectionState::Connected;
        LOG(Info, "Client id={0} connected", event.Sender.ConnectionId);
        ActiveConnections.Add(event.Sender);
        Keys.SendAll(&event.Sender);
        NetworkManager::ClientConnected(client);
        NetworkInternal::NetworkReplicatorClientConnected(client);
    }
}

void OnNetworkMessageHandshakeReply(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer)
{
    ASSERT_LOW_LAYER(NetworkManager::IsClient());
    NetworkMessageHandshakeReply msgData;
    event.Message.ReadStructure(msgData);
    if (msgData.Result != 0)
    {
        // Server failed to connect with client
        // TODO: feed game with result from msgData.Result
        NetworkManager::Stop();
        return;
    }

    // Client got connected with server
    NetworkManager::LocalClientId = msgData.ClientId;
    NetworkManager::LocalClient->ClientId = msgData.ClientId;
    NetworkManager::LocalClient->State = NetworkConnectionState::Connected;
    NetworkManager::State = NetworkConnectionState::Connected;
    NetworkManager::StateChanged();
}

void OnNetworkMessageKey(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer)
{
    // Read key data
    NetworkMessageKey msgData;
    event.Message.ReadStructure(msgData);
    Guid id;
    StringAnsiView name;
    if (msgData.Type == NetworkKey::TypeId)
        event.Message.ReadBytes((uint8*)&id, sizeof(Guid));
    else
        name = event.Message.ReadStringAnsi();

    ScopeLock lock(Keys.Lock);
    if (NetworkManager::IsClient())
    {
        // Add new key
        if (msgData.Index >= (uint32)Keys.Table.Count())
            Keys.Table.Resize(msgData.Index + 1);
        NetworkKey& key = Keys.Table[msgData.Index];
        ASSERT_LOW_LAYER(key.Type == NetworkKey::TypeNone);
        key.Type = (NetworkKey::Types)msgData.Type;
        if (key.Type == NetworkKey::TypeId)
        {
            key.Id = id;
            Keys.LookupId.Add(id, msgData.Index);
        }
        else
        {
            key.Name = CloneAllocName(name);
            Keys.LookupName.Add(key.Name, msgData.Index);
        }
    }
    else
    {
        // TODO: make new pending key if client explicitly sends it
    }
}

namespace
{
    // Network message handlers table
    void (*MessageHandlers[(int32)NetworkMessageIDs::MAX])(NetworkEvent&, NetworkClient*, NetworkPeer*) =
    {
        nullptr,
        OnNetworkMessageHandshake,
        OnNetworkMessageHandshakeReply,
        OnNetworkMessageKey,
        NetworkInternal::OnNetworkMessageObjectReplicate,
        NetworkInternal::OnNetworkMessageObjectReplicatePart,
        NetworkInternal::OnNetworkMessageObjectSpawn,
        NetworkInternal::OnNetworkMessageObjectSpawnPart,
        NetworkInternal::OnNetworkMessageObjectDespawn,
        NetworkInternal::OnNetworkMessageObjectRole,
        NetworkInternal::OnNetworkMessageObjectRpc,
    };
}

class NetworkManagerService : public EngineService
{
public:
    NetworkManagerService()
        : EngineService(TEXT("Network Manager"), 1000)
    {
    }

    void Update() override;

    void Dispose() override
    {
        // Ensure to dispose any resources upon exiting
        NetworkManager::Stop();
    }
};

NetworkManagerService NetworkManagerServiceInstance;

bool StartPeer()
{
    PROFILE_CPU();
    ASSERT_LOW_LAYER(!NetworkManager::Peer);
    NetworkManager::State = NetworkConnectionState::Connecting;
    NetworkManager::StateChanged();
    const auto& settings = *NetworkSettings::Get();

    // Create Network Peer that will use underlying INetworkDriver to send messages over the network
    NetworkConfig networkConfig;
    if (NetworkManager::Mode == NetworkManagerMode::Client)
    {
        // Client
        networkConfig.Address = settings.Address;
        networkConfig.Port = settings.Port;
        networkConfig.ConnectionsLimit = 1;
    }
    else
    {
        // Server or Host
        networkConfig.Address = TEXT("any");
        networkConfig.Port = settings.Port;
        networkConfig.ConnectionsLimit = (uint16)settings.MaxClients;
    }
    const ScriptingTypeHandle networkDriverType = Scripting::FindScriptingType(settings.NetworkDriver);
    if (!networkDriverType)
    {
        LOG(Error, "Unknown Network Driver type {0}", String(settings.NetworkDriver));
        return true;
    }
    networkConfig.NetworkDriver = ScriptingObject::NewObject(networkDriverType);
    NetworkManager::Peer = NetworkPeer::CreatePeer(networkConfig);
    if (!NetworkManager::Peer)
    {
        LOG(Error, "Failed to create Network Peer at {0}:{1}", networkConfig.Address, networkConfig.Port);
        NetworkManager::State = NetworkConnectionState::Offline;
        return true;
    }
    NetworkManager::Frame = 0;

    return false;
}

void StopPeer()
{
    if (!NetworkManager::Peer)
        return;
    PROFILE_CPU();
    if (NetworkManager::Mode == NetworkManagerMode::Client)
        NetworkManager::Peer->Disconnect();
    NetworkPeer::ShutdownPeer(NetworkManager::Peer);
    NetworkManager::Peer = nullptr;
}

void NetworkSettings::Apply()
{
    NetworkManager::NetworkFPS = NetworkFPS;
    GameProtocolVersion = ProtocolVersion;
}

NetworkClient::NetworkClient(uint32 id, NetworkConnection connection)
    : ScriptingObject(SpawnParams(Guid::New(), TypeInitializer))
    , ClientId(id)
    , Connection(connection)
    , State(NetworkConnectionState::Connecting)
{
}

NetworkClient* NetworkManager::GetClient(const NetworkConnection& connection)
{
    if (connection.ConnectionId == 0)
        return LocalClient;
    for (NetworkClient* client : Clients)
    {
        if (client->Connection == connection)
            return client;
    }
    return nullptr;
}

NetworkClient* NetworkManager::GetClient(uint32 clientId)
{
    for (NetworkClient* client : Clients)
    {
        if (client->ClientId == clientId)
            return client;
    }
    return nullptr;
}

bool NetworkManager::StartServer()
{
    PROFILE_CPU();
    Stop();

    LOG(Info, "Starting network manager as server");
    Mode = NetworkManagerMode::Server;
    if (StartPeer())
    {
        Mode = NetworkManagerMode::Offline;
        return true;
    }
    if (!Peer->Listen())
    {
        Stop();
        return true;
    }
    LocalClientId = ServerClientId;
    NextClientId = ServerClientId + 1;

    State = NetworkConnectionState::Connected;
    StateChanged();
    return false;
}

bool NetworkManager::StartClient()
{
    PROFILE_CPU();
    Stop();

    LOG(Info, "Starting network manager as client");
    Mode = NetworkManagerMode::Client;
    if (StartPeer())
    {
        Mode = NetworkManagerMode::Offline;
        return true;
    }
    if (!Peer->Connect())
    {
        Stop();
        return true;
    }
    LocalClientId = 0; // Id gets assigned by server later after connection
    NextClientId = 0;
    LocalClient = New<NetworkClient>(LocalClientId, NetworkConnection{ 0 });

    return false;
}

bool NetworkManager::StartHost()
{
    PROFILE_CPU();
    Stop();

    LOG(Info, "Starting network manager as host");
    Mode = NetworkManagerMode::Host;
    if (StartPeer())
    {
        Mode = NetworkManagerMode::Offline;
        return true;
    }
    if (!Peer->Listen())
    {
        Mode = NetworkManagerMode::Offline;
        return true;
    }
    LocalClientId = ServerClientId;
    NextClientId = ServerClientId + 1;
    LocalClient = New<NetworkClient>(LocalClientId, NetworkConnection{ 0 });

    // Auto-connect host
    LocalClient->State = NetworkConnectionState::Connecting;
    State = NetworkConnectionState::Connected;
    StateChanged();
    LocalClient->State = NetworkConnectionState::Connected;
    ClientConnected(LocalClient);

    return false;
}

void NetworkManager::Stop()
{
    if (Mode == NetworkManagerMode::Offline && State == NetworkConnectionState::Offline)
        return;
    PROFILE_CPU();

    LOG(Info, "Stopping network manager");
    State = NetworkConnectionState::Disconnecting;
    if (LocalClient)
        LocalClient->State = NetworkConnectionState::Disconnecting;
    for (NetworkClient* client : Clients)
        client->State = NetworkConnectionState::Disconnecting;
    StateChanged();

    for (int32 i = Clients.Count() - 1; i >= 0; i--)
    {
        NetworkClient* client = Clients[i];
        ClientDisconnected(client);
        client->State = NetworkConnectionState::Disconnected;
        Delete(client);
        Clients.RemoveAt(i);
    }
    if (Mode == NetworkManagerMode::Host && LocalClient)
    {
        ClientDisconnected(LocalClient);
        LocalClient->State = NetworkConnectionState::Disconnected;
    }
    NetworkInternal::NetworkReplicatorClear();
    StopPeer();
    if (LocalClient)
    {
        Delete(LocalClient);
        LocalClient = nullptr;
    }

    // Clear local state
    NextClientId = 0;
    LastUpdateTime = 0;
    ActiveConnections.Clear();
    Keys.Clear();

    State = NetworkConnectionState::Disconnected;
    Mode = NetworkManagerMode::Offline;
    LastUpdateTime = 0;

    StateChanged();
}

void NetworkKeys::SendPending()
{
    PROFILE_CPU();
    ScopeLock lock(Lock);

    // Add new keys
    int32 initialCount = Table.Count();
    int32 sendIndex = initialCount;
    for (auto& e : PendingIds)
    {
        const int32 key = sendIndex++;
        LookupId.Add(e.Key, key);
        Table.Add(e.Value);
    }
    for (auto& e : PendingNames)
    {
        const int32 key = sendIndex++;
        LookupName.Add(e.Key, key);
        Table.Add(e.Value);
    }

    // Send new entries
    sendIndex = initialCount;
    for (auto& e : PendingIds)
        Send(e.Value, sendIndex++);
    for (auto& e : PendingNames)
        Send(e.Value, sendIndex++);

    // Clear lists
    PendingIds.Clear();
    PendingNames.Clear();
}

void NetworkKeys::SendAll(const NetworkConnection* target)
{
    PROFILE_CPU();
    ScopeLock lock(Lock);
    int32 sendIndex = 0;
    for (auto& e : Table)
        Send(e, sendIndex++, target);
}

void NetworkKeys::Clear()
{
    ScopeLock lock(Lock);
    LookupId.Clear();
    LookupName.Clear();
    PendingNames.GetValues(Table);
    PendingNames.Clear();
    for (auto& e : Table)
    {
        if (e.Type == NetworkKey::TypeName)
        {
            // Free allocated string
            Allocator::Free((void*)e.Name.Get());
        }
    }
    Table.Clear();
}

void NetworkKeys::Send(const NetworkKey& key, uint32 index, const NetworkConnection* target)
{
    // TODO: optimize with batching multiple keys into a single message
    auto peer = NetworkManager::Peer;
    NetworkMessage msg = peer->BeginSendMessage();
    NetworkMessageKey msgData;
    msgData.Type = key.Type;
    msgData.Index = index;
    msg.WriteStructure(msgData);
    if (key.Type == NetworkKey::TypeId)
        msg.WriteGuid(key.Id);
    else
        msg.WriteStringAnsi(key.Name);
    if (NetworkManager::IsClient())
        peer->EndSendMessage(NetworkChannelType::Reliable, msg);
    else if (target)
        peer->EndSendMessage(NetworkChannelType::Reliable, msg, *target);
    else
        peer->EndSendMessage(NetworkChannelType::Reliable, msg, ActiveConnections);
}

void NetworkManagerService::Update()
{
    const double currentTime = Time::Update.UnscaledTime.GetTotalSeconds();
    const float minDeltaTime = NetworkManager::NetworkFPS > 0 ? 1.0f / NetworkManager::NetworkFPS : 0.0f;
    auto peer = NetworkManager::Peer;
    if (NetworkManager::Mode == NetworkManagerMode::Offline || (float)(currentTime - LastUpdateTime) < minDeltaTime || !peer)
        return;
    PROFILE_CPU();
    LastUpdateTime = currentTime;
    NetworkManager::Frame++;
    NetworkInternal::NetworkReplicatorPreUpdate();
    // TODO: convert into TaskGraphSystems and use async jobs

    // Process network messages
    NetworkEvent event;
    bool eventIsValid = true;
    while (peer->PopEvent(event) && eventIsValid)
    {
        switch (event.EventType)
        {
        case NetworkEventType::Connected:
            LOG(Info, "Incoming connection with Id={0}", event.Sender.ConnectionId);
            if (NetworkManager::IsClient())
            {
                // Initialize client connection data
                NetworkClientConnectionData connectionData;
                connectionData.Client = NetworkManager::LocalClient;
                connectionData.Result = 0;
                connectionData.Platform = PLATFORM_TYPE;
                connectionData.Architecture = PLATFORM_ARCH;
                NetworkManager::ClientConnecting(connectionData); // Allow client to validate connection or inject custom connection data
                if (connectionData.Result != 0)
                {
                    LOG(Info, "Connection blocked with result {0}.", connectionData.Result);
                    NetworkManager::Stop();
                    break;
                }

                // Send initial handshake message from client to server
                NetworkMessageHandshake msgData;
                msgData.EngineBuild = FLAXENGINE_VERSION_BUILD;
                msgData.EngineProtocolVersion = NETWORK_PROTOCOL_VERSION;
                msgData.GameProtocolVersion = GameProtocolVersion;
                msgData.Platform = (byte)connectionData.Platform;
                msgData.Architecture = (byte)connectionData.Architecture;
                msgData.PayloadDataSize = (uint16)connectionData.PayloadData.Count();
                NetworkMessage msg = peer->BeginSendMessage();
                msg.WriteStructure(msgData);
                msg.WriteBytes(connectionData.PayloadData.Get(), connectionData.PayloadData.Count());
                peer->EndSendMessage(NetworkChannelType::ReliableOrdered, msg);
            }
            else
            {
                // Create incoming client
                auto client = New<NetworkClient>(NextClientId++, event.Sender);
                NetworkManager::Clients.Add(client);
            }
            break;
        case NetworkEventType::Disconnected:
        case NetworkEventType::Timeout:
            LOG(Info, "{1} with Id={0}", event.Sender.ConnectionId, event.EventType == NetworkEventType::Disconnected ? TEXT("Disconnected") : TEXT("Disconnected on timeout"));
            if (NetworkManager::IsClient())
            {
                // Server disconnected from client
                NetworkManager::Stop();
                return;
            }
            else
            {
                // Client disconnected from server/host
                NetworkClient* client = NetworkManager::GetClient(event.Sender);
                if (!client)
                {
                    LOG(Error, "Unknown client");
                    break;
                }
                client->State = NetworkConnectionState::Disconnecting;
                LOG(Info, "Client id={0} disconnected", event.Sender.ConnectionId);
                NetworkManager::Clients.RemoveKeepOrder(client);
                NetworkInternal::NetworkReplicatorClientDisconnected(client);
                NetworkManager::ClientDisconnected(client);
                client->State = NetworkConnectionState::Disconnected;
                Delete(client);
                ActiveConnections.Remove(event.Sender);
            }
            break;
        case NetworkEventType::Message:
        {
            // Process network message
            NetworkClient* client = NetworkManager::GetClient(event.Sender);
            if (!client && NetworkManager::Mode != NetworkManagerMode::Client)
            {
                LOG(Error, "Unknown client");
                break;
            }
            uint8 id = *event.Message.Buffer;
            if (id < (uint8)NetworkMessageIDs::MAX)
            {
                MessageHandlers[id](event, client, peer);
            }
            else
            {
                LOG(Warning, "Unknown message id={0} from connection {1}", id, event.Sender.ConnectionId);
            }
        }
            peer->RecycleMessage(event.Message);
            break;
        default:
            eventIsValid = false;
            break;
        }
    }

    // Update replication
    NetworkInternal::NetworkReplicatorUpdate();

    // Flush pending network key updates
    Keys.SendPending();
}
