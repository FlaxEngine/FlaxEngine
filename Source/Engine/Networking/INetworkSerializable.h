// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Compiler.h"
#include "Engine/Core/Config.h"

class NetworkStream;

/// <summary>
/// Interface for values and objects that can be serialized/deserialized for network replication.
/// </summary>
API_INTERFACE(Namespace="FlaxEngine.Networking") class FLAXENGINE_API INetworkSerializable
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(INetworkSerializable);
public:
    /// <summary>
    /// Serializes object to the output stream.
    /// </summary>
    /// <param name="stream">The output stream to write serialized data.</param>
    API_FUNCTION() virtual void Serialize(NetworkStream* stream) = 0;

    /// <summary>
    /// Deserializes object from the input stream.
    /// </summary>
    /// <param name="stream">The input stream to read serialized data.</param>
    API_FUNCTION() virtual void Deserialize(NetworkStream* stream) = 0;
};
