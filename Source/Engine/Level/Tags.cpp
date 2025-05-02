// Copyright (c) Wojciech Figat. All rights reserved.

#include "Tags.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Serialization/SerializationFwd.h"

Array<String> Tags::List;
#if !BUILD_RELEASE
FLAXENGINE_API String* TagsListDebug = nullptr;
#endif

const String& Tag::ToString() const
{
    const int32 index = (int32)Index - 1;
    return index >= 0 && index < Tags::List.Count() ? Tags::List.Get()[index] : String::Empty;
}

bool Tag::operator==(const StringView& other) const
{
    return ToString() == other;
}

bool Tag::operator!=(const StringView& other) const
{
    return ToString() != other;
}

void FLAXENGINE_API Serialization::Serialize(ISerializable::SerializeStream& stream, const Tag& v, const void* otherObj)
{
    if (v.Index != 0)
        stream.String(v.ToString());
    else
        stream.String("", 0);
}

void FLAXENGINE_API Serialization::Deserialize(ISerializable::DeserializeStream& stream, Tag& v, ISerializeModifier* modifier)
{
    v = Tags::Get(stream.GetText());
}

Tag Tags::Get(const StringView& tagName)
{
    if (tagName.IsEmpty())
        return Tag();
    Tag tag(List.Find(tagName) + 1);
    if (tag.Index == 0 && tagName.HasChars())
    {
        List.AddOne() = tagName;
        tag.Index = List.Count();
#if !BUILD_RELEASE
        TagsListDebug = List.Get();
#endif
    }
    return tag;
}

Tag Tags::Find(const StringView& tagName)
{
    return Tag(List.Find(tagName) + 1);
}

Array<Tag> Tags::GetSubTags(Tag parentTag)
{
    Array<Tag> subTags;
    const String& parentTagName = parentTag.ToString();
    for (int32 i = 0; i < List.Count(); i++)
    {
        const Tag tag(i + 1);
        const String& tagName = List[i];
        if (tagName.StartsWith(parentTagName) && parentTag.Index != tag.Index)
            subTags.Add(tag);
    }
    return subTags;
}

bool Tags::HasTag(const Array<Tag>& list, const Tag tag)
{
    if (tag.Index == 0)
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

bool Tags::HasTagExact(const Array<Tag>& list, const Tag tag)
{
    if (tag.Index == 0)
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

const String& Tags::GetTagName(uint32 tag)
{
    return Tag(tag).ToString();
}
