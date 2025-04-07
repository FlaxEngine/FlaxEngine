// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// A video media file proxy object.
    /// </summary>
    public class VideoProxy : ContentProxy
    {
        private readonly string _extension;

        internal VideoProxy(string extension)
        {
            _extension = extension;
        }

        /// <inheritdoc />
        public override string Name => "Video";

        /// <inheritdoc />
        public override string FileExtension => _extension;

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0x11f7f1);

        /// <inheritdoc />
        public override bool IsProxyFor(ContentItem item)
        {
            return item is VideoItem;
        }

        /// <inheritdoc />
        public override ContentItem ConstructItem(string path)
        {
            return new VideoItem(path);
        }

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new VideoWindow(editor, (VideoItem)item);
        }
    }
}
