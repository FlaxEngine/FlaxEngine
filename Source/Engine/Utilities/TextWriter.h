// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/NonCopyable.h"
#include "Engine/Core/Formatting.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Serialization/MemoryWriteStream.h"

/// <summary>
/// Useful tool to write many text data
/// </summary>
template<typename CharType>
class TextWriter : public Object, public NonCopyable
{
private:

    MemoryWriteStream _buffer;

public:

    /// <summary>
    /// Init with default capacity
    /// </summary>
    /// <param name="capacity">Initial capacity in bytes</param>
    TextWriter(uint32 capacity)
        : _buffer(capacity)
    {
    }

    /// <summary>
    /// Destructor
    /// </summary>
    ~TextWriter()
    {
    }

public:

    /// <summary>
    /// Gets writer private buffer
    /// </summary>
    /// <returns>Buffer</returns>
    FORCE_INLINE MemoryWriteStream* GetBuffer()
    {
        return &_buffer;
    }

    /// <summary>
    /// Gets writer private buffer
    /// </summary>
    /// <returns>Buffer</returns>
    const FORCE_INLINE MemoryWriteStream* GetBuffer() const
    {
        return &_buffer;
    }

public:

    /// <summary>
    /// Write line terminator sign
    /// </summary>
    template<class Q = CharType>
    typename std::enable_if<std::is_same<Q, char>::value, void>::type WriteLine()
    {
        _buffer.WriteChar('\n');
    }

    /// <summary>
    /// Write line terminator sign
    /// </summary>
    template<class Q = CharType>
    typename std::enable_if<std::is_same<Q, Char>::value, void>::type WriteLine()
    {
        _buffer.WriteUint16(TEXT('\n'));
    }

    /// <summary>
    /// Write single line of text to the buffer
    /// </summary>
    /// <param name="text">Data</param>
    void WriteLine(const CharType* text)
    {
        _buffer.WriteBytes((void*)text, StringUtils::Length(text) * sizeof(CharType));
        WriteLine();
    }

    /// <summary>
    /// Format text and write line to the buffer
    /// </summary>
    template<typename... Args>
    void WriteLine(const CharType* format, const Args& ... args)
    {
        fmt::basic_memory_buffer<CharType> w;
        format_to(w, format, args...);
        const int32 len = (int32)w.size();
        _buffer.WriteBytes((void*)w.data(), len * sizeof(CharType));
        WriteLine();
    }

    /// <summary>
    /// Write text to the buffer
    /// </summary>
    /// <param name="text">Data</param>
    void Write(const StringViewBase<CharType>& text)
    {
        _buffer.WriteBytes((void*)text.Get(), text.Length() * sizeof(CharType));
    }

    /// <summary>
    /// Write text to the buffer
    /// </summary>
    /// <param name="text">Data</param>
    void Write(const StringBase<CharType>& text)
    {
        _buffer.WriteBytes((void*)text.Get(), text.Length() * sizeof(CharType));
    }

    /// <summary>
    /// Write text to the buffer
    /// </summary>
    /// <param name="text">Data</param>
    void Write(const CharType* text)
    {
        _buffer.WriteBytes((void*)text, StringUtils::Length(text) * sizeof(CharType));
    }

    /// <summary>
    /// Format text and write it
    /// </summary>
    template<typename... Args>
    void Write(const CharType* format, const Args& ... args)
    {
        fmt::basic_memory_buffer<CharType> w;
        format_to(w, format, args...);
        const int32 len = (int32)w.size();
        _buffer.WriteBytes((void*)w.data(), len * sizeof(CharType));
    }

public:

    /// <summary>
    /// Clear whole data
    /// </summary>
    void Clear()
    {
        _buffer.SetPosition(0);
    }

public:

    // [Object]
    String ToString() const override
    {
        return String((CharType*)_buffer.GetHandle(), _buffer.GetPosition() / sizeof(CharType));
    }
};

typedef TextWriter<char> TextWriterANSI;
typedef TextWriter<Char> TextWriterUnicode;
