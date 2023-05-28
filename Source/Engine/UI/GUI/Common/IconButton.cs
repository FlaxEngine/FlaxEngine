// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Button with an icon.
    /// </summary>
    public class IconButton : Button
    {
        /// <summary>
        /// The sprite rendered on the button.
        /// </summary>
        public SpriteHandle ButtonSprite { get; set; }

        /// <summary>
        /// Whether or not to hide the border of the button.
        /// </summary>
        public bool HideBorder = true;

        /// <summary>
        /// Initializes a new instance of the <see cref="IconButton"/> class.
        /// </summary>
        /// <param name="buttonSprite">The sprite used by the button.</param>
        public IconButton(SpriteHandle buttonSprite)
        : this(0, 0, buttonSprite)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="IconButton"/> class.
        /// </summary>
        /// <param name="x">Position X coordinate</param>
        /// <param name="y">Position Y coordinate</param>
        /// <param name="buttonSprite">The sprite used by the button.</param>
        /// <param name="width">Width</param>
        /// <param name="height">Height</param>
        /// <param name="hideBorder">Whether or not to hide the border.</param>
        public IconButton(float x, float y, SpriteHandle buttonSprite, float width = 120, float height = DefaultHeight, bool hideBorder = true)
        : base(x, y, width, height)
        {
            ButtonSprite = buttonSprite;
            BackgroundBrush = new SpriteBrush(ButtonSprite);
            HideBorder = hideBorder;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="IconButton"/> class.
        /// </summary>
        /// <param name="location">Position</param>
        /// <param name="size">Size</param>
        /// <param name="buttonSprite">The sprite used by the button.</param>
        public IconButton(Float2 location, Float2 size, SpriteHandle buttonSprite)
        : this(location.X, location.Y, buttonSprite, size.X, size.Y)
        {
        }

        /// <summary>
        /// Sets the colors of the button, taking into account the <see cref="HideBorder"/> field.>
        /// </summary>
        /// <param name="color">The color to use.</param>
        public override void SetColors(Color color)
        {
            BackgroundColor = color;
            BackgroundColorSelected = color.RGBMultiplied(0.8f);
            BackgroundColorHighlighted = color.RGBMultiplied(1.2f);

            BorderColor = HideBorder ? Color.Transparent : color.RGBMultiplied(0.5f);
            BorderColorSelected = BorderColor;
            BorderColorHighlighted = BorderColor;
        }
    }
}
