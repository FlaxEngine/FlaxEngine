// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.ContextMenu
{
    /// <summary>
    /// Context Menu button control.
    /// </summary>
    /// <seealso cref="ContextMenuItem" />
    [HideInEditor]
    public class ContextMenuButton : ContextMenuItem
    {
        private bool _isMouseDown;
        
        /// <summary>
        /// The amount to adjust the short keys and arrow image by in x coordinates.
        /// </summary>
        public float ExtraAdjustmentAmount = 0;

        /// <summary>
        /// Event fired when user clicks on the button.
        /// </summary>
        public event Action Clicked;

        /// <summary>
        /// Event fired when user clicks on the button.
        /// </summary>
        public event Action<ContextMenuButton> ButtonClicked;

        /// <summary>
        /// The button text.
        /// </summary>
        public string Text;

        /// <summary>
        /// The button short keys information (eg. 'Ctrl+C').
        /// </summary>
        public string ShortKeys;

        /// <summary>
        /// Item icon (best is 16x16).
        /// </summary>
        public SpriteHandle Icon;

        /// <summary>
        /// The checked state.
        /// </summary>
        public bool Checked;

        /// <summary>
        /// The automatic check mode.
        /// </summary>
        public bool AutoCheck;

        /// <summary>
        /// Closes the context menu after clicking the button, otherwise menu will stay open.
        /// </summary>
        public bool CloseMenuOnClick = true;

        /// <summary>
        /// Initializes a new instance of the <see cref="ContextMenuButton"/> class.
        /// </summary>
        /// <param name="parent">The parent context menu.</param>
        /// <param name="text">The text.</param>
        /// <param name="shortKeys">The short keys tip.</param>
        public ContextMenuButton(ContextMenu parent, string text, string shortKeys = "")
        : base(parent, 8, 22)
        {
            Text = text;
            ShortKeys = shortKeys;
        }

        /// <summary>
        /// Sets the automatic check mode. In auto check mode the button sets the check sprite as an icon when user clicks it.
        /// </summary>
        /// <param name="value">True if use auto check, otherwise false.</param>
        /// <returns>This button.</returns>
        public ContextMenuButton SetAutoCheck(bool value)
        {
            AutoCheck = value;
            return this;
        }

        /// <summary>
        /// Sets the checked state.
        /// </summary>
        /// <param name="value">True if check it, otherwise false.</param>
        /// <returns>This button.</returns>
        public ContextMenuButton SetChecked(bool value)
        {
            Checked = value;
            return this;
        }

        /// <summary>
        /// Clicks this button.
        /// </summary>
        public void Click()
        {
            if (CloseMenuOnClick)
            {
                // Close topmost context menu
                ParentContextMenu?.TopmostCM.Hide();
            }

            // Auto check logic
            if (AutoCheck)
                Checked = !Checked;

            // Fire event
            Clicked?.Invoke();
            ButtonClicked?.Invoke(this);
            ParentContextMenu?.OnButtonClicked(this);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;
            var backgroundRect = new Rectangle(-X + 3, 0, Parent.Width - 6, Height);
            var textRect = new Rectangle(0, 0, Width - 8, Height);
            var textColor = Enabled ? style.Foreground : style.ForegroundDisabled;

            // Draw background
            if (IsMouseOver && Enabled)
                Render2D.FillRectangle(backgroundRect, style.LightBackground);
            else if (IsFocused)
                Render2D.FillRectangle(backgroundRect, style.LightBackground);

            base.Draw();

            // Draw text
            Render2D.DrawText(style.FontMedium, Text, textRect, textColor, TextAlignment.Near, TextAlignment.Center);

            if (!string.IsNullOrEmpty(ShortKeys))
            {
                // Draw short keys
                Render2D.DrawText(style.FontMedium, ShortKeys, new Rectangle(textRect.X + ExtraAdjustmentAmount, textRect.Y, textRect.Width, textRect.Height), textColor, TextAlignment.Far, TextAlignment.Center);
            }

            // Draw icon
            const float iconSize = 14;
            var icon = Checked ? style.CheckBoxTick : Icon;
            if (icon.IsValid)
                Render2D.DrawSprite(icon, new Rectangle(-iconSize - 1, (Height - iconSize) / 2, iconSize, iconSize), textColor);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            _isMouseDown = false;

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            _isMouseDown = true;
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
                return true;

            if (_isMouseDown)
            {
                _isMouseDown = false;
                Click();
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
                return true;

            switch (key)
            {
            case KeyboardKeys.ArrowUp:
                for (int i = IndexInParent - 1; i >= 0; i--)
                {
                    if (ParentContextMenu.ItemsContainer.Children[i] is ContextMenuButton item && item.Visible && item.Enabled)
                    {
                        item.Focus();
                        ParentContextMenu.ItemsContainer.ScrollViewTo(item);
                        return true;
                    }
                }
                break;
            case KeyboardKeys.ArrowDown:
                for (int i = IndexInParent + 1; i < ParentContextMenu.ItemsContainer.Children.Count; i++)
                {
                    if (ParentContextMenu.ItemsContainer.Children[i] is ContextMenuButton item && item.Visible && item.Enabled)
                    {
                        item.Focus();
                        ParentContextMenu.ItemsContainer.ScrollViewTo(item);
                        return true;
                    }
                }
                break;
            case KeyboardKeys.Return:
                Click();
                return true;
            case KeyboardKeys.Escape:
                ParentContextMenu.Hide();
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            _isMouseDown = false;

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override float MinimumWidth
        {
            get
            {
                var style = Style.Current;
                float width = 20;
                if (style.FontMedium)
                {
                    width += style.FontMedium.MeasureText(Text).X;
                    if (!string.IsNullOrEmpty(ShortKeys))
                        width += 40 + style.FontMedium.MeasureText(ShortKeys).X;
                }

                return Mathf.Max(width, base.MinimumWidth);
            }
        }
    }
}
