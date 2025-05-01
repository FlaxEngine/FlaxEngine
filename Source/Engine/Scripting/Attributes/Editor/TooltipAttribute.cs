// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Specifies a tooltip for a property/field in the editor.
    /// </summary>
    /// <seealso cref="System.Attribute" />
    [Serializable]
    [AttributeUsage(AttributeTargets.All)]
    public sealed class TooltipAttribute : Attribute
    {
        /// <summary>
        /// The tooltip text.
        /// </summary>
        public string Text;

        private TooltipAttribute()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TooltipAttribute"/> class.
        /// </summary>
        /// <param name="text">The tooltip text.</param>
        public TooltipAttribute(string text)
        {
            Text = text;
        }
    }
}
