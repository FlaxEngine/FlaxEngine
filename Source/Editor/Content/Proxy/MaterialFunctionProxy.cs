// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// A <see cref="MaterialFunction"/> asset proxy object.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetProxy" />
    [ContentContextMenu("New/Material/Material Function")]
    public class MaterialFunctionProxy : BinaryAssetProxy
    {
        /// <inheritdoc />
        public override string Name => "Material Function";

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new MaterialFunctionWindow(editor, item as AssetItem);
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0x6da016);

        /// <inheritdoc />
        public override Type AssetType => typeof(MaterialFunction);

        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            return targetLocation.CanHaveAssets;
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            if (Editor.CreateAsset("MaterialFunction", outputPath))
                throw new Exception("Failed to create new asset.");
        }
    }
}
