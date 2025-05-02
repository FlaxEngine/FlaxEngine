// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Status strip GUI control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public class StatusBar : ContainerControl
    {
        /// <summary>
        /// The default height.
        /// </summary>
        public const int DefaultHeight = 22;

        /// <summary>
        /// Gets or sets the color of the status strip.
        /// </summary>
        public Color StatusColor
        {
            get => BackgroundColor;
            set => BackgroundColor = value;
        }

        /// <summary>
        /// Gets or sets the status text.
        /// </summary>
        public string Text { get; set; }

        /// <summary>
        /// Gets or sets the status text color
        /// </summary>
        public Color TextColor { get; set; } = Style.Current.Foreground;

        /// <summary>
        /// Initializes a new instance of the <see cref="StatusBar"/> class.
        /// </summary>
        public StatusBar()
        {
            AutoFocus = false;
            AnchorPreset = AnchorPresets.HorizontalStretchBottom;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            var style = Style.Current;

            // Draw size grip
            if (Root is WindowRootControl window && !window.IsMaximized)
                Render2D.DrawSprite(style.StatusBarSizeGrip, new Rectangle(Width - 12, 10, 12, 12), style.Foreground);

            // Draw status text
            Render2D.DrawText(style.FontSmall, Text, new Rectangle(4, 0, Width - 20, Height), TextColor, TextAlignment.Near, TextAlignment.Center);
        }
    }
}
