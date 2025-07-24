// Copyright (c) Wojciech Figat. All rights reserved.

// Parsing HTML Tags in C# by Jonathan Wood (4 Jul 2012)
// https://www.codeproject.com/Articles/57176/Parsing-HTML-Tags-in-C

using System;
using System.Collections.Generic;

namespace FlaxEngine.Utilities
{
    /// <summary>
    /// The HTML tag description.
    /// </summary>
    public struct HtmlTag
    {
        /// <summary>
        /// Name of the tag.
        /// </summary>
        public string Name;

        /// <summary>
        /// Tag start position in the source text (character index).
        /// </summary>
        public int StartPosition;

        /// <summary>
        /// Tag end position in the source text (character index). Includes any tag attributes data.
        /// </summary>
        public int EndPosition;

        /// <summary>
        /// Collection of attributes for this tag (name + value pairs).
        /// </summary>
        public Dictionary<string, string> Attributes;

        /// <summary>
        /// True if this tag contained a leading forward slash (begin of the tag).
        /// </summary>
        public bool IsLeadingSlash;

        /// <summary>
        /// True if this tag contained a trailing forward slash (end of the tag).
        /// </summary>
        public bool IsEndingSlash;

        /// <summary>
        /// True if this tag contained a leading or trailing forward slash.
        /// </summary>
        public bool IsSlash => IsLeadingSlash || IsEndingSlash;

        /// <inheritdoc />
        public override string ToString()
        {
            return Name;
        }
    };

    /// <summary>
    /// Utility for HTML documents parsing into series of tags.
    /// </summary>
    public class HtmlParser
    {
        private string _html;
        private int _pos;
        private bool _scriptBegin;
        private Dictionary<string, string> _attributes;
        private char[] _newLineChars;

        /// <summary>
        /// Initializes a new instance of the <see cref="HtmlParser"/> class.
        /// </summary>
        public HtmlParser()
        : this(string.Empty)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="HtmlParser"/> class.
        /// </summary>
        /// <param name="html">Text to parse.</param>
        public HtmlParser(string html)
        {
            _html = html;
            _pos = 0;
            _attributes = new Dictionary<string, string>();
            _newLineChars = new[] { '\0', '\r', '\n' }; // [0] is modified by ParseAttributeValue
        }

        /// <summary>
        /// Resets the reading position to the text beginning.
        /// </summary>
        /// <param name="html">Text to parse. Null if unchanged.</param>
        public void Reset(string html = null)
        {
            if (html != null)
                _html = html;
            _pos = 0;
        }

        /// <summary>
        /// Indicates if the current position is at the end of the current document.
        /// </summary>
        private bool EOF => _pos >= _html.Length;

        /// <summary>
        /// Parses the next tag that matches the specified tag name.
        /// </summary>
        /// <param name="tag">Returns information on the next occurrence of the specified tag or null if none found.</param>
        /// <param name="name">Name of the tags to parse (null to parse all tags).</param>
        /// <returns>True if a tag was parsed or false if the end of the document was reached.</returns>
        public bool ParseNext(out HtmlTag tag, string name = null)
        {
            tag = new HtmlTag();

            // Loop until match is found or there are no more tags
            while (MoveToNextTag())
            {
                // Skip opening '<'
                Move();

                char c = Peek();
                if (c == '!' && Peek(1) == '-' && Peek(2) == '-')
                {
                    // Skip over comments
                    const string endComment = "-->";
                    _pos = _html.IndexOf(endComment, _pos, StringComparison.Ordinal);
                    NormalizePosition();
                    Move(endComment.Length);
                }
                else
                {
                    // Skip leading slash
                    bool isLeadingSlash = c == '/';
                    if (isLeadingSlash)
                        Move();

                    // Dont process if wrong slash is used.
                    if (c =='\\')
                        return false;

                    // Parse tag
                    bool result = ParseTag(ref tag, name);

                    // Because scripts may contain tag characters, we need special handling to skip over script contents
                    if (_scriptBegin)
                    {
                        const string endScript = "</script";
                        _pos = _html.IndexOf(endScript, _pos, StringComparison.OrdinalIgnoreCase);
                        NormalizePosition();
                        Move(endScript.Length);
                        SkipWhitespace();
                        if (Peek() == '>')
                            Move();
                    }

                    if (result)
                    {
                        if (isLeadingSlash)
                        {
                            // Tag starts with '/'
                            tag.StartPosition--;
                            tag.IsLeadingSlash = true;
                        }
                        return true;
                    }
                }
            }

            return false;
        }

