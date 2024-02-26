// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// The image element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElement" />
    public class ImageElement : LayoutElement
    {
        /// <summary>
        /// The image.
        /// </summary>
        public readonly Image Image = new Image();

        /// <inheritdoc />
        public override Control Control => Image;
    }
}
