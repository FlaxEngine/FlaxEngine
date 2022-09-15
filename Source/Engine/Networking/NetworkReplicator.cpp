// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "NetworkReplicator.h"
#include "NetworkClient.h"
#include "NetworkManager.h"
#include "NetworkInternal.h"
#include "NetworkStream.h"
#include "INetworkSerializable.h"
#include "NetworkMessage.h"
#include "NetworkPeer.h"
#include "NetworkChannelType.h"
#include "NetworkEvent.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/HashSet.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Threading/Threading.h"

// Enables verbose logging for Network Replicator actions (dev-only)
#define NETWORK_REPLICATOR_DEBUG_LOG 1

PACK_STRUCT(struct NetworkMessageReplicatedObject
    {
    NetworkMessageIDs ID;
    Guid ObjectId; // TODO: introduce networked-ids to synchronize unique ids as ushort (less data over network)
    Guid OwnerId;
    char ObjectTypeName[128]; // TODO: introduce networked-name to synchronize unique names as ushort (less data over network)
    uint16 DataSize;
    });

struct NetworkReplicatedObject
{
    ScriptingObjectReference<ScriptingObject> Object;
    Guid ObjectId;
    Guid OwnerId;
#if NETWORK_REPLICATOR_DEBUG_LOG
    bool InvalidTypeWarn = false;
#endif

    bool operator==(const NetworkReplicatedObject& other) const
    {
        return Object == other.Object;
    }

    bool operator==(const ScriptingObject* other) const
    {
        return Object == other;
    }

    bool operator==(const Guid& other) const
    {
        return ObjectId == other;
    }

    String ToString() const
    {
        return ObjectId.ToString();
    }
};

inline uint32 GetHash(const NetworkReplicatedObject& key)
{
    return GetHash(key.Object.Get());
}

namespace
{
    CriticalSection ObjectsLock;
    HashSet<NetworkReplicatedObject> Objects;
    NetworkStream* CachedWriteStream = nullptr;
    NetworkStream* CachedReadStream = nullptr;
    Array<NetworkConnection> CachedTargets;
}

class NetworkReplicationService : public EngineService
{
public:
    NetworkReplicationService()
        : EngineService(TEXT("Network Replication"), 1100)
    {
    }

    void Dispose() override;
};

void NetworkReplicationService::Dispose()
{
    NetworkInternal::NetworkReplicatorClear();
}

NetworkReplicationService NetworkReplicationServiceInstance;

NetworkReplicatedObject* ResolveObject(const Guid& objectId, const Guid& ownerId, char objectTypeName[128])
{
    const auto it = Objects.Find(objectId);
    if (it != Objects.End())
        return &it->Item;
    
    // TODO: cache objects remapping table to skip this search on 2nd sync

    // Try to use remapped object
    const auto ownerIt = Objects.Find(ownerId);
    if (ownerIt != Objects.End())
    {
        // TODO: find object of given type within owner (only objects that ahs not been sync-replicated yet)
        //return &ownerIt->Item;
    }

    return nullptr;
}

void NetworkReplicator::AddObject(ScriptingObject* obj, ScriptingObject* owner)
{
    if (!obj || NetworkManager::State == NetworkConnectionState::Offline)
        return;
    CHECK(owner && owner != obj);
    ScopeLock lock(ObjectsLock);
    if (Objects.Contains(obj))
        return;

    // Add object to the list
    NetworkReplicatedObject item;
    item.Object = obj;
    item.ObjectId = obj->GetID();
    item.OwnerId = owner->GetID();
#if NETWORK_REPLICATOR_DEBUG_LOG
    LOG(Info, "[NetworkReplicator] Add new object {}:{}, owned by {}:{}", item.ToString(), obj->GetType().ToString(), item.OwnerId.ToString(), owner->GetType().ToString());
#endif
    Objects.Add(MoveTemp(item));
}

void NetworkInternal::NetworkReplicatorClear()
{
    ScopeLock lock(ObjectsLock);

    // Cleanup
#if NETWORK_REPLICATOR_DEBUG_LOG
    LOG(Info, "[NetworkReplicator] Shutdown");
#endif
    Objects.Clear();
    Objects.SetCapacity(0);
    SAFE_DELETE(CachedWriteStream);
    SAFE_DELETE(CachedReadStream);
    CachedTargets.Resize(0);
}

