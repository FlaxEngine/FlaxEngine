// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// The custom layout element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElementsContainer" />
    public class CustomElementsContainer<T> : LayoutElementsContainer
    where T : ContainerControl, new()
    {
        /// <summary>
        /// The custom control.
        /// </summary>
        public readonly T CustomControl = new T();

        /// <inheritdoc />
        public override ContainerControl ContainerControl => CustomControl;
    }

    /// <summary>
    /// The custom layout element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElement" />
    public class CustomElement<T> : LayoutElement
    where T : Control, new()
    {
        /// <summary>
        /// The custom control.
        /// </summary>
        public readonly T CustomControl = new T();

        /// <inheritdoc />
        public override Control Control => CustomControl;
    }
}
