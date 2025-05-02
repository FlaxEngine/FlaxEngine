// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Windows;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Editor control for the UI style.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    [HideInEditor]
    public class StyleValueEditor : Control
    {
        /// <summary>
        /// Delegate function used for the change events handling.
        /// </summary>
        /// <param name="value">The selected value.</param>
        /// <param name="sliding">True if user is using a slider, otherwise false.</param>
        public delegate void ValueChangedEvent(Style value, bool sliding);

        /// <summary>
        /// True if slider is in use.
        /// </summary>
        protected bool _isSliding;

        /// <summary>
        /// The style.
        /// </summary>
        protected Style _value;

        /// <summary>
        /// The style
        /// </summary>
        public Style Value
        {
            get => _value;
            set
            {
                if (_value == null || _value != value)
                {
                    _value = value;

                    // Fire event
                    ValueChanged?.Invoke();
                }
            }
        }

        /// <summary>
        /// Fires when the style changes
        /// </summary>
        public event Action ValueChanged;

        /// <summary>
        /// Initializes a new instance of the <see cref="StyleValueEditor"/> class.
        /// </summary>
        public StyleValueEditor()
        : base(0, 0, 32, 18)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="StyleValueEditor"/> class.
        /// </summary>
        /// <param name="value">The initial value.</param>
        /// <param name="x">The x location</param>
        /// <param name="y">The y location</param>
        public StyleValueEditor(Style value, float x, float y)
        : base(x, y, 32, 18)
        {
            _value = value;
        }

        private void OnValueChanged(Style style, bool sliding)
        {
            // Force send ValueChanged event
            if (_isSliding != sliding)
            {
                _value = null;
            }

            _isSliding = sliding;
            Value = style;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            var style = Style.Current;
            var r = new Rectangle(2, 2, Width - 4, Height - 4);

            if (Value != null)
            {
                // Draw the style colors
                /*
                -Background
                -LightBackground
                -DragWindow
                -Foreground
                -ForegroundDisabled
                -BackgroundHighlighted
                -BorderHighlighted
                -BackgroundSelected
                -BorderSelected
                -BackgroundNormal
                -BorderNormal
                -TextBoxBackground
                -TextBoxBackgroundSelected
                -ProgressNormal
                */

                var size = new Float2(r.Width / 7f, r.Height / 2f);
                float x = 2;

                Render2D.FillRectangle(new Rectangle(x, 2, size), Value.Foreground);
                x += size.X;
                Render2D.FillRectangle(new Rectangle(x, 2, size), Value.ForegroundDisabled);
                x += size.X;
                Render2D.FillRectangle(new Rectangle(x, 2, size), Value.DragWindow);
                x += size.X;
                Render2D.FillRectangle(new Rectangle(x, 2, size), Value.BorderNormal);
                x += size.X;
                Render2D.FillRectangle(new Rectangle(x, 2, size), Value.BorderHighlighted);
                x += size.X;
                Render2D.FillRectangle(new Rectangle(x, 2, size), Value.BorderSelected);
                x += size.X;
                Render2D.FillRectangle(new Rectangle(x, 2, size), Value.ProgressNormal);
                x = 2;

                Render2D.FillRectangle(new Rectangle(x, 2 + size.Y, size), Value.Background);
                x += size.X;
                Render2D.FillRectangle(new Rectangle(x, 2 + size.Y, size), Value.BackgroundNormal);
                x += size.X;
                Render2D.FillRectangle(new Rectangle(x, 2 + size.Y, size), Value.LightBackground);
                x += size.X;
                Render2D.FillRectangle(new Rectangle(x, 2 + size.Y, size), Value.BackgroundHighlighted);
                x += size.X;
                Render2D.FillRectangle(new Rectangle(x, 2 + size.Y, size), Value.BackgroundSelected);
                x += size.X;
                Render2D.FillRectangle(new Rectangle(x, 2 + size.Y, size), Value.TextBoxBackground);
                x += size.X;
                Render2D.FillRectangle(new Rectangle(x, 2 + size.Y, size), Value.TextBoxBackgroundSelected);
            }
            else
            {
                Render2D.FillRectangle(r, Style.Current.BackgroundNormal);
            }

            Render2D.DrawRectangle(r, IsMouseOver ? style.BackgroundSelected : style.Background);

            if (_value == null)
            {
                Rectangle textRectangle = r;
                textRectangle.X = 4;
                Render2D.DrawText(style.FontMedium, "No Style", textRectangle, style.Foreground);
            }
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (Value == null)
            {
                Value = Editor.Instance.Options.CreateDefaultStyle();
            }

            var editorWindow = new StyleEditorWindow(Editor.Instance, this.Value, OnValueChanged, true);
            editorWindow.Show();

            return base.OnMouseUp(location, button);
        }
    }
}
