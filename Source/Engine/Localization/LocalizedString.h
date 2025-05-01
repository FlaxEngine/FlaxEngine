// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Serialization/SerializationFwd.h"

/// <summary>
/// The string container that supports using localized text.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API LocalizedString
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(LocalizedString);
public:
    /// <summary>
    /// The localized string identifier. Used to lookup text value for a current language (via <see cref="Localization::GetString"/>).
    /// </summary>
    API_FIELD() String Id;

    /// <summary>
    /// The overriden string value to use. If empty, the localized string will be used.
    /// </summary>
    API_FIELD() String Value;

public:
    LocalizedString() = default;
    LocalizedString(const LocalizedString& other);
    LocalizedString(LocalizedString&& other) noexcept;
    LocalizedString(const StringView& value);
    LocalizedString(String&& value) noexcept;

    LocalizedString& operator=(const LocalizedString& other);
    LocalizedString& operator=(LocalizedString&& other) noexcept;
    LocalizedString& operator=(const StringView& value);
    LocalizedString& operator=(String&& value) noexcept;

    bool operator==(const LocalizedString& other) const
    {
        return Id == other.Id && Value == other.Value;
    }

    friend bool operator!=(const LocalizedString& a, const LocalizedString& b)
    {
        return !(a == b);
    }

    friend bool operator==(const LocalizedString& a, const StringView& b)
    {
        return a.Value == b || a.ToString() == b;
    }

    friend bool operator!=(const LocalizedString& a, const StringView& b)
    {
        return !(a == b);
    }

public:
    String ToString() const;
    String ToStringPlural(int32 n) const;
};

inline uint32 GetHash(const LocalizedString& key)
{
    return GetHash(key.ToString());
}

namespace Serialization
{
    inline bool ShouldSerialize(const LocalizedString& v, const void* otherObj)
    {
        return !otherObj || v != *(LocalizedString*)otherObj;
    }

    inline void Serialize(ISerializable::SerializeStream& stream, const LocalizedString& v, const void* otherObj)
    {
        if (v.Id.IsEmpty())
        {
            stream.String(v.Value);
        }
        else
        {
            stream.StartObject();
            stream.JKEY("Id");
            stream.String(v.Id);
            if (v.Value.HasChars())
            {
                stream.JKEY("Value");
                stream.String(v.Value);
            }
            stream.EndObject();
        }
    }

    inline void Deserialize(ISerializable::DeserializeStream& stream, LocalizedString& v, ISerializeModifier* modifier)
    {
        if (stream.IsString())
        {
            v.Id = String::Empty;
            v.Value = stream.GetText();
        }
        else if (stream.IsObject())
        {
            auto e = SERIALIZE_FIND_MEMBER(stream, "Id");
            if (e != stream.MemberEnd())
                v.Id.SetUTF8(e->value.GetString(), e->value.GetStringLength());
            e = SERIALIZE_FIND_MEMBER(stream, "Value");
            if (e != stream.MemberEnd())
                v.Value.SetUTF8(e->value.GetString(), e->value.GetStringLength());
            else if (v.Id.HasChars())
                v.Value.Clear();
        }
        else
        {
            v = LocalizedString();
        }
    }
}

DEFINE_DEFAULT_FORMATTING_VIA_TO_STRING(LocalizedString);
