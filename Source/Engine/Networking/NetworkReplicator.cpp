// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "NetworkReplicator.h"
#include "NetworkClient.h"
#include "NetworkManager.h"
#include "NetworkInternal.h"
#include "NetworkStream.h"
#include "NetworkMessage.h"
#include "NetworkPeer.h"
#include "NetworkChannelType.h"
#include "NetworkEvent.h"
#include "NetworkRpc.h"
#include "INetworkSerializable.h"
#include "INetworkObject.h"
#include "Engine/Core/Collections/HashSet.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Core/Collections/ChunkedArray.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Level/Actor.h"
#include "Engine/Level/SceneObject.h"
#include "Engine/Level/Prefabs/Prefab.h"
#include "Engine/Level/Prefabs/PrefabManager.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/Script.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Threading/ThreadLocal.h"

#if !BUILD_RELEASE
bool NetworkReplicator::EnableLog = false;
#include "Engine/Core/Log.h"
#define NETWORK_REPLICATOR_LOG(messageType, format, ...) if (NetworkReplicator::EnableLog) { LOG(messageType, format, ##__VA_ARGS__); }
#else
#define NETWORK_REPLICATOR_LOG(messageType, format, ...)
#endif

PACK_STRUCT(struct NetworkMessageObjectReplicate
    {
    NetworkMessageIDs ID = NetworkMessageIDs::ObjectReplicate;
    uint32 OwnerFrame;
    Guid ObjectId; // TODO: introduce networked-ids to synchronize unique ids as ushort (less data over network)
    Guid ParentId;
    char ObjectTypeName[128]; // TODO: introduce networked-name to synchronize unique names as ushort (less data over network)
    uint16 DataSize;
    uint16 PartsCount;
    });

PACK_STRUCT(struct NetworkMessageObjectReplicatePart
    {
    NetworkMessageIDs ID = NetworkMessageIDs::ObjectReplicatePart;
    uint32 OwnerFrame;
    uint16 DataSize;
    uint16 PartsCount;
    uint16 PartStart;
    uint16 PartSize;
    Guid ObjectId; // TODO: introduce networked-ids to synchronize unique ids as ushort (less data over network)
    });

PACK_STRUCT(struct NetworkMessageObjectSpawn
    {
    NetworkMessageIDs ID = NetworkMessageIDs::ObjectSpawn;
    uint32 OwnerClientId;
    Guid PrefabId;
    uint16 ItemsCount;
    });

PACK_STRUCT(struct NetworkMessageObjectSpawnItem
    {
    Guid ObjectId;
    Guid ParentId;
    Guid PrefabObjectID;
    char ObjectTypeName[128]; // TODO: introduce networked-name to synchronize unique names as ushort (less data over network)
    });

PACK_STRUCT(struct NetworkMessageObjectDespawn
    {
    NetworkMessageIDs ID = NetworkMessageIDs::ObjectDespawn;
    Guid ObjectId;
    });

PACK_STRUCT(struct NetworkMessageObjectRole
    {
    NetworkMessageIDs ID = NetworkMessageIDs::ObjectRole;
    Guid ObjectId;
    uint32 OwnerClientId;
    });

PACK_STRUCT(struct NetworkMessageObjectRpc
    {
    NetworkMessageIDs ID = NetworkMessageIDs::ObjectRpc;
    Guid ObjectId;
    char RpcTypeName[128]; // TODO: introduce networked-name to synchronize unique names as ushort (less data over network)
    char RpcName[128]; // TODO: introduce networked-name to synchronize unique names as ushort (less data over network)
    uint16 ArgsSize;
    });

