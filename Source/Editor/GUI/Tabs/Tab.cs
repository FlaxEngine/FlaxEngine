// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Tabs
{
    /// <summary>
    /// Single tab control used by <see cref="Tabs"/>.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [HideInEditor]
    public class Tab : ContainerControl
    {
        /// <summary>
        /// Gets or sets the text.
        /// </summary>
        public string Text;

        /// <summary>
        /// Gets or sets the icon.
        /// </summary>
        public SpriteHandle Icon;

        /// <summary>
        /// Occurs when tab gets selected.
        /// </summary>
        public event Action<Tab> Selected;

        /// <summary>
        /// Occurs when tab gets deselected.
        /// </summary>
        public event Action<Tab> Deselected;

        /// <summary>
        /// Initializes a new instance of the <see cref="Tab"/> class.
        /// </summary>
        /// <param name="icon">The icon.</param>
        public Tab(SpriteHandle icon)
        : this(string.Empty, icon)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Tab"/> class.
        /// </summary>
        /// <param name="text">The text.</param>
        public Tab(string text)
        : this(text, SpriteHandle.Invalid)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Tab"/> class.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="icon">The icon.</param>
        public Tab(string text, SpriteHandle icon)
        {
            Text = text;
            Icon = icon;
        }

        /// <summary>
        /// Called when tab gets selected.
        /// </summary>
        public virtual void OnSelected()
        {
            Selected?.Invoke(this);
        }

        /// <summary>
        /// Called when tab gets deselected.
        /// </summary>
        public virtual void OnDeselected()
        {
            Deselected?.Invoke(this);
        }
    }
}
