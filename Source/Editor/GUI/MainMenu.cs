// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Menu strip with child buttons.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public sealed class MainMenu : ContainerControl
    {
        private MainMenuButton _selected;

        /// <summary>
        /// Gets or sets the selected button (with opened context menu).
        /// </summary>
        public MainMenuButton Selected
        {
            get => _selected;
            set
            {
                if (_selected == value)
                    return;

                if (_selected != null)
                {
                    _selected.ContextMenu.VisibleChanged -= OnSelectedContextMenuVisibleChanged;
                    _selected.ContextMenu.Hide();
                }

                _selected = value;

                if (_selected != null && _selected.ContextMenu.HasChildren)
                {
                    _selected.ContextMenu.Show(_selected, new Float2(0, _selected.Height));
                    _selected.ContextMenu.VisibleChanged += OnSelectedContextMenuVisibleChanged;
                }
            }
        }

        private void OnSelectedContextMenuVisibleChanged(Control control)
        {
            if (_selected != null)
                Selected = null;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="MainMenu"/> class.
        /// </summary>
        public MainMenu()
        : base(0, 0, 0, 20)
        {
            AutoFocus = false;
            AnchorPreset = AnchorPresets.HorizontalStretchTop;
            BackgroundColor = Style.Current.LightBackground;
        }

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The button text.</param>
        /// <returns>Created button control.</returns>
        public MainMenuButton AddButton(string text)
        {
            return AddChild(new MainMenuButton(text));
        }

        /// <summary>
        /// Gets or adds a button.
        /// </summary>
        /// <param name="text">The button text</param>
        /// <returns>The existing or created button control.</returns>
        public MainMenuButton GetOrAddButton(string text)
        {
            MainMenuButton result = GetButton(text);
            if (result == null)
                result = AddButton(text);
            return result;
        }

        /// <summary>
        /// Gets the button.
        /// </summary>
        /// <param name="text">The button text.</param>
        /// <returns>The button or null if missing.</returns>
        public MainMenuButton GetButton(string text)
        {
            MainMenuButton result = null;
            for (int i = 0; i < Children.Count; i++)
            {
                if (Children[i] is MainMenuButton button && string.Equals(button.Text, text, StringComparison.OrdinalIgnoreCase))
                {
                    result = button;
                    break;
                }
            }
            return result;
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
                return true;

            // Fallback to the edit window for shortcuts
            var editor = Editor.Instance;
            return editor.Windows.EditWin.InputActions.Process(editor, this, key);
        }

        /// <inheritdoc />
        protected override void PerformLayoutAfterChildren()
        {
            float x = 0;
            WindowDecorations decorations = Parent.GetChild<WindowDecorations>();
            x += decorations?.Icon?.Width ?? 0;

            // Arrange controls
            MainMenuButton rightMostButton = null;
            for (int i = 0; i < _children.Count; i++)
            {
                var c = _children[i];
                if (c is MainMenuButton b && c.Visible)
                {
                    b.Bounds = new Rectangle(x, 0, b.Width, Height);

                    if (rightMostButton == null)
                        rightMostButton = b;
                    else if (rightMostButton.X < b.X)
                        rightMostButton = b;

                    x += b.Width;
                }
            }
            
            // Fill the right side if title and buttons are not present
            if (decorations?.Title == null)
                Width = Parent.Width;
            else
                Width = x;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            base.OnDestroy();
            
            if (_selected != null)
                Selected = null;
        }
    }
}
