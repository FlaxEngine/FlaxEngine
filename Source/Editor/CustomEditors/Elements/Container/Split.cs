// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// The split element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElementsContainer" />
    public class SplitElement<TElement1, TElement2> : LayoutElementsContainer
        where TElement1 : LayoutElement
        where TElement2 : LayoutElement
    {

        /// <summary>
        /// The split container.
        /// </summary>
        public readonly SplitContainer Split;

        /// <summary>
        /// The first control (left or upper based on Orientation).
        /// </summary>
        public readonly TElement1 Element1;

        /// <summary>
        /// The second control.
        /// </summary>
        public readonly TElement2 Element2;

        /// <summary>
        /// Initializes a new instance of the <see cref="SplitElement{TElement1, TElement2}"/> class.
        /// </summary>
        /// <param name="orientation">The orientation.</param>       
        public SplitElement(Orientation orientation) : base()
        {
            Element1 = Element<TElement1>();
            Element2 = Element<TElement2>();
            Split = new SplitContainer(Element1.Control, Element2.Control, orientation);
        }

        /// <inheritdoc />
        public override ContainerControl ContainerControl => Split;
    }


    /// <summary>
    /// The split element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElementsContainer" />
    public class SplitPanelElement : SplitElement<VerticalPanelElement, VerticalPanelElement>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="SplitPanelElement"/> class.
        /// </summary>
        /// <param name="orientation">The orientation.</param>       
        public SplitPanelElement(Orientation orientation = Orientation.Horizontal) : base(orientation)
        {
        }
    }
}
