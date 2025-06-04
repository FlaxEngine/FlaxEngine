// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI.ContextMenu;
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
            Pivot = Float2.Zero,
            ArrowImageClosed = new SpriteBrush(Style.Current.ArrowRight),
            ArrowImageOpened = new SpriteBrush(Style.Current.ArrowDown),
            EnableDropDownIcon = true,
            ItemsMargin = new Margin(Utilities.Constants.UIMargin),
            ItemsSpacing = Utilities.Constants.UIMargin,
            HeaderHeight = 18.0f,
            EnableContainmentLines = true,
        };

        /// <summary>
        /// Event is fired if the group can setup a context menu and the context menu is being setup.
        /// </summary>
        public Action<ContextMenu, DropPanel> SetupContextMenu;

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
