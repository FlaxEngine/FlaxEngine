// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content.Thumbnails;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// A <see cref="Material"/> asset proxy object.
    /// </summary>
    [ContentContextMenu("New/Material/Material")]
    public class MaterialProxy : MaterialBaseProxy
    {
        /// <inheritdoc />
        public override string Name => "Material";

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new MaterialWindow(editor, item as AssetItem);
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0x16a085);

        /// <inheritdoc />
        public override Type AssetType => typeof(Material);

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            if (Editor.CreateAsset("Material", outputPath))
                throw new Exception("Failed to create new asset.");
        }

        /// <inheritdoc />
        public override bool CanDrawThumbnail(ThumbnailRequest request)
        {
            return _preview.HasLoadedAssets && ThumbnailsModule.HasMinimumQuality((Material)request.Asset);
        }
    }
}
