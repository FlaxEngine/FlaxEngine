// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// Gameplay tag that represents a hierarchical name of the form 'X.Y.Z' (namespaces separated with a dot). Tags are defined in project LayersAndTagsSettings asset but can be also created from code.
/// </summary>
API_STRUCT(NoDefault) struct FLAXENGINE_API Tag
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(Tag);

    /// <summary>
    /// Index of the tag (in global Level.Tags list).
    /// </summary>
    API_FIELD() int32 Index = -1;

    /// <summary>
    /// Gets the tag name.
    /// </summary>
    const String& ToString() const;

public:
    Tag() = default;

    FORCE_INLINE Tag(int32 index)
        : Index(index)
    {
    }

    FORCE_INLINE operator bool() const
    {
        return Index != -1;
    }

    FORCE_INLINE bool operator==(const Tag& other) const
    {
        return Index == other.Index;
    }

    FORCE_INLINE bool operator!=(const Tag& other) const
    {
        return Index != other.Index;
    }

    bool operator==(const StringView& other) const;
    bool operator!=(const StringView& other) const;
};

template<>
struct TIsPODType<Tag>
{
    enum { Value = true };
};

inline uint32 GetHash(const Tag& key)
{
    return (uint32)key.Index;
}

/// <summary>
/// Gameplay tags utilities.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API Tags
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(Tags);

    /// <summary>
    /// List of all tags.
    /// </summary>
    API_FIELD(ReadOnly) static Array<String> List;

    /// <summary>
    /// Gets or adds the tag.
    /// </summary>
    /// <param name="tagName">The tag name.</param>
    /// <returns>The tag.</returns>
    API_FUNCTION() static Tag Get(const StringView& tagName);

private:
    API_FUNCTION(NoProxy) static const String& GetTagName(int32 tag);
};

#if !BUILD_RELEASE
extern FLAXENGINE_API String* TagsListDebug; // Used by flax.natvis
#endif
