// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using FlaxEditor.Content.Thumbnails;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Content
{
    /// <summary>
    /// A <see cref="BehaviorTree"/> asset proxy object.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetProxy" />
    [ContentContextMenu("New/AI/Behavior Tree")]
    public class BehaviorTreeProxy : BinaryAssetProxy
    {
        /// <inheritdoc />
        public override string Name => "Behavior Tree";

        /// <inheritdoc />
        public override bool CanReimport(ContentItem item)
        {
            return true;
        }

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new BehaviorTreeWindow(editor, item as BinaryAssetItem);
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0x3256A8);

        /// <inheritdoc />
        public override Type AssetType => typeof(BehaviorTree);

        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            return targetLocation.CanHaveAssets;
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            if (Editor.CreateAsset("BehaviorTree", outputPath))
                throw new Exception("Failed to create new asset.");
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawBegin(ThumbnailRequest request, ContainerControl guiRoot, GPUContext context)
        {
            guiRoot.AddChild(new Label
            {
                Text = Path.GetFileNameWithoutExtension(request.Asset.Path),
                Offsets = Margin.Zero,
                AnchorPreset = AnchorPresets.StretchAll,
                Wrapping = TextWrapping.WrapWords
            });
        }
    }
}
