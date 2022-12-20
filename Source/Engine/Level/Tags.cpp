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

const String& Tags::GetTagName(int32 tag)
{
    return Tag(tag).ToString();
}
