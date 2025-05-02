// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// The label element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElement" />
    public class LabelElement : LayoutElement
    {
        private Action<ContextMenu> _customContextualOptions;

        /// <summary>
        /// The label.
        /// </summary>
        public readonly ClickableLabel Label;

        /// <summary>
        /// Initializes a new instance of the <see cref="CheckBoxElement"/> class.
        /// </summary>
        public LabelElement()
        {
            Label = new ClickableLabel
            {
                Size = new Float2(100, 18),
                HorizontalAlignment = TextAlignment.Near,
            };
            // TODO: auto height for label
        }

        /// <summary>
        /// Adds a simple context menu with utility to copy label text. Can be extended with more options.
        /// </summary>
        public LabelElement AddCopyContextMenu(Action<ContextMenu> customOptions = null)
        {
            Label.RightClick += OnRightClick;
            _customContextualOptions = customOptions;
            return this;
        }

        private void OnRightClick()
        {
            var menu = new ContextMenu();
            menu.AddButton("Copy text").Clicked += OnCopyText;
            _customContextualOptions?.Invoke(menu);
            menu.Show(Label, Label.PointFromScreen(Input.MouseScreenPosition));
        }

        private void OnCopyText()
        {
            Clipboard.Text = Label.Text;
        }

        /// <inheritdoc />
        public override Control Control => Label;
    }
}
