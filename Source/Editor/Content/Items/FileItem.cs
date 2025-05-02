// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Content item for the auxiliary files.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.ContentItem" />
    public class FileItem : ContentItem
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="FileItem"/> class.
        /// </summary>
        /// <param name="path">The path to the file.</param>
        public FileItem(string path)
        : base(path)
        {
        }

        /// <inheritdoc />
        public override ContentItemType ItemType => ContentItemType.Other;

        /// <inheritdoc />
        public override ContentItemSearchFilter SearchFilter => ContentItemSearchFilter.Other;

        /// <inheritdoc />
        public override string TypeDescription => "File";

        /// <inheritdoc />
        public override SpriteHandle DefaultThumbnail => Editor.Instance.Icons.Document128;
    }
}
