// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// A ClickableImage
    /// </summary>
    [HideInEditor]
    public class ClickableImage : Image
    {

        /// <inheritdoc />
        public ClickableImage() : base(0, 0, 64, 64) {}

        /// <inheritdoc />
        public ClickableImage(float x, float y, float width, float height) : base(x, y, width, height) { }

        /// <inheritdoc />
        public ClickableImage(Vector2 location, Vector2 size) : base(location, size) { }

        /// <inheritdoc />
        public ClickableImage(Rectangle bounds) : base(bounds){}


        /// <summary>
        /// Gets or sets the color used to multiply the image pixels when mouse is over the image.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public Color MouseOverColor { get; set; } = Color.White;

        /// <summary>
        /// Gets or sets the color used to multiply the image pixels when control is disabled.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public Color DisabledTint { get; set; } = Color.Gray;

        /// <summary>
        /// Occurs when mouse clicks on the image.
        /// </summary>
        public event Action<Image, MouseButton> Clicked;

        /// <inheritdoc />
        public override Color GetColorToDraw()
        {
            var color = IsMouseOver ? MouseOverColor : base.GetColorToDraw();
            if (!Enabled)
                color *= DisabledTint;
            return color;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Vector2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
                return true;

            if (Clicked != null)
            {
                Clicked.Invoke(this, button);
                return true;
            }

            return false;
        }
    }
}
