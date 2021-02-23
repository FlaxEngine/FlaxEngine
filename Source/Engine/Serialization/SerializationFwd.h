// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/ISerializable.h"
#include "ISerializeModifier.h"
#include "Json.h"
#include "JsonWriter.h"

// The floating-point values serialization epsilon for equality checks precision
#define SERIALIZE_EPSILON 0.0000001f

// Helper macro to cast object on diff serialization
#define SERIALIZE_GET_OTHER_OBJ(type) const auto other = static_cast<const type*>(otherObj)

// Helper macro to use find member in the stream by name (skips strlen call if string is constant)
#define SERIALIZE_FIND_MEMBER(stream, name) stream.FindMember(rapidjson_flax::Value(name, ARRAY_COUNT(name) - 1))

// Serialization helper macro
#define SERIALIZE(name) \
    if (Serialization::ShouldSerialize(name, other ? &other->name : nullptr)) \
	{ \
		stream.JKEY(#name); \
		Serialization::Serialize(stream, name, other ? &other->name : nullptr); \
	}

// Serialization helper macro (for private members, or with custom member name)
#define SERIALIZE_MEMBER(name, member) \
    if (Serialization::ShouldSerialize(member, other ? &other->member : nullptr)) \
	{ \
		stream.JKEY(#name); \
		Serialization::Serialize(stream, member, other ? &other->member : nullptr); \
	}

// Deserialization helper macro
#define DESERIALIZE(name)  \
    { \
        const auto e = SERIALIZE_FIND_MEMBER(stream, #name); \
        if (e != stream.MemberEnd()) \
            Serialization::Deserialize(e->value, name, modifier); \
    }

// Deserialization helper macro (for private members, or with custom member name)
#define DESERIALIZE_MEMBER(name, member) \
    { \
        const auto e = SERIALIZE_FIND_MEMBER(stream, #name); \
        if (e != stream.MemberEnd()) \
            Serialization::Deserialize(e->value, member, modifier); \
    }

// Helper macros for bit fields

#define SERIALIZE_BIT(name) \
    if (!other || name != other->name) \
	{ \
		stream.JKEY(#name); \
		stream.Bool(name != 0); \
	}
#define SERIALIZE_BIT_MEMBER(name, member) \
    if (!other || member != other->member) \
	{ \
		stream.JKEY(#name); \
		stream.Bool(member != 0); \
	}
#define DESERIALIZE_BIT(name) \
    { \
        const auto e = SERIALIZE_FIND_MEMBER(stream, #name); \
        if (e != stream.MemberEnd() && e->value.IsBool()) \
            name = e->value.GetBool() ? 1 : 0; \
    }
#define DESERIALIZE_BIT_MEMBER(name, member) \
    { \
        const auto e = SERIALIZE_FIND_MEMBER(stream, #name); \
        if (e != stream.MemberEnd() && e->value.IsBool()) \
            member = e->value.GetBool() ? 1 : 0; \
    }
