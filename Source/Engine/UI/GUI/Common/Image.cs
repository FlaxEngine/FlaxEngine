// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// The basic GUI image control. Shows texture, sprite or render target.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Colorable" />
    public class Image : Colorable
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
        /// Gets or sets a value indicating whether keep aspect ratio when drawing the image.
        /// </summary>
        [EditorOrder(60), Tooltip("If checked, control will keep aspect ratio of the image.")]
        public bool KeepAspectRatio { get; set; } = true;

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
        public Image(Vector2 location, Vector2 size)
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
                rect = new Rectangle(Vector2.Zero, Size);
            }

            Margin.ShrinkRectangle(ref rect);
                        
            Brush.Draw(rect, GetColorToDraw());
        }

    }
}
