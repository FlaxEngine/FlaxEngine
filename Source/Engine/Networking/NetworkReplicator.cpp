// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
#include "NetworkReplicationHierarchy.h"
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
#if USE_EDITOR
#include "FlaxEngine.Gen.h"
#endif

#if !BUILD_RELEASE
bool NetworkReplicator::EnableLog = false;
#include "Engine/Core/Log.h"
#include "Engine/Content/Content.h"
#define NETWORK_REPLICATOR_LOG(messageType, format, ...) if (NetworkReplicator::EnableLog) { LOG(messageType, format, ##__VA_ARGS__); }
#define USE_NETWORK_REPLICATOR_LOG 1
#else
#define NETWORK_REPLICATOR_LOG(messageType, format, ...)
#endif

#if COMPILE_WITH_PROFILER
bool NetworkInternal::EnableProfiling = false;
Dictionary<Pair<ScriptingTypeHandle, StringAnsiView>, NetworkInternal::ProfilerEvent> NetworkInternal::ProfilerEvents;
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
    uint32 OwnerSpawnId; // Unique for peer who spawned it and matches OwnerSpawnId inside following part messages
    Guid PrefabId;
    uint16 ItemsCount; // Total items count
    uint8 UseParts : 1; // True if spawn message is header-only and all items come in the separate parts
    });