struct NetworkReplicatedObject
{
    ScriptingObjectReference<ScriptingObject> Object;
    Guid ObjectId;
    Guid ParentId;
    uint32 OwnerClientId;
    uint32 LastOwnerFrame = 0;
    NetworkObjectRole Role;
    uint8 Spawned = false;
    DataContainer<uint32> TargetClientIds;
    INetworkObject* AsNetworkObject;

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

struct ReplicateItem
{
    ScriptingObjectReference<ScriptingObject> Object;
    Guid ObjectId;
    uint16 PartsLeft;
    uint32 OwnerFrame;
    uint32 OwnerClientId;
    Array<byte> Data;
};

struct SpawnItem
{
    ScriptingObjectReference<ScriptingObject> Object;
    DataContainer<uint32> Targets;
    bool HasOwnership = false;
    bool HierarchicalOwnership = false;
    uint32 OwnerClientId;
    NetworkObjectRole Role;
};

struct SpawnGroup
{
    Array<SpawnItem*, InlinedAllocation<8>> Items;
};

struct DespawnItem
{
    Guid Id;
    DataContainer<uint32> Targets;
};

struct RpcItem
{
    ScriptingObjectReference<ScriptingObject> Object;
    NetworkRpcName Name;
    NetworkRpcInfo Info;
    BytesContainer ArgsData;
};

namespace
{
    CriticalSection ObjectsLock;
    HashSet<NetworkReplicatedObject> Objects;
    Array<ReplicateItem> ReplicationParts;
    Array<SpawnItem> SpawnQueue;
    Array<DespawnItem> DespawnQueue;
    Array<RpcItem> RpcQueue;
    Dictionary<Guid, Guid> IdsRemappingTable;
    NetworkStream* CachedWriteStream = nullptr;
    NetworkStream* CachedReadStream = nullptr;
    Array<NetworkClient*> NewClients;
    Array<NetworkConnection> CachedTargets;
    Dictionary<ScriptingTypeHandle, Serializer> SerializersTable;
#if !COMPILE_WITHOUT_CSHARP
    Dictionary<StringAnsiView, StringAnsi*> CSharpCachedNames;
#endif
    Array<Guid> DespawnedObjects;
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
#if !COMPILE_WITHOUT_CSHARP
    CSharpCachedNames.ClearDelete();
#endif
}

NetworkReplicationService NetworkReplicationServiceInstance;

void INetworkSerializable_Serialize(void* instance, NetworkStream* stream, void* tag)
{
    const int16 vtableOffset = (int16)(intptr)tag;
    ((INetworkSerializable*)((byte*)instance + vtableOffset))->Serialize(stream);
}

void INetworkSerializable_Deserialize(void* instance, NetworkStream* stream, void* tag)
{
    const int16 vtableOffset = (int16)(intptr)tag;
    ((INetworkSerializable*)((byte*)instance + vtableOffset))->Deserialize(stream);
}

NetworkReplicatedObject* ResolveObject(Guid objectId)
{
    auto it = Objects.Find(objectId);
    if (it != Objects.End())
        return &it->Item;
    IdsRemappingTable.TryGet(objectId, objectId);
    it = Objects.Find(objectId);
    return it != Objects.End() ? &it->Item : nullptr;
}

NetworkReplicatedObject* ResolveObject(Guid objectId, Guid parentId, char objectTypeName[128])
{
    // Lookup object
    NetworkReplicatedObject* obj = ResolveObject(objectId);
    if (obj)
        return obj;

    // Try to find the object within the same parent (eg. spawned locally on both client and server)
    IdsRemappingTable.TryGet(parentId, parentId);
    const ScriptingTypeHandle objectType = Scripting::FindScriptingType(StringAnsiView(objectTypeName));
    if (!objectType)
        return nullptr;
    for (auto& e : Objects)
    {
        auto& item = e.Item;
        const ScriptingObject* obj = item.Object.Get();
        if (item.LastOwnerFrame == 0 &&
            item.ParentId == parentId &&
            obj &&
            obj->GetTypeHandle() == objectType &&
            !IdsRemappingTable.ContainsValue(item.ObjectId))
        {
            if (NetworkManager::IsClient())
            {
                // Boost future lookups by using indirection
                NETWORK_REPLICATOR_LOG(Info, "[NetworkReplicator] Remap object ID={} into object {}:{}", objectId, item.ToString(), obj->GetType().ToString());
                IdsRemappingTable.Add(objectId, item.ObjectId);
            }

            return &item;
        }
    }

    return nullptr;
}

void BuildCachedTargets(const Array<NetworkClient*>& clients)
{
    CachedTargets.Clear();
    for (const NetworkClient* client : clients)
    {
        if (client->State == NetworkConnectionState::Connected)
            CachedTargets.Add(client->Connection);
    }
}

void BuildCachedTargets(const Array<NetworkClient*>& clients, const NetworkClient* excludedClient)
{
    CachedTargets.Clear();
    for (const NetworkClient* client : clients)
    {
        if (client->State == NetworkConnectionState::Connected && client != excludedClient)
            CachedTargets.Add(client->Connection);
    }
}

void BuildCachedTargets(const Array<NetworkClient*>& clients, const DataContainer<uint32>& clientIds, const uint32 excludedClientId = NetworkManager::ServerClientId)
{
    CachedTargets.Clear();
    if (clientIds.IsValid())
    {
        for (const NetworkClient* client : clients)
        {
            if (client->State == NetworkConnectionState::Connected && client->ClientId != excludedClientId)
            {
                for (int32 i = 0; i < clientIds.Length(); i++)
                {
                    if (clientIds[i] == client->ClientId)
                    {
                        CachedTargets.Add(client->Connection);
                        break;
                    }
                }
            }
        }
    }
    else
    {
        for (const NetworkClient* client : clients)
        {
            if (client->State == NetworkConnectionState::Connected && client->ClientId != excludedClientId)
                CachedTargets.Add(client->Connection);
        }
    }
}

FORCE_INLINE void BuildCachedTargets(const NetworkReplicatedObject& item)
{
    // By default send object to all connected clients excluding the owner but with optional TargetClientIds list
    BuildCachedTargets(NetworkManager::Clients, item.TargetClientIds, item.OwnerClientId);
}

FORCE_INLINE void GetNetworkName(char buffer[128], const StringAnsiView& name)
{
    Platform::MemoryCopy(buffer, name.Get(), name.Length());
    buffer[name.Length()] = 0;
}

void SendObjectSpawnMessage(const SpawnGroup& group, const Array<NetworkClient*>& clients)
{
    const bool isClient = NetworkManager::IsClient();
    auto* peer = NetworkManager::Peer;
    NetworkMessage msg = peer->BeginSendMessage();
    NetworkMessageObjectSpawn msgData;
    msgData.ItemsCount = group.Items.Count();
    {
        // The first object is a root of the group (eg. prefab instance root actor)
        SpawnItem* e = group.Items[0];
        ScriptingObject* obj = e->Object.Get();
        msgData.OwnerClientId = e->OwnerClientId;
        auto* objScene = ScriptingObject::Cast<SceneObject>(obj);
        msgData.PrefabId = objScene && objScene->HasPrefabLink() ? objScene->GetPrefabID() : Guid::Empty;

        // Setup clients that should receive this spawn message
        auto it = Objects.Find(obj->GetID());
        auto& item = it->Item;
        BuildCachedTargets(clients, item.TargetClientIds);
    }
    msg.WriteStructure(msgData);
    for (SpawnItem* e : group.Items)
    {
        ScriptingObject* obj = e->Object.Get();
        auto it = Objects.Find(obj->GetID());
        auto& item = it->Item;

        // Add object into spawn message
        NetworkMessageObjectSpawnItem msgDataItem;
        msgDataItem.ObjectId = item.ObjectId;
        msgDataItem.ParentId = item.ParentId;
        if (isClient)
        {
            // Remap local client object ids into server ids
            IdsRemappingTable.KeyOf(msgDataItem.ObjectId, &msgDataItem.ObjectId);
            IdsRemappingTable.KeyOf(msgDataItem.ParentId, &msgDataItem.ParentId);
        }
        msgDataItem.PrefabObjectID = Guid::Empty;
        auto* objScene = ScriptingObject::Cast<SceneObject>(obj);
        if (objScene && objScene->HasPrefabLink())
            msgDataItem.PrefabObjectID = objScene->GetPrefabObjectID();
        GetNetworkName(msgDataItem.ObjectTypeName, obj->GetType().Fullname);
        msg.WriteStructure(msgDataItem);
    }
    if (isClient)
        peer->EndSendMessage(NetworkChannelType::Reliable, msg);
    else
        peer->EndSendMessage(NetworkChannelType::Reliable, msg, CachedTargets);
}

void SendObjectRoleMessage(const NetworkReplicatedObject& item, const NetworkClient* excludedClient = nullptr)
{
    NetworkMessageObjectRole msgData;
    msgData.ObjectId = item.ObjectId;
    msgData.OwnerClientId = item.OwnerClientId;
    auto peer = NetworkManager::Peer;
    NetworkMessage msg = peer->BeginSendMessage();
    msg.WriteStructure(msgData);
    if (NetworkManager::IsClient())
    {
        NetworkManager::Peer->EndSendMessage(NetworkChannelType::ReliableOrdered, msg);
    }
    else
    {
        BuildCachedTargets(NetworkManager::Clients, excludedClient);
        peer->EndSendMessage(NetworkChannelType::ReliableOrdered, msg, CachedTargets);
    }
}

void DeleteNetworkObject(ScriptingObject* obj)
{
    // Remove from the mapping table
    const Guid id = obj->GetID();
    IdsRemappingTable.Remove(id);
    IdsRemappingTable.RemoveValue(id);

    if (obj->Is<Script>() && ((Script*)obj)->GetParent())
        ((Script*)obj)->GetParent()->DeleteObject();
    else
        obj->DeleteObject();
}

bool IsParentOf(ScriptingObject* obj, ScriptingObject* parent)
{
    if (const auto* sceneObject = ScriptingObject::Cast<SceneObject>(obj))
        return sceneObject->GetParent() == parent || IsParentOf(sceneObject->GetParent(), parent);
    return false;
}

SceneObject* FindPrefabObject(Actor* a, const Guid& prefabObjectId)
{
    if (a->GetPrefabObjectID() == prefabObjectId)
        return a;
    for (auto* script : a->Scripts)
    {
        if (script->GetPrefabObjectID() == prefabObjectId)
            return script;
    }
    SceneObject* result = nullptr;
    for (int32 i = 0; i < a->Children.Count() && !result; i++)
        result = FindPrefabObject(a->Children[i], prefabObjectId);
    return result;
}

void SetupObjectSpawnGroupItem(ScriptingObject* obj, Array<SpawnGroup, InlinedAllocation<8>>& spawnGroups, SpawnItem& spawnItem)
{
    // Check if can fit this object into any of the existing groups (eg. script which can be spawned with parent actor)
    SpawnGroup* group = nullptr;
    for (auto& g : spawnGroups)
    {
        ScriptingObject* groupRoot = g.Items[0]->Object.Get();
        if (IsParentOf(obj, groupRoot))
        {
            // Reuse existing group (append)
            g.Items.Add(&spawnItem);
            group = &g;
            break;
        }
    }
    if (group)
        return;

    // Check if can override any of the existing groups (eg. actor which should be spawned before scripts)
    for (auto& g : spawnGroups)
    {
        ScriptingObject* groupRoot = g.Items[0]->Object.Get();
        if (IsParentOf(groupRoot, obj))
        {
            // Reuse existing group (as a root)
            g.Items.Insert(0, &spawnItem);
            group = &g;
            break;
        }
    }
    if (group)
        return;

    // Create new group
    group = &spawnGroups.AddOne();
    group->Items.Add(&spawnItem);
}

void DirtyObjectImpl(NetworkReplicatedObject& item, ScriptingObject* obj)
{
    // TODO: implement objects state replication frequency and dirtying
}

template<typename MessageType>
ReplicateItem* AddObjectReplicateItem(NetworkEvent& event, const MessageType& msgData, uint16 partStart, uint16 partSize, uint32 senderClientId)
{
    // Reuse or add part item
    ReplicateItem* replicateItem = nullptr;
    for (auto& e : ReplicationParts)
    {
        if (e.OwnerFrame == msgData.OwnerFrame && e.Data.Count() == msgData.DataSize && e.ObjectId == msgData.ObjectId)
        {
            // Reuse
            replicateItem = &e;
            break;
        }
    }
    if (!replicateItem)
    {
        // Add
        replicateItem = &ReplicationParts.AddOne();
        replicateItem->ObjectId = msgData.ObjectId;
        replicateItem->PartsLeft = msgData.PartsCount;
        replicateItem->OwnerFrame = msgData.OwnerFrame;
        replicateItem->OwnerClientId = senderClientId;
        replicateItem->Data.Resize(msgData.DataSize);
    }

    // Copy part data
    ASSERT(replicateItem->PartsLeft > 0);
    replicateItem->PartsLeft--;
    ASSERT(partStart + partSize <= replicateItem->Data.Count());
    const void* partData = event.Message.SkipBytes(partSize);
    Platform::MemoryCopy(replicateItem->Data.Get() + partStart, partData, partSize);

    return replicateItem;
}

void InvokeObjectReplication(NetworkReplicatedObject& item, uint32 ownerFrame, byte* data, uint32 dataSize, uint32 senderClientId)
{
    ScriptingObject* obj = item.Object.Get();
    if (!obj)
        return;

    // Skip replication if we own the object (eg. late replication message after ownership change)
    if (item.Role == NetworkObjectRole::OwnedAuthoritative)
        return;

    // Drop object replication if it has old data (eg. newer message was already processed due to unordered channel usage)
    if (item.LastOwnerFrame >= ownerFrame)
        return;
    item.LastOwnerFrame = ownerFrame;

    // Setup message reading stream
    if (CachedReadStream == nullptr)
        CachedReadStream = New<NetworkStream>();
    NetworkStream* stream = CachedReadStream;
    stream->Initialize(data, dataSize);
    stream->SenderId = senderClientId;

    // Deserialize object
    const bool failed = NetworkReplicator::InvokeSerializer(obj->GetTypeHandle(), obj, stream, false);
    if (failed)
    {
        //NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Cannot serialize object {} of type {} (missing serialization logic)", item.ToString(), obj->GetType().ToString());
    }

    if (item.AsNetworkObject)
        item.AsNetworkObject->OnNetworkDeserialize();

    // Speed up replication of client-owned objects to other clients from server to reduce lag (data has to go from client to server and then to other clients)
    if (NetworkManager::IsServer())
        DirtyObjectImpl(item, obj);
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
    // This assumes that C# glue code passed static method pointer (via Marshal.GetFunctionPointerForDelegate)
    AddSerializer(typeHandle, INetworkSerializable_Managed, INetworkSerializable_Managed, (void*)*(SerializeFunc*)&serialize, (void*)*(SerializeFunc*)&deserialize);
}

void RPC_Execute_Managed(ScriptingObject* obj, NetworkStream* stream, void* tag)
{
    auto signature = (Function<void(void*, void*)>::Signature)tag;
    signature(obj, stream);
}

void NetworkReplicator::AddRPC(const ScriptingTypeHandle& typeHandle, const StringAnsiView& name, const Function<void(void*, void*)>& execute, bool isServer, bool isClient, NetworkChannelType channel)
{
    if (!typeHandle)
        return;

    const NetworkRpcName rpcName(typeHandle, GetCSharpCachedName(name));

    NetworkRpcInfo rpcInfo;
    rpcInfo.Server = isServer;
    rpcInfo.Client = isClient;
    rpcInfo.Channel = (uint8)channel;
    rpcInfo.Invoke = nullptr; // C# RPCs invoking happens on C# side (build-time code generation)
    rpcInfo.Execute = RPC_Execute_Managed;
    rpcInfo.Tag = (void*)*(SerializeFunc*)&execute;

    // Add to the global RPCs table
    NetworkRpcInfo::RPCsTable[rpcName] = rpcInfo;
}

void NetworkReplicator::CSharpEndInvokeRPC(ScriptingObject* obj, const ScriptingTypeHandle& type, const StringAnsiView& name, NetworkStream* argsStream)
{
    EndInvokeRPC(obj, type, GetCSharpCachedName(name), argsStream);
}

StringAnsiView NetworkReplicator::GetCSharpCachedName(const StringAnsiView& name)
{
    // Cache method name on a heap to support C# hot-reloads (also glue code from C# passes view to the stack-only text so cache it here)
    StringAnsi* result;
    if (!CSharpCachedNames.TryGet(name, result))
    {
        result = New<StringAnsi>(name);
        CSharpCachedNames.Add(StringAnsiView(*result), result);
    }
    return StringAnsiView(*result);
}

#endif

void NetworkReplicator::AddSerializer(const ScriptingTypeHandle& typeHandle, SerializeFunc serialize, SerializeFunc deserialize, void* serializeTag, void* deserializeTag)
{
    if (!typeHandle)
        return;
    const Serializer serializer{ { serialize, deserialize }, { serializeTag, deserializeTag } };
    SerializersTable[typeHandle] = serializer;
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
            serializer.Tags[0] = serializer.Tags[1] = (void*)(intptr)interface->VTableOffset; // Pass VTableOffset to the callback
            SerializersTable.Add(typeHandle, serializer);
        }
        else if (const ScriptingTypeHandle baseTypeHandle = typeHandle.GetType().GetBaseType())
        {
            // Fallback to base type
            return InvokeSerializer(baseTypeHandle, instance, stream, serialize);
        }
        else
            return true;
    }

    // Invoke serializer
    const byte idx = serialize ? 0 : 1;
    serializer.Methods[idx](instance, stream, serializer.Tags[idx]);
    return false;
}

