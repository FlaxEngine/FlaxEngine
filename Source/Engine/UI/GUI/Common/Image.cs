// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// The basic GUI image control. Shows texture, sprite or render target.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [ActorToolbox("GUI")]
    public class Image : ContainerControl
    {
        /// <summary>
        /// Gets or sets the image source.
        /// </summary>
        [EditorOrder(10), Tooltip("The image to draw.")]
        public IBrush Brush { get; set; }

        /// <summary>
        /// Gets or sets the margin for the image.
        /// </summary>
        [EditorOrder(20), Tooltip("Margins of the image area.")]
        public Margin Margin { get; set; }

        /// <summary>
        /// Gets or sets the color used to multiply the image pixels.
        /// </summary>
        [EditorDisplay("Image Style"), EditorOrder(2010), ExpandGroups]
        public Color Color { get; set; } = Color.White;

        /// <summary>
        /// Gets or sets the color used to multiply the image pixels when mouse is over the image.
        /// </summary>
        [EditorDisplay("Image Style"), EditorOrder(2011)]
        public Color MouseOverColor { get; set; } = Color.White;

        /// <summary>
        /// Gets or sets the color used to multiply the image pixels when control is disabled.
        /// </summary>
        [EditorDisplay("Image Style"), EditorOrder(2012)]
        public Color DisabledTint { get; set; } = Color.Gray;

        /// <summary>
        /// Gets or sets a value indicating whether keep aspect ratio when drawing the image.
        /// </summary>
        [EditorOrder(60), Tooltip("If checked, control will keep aspect ratio of the image.")]
        public bool KeepAspectRatio { get; set; } = true;

        /// <summary>
        /// Occurs when mouse clicks on the image.
        /// </summary>
        public event Action<Image, MouseButton> Clicked;

        /// <inheritdoc />
        public Image()
        : base(0, 0, 64, 64)
        {
            AutoFocus = false;
        }

        /// <inheritdoc />
        public Image(float x, float y, float width, float height)
        : base(x, y, width, height)
        {
            AutoFocus = false;
        }

        /// <inheritdoc />
        public Image(Float2 location, Float2 size)
        : base(location, size)
        {
            AutoFocus = false;
        }

        /// <inheritdoc />
        public Image(Rectangle bounds)
        : base(bounds)
        {
            AutoFocus = false;
        }

        /// <inheritdoc />
        public override void DrawSelf()
        {
            base.DrawSelf();

            if (Brush == null)
                return;

            Rectangle rect;
            if (KeepAspectRatio)
            {
                // Figure out the ratio
                var size = Size;
                var imageSize = Brush.Size;
                if (imageSize.LengthSquared < 1)
                    return;
                var ratio = size / imageSize;
                var aspectRatio = ratio.MinValue;

                // Get the new height and width
                var newSize = imageSize * aspectRatio;

                // Calculate the X,Y position of the upper-left corner 
                // (one of these will always be zero)
                var newPos = (size - newSize) / 2;

                rect = new Rectangle(newPos, newSize);
            }
            else
            {
                rect = new Rectangle(Float2.Zero, Size);
            }

            Margin.ShrinkRectangle(ref rect);

            var color = IsMouseOver || IsNavFocused ? MouseOverColor : Color;
            if (!Enabled)
                color *= DisabledTint;
            Brush.Draw(rect, color);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
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

        /// <inheritdoc />
        public override void OnSubmit()
        {
            base.OnSubmit();

            // Execute default user interaction via mouse click
            Clicked?.Invoke(this, MouseButton.Left);
        }
    }
}
