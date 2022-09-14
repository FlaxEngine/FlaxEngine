// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "NetworkReplicator.h"
#include "NetworkClient.h"
#include "NetworkManager.h"
#include "NetworkInternal.h"
#include "NetworkStream.h"
#include "INetworkSerializable.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/HashSet.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Threading/Threading.h"

// Enables verbose logging for Network Replicator actions (dev-only)
#define NETWORK_REPLICATOR_DEBUG_LOG 1

struct NetworkReplicatedObject
{
    ScriptingObjectReference<ScriptingObject> Object;
    Guid OwnerId;
#if NETWORK_REPLICATOR_DEBUG_LOG
    Guid ObjectId;
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
};

#if NETWORK_REPLICATOR_DEBUG_LOG
#include "Engine/Core/Formatting.h"
DEFINE_DEFAULT_FORMATTING(NetworkReplicatedObject, "{}", v.ObjectId);
#endif

inline uint32 GetHash(const NetworkReplicatedObject& key)
{
    return GetHash(key.Object.Get());
}

namespace
{
    CriticalSection ObjectsLock;
    HashSet<NetworkReplicatedObject> Objects;
    NetworkStream* CachedStream = nullptr;
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
    item.OwnerId = owner->GetID();
#if NETWORK_REPLICATOR_DEBUG_LOG
    item.ObjectId = obj->GetID();
    LOG(Info, "[NetworkReplicator] Add new object {}:{}, owned by {}:{}", item, obj->GetType().ToString(), item.OwnerId, owner->GetType().ToString());
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
    SAFE_DELETE(CachedStream);
}

void NetworkInternal::NetworkReplicatorUpdate()
{
    PROFILE_CPU();
    ScopeLock lock(ObjectsLock);
    if (Objects.Count() == 0)
        return;
    if (CachedStream == nullptr)
        CachedStream = New<NetworkStream>();
    // TODO: introduce NetworkReplicationHierarchy to optimize objects replication in large worlds (eg. batched culling networked scene objects that are too far from certain client to be relevant)
    // TODO: per-object sync interval (in frames) - could be scaled by hierarchy (eg. game could slow down sync rate for objects far from player)
    // TODO: network authority (eg. object owned by client)

    if (NetworkManager::IsClient())
    {
        // TODO: client logic to apply replication changes
    }
    else
    {
        // Brute force synchronize all networked objects with clients
        for (auto it = Objects.Begin(); it.IsNotEnd(); ++it)
        {
            auto& item = it->Item;
            ScriptingObject* obj = item.Object.Get();
            if (!obj)
            {
                // Object got deleted
#if NETWORK_REPLICATOR_DEBUG_LOG
                LOG(Info, "[NetworkReplicator] Remove object {}, owned by {}", item.Object, item.OwnerId);
#endif
                Objects.Remove(it);
                continue;
            }

            // Serialize object
            // TODO: cache per-type serialization thunk to boost CPU performance
            CachedStream->Initialize(1024);
            if (auto* serializable = ScriptingObject::ToInterface<INetworkSerializable>(obj))
            {
                serializable->Serialize(CachedStream);
            }
            else
            {
#if NETWORK_REPLICATOR_DEBUG_LOG
                if (!item.InvalidTypeWarn)
                {
                    item.InvalidTypeWarn = true;
                    LOG(Error, "[NetworkReplicator] Cannot serialize object {} (missing serialization logic)", item);
                }
#endif
                continue;
            }
            // TODO: how to serialize object? in memory to MemoryWriteStream? handle both C++ and C# without any memory alloc!

            // Brute force object to all clients
            for (NetworkClient* client : NetworkManager::Clients)
            {
                // TODO: split object data (eg. more messages) if needed
                // TODO: send message from Peer to client->Connection
            }
        }
    }
}
