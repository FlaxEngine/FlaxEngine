// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/ISerializable.h"
#include "Engine/Scripting/ManagedCLR/MTypes.h"

/// <summary>
/// Managed objects serialization utilities. Helps with C# scripts saving to JSON or loading.
/// </summary>
class FLAXENGINE_API ManagedSerialization
{
public:
#if USE_CSHARP
    /// <summary>
    /// Serializes managed object to JSON.
    /// </summary>
    /// <param name="stream">The output stream.</param>
    /// <param name="object">The object to serialize.</param>
    static void Serialize(ISerializable::SerializeStream& stream, MObject* object);

    /// <summary>
    /// Serializes managed object difference to JSON.
    /// </summary>
    /// <param name="stream">The output stream.</param>
    /// <param name="object">The object to serialize.</param>
    /// <param name="other">The reference object to serialize diff compared to it.</param>
    static void SerializeDiff(ISerializable::SerializeStream& stream, MObject* object, MObject* other);

    /// <summary>
    /// Deserializes managed object from the JSON.
    /// </summary>
    /// <param name="stream">The input stream.</param>
    /// <param name="object">The object to deserialize.</param>
    static void Deserialize(ISerializable::DeserializeStream& stream, MObject* object);

    /// <summary>
    /// Deserializes managed object from the JSON.
    /// </summary>
    /// <param name="data">The input data.</param>
    /// <param name="object">The object to deserialize.</param>
    static void Deserialize(const StringAnsiView& data, MObject* object);
#endif
};
