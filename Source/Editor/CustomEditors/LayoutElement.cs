// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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

        /// <summary>
        /// Implicit cast using LayoutElement.Control
        /// </summary>
        /// <param name="layoutElement"></param>
        public static implicit operator Control(LayoutElement layoutElement) => layoutElement?.Control;

    }
}
