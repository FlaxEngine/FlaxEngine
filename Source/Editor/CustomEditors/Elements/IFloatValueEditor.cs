// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// The floating point value editor element.
    /// </summary>
    [HideInEditor]
    public interface IFloatValueEditor
    {
        /// <summary>
        /// Gets or sets the value.
        /// </summary>
        float Value { get; set; }

        /// <summary>
        /// Gets a value indicating whether user is using a slider.
        /// </summary>
        bool IsSliding { get; }

        /// <summary>
        /// Sets the editor limits from member <see cref="LimitAttribute"/>.
        /// </summary>
        /// <param name="limit">The limit.</param>
        void SetLimits(LimitAttribute limit);
    }
}
