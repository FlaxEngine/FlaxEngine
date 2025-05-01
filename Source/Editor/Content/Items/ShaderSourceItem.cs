// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Content item that contains shader source code.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.ContentItem" />
    public class ShaderSourceItem : ContentItem
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ShaderSourceItem"/> class.
        /// </summary>
        /// <param name="path">The path to the item.</param>
        public ShaderSourceItem(string path)
        : base(path)
        {
            ShowFileExtension = true;
        }

        /// <inheritdoc />
        public override ContentItemType ItemType => ContentItemType.Asset;

        /// <inheritdoc />
        public override ContentItemSearchFilter SearchFilter => ContentItemSearchFilter.Shader;

        /// <inheritdoc />
        public override string TypeDescription => "Shader Source Code";

        /// <inheritdoc />
        public override SpriteHandle DefaultThumbnail => Editor.Instance.Icons.Document128;
    }
}
