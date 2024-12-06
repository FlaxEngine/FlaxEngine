// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Displays the method in the properties panel where user can click and invoke this method.
    /// </summary>
    /// <remarks>Supported on both static and member methods that are parameterless.</remarks>
    [AttributeUsage(AttributeTargets.Method)]
    public sealed class ButtonAttribute : Attribute
    {
        /// <summary>
        /// The button text. Empty value will use method name (auto-formatted).
        /// </summary>
        public string Text;

        /// <summary>
        /// The button tooltip text. Empty value will use method documentation.
        /// </summary>
        public string Tooltip;

        /// <summary>
        /// Initializes a new instance of the <see cref="ButtonAttribute"/> class.
        /// </summary>
        public ButtonAttribute()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ButtonAttribute"/> class.
        /// </summary>
        /// <param name="text">The button text.</param>
        /// <param name="tooltip">The button tooltip.</param>
        public ButtonAttribute(string text, string tooltip = null)
        {
            Text = text;
            Tooltip = tooltip;
        }
    }
}
