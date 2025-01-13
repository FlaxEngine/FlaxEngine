// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "TextProcessing.h"
#include "Engine/Core/Log.h"

TextProcessing::TextProcessing(const char* input, int32 length)
    : _buffer(input)
    , _length(length)
    , _cursor(const_cast<char*>(input))
    , _position(0)
    , _line(1)
    , Separators(32)
    , Whitespaces(8)
{
}

void TextProcessing::Setup_HLSL()
{
    static SeparatorData separators[] =
    {
        { '\r', '\n' },
        { '/', '/' },
        { '/', '*' },
        { '\n', 0 },
        { '\t', 0 },
        { ' ', 0 },
        { '.', 0 },
        { ',', 0 },
        { ':', 0 },
        { ';', 0 },
        { '+', 0 },
        { '-', 0 },
        { '(', 0 },
        { ')', 0 },
        { '!', 0 },
        { '=', 0 },
        { '&', 0 },
        { '%', 0 },
        { '*', 0 },
        { '<', 0 },
        { '>', 0 },
        { '[', 0 },
        { ']', 0 },
        { '{', 0 },
        { '}', 0 },
    };
    Separators.Set(separators, ARRAY_COUNT(separators));
    static char whitespaces[] =
    {
        9,
        10,
        11,
        12,
        13,
        32,
    };
    Whitespaces.Set(whitespaces, ARRAY_COUNT(whitespaces));
    SingleLineComment = SeparatorData('/', '/');
    MultiLineCommentSeparator = SeparatorData('/', '*');
}

char TextProcessing::ReadChar()
{
    if (_position >= _length)
        return 0;
    return moveForward();
}

void TextProcessing::EatWhiteSpaces()
{
    // Read until can
    while (_position < _length)
    {
        // Peek char
        char c = PeekChar();

        // Check if is whitespace
        bool isToken = true;
        for (int32 i = 0; i < Whitespaces.Count(); i++)
        {
            if (Whitespaces[i] == c)
            {
                // Read it
                moveForward();
                isToken = false;
                break;
            }
        }
        if (isToken)
        {
            // End
            return;
        }
    }
}

void TextProcessing::ReadToken(Token* token)
{
    // Eat whitespaces
    EatWhiteSpaces();

    // Set token start but reset its length
    token->Start = _cursor;
    token->Length = 0;
    token->Separator = { 0, 0 };

    // Read until can
    while (_position < _length)
    {
        // Read char
        char c = moveForward();

        // Check if it's a separator char
        char c1 = PeekChar();
        for (int32 s = 0; s < Separators.Count(); s++)
        {
            if (Separators[s].C0 == c && Separators[s].C1 == c1)
            {
                /*if (*length == 0)
                {
                    // Skip separator after separator case and start searching for new token
                    *token = _cursor;
                    *length = -1;
                    break;
                }*/

                token->Separator = Separators[s];
                ReadChar();
                
                // Check for comments
                if (token->Separator == SingleLineComment)
                {
                    // Read whole line
                    ReadLine();

                    // Read another token
                    ReadToken(token);
                }
                else if (token->Separator == MultiLineCommentSeparator)
                {
                    // Read tokens until end sequence
                    char prev = ' ';
                    while (CanRead())
                    {
                        c = ReadChar();
                        if (prev == '*' && c == '/')
                            break;
                        prev = c;
                    }

                    // Check if comment is valid (has end before file end)
                    if (!CanRead())
                    {
                        LOG(Warning, "Missing multiline comment ending");
                    }

                    // Read another token
                    ReadToken(token);
                }

                return;
            }
        }
        for (int32 s = 0; s < Separators.Count(); s++)
        {
            if (Separators[s].C0 == c && Separators[s].C1 == 0)
            {
                if (token->Length == 0)
                {
                    // Skip separator after separator case and start searching for new token
                    token->Start = _cursor;
                    token->Length = -1;
                    break;
                }

                token->Separator = Separators[s];
                return;
            }
        }

        // Move to the next character
        token->Length++;
    }
}

void TextProcessing::ReadLine(Token* token)
{
    // Set line start but reset its length
    token->Start = _cursor;
    token->Length = 0;

    // Read until can
    while (_position < _length)
    {
        // Read char
        char c = moveForward();

        // Check new-line char
        if (c == '\n')
        {
            return;
        }

        // Move to the next char
        token->Length++;
    }
}

void TextProcessing::ReadLine()
{
    // Read until can
    while (_position < _length)
    {
        // Read char
        char c = moveForward();

        // Check new-line char
        if (c == '\n')
        {
            return;
        }
    }
}

char TextProcessing::moveForward()
{
    char c = *_cursor;
    if (c == '\n')
        _line++;
    _cursor++;
    _position++;
    return c;
}

char TextProcessing::moveBack()
{
    _position--;
    _cursor--;
    char c = *_cursor;
    if (c == '\n')
        _line--;
    return c;
}
