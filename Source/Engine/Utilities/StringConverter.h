// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/StringUtils.h"
#include "Engine/Core/Memory/Memory.h"

template<typename CharType, int InlinedSize = 128>
class StringAsBase
{
protected:

    const CharType* _static = nullptr;
    CharType* _dynamic = nullptr;
    CharType _inlined[InlinedSize];

public:

    ~StringAsBase()
    {
        Allocator::Free(_dynamic);
    }

public:

    const CharType* Get() const
    {
        return _static ? _static : (_dynamic ? _dynamic : _inlined);
    }

    int32 Length() const
    {
        return StringUtils::Length(Get());
    }
};

template<int InlinedSize = 128>
class StringAsANSI : public StringAsBase<char, InlinedSize>
{
public:

    typedef char CharType;
    typedef StringAsBase<CharType, InlinedSize> Base;

public:

    StringAsANSI(const char* text)
    {
        this->_static = text;
    }

    StringAsANSI(const Char* text)
    {
        const int32 length = StringUtils::Length(text);
        if (length + 1 < InlinedSize)
        {
            StringUtils::ConvertUTF162ANSI(text, this->_inlined, length);
            this->_inlined[length] = 0;
        }
        else
        {
            this->_dynamic = (CharType*)Allocator::Allocate((length + 1) * sizeof(CharType));
            StringUtils::ConvertUTF162ANSI(text, this->_dynamic, length);
            this->_dynamic[length] = 0;
        }
    }

    StringAsANSI(const Char* text, const int32 length)
    {
        if (length + 1 < InlinedSize)
        {
            StringUtils::ConvertUTF162ANSI(text, this->_inlined, length);
            this->_inlined[length] = 0;
        }
        else
        {
            this->_dynamic = (CharType*)Allocator::Allocate((length + 1) * sizeof(CharType));
            StringUtils::ConvertUTF162ANSI(text, this->_dynamic, length);
            this->_dynamic[length] = 0;
        }
    }
};

template<int InlinedSize = 128>
class StringAsUTF16 : public StringAsBase<Char>
{
public:

    typedef Char CharType;
    typedef StringAsBase<CharType, InlinedSize> Base;

public:

    StringAsUTF16(const char* text)
    {
        const int32 length = StringUtils::Length(text);
        if (length + 1 < InlinedSize)
        {
            StringUtils::ConvertANSI2UTF16(text, this->_inlined, length);
            this->_inlined[length] = 0;
        }
        else
        {
            this->_dynamic = (CharType*)Allocator::Allocate((length + 1) * sizeof(CharType));
            StringUtils::ConvertANSI2UTF16(text, this->_dynamic, length);
            this->_dynamic[length] = 0;
        }
    }

    StringAsUTF16(const char* text, const int32 length)
    {
        if (length + 1 < InlinedSize)
        {
            StringUtils::ConvertANSI2UTF16(text, this->_inlined, length);
            this->_inlined[length] = 0;
        }
        else
        {
            this->_dynamic = (CharType*)Allocator::Allocate((length + 1) * sizeof(CharType));
            StringUtils::ConvertANSI2UTF16(text, this->_dynamic, length);
            this->_dynamic[length] = 0;
        }
    }

    StringAsUTF16(const Char* text)
    {
        this->_static = text;
    }
};
