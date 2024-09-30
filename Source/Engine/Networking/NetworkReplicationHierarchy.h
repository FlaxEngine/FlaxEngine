// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Scripting/ScriptingObjectReference.h"

class Actor;

/// <summary>
/// Network replication hierarchy object data.
/// </summary>
API_STRUCT(NoDefault, Namespace = "FlaxEngine.Networking") struct FLAXENGINE_API NetworkReplicationHierarchyObject
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkReplicationObjectInfo);

    // The object to replicate.
    API_FIELD() ScriptingObjectReference<ScriptingObject> Object;
    // The target amount of the replication updates per second (frequency of the replication). Constrained by NetworkManager::NetworkFPS. Use 0 for 'always relevant' object and less than 0 (eg. -1) for 'never relevant' objects that would only get synced on client join once (or upon DirtyObject).
    API_FIELD() float ReplicationFPS = 60;
    // The minimum distance from the player to the object at which it can process replication. For example, players further away won't receive object data. Use 0 if unused.
    API_FIELD() float CullDistance = 15000;
    // Runtime value for update frames left for the next replication of this object. Matches NetworkManager::NetworkFPS calculated from ReplicationFPS. Set to 1 if ReplicationFPS less than 0 to indicate dirty object.
    API_FIELD(Attributes="HideInEditor") uint16 ReplicationUpdatesLeft = 0;

    FORCE_INLINE NetworkReplicationHierarchyObject(const ScriptingObjectReference<ScriptingObject>& obj)
        : Object(obj.Get())
    {
    }

    FORCE_INLINE NetworkReplicationHierarchyObject(ScriptingObject* obj = nullptr)
        : Object(obj)
    {
    }

    // Gets the actors context (object itself or parent actor).
    Actor* GetActor() const;

    bool operator==(const NetworkReplicationHierarchyObject& other) const
    {
        return Object == other.Object;
    }

    bool operator==(const ScriptingObject* other) const
    {
        return Object == other;
    }
};

inline uint32 GetHash(const NetworkReplicationHierarchyObject& key)
{
    return GetHash(key.Object);
}

/// <summary>
/// Bit mask for NetworkClient list (eg. to selectively send object replication).
/// </summary>
API_STRUCT(NoDefault, Namespace = "FlaxEngine.Networking") struct FLAXENGINE_API NetworkClientsMask
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkClientsMask);

    // The first 64 bits (each for one client).
    API_FIELD() uint64 Word0 = 0;
    // The second 64 bits (each for one client).
    API_FIELD() uint64 Word1 = 0;

    // All bits set for all clients.
    API_FIELD() static NetworkClientsMask All;

    FORCE_INLINE bool HasBit(int32 bitIndex) const
    {
        const int32 wordIndex = bitIndex / 64;
        const uint64 wordMask = 1ull << (uint64)(bitIndex - wordIndex * 64);
        const uint64 word = *(&Word0 + wordIndex);
        return (word & wordMask) == wordMask;
    }

    FORCE_INLINE void SetBit(int32 bitIndex)
    {
        const int32 wordIndex = bitIndex / 64;
        const uint64 wordMask = 1ull << (uint64)(bitIndex - wordIndex * 64);
        uint64& word = *(&Word0 + wordIndex);
        word |= wordMask;
    }

    FORCE_INLINE void UnsetBit(int32 bitIndex)
    {
        const int32 wordIndex = bitIndex / 64;
        const uint64 wordMask = 1ull << (uint64)(bitIndex - wordIndex * 64);
        uint64& word = *(&Word0 + wordIndex);
        word &= ~wordMask;
    }

    FORCE_INLINE operator bool() const
    {
        return Word0 + Word1 != 0;
    }

    bool operator==(const NetworkClientsMask& other) const
    {
        return Word0 == other.Word0 && Word1 == other.Word1;
    }
};

/// <summary>
/// Network replication hierarchy output data to send.
/// </summary>
API_CLASS(Namespace = "FlaxEngine.Networking") class FLAXENGINE_API NetworkReplicationHierarchyUpdateResult : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(NetworkReplicationHierarchyUpdateResult, ScriptingObject);
    friend class NetworkInternal;
    friend class NetworkReplicationNode;
    friend class NetworkReplicationGridNode;

private:
    struct Client
    {
        bool HasLocation;
        Vector3 Location;
    };

    struct Entry
    {
        ScriptingObject* Object;
        NetworkClientsMask TargetClients;
    };

    bool _clientsHaveLocation;
    NetworkClientsMask _clientsMask;
    Array<Client> _clients;
    Array<Entry> _entries;

    void Init();

