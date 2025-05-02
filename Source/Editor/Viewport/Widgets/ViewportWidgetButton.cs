// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Viewport.Widgets
{
    /// <summary>
    /// Viewport Widget Button class.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    [HideInEditor]
    public class ViewportWidgetButton : Control
    {
        private string _text;
        private ContextMenu _cm;
        private bool _checked;
        private bool _autoCheck;
        private bool _isMosueDown;
        private float _forcedTextWidth;

        /// <summary>
        /// Event fired when user toggles checked state.
        /// </summary>
        public event Action<ViewportWidgetButton> Toggled;

        /// <summary>
        /// Event fired when user click the button.
        /// </summary>
        public event Action<ViewportWidgetButton> Clicked;

        /// <summary>
        /// The icon.
        /// </summary>
        public SpriteHandle Icon;

        /// <summary>
        /// Gets or sets the text.
        /// </summary>
        public string Text
        {
            get => _text;
            set
            {
                _text = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether this <see cref="ViewportWidgetButton"/> is checked.
        /// </summary>
        public bool Checked
        {
            get => _checked;
            set => _checked = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ViewportWidgetButton"/> class.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="icon">The icon.</param>
        /// <param name="contextMenu">The context menu.</param>
        /// <param name="autoCheck">If set to <c>true</c> will be automatic checked on mouse click.</param>
        /// <param name="textWidth">Forces the text to be drawn with the specified width.</param>
        public ViewportWidgetButton(string text, SpriteHandle icon, ContextMenu contextMenu = null, bool autoCheck = false, float textWidth = 0.0f)
        : base(0, 0, CalculateButtonWidth(textWidth, icon.IsValid), ViewportWidgetsContainer.WidgetsHeight)
        {
            _text = text;
            Icon = icon;
            _cm = contextMenu;
            _autoCheck = autoCheck;
            _forcedTextWidth = textWidth;

            if (_cm != null)
                _cm.VisibleChanged += CmOnVisibleChanged;
        }

        private void CmOnVisibleChanged(Control control)
        {
            if (_cm != null && !_cm.IsOpened)
            {
                if (HasParent && Parent.HasParent)
                {
                    // Focus viewport
                    Parent.Parent.Focus();
                }
            }
        }

        private static float CalculateButtonWidth(float textWidth, bool hasIcon)
        {
            return (hasIcon ? ViewportWidgetsContainer.WidgetsIconSize : 0) + (textWidth > 0 ? textWidth + 8 : 0);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            // Cache data
            var style = Style.Current;
            const float iconSize = ViewportWidgetsContainer.WidgetsIconSize;
            var iconRect = new Rectangle(0, (Height - iconSize) / 2, iconSize, iconSize);
            var textRect = new Rectangle(0, 0, Width + 1, Height + 1);

            // Check if is checked or mouse is over and auto check feature is enabled
            if (_checked)
                Render2D.FillRectangle(textRect, style.BackgroundSelected * (IsMouseOver ? 0.9f : 0.6f));
            else if (_autoCheck && IsMouseOver)
                Render2D.FillRectangle(textRect, style.BackgroundHighlighted);

            // Check if has icon
            if (Icon.IsValid)
            {
                // Draw icon
                Render2D.DrawSprite(Icon, iconRect, style.ForegroundViewport);

                // Update text rectangle
                textRect.Location.X += iconSize;
                textRect.Size.X -= iconSize;
            }

            // Draw text
            Render2D.DrawText(style.FontMedium, _text, textRect, style.ForegroundViewport * (IsMouseOver ? 1.0f : 0.9f), TextAlignment.Center, TextAlignment.Center);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                _isMosueDown = true;
            }
            if (_autoCheck)
            {
                // Toggle
                Checked = !_checked;
                Toggled?.Invoke(this);
            }

            _cm?.Show(this, new Float2(-1, Height + 2));

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _isMosueDown)
            {
                _isMosueDown = false;
                Clicked?.Invoke(this);
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void PerformLayout(bool force = false)
        {
            var style = Style.Current;

            if (style != null && style.FontMedium)
                Width = CalculateButtonWidth(_forcedTextWidth > 0.0f ? _forcedTextWidth : style.FontMedium.MeasureText(_text).X, Icon.IsValid);
        }
    }
}
