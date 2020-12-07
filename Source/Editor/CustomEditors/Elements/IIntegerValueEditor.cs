// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// The integer value editor element.
    /// </summary>
    public interface IIntegerValueEditor
    {
        /// <summary>
        /// Gets or sets the value.
        /// </summary>
        int Value { get; set; }

        /// <summary>
        /// Gets a value indicating whether user is using a slider.
        /// </summary>
        bool IsSliding { get; }
    }
}