void NetworkInternal::NetworkReplicatorUpdate()
{
    PROFILE_CPU();
    ScopeLock lock(ObjectsLock);
    if (Objects.Count() == 0)
        return;
    if (CachedWriteStream == nullptr)
        CachedWriteStream = New<NetworkStream>();
    NetworkStream* stream = CachedWriteStream;
    NetworkPeer* peer = NetworkManager::Peer;
    // TODO: introduce NetworkReplicationHierarchy to optimize objects replication in large worlds (eg. batched culling networked scene objects that are too far from certain client to be relevant)
    // TODO: per-object sync interval (in frames) - could be scaled by hierarchy (eg. game could slow down sync rate for objects far from player)
    // TODO: network authority (eg. object owned by client that can affect server)

    if (NetworkManager::IsClient())
    {
        // TODO: client logic to apply replication changes
    }
    else
    {
        // Collect clients for replication
        CachedTargets.Clear();
        // TODO: per-object relevancy for connected clients (eg. skip replicating actor to far players)
        for (const NetworkClient* client : NetworkManager::Clients)
        {
            if (client->State == NetworkConnectionState::Connected)
            {
                CachedTargets.Add(client->Connection);
            }
        }

        // Brute force synchronize all networked objects with clients
        for (auto it = Objects.Begin(); it.IsNotEnd(); ++it)
        {
            auto& item = it->Item;
            ScriptingObject* obj = item.Object.Get();
            if (!obj)
            {
                // Object got deleted
#if NETWORK_REPLICATOR_DEBUG_LOG
                LOG(Info, "[NetworkReplicator] Remove object {}, owned by {}", item.ToString(), item.OwnerId.ToString());
#endif
                Objects.Remove(it);
                continue;
            }

            // Serialize object
            // TODO: cache per-type serialization thunk to boost CPU performance
            stream->Initialize();
            if (auto* serializable = ScriptingObject::ToInterface<INetworkSerializable>(obj))
            {
                serializable->Serialize(stream);
            }
            else
            {
#if NETWORK_REPLICATOR_DEBUG_LOG
                if (!item.InvalidTypeWarn)
                {
                    item.InvalidTypeWarn = true;
                    LOG(Error, "[NetworkReplicator] Cannot serialize object {} (missing serialization logic)", item.ToString());
                }
#endif
                continue;
            }

            // Send object to clients
            {
                const uint32 size = stream->GetPosition();
                ASSERT(size <= MAX_uint16)
                NetworkMessageReplicatedObject msgData;
                msgData.ID = NetworkMessageIDs::ReplicatedObject;
                msgData.ObjectId = item.ObjectId;
                msgData.OwnerId = item.OwnerId;
                // TODO: put timestamp (or server tick number) to prevent applying replicated object changes from previous packet (Unreliable and Unordered channel is used)
                const StringAnsiView& objectTypeName = obj->GetType().Fullname;
                Platform::MemoryCopy(msgData.ObjectTypeName, objectTypeName.Get(), objectTypeName.Length());
                msgData.ObjectTypeName[objectTypeName.Length()] = 0;
                msgData.DataSize = size;
                // TODO: split object data (eg. more messages) if needed
                NetworkMessage msg = peer->BeginSendMessage();
                msg.WriteStructure(msgData);
                msg.WriteBytes(stream->GetBuffer(), size);
                peer->EndSendMessage(NetworkChannelType::Unreliable, msg, CachedTargets);

                // TODO: stats for bytes send per object type
            }
        }
    }
}

void NetworkInternal::OnNetworkMessageReplicatedObject(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer)
{
    NetworkMessageReplicatedObject msgData;
    event.Message.ReadStructure(msgData);
    ScopeLock lock(ObjectsLock);
    NetworkReplicatedObject* e = ResolveObject(msgData.ObjectId, msgData.OwnerId, msgData.ObjectTypeName);
    if (e)
    {
        auto& item = *e;
        ScriptingObject* obj = item.Object.Get();
        if (!obj)
            return;

        // Setup message reading stream
        if (CachedReadStream == nullptr)
            CachedReadStream = New<NetworkStream>();
        NetworkStream* stream = CachedReadStream;
        stream->Initialize(event.Message.Buffer + event.Message.Position, msgData.DataSize);

        // Deserialize object
        // TODO: cache per-type serialization thunk to boost CPU performance
        if (auto* serializable = ScriptingObject::ToInterface<INetworkSerializable>(obj))
        {
            serializable->Deserialize(stream);
        }
        else
        {
#if NETWORK_REPLICATOR_DEBUG_LOG
            if (!item.InvalidTypeWarn)
            {
                item.InvalidTypeWarn = true;
                LOG(Error, "[NetworkReplicator] Cannot serialize object {} (missing serialization logic)", item.ToString());
            }
#endif
        }
    }
    else
    {
        // TODO: put message to the queue to be resolved later (eg. object replication came before spawn packet) - use TTL to prevent memory overgrowing
    }
}
