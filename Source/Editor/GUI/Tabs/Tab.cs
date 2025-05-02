// Copyright (c) Wojciech Figat. All rights reserved.

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
        internal Tabs _selectedInTabs;

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

        /// <summary>
        /// Factory method for tabs header control (UI for the tabs strip to represent this tab).
        /// </summary>
        /// <returns>The tab header control.</returns>
        public virtual Tabs.TabHeader CreateHeader()
        {
            return new Tabs.TabHeader((Tabs)Parent, this);
        }

        /// <inheritdoc />
        protected override void OnParentChangedInternal()
        {
            if (_selectedInTabs != null)
                _selectedInTabs.SelectedTab = null;

            base.OnParentChangedInternal();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;
            if (_selectedInTabs != null)
                _selectedInTabs.SelectedTab = null;

            base.OnDestroy();
        }
    }
}
