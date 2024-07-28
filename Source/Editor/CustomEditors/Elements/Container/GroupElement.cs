// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// The layout group element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElement" />
    public class GroupElement : LayoutElementsContainer
    {
        /// <summary>
        /// The drop panel.
        /// </summary>
        public readonly DropPanel Panel = new DropPanel
        {
            ArrowImageClosed = new SpriteBrush(Style.Current.ArrowRight),
            ArrowImageOpened = new SpriteBrush(Style.Current.ArrowDown),
            EnableDropDownIcon = true,
            ItemsMargin = new Margin(7),
            HeaderHeight = 18.0f,
            EnableContainmentLines = true,
        };

        /// <inheritdoc />
        public override ContainerControl ContainerControl => Panel;

        /// <summary>
        /// Adds utility settings button to the group header.
        /// </summary>
        /// <returns>The created control.</returns>
        public Image AddSettingsButton()
        {
            var style = Style.Current;
            var settingsButtonSize = Panel.HeaderHeight;
            return new Image
            {
                TooltipText = "Settings",
                AutoFocus = true,
                AnchorPreset = AnchorPresets.TopRight,
                Parent = Panel,
                Bounds = new Rectangle(Panel.Width - settingsButtonSize, 0, settingsButtonSize, settingsButtonSize),
                IsScrollable = false,
                Color = style.ForegroundGrey,
                Margin = new Margin(1),
                Brush = new SpriteBrush(style.Settings),
            };
        }
    }
}
