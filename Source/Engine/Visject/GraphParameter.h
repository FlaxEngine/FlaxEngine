// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/Variant.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "VisjectMeta.h"

template<typename T>
class DataContainer;
typedef DataContainer<byte> BytesContainer;

/// <summary>
/// The channel mask modes.
/// </summary>
API_ENUM() enum class ChannelMask
{
    /// <summary>
    /// The red channel.
    /// </summary>
    Red = 0,

    /// <summary>
    /// The green channel.
    /// </summary>
    Green = 1,

    /// <summary>
    /// The blue channel.
    /// </summary>
    Blue = 2,

    /// <summary>
    /// The alpha channel.
    /// </summary>
    Alpha = 3,
};

/// <summary>
/// Represents a parameter in the Graph.
/// </summary>
API_CLASS() class FLAXENGINE_API GraphParameter : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(GraphParameter, ScriptingObject);
public:
    /// <summary>
    /// Parameter type
    /// </summary>
    API_FIELD(ReadOnly) VariantType Type;

    /// <summary>
    /// Parameter unique ID
    /// </summary>
    API_FIELD(ReadOnly) Guid Identifier;

    /// <summary>
    /// Parameter name
    /// </summary>
    API_FIELD(ReadOnly) String Name;

    /// <summary>
    /// Parameter value
    /// </summary>
    API_FIELD() Variant Value;

    /// <summary>
    /// True if is exposed outside
    /// </summary>
    API_FIELD() bool IsPublic = true;

    /// <summary>
    /// Additional metadata
    /// </summary>
    VisjectMeta Meta;

public:
    /// <summary>
    /// Gets the typename of the parameter type (excluding in-build types).
    /// </summary>
    /// <returns>The typename of the parameter type.</returns>
    API_PROPERTY() StringAnsiView GetTypeTypeName() const
    {
        return StringAnsiView(Type.TypeName);
    }

    /// <summary>
    /// Gets the data of the Visject Meta entry assigned to this parameter.
    /// </summary>
    /// <param name="typeID">Entry type ID</param>
    /// <returns>The entry data or empty if missing or not loaded.</returns>
    API_FUNCTION() BytesContainer GetMetaData(int32 typeID) const;
};