public:
    // Scales the ReplicationFPS property of objects in hierarchy. Can be used to slow down or speed up replication rate.
    API_FIELD() float ReplicationScale = 1.0f;

    // Adds object to the update results.
    API_FUNCTION() void AddObject(ScriptingObject* obj)
    {
        Entry& e = _entries.AddOne();
        e.Object = obj;
        e.TargetClients = NetworkClientsMask::All;
    }

    // Adds object to the update results. Defines specific clients to receive the update (server-only, unused on client). Mask matches NetworkManager::Clients.
    API_FUNCTION() void AddObject(ScriptingObject* obj, NetworkClientsMask targetClients)
    {
        Entry& e = _entries.AddOne();
        e.Object = obj;
        e.TargetClients = targetClients;
    }

    // Gets amount of the clients to use. Matches NetworkManager::Clients.
    API_PROPERTY() int32 GetClientsCount() const
    {
        return _clients.Count();
    }

    // Gets mask with all client bits set. Matches NetworkManager::Clients.
    API_PROPERTY() NetworkClientsMask GetClientsMask() const
    {
        return _clientsMask;
    }

    // Sets the viewer location for a certain client. Client index must match NetworkManager::Clients.
    API_FUNCTION() void SetClientLocation(int32 clientIndex, const Vector3& location);

    // Gets the viewer location for a certain client. Client index must match NetworkManager::Clients. Returns true if got a location set, otherwise false.
    API_FUNCTION() bool GetClientLocation(int32 clientIndex, API_PARAM(out) Vector3& location) const;
};

/// <summary>
/// Base class for the network objects replication hierarchy nodes. Contains a list of objects.
/// </summary>
API_CLASS(Abstract, Namespace = "FlaxEngine.Networking") class FLAXENGINE_API NetworkReplicationNode : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(NetworkReplicationNode, ScriptingObject);

    /// <summary>
    /// List with objects stored in this node.
    /// </summary>
    API_FIELD() Array<NetworkReplicationHierarchyObject> Objects;

    /// <summary>
    /// Adds an object into the hierarchy.
    /// </summary>
    /// <param name="obj">The object to add.</param>
    API_FUNCTION() virtual void AddObject(NetworkReplicationHierarchyObject obj);

    /// <summary>
    /// Removes object from the hierarchy.
    /// </summary>
    /// <param name="obj">The object to remove.</param>
    /// <returns>True on successful removal, otherwise false.</returns>
    API_FUNCTION() virtual bool RemoveObject(ScriptingObject* obj);

    /// <summary>
    /// Gets object from the hierarchy.
    /// </summary>
    /// <param name="obj">The object to get.</param>
    /// <param name="result">The hierarchy object to retrieve.</param>
    /// <returns>True on successful retrieval, otherwise false.</returns>
    API_FUNCTION() virtual bool GetObject(ScriptingObject* obj, API_PARAM(Out) NetworkReplicationHierarchyObject& result);

    /// <summary>
    /// Sets object properties in the hierarchy. Can be used to modify replication settings at runtime.
    /// </summary>
    /// <param name="value">The object data to update.</param>
    /// <returns>True on successful update, otherwise false (eg, if specific object has not been added to this node).</returns>
    API_FUNCTION() virtual bool SetObject(const NetworkReplicationHierarchyObject& value);

    /// <summary>
    /// Force replicates the object during the next update. Resets any internal tracking state to force the synchronization.
    /// </summary>
    /// <param name="obj">The object to update.</param>
    /// <returns>True on successful update, otherwise false.</returns>
    API_FUNCTION() virtual bool DirtyObject(ScriptingObject* obj);

    /// <summary>
    /// Iterates over all objects and adds them to the replication work.
    /// </summary>
    /// <param name="result">The update results container.</param>
    API_FUNCTION() virtual void Update(NetworkReplicationHierarchyUpdateResult* result);
};

inline uint32 GetHash(const Int3& key)
{
    uint32 hash = GetHash(key.X);
    CombineHash(hash, GetHash(key.Y));
    CombineHash(hash, GetHash(key.Z));
    return hash;
}

/// <summary>
/// Network replication hierarchy node with 3D grid spatialization. Organizes static objects into chunks to improve performance in large worlds.
/// </summary>
API_CLASS(Namespace = "FlaxEngine.Networking") class FLAXENGINE_API NetworkReplicationGridNode : public NetworkReplicationNode
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(NetworkReplicationGridNode, NetworkReplicationNode);
    ~NetworkReplicationGridNode();

private:
    struct Cell
    {
        NetworkReplicationNode* Node;
        float MinCullDistance;
    };

    Dictionary<Int3, Cell> _children;
    Dictionary<ScriptingObject*, Int3> _objectToCell;

public:
    /// <summary>
    /// Size of the grid cell (in world units). Used to chunk the space for separate nodes.
    /// </summary>
    API_FIELD() float CellSize = 10000.0f;

    void AddObject(NetworkReplicationHierarchyObject obj) override;
    bool RemoveObject(ScriptingObject* obj) override;
    bool GetObject(ScriptingObject* obj, NetworkReplicationHierarchyObject& result) override;
    bool SetObject(const NetworkReplicationHierarchyObject& value) override;
    bool DirtyObject(ScriptingObject* obj) override;
    void Update(NetworkReplicationHierarchyUpdateResult* result) override;
};

/// <summary>
/// Defines the network objects replication hierarchy (tree structure) that controls chunking and configuration of the game objects replication.
/// Contains only 'owned' objects. It's used by the networking system only on a main thread.
/// </summary>
API_CLASS(Namespace = "FlaxEngine.Networking") class FLAXENGINE_API NetworkReplicationHierarchy : public NetworkReplicationNode
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(NetworkReplicationHierarchy, NetworkReplicationNode);
};
