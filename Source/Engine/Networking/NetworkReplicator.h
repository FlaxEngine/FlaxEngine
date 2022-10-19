// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Scripting/ScriptingType.h"

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
    /// <param name="owner">The owner of the object (eg. player that spawned it).</param>
    API_FUNCTION() static void AddObject(ScriptingObject* obj, ScriptingObject* owner);

private:
#if !COMPILE_WITHOUT_CSHARP
    API_FUNCTION(NoProxy) static void AddSerializer(const ScriptingTypeHandle& type, const Function<void(void*, void*)>& serialize, const Function<void(void*, void*)>& deserialize);
#endif
};
