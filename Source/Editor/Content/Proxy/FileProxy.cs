// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEditor.Windows;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// File proxy object.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.ContentProxy" />
    public class FileProxy : ContentProxy
    {
        /// <inheritdoc />
        public override string Name => "File";

        /// <inheritdoc />
        public override bool IsProxyFor(ContentItem item)
        {
            return item is FileItem;
        }

        /// <inheritdoc />
        public override string FileExtension => string.Empty;

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return null;
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0x441c9c);
    }
}
