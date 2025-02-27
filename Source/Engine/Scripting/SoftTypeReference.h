// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Scripting.h"
#include "ScriptingObject.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Serialization/SerializationFwd.h"

/// <summary>
/// The soft reference to the scripting type contained in the scripting assembly.
/// </summary>
template<typename T = ScriptingObject>
API_STRUCT(InBuild, MarshalAs=StringAnsi) struct SoftTypeReference
{
protected:
    StringAnsi _typeName;

public:
    SoftTypeReference() = default;

    SoftTypeReference(const SoftTypeReference& s)
        : _typeName(s._typeName)
    {
    }

    SoftTypeReference(SoftTypeReference&& s) noexcept
        : _typeName(MoveTemp(s._typeName))
    {
    }

    SoftTypeReference(const StringView& s)
        : _typeName(s)
    {
    }

    SoftTypeReference(const StringAnsiView& s)
        : _typeName(s)
    {
    }

    SoftTypeReference(const char* s)
        : _typeName(s)
    {
    }

public:
    FORCE_INLINE SoftTypeReference& operator=(SoftTypeReference&& s) noexcept
    {
        _typeName = MoveTemp(s._typeName);
        return *this;
    }

    FORCE_INLINE SoftTypeReference& operator=(StringAnsi&& s) noexcept
    {
        _typeName = MoveTemp(s);
        return *this;
    }

    FORCE_INLINE SoftTypeReference& operator=(const SoftTypeReference& s)
    {
        _typeName = s._typeName;
        return *this;
    }

    FORCE_INLINE SoftTypeReference& operator=(const StringAnsiView& s) noexcept
    {
        _typeName = s;
        return *this;
    }

    FORCE_INLINE bool operator==(const SoftTypeReference& other) const
    {
        return _typeName == other._typeName;
    }

    FORCE_INLINE bool operator!=(const SoftTypeReference& other) const
    {
        return _typeName != other._typeName;
    }

    FORCE_INLINE bool operator==(const StringAnsiView& other) const
    {
        return _typeName == other;
    }

    FORCE_INLINE bool operator!=(const StringAnsiView& other) const
    {
        return _typeName != other;
    }

    FORCE_INLINE operator bool() const
    {
        return _typeName.HasChars();
    }

    operator StringAnsi() const
    {
        return _typeName;
    }

    String ToString() const
    {
        return _typeName.ToString();
    }

public:
    // Gets the type full name (eg. FlaxEngine.Actor).
    StringAnsiView GetTypeName() const
    {
        return StringAnsiView(_typeName);
    }

    // Gets the type (resolves soft reference).
    ScriptingTypeHandle GetType() const
    {
        return Scripting::FindScriptingType(_typeName);
    }

    // Creates a new objects of that type (or of type T if failed to solve typename).
    T* NewObject() const
    {
        const ScriptingTypeHandle type = Scripting::FindScriptingType(_typeName);
        auto obj = ScriptingObject::NewObject<T>(type);
        if (!obj)
        {
            if (_typeName.HasChars())
                LOG(Error, "Unknown or invalid type {0}", String(_typeName));
            obj = ScriptingObject::NewObject<T>();
        }
        return obj;
    }
};

template<typename T>
uint32 GetHash(const SoftTypeReference<T>& key)
{
    return GetHash(key.GetTypeName());
}

// @formatter:off
namespace Serialization
{
    template<typename T>
    bool ShouldSerialize(const SoftTypeReference<T>& v, const void* otherObj)
    {
        return !otherObj || v != *(SoftTypeReference<T>*)otherObj;
    }
    template<typename T>
    void Serialize(ISerializable::SerializeStream& stream, const SoftTypeReference<T>& v, const void* otherObj)
    {
        stream.String(v.GetTypeName());
    }
    template<typename T>
    void Deserialize(ISerializable::DeserializeStream& stream, SoftTypeReference<T>& v, ISerializeModifier* modifier)
    {
        v = stream.GetTextAnsi();
    }
}
// @formatter:on
