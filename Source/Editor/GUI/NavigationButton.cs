// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// A navigation bar button. Allows to change the current location and view the path.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Button" />
    [HideInEditor]
    public class NavigationButton : Button
    {
        /// <summary>
        /// The valid drag is over flag.
        /// </summary>
        protected bool _validDragOver;

        /// <summary>
        /// The default margin (horizontal).
        /// </summary>
        public const float DefaultMargin = 6.0f;

        /// <summary>
        /// Initializes a new instance of the <see cref="NavigationButton"/> class.
        /// </summary>
        /// <param name="x">The x position.</param>
        /// <param name="y">The y position.</param>
        /// <param name="height">The height.</param>
        public NavigationButton(float x, float y, float height)
        : base(x, y, 2 * DefaultMargin)
        {
            Height = height;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;
            var clientRect = new Rectangle(Float2.Zero, Size);

            // Draw background
            if (IsDragOver && _validDragOver)
            {
                Render2D.FillRectangle(clientRect, style.Selection);
                Render2D.DrawRectangle(clientRect, style.SelectionBorder);
            }
            else if (_isPressed)
            {
                Render2D.FillRectangle(clientRect, style.BackgroundSelected);
            }
            else if (IsMouseOver)
            {
                Render2D.FillRectangle(clientRect, style.BackgroundHighlighted);
            }

            // Draw text
            Render2D.DrawText(style.FontMedium, Text, clientRect, EnabledInHierarchy ? style.Foreground : style.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center);
        }

        /// <inheritdoc />
        public override void PerformLayout(bool force = false)
        {
            var style = Style.Current;
            if (style.FontMedium)
            {
                Width = style.FontMedium.MeasureText(Text).X + 2 * DefaultMargin;
            }
        }
    }
}