void NetworkReplicator::AddObject(ScriptingObject* obj, ScriptingObject* parent)
{
    if (!obj || NetworkManager::IsOffline())
        return;
    ScopeLock lock(ObjectsLock);
    if (Objects.Contains(obj))
        return;

    // Automatic parenting for scene objects
    if (!parent)
    {
        auto sceneObject = ScriptingObject::Cast<SceneObject>(obj);
        if (sceneObject)
            parent = sceneObject->GetParent();
    }

    // Ensure to register object in a scripting system (eg. lookup by ObjectId will work)
    if (!obj->IsRegistered())
        obj->RegisterObject();

    // Add object to the list
    NetworkReplicatedObject item;
    item.Object = obj;
    item.AsNetworkObject = ScriptingObject::ToInterface<INetworkObject>(obj);
    item.ObjectId = obj->GetID();
    item.ParentId = parent ? parent->GetID() : Guid::Empty;
    item.OwnerClientId = NetworkManager::ServerClientId; // Server owns objects by default
    item.Role = NetworkManager::IsClient() ? NetworkObjectRole::Replicated : NetworkObjectRole::OwnedAuthoritative;
    NETWORK_REPLICATOR_LOG(Info, "[NetworkReplicator] Add new object {}:{}, parent {}:{}", item.ToString(), obj->GetType().ToString(), item.ParentId.ToString(), parent ? parent->GetType().ToString() : String::Empty);
    Objects.Add(MoveTemp(item));
}

