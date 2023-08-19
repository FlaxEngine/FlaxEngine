// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/Variant.h"
#include "Engine/Serialization/SerializationFwd.h"

class BehaviorKnowledge;

/// <summary>
/// Behavior knowledge value selector that can reference blackboard item, behavior goal or sensor values.
/// </summary>
API_STRUCT(NoDefault, MarshalAs=StringAnsi) struct FLAXENGINE_API BehaviorKnowledgeSelectorAny
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(BehaviorKnowledgeSelectorAny);

    /// <summary>
    /// Selector path that redirects to the specific knowledge value.
    /// </summary>
    API_FIELD() StringAnsi Path;

    // Sets the selected knowledge value (as Variant).
    bool Set(BehaviorKnowledge* knowledge, const Variant& value);

    // Gets the selected knowledge value (as Variant).
    Variant Get(BehaviorKnowledge* knowledge);

    // Tries to get the selected knowledge value (as Variant). Returns true if got value, otherwise false.
    bool TryGet(BehaviorKnowledge* knowledge, Variant& value);

    FORCE_INLINE bool operator==(const BehaviorKnowledgeSelectorAny& other) const
    {
        return Path == other.Path;
    }

    BehaviorKnowledgeSelectorAny& operator=(const StringAnsiView& other) noexcept
    {
        Path = other;
        return *this;
    }

    BehaviorKnowledgeSelectorAny& operator=(StringAnsi&& other) noexcept
    {
        Path = MoveTemp(other);
        return *this;
    }

    operator StringAnsi() const
    {
        return Path;
    }
};

/// <summary>
/// Behavior knowledge value selector that can reference blackboard item, behavior goal or sensor values.
/// </summary>
template<typename T>
API_STRUCT(InBuild, Template, MarshalAs=StringAnsi) struct FLAXENGINE_API BehaviorKnowledgeSelector : BehaviorKnowledgeSelectorAny
{
    using BehaviorKnowledgeSelectorAny::Set;
    using BehaviorKnowledgeSelectorAny::Get;
    using BehaviorKnowledgeSelectorAny::TryGet;

    // Sets the selected knowledge value (typed).
    FORCE_INLINE void Set(BehaviorKnowledge* knowledge, const T& value)
    {
        BehaviorKnowledgeSelectorAny::Set(knowledge, Variant(value));
    }

    // Gets the selected knowledge value (typed).
    FORCE_INLINE T Get(BehaviorKnowledge* knowledge)
    {
        return (T)BehaviorKnowledgeSelectorAny::Get(knowledge);
    }

    // Tries to get the selected knowledge value (typed). Returns true if got value, otherwise false.
    FORCE_INLINE bool TryGet(BehaviorKnowledge* knowledge, T& value)
    {
        Variant variant;
        if (BehaviorKnowledgeSelectorAny::TryGet(knowledge, variant))
        {
            value = (T)variant;
            return true;
        }
        return false;
    }

    BehaviorKnowledgeSelector& operator=(const StringAnsiView& other) noexcept
    {
        Path = other;
        return *this;
    }

    BehaviorKnowledgeSelector& operator=(StringAnsi&& other) noexcept
    {
        Path = MoveTemp(other);
        return *this;
    }

    operator StringAnsi() const
    {
        return Path;
    }
};

inline uint32 GetHash(const BehaviorKnowledgeSelectorAny& key)
{
    return GetHash(key.Path);
}

// @formatter:off
namespace Serialization
{
    inline bool ShouldSerialize(const BehaviorKnowledgeSelectorAny& v, const void* otherObj)
    {
        return !otherObj || v.Path != ((BehaviorKnowledgeSelectorAny*)otherObj)->Path;
    }
    inline void Serialize(ISerializable::SerializeStream& stream, const BehaviorKnowledgeSelectorAny& v, const void* otherObj)
    {
        stream.String(v.Path);
    }
    inline void Deserialize(ISerializable::DeserializeStream& stream, BehaviorKnowledgeSelectorAny& v, ISerializeModifier* modifier)
    {
        v.Path = stream.GetTextAnsi();
    }
}
// @formatter:on
