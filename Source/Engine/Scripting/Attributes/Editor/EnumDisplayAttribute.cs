// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Allows to change enum type field or property display mode in the editor.
    /// </summary>
    /// <seealso cref="System.Attribute" />
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class EnumDisplayAttribute : Attribute
    {
        /// <summary>
        /// Enumeration items names formatting modes.
        /// </summary>
        public enum FormatMode
        {
            /// <summary>
            /// The default formatting. Performs standard name processing to create more human-readable label for User Interface.
            /// </summary>
            Default = 0,

            /// <summary>
            /// The none formatting. The enum items names won't be modified.
            /// </summary>
            None = 1,
        }

        /// <summary>
        /// The formatting mode.
        /// </summary>
        public FormatMode Mode;

        /// <summary>
        /// Initializes a new instance of the <see cref="EnumDisplayAttribute"/> class.
        /// </summary>
        /// <param name="mode">The formatting mode.</param>
        public EnumDisplayAttribute(FormatMode mode)
        {
            Mode = mode;
        }
    }
}