void NetworkReplicator::RemoveObject(ScriptingObject* obj)
{
    if (!obj || NetworkManager::IsOffline())
        return;
    ScopeLock lock(ObjectsLock);
    const auto it = Objects.Find(obj->GetID());
    if (it != Objects.End())
        return;

    // Remove object from the list
    NETWORK_REPLICATOR_LOG(Info, "[NetworkReplicator] Remove object {}, owned by {}", obj->GetID().ToString(), it->Item.ParentId.ToString());
    Objects.Remove(it);
}

void NetworkReplicator::SpawnObject(ScriptingObject* obj)
{
    DataContainer<uint32> clientIds;
    SpawnObject(obj, MoveTemp(clientIds));
}

void NetworkReplicator::SpawnObject(ScriptingObject* obj, const DataContainer<uint32>& clientIds)
{
    if (!obj || NetworkManager::IsOffline())
        return;
    ScopeLock lock(ObjectsLock);
    const auto it = Objects.Find(obj->GetID());
    if (it != Objects.End() && it->Item.Spawned)
        return; // Skip if object was already spawned

    // Register for spawning (batched during update)
    auto& spawn = SpawnQueue.AddOne();
    spawn.Object = obj;
    spawn.Targets.Copy(clientIds);
}

void NetworkReplicator::DespawnObject(ScriptingObject* obj)
{
    if (!obj || NetworkManager::IsOffline())
        return;
    ScopeLock lock(ObjectsLock);
    const auto it = Objects.Find(obj->GetID());
    if (it == Objects.End())
    {
        // Special case if we're just spawning this object
        for (int32 i = 0; i < SpawnQueue.Count(); i++)
        {
            auto& item = SpawnQueue[i];
            if (item.Object == obj)
            {
                SpawnQueue.RemoveAt(i);
                DeleteNetworkObject(obj);
                break;
            }
        }
        return;
    }
    auto& item = it->Item;
    if (item.Object != obj || !item.Spawned || item.OwnerClientId != NetworkManager::LocalClientId)
        return;

    // Register for despawning (batched during update)
    auto& despawn = DespawnQueue.AddOne();
    despawn.Id = obj->GetID();
    despawn.Targets = item.TargetClientIds;

    // Prevent spawning
    for (int32 i = 0; i < SpawnQueue.Count(); i++)
    {
        if (SpawnQueue[i].Object == obj)
        {
            SpawnQueue.RemoveAt(i);
            break;
        }
    }

    // Delete object locally
    NETWORK_REPLICATOR_LOG(Info, "[NetworkReplicator] Despawn object {}", item.ObjectId);
    DespawnedObjects.Add(item.ObjectId);
    if (item.AsNetworkObject)
        item.AsNetworkObject->OnNetworkDespawn();
    Objects.Remove(it);
    DeleteNetworkObject(obj);
}

uint32 NetworkReplicator::GetObjectOwnerClientId(ScriptingObject* obj)
{
    uint32 id = NetworkManager::ServerClientId;
    if (obj)
    {
        ScopeLock lock(ObjectsLock);
        const auto it = Objects.Find(obj->GetID());
        if (it != Objects.End())
            id = it->Item.OwnerClientId;
    }
    return id;
}

NetworkObjectRole NetworkReplicator::GetObjectRole(ScriptingObject* obj)
{
    NetworkObjectRole role = NetworkObjectRole::None;
    if (obj)
    {
        ScopeLock lock(ObjectsLock);
        const auto it = Objects.Find(obj->GetID());
        if (it != Objects.End())
            role = it->Item.Role;
    }
    return role;
}

void NetworkReplicator::SetObjectOwnership(ScriptingObject* obj, uint32 ownerClientId, NetworkObjectRole localRole, bool hierarchical)
{
    if (!obj)
        return;
    ScopeLock lock(ObjectsLock);
    const auto it = Objects.Find(obj->GetID());
    if (it == Objects.End())
    {
        // Special case if we're just spawning this object
        for (int32 i = 0; i < SpawnQueue.Count(); i++)
        {
            auto& item = SpawnQueue[i];
            if (item.Object == obj)
            {
                item.HasOwnership = true;
                item.HierarchicalOwnership = hierarchical;
                item.OwnerClientId = ownerClientId;
                item.Role = localRole;
                break;
            }
        }
        return;
    }
    auto& item = it->Item;
    if (item.Object != obj)
        return;

    // Check if this client is object owner
    if (item.OwnerClientId == NetworkManager::LocalClientId)
    {
        // Check if object owner will change
        if (item.OwnerClientId != ownerClientId)
        {
            // Change role locally
            CHECK(localRole != NetworkObjectRole::OwnedAuthoritative);
            item.OwnerClientId = ownerClientId;
            item.LastOwnerFrame = 1;
            item.Role = localRole;
            SendObjectRoleMessage(item);
        }
    }
    else
    {
        // Allow to change local role of the object (except ownership)
        CHECK(localRole != NetworkObjectRole::OwnedAuthoritative);
        item.Role = localRole;
    }

    // Go down hierarchy
    if (hierarchical)
    {
        for (auto& e : Objects)
        {
            if (e.Item.ParentId == item.ObjectId)
                SetObjectOwnership(e.Item.Object.Get(), ownerClientId, localRole, hierarchical);
        }
    }
}

void NetworkReplicator::DirtyObject(ScriptingObject* obj)
{
    ScopeLock lock(ObjectsLock);
    const auto it = Objects.Find(obj->GetID());
    if (it == Objects.End())
        return;
    auto& item = it->Item;
    if (item.Object != obj || item.Role != NetworkObjectRole::OwnedAuthoritative)
        return;
    DirtyObjectImpl(item, obj);
}

Dictionary<NetworkRpcName, NetworkRpcInfo> NetworkRpcInfo::RPCsTable;

NetworkStream* NetworkReplicator::BeginInvokeRPC()
{
    if (CachedWriteStream == nullptr)
        CachedWriteStream = New<NetworkStream>();
    CachedWriteStream->Initialize();
    CachedWriteStream->SenderId = NetworkManager::LocalClientId;
    return CachedWriteStream;
}

