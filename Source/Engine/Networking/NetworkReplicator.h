// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Scripting/ScriptingType.h"

/// <summary>
/// The high-level network object role and authority. Used to define who owns the object and when it can be simulated or just replicated.
/// </summary>
API_ENUM(Namespace="FlaxEngine.Networking") enum class NetworkObjectRole : byte
{
    // Not replicated object.
    None = 0,
    // Server/client owns the object and replicates it to others. Only owning client can simulate object and provides current state.
    OwnedAuthoritative,
    // Server/client gets replicated object from other server/client who owns it. Object cannot be simulated locally (any changes will be overriden by replication).
    Replicated,
    // Client gets replicated object from server but still can locally autonomously simulate it too. For example, client can control local pawn with real human input but will validate with server proper state (eg. to prevent cheats).
    ReplicatedAutonomous,
};

/// <summary>
/// High-level networking replication system for game objects.
/// </summary>
API_CLASS(static, Namespace = "FlaxEngine.Networking") class FLAXENGINE_API NetworkReplicator
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkReplicator);
    friend class NetworkReplicatorInternal;
    typedef void (*SerializeFunc)(void* instance, NetworkStream* stream, void* tag);

public:
    /// <summary>
    /// Adds the network replication serializer for a given type.
    /// </summary>
    /// <param name="typeHandle">The scripting type to serialize.</param>
    /// <param name="serialize">Serialization callback method.</param>
    /// <param name="deserialize">Deserialization callback method.</param>
    /// <param name="serializeTag">Serialization callback method tag value.</param>
    /// <param name="deserializeTag">Deserialization callback method tag value.</param>
    static void AddSerializer(const ScriptingTypeHandle& typeHandle, SerializeFunc serialize, SerializeFunc deserialize, void* serializeTag = nullptr, void* deserializeTag = nullptr);

    /// <summary>
    /// Invokes the network replication serializer for a given type.
    /// </summary>
    /// <param name="typeHandle">The scripting type to serialize.</param>
    /// <param name="instance">The value instance to serialize.</param>
    /// <param name="stream">The input/output stream to use for serialization.</param>
    /// <param name="serialize">True if serialize, otherwise deserialize mode.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION(NoProxy) static bool InvokeSerializer(const ScriptingTypeHandle& typeHandle, void* instance, NetworkStream* stream, bool serialize);

    /// <summary>
    /// Adds the object to the network replication system.
    /// </summary>
    /// <remarks>Does nothing if network is offline.</remarks>
    /// <param name="obj">The object to replicate.</param>
    /// <param name="parent">The parent of the object (eg. player that spawned it).</param>
    API_FUNCTION() static void AddObject(ScriptingObject* obj, ScriptingObject* parent = nullptr);

    /// <summary>
    /// Spawns the object to the other clients. Can be spawned by the owner who locally created it (eg. from prefab).
    /// </summary>
    /// <remarks>Does nothing if network is offline.</remarks>
    /// <param name="obj">The object to spawn on other clients.</param>
    API_FUNCTION() static void SpawnObject(ScriptingObject* obj);

    /// <summary>
    /// Despawns the object from the other clients. Deletes object from remove clients.
    /// </summary>
    /// <remarks>Does nothing if network is offline.</remarks>
    /// <param name="obj">The object to despawn on other clients.</param>
    API_FUNCTION() static void DespawnObject(ScriptingObject* obj);

    /// <summary>
    /// Gets the Client Id of the network object owner.
    /// </summary>
    /// <param name="obj">The network object.</param>
    /// <returns>The Client Id.</returns>
    API_FUNCTION() static uint32 GetObjectClientId(ScriptingObject* obj);

    /// <summary>
    /// Gets the role of the network object used locally (eg. to check if can simulate object).
    /// </summary>
    /// <param name="obj">The network object.</param>
    /// <returns>The object role.</returns>
    API_FUNCTION() static NetworkObjectRole GetObjectRole(ScriptingObject* obj);

    /// <summary>
    /// Sets the network object ownership - owning client identifier and local role to use.
    /// </summary>
    /// <param name="obj">The network object.</param>
    /// <param name="ownerClientId">The new owner. Set to NetworkManager::LocalClientId for local client to be owner (server might reject it).</param>
    /// <param name="localRole">The local role to assign for the object.</param>
    API_FUNCTION() static void SetObjectOwnership(ScriptingObject* obj, uint32 ownerClientId, NetworkObjectRole localRole = NetworkObjectRole::Replicated);

private:
#if !COMPILE_WITHOUT_CSHARP
    API_FUNCTION(NoProxy) static void AddSerializer(const ScriptingTypeHandle& type, const Function<void(void*, void*)>& serialize, const Function<void(void*, void*)>& deserialize);
#endif
};
