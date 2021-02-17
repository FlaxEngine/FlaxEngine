// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;

namespace Flax.Build
{
    /// <summary>
    /// Types of the tokens supported by the <see cref="Tokenizer"/>.
    /// </summary>
    public enum TokenType
    {
        /// <summary>
        /// A whitespace.
        /// </summary>
        Whitespace,

        /// <summary>
        /// A Newline.
        /// </summary>
        Newline,

        /// <summary>
        /// A multi line comment.
        /// </summary>
        CommentMultiLine,

        /// <summary>
        /// A single line comment.
        /// </summary>
        CommentSingleLine,

        /// <summary>
        /// An identifier.
        /// </summary>
        Identifier,

        /// <summary>
        /// A number in hexadecimal form.
        /// </summary>
        Hex,

        /// <summary>
        /// A number.
        /// </summary>
        Number,

        /// <summary>
        /// The symbol '='.
        /// </summary>
        Equal,

        /// <summary>
        /// A comma ','.
        /// </summary>
        Comma,

        /// <summary>
        /// A Semicolon ';'.
        /// </summary>
        SemiColon,

        /// <summary>
        /// A left curly brace '{'.
        /// </summary>
        LeftCurlyBrace,

        /// <summary>
        /// A right curly brace '}'.
        /// </summary>
        RightCurlyBrace,

        /// <summary>
        /// A left parenthesis '('.
        /// </summary>
        LeftParent,

        /// <summary>
        /// A right parenthesis ')'.
        /// </summary>
        RightParent,

        /// <summary>
        /// A left bracket '['.
        /// </summary>
        LeftBracket,

        /// <summary>
        /// A right bracket ']'.
        /// </summary>
        RightBracket,

        /// <summary>
        /// A text.
        /// </summary>
        String,

        /// <summary>
        /// An character.
        /// </summary>
        Character,

        /// <summary>
        /// A preprocessor token '#'
        /// </summary>
        Preprocessor,

        /// <summary>
        /// A colon ':'.
        /// </summary>
        Colon,

        /// <summary>
        /// A double colon '::'.
        /// </summary>
        DoubleColon,

        /// <summary>
        /// A dot '.'.
        /// </summary>
        Dot,

        /// <summary>
        /// A '&lt;'.
        /// </summary>
        LessThan,

        /// <summary>
        /// A '&gt;'.
        /// </summary>
        GreaterThan,

        /// <summary>
        /// A '&amp;'.
        /// </summary>
        And,

        /// <summary>
        /// A '*'.
        /// </summary>
        Multiply,

        /// <summary>
        /// A '/'.
        /// </summary>
        Divide,

        /// <summary>
        /// A '+'.
        /// </summary>
        Add,

        /// <summary>
        /// A '-'.
        /// </summary>
        Sub,

        /// <summary>
        /// An unknown symbol.
        /// </summary>
        Unknown,

        /// <summary>
        /// A end of file token.
        /// </summary>
        EndOfFile,

        /// <summary>
        /// A '&lt;'.
        /// </summary>
        LeftAngleBracket = LessThan,

        /// <summary>
        /// A '&gt;'.
        /// </summary>
        RightAngleBracket = GreaterThan,
    }

    /// <summary>
    /// Contains information about a token language.
    /// </summary>
    public class Token : IEquatable<Token>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="Token"/> class.
        /// </summary>
        public Token()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Token" /> struct.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <param name="value">The value.</param>
        public Token(TokenType type, string value)
        {
            Type = type;
            Value = value;
        }

        /// <summary>
        /// The type of the token.
        /// </summary>
        public TokenType Type;

        /// <summary>
        /// Value of the token.
        /// </summary>
        public string Value;

        /// <inheritdoc />
        public override string ToString()
        {
            return string.Format("{{{0}:{1}}}", Type, Value);
        }

