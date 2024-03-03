// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors
{
    /// <summary>
    /// Represents single element of the Custom Editor layout.
    /// </summary>
    [HideInEditor]
    public abstract class LayoutElement
    {
        /// <summary>
        /// Gets the control represented by this element.
        /// </summary>
        public abstract Control Control { get; }
    }
}
