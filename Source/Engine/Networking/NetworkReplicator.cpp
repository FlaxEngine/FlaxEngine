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
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Threading/ThreadLocal.h"

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
    uint32 LastFrameSync = 0;
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
    return GetHash(key.ObjectId);
}

struct Serializer
{
    NetworkReplicator::SerializeFunc Methods[2];
    void* Tags[2];
};

namespace
{
    CriticalSection ObjectsLock;
    HashSet<NetworkReplicatedObject> Objects;
    Dictionary<Guid, Guid> IdsRemappingTable;
    NetworkStream* CachedWriteStream = nullptr;
    NetworkStream* CachedReadStream = nullptr;
    Array<NetworkConnection> CachedTargets;
    Dictionary<ScriptingTypeHandle, Serializer> SerializersTable;
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

void INetworkSerializable_Serialize(void* instance, NetworkStream* stream, void* tag)
{
    ((INetworkSerializable*)instance)->Serialize(stream);
}

void INetworkSerializable_Deserialize(void* instance, NetworkStream* stream, void* tag)
{
    ((INetworkSerializable*)instance)->Deserialize(stream);
}

NetworkReplicatedObject* ResolveObject(Guid objectId, Guid ownerId, char objectTypeName[128])
{
    // Lookup object
    IdsRemappingTable.TryGet(objectId, objectId);
    const auto it = Objects.Find(objectId);
    if (it != Objects.End())
        return &it->Item;

    // Try to find the object within the same parent (eg. spawned locally on both client and server)
    IdsRemappingTable.TryGet(ownerId, ownerId);
    const ScriptingTypeHandle objectType = Scripting::FindScriptingType(StringAnsiView(objectTypeName));
    if (!objectType)
        return nullptr;
    for (auto& e : Objects)
    {
        auto& item = e.Item;
        const ScriptingObject* obj = item.Object.Get();
        if (item.OwnerId == ownerId &&
            item.LastFrameSync == 0 &&
            obj &&
            obj->GetTypeHandle() == objectType)
        {
            // Boost future lookups by using indirection
#if NETWORK_REPLICATOR_DEBUG_LOG
            LOG(Info, "[NetworkReplicator] Remap object ID={} into object {}:{}", objectId, item.ToString(), obj->GetType().ToString());
#endif
            IdsRemappingTable.Add(objectId, item.ObjectId);

            return &item;
        }
    }

    return nullptr;
}

#if !COMPILE_WITHOUT_CSHARP

#include "Engine/Scripting/ManagedCLR/MUtils.h"

void INetworkSerializable_Managed(void* instance, NetworkStream* stream, void* tag)
{
    auto signature = (Function<void(void*, void*)>::Signature)tag;
    signature(instance, stream);
}

void NetworkReplicator::AddSerializer(const ScriptingTypeHandle& typeHandle, const Function<void(void*, void*)>& serialize, const Function<void(void*, void*)>& deserialize)
{
    if (!typeHandle)
        return;

    // This assumes that C# glue code passed static method pointer (via Marshal.GetFunctionPointerForDelegate)
    const Serializer serializer{ INetworkSerializable_Managed, INetworkSerializable_Managed, *(SerializeFunc*)&serialize, *(SerializeFunc*)&deserialize };
    SerializersTable.Add(typeHandle, serializer);
}

#endif

void NetworkReplicator::AddSerializer(const ScriptingTypeHandle& typeHandle, SerializeFunc serialize, SerializeFunc deserialize, void* serializeTag, void* deserializeTag)
{
    const Serializer serializer{ serialize, deserialize, serializeTag, deserializeTag };
    SerializersTable.Add(typeHandle, serializer);
}

bool NetworkReplicator::InvokeSerializer(const ScriptingTypeHandle& typeHandle, void* instance, NetworkStream* stream, bool serialize)
{
    if (!typeHandle || !instance || !stream)
        return true;

    // Get serializers pair from table
    Serializer serializer;
    if (!SerializersTable.TryGet(typeHandle, serializer))
    {
        // Fallback to INetworkSerializable interface (if type implements it)
        const ScriptingType& type = typeHandle.GetType();
        const ScriptingType::InterfaceImplementation* interface = type.GetInterface(INetworkSerializable::TypeInitializer);
        if (interface)
        {
            serializer.Methods[0] = INetworkSerializable_Serialize;
            serializer.Methods[1] = INetworkSerializable_Deserialize;
            SerializersTable.Add(typeHandle, serializer);
        }
        else
            return true;
    }

    // Invoke serializer
    const byte idx = serialize ? 0 : 1;
    serializer.Methods[idx](instance, stream, serializer.Tags[idx]);
    return false;
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
    IdsRemappingTable.Clear();
    IdsRemappingTable.SetCapacity(0);
    SAFE_DELETE(CachedWriteStream);
    SAFE_DELETE(CachedReadStream);
    CachedTargets.Resize(0);
}

void NetworkInternal::NetworkReplicatorPreUpdate()
{
    // Inject ObjectsLookupIdMapping to properly map networked object ids into local object ids (deserialization with Scripting::TryFindObject will remap objects)
    Scripting::ObjectsLookupIdMapping.Set(&IdsRemappingTable);
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
            stream->Initialize();
            const bool failed = NetworkReplicator::InvokeSerializer(obj->GetTypeHandle(), obj, stream, true);
            if (failed)
            {
#if NETWORK_REPLICATOR_DEBUG_LOG
                if (!item.InvalidTypeWarn)
                {
                    item.InvalidTypeWarn = true;
                    LOG(Error, "[NetworkReplicator] Cannot serialize object {} of type {} (missing serialization logic)", item.ToString(), obj->GetType().ToString());
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

    // Clear networked objects mapping table
    Scripting::ObjectsLookupIdMapping.Set(nullptr);
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
        item.LastFrameSync = NetworkManager::Frame;

        // Setup message reading stream
        if (CachedReadStream == nullptr)
            CachedReadStream = New<NetworkStream>();
        NetworkStream* stream = CachedReadStream;
        stream->Initialize(event.Message.Buffer + event.Message.Position, msgData.DataSize);

        // Deserialize object
        const bool failed = NetworkReplicator::InvokeSerializer(obj->GetTypeHandle(), obj, stream, false);
        if (failed)
        {
#if NETWORK_REPLICATOR_DEBUG_LOG
            if (failed && !item.InvalidTypeWarn)
            {
                item.InvalidTypeWarn = true;
                LOG(Error, "[NetworkReplicator] Cannot serialize object {} of type {} (missing serialization logic)", item.ToString(), obj->GetType().ToString());
            }
#endif
        }
    }
    else
    {
        // TODO: put message to the queue to be resolved later (eg. object replication came before spawn packet) - use TTL to prevent memory overgrowing
    }
}