        /// <summary>
        /// Parses the contents of an HTML tag. The current position should be at the first character following the tag's opening less-than character.
        /// </summary>
        /// <remarks>We parse to the end of the tag even if this tag was not requested by the caller. This ensures subsequent parsing takes place after this tag.</remarks>
        /// <param name="tag">Returns information on this tag if it's one the caller is requesting.</param>
        /// <param name="name">Name of the tags to parse (null to parse all tags).</param>
        /// <returns>True if data is being returned for a tag requested by the caller or false otherwise.</returns>
        private bool ParseTag(ref HtmlTag tag, string name = null)
        {
            // Get name of this tag
            int start = _pos;
            string s = ParseTagName();

            // Special handling
            bool doctype = _scriptBegin = false;
            if (string.Compare(s, "!DOCTYPE", StringComparison.OrdinalIgnoreCase) == 0)
                doctype = true;
            else if (string.Compare(s, "script", StringComparison.OrdinalIgnoreCase) == 0)
                _scriptBegin = true;

            // Is this a tag requested by caller?
            bool requested = false;
            if (name == null || string.Compare(s, name, StringComparison.OrdinalIgnoreCase) == 0)
            {
                // Setup new tag
                _attributes.Clear();
                tag = new HtmlTag
                {
                    Name = s,
                    StartPosition = start - 1,
                    Attributes = _attributes,
                };
                requested = true;
            }

            // Parse attributes
            SkipWhitespace();
            while (Peek() != '>')
            {
                // Return false if start of new html tag is detected.
                if (Peek() == '<')
                    return false;
                
                if (Peek() == '/')
                {
                    // Handle trailing forward slash
                    if (requested)
                        tag.IsEndingSlash = true;
                    Move();
                    SkipWhitespace();

                    // If this is a script tag, it was closed
                    _scriptBegin = false;
                }
                else
                {
                    // Parse attribute name
                    s = !doctype ? ParseAttributeName() : ParseAttributeValue();
                    SkipWhitespace();

                    // Parse attribute value
                    var value = string.Empty;
                    if (Peek() == '=')
                    {
                        Move();
                        SkipWhitespace();
                        value = ParseAttributeValue();
                        SkipWhitespace();
                    }

                    // Add attribute to collection if requested tag
                    if (requested)
                    {
                        tag.Attributes[s] = value;
                    }
                }

                if (EOF)
                    return false;
            }

            // Skip over closing '>'
            Move();

            tag.EndPosition = _pos;
            return requested;
        }

        /// <summary>
        /// Parses a tag name. The current position should be the first character of the name.
        /// </summary>
        /// <returns>Returns the parsed name string.</returns>
        private string ParseTagName()
        {
            int start = _pos;
            while (!EOF)
            {
                var c = Peek();
                if (!char.IsLetterOrDigit(c))
                    break;
                Move();
            }
            return _html.Substring(start, _pos - start);
        }

        /// <summary>
        /// Parses an attribute name. The current position should be the first character of the name.
        /// </summary>
        /// <returns>Returns the parsed name string.</returns>
        private string ParseAttributeName()
        {
            int start = _pos;
            while (!EOF)
            {
                var c = Peek();
                if (!char.IsLetterOrDigit(c) && c != '-')
                    break;
                Move();
            }
            return _html.Substring(start, _pos - start);
        }

        /// <summary>
        /// Parses an attribute value. The current position should be the first non-whitespace character following the equal sign.
        /// </summary>
        /// <remarks>We terminate the name or value if we encounter a new line. This seems to be the best way of handling errors such as values missing closing quotes, etc.</remarks>
        /// <returns>Returns the parsed value string.</returns>
        private string ParseAttributeValue()
        {
            int start, end;
            char c = Peek();
            if (c == '"' || c == '\'')
            {
                // Move past opening quote
                Move();

                // Parse quoted value
                start = _pos;
                _newLineChars[0] = c;
                _pos = _html.IndexOfAny(_newLineChars, start);
                NormalizePosition();
                end = _pos;

                // Move past closing quote
                if (Peek() == c)
                    Move();
            }
            else
            {
                // Parse unquoted value
                start = _pos;
                while (!EOF && !char.IsWhiteSpace(c) && c != '>' && c != '/')
                {
                    Move();
                    c = Peek();
                }
                end = _pos;
            }
            return _html.Substring(start, end - start);
        }

        /// <summary>
        /// Moves to the start of the next tag.
        /// </summary>
        /// <returns>True if another tag was found, false otherwise.</returns>
        private bool MoveToNextTag()
        {
            _pos = _html.IndexOf('<', _pos);
            NormalizePosition();
            return !EOF;
        }

        /// <summary>
        /// Returns the character at the specified number of characters beyond the current position, or a null character if the specified position is at the end of the document.
        /// </summary>
        /// <param name="ahead">The number of characters beyond the current position.</param>
        /// <returns>The character at the specified position.</returns>
        private char Peek(int ahead = 0)
        {
            int pos = _pos + ahead;
            if (pos < _html.Length)
                return _html[pos];
            return (char)0;
        }

        /// <summary>
        /// Moves the current position ahead the specified number of characters.
        /// </summary>
        /// <param name="ahead">The number of characters to move ahead.</param>
        private void Move(int ahead = 1)
        {
            _pos = Math.Min(_pos + ahead, _html.Length);
        }

        /// <summary>
        /// Moves the current position to the next character that is not whitespace.
        /// </summary>
        private void SkipWhitespace()
        {
            while (!EOF && char.IsWhiteSpace(Peek()))
                Move();
        }

        /// <summary>
        /// Normalizes the current position. This is primarily for handling conditions where IndexOf(), etc. return negative values when the item being sought was not found.
        /// </summary>
        private void NormalizePosition()
        {
            if (_pos < 0)
                _pos = _html.Length;
        }
    }
}
