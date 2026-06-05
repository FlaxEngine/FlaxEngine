// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content.Thumbnails;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// A <see cref="MaterialInstance"/> asset proxy object.
    /// </summary>
    [ContentContextMenu("New/Material/Material Instance")]
    public class MaterialInstanceProxy : MaterialBaseProxy
    {
        /// <inheritdoc />
        public override string Name => "Material Instance";

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new MaterialInstanceWindow(editor, item as AssetItem);
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0x2c3e50);

        /// <inheritdoc />
        public override Type AssetType => typeof(MaterialInstance);

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            if (Editor.CreateAsset("MaterialInstance", outputPath))
                throw new Exception("Failed to create new asset.");
        }

        /// <inheritdoc />
        public override bool CanDrawThumbnail(ThumbnailRequest request)
        {
            return _preview.HasLoadedAssets && ThumbnailsModule.HasMinimumQuality((MaterialInstance)request.Asset);
        }
    }
}
