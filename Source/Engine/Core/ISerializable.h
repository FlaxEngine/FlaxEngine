// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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

public:

    /// <summary>
    /// Finalizes an instance of the <see cref="ISerializable"/> class.
    /// </summary>
    virtual ~ISerializable() = default;

    /// <summary>
    /// Serialize object to the output stream compared to the values of the other object instance (eg. default class object). If other object is null then serialize all properties.
    /// </summary>
    /// <param name="stream">The output stream.</param>
    /// <param name="otherObj">The instance of the object to compare with and serialize only the modified properties. If null, then serialize all properties.</param>
    virtual void Serialize(SerializeStream& stream, const void* otherObj) = 0;

    /// <summary>
    /// Deserialize object from the input stream
    /// </summary>
    /// <param name="stream">The input stream.</param>
    /// <param name="modifier">The deserialization modifier object. Always valid.</param>
    virtual void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) = 0;

    /// <summary>
    /// Deserialize object from the input stream child member. Won't deserialize it if member is missing.
    /// </summary>
    /// <param name="stream">The input stream.</param>
    /// <param name="memberName">The input stream member to lookup.</param>
    /// <param name="modifier">The deserialization modifier object. Always valid.</param>
    void DeserializeIfExists(DeserializeStream& stream, const char* memberName, ISerializeModifier* modifier);
};
