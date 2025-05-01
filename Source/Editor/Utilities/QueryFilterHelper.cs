// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEngine;

namespace FlaxEditor.Utilities
{
    /// <summary>
    /// Helper class to filter items based on a input filter query.
    /// </summary>
    [HideInEditor]
    public static class QueryFilterHelper
    {
        /// <summary>
        /// The minimum text match length.
        /// </summary>
        public const int MinLength = 1;

        /// <summary>
        /// Matches the specified text with the filter.
        /// </summary>
        /// <param name="filter">The filter.</param>
        /// <param name="text">The text.</param>
        /// <returns>True if text has one or more matches, otherwise false.</returns>
        public static bool Match(string filter, string text)
        {
            // Empty inputs
            if (string.IsNullOrEmpty(filter) || string.IsNullOrEmpty(text))
                return false;

            // Full match
            if (string.Equals(filter, text, StringComparison.CurrentCultureIgnoreCase))
            {
                return true;
            }

            bool hasMatch = false;

            // Find matching sequences
            // We do simple iteration over the characters
            int textLength = text.Length;
            int filterLength = filter.Length;
            int searchEnd = textLength - filterLength;
            for (int textPos = 0; textPos <= searchEnd; textPos++)
            {
                // Skip if the current text position doesn't match the filter start
                if (char.ToLower(filter[0]) != char.ToLower(text[textPos]))
                    continue;

                int matchStartPos = -1;
                int endPos = textPos + filterLength;
                int filterPos = 0;

                for (int i = textPos; i < endPos; i++, filterPos++)
                {
                    var filterChar = char.ToLower(filter[filterPos]);
                    var textChar = char.ToLower(text[i]);

                    if (filterChar == textChar)
                    {
                        // Check if start the matching sequence
                        if (matchStartPos == -1)
                        {
                            matchStartPos = textPos;
                        }
                    }
                    else
                    {
                        // Check if stop matching sequence
                        if (matchStartPos != -1)
                        {
                            var length = textPos - matchStartPos;
                            if (length >= MinLength)
                                hasMatch = true;
                            textPos = matchStartPos + length;
                            matchStartPos = -1;
                        }
                        break;
                    }
                }

                // Check sequence on the end
                if (matchStartPos != -1 && filterPos == filterLength)
                {
                    var length = endPos - matchStartPos;
                    if (length >= MinLength)
                        hasMatch = true;
                    textPos = matchStartPos + length;
                }
            }

            return hasMatch;
        }

        /// <summary>
        /// Matches the specified text with the filter.
        /// </summary>
        /// <param name="filter">The filter.</param>
        /// <param name="text">The text.</param>
        /// <param name="matches">The found matches.</param>
        /// <returns>True if text has one or more matches, otherwise false.</returns>
        public static bool Match(string filter, string text, out Range[] matches)
        {
            // Empty inputs
            matches = null;
            if (string.IsNullOrEmpty(filter) || string.IsNullOrEmpty(text))
                return false;

            // Full match
            if (string.Equals(filter, text, StringComparison.CurrentCultureIgnoreCase))
            {
                matches = new[] { new Range(0, filter.Length) };
                return true;
            }

            List<Range> ranges = null;

            // Find matching sequences by doing simple iteration over the characters
            int textLength = text.Length;
            int filterLength = filter.Length;
            int searchEnd = textLength - filterLength;
            for (int textPos = 0; textPos <= searchEnd; textPos++)
            {
                // Skip if the current text position doesn't match the filter start
                if (char.ToLower(filter[0]) != char.ToLower(text[textPos]))
                    continue;

                int matchStartPos = -1;
                int endPos = textPos + filterLength;
                int filterPos = 0;

                for (int i = textPos; i < endPos; i++, filterPos++)
                {
                    var filterChar = char.ToLower(filter[filterPos]);
                    var textChar = char.ToLower(text[i]);

                    if (filterChar == textChar)
                    {
                        // Check if start the matching sequence
                        if (matchStartPos == -1)
                        {
                            ranges ??= new List<Range>();
                            matchStartPos = textPos;
                        }
                    }
                    else
                    {
                        // Check if stop matching sequence
                        if (matchStartPos != -1)
                        {
                            var length = textPos - matchStartPos;
                            if (length >= MinLength)
                                ranges!.Add(new Range(matchStartPos, length));
                            textPos = matchStartPos + length;
                            matchStartPos = -1;
                        }
                        break;
                    }
                }

                // Check sequence on the end
                if (matchStartPos != -1 && filterPos == filterLength)
                {
                    var length = endPos - matchStartPos;
                    if (length >= MinLength)
                        ranges!.Add(new Range(matchStartPos, length));
                    textPos = matchStartPos + length;
                }
            }

            // Check if has any range
            if (ranges is { Count: > 0 })
            {
                matches = ranges.ToArray();
                return true;
            }

            return false;
        }

        /// <summary>
        /// Describes sub range of the text.
        /// </summary>
        public readonly struct Range
        {
            /// <summary>
            /// The start index of the range.
            /// </summary>
            public readonly int StartIndex;

            /// <summary>
            /// The length.
            /// </summary>
            public readonly int Length;

            /// <summary>
            /// The end index of the range.
            /// </summary>
            public int EndIndex => StartIndex + Length;

            /// <summary>
            /// Initializes a new instance of the <see cref="Range"/> struct.
            /// </summary>
            /// <param name="start">The start.</param>
            /// <param name="length">The length.</param>
            public Range(int start, int length)
            {
                StartIndex = start;
                Length = length;
            }

            /// <summary>
            /// Tests for equality between two objects.
            /// </summary>
            /// <param name="left">The first value to compare.</param>
            /// <param name="right">The second value to compare.</param>
            /// <returns><c>true</c> if <paramref name="left"/> has the same value as <paramref name="right"/>; otherwise, <c>false</c>.</returns>
            public static bool operator ==(Range left, Range right)
            {
                return left.Equals(right);
            }

            /// <summary>
            /// Tests for equality between two objects.
            /// </summary>
            /// <param name="left">The first value to compare.</param>
            /// <param name="right">The second value to compare.</param>
            /// <returns><c>true</c> if <paramref name="left"/> has the same value as <paramref name="right"/>; otherwise, <c>false</c>.</returns>
            public static bool operator !=(Range left, Range right)
            {
                return !left.Equals(right);
            }

            /// <summary>
            /// Compares this object with the other instance.
            /// </summary>
            /// <param name="other">The other object.</param>
            /// <returns>True if objects are equal.</returns>
            public bool Equals(Range other)
            {
                return StartIndex == other.StartIndex && Length == other.Length;
            }

            /// <inheritdoc />
            public override bool Equals(object obj)
            {
                if (ReferenceEquals(null, obj))
                    return false;
                return obj is Range && Equals((Range)obj);
            }

            /// <inheritdoc />
            public override int GetHashCode()
            {
                unchecked
                {
                    return (StartIndex * 397) ^ Length;
                }
            }

            /// <inheritdoc />
            public override string ToString()
            {
                return $"StartIndex: {StartIndex}, Length: {Length}";
            }
        }
    }
}