PACK_STRUCT(struct NetworkMessageObjectSpawnPart
    {
    NetworkMessageIDs ID = NetworkMessageIDs::ObjectSpawnPart;
    uint32 OwnerClientId;
    uint32 OwnerSpawnId;
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
    Guid ParentId;
    char ObjectTypeName[128]; // TODO: introduce networked-name to synchronize unique names as ushort (less data over network)
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
    uint8 Spawned : 1;
    uint8 Synced : 1;
    DataContainer<uint32> TargetClientIds;
    INetworkObject* AsNetworkObject;

    NetworkReplicatedObject()
    {
        Spawned = 0;
        Synced = 0;
    }

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

struct SpawnItemParts
{
    NetworkMessageObjectSpawn MsgData;
    Array<NetworkMessageObjectSpawnItem> Items;
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
    DataContainer<uint32> Targets;
};

namespace
{
    CriticalSection ObjectsLock;
    HashSet<NetworkReplicatedObject> Objects;
    Array<ReplicateItem> ReplicationParts;
    Array<SpawnItemParts> SpawnParts;
    Array<SpawnItem> SpawnQueue;
    Array<DespawnItem> DespawnQueue;
    Array<RpcItem> RpcQueue;
    Dictionary<Guid, Guid> IdsRemappingTable;
    NetworkStream* CachedWriteStream = nullptr;
    NetworkStream* CachedReadStream = nullptr;
    NetworkReplicationHierarchyUpdateResult* CachedReplicationResult = nullptr;
    NetworkReplicationHierarchy* Hierarchy = nullptr;
    Array<NetworkClient*> NewClients;
    Array<NetworkConnection> CachedTargets;
    Dictionary<ScriptingTypeHandle, Serializer> SerializersTable;
#if !COMPILE_WITHOUT_CSHARP
    Dictionary<StringAnsiView, StringAnsi*> CSharpCachedNames;
#endif
    Array<Guid> DespawnedObjects;
    uint32 SpawnId = 0;

#if USE_EDITOR
    void OnScriptsReloading()
    {
        ScopeLock lock(ObjectsLock);
        if (Objects.HasItems())
            LOG(Warning, "Hot-reloading scripts with network objects active.");
        if (Hierarchy)
        {
            Delete(Hierarchy);
            Hierarchy = nullptr;
        }

        // Clear any references to non-engine scripts before code hot-reload
        BinaryModule* flaxModule = GetBinaryModuleFlaxEngine();
        for (auto i = SerializersTable.Begin(); i.IsNotEnd(); ++i)
        {
            if (i->Key.Module != flaxModule)
                SerializersTable.Remove(i);
        }
        for (auto i = NetworkRpcInfo::RPCsTable.Begin(); i.IsNotEnd(); ++i)
        {
            if (i->Key.First.Module != flaxModule)
                NetworkRpcInfo::RPCsTable.Remove(i);
        }
    }
#endif
}

class NetworkReplicationService : public EngineService
{
public:
    NetworkReplicationService()
        : EngineService(TEXT("Network Replication"), 1100)
    {
    }

    bool Init() override;
    void Dispose() override;
};

bool NetworkReplicationService::Init()
{
#if USE_EDITOR
    Scripting::ScriptsReloading.Bind(OnScriptsReloading);
#endif
    return false;
}

void NetworkReplicationService::Dispose()
{
    NetworkInternal::NetworkReplicatorClear();
#if !COMPILE_WITHOUT_CSHARP
    CSharpCachedNames.ClearDelete();
#endif
}

NetworkReplicationService NetworkReplicationServiceInstance;

void INetworkSerializable_Native_Serialize(void* instance, NetworkStream* stream, void* tag)
{
    const int16 vtableOffset = (int16)(intptr)tag;
    ((INetworkSerializable*)((byte*)instance + vtableOffset))->Serialize(stream);
}

void INetworkSerializable_Native_Deserialize(void* instance, NetworkStream* stream, void* tag)
{
    const int16 vtableOffset = (int16)(intptr)tag;
    ((INetworkSerializable*)((byte*)instance + vtableOffset))->Deserialize(stream);
}

void INetworkSerializable_Script_Serialize(void* instance, NetworkStream* stream, void* tag)
{
    auto obj = (ScriptingObject*)instance;
    auto interface = ScriptingObject::ToInterface<INetworkSerializable>(obj);
    interface->Serialize(stream);
}

void INetworkSerializable_Script_Deserialize(void* instance, NetworkStream* stream, void* tag)
{
    auto obj = (ScriptingObject*)instance;
    auto interface = ScriptingObject::ToInterface<INetworkSerializable>(obj);
    interface->Deserialize(stream);
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

NetworkReplicatedObject* ResolveObject(Guid objectId, Guid parentId, const char objectTypeName[128])
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

void BuildCachedTargets(const Array<NetworkClient*>& clients, const DataContainer<uint32>& clientIds, const uint32 excludedClientId = NetworkManager::ServerClientId, const NetworkClientsMask clientsMask = NetworkClientsMask::All)
{
    CachedTargets.Clear();
    if (clientIds.IsValid())
    {
        for (int32 clientIndex = 0; clientIndex < clients.Count(); clientIndex++)
        {
            const NetworkClient* client = clients.Get()[clientIndex];
            if (client->State == NetworkConnectionState::Connected && client->ClientId != excludedClientId && clientsMask.HasBit(clientIndex))
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
        for (int32 clientIndex = 0; clientIndex < clients.Count(); clientIndex++)
        {
            const NetworkClient* client = clients.Get()[clientIndex];
            if (client->State == NetworkConnectionState::Connected && client->ClientId != excludedClientId && clientsMask.HasBit(clientIndex))
                CachedTargets.Add(client->Connection);
        }
    }
}

void BuildCachedTargets(const Array<NetworkClient*>& clients, const DataContainer<uint32>& clientIds1, const Span<uint32>& clientIds2, const uint32 excludedClientId = NetworkManager::ServerClientId)
{
    CachedTargets.Clear();
    if (clientIds1.IsValid())
    {
        if (clientIds2.IsValid())
        {
            for (const NetworkClient* client : clients)
            {
                if (client->State == NetworkConnectionState::Connected && client->ClientId != excludedClientId)
                {
                    for (int32 i = 0; i < clientIds1.Length(); i++)
                    {
                        if (clientIds1[i] == client->ClientId)
                        {
                            for (int32 j = 0; j < clientIds2.Length(); j++)
                            {
                                if (clientIds2[j] == client->ClientId)
                                {
                                    CachedTargets.Add(client->Connection);
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            BuildCachedTargets(clients, clientIds1, excludedClientId);
        }
    }
    else
    {
        BuildCachedTargets(clients, clientIds2, excludedClientId);
    }
}

FORCE_INLINE void BuildCachedTargets(const NetworkReplicatedObject& item, const NetworkClientsMask clientsMask = NetworkClientsMask::All)
{
    // By default send object to all connected clients excluding the owner but with optional TargetClientIds list
    BuildCachedTargets(NetworkManager::Clients, item.TargetClientIds, item.OwnerClientId, clientsMask);
}

FORCE_INLINE void GetNetworkName(char buffer[128], const StringAnsiView& name)
{
    Platform::MemoryCopy(buffer, name.Get(), name.Length());
    buffer[name.Length()] = 0;
}

void SetupObjectSpawnMessageItem(SpawnItem* e, NetworkMessage& msg)
{
    ScriptingObject* obj = e->Object.Get();
    auto it = Objects.Find(obj->GetID());
    const auto& item = it->Item;

    // Add object into spawn message
    NetworkMessageObjectSpawnItem msgDataItem;
    msgDataItem.ObjectId = item.ObjectId;
    msgDataItem.ParentId = item.ParentId;
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

void SendObjectSpawnMessage(const SpawnGroup& group, const Array<NetworkClient*>& clients)
{
    PROFILE_CPU();
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
        const auto& item = it->Item;
        BuildCachedTargets(clients, item.TargetClientIds);
    }

    // Network Peer has fixed size of messages so split spawn message into parts if there are too many objects to fit at once
    msgData.OwnerSpawnId = ++SpawnId;
    msgData.UseParts = msg.BufferSize - msg.Position < group.Items.Count() * sizeof(NetworkMessageObjectSpawnItem);
    msg.WriteStructure(msgData);
    if (msgData.UseParts)
    {
        if (isClient)
            peer->EndSendMessage(NetworkChannelType::Reliable, msg);
        else
            peer->EndSendMessage(NetworkChannelType::Reliable, msg, CachedTargets);

        // Send spawn items in separate parts
        NetworkMessageObjectSpawnPart msgDataPart;
        msgDataPart.OwnerClientId = msgData.OwnerClientId;
        msgDataPart.OwnerSpawnId = msgData.OwnerSpawnId;
        uint16 itemIndex = 0;
        constexpr uint32 spawnItemMaxSize = sizeof(uint16) + sizeof(NetworkMessageObjectSpawnItem); // Index + Data
        while (itemIndex < msgData.ItemsCount)
        {
            msg = peer->BeginSendMessage();
            msg.WriteStructure(msgDataPart);

            // Write as many items as possible into this message
            while (msg.Position + spawnItemMaxSize <= msg.BufferSize && itemIndex < msgData.ItemsCount)
            {
                msg.WriteUInt16(itemIndex);
                SetupObjectSpawnMessageItem(group.Items[itemIndex], msg);
                itemIndex++;
            }

            if (isClient)
                peer->EndSendMessage(NetworkChannelType::Reliable, msg);
            else
                peer->EndSendMessage(NetworkChannelType::Reliable, msg, CachedTargets);
        }
    }
    else
    {
        // Send all spawn items within the spawn message
        for (SpawnItem* e : group.Items)
            SetupObjectSpawnMessageItem(e, msg);
        if (isClient)
            peer->EndSendMessage(NetworkChannelType::Reliable, msg);
        else
            peer->EndSendMessage(NetworkChannelType::Reliable, msg, CachedTargets);
    }
}

void SendObjectRoleMessage(const NetworkReplicatedObject& item, const NetworkClient* excludedClient = nullptr)
{
    NetworkMessageObjectRole msgData;
    msgData.ObjectId = item.ObjectId;
    IdsRemappingTable.KeyOf(msgData.ObjectId, &msgData.ObjectId);
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

void FindObjectsForSpawn(SpawnGroup& group, ChunkedArray<SpawnItem, 256>& spawnItems, ScriptingObject* obj)
{
    // Add any registered network objects
    auto it = Objects.Find(obj->GetID());
    if (it != Objects.End())
    {
        auto& item = it->Item;
        if (!item.Spawned)
        {
            // One of the parents of this object is being spawned so spawn it too
            item.Spawned = true;
            auto& spawnItem = spawnItems.AddOne();
            spawnItem.Object = obj;
            spawnItem.Targets.Link(item.TargetClientIds);
            spawnItem.OwnerClientId = item.OwnerClientId;
            spawnItem.Role = item.Role;
            group.Items.Add(&spawnItem);
        }
    }

    // Iterate over children
    if (auto* actor = ScriptingObject::Cast<Actor>(obj))
    {
        for (auto* script : actor->Scripts)
            FindObjectsForSpawn(group, spawnItems, script);
        for (auto* child : actor->Children)
            FindObjectsForSpawn(group, spawnItems, child);
    }
}

FORCE_INLINE void DirtyObjectImpl(NetworkReplicatedObject& item, ScriptingObject* obj)
{
    if (Hierarchy)
        Hierarchy->DirtyObject(obj);
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
    Scripting::ObjectsLookupIdMapping.Set(&IdsRemappingTable);
    const bool failed = NetworkReplicator::InvokeSerializer(obj->GetTypeHandle(), obj, stream, false);
    if (failed)
    {
        //NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Cannot serialize object {} of type {} (missing serialization logic)", item.ToString(), obj->GetType().ToString());
    }

    if (item.AsNetworkObject)
    {
        item.AsNetworkObject->OnNetworkDeserialize();
        if (!item.Synced)
        {
            item.Synced = true;
            item.AsNetworkObject->OnNetworkSync();
        }
    }

    // Speed up replication of client-owned objects to other clients from server to reduce lag (data has to go from client to server and then to other clients)
    if (NetworkManager::IsServer())
        DirtyObjectImpl(item, obj);
}

void InvokeObjectSpawn(const NetworkMessageObjectSpawn& msgData, const NetworkMessageObjectSpawnItem* msgDataItems)
{
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
                {
                    if (Hierarchy)
                        Hierarchy->AddObject(item.Object);
                    item.Role = NetworkObjectRole::Replicated;
                }
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
    Actor* prefabInstance = nullptr;
    Array<ScriptingObject*> objects;
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
                        ScriptingObject* obj = FindPrefabObject(child, rootItem.PrefabObjectID);
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

        // Resolve objects from prefab instance
        objects.Resize(msgData.ItemsCount);
        for (int32 i = 0; i < msgData.ItemsCount; i++)
        {
            auto& msgDataItem = msgDataItems[i];
            ScriptingObject* obj = FindPrefabObject(prefabInstance, msgDataItem.PrefabObjectID);
            if (!obj)
            {
                NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Failed to find object {} in prefab {}", msgDataItem.PrefabObjectID.ToString(), msgData.PrefabId.ToString());
                Delete(prefabInstance);
                return;
            }
            objects[i] = obj;
        }
    }
    else if (msgData.ItemsCount == 1)
    {
        // Spawn object
        const ScriptingTypeHandle objectType = Scripting::FindScriptingType(rootItem.ObjectTypeName);
        ScriptingObject* obj = ScriptingObject::NewObject(objectType);
        if (!obj)
        {
            NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Failed to spawn object type {}", String(rootItem.ObjectTypeName));
            return;
        }
        objects.Add(obj);
    }
    else
    {
        // Spawn objects
        objects.Resize(msgData.ItemsCount);
        for (int32 i = 0; i < msgData.ItemsCount; i++)
        {
            auto& msgDataItem = msgDataItems[i];
            const ScriptingTypeHandle objectType = Scripting::FindScriptingType(msgDataItem.ObjectTypeName);
            ScriptingObject* obj = ScriptingObject::NewObject(objectType);
            if (!obj)
            {
                NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Failed to spawn object type {}", String(msgDataItem.ObjectTypeName));
                for (ScriptingObject* e : objects)
                    Delete(e);
                return;
            }
            objects[i] = obj;
            if (i != 0)
            {
                // Link hierarchy of spawned objects before calling any networking code for them
                if (auto sceneObject = ScriptingObject::Cast<SceneObject>(obj))
                {
                    Actor* parent = nullptr;
                    for (int32 j = 0; j < i; j++)
                    {
                        if (msgDataItems[j].ObjectId == msgDataItem.ParentId)
                        {
                            parent = ScriptingObject::Cast<Actor>(objects[j]);
                            break;
                        }
                    }
                    if (parent)
                        sceneObject->SetParent(parent);
                }
            }
        }
    }

    // Add all newly spawned objects
    for (int32 i = 0; i < msgData.ItemsCount; i++)
    {
        auto& msgDataItem = msgDataItems[i];
        ScriptingObject* obj = objects[i];
        if (!obj->IsRegistered())
            obj->RegisterObject();
        const NetworkReplicatedObject* parent = ResolveObject(msgDataItem.ParentId);

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
        if (Hierarchy && item.Role == NetworkObjectRole::OwnedAuthoritative)
            Hierarchy->AddObject(obj);

        // Boost future lookups by using indirection
        NETWORK_REPLICATOR_LOG(Info, "[NetworkReplicator] Remap object ID={} into object {}:{}", msgDataItem.ObjectId, item.ToString(), obj->GetType().ToString());
        IdsRemappingTable.Add(msgDataItem.ObjectId, item.ObjectId);
    }

    // Spawn all newly spawned objects (ensure to have valid ownership hierarchy set before spawning object)
    for (int32 i = 0; i < msgData.ItemsCount; i++)
    {
        auto& msgDataItem = msgDataItems[i];
        ScriptingObject* obj = objects[i];
        auto it = Objects.Find(obj->GetID());
        auto& item = it->Item;
        const NetworkReplicatedObject* parent = ResolveObject(msgDataItem.ParentId);

        // Automatic parenting for scene objects
        auto sceneObject = ScriptingObject::Cast<SceneObject>(obj);
        if (sceneObject)
        {
            if (parent && parent->Object.Get() && parent->Object->Is<Actor>())
                sceneObject->SetParent(parent->Object.As<Actor>());
            else if (auto* parentActor = Scripting::TryFindObject<Actor>(msgDataItem.ParentId))
                sceneObject->SetParent(parentActor);
            else if (msgDataItem.ParentId.IsValid())
            {
#if USE_NETWORK_REPLICATOR_LOG
                // Ignore case when parent object in a message was a scene (eg. that is already unloaded on a client)
                AssetInfo assetInfo;
                if (!Content::GetAssetInfo(msgDataItem.ParentId, assetInfo) || assetInfo.TypeName != TEXT("FlaxEngine.SceneAsset"))
                {
                    NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Failed to find object {} as parent to spawned object", msgDataItem.ParentId.ToString());
                }
#endif
            }
        }
        else if (!parent && msgDataItem.ParentId.IsValid())
        {
            NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Failed to find object {} as parent to spawned object", msgDataItem.ParentId.ToString());
        }

        if (item.AsNetworkObject)
            item.AsNetworkObject->OnNetworkSpawn();
    }

    // TODO: if  we're server then spawn this object further on other clients (use TargetClientIds for that object - eg. object spawned by client on client for certain set of other clients only)
}

NetworkRpcParams::NetworkRpcParams(const NetworkStream* stream)
    : SenderId(stream->SenderId)
{
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

    NetworkRpcInfo rpcInfo;
    rpcInfo.Server = isServer;
    rpcInfo.Client = isClient;
    rpcInfo.Channel = (uint8)channel;
    rpcInfo.Invoke = nullptr; // C# RPCs invoking happens on C# side (build-time code generation)
    rpcInfo.Execute = RPC_Execute_Managed;
    rpcInfo.Tag = (void*)*(SerializeFunc*)&execute;

    // Add to the global RPCs table
    const NetworkRpcName rpcName(typeHandle, GetCSharpCachedName(name));
    NetworkRpcInfo::RPCsTable[rpcName] = rpcInfo;
}

bool NetworkReplicator::CSharpEndInvokeRPC(ScriptingObject* obj, const ScriptingTypeHandle& type, const StringAnsiView& name, NetworkStream* argsStream, MArray* targetIds)
{
    return EndInvokeRPC(obj, type, GetCSharpCachedName(name), argsStream, MUtils::ToSpan<uint32>(targetIds));
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

NetworkReplicationHierarchy* NetworkReplicator::GetHierarchy()
{
    return Hierarchy;
}

void NetworkReplicator::SetHierarchy(NetworkReplicationHierarchy* value)
{
    ScopeLock lock(ObjectsLock);
    if (Hierarchy == value)
        return;
    NETWORK_REPLICATOR_LOG(Info, "[NetworkReplicator] Set hierarchy to '{}'", value ? value->ToString() : String::Empty);
    if (Hierarchy)
    {
        // Clear old hierarchy
        Delete(Hierarchy);
    }
    Hierarchy = value;
    if (value)
    {
        // Add all owned objects to the hierarchy
        for (auto& e : Objects)
        {
            if (e.Item.Object && e.Item.Role == NetworkObjectRole::OwnedAuthoritative)
                value->AddObject(e.Item.Object);
        }
    }
}

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
            if (interface->IsNative)
            {
                // Native interface (implemented in C++)
                serializer.Methods[0] = INetworkSerializable_Native_Serialize;
                serializer.Methods[1] = INetworkSerializable_Native_Deserialize;
                serializer.Tags[0] = serializer.Tags[1] = (void*)(intptr)interface->VTableOffset; // Pass VTableOffset to the callback
            }
            else
            {
                // Generic interface (implemented in C# or elsewhere)
                ASSERT(type.Type == ScriptingTypes::Script);
                serializer.Methods[0] = INetworkSerializable_Script_Serialize;
                serializer.Methods[1] = INetworkSerializable_Script_Deserialize;
                serializer.Tags[0] = serializer.Tags[1] = nullptr;
            }
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

void NetworkReplicator::AddObject(ScriptingObject* obj, const ScriptingObject* parent)
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
    for (const SpawnItem& spawnItem : SpawnQueue)
    {
        if (spawnItem.HasOwnership && spawnItem.HierarchicalOwnership)
        {
            if (IsParentOf(obj, spawnItem.Object))
            {
                // Inherit ownership
                item.Role = spawnItem.Role;
                item.OwnerClientId = spawnItem.OwnerClientId;
                break;
            }
        }
    }
    Objects.Add(MoveTemp(item));
    if (Hierarchy && item.Role == NetworkObjectRole::OwnedAuthoritative)
        Hierarchy->AddObject(obj);
}

void NetworkReplicator::RemoveObject(ScriptingObject* obj)
{
    if (!obj || NetworkManager::IsOffline())
        return;
    ScopeLock lock(ObjectsLock);
    const auto it = Objects.Find(obj->GetID());
    if (it == Objects.End())
        return;

    // Remove object from the list
    NETWORK_REPLICATOR_LOG(Info, "[NetworkReplicator] Remove object {}, owned by {}", obj->GetID().ToString(), it->Item.ParentId.ToString());
    if (Hierarchy && it->Item.Role == NetworkObjectRole::OwnedAuthoritative)
        Hierarchy->RemoveObject(obj);
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
    if (Hierarchy && item.Role == NetworkObjectRole::OwnedAuthoritative)
        Hierarchy->RemoveObject(obj);
    Objects.Remove(it);
    DeleteNetworkObject(obj);
}

bool NetworkReplicator::HasObject(const ScriptingObject* obj)
{
    if (obj)
    {
        ScopeLock lock(ObjectsLock);
        const auto it = Objects.Find(obj->GetID());
        if (it != Objects.End())
            return true;
        for (const SpawnItem& item : SpawnQueue)
        {
            if (item.Object == obj)
                return true;
        }
    }
    return false;
}

void NetworkReplicator::MapObjectId(Guid& objectId)
{
    if (!IdsRemappingTable.TryGet(objectId, objectId))
    {
        // Try inverse mapping
        IdsRemappingTable.KeyOf(objectId, &objectId);
    }
}

void NetworkReplicator::AddObjectIdMapping(const ScriptingObject* obj, const Guid& objectId)
{
    CHECK(obj);
    CHECK(objectId.IsValid());
    const Guid id = obj->GetID();
    NETWORK_REPLICATOR_LOG(Info, "[NetworkReplicator] Remap object ID={} into object {}:{}", objectId, id.ToString(), obj->GetType().ToString());
    IdsRemappingTable[objectId] = id;
}

ScriptingObject* NetworkReplicator::ResolveForeignObject(Guid objectId)
{
    if (const auto& object = ResolveObject(objectId))
        return object->Object.Get();
    return nullptr;
}

uint32 NetworkReplicator::GetObjectOwnerClientId(const ScriptingObject* obj)
{
    uint32 id = NetworkManager::ServerClientId;
    if (obj && NetworkManager::IsConnected())
    {
        ScopeLock lock(ObjectsLock);
        const auto it = Objects.Find(obj->GetID());
        if (it != Objects.End())
            id = it->Item.OwnerClientId;
        else
        {
            for (const SpawnItem& item : SpawnQueue)
            {
                if (item.Object == obj)
                {
                    if (item.HasOwnership)
                        id = item.OwnerClientId;
#if USE_NETWORK_REPLICATOR_LOG
                    return id;
#else
                    break;
#endif
                }
            }
#if USE_NETWORK_REPLICATOR_LOG
            NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Failed to get ownership of unregistered network object {} ({})", obj->GetID(), obj->GetType().ToString());
#endif
        }
    }
    return id;
}

NetworkObjectRole NetworkReplicator::GetObjectRole(const ScriptingObject* obj)
{
    NetworkObjectRole role = NetworkObjectRole::None;
    if (obj && NetworkManager::IsConnected())
    {
        ScopeLock lock(ObjectsLock);
        const auto it = Objects.Find(obj->GetID());
        if (it != Objects.End())
            role = it->Item.Role;
        else
        {
            for (const SpawnItem& item : SpawnQueue)
            {
                if (item.Object == obj)
                {
                    if (item.HasOwnership)
                        role = item.Role;
#if USE_NETWORK_REPLICATOR_LOG
                    return role;
#else
                    break;
#endif
                }
            }
#if USE_NETWORK_REPLICATOR_LOG
            NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Failed to get ownership of unregistered network object {} ({})", obj->GetID(), obj->GetType().ToString());
#endif
        }
    }
    return role;
}

void NetworkReplicator::SetObjectOwnership(ScriptingObject* obj, uint32 ownerClientId, NetworkObjectRole localRole, bool hierarchical)
{
    if (!obj || NetworkManager::IsOffline())
        return;
    const Guid objectId = obj->GetID();
    ScopeLock lock(ObjectsLock);
    const auto it = Objects.Find(objectId);
    if (it == Objects.End())
    {
        // Special case if we're just spawning this object
        for (int32 i = 0; i < SpawnQueue.Count(); i++)
        {
            auto& item = SpawnQueue[i];
            if (item.Object == obj)
            {
#if !BUILD_RELEASE
                if (ownerClientId == NetworkManager::LocalClientId)
                {
                    // Ensure local client owns that object actually
                    if (localRole != NetworkObjectRole::OwnedAuthoritative)
                    {
                        LOG(Error, "Cannot change overship of object (Id={}) to the local client (Id={}) if the local role is not set to OwnedAuthoritative.", obj->GetID(), ownerClientId);
                        return;
                    }
                }
                else
                {
                    // Ensure local client doesn't own that object since it's owned by other client
                    if (localRole == NetworkObjectRole::OwnedAuthoritative)
                    {
                        LOG(Error, "Cannot change overship of object (Id={}) to the remote client (Id={}) if the local role is set to OwnedAuthoritative.", obj->GetID(), ownerClientId);
                        return;
                    }
                }
#endif
                item.HasOwnership = true;
                item.HierarchicalOwnership = hierarchical;
                item.OwnerClientId = ownerClientId;
                item.Role = localRole;
                break;
            }
        }
    }
    else
    {
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
#if !BUILD_RELEASE
                if (localRole == NetworkObjectRole::OwnedAuthoritative)
                {
                    LOG(Error, "Cannot change overship of object (Id={}) to the remote client (Id={}) if the local role is set to OwnedAuthoritative.", obj->GetID(), ownerClientId);
                    return;
                }
#endif
                if (Hierarchy && item.Role == NetworkObjectRole::OwnedAuthoritative)
                    Hierarchy->RemoveObject(obj);
                item.OwnerClientId = ownerClientId;
                item.LastOwnerFrame = 1;
                item.Role = localRole;
                SendObjectRoleMessage(item);
            }
        }
        else
        {
            // Allow to change local role of the object (except ownership)
#if !BUILD_RELEASE
                if (localRole == NetworkObjectRole::OwnedAuthoritative)
                {
                    LOG(Error, "Cannot change overship of object (Id={}) to the remote client (Id={}) if the local role is set to OwnedAuthoritative.", obj->GetID(), ownerClientId);
                    return;
                }
#endif
            if (Hierarchy && it->Item.Role == NetworkObjectRole::OwnedAuthoritative)
                Hierarchy->RemoveObject(obj);
            item.Role = localRole;
        }
    }

    // Go down hierarchy
    if (hierarchical)
    {
        for (auto& e : Objects)
        {
            if (e.Item.ParentId == objectId)
                SetObjectOwnership(e.Item.Object.Get(), ownerClientId, localRole, hierarchical);
        }

        for (const SpawnItem& spawnItem : SpawnQueue)
        {
            if (IsParentOf(spawnItem.Object, obj))
                SetObjectOwnership(spawnItem.Object, ownerClientId, localRole, hierarchical);
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
    Scripting::ObjectsLookupIdMapping.Set(&IdsRemappingTable);
    return CachedWriteStream;
}

bool NetworkReplicator::EndInvokeRPC(ScriptingObject* obj, const ScriptingTypeHandle& type, const StringAnsiView& name, NetworkStream* argsStream, Span<uint32> targetIds)
{
    if (targetIds.IsValid() && targetIds.Length() == 0)
        return true; // Target list is provided, but it's empty so nobody will get this RPC
    Scripting::ObjectsLookupIdMapping.Set(nullptr);
    const NetworkRpcInfo* info = NetworkRpcInfo::RPCsTable.TryGet(NetworkRpcName(type, name));
    if (!info || !obj || NetworkManager::IsOffline())
        return false;
    ObjectsLock.Lock();
    auto& rpc = RpcQueue.AddOne();
    rpc.Object = obj;
    rpc.Name.First = type;
    rpc.Name.Second = name;
    rpc.Info = *info;
    rpc.ArgsData.Copy(Span<byte>(argsStream->GetBuffer(), argsStream->GetPosition()));
    rpc.Targets.Copy(targetIds);
    ObjectsLock.Unlock();

    // Check if skip local execution (eg. server rpc called from client or client rpc with specific targets)
    const NetworkManagerMode networkMode = NetworkManager::Mode;
    if (info->Server && networkMode == NetworkManagerMode::Client)
        return true;
    if (info->Client && networkMode == NetworkManagerMode::Server)
        return true;
    if (info->Client && networkMode == NetworkManagerMode::Host && targetIds.IsValid() && !SpanContains(targetIds, NetworkManager::LocalClientId))
        return true;
    return false;
}

void NetworkInternal::NetworkReplicatorClientConnected(NetworkClient* client)
{
    ScopeLock lock(ObjectsLock);
    NewClients.Add(client);
    ASSERT(sizeof(NetworkClientsMask) * 8 >= (uint32)NetworkManager::Clients.Count()); // Ensure that clients mask can hold all of clients
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
            if (Hierarchy && item.Role == NetworkObjectRole::OwnedAuthoritative)
                Hierarchy->RemoveObject(obj);
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
    NetworkReplicator::SetHierarchy(nullptr);
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
    Objects.Clear();
    RpcQueue.Clear();
    SpawnQueue.Clear();
    DespawnQueue.Clear();
    IdsRemappingTable.Clear();
    SAFE_DELETE(CachedWriteStream);
    SAFE_DELETE(CachedReadStream);
    SAFE_DELETE(CachedReplicationResult);
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
                        // Inherit ownership
                        q.HasOwnership = true;
                        q.Role = e.Role;
                        q.OwnerClientId = e.OwnerClientId;
                        break;
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
                if (item.Role != e.Role)
                {
                    if (Hierarchy && item.Role == NetworkObjectRole::OwnedAuthoritative)
                        Hierarchy->RemoveObject(obj);
                    item.Role = e.Role;
                    if (Hierarchy && item.Role == NetworkObjectRole::OwnedAuthoritative)
                        Hierarchy->AddObject(obj);
                }
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
        ChunkedArray<SpawnItem, 256> spawnItems;
        for (SpawnGroup& g : spawnGroups)
        {
            // Include any added objects within spawn group that were not spawned manually (eg. AddObject for script/actor attached to spawned actor)
            ScriptingObject* groupRoot = g.Items[0]->Object.Get();
            FindObjectsForSpawn(g, spawnItems, groupRoot);

            SendObjectSpawnMessage(g, NetworkManager::Clients);

            spawnItems.Clear();
        }
        SpawnQueue.Clear();
    }

    // Apply parts replication
    {
        PROFILE_CPU_NAMED("ReplicationParts");
        for (int32 i = ReplicationParts.Count() - 1; i >= 0; i--)
        {
            auto& e = ReplicationParts[i];
            if (e.PartsLeft > 0)
            {
                // TODO: remove replication items after some TTL to reduce memory usage
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
    }

    // TODO: remove items from SpawnParts after some TTL to reduce memory usage

    // Replicate all owned networked objects with other clients or server
    if (!CachedReplicationResult)
        CachedReplicationResult = New<NetworkReplicationHierarchyUpdateResult>();
    CachedReplicationResult->Init();
    if ((!isClient && NetworkManager::Clients.IsEmpty()) || NetworkManager::NetworkFPS < -ZeroTolerance)
    {
        // No need to update replication when nobody's around or when replication is disabled
    }
    else if (Hierarchy)
    {
        // Tick using hierarchy
        PROFILE_CPU_NAMED("ReplicationHierarchyUpdate");
        Hierarchy->Update(CachedReplicationResult);
    }
    else
    {
        // Tick all owned objects
        PROFILE_CPU_NAMED("ReplicationUpdate");
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
            if (item.Role != NetworkObjectRole::OwnedAuthoritative)
                continue; // Send replication messages of only owned objects or from other client objects
            CachedReplicationResult->AddObject(obj);
        }
    }
    if (CachedReplicationResult->_entries.HasItems())
    {
        PROFILE_CPU_NAMED("Replication");
        if (CachedWriteStream == nullptr)
            CachedWriteStream = New<NetworkStream>();
        NetworkStream* stream = CachedWriteStream;
        stream->SenderId = NetworkManager::LocalClientId;
        // TODO: use Job System when replicated objects count is large
        for (auto& e : CachedReplicationResult->_entries)
        {
            ScriptingObject* obj = e.Object;
            auto it = Objects.Find(obj->GetID());
            if (it.IsEnd())
                continue;
            auto& item = it->Item;

            // Skip serialization of objects that none will receive
            if (!isClient)
            {
                BuildCachedTargets(item, e.TargetClients);
                if (CachedTargets.Count() == 0)
                    continue;
            }

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
            const uint32 size = stream->GetPosition();
            ASSERT(size <= MAX_uint16);
            NetworkMessageObjectReplicate msgData;
            msgData.OwnerFrame = NetworkManager::Frame;
            msgData.ObjectId = item.ObjectId;
            msgData.ParentId = item.ParentId;
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
            ASSERT(partsCount <= MAX_uint8);
            msgData.PartsCount = partsCount;
            NetworkMessage msg = peer->BeginSendMessage();
            msg.WriteStructure(msgData);
            msg.WriteBytes(stream->GetBuffer(), msgDataSize);
            uint32 dataSize = msgDataSize, messageSize = msg.Length;
            if (isClient)
                peer->EndSendMessage(NetworkChannelType::Unreliable, msg);
            else
                peer->EndSendMessage(NetworkChannelType::Unreliable, msg, CachedTargets);

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
                messageSize += msg.Length;
                dataSize += msgDataPart.PartSize;
                dataStart += msgDataPart.PartSize;
                if (isClient)
                    peer->EndSendMessage(NetworkChannelType::Unreliable, msg);
                else
                    peer->EndSendMessage(NetworkChannelType::Unreliable, msg, CachedTargets);
            }
            ASSERT_LOW_LAYER(dataStart == size);

#if COMPILE_WITH_PROFILER
            // Network stats recording
            if (EnableProfiling)
            {
                const Pair<ScriptingTypeHandle, StringAnsiView> name(obj->GetTypeHandle(), StringAnsiView::Empty);
                auto& profileEvent = ProfilerEvents[name];
                profileEvent.Count++;
                profileEvent.DataSize += dataSize;
                profileEvent.MessageSize += messageSize;
                profileEvent.Receivers += isClient ? 1 : CachedTargets.Count();
            }
#endif
        }
    }

    // Invoke RPCs
    {
        PROFILE_CPU_NAMED("Rpc");
        for (auto& e : RpcQueue)
        {
            ScriptingObject* obj = e.Object.Get();
            if (!obj)
                continue;
            auto it = Objects.Find(obj->GetID());
            if (it == Objects.End())
            {
#if USE_EDITOR || !BUILD_RELEASE
                if (!DespawnedObjects.Contains(obj->GetID()))
                    LOG(Error, "Cannot invoke RPC method '{0}.{1}' on object '{2}' that is not registered in networking (use 'NetworkReplicator.AddObject').", e.Name.First.ToString(), String(e.Name.Second), obj->GetID());
#endif
                continue;
            }
            auto& item = it->Item;

            // Send RPC message
            //NETWORK_REPLICATOR_LOG(Info, "[NetworkReplicator] Rpc {}::{} object ID={}", e.Name.First.ToString(), String(e.Name.Second), item.ToString());
            NetworkMessageObjectRpc msgData;
            msgData.ObjectId = item.ObjectId;
            msgData.ParentId = item.ParentId;
            {
                // Remap local client object ids into server ids
                IdsRemappingTable.KeyOf(msgData.ObjectId, &msgData.ObjectId);
                IdsRemappingTable.KeyOf(msgData.ParentId, &msgData.ParentId);
            }
            GetNetworkName(msgData.ObjectTypeName, obj->GetType().Fullname);
            GetNetworkName(msgData.RpcTypeName, e.Name.First.GetType().Fullname);
            GetNetworkName(msgData.RpcName, e.Name.Second);
            msgData.ArgsSize = (uint16)e.ArgsData.Length();
            NetworkMessage msg = peer->BeginSendMessage();
            msg.WriteStructure(msgData);
            msg.WriteBytes(e.ArgsData.Get(), e.ArgsData.Length());
            uint32 dataSize = e.ArgsData.Length(), messageSize = msg.Length, receivers = 0;
            NetworkChannelType channel = (NetworkChannelType)e.Info.Channel;
            if (e.Info.Server && isClient)
            {
                // Client -> Server
#if USE_NETWORK_REPLICATOR_LOG
                if (e.Targets.Length() != 0)
                    NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Server RPC '{}::{}' called with non-empty list of targets is not supported (only server will receive it)", e.Name.First.ToString(), e.Name.Second.ToString());
#endif
                peer->EndSendMessage(channel, msg);
                receivers = 1;
            }
            else if (e.Info.Client && (isServer || isHost))
            {
                // Server -> Client(s)
                BuildCachedTargets(NetworkManager::Clients, item.TargetClientIds, e.Targets, NetworkManager::LocalClientId);
                peer->EndSendMessage(channel, msg, CachedTargets);
                receivers = CachedTargets.Count();
            }

#if COMPILE_WITH_PROFILER
            // Network stats recording
            if (EnableProfiling && receivers)
            {
                auto& profileEvent = ProfilerEvents[e.Name];
                profileEvent.Count++;
                profileEvent.DataSize += dataSize;
                profileEvent.MessageSize += messageSize;
                profileEvent.Receivers += receivers;
            }
#endif
        }
        RpcQueue.Clear();
    }

    // Clear networked objects mapping table
    Scripting::ObjectsLookupIdMapping.Set(nullptr);
}

void NetworkInternal::OnNetworkMessageObjectReplicate(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer)
{
    PROFILE_CPU();
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

    const uint32 senderClientId = client ? client->ClientId : NetworkManager::ServerClientId;
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
    PROFILE_CPU();
    NetworkMessageObjectReplicatePart msgData;
    event.Message.ReadStructure(msgData);
    ScopeLock lock(ObjectsLock);
    if (DespawnedObjects.Contains(msgData.ObjectId))
        return; // Skip replicating not-existing objects

    const uint32 senderClientId = client ? client->ClientId : NetworkManager::ServerClientId;
    AddObjectReplicateItem(event, msgData, msgData.PartStart, msgData.PartSize, senderClientId);
}

void NetworkInternal::OnNetworkMessageObjectSpawn(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer)
{
    PROFILE_CPU();
    NetworkMessageObjectSpawn msgData;
    event.Message.ReadStructure(msgData);
    if (msgData.ItemsCount == 0)
        return;
    if (msgData.UseParts)
    {
        // Allocate spawn message parts collecting
        auto& parts = SpawnParts.AddOne();
        parts.MsgData = msgData;
        parts.Items.Resize(msgData.ItemsCount);
        for (auto& item : parts.Items)
            item.ObjectId = Guid::Empty; // Mark as not yet received
    }
    else
    {
        const auto* msgDataItems = (NetworkMessageObjectSpawnItem*)event.Message.SkipBytes(msgData.ItemsCount * sizeof(NetworkMessageObjectSpawnItem));
        InvokeObjectSpawn(msgData, msgDataItems);
    }
}

void NetworkInternal::OnNetworkMessageObjectSpawnPart(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer)
{
    PROFILE_CPU();
    NetworkMessageObjectSpawnPart msgData;
    event.Message.ReadStructure(msgData);
    int32 spawnPartsIndex;
    for (spawnPartsIndex = 0; spawnPartsIndex < SpawnParts.Count(); spawnPartsIndex++)
    {
        // Find spawn parts container that matches this spawn message (unique pair of sender and id assigned by sender)
        const auto& e = SpawnParts.Get()[spawnPartsIndex];
        if (e.MsgData.OwnerClientId == msgData.OwnerClientId && e.MsgData.OwnerSpawnId == msgData.OwnerSpawnId)
            break;
    }
    if (spawnPartsIndex >= SpawnParts.Count())
    {
        // Invalid part or data, ignore it
        return;
    }
    auto& spawnParts = SpawnParts.Get()[spawnPartsIndex];

    // Read all items from this part
    constexpr uint32 spawnItemMaxSize = sizeof(uint16) + sizeof(NetworkMessageObjectSpawnItem); // Index + Data
    while (event.Message.Position + spawnItemMaxSize <= event.Message.BufferSize)
    {
        const uint16 itemIndex = event.Message.ReadUInt16();
        event.Message.ReadStructure(spawnParts.Items[itemIndex]);
    }

    // Invoke spawning if we've got all items
    for (auto& e : spawnParts.Items)
    {
        if (!e.ObjectId.IsValid())
            return;
    }
    InvokeObjectSpawn(spawnParts.MsgData, spawnParts.Items.Get());
    SpawnParts.RemoveAt(spawnPartsIndex);
}

void NetworkInternal::OnNetworkMessageObjectDespawn(NetworkEvent& event, NetworkClient* client, NetworkPeer* peer)
{
    PROFILE_CPU();
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
        if (Hierarchy && item.Role == NetworkObjectRole::OwnedAuthoritative)
            Hierarchy->RemoveObject(obj);
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
    PROFILE_CPU();
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
            if (Hierarchy && item.Role != NetworkObjectRole::OwnedAuthoritative)
                Hierarchy->AddObject(obj);
            item.Role = NetworkObjectRole::OwnedAuthoritative;
            item.LastOwnerFrame = 0;
        }
        else if (item.Role == NetworkObjectRole::OwnedAuthoritative)
        {
            // Downgrade ownership automatically
            if (Hierarchy)
                Hierarchy->RemoveObject(obj);
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
    PROFILE_CPU();
    NetworkMessageObjectRpc msgData;
    event.Message.ReadStructure(msgData);
    ScopeLock lock(ObjectsLock);

    // Find RPC info
    NetworkRpcName name;
    name.First = Scripting::FindScriptingType(msgData.RpcTypeName);
    name.Second = msgData.RpcName;
    const NetworkRpcInfo* info = NetworkRpcInfo::RPCsTable.TryGet(name);
    if (!info)
    {
        NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Unknown RPC {}::{} for object {}", String(msgData.RpcTypeName), String(msgData.RpcName), msgData.ObjectId);
        return;
    }

    NetworkReplicatedObject* e = ResolveObject(msgData.ObjectId, msgData.ParentId, msgData.ObjectTypeName);
    if (e)
    {
        auto& item = *e;
        ScriptingObject* obj = item.Object.Get();
        if (!obj)
            return;

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
        stream->SenderId = client ? client->ClientId : NetworkManager::ServerClientId;
        stream->Initialize(event.Message.Buffer + event.Message.Position, msgData.ArgsSize);

        // Execute RPC
        info->Execute(obj, stream, info->Tag);
    }
    else if (info->Channel != static_cast<uint8>(NetworkChannelType::Unreliable) && info->Channel != static_cast<uint8>(NetworkChannelType::UnreliableOrdered))
    {
        NETWORK_REPLICATOR_LOG(Error, "[NetworkReplicator] Unknown object {} RPC {}::{}", msgData.ObjectId, String(msgData.RpcTypeName), String(msgData.RpcName));
    }
}
