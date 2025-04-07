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
        /// Initializes a new instance of the <see cref="HideInEditorAttribute"/> class.
        /// </summary>
        public HideInEditorAttribute()
        {
        }
    }
}