void NetworkReplicator::EndInvokeRPC(ScriptingObject* obj, const ScriptingTypeHandle& type, const StringAnsiView& name, NetworkStream* argsStream)
{
    const NetworkRpcInfo* info = NetworkRpcInfo::RPCsTable.TryGet(NetworkRpcName(type, name));
    if (!info || !obj || NetworkManager::IsOffline())
        return;
    ObjectsLock.Lock();
    auto& rpc = RpcQueue.AddOne();
    rpc.Object = obj;
    rpc.Name.First = type;
    rpc.Name.Second = name;
    rpc.Info = *info;
    const Span<byte> argsData(argsStream->GetBuffer(), argsStream->GetPosition());
    rpc.ArgsData.Copy(argsData);
#if USE_EDITOR || !BUILD_RELEASE
    auto it = Objects.Find(obj->GetID());
    if (it == Objects.End())
    {
        LOG(Error, "Cannot invoke RPC method '{0}.{1}' on object '{2}' that is not registered in networking (use 'NetworkReplicator.AddObject').", type.ToString(), String(name), obj->GetID());
    }
#endif
    ObjectsLock.Unlock();
}

void NetworkInternal::NetworkReplicatorClientConnected(NetworkClient* client)
{
    ScopeLock lock(ObjectsLock);
    NewClients.Add(client);
}

void NetworkInternal::NetworkReplicatorClientDisconnected(NetworkClient* client)
{
    ScopeLock lock(ObjectsLock);
    NewClients.Remove(client);

    // Remove any objects owned by that client
    const uint32 clientId = client->ClientId;
    for (auto it = Objects.Begin(); it.IsNotEnd(); ++it)
    {
        auto& item = it->Item;
        ScriptingObject* obj = item.Object.Get();
        if (obj && item.Spawned && item.OwnerClientId == clientId)
        {
            // Register for despawning (batched during update)
            auto& despawn = DespawnQueue.AddOne();
            despawn.Id = obj->GetID();
            despawn.Targets = MoveTemp(item.TargetClientIds);

            // Delete object locally
            NETWORK_REPLICATOR_LOG(Info, "[NetworkReplicator] Despawn object {}", item.ObjectId);
            if (item.AsNetworkObject)
                item.AsNetworkObject->OnNetworkDespawn();
            DeleteNetworkObject(obj);
            Objects.Remove(it);
        }
    }
}

