// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Windows;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Tool strip with child items.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public class ToolStrip : ContainerControl
    {
        /// <summary>
        /// The default margin vertically.
        /// </summary>
        public const int DefaultMarginV = 1;

        /// <summary>
        /// The default margin horizontally.
        /// </summary>
        public const int DefaultMarginH = 2;

        /// <summary>
        /// Event fired when button gets clicked with the primary mouse button.
        /// </summary>
        public Action<ToolStripButton> ButtonClicked;

        /// <summary>
        /// Event fired when button gets clicked with the secondary mouse button.
        /// </summary>
        public Action<ToolStripButton> SecondaryButtonClicked;

        /// <summary>
        /// Tries to get the last button.
        /// </summary>
        public ToolStripButton LastButton
        {
            get
            {
                for (int i = _children.Count - 1; i >= 0; i--)
                {
                    if (_children[i] is ToolStripButton button)
                        return button;
                }
                return null;
            }
        }

        /// <summary>
        /// Gets amount of buttons that has been added
        /// </summary>
        public int ButtonsCount
        {
            get
            {
                int result = 0;
                for (int i = 0; i < _children.Count; i++)
                {
                    if (_children[i] is ToolStripButton)
                        result++;
                }
                return result;
            }
        }

        /// <summary>
        /// Gets the height for the items.
        /// </summary>
        public float ItemsHeight => Height - 2 * DefaultMarginV;

        /// <summary>
        /// Initializes a new instance of the <see cref="ToolStrip"/> class.
        /// </summary>
        /// <param name="height">The toolstrip height.</param>
        /// <param name="y">The toolstrip Y position.</param>
        public ToolStrip(float height = 32.0f, float y = 0)
        {
            AutoFocus = false;
            AnchorPreset = AnchorPresets.HorizontalStretchTop;
            BackgroundColor = Style.Current.LightBackground;
            Offsets = new Margin(0, 0, y, height * Editor.Instance.Options.Options.Interface.IconsScale);
        }

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="sprite">The icon sprite.</param>
        /// <param name="onClick">The custom action to call on button clicked.</param>
        /// <returns>The button.</returns>
        public ToolStripButton AddButton(SpriteHandle sprite, Action onClick = null)
        {
            var button = new ToolStripButton(ItemsHeight, ref sprite)
            {
                Parent = this,
            };
            if (onClick != null)
                button.Clicked += onClick;
            return button;
        }

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="sprite">The icon sprite.</param>
        /// <param name="text">The text.</param>
        /// <param name="onClick">The custom action to call on button clicked.</param>
        /// <returns>The button.</returns>
        public ToolStripButton AddButton(SpriteHandle sprite, string text, Action onClick = null)
        {
            var button = new ToolStripButton(ItemsHeight, ref sprite)
            {
                Text = text,
                Parent = this,
            };
            if (onClick != null)
                button.Clicked += onClick;
            return button;
        }

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="onClick">The custom action to call on button clicked.</param>
        /// <returns>The button.</returns>
        public ToolStripButton AddButton(string text, Action onClick = null)
        {
            var button = new ToolStripButton(ItemsHeight, ref SpriteHandle.Invalid)
            {
                Text = text,
                Parent = this,
            };
            if (onClick != null)
                button.Clicked += onClick;
            return button;
        }

        /// <summary>
        /// Adds the separator.
        /// </summary>
        /// <returns>The separator.</returns>
        public ToolStripSeparator AddSeparator()
        {
            return AddChild(new ToolStripSeparator(ItemsHeight));
        }

        internal void OnButtonClicked(ToolStripButton button)
        {
            ButtonClicked?.Invoke(button);
        }

        internal void OnSecondaryButtonClicked(ToolStripButton button)
        {
            SecondaryButtonClicked?.Invoke(button);
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            // Arrange controls
            float x = DefaultMarginH;
            float h = ItemsHeight;
            for (int i = 0; i < _children.Count; i++)
            {
                var c = _children[i];
                if (c.Visible)
                {
                    var w = c.Width;
                    c.Bounds = new Rectangle(x, DefaultMarginV, w, h);
                    x += w + DefaultMarginH;
                }
            }
        }

        /// <inheritdoc />
        public override void OnChildResized(Control control)
        {
            base.OnChildResized(control);

            PerformLayout();
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
                return true;

            // Fallback to the owning window for shortcuts
            EditorWindow editorWindow = null;
            ContainerControl c = Parent;
            while (c != null && editorWindow == null)
            {
                editorWindow = c as EditorWindow;
                c = c.Parent;
            }
            var editor = Editor.Instance;
            if (editorWindow == null)
                editorWindow = editor.Windows.EditWin; // Fallback to main editor window
            return editorWindow.InputActions.Process(editor, this, key);
        }
    }
}
