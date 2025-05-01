// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Border control that draws the border around the control edges (inner and outer sides).
    /// </summary>
    [ActorToolbox("GUI")]
    public class Border : ContainerControl
    {
        /// <summary>
        /// Gets or sets the color used to draw border lines.
        /// </summary>
        [EditorOrder(0), Tooltip("The color used to draw border lines.")]
        public Color BorderColor { get; set; }

        /// <summary>
        /// The border lines width.
        /// </summary>
        [EditorOrder(10), Limit(0, float.MaxValue, 0.1f), Tooltip("The border lines width.")]
        public float BorderWidth { get; set; }

        /// <summary>
        /// Initializes a new instance of the <see cref="Border"/> class.
        /// </summary>
        public Border()
        {
            var style = Style.Current;
            BorderColor = style.BorderNormal;
            BorderWidth = 2.0f;
        }

        /// <inheritdoc />
        public override void DrawSelf()
        {
            base.DrawSelf();

            Render2D.DrawRectangle(new Rectangle(Float2.Zero, Size), BorderColor, BorderWidth);
        }

        /// <inheritdoc />
        public override bool ContainsPoint(ref Float2 location, bool precise = false)
        {
            if (precise) // Ignore border
                return false;
            return base.ContainsPoint(ref location, precise);
        }
    }
}
