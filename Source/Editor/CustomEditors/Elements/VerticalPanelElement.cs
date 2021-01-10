// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// The vertical panel element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElement" />
    public class VerticalPanelElement : LayoutElementsContainer
    {
        /// <summary>
        /// The panel.
        /// </summary>
        public readonly VerticalPanel Panel = new VerticalPanel();

        /// <inheritdoc />
        public override ContainerControl ContainerControl => Panel;
    }
}