void NetworkInternal::NetworkReplicatorClear()
{
    ScopeLock lock(ObjectsLock);

    // Cleanup
    NETWORK_REPLICATOR_LOG(Info, "[NetworkReplicator] Shutdown");
    for (auto it = Objects.Begin(); it.IsNotEnd(); ++it)
    {
        auto& item = it->Item;
        ScriptingObject* obj = item.Object.Get();
        if (obj && item.Spawned)
        {
            // Cleanup any spawned objects
            if (item.AsNetworkObject)
                item.AsNetworkObject->OnNetworkDespawn();
            DeleteNetworkObject(obj);
            Objects.Remove(it);
        }
    }
    RpcQueue.Clear();
    SpawnQueue.Clear();
    DespawnQueue.Clear();
    IdsRemappingTable.Clear();
    SAFE_DELETE(CachedWriteStream);
    SAFE_DELETE(CachedReadStream);
    NewClients.Clear();
    CachedTargets.Clear();
    DespawnedObjects.Clear();
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
    const bool isClient = NetworkManager::IsClient();
    const bool isServer = NetworkManager::IsServer();
    const bool isHost = NetworkManager::IsHost();
    NetworkPeer* peer = NetworkManager::Peer;

    if (!isClient && NewClients.Count() != 0)
    {
        // Sync any previously spawned objects with late-joining clients
        PROFILE_CPU_NAMED("NewClients");
        // TODO: try iterative loop over several frames to reduce both server and client perf-spikes in case of large amount of spawned objects
        ChunkedArray<SpawnItem, 256> spawnItems;
        Array<SpawnGroup, InlinedAllocation<8>> spawnGroups;
        for (auto it = Objects.Begin(); it.IsNotEnd(); ++it)
        {
            auto& item = it->Item;
            ScriptingObject* obj = item.Object.Get();
            if (!obj || !item.Spawned)
                continue;

            // Setup spawn item for this object
            auto& spawnItem = spawnItems.AddOne();
            spawnItem.Object = obj;
            spawnItem.Targets.Link(item.TargetClientIds);
            spawnItem.OwnerClientId = item.OwnerClientId;
            spawnItem.Role = item.Role;

            SetupObjectSpawnGroupItem(obj, spawnGroups, spawnItem);
        }

        // Groups of objects to spawn
        for (SpawnGroup& g : spawnGroups)
        {
            SendObjectSpawnMessage(g, NewClients);
        }
        NewClients.Clear();
    }

    // Despawn
    if (DespawnQueue.Count() != 0)
    {
        PROFILE_CPU_NAMED("DespawnQueue");
        for (DespawnItem& e : DespawnQueue)
        {
            // Send despawn message
            NETWORK_REPLICATOR_LOG(Info, "[NetworkReplicator] Despawn object ID={}", e.Id.ToString());
            NetworkMessageObjectDespawn msgData;
            msgData.ObjectId = e.Id;
            if (isClient)
            {
                // Remap local client object ids into server ids
                IdsRemappingTable.KeyOf(msgData.ObjectId, &msgData.ObjectId);
            }
            NetworkMessage msg = peer->BeginSendMessage();
            msg.WriteStructure(msgData);
            BuildCachedTargets(NetworkManager::Clients, e.Targets);
            if (isClient)
                peer->EndSendMessage(NetworkChannelType::ReliableOrdered, msg);
            else
                peer->EndSendMessage(NetworkChannelType::ReliableOrdered, msg, CachedTargets);
        }
        DespawnQueue.Clear();
    }

    // Spawn
    if (SpawnQueue.Count() != 0)
    {
        PROFILE_CPU_NAMED("SpawnQueue");

        // Propagate hierarchical ownership from spawned parent to spawned child objects (eg. spawned script and spawned actor with set hierarchical ownership on actor which should affect script too)
        // TODO: maybe we can propagate ownership within spawn groups only?
        for (SpawnItem& e : SpawnQueue)
        {
            if (e.HasOwnership && e.HierarchicalOwnership)
            {
                for (auto& q : SpawnQueue)
                {
                    if (!q.HasOwnership && IsParentOf(q.Object, e.Object))
                    {
                        q.HasOwnership = true;
                        q.Role = e.Role;
                        q.OwnerClientId = e.OwnerClientId;
                    }
                }
            }
        }

        // Batch spawned objects into groups (eg. player actor with scripts and child actors merged as a single spawn message)
        // That's because NetworkReplicator::SpawnObject can be called in separate for different actors/scripts of a single prefab instance but we want to spawn it at once over the network
        Array<SpawnGroup, InlinedAllocation<8>> spawnGroups;
        for (SpawnItem& e : SpawnQueue)
        {
            ScriptingObject* obj = e.Object.Get();
            if (!obj)
                continue;
            auto it = Objects.Find(obj->GetID());
            if (it == Objects.End())
            {
                // Ensure that object is added to the replication locally
                NetworkReplicator::AddObject(obj);
                it = Objects.Find(obj->GetID());
            }
            if (it == Objects.End())
                continue; // Skip deleted objects
            auto& item = it->Item;
            if (item.OwnerClientId != NetworkManager::LocalClientId || item.Role != NetworkObjectRole::OwnedAuthoritative)
                continue; // Skip spawning objects that we don't own

            if (e.HasOwnership)
            {
                item.Role = e.Role;
                item.OwnerClientId = e.OwnerClientId;
                if (e.HierarchicalOwnership)
                    NetworkReplicator::SetObjectOwnership(obj, e.OwnerClientId, e.Role, true);
            }
            if (e.Targets.IsValid())
            {
                // TODO: if we spawn object with custom set of targets clientsIds on client, then send it over to the server
                if (NetworkManager::IsClient())
                    MISSING_CODE("Sending TargetClientIds over to server for partial object replication.");
                item.TargetClientIds = MoveTemp(e.Targets);
            }
            item.Spawned = true;
            NETWORK_REPLICATOR_LOG(Info, "[NetworkReplicator] Spawn object ID={}", item.ToString());

            SetupObjectSpawnGroupItem(obj, spawnGroups, e);
        }

        // Spawn groups of objects
        for (SpawnGroup& g : spawnGroups)
        {
            SendObjectSpawnMessage(g, NetworkManager::Clients);
        }
        SpawnQueue.Clear();
    }

    // Apply parts replication
    for (int32 i = ReplicationParts.Count() - 1; i >= 0; i--)
    {
        auto& e = ReplicationParts[i];
        if (e.PartsLeft > 0)
        {
            // TODO: remove replication items after some TTL to prevent memory leaks
            continue;
        }
        ScriptingObject* obj = e.Object.Get();
        if (obj)
        {
            auto it = Objects.Find(obj->GetID());
            if (it != Objects.End())
            {
                auto& item = it->Item;

                // Replicate from all collected parts data
                InvokeObjectReplication(item, e.OwnerFrame, e.Data.Get(), e.Data.Count(), e.OwnerClientId);
            }
        }

        ReplicationParts.RemoveAt(i);
    }

    // Brute force synchronize all networked objects with clients
    if (CachedWriteStream == nullptr)
        CachedWriteStream = New<NetworkStream>();
    NetworkStream* stream = CachedWriteStream;
    stream->SenderId = NetworkManager::LocalClientId;
    // TODO: introduce NetworkReplicationHierarchy to optimize objects replication in large worlds (eg. batched culling networked scene objects that are too far from certain client to be relevant)
    // TODO: per-object sync interval (in frames) - could be scaled by hierarchy (eg. game could slow down sync rate for objects far from player)
    for (auto it = Objects.Begin(); it.IsNotEnd(); ++it)
    {
        auto& item = it->Item;
        ScriptingObject* obj = item.Object.Get();
        if (!obj)
        {
            // Object got deleted
            NETWORK_REPLICATOR_LOG(Info, "[NetworkReplicator] Remove object {}, owned by {}", item.ToString(), item.ParentId.ToString());
            Objects.Remove(it);
            continue;
        }
        if (item.Role != NetworkObjectRole::OwnedAuthoritative && (!isClient && item.OwnerClientId != NetworkManager::LocalClientId))
            continue; // Send replication messages of only owned objects or from other client objects

        if (item.AsNetworkObject)
            item.AsNetworkObject->OnNetworkSerialize();

        // Serialize object
        stream->Initialize();
        const bool failed = NetworkReplicator::InvokeSerializer(obj->GetTypeHandle(), obj, stream, true);
        if (failed)
        {
            //NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Cannot serialize object {} of type {} (missing serialization logic)", item.ToString(), obj->GetType().ToString());
            continue;
        }

        // Send object to clients
        {
            const uint32 size = stream->GetPosition();
            ASSERT(size <= MAX_uint16)
            NetworkMessageObjectReplicate msgData;
            msgData.OwnerFrame = NetworkManager::Frame;
            msgData.ObjectId = item.ObjectId;
            msgData.ParentId = item.ParentId;
            if (isClient)
            {
                // Remap local client object ids into server ids
                IdsRemappingTable.KeyOf(msgData.ObjectId, &msgData.ObjectId);
                IdsRemappingTable.KeyOf(msgData.ParentId, &msgData.ParentId);
            }
            GetNetworkName(msgData.ObjectTypeName, obj->GetType().Fullname);
            msgData.DataSize = size;
            const uint32 msgMaxData = peer->Config.MessageSize - sizeof(NetworkMessageObjectReplicate);
            const uint32 partMaxData = peer->Config.MessageSize - sizeof(NetworkMessageObjectReplicatePart);
            uint32 partsCount = 1;
            uint32 dataStart = 0;
            uint32 msgDataSize = size;
            if (size > msgMaxData)
            {
                // Send msgMaxData within first message
                msgDataSize = msgMaxData;
                dataStart += msgMaxData;

                // Send rest of the data in separate parts
                partsCount += Math::DivideAndRoundUp(size - dataStart, partMaxData);
            }
            else
                dataStart += size;
            ASSERT(partsCount <= MAX_uint8)
            msgData.PartsCount = partsCount;
            NetworkMessage msg = peer->BeginSendMessage();
            msg.WriteStructure(msgData);
            msg.WriteBytes(stream->GetBuffer(), msgDataSize);
            if (isClient)
                peer->EndSendMessage(NetworkChannelType::Unreliable, msg);
            else
            {
                // TODO: per-object relevancy for connected clients (eg. skip replicating actor to far players)
                BuildCachedTargets(item);
                peer->EndSendMessage(NetworkChannelType::Unreliable, msg, CachedTargets);
            }

            // Send all other parts
            for (uint32 partIndex = 1; partIndex < partsCount; partIndex++)
            {
                NetworkMessageObjectReplicatePart msgDataPart;
                msgDataPart.OwnerFrame = msgData.OwnerFrame;
                msgDataPart.ObjectId = msgData.ObjectId;
                msgDataPart.DataSize = msgData.DataSize;
                msgDataPart.PartsCount = msgData.PartsCount;
                msgDataPart.PartStart = dataStart;
                msgDataPart.PartSize = Math::Min(size - dataStart, partMaxData);
                msg = peer->BeginSendMessage();
                msg.WriteStructure(msgDataPart);
                msg.WriteBytes(stream->GetBuffer() + msgDataPart.PartStart, msgDataPart.PartSize);
                dataStart += msgDataPart.PartSize;
                if (isClient)
                    peer->EndSendMessage(NetworkChannelType::Unreliable, msg);
                else
                    peer->EndSendMessage(NetworkChannelType::Unreliable, msg, CachedTargets);
            }
            ASSERT_LOW_LAYER(dataStart == size);

            // TODO: stats for bytes send per object type
        }
    }

    // Invoke RPCs
    for (auto& e : RpcQueue)
    {
        ScriptingObject* obj = e.Object.Get();
        if (!obj)
            continue;
        auto it = Objects.Find(obj->GetID());
        if (it == Objects.End())
            continue;
        auto& item = it->Item;

        // Send despawn message
        //NETWORK_REPLICATOR_LOG(Info, "[NetworkReplicator] Rpc {}::{} object ID={}", e.Name.First.ToString(), String(e.Name.Second), item.ToString());
        NetworkMessageObjectRpc msgData;
        msgData.ObjectId = item.ObjectId;
        if (isClient)
        {
            // Remap local client object ids into server ids
            IdsRemappingTable.KeyOf(msgData.ObjectId, &msgData.ObjectId);
        }
        GetNetworkName(msgData.RpcTypeName, e.Name.First.GetType().Fullname);
        GetNetworkName(msgData.RpcName, e.Name.Second);
        msgData.ArgsSize = (uint16)e.ArgsData.Length();
        NetworkMessage msg = peer->BeginSendMessage();
        msg.WriteStructure(msgData);
        msg.WriteBytes(e.ArgsData.Get(), e.ArgsData.Length());
        NetworkChannelType channel = (NetworkChannelType)e.Info.Channel;
        if (e.Info.Server && isClient)
        {
            // Client -> Server
            peer->EndSendMessage(channel, msg);
        }
        else if (e.Info.Client && (isServer || isHost))
        {
            // Server -> Client(s)
            BuildCachedTargets(NetworkManager::Clients, item.TargetClientIds, NetworkManager::LocalClientId);
            peer->EndSendMessage(channel, msg, CachedTargets);
        }
    }
    RpcQueue.Clear();

    // Clear networked objects mapping table
    Scripting::ObjectsLookupIdMapping.Set(nullptr);
}

