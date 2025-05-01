// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// <see cref="LocalizedStringTable"/> proxy.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.JsonAssetProxy" />
    public class LocalizedStringTableProxy : JsonAssetProxy
    {
        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new LocalizedStringTableWindow(editor, (JsonAssetItem)item);
        }

        /// <inheritdoc />
        public override string TypeName => "FlaxEngine.LocalizedStringTable";
    }
}
