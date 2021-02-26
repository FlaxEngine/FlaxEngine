// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "StringView.h"
#include "String.h"
#include "StringBuilder.h"

StringView StringBuilder::ToStringView() const
{
    return StringView(_data.Get(), _data.Count());
}

StringView StringView::Empty;

StringView::StringView(const String& str)
{
    _data = str.Get();
    _length = str.Length();
}

bool StringView::operator==(const String& other) const
{
    return StringUtils::Compare(this->GetText(), *other) == 0;
}

bool StringView::operator!=(const String& other) const
{
    return StringUtils::Compare(this->GetText(), *other) != 0;
}

String StringView::Substring(int32 startIndex) const
{
    ASSERT(startIndex >= 0 && startIndex < Length());
    return String(Get() + startIndex, Length() - startIndex);
}

String StringView::Substring(int32 startIndex, int32 count) const
{
    ASSERT(startIndex >= 0 && startIndex + count <= Length() && count >= 0);
    return String(Get() + startIndex, count);
}

String StringView::ToString() const
{
    return String(_data, _length);
}

StringAnsi StringView::ToStringAnsi() const
{
    return StringAnsi(_data, _length);
}

bool operator==(const String& a, const StringView& b)
{
    return a.Length() == b.Length() && StringUtils::Compare(a.GetText(), b.GetText(), b.Length()) == 0;
}

bool operator!=(const String& a, const StringView& b)
{
    return a.Length() != b.Length() || StringUtils::Compare(a.GetText(), b.GetText(), b.Length()) != 0;
}

StringAnsiView StringAnsiView::Empty;

StringAnsiView::StringAnsiView(const StringAnsi& str)
{
    _data = str.Get();
    _length = str.Length();
}

bool StringAnsiView::operator==(const StringAnsi& other) const
{
    return StringUtils::Compare(this->GetText(), *other) == 0;
}

bool StringAnsiView::operator!=(const StringAnsi& other) const
{
    return StringUtils::Compare(this->GetText(), *other) != 0;
}

StringAnsi StringAnsiView::Substring(int32 startIndex) const
{
    ASSERT(startIndex >= 0 && startIndex < Length());
    return StringAnsi(Get() + startIndex, Length() - startIndex);
}

StringAnsi StringAnsiView::Substring(int32 startIndex, int32 count) const
{
    ASSERT(startIndex >= 0 && startIndex + count <= Length() && count >= 0);
    return StringAnsi(Get() + startIndex, count);
}

String StringAnsiView::ToString() const
{
    return String(_data, _length);
}

StringAnsi StringAnsiView::ToStringAnsi() const
{
    return StringAnsi(*this);
}

bool operator==(const StringAnsi& a, const StringAnsiView& b)
{
    return a.Length() == b.Length() && StringUtils::Compare(a.GetText(), b.GetText(), b.Length()) == 0;
}

bool operator!=(const StringAnsi& a, const StringAnsiView& b)
{
    return a.Length() != b.Length() || StringUtils::Compare(a.GetText(), b.GetText(), b.Length()) != 0;
}