        /// <inheritdoc />
        public bool Equals(Token other)
        {
            if (ReferenceEquals(null, other))
                return false;
            if (ReferenceEquals(this, other))
                return true;
            return Type == other.Type && Value == other.Value;
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj))
                return false;
            if (ReferenceEquals(this, obj))
                return true;
            if (obj.GetType() != this.GetType())
                return false;
            return Equals((Token)obj);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                return ((int)Type * 397) ^ (Value != null ? Value.GetHashCode() : 0);
            }
        }
    }

    /// <summary>
    /// The tokens parsing utility that implements basic logic for generic C-like syntax source code parsing.
    /// </summary>
    public class Tokenizer : IDisposable
    {
        private static readonly Regex RegexTokenizer = new Regex
        (
         @"(?<ws>[ \t]+)|" +
         @"(?<nl>(?:\r\n|\n))|" +
         @"(?<commul>/\*(?:(?!\*/)(?:.|[\r\n]+))*\*/)|" +
         @"(?<comsin>//(.*?)\r?\n)|" +
         @"(?<ident>[a-zA-Z_][a-zA-Z0-9_]*)|" +
         @"(?<hex>0x[0-9a-fA-F]+)|" +
         @"(?<number>[\-\+]?\s*[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?f?)|" +
         @"(?<equal>=)|" +
         @"(?<comma>,)|" +
         @"(?<semicolon>;)|" +
         @"(?<lcb>\{)|" +
         @"(?<rcb>\})|" +
         @"(?<lpar>\()|" +
         @"(?<rpar>\))|" +
         @"(?<lb>\[)|" +
         @"(?<rb>\])|" +
         @"(?<str>""[^""\\]*(?:\\.[^""\\]*)*"")|" +
         @"(?<char>'[^'\\]*(?:\\.[^'\\]*)*')|" +
         @"(?<prep>#)|" +
         @"(?<colon>:)|" +
         @"(?<doublecolon>::)|" +
         @"(?<dot>\.)|" +
         @"(?<lt>\<)|" +
         @"(?<gt>\>)|" +
         @"(?<and>\&)|" +
         @"(?<mul>\*)|" +
         @"(?<div>\/)|" +
         @"(?<add>\+)|" +
         @"(?<sub>\-)|" +
         @"(?<unk>[^\s]+)",
         RegexOptions.Compiled
        );

        private ITwoWayEnumerator<Token> _tokenEnumerator;
        private int _line = 1;

        /// <summary>
        /// Gets the current token.
        /// </summary>
        public Token CurrentToken => _tokenEnumerator.Current;

        /// <summary>
        /// Gets the current line number (starting from number 1).
        /// </summary>
        public int CurrentLine => _line;

        /// <summary>
        /// Tokenizes the given file (through constructor).
        /// </summary>
        /// <param name="sourceCode">The source code for this tokenizer to run on.</param>
        public void Tokenize(string sourceCode)
        {
            if (_tokenEnumerator != null)
                throw new Exception("This code is already parsed!");

            var tokens = TokenizeInternal(sourceCode);
            _tokenEnumerator = tokens.GetTwoWayEnumerator();
        }

        /// <summary>
        /// Gets next token.
        /// </summary>
        /// <param name="includeWhitespaces">When false, all white-space tokens will be ignored.</param>
        /// <param name="includeComments">When false, all comment (single line and multi-line) tokens will be ignored.</param>
        /// <returns>The token. Check for EndOfFile token-type to detect end-of-file.</returns>
        public Token NextToken(bool includeWhitespaces = false, bool includeComments = false)
        {
            while (_tokenEnumerator.MoveNext())
            {
                var token = _tokenEnumerator.Current;
                if (token == null)
                    continue;

                _line += CountLines(token);

                if (token.Type == TokenType.Newline)
                {
                    if (includeWhitespaces)
                        return token;
                    continue;
                }

                if (!includeWhitespaces && token.Type == TokenType.Whitespace)
                {
                    continue;
                }

                return token;
            }

            return new Token(TokenType.EndOfFile, string.Empty);
        }

        /// <summary>
        /// Moves to the previous the token.
        /// </summary>
        /// <param name="includeWhitespaces">If set to <c>true</c> includes whitespaces.</param>
        /// <param name="includeComments">If set to <c>true</c> include comments.</param>
        /// <returns>The token. Check for EndOfFile token-type to detect end-of-file.</returns>
        public Token PreviousToken(bool includeWhitespaces = false, bool includeComments = false)
        {
            while (_tokenEnumerator.MovePrevious())
            {
                var token = _tokenEnumerator.Current;
                if (token == null)
                    continue;

                _line -= CountLines(token);

                if (token.Type == TokenType.Newline)
                {
                    if (includeWhitespaces)
                        return token;
                    continue;
                }

                if (!includeWhitespaces && token.Type == TokenType.Whitespace)
                {
                    continue;
                }

                return token;
            }

            return new Token(TokenType.EndOfFile, string.Empty);
        }

        /// <summary>
        /// Expects any token of given types. Throws <see cref="Exception"/> when token is not found.
        /// </summary>
        /// <param name="tokenTypes">The allowed token types.</param>
        /// <param name="includeWhitespaces">When false, all white-space tokens will be ignored.</param>
        /// <param name="includeComments">When false, all comment (single line and multi-line) tokens will be ignored.</param>
        /// <returns>The found token.</returns>
        public Token ExpectAnyTokens(TokenType[] tokenTypes, bool includeWhitespaces = false, bool includeComments = false)
        {
            var token = NextToken(includeWhitespaces, includeComments);

            if (tokenTypes.Contains(token.Type))
                return token;

            throw new Exception($"Expected {string.Join(" or ", tokenTypes)}, but got {token} at line {_line}.");
        }

        /// <summary>
        /// Expects token of given types in the same order. Throws <see cref="Exception"/> when token is not found.
        /// </summary>
        /// <param name="tokenTypes">The allowed token types.</param>
        /// <param name="includeWhitespaces">When false, all white-space tokens will be ignored.</param>
        /// <param name="includeComments">When false, all comment (single line and multi-line) tokens will be ignored.</param>
        /// <returns>The found token.</returns>
        public void ExpectAllTokens(TokenType[] tokenTypes, bool includeWhitespaces = false, bool includeComments = false)
        {
            foreach (var tokenType in tokenTypes)
            {
                var token = NextToken(includeWhitespaces, includeComments);

                if (token.Type != tokenType)
                    throw new Exception($"Expected {tokenType}, but got {token} at line {_line}.");
            }
        }

        /// <summary>
        /// Expects any token of given type. Throws <see cref="Exception"/> when token is not found.
        /// </summary>
        /// <param name="tokenType">The only allowed token type.</param>
        /// <param name="includeWhitespaces">When false, all white-space tokens will be ignored.</param>
        /// <param name="includeComments">When false, all comment (single line and multi-line) tokens will be ignored.</param>
        /// <returns>The found token.</returns>
        public Token ExpectToken(TokenType tokenType, bool includeWhitespaces = false, bool includeComments = false)
        {
            var token = NextToken(includeWhitespaces, includeComments);

            if (token.Type == tokenType)
                return token;

            throw new Exception($"Expected {tokenType}, but got {token} at line {_line}.");
        }

        /// <summary>
        /// Skips all tokens until the tokenizer steps into token of given type (and it is also skipped, so, NextToken will give the next token).
        /// </summary>
        /// <param name="tokenType">The expected token type.</param>
        public void SkipUntil(TokenType tokenType)
        {
            do
            {
            } while (NextToken(true).Type != tokenType);
        }

        /// <summary>
        /// Skips all tokens until the tokenizer steps into token of given type (and it is also skipped, so, NextToken will give the next token).
        /// </summary>
        /// <param name="tokenType">The expected token type.</param>
        /// <param name="context">The output contents of the skipped tokens.</param>
        public void SkipUntil(TokenType tokenType, out string context)
        {
            context = string.Empty;
            while (NextToken(true).Type != tokenType)
            {
                context += CurrentToken.Value;
            }
        }

        /// <summary>
        /// Skips all tokens until the tokenizer steps into token of given type (and it is also skipped, so, NextToken will give the next token).
        /// </summary>
        /// <param name="tokenType">The expected token type.</param>
        /// <param name="context">The output contents of the skipped tokens.</param>
        /// <param name="includeWhitespaces">When false, all white-space tokens will be ignored.</param>
        public void SkipUntil(TokenType tokenType, out string context, bool includeWhitespaces)
        {
            context = string.Empty;
            while (NextToken(true).Type != tokenType)
            {
                var token = CurrentToken;
                if (!includeWhitespaces && (token.Type == TokenType.Newline || token.Type == TokenType.Whitespace))
                    continue;
                context += token.Value;
            }
        }

        /// <summary>
        /// Disposes the <see cref="Tokenizer"/>.
        /// </summary>
        public void Dispose()
        {
            _tokenEnumerator?.Dispose();
        }

        private IEnumerable<Token> TokenizeInternal(string input)
        {
            var matches = RegexTokenizer.Matches(input);
            foreach (Match match in matches)
            {
                var i = 0;
                foreach (Group group in match.Groups)
                {
                    var matchValue = group.Value;

                    if (group.Success && i > 1)
                    {
                        yield return new Token
                        {
                            Type = (TokenType)(i - 2),
                            Value = matchValue
                        };
                    }

                    i++;
                }
            }
        }

        private int CountLines(Token token)
        {
            int result = 0;
            switch (token.Type)
            {
            case TokenType.Newline:
            case TokenType.CommentSingleLine:
                result = 1;
                break;
            case TokenType.CommentMultiLine:
                for (int i = 0; i < token.Value.Length; i++)
                {
                    if (token.Value[i] == '\n')
                        result++;
                }
                break;
            }
            return result;
        }
    }
}
