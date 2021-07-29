// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Serialization/ISerializable.h"
#include "ManagedCLR/MTypes.h"

/// <summary>
/// Managed objects serialization utilities. Helps with C# scripts saving to JSON or loading.
/// </summary>
class FLAXENGINE_API ManagedSerialization
{
public:

    /// <summary>
    /// Serializes managed object to JSON.
    /// </summary>
    /// <param name="stream">The output stream.</param>
    /// <param name="object">The object to serialize.</param>
    static void Serialize(ISerializable::SerializeStream& stream, MonoObject* object);

    /// <summary>
    /// Serializes managed object difference to JSON.
    /// </summary>
    /// <param name="stream">The output stream.</param>
    /// <param name="object">The object to serialize.</param>
    /// <param name="other">The reference object to serialize diff compared to it.</param>
    static void SerializeDiff(ISerializable::SerializeStream& stream, MonoObject* object, MonoObject* other);

    /// <summary>
    /// Deserializes managed object from the JSON.
    /// </summary>
    /// <param name="stream">The input stream.</param>
    /// <param name="object">The object to deserialize.</param>
    static void Deserialize(ISerializable::DeserializeStream& stream, MonoObject* object);

    /// <summary>
    /// Deserializes managed object from the JSON.
    /// </summary>
    /// <param name="data">The input data.</param>
    /// <param name="object">The object to deserialize.</param>
    static void Deserialize(const StringAnsiView& data, MonoObject* object);
};