void NetworkInternal::OnNetworkMessageObjectReplicate(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer)
{
    NetworkMessageObjectReplicate msgData;
    event.Message.ReadStructure(msgData);
    ScopeLock lock(ObjectsLock);
    if (DespawnedObjects.Contains(msgData.ObjectId))
        return; // Skip replicating not-existing objects
    NetworkReplicatedObject* e = ResolveObject(msgData.ObjectId, msgData.ParentId, msgData.ObjectTypeName);
    if (!e)
        return;
    auto& item = *e;

    // Reject event from someone who is not an object owner
    if (client && item.OwnerClientId != client->ClientId)
        return;

    const uint32 senderClientId = client ? client->ClientId : NetworkManager::LocalClientId;
    if (msgData.PartsCount == 1)
    {
        // Replicate
        InvokeObjectReplication(item, msgData.OwnerFrame, event.Message.Buffer + event.Message.Position, msgData.DataSize, senderClientId);
    }
    else
    {
        // Add to replication from multiple parts
        const uint16 msgMaxData = peer->Config.MessageSize - sizeof(NetworkMessageObjectReplicate);
        ReplicateItem* replicateItem = AddObjectReplicateItem(event, msgData, 0, msgMaxData, senderClientId);
        replicateItem->Object = e->Object;
    }
}

void NetworkInternal::OnNetworkMessageObjectReplicatePart(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer)
{
    NetworkMessageObjectReplicatePart msgData;
    event.Message.ReadStructure(msgData);
    ScopeLock lock(ObjectsLock);
    if (DespawnedObjects.Contains(msgData.ObjectId))
        return; // Skip replicating not-existing objects

    const uint32 senderClientId = client ? client->ClientId : NetworkManager::LocalClientId;
    AddObjectReplicateItem(event, msgData, msgData.PartStart, msgData.PartSize, senderClientId);
}

void NetworkInternal::OnNetworkMessageObjectSpawn(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer)
{
    NetworkMessageObjectSpawn msgData;
    event.Message.ReadStructure(msgData);
    auto* msgDataItems = (NetworkMessageObjectSpawnItem*)event.Message.SkipBytes(msgData.ItemsCount * sizeof(NetworkMessageObjectSpawnItem));
    if (msgData.ItemsCount == 0)
        return;
    ScopeLock lock(ObjectsLock);

    // Check if that object has been already spawned
    auto& rootItem = msgDataItems[0];
    NetworkReplicatedObject* root = ResolveObject(rootItem.ObjectId, rootItem.ParentId, rootItem.ObjectTypeName);
    if (root)
    {
        // Object already exists locally so just synchronize the ownership (and mark as spawned)
        for (int32 i = 0; i < msgData.ItemsCount; i++)
        {
            auto& msgDataItem = msgDataItems[i];
            NetworkReplicatedObject* e = ResolveObject(msgDataItem.ObjectId, msgDataItem.ParentId, msgDataItem.ObjectTypeName);
            auto& item = *e;
            item.Spawned = true;
            if (NetworkManager::IsClient())
            {
                // Server always knows the best so update ownership of the existing object
                item.OwnerClientId = msgData.OwnerClientId;
                if (item.Role == NetworkObjectRole::OwnedAuthoritative)
                    item.Role = NetworkObjectRole::Replicated;
            }
            else if (item.OwnerClientId != msgData.OwnerClientId)
            {
                // Other client spawned object with a different owner
                // TODO: send reply message to inform about proper object ownership that client
            }
        }
        return;
    }

    // Recreate object locally (spawn only root)
    ScriptingObject* obj = nullptr;
    Actor* prefabInstance = nullptr;
    if (msgData.PrefabId.IsValid())
    {
        const NetworkReplicatedObject* parent = ResolveObject(rootItem.ParentId);
        Actor* parentActor = parent && parent->Object && parent->Object->Is<Actor>() ? parent->Object.As<Actor>() : nullptr;
        if (parentActor && parentActor->GetPrefabID() == msgData.PrefabId)
        {
            // Reuse parent object as prefab instance
            prefabInstance = parentActor;
        }
        else if ((parentActor = Scripting::TryFindObject<Actor>(rootItem.ParentId)))
        {
            // Try to find that spawned prefab (eg. prefab with networked script was spawned before so now we need to link it)
            for (Actor* child : parentActor->Children)
            {
                if (child->GetPrefabID() == msgData.PrefabId)
                {
                    if (Objects.Contains(child->GetID()))
                    {
                        obj = FindPrefabObject(child, rootItem.PrefabObjectID);
                        if (Objects.Contains(obj->GetID()))
                        {
                            // Other instance with already spawned network object
                            obj = nullptr;
                        }
                        else
                        {
                            // Reuse already spawned object within a parent
                            prefabInstance = child;
                            break;
                        }
                    }
                }
            }
        }
        if (!prefabInstance)
        {
            // Spawn prefab
            auto prefab = (Prefab*)LoadAsset(msgData.PrefabId, Prefab::TypeInitializer);
            if (!prefab)
            {
                NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Failed to find prefab {}", msgData.PrefabId.ToString());
                return;
            }
            prefabInstance = PrefabManager::SpawnPrefab(prefab, nullptr, nullptr);
            if (!prefabInstance)
            {
                NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Failed to spawn object type {}", msgData.PrefabId.ToString());
                return;
            }
        }
        if (!obj)
            obj = FindPrefabObject(prefabInstance, rootItem.PrefabObjectID);
        if (!obj)
        {
            NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Failed to find object {} in prefab {}", rootItem.PrefabObjectID.ToString(), msgData.PrefabId.ToString());
            Delete(prefabInstance);
            return;
        }
    }
    else
    {
        // Spawn object
        if (msgData.ItemsCount != 1)
        {
            NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Only prefab object spawning can contain more than one object (for type {})", String(rootItem.ObjectTypeName));
            return;
        }
        const ScriptingTypeHandle objectType = Scripting::FindScriptingType(rootItem.ObjectTypeName);
        obj = ScriptingObject::NewObject(objectType);
        if (!obj)
        {
            NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Failed to spawn object type {}", String(rootItem.ObjectTypeName));
            return;
        }
    }

    // Setup all newly spawned objects
    for (int32 i = 0; i < msgData.ItemsCount; i++)
    {
        auto& msgDataItem = msgDataItems[i];
        if (i != 0)
        {
            obj = FindPrefabObject(prefabInstance, msgDataItem.PrefabObjectID);
            if (!obj)
            {
                NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Failed to find object {} in prefab {}", msgDataItem.PrefabObjectID.ToString(), msgData.PrefabId.ToString());
                Delete(prefabInstance);
                return;
            }
        }
        if (!obj->IsRegistered())
            obj->RegisterObject();
        const NetworkReplicatedObject* parent = ResolveObject(msgDataItem.ParentId);
        if (!parent && msgDataItem.ParentId.IsValid())
        {
            NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Failed to find object {} as parent to spawned object", msgDataItem.ParentId.ToString());
        }

        // Add object to the list
        NetworkReplicatedObject item;
        item.Object = obj;
        item.AsNetworkObject = ScriptingObject::ToInterface<INetworkObject>(obj);
        item.ObjectId = obj->GetID();
        item.ParentId = parent ? parent->ObjectId : Guid::Empty;
        item.OwnerClientId = msgData.OwnerClientId;
        item.Role = NetworkObjectRole::Replicated;
        if (item.OwnerClientId == NetworkManager::LocalClientId)
        {
            // Upgrade ownership automatically (eg. server spawned object that local client should own)
            item.Role = NetworkObjectRole::OwnedAuthoritative;
        }
        item.Spawned = true;
        NETWORK_REPLICATOR_LOG(Info, "[NetworkReplicator] Add new object {}:{}, parent {}:{}", item.ToString(), obj->GetType().ToString(), item.ParentId.ToString(), parent ? parent->Object->GetType().ToString() : String::Empty);
        Objects.Add(MoveTemp(item));

        // Boost future lookups by using indirection
        NETWORK_REPLICATOR_LOG(Info, "[NetworkReplicator] Remap object ID={} into object {}:{}", msgDataItem.ObjectId, item.ToString(), obj->GetType().ToString());
        IdsRemappingTable.Add(msgDataItem.ObjectId, item.ObjectId);

        // Automatic parenting for scene objects
        auto sceneObject = ScriptingObject::Cast<SceneObject>(obj);
        if (sceneObject)
        {
            if (parent && parent->Object.Get() && parent->Object->Is<Actor>())
                sceneObject->SetParent(parent->Object.As<Actor>());
            else if (auto* parentActor = Scripting::TryFindObject<Actor>(msgDataItem.ParentId))
                sceneObject->SetParent(parentActor);
        }

        if (item.AsNetworkObject)
            item.AsNetworkObject->OnNetworkSpawn();
    }

    // TODO: if  we're server then spawn this object further on other clients (use TargetClientIds for that object - eg. object spawned by client on client for certain set of other clients only)
}

