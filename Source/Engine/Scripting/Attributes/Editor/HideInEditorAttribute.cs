// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Makes a variable not show up in the editor.
    /// </summary>
    [Serializable]
    public sealed class HideInEditorAttribute : Attribute
    {
        /// <summary>
        /// Shows the variable only in play mode (when the game is running), otherwise it will be hidden. Useful for runtime fields or properties to remain invisible at edit time.
        /// </summary>
        public bool ShowInPlayMode;

        /// <summary>
        /// Initializes a new instance of the <see cref="HideInEditorAttribute"/> class.
        /// </summary>
        public HideInEditorAttribute()
        {
        }
    }
}
