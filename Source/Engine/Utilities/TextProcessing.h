// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/NonCopyable.h"
#include "Engine/Platform/StringUtils.h"

/// <summary>
/// Helper class to fast ANSI text processing (tokenization, reading, streaming etc.).
/// </summary>
class FLAXENGINE_API TextProcessing : public NonCopyable
{
public:
    /// <summary>
    /// Separator structure.
    /// </summary>
    struct SeparatorData
    {
    public:
        char C0;
        char C1;

    public:
        SeparatorData()
            : C0(0)
            , C1(0)
        {
        }

        SeparatorData(char c0)
            : C0(c0)
            , C1(0)
        {
        }

        SeparatorData(char c0, char c1)
            : C0(c0)
            , C1(c1)
        {
        }

        SeparatorData(const SeparatorData& other)
            : C0(other.C0)
            , C1(other.C1)
        {
        }

    public:
        bool IsWhiteSpace() const
        {
            return *this == SeparatorData('\r', '\n')
                    || *this == SeparatorData('\n')
                    || *this == SeparatorData('\t')
                    || *this == SeparatorData(' ');
        }

    public:
        bool operator==(const SeparatorData& other) const
        {
            return C0 == other.C0 && C1 == other.C1;
        }

        bool operator!=(const SeparatorData& other) const
        {
            return C0 != other.C0 || C1 != other.C1;
        }
    };

    /// <summary>
    /// Token structure
    /// </summary>
    struct Token
    {
    public:
        const char* Start;
        int32 Length;
        SeparatorData Separator;

    public:
        Token()
            : Start(nullptr)
            , Length(0)
            , Separator()
        {
        }

        explicit Token(const char* text)
            : Start(text)
            , Length(StringUtils::Length(text))
            , Separator()
        {
        }

        explicit Token(const StringAnsi& text)
            : Start(text.Get())
            , Length(text.Length())
            , Separator()
        {
        }

        Token(const char* text, int32 length)
            : Start(text)
            , Length(length)
            , Separator()
        {
        }

        Token(const char* text, int32 length, const SeparatorData& separator)
            : Start(text)
            , Length(length)
            , Separator(separator)
        {
        }

        Token(const Token& other)
            : Start(other.Start)
            , Length(other.Length)
            , Separator(other.Separator)
        {
        }

        /*Token& operator=(const Token& other) const
        {
            return Token(other);
        }*/

    public:
        StringAnsi ToString() const
        {
            return StringAnsi(Start, Length);
        }

    public:
        FORCE_INLINE bool Equals(const Token& other) const
        {
            return Equals(other.Start, other.Length);
        }

        FORCE_INLINE bool Equals(const char* text) const
        {
            return Equals(text, StringUtils::Length(text));
        }

        bool Equals(const char* text, int32 length) const
        {
            return Length == length && StringUtils::Compare(Start, text, Length) == 0;
        }

    public:
        FORCE_INLINE bool EqualsIgnoreCase(const Token& other) const
        {
            return EqualsIgnoreCase(other.Start, other.Length);
        }

        FORCE_INLINE bool EqualsIgnoreCase(const char* text) const
        {
            return EqualsIgnoreCase(text, StringUtils::Length(text));
        }

        bool EqualsIgnoreCase(const char* text, int32 length) const
        {
            return Length == length && StringUtils::CompareIgnoreCase(Start, text, Length) == 0;
        }

    public:
        FORCE_INLINE bool operator==(const char* text) const
        {
            auto token = Token(text);
            return Equals(token);
        }

        FORCE_INLINE bool operator==(const Token& other) const
        {
            return Equals(other);
        }

        FORCE_INLINE bool operator!=(const Token& other) const
        {
            return !Equals(other);
        }
    };

private:
    const char* _buffer;
    int32 _length;
    char* _cursor;
    int32 _position;
    int32 _line;

public:
    /// <summary>
    /// Init
    /// </summary>
    /// <param name="input">Input ANSI text buffer to process</param>
    /// <param name="length">Input data length</param>
    TextProcessing(const char* input, int32 length);

public:
    /// <summary>
    /// Array with all token separators.
    /// </summary>
    Array<SeparatorData> Separators;

    /// /// <summary>
    /// Array with all white characters.
    /// </summary>
    Array<char> Whitespaces;

    SeparatorData SingleLineComment;
    SeparatorData MultiLineCommentSeparator;

public:
    /// <summary>
    /// Sets up separators and white chars for HLSL language.
    /// </summary>
    void Setup_HLSL();

public:
    /// <summary>
    /// Returns true if there are still characters in the buffer and can read data from it
    /// </summary>
    FORCE_INLINE bool CanRead() const
    {
        return _position < _length;
    }

    /// <summary>
    /// Peeks a single character without moving forward in the buffer.
    /// </summary>
    FORCE_INLINE char PeekChar() const
    {
        return *_cursor;
    }

    /// <summary>
    /// Gets the current line number.
    /// </summary>
    FORCE_INLINE int32 GetLine() const
    {
        return _line;
    }

public:
    /// <summary>
    /// Read single character from the buffer
    /// </summary>
    /// <returns>Character</returns>
    char ReadChar();

    /// <summary>
    /// Read all whitespace chars like spaces, tabs, new lines
    /// </summary>
    void EatWhiteSpaces();

    /// <summary>
    /// Reads next token
    /// </summary>
    /// <param name="token">Output token</param>
    void ReadToken(Token* token);

    /// <summary>
    /// Read whole line until new line char '\n'
    /// </summary>
    /// <param name="token">Read token</param>
    void ReadLine(Token* token);

    /// <summary>
    /// Read whole line until new line char '\n'
    /// </summary>
    void ReadLine();

private:
    char moveForward();
    char moveBack();
};
