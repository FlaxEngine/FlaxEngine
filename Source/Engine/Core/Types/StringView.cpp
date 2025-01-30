// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "StringView.h"
#include "String.h"
#include "StringBuilder.h"

StringView StringBuilder::ToStringView() const
{
    return StringView(_data.Get(), _data.Count());
}

StringView StringView::Empty;

StringView::StringView(const String& str)
    : StringViewBase<Char>(str.Get(), str.Length())
{
}

bool StringView::operator==(const String& other) const
{
    return this->Compare(StringView(other)) == 0;
}

bool StringView::operator!=(const String& other) const
{
    return this->Compare(StringView(other)) != 0;
}

StringView StringView::Left(int32 count) const
{
    const int32 countClamped = count < 0 ? 0 : count < Length() ? count : Length();
    return StringView(**this, countClamped);
}

StringView StringView::Right(int32 count) const
{
    const int32 countClamped = count < 0 ? 0 : count < Length() ? count : Length();
    return StringView(**this + countClamped, Length() - countClamped);
}

StringView StringView::Substring(int32 startIndex) const
{
    ASSERT(startIndex >= 0 && startIndex < Length());
    return StringView(Get() + startIndex, Length() - startIndex);
}

StringView StringView::Substring(int32 startIndex, int32 count) const
{
    ASSERT(startIndex >= 0 && startIndex + count <= Length() && count >= 0);
    return StringView(Get() + startIndex, count);
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
    : StringViewBase<char>(str.Get(), str.Length())
{
}

bool StringAnsiView::operator==(const StringAnsi& other) const
{
    return this->Compare(StringAnsiView(other)) == 0;
}

bool StringAnsiView::operator!=(const StringAnsi& other) const
{
    return this->Compare(StringAnsiView(other)) != 0;
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
