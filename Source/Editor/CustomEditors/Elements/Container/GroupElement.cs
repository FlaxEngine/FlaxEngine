// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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
            ItemsMargin = new Margin(7, 7, 3, 3),
            HeaderHeight = 20,
            EnableContainmentLines = true,
        };

        /// <inheritdoc />
        public override ContainerControl ContainerControl => Panel;
    }
}
