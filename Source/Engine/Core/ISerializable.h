// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Serialization/JsonFwd.h"
#include "Engine/Core/Compiler.h"
#include "Engine/Core/Config.h"

class JsonWriter;
class ISerializeModifier;

/// <summary>
/// Interface for objects that can be serialized/deserialized to/from JSON format.
/// </summary>
API_INTERFACE() class FLAXENGINE_API ISerializable
{
DECLARE_SCRIPTING_TYPE_MINIMAL(ISerializable);
public:

    typedef rapidjson_flax::Document SerializeDocument;

    /// <summary>
    /// Serialization output stream
    /// </summary>
    typedef rapidjson_flax::Value DeserializeStream;

    /// <summary>
    /// Serialization input stream
    /// </summary>
    typedef JsonWriter SerializeStream;

    /// <summary>
    /// Serialization callback context container. Used by OnSerializing, OnSerialized, OnDeserializing, OnDeserialized methods.
    /// </summary>
    struct FLAXENGINE_API CallbackContext
    {
        // The deserialization modifier object.
        ISerializeModifier* Modifier = nullptr;
    };

public:

    /// <summary>
    /// Finalizes an instance of the <see cref="ISerializable"/> class.
    /// </summary>
    virtual ~ISerializable() = default;

    /// <summary>
    /// Compares with other instance to decide whether serialize this instance (eg. any field orp property is modified). Used to skip object serialization if not needed.
    /// </summary>
    /// <param name="otherObj">The instance of the object (always valid) to compare with to decide whether serialize this instance.</param>
    /// <returns>True if any field or property is modified compared to the other object instance, otherwise false.</returns>
    virtual bool ShouldSerialize(const void* otherObj) const { return true; }

    /// <summary>
    /// Serializes object to the output stream compared to the values of the other object instance (eg. default class object). If other object is null then serialize all properties.
    /// </summary>
    /// <param name="stream">The output stream.</param>
    /// <param name="otherObj">The instance of the object to compare with and serialize only the modified properties. If null, then serialize all properties.</param>
    virtual void Serialize(SerializeStream& stream, const void* otherObj) = 0;

    /// <summary>
    /// Deserializes object from the input stream.
    /// </summary>
    /// <param name="stream">The input stream.</param>
    /// <param name="modifier">The deserialization modifier object. Always valid.</param>
    virtual void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) = 0;

    /// <summary>
    /// Deserializes object from the input stream child member. Won't deserialize it if member is missing.
    /// </summary>
    /// <param name="stream">The input stream.</param>
    /// <param name="memberName">The input stream member to lookup.</param>
    /// <param name="modifier">The deserialization modifier object. Always valid.</param>
    void DeserializeIfExists(DeserializeStream& stream, const char* memberName, ISerializeModifier* modifier);
};
