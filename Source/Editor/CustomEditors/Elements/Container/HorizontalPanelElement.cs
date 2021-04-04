// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// The horizontal panel element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElement" />
    public class HorizontalPanelElement : LayoutElementsContainer
    {
        /// <summary>
        /// The panel.
        /// </summary>
        public readonly HorizontalPanel Panel = new HorizontalPanel();

        /// <inheritdoc />
        public override ContainerControl ContainerControl => Panel;
    }
}