void NetworkInternal::OnNetworkMessageObjectDespawn(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer)
{
    NetworkMessageObjectDespawn msgData;
    event.Message.ReadStructure(msgData);
    ScopeLock lock(ObjectsLock);
    NetworkReplicatedObject* e = ResolveObject(msgData.ObjectId);
    if (e)
    {
        auto& item = *e;
        ScriptingObject* obj = item.Object.Get();
        if (!obj || !item.Spawned)
            return;

        // Reject event from someone who is not an object owner
        if (client && item.OwnerClientId != client->ClientId)
            return;

        // Remove object
        NETWORK_REPLICATOR_LOG(Info, "[NetworkReplicator] Despawn object {}", msgData.ObjectId);
        DespawnedObjects.Add(msgData.ObjectId);
        if (item.AsNetworkObject)
            item.AsNetworkObject->OnNetworkDespawn();
        Objects.Remove(obj);
        DeleteNetworkObject(obj);
    }
    else
    {
        NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Failed to despawn object {}", msgData.ObjectId);
    }
}

void NetworkInternal::OnNetworkMessageObjectRole(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer)
{
    NetworkMessageObjectRole msgData;
    event.Message.ReadStructure(msgData);
    ScopeLock lock(ObjectsLock);
    NetworkReplicatedObject* e = ResolveObject(msgData.ObjectId);
    if (e)
    {
        auto& item = *e;
        ScriptingObject* obj = item.Object.Get();
        if (!obj)
            return;

        // Reject event from someone who is not an object owner
        if (client && item.OwnerClientId != client->ClientId)
            return;

        // Update
        item.OwnerClientId = msgData.OwnerClientId;
        item.LastOwnerFrame = 1;
        if (item.OwnerClientId == NetworkManager::LocalClientId)
        {
            // Upgrade ownership automatically
            item.Role = NetworkObjectRole::OwnedAuthoritative;
            item.LastOwnerFrame = 0;
        }
        else if (item.Role == NetworkObjectRole::OwnedAuthoritative)
        {
            // Downgrade ownership automatically
            item.Role = NetworkObjectRole::Replicated;
        }
        if (!NetworkManager::IsClient())
        {
            // Server has to broadcast ownership message to the other clients
            SendObjectRoleMessage(item, client);
        }
    }
    else
    {
        NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Unknown object role update {}", msgData.ObjectId);
    }
}

void NetworkInternal::OnNetworkMessageObjectRpc(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer)
{
    NetworkMessageObjectRpc msgData;
    event.Message.ReadStructure(msgData);
    ScopeLock lock(ObjectsLock);
    NetworkReplicatedObject* e = ResolveObject(msgData.ObjectId);
    if (e)
    {
        auto& item = *e;
        ScriptingObject* obj = item.Object.Get();
        if (!obj)
            return;

        // Find RPC info
        NetworkRpcName name;
        name.First = Scripting::FindScriptingType(msgData.RpcTypeName);
        name.Second = msgData.RpcName;
        const NetworkRpcInfo* info = NetworkRpcInfo::RPCsTable.TryGet(name);
        if (!info)
        {
            NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Unknown object {} RPC {}::{}", msgData.ObjectId, String(msgData.RpcTypeName), String(msgData.RpcName));
            return;
        }

        // Validate RPC
        if (info->Server && NetworkManager::IsClient())
        {
            NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Cannot invoke server RPC {}::{} on client", String(msgData.RpcTypeName), String(msgData.RpcName));
            return;
        }
        if (info->Client && NetworkManager::IsServer())
        {
            NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Cannot invoke client RPC {}::{} on server", String(msgData.RpcTypeName), String(msgData.RpcName));
            return;
        }

        // Setup message reading stream
        if (CachedReadStream == nullptr)
            CachedReadStream = New<NetworkStream>();
        NetworkStream* stream = CachedReadStream;
        stream->SenderId = client ? client->ClientId : NetworkManager::LocalClientId;
        stream->Initialize(event.Message.Buffer + event.Message.Position, msgData.ArgsSize);

        // Execute RPC
        info->Execute(obj, stream, info->Tag);
    }
    else
    {
        NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Unknown object {} RPC {}::{}", msgData.ObjectId, String(msgData.RpcTypeName), String(msgData.RpcName));
    }
}
