// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "Tags.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"

Array<String> Tags::List;
#if !BUILD_RELEASE
FLAXENGINE_API String* TagsListDebug = nullptr;
#endif

const String& Tag::ToString() const
{
    return Index >= 0 && Index < Tags::List.Count() ? Tags::List.Get()[Index] : String::Empty;
}

bool Tag::operator==(const StringView& other) const
{
    return ToString() == other;
}

bool Tag::operator!=(const StringView& other) const
{
    return ToString() != other;
}

Tag Tags::Get(const StringView& tagName)
{
    Tag tag = List.Find(tagName);
    if (tag.Index == -1 && tagName.HasChars())
    {
        tag.Index = List.Count();
        List.AddOne() = tagName;
#if !BUILD_RELEASE
        TagsListDebug = List.Get();
#endif
    }
    return tag;
}

bool Tags::HasTag(const Array<Tag>& list, const Tag& tag)
{
    if (tag.Index == -1)
        return false;
    const String& tagName = tag.ToString();
    for (const Tag& e : list)
    {
        const String& eName = e.ToString();
        if (e == tag || (eName.Length() > tagName.Length() + 1 && eName.StartsWith(tagName) && eName[tagName.Length()] == '.'))
            return true;
    }
    return false;
}

bool Tags::HasTagExact(const Array<Tag>& list, const Tag& tag)
{
    if (tag.Index == -1)
        return false;
    return list.Contains(tag);
}

bool Tags::HasAny(const Array<Tag>& list, const Array<Tag>& tags)
{
    for (const Tag& tag : tags)
    {
        if (HasTag(list, tag))
            return true;
    }
    return false;
}

bool Tags::HasAnyExact(const Array<Tag>& list, const Array<Tag>& tags)
{
    for (const Tag& tag : tags)
    {
        if (HasTagExact(list, tag))
            return true;
    }
    return false;
}

bool Tags::HasAll(const Array<Tag>& list, const Array<Tag>& tags)
{
    if (tags.IsEmpty())
        return true;
    for (const Tag& tag : tags)
    {
        if (!HasTag(list, tag))
            return false;
    }
    return true;
}

bool Tags::HasAllExact(const Array<Tag>& list, const Array<Tag>& tags)
{
    if (tags.IsEmpty())
        return true;
    for (const Tag& tag : tags)
    {
        if (!HasTagExact(list, tag))
            return false;
    }
    return true;
}

const String& Tags::GetTagName(int32 tag)
{
    return Tag(tag).ToString();
}
